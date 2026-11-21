#include "BookingManager.h"
#include "RoomManager.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>

BookingManager::BookingManager(std::shared_ptr<Database> db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Row → Booking
// ─────────────────────────────────────────────────────────────────────────────
Booking BookingManager::rowToBooking(const DBRow& row) {
    Booking b;
    for (auto& [k,v] : row) {
        if      (k=="id")              b.id             = v.empty()?0:std::stoi(v);
        else if (k=="booking_id")      b.booking_id     = v;
        else if (k=="hotel_id")        b.hotel_id       = v.empty()?0:std::stoi(v);
        else if (k=="room_id")         b.room_id        = v.empty()?0:std::stoi(v);
        else if (k=="room_type_id")    b.room_type_id   = v.empty()?0:std::stoi(v);
        else if (k=="room_number")     b.room_number    = v;
        else if (k=="room_type_name")  b.room_type_name = v;
        else if (k=="guest_name")      b.guest_name     = v;
        else if (k=="guest_email")     b.guest_email    = v;
        else if (k=="guest_phone")     b.guest_phone    = v;
        else if (k=="guest_address")   b.guest_address  = v;
        else if (k=="check_in_date")   b.check_in_date  = v;
        else if (k=="check_out_date")  b.check_out_date = v;
        else if (k=="nights")          b.nights         = v.empty()?1:std::stoi(v);
        else if (k=="breakfast")       b.breakfast      = (v=="1");
        else if (k=="activities_json") b.activities_json= v;
        else if (k=="room_cost")       b.room_cost      = v.empty()?0:std::stod(v);
        else if (k=="activity_cost")   b.activity_cost  = v.empty()?0:std::stod(v);
        else if (k=="breakfast_cost")  b.breakfast_cost = v.empty()?0:std::stod(v);
        else if (k=="subtotal")        b.subtotal       = v.empty()?0:std::stod(v);
        else if (k=="gst_amount")      b.gst_amount     = v.empty()?0:std::stod(v);
        else if (k=="total_amount")    b.total_amount   = v.empty()?0:std::stod(v);
        else if (k=="coupon_code")     b.coupon_code    = v;
        else if (k=="discount_amount") b.discount_amount= v.empty()?0:std::stod(v);
        else if (k=="payment_method")  b.payment_method = v;
        else if (k=="payment_status")  b.payment_status = v;
        else if (k=="commission_rate") b.commission_rate= v.empty()?15.0:std::stod(v);
        else if (k=="commission_amount") b.commission_amount= v.empty()?0:std::stod(v);
        else if (k=="hotel_amount")    b.hotel_amount   = v.empty()?0:std::stod(v);
        else if (k=="transaction_id")  b.transaction_id = v;
        else if (k=="created_by")      b.created_by     = v.empty()?0:std::stoi(v);
        else if (k=="created_at")      b.created_at     = v;
        else if (k=="updated_at")      b.updated_at     = v;
        else if (k=="status") {
            if      (v=="checked_out") b.status = BookingStatus::CHECKED_OUT;
            else if (v=="checked_in")  b.status = BookingStatus::CHECKED_IN;
            else if (v=="cancelled")   b.status = BookingStatus::CANCELLED;
            else if (v=="no_show")     b.status = BookingStatus::NO_SHOW;
            else                       b.status = BookingStatus::ACTIVE;
        }
    }
    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generate unique booking ID: BK-2026-001
// ─────────────────────────────────────────────────────────────────────────────
std::string BookingManager::generateBookingId(int hotel_id) {
    (void)hotel_id;
    std::string year = Utils::todayDate().substr(0,4);
    // Count bookings this year globally to maintain the UNIQUE database constraint
    int count = db_->queryInt(
        "SELECT COUNT(*) FROM bookings WHERE booking_id LIKE ?;",
        {"BK-" + year + "-%"});
    std::ostringstream oss;
    oss << "BK-" << year << "-" << std::setw(3) << std::setfill('0') << (count+1);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Duplicate phone check
// ─────────────────────────────────────────────────────────────────────────────
bool BookingManager::isDuplicatePhone(int hotel_id, const std::string& phone,
                                      int exclude_id) {
    int count = db_->queryInt(
        "SELECT COUNT(*) FROM bookings "
        "WHERE hotel_id=? AND guest_phone=? AND status IN ('active', 'checked_in') AND id!=?;",
        {std::to_string(hotel_id), phone, std::to_string(exclude_id)});
    return count > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cost calculation (pure — no DB write)
// ─────────────────────────────────────────────────────────────────────────────
Booking BookingManager::calculateCosts(Booking b) const {
    // Get room price
    double room_price = db_->queryDouble(
        "SELECT current_price FROM room_types WHERE id=?;",
        {std::to_string(b.room_type_id)});

    b.nights = Utils::daysBetween(b.check_in_date, b.check_out_date);
    if (b.nights < 1) b.nights = 1;

    b.room_cost = room_price * b.nights;

    // Activities
    b.activity_cost = 0.0;
    try {
        auto acts = json::parse(b.activities_json.empty() ? "[]" : b.activities_json);
        for (auto& act_id : acts) {
            int id = act_id.get<int>();
            double price = db_->queryDouble(
                "SELECT price FROM activities WHERE id=?;",
                {std::to_string(id)});
            b.activity_cost += price;
        }
    } catch (...) {}

    // Breakfast: Rs.350 per day
    b.breakfast_cost = b.breakfast ? (350.0 * b.nights) : 0.0;

    b.subtotal   = b.room_cost + b.activity_cost + b.breakfast_cost;
    b.gst_amount = b.subtotal * 0.05;
    b.total_amount = b.subtotal + b.gst_amount;

    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Create booking
// ─────────────────────────────────────────────────────────────────────────────
std::optional<Booking> BookingManager::createBooking(Booking& booking) {
    try {
        // 1. Validate required fields
        if (booking.guest_name.empty() || booking.guest_phone.empty())
            throw std::runtime_error("Guest name and phone are required");
        if (!Utils::isValidDate(booking.check_in_date))
            throw std::runtime_error("Invalid check_in_date (use YYYY-MM-DD)");
        if (booking.check_out_date.empty())
            booking.check_out_date = Utils::addDaysToDate(booking.check_in_date, booking.nights);

        // 2. Duplicate phone check
        if (isDuplicatePhone(booking.hotel_id, booking.guest_phone))
            throw std::runtime_error("A booking with this phone number is already active");

        // 3. Find available room of requested type
        RoomManager rooms(db_);
        auto room = rooms.findFirstAvailable(
            booking.hotel_id, booking.room_type_id,
            booking.check_in_date, booking.check_out_date);
        if (!room)
            throw std::runtime_error("No rooms available for selected type and dates");

        booking.room_id      = room->id;
        booking.room_number  = room->room_number;

        // Get room type name
        booking.room_type_name = db_->queryString(
            "SELECT name FROM room_types WHERE id=?;",
            {std::to_string(booking.room_type_id)});

        // 4. Calculate all costs
        booking = calculateCosts(booking);

        // 6. Insert into DB (transaction)
        db_->beginTransaction();
        try {
            // 5. Generate booking ID inside transaction to prevent race conditions.
            //    Loop on the rare UNIQUE collision: two concurrent transactions
            //    can read the same COUNT+1, but only one INSERT wins; the loser
            //    retries with a fresh count and a new suffix.
            const std::string year = Utils::todayDate().substr(0, 4);
            int attempt = 0;
            while (true) {
                attempt += 1;
                int count = db_->queryInt(
                    "SELECT COUNT(*) FROM bookings WHERE booking_id LIKE ?;",
                    {"BK-" + year + "-%"});
                std::ostringstream oss;
                oss << "BK-" << year << "-"
                    << std::setw(3) << std::setfill('0') << (count + 1)
                    << "-" << attempt;
                booking.booking_id = oss.str();
                booking.created_at = Utils::nowISO();

                try {
                    int newId = db_->executeAndGetId(R"sql(
                INSERT INTO bookings (
                    booking_id, hotel_id, room_id, room_type_id,
                    room_number, room_type_name,
                    guest_name, guest_email, guest_phone, guest_address,
                    check_in_date, check_out_date, nights,
                    breakfast, activities_json,
                    room_cost, activity_cost, breakfast_cost,
                    subtotal, gst_amount, total_amount,
                    status, created_by, created_at, updated_at
                ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,'active',?,?,?);
            )sql", {
                booking.booking_id,
                std::to_string(booking.hotel_id),
                std::to_string(booking.room_id),
                std::to_string(booking.room_type_id),
                booking.room_number, booking.room_type_name,
                booking.guest_name, booking.guest_email,
                booking.guest_phone, booking.guest_address,
                booking.check_in_date, booking.check_out_date,
                std::to_string(booking.nights),
                booking.breakfast ? "1" : "0",
                booking.activities_json.empty() ? "[]" : booking.activities_json,
                std::to_string(booking.room_cost),
                std::to_string(booking.activity_cost),
                std::to_string(booking.breakfast_cost),
                std::to_string(booking.subtotal),
                std::to_string(booking.gst_amount),
                std::to_string(booking.total_amount),
                std::to_string(booking.created_by),
                booking.created_at, booking.created_at
                });
                    booking.id = newId;
                    break; // success
                } catch (const DatabaseException& e) {
                    const std::string msg = e.what();
                    const bool isUniqueCollision =
                        msg.find("UNIQUE") != std::string::npos ||
                        msg.find("unique") != std::string::npos ||
                        msg.find("constraint") != std::string::npos;
                    if (!isUniqueCollision || attempt >= 50) throw;
                    // loop: retry with a new suffix
                }
            }

            // 7. Mark room as occupied
            rooms.setStatus(booking.room_id, RoomStatus::OCCUPIED);

            // 8. Update guest history
            db_->execute(R"sql(
                INSERT INTO guest_history (hotel_id, guest_phone, guest_name, total_visits, total_spent, preferred_room)
                VALUES (?,?,?,1,?,?)
                ON CONFLICT(hotel_id, guest_phone) DO UPDATE SET
                    total_visits  = total_visits + 1,
                    total_spent   = total_spent + excluded.total_spent,
                    guest_name    = excluded.guest_name,
                    last_visit    = datetime('now','localtime');
            )sql", {
                std::to_string(booking.hotel_id),
                booking.guest_phone, booking.guest_name,
                std::to_string(booking.total_amount),
                booking.room_type_name
            });

            db_->commit();
            std::cout << "[Booking] Created " << booking.booking_id
                      << " for " << booking.guest_name << "\n";
            return booking;

        } catch (...) {
            db_->rollback();
            throw;
        }
    } catch (const std::exception& e) {
        std::cerr << "[Booking] Error: " << e.what() << "\n";
        throw;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Fetch methods
// ─────────────────────────────────────────────────────────────────────────────
std::optional<Booking> BookingManager::getByRef(const std::string& booking_id) {
    auto rows = db_->query(
        "SELECT * FROM bookings WHERE booking_id=?;", {booking_id});
    if (rows.empty()) return std::nullopt;
    return rowToBooking(rows[0]);
}

std::optional<Booking> BookingManager::getById(int id) {
    auto rows = db_->query(
        "SELECT * FROM bookings WHERE id=?;", {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return rowToBooking(rows[0]);
}

std::vector<Booking> BookingManager::getByHotel(int hotel_id,
                                                  const std::string& status) {
    std::string sql = status == "all"
        ? "SELECT * FROM bookings WHERE hotel_id=? ORDER BY created_at DESC;"
        : "SELECT * FROM bookings WHERE hotel_id=? AND status=? ORDER BY created_at DESC;";

    DBResult rows = status == "all"
        ? db_->query(sql, {std::to_string(hotel_id)})
        : db_->query(sql, {std::to_string(hotel_id), status});

    std::vector<Booking> out;
    for (auto& r : rows) out.push_back(rowToBooking(r));
    return out;
}

std::vector<Booking> BookingManager::getByGuest(int hotel_id,
                                                  const std::string& phone) {
    auto rows = db_->query(
        "SELECT * FROM bookings WHERE hotel_id=? AND guest_phone=? "
        "ORDER BY created_at DESC;",
        {std::to_string(hotel_id), phone});
    std::vector<Booking> out;
    for (auto& r : rows) out.push_back(rowToBooking(r));
    return out;
}

bool BookingManager::checkOut(const std::string& booking_ref) {
    auto b = getByRef(booking_ref);
    if (!b || (b->status != BookingStatus::ACTIVE && b->status != BookingStatus::CHECKED_IN)) return false;

    db_->execute(
        "UPDATE bookings SET status='checked_out', updated_at=? WHERE booking_id=?;",
        {Utils::nowISO(), booking_ref});

    RoomManager rooms(db_);
    rooms.releaseRoom(b->room_id);
    return true;
}

bool BookingManager::updateBooking(const Booking& b) {
    db_->execute(
        "UPDATE bookings SET guest_name=?, guest_email=?, guest_address=?, "
        "updated_at=? WHERE id=?;",
        {b.guest_name, b.guest_email, b.guest_address,
         Utils::nowISO(), std::to_string(b.id)});
    return true;
}
