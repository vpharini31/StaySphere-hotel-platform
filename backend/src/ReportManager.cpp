#include "Managers.h"
#include "Utils.h"

ReportManager::ReportManager(std::shared_ptr<Database> db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Revenue report for a date range
// ─────────────────────────────────────────────────────────────────────────────
RevenueReport ReportManager::getRevenueReport(int hotel_id,
                                               const std::string& from,
                                               const std::string& to) {
    std::string hid = std::to_string(hotel_id);
    RevenueReport r;
    r.period = from + " to " + to;

    auto rows = db_->query(R"sql(
        SELECT
            COALESCE(SUM(total_amount),   0) AS total,
            COALESCE(SUM(room_cost),      0) AS room,
            COALESCE(SUM(activity_cost),  0) AS activity,
            COALESCE(SUM(breakfast_cost), 0) AS breakfast,
            COUNT(*)                          AS bookings,
            COALESCE(AVG(nights),         0) AS avg_nights
        FROM bookings
        WHERE hotel_id = ?
          AND status   != 'cancelled'
          AND check_in_date >= ?
          AND check_in_date <= ?;
    )sql", {hid, from, to});

    if (!rows.empty()) {
        for (auto& [k, v] : rows[0]) {
            if      (k == "total")      r.total_revenue     = v.empty() ? 0 : std::stod(v);
            else if (k == "room")       r.room_revenue      = v.empty() ? 0 : std::stod(v);
            else if (k == "activity")   r.activity_revenue  = v.empty() ? 0 : std::stod(v);
            else if (k == "breakfast")  r.breakfast_revenue = v.empty() ? 0 : std::stod(v);
            else if (k == "bookings")   r.total_bookings    = v.empty() ? 0 : std::stoi(v);
            else if (k == "avg_nights") r.avg_stay_nights   = v.empty() ? 0 : std::stod(v);
        }
    }

    r.cancellations = db_->queryInt(
        "SELECT COUNT(*) FROM bookings "
        "WHERE hotel_id=? AND status='cancelled' "
        "AND check_in_date>=? AND check_in_date<=?;",
        {hid, from, to});

    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Revenue broken down by room type
// ─────────────────────────────────────────────────────────────────────────────
json ReportManager::getRevenueByRoomType(int hotel_id,
                                          const std::string& from,
                                          const std::string& to) {
    auto rows = db_->query(R"sql(
        SELECT
            room_type_name,
            COUNT(*)                       AS bookings,
            COALESCE(SUM(total_amount), 0) AS revenue,
            COALESCE(AVG(nights),       0) AS avg_nights
        FROM bookings
        WHERE hotel_id = ?
          AND status   != 'cancelled'
          AND check_in_date >= ?
          AND check_in_date <= ?
        GROUP BY room_type_name
        ORDER BY revenue DESC;
    )sql", {std::to_string(hotel_id), from, to});

    json arr = json::array();
    for (auto& row : rows) {
        json item;
        for (auto& [k, v] : row) item[k] = v;
        arr.push_back(item);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Top 5 most booked room types
// ─────────────────────────────────────────────────────────────────────────────
json ReportManager::getMostBookedRoom(int hotel_id) {
    auto rows = db_->query(R"sql(
        SELECT room_type_name, COUNT(*) AS bookings
        FROM bookings
        WHERE hotel_id = ? AND status != 'cancelled'
        GROUP BY room_type_name
        ORDER BY bookings DESC
        LIMIT 5;
    )sql", {std::to_string(hotel_id)});

    json arr = json::array();
    for (auto& row : rows) {
        json item;
        for (auto& [k, v] : row) item[k] = v;
        arr.push_back(item);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Most popular activities by booking count
// ─────────────────────────────────────────────────────────────────────────────
json ReportManager::getMostPopularActivity(int hotel_id) {
    // activities_json stores a JSON array like [1,3,5]
    // We check if each activity id appears in any booking's activities_json
    auto rows = db_->query(R"sql(
        SELECT
            a.name,
            a.price,
            COUNT(b.id)        AS times_booked,
            SUM(a.price)       AS total_revenue
        FROM activities a
        LEFT JOIN bookings b
            ON b.activities_json LIKE '%' || CAST(a.id AS TEXT) || '%'
            AND b.hotel_id = ?
            AND b.status  != 'cancelled'
        GROUP BY a.id
        ORDER BY times_booked DESC;
    )sql", {std::to_string(hotel_id)});

    json arr = json::array();
    for (auto& row : rows) {
        json item;
        for (auto& [k, v] : row) item[k] = v;
        arr.push_back(item);
    }
    return arr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Overall booking statistics
// ─────────────────────────────────────────────────────────────────────────────
json ReportManager::getBookingStats(int hotel_id) {
    std::string hid = std::to_string(hotel_id);

    int total       = db_->queryInt("SELECT COUNT(*) FROM bookings WHERE hotel_id=?;", {hid});
    int active      = db_->queryInt("SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status IN ('active', 'checked_in');", {hid});
    int checked_out = db_->queryInt("SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status='checked_out';", {hid});
    int cancelled   = db_->queryInt("SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status='cancelled';", {hid});
    double revenue  = db_->queryDouble(
        "SELECT COALESCE(SUM(total_amount),0) FROM bookings "
        "WHERE hotel_id=? AND status!='cancelled';", {hid});
    double avg_stay = db_->queryDouble(
        "SELECT COALESCE(AVG(nights),0) FROM bookings "
        "WHERE hotel_id=? AND status!='cancelled';", {hid});

    return {
        {"total",          total},
        {"active",         active},
        {"checked_out",    checked_out},
        {"cancelled",      cancelled},
        {"total_revenue",  revenue},
        {"avg_stay_nights",avg_stay}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Current occupancy rate
// ─────────────────────────────────────────────────────────────────────────────
double ReportManager::getOccupancyRate(int hotel_id) {
    std::string hid = std::to_string(hotel_id);
    int total    = db_->queryInt("SELECT COUNT(*) FROM rooms WHERE hotel_id=?;", {hid});
    int occupied = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status='occupied';", {hid});
    return total > 0 ? (occupied * 100.0 / total) : 0.0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Daily revenue timeline for the last N days
// ─────────────────────────────────────────────────────────────────────────────
json ReportManager::getRevenueTimeline(int hotel_id, int days) {
    std::string interval = "-" + std::to_string(days) + " days";
    auto rows = db_->query(R"sql(
        SELECT
            date(created_at)               AS day,
            COUNT(*)                        AS bookings,
            COALESCE(SUM(total_amount), 0) AS revenue
        FROM bookings
        WHERE hotel_id = ?
          AND status   != 'cancelled'
          AND date(created_at) >= date('now', 'localtime', ?)
        GROUP BY day
        ORDER BY day ASC;
    )sql", {std::to_string(hotel_id), interval});

    json arr = json::array();
    for (auto& row : rows) {
        json item;
        for (auto& [k, v] : row) item[k] = v;
        arr.push_back(item);
    }
    return arr;
}
