#include "Managers.h"
#include "Utils.h"

SearchManager::SearchManager(std::shared_ptr<Database> db) : db_(db) {}

SearchResult SearchManager::search(const SearchParams& p) {
    // ── Build WHERE clause dynamically ──────────────────────────────────────
    std::string where = "WHERE b.hotel_id = ?";
    std::vector<std::string> params = {std::to_string(p.hotel_id)};

    if (!p.filter_name.empty()) {
        where += " AND LOWER(b.guest_name) LIKE LOWER(?)";
        params.push_back("%" + p.filter_name + "%");
    }
    if (!p.filter_phone.empty()) {
        where += " AND b.guest_phone LIKE ?";
        params.push_back("%" + p.filter_phone + "%");
    }
    if (!p.filter_booking_id.empty()) {
        where += " AND b.booking_id LIKE ?";
        params.push_back("%" + p.filter_booking_id + "%");
    }
    if (!p.filter_room_type.empty()) {
        where += " AND LOWER(b.room_type_name) LIKE LOWER(?)";
        params.push_back("%" + p.filter_room_type + "%");
    }
    if (!p.filter_status.empty()) {
        where += " AND b.status = ?";
        params.push_back(p.filter_status);
    }
    if (!p.date_from.empty()) {
        where += " AND b.check_in_date >= ?";
        params.push_back(p.date_from);
    }
    if (!p.date_to.empty()) {
        where += " AND b.check_in_date <= ?";
        params.push_back(p.date_to);
    }

    // ── Count total matching rows ────────────────────────────────────────────
    int total = db_->queryInt(
        "SELECT COUNT(*) FROM bookings b " + where + ";", params);

    // ── Sort ─────────────────────────────────────────────────────────────────
    std::string order = " ORDER BY ";
    if      (p.sort_by == "cost")  order += "b.total_amount";
    else if (p.sort_by == "name")  order += "b.guest_name";
    else                           order += "b.created_at";
    order += (p.sort_dir == "asc") ? " ASC" : " DESC";

    // ── Pagination ───────────────────────────────────────────────────────────
    int page     = std::max(1, p.page);
    int per_page = std::max(1, std::min(100, p.per_page));
    int offset   = (page - 1) * per_page;

    std::string sql = "SELECT b.* FROM bookings b "
                    + where + order
                    + " LIMIT ? OFFSET ?;";
    params.push_back(std::to_string(per_page));
    params.push_back(std::to_string(offset));

    auto rows = db_->query(sql, params);

    // ── Build result ─────────────────────────────────────────────────────────
    SearchResult result;
    result.total_count = total;
    result.page        = page;
    result.per_page    = per_page;
    result.total_pages = total > 0 ? (total + per_page - 1) / per_page : 0;

    for (auto& row : rows) {
        Booking b;
        for (auto& [k, v] : row) {
            if      (k == "id")             b.id             = v.empty() ? 0 : std::stoi(v);
            else if (k == "booking_id")     b.booking_id     = v;
            else if (k == "hotel_id")       b.hotel_id       = v.empty() ? 0 : std::stoi(v);
            else if (k == "room_id")        b.room_id        = v.empty() ? 0 : std::stoi(v);
            else if (k == "room_type_id")   b.room_type_id   = v.empty() ? 0 : std::stoi(v);
            else if (k == "room_number")    b.room_number    = v;
            else if (k == "room_type_name") b.room_type_name = v;
            else if (k == "guest_name")     b.guest_name     = v;
            else if (k == "guest_email")    b.guest_email    = v;
            else if (k == "guest_phone")    b.guest_phone    = v;
            else if (k == "guest_address")  b.guest_address  = v;
            else if (k == "check_in_date")  b.check_in_date  = v;
            else if (k == "check_out_date") b.check_out_date = v;
            else if (k == "nights")         b.nights         = v.empty() ? 1 : std::stoi(v);
            else if (k == "breakfast")      b.breakfast      = (v == "1");
            else if (k == "activities_json")b.activities_json= v;
            else if (k == "room_cost")      b.room_cost      = v.empty() ? 0 : std::stod(v);
            else if (k == "activity_cost")  b.activity_cost  = v.empty() ? 0 : std::stod(v);
            else if (k == "breakfast_cost") b.breakfast_cost = v.empty() ? 0 : std::stod(v);
            else if (k == "subtotal")       b.subtotal       = v.empty() ? 0 : std::stod(v);
            else if (k == "gst_amount")     b.gst_amount     = v.empty() ? 0 : std::stod(v);
            else if (k == "total_amount")   b.total_amount   = v.empty() ? 0 : std::stod(v);
            else if (k == "created_by")     b.created_by     = v.empty() ? 0 : std::stoi(v);
            else if (k == "created_at")     b.created_at     = v;
            else if (k == "updated_at")     b.updated_at     = v;
            else if (k == "status") {
                if      (v == "checked_out") b.status = BookingStatus::CHECKED_OUT;
                else if (v == "cancelled")   b.status = BookingStatus::CANCELLED;
                else if (v == "no_show")     b.status = BookingStatus::NO_SHOW;
                else                         b.status = BookingStatus::ACTIVE;
            }
        }
        result.bookings.push_back(b);
    }
    return result;
}
