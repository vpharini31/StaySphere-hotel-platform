#include "Managers.h"
#include "RoomManager.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>

CancellationManager::CancellationManager(std::shared_ptr<Database> db) : db_(db) {}

Cancellation CancellationManager::rowToCancellation(const DBRow& row) {
    Cancellation c;
    for (auto& [k,v] : row) {
        if      (k=="id")            c.id            = v.empty()?0:std::stoi(v);
        else if (k=="booking_id")    c.booking_id    = v.empty()?0:std::stoi(v);
        else if (k=="booking_ref")   c.booking_ref   = v;
        else if (k=="reason")        c.reason        = v;
        else if (k=="cancelled_by")  c.cancelled_by  = v.empty()?0:std::stoi(v);
        else if (k=="refund_amount") c.refund_amount = v.empty()?0:std::stod(v);
        else if (k=="cancelled_at")  c.cancelled_at  = v;
    }
    return c;
}

std::optional<Cancellation> CancellationManager::cancelByRef(
        const std::string& booking_ref,
        const std::string& reason,
        int cancelled_by) {

    // 1. Find the active booking
    auto rows = db_->query(
        "SELECT id, hotel_id, room_id, total_amount, status, created_at, "
        "CASE WHEN datetime(created_at) <= datetime('now','localtime','-1 day') THEN 1 ELSE 0 END AS cancellation_expired "
        "FROM bookings WHERE booking_id=?;", {booking_ref});

    if (rows.empty()) {
        std::cerr << "[Cancel] Booking not found: " << booking_ref << "\n";
        return std::nullopt;
    }

    int booking_db_id = 0, room_id = 0;
    double total = 0;
    std::string status;
    bool cancellation_expired = false;
    for (auto& [k,v] : rows[0]) {
        if      (k=="id")           booking_db_id = std::stoi(v);
        else if (k=="room_id")      room_id       = std::stoi(v);
        else if (k=="total_amount") total         = std::stod(v);
        else if (k=="status")       status        = v;
        else if (k=="cancellation_expired") cancellation_expired = (v == "1");
    }

    if (status != "active") {
        std::cerr << "[Cancel] Booking " << booking_ref << " is not active (status=" << status << ")\n";
        return std::nullopt;
    }

    if (cancellation_expired) {
        std::cerr << "[Cancel] Booking " << booking_ref << " is past the 1-day cancellation window\n";
        throw std::runtime_error("Cancellation is disabled after 1 day of booking");
    }

    // 2. Transaction: mark cancelled + free room + archive
    db_->beginTransaction();
    try {
        // Mark booking cancelled
        db_->execute(
            "UPDATE bookings SET status='cancelled', updated_at=? WHERE booking_id=?;",
            {Utils::nowISO(), booking_ref});

        // Free the room
        RoomManager rooms(db_);
        rooms.releaseRoom(room_id);

        // Compute refund (50% if cancelled, full if > 3 days before check-in)
        double refund = total * 0.5;

        // Insert cancellation record
        int cancel_id = db_->executeAndGetId(
            "INSERT INTO cancellations "
            "(booking_id, booking_ref, reason, cancelled_by, refund_amount) "
            "VALUES (?,?,?,?,?);",
            {std::to_string(booking_db_id), booking_ref, reason,
             std::to_string(cancelled_by), std::to_string(refund)});

        db_->commit();

        Cancellation c;
        c.id            = cancel_id;
        c.booking_id    = booking_db_id;
        c.booking_ref   = booking_ref;
        c.reason        = reason;
        c.cancelled_by  = cancelled_by;
        c.refund_amount = refund;
        c.cancelled_at  = Utils::nowISO();

        std::cout << "[Cancel] " << booking_ref << " cancelled. Refund: Rs."
                  << refund << "\n";
        return c;

    } catch (...) {
        db_->rollback();
        throw;
    }
}

std::optional<Cancellation> CancellationManager::cancelByPhone(
        const std::string& phone,
        int hotel_id,
        const std::string& reason,
        int cancelled_by) {

    auto rows = db_->query(
        "SELECT booking_id FROM bookings "
        "WHERE hotel_id=? AND guest_phone=? AND status='active' LIMIT 1;",
        {std::to_string(hotel_id), phone});

    if (rows.empty()) {
        std::cerr << "[Cancel] No active booking found for phone " << phone << "\n";
        return std::nullopt;
    }

    std::string booking_ref = rows[0][0].second;
    return cancelByRef(booking_ref, reason, cancelled_by);
}

std::vector<Cancellation> CancellationManager::getCancellations(int hotel_id) {
    auto rows = db_->query(
        "SELECT c.* FROM cancellations c "
        "JOIN bookings b ON c.booking_id = b.id "
        "WHERE b.hotel_id=? ORDER BY c.cancelled_at DESC;",
        {std::to_string(hotel_id)});
    std::vector<Cancellation> out;
    for (auto& r : rows) out.push_back(rowToCancellation(r));
    return out;
}

std::optional<Cancellation> CancellationManager::getCancellationById(int id) {
    auto rows = db_->query("SELECT * FROM cancellations WHERE id=?;",
                           {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return rowToCancellation(rows[0]);
}
