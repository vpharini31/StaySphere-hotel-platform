#pragma once
#include "Database.h"
#include "Models.h"
#include <vector>
#include <optional>

class BookingManager {
public:
    explicit BookingManager(std::shared_ptr<Database> db);

    // ── Create / Update ───────────────────────────────────────────────────
    std::optional<Booking> createBooking(Booking& booking);  // fills booking_id, dates
    bool                   updateBooking(const Booking& booking);
    bool                   checkOut(const std::string& booking_ref);

    // ── Fetch ─────────────────────────────────────────────────────────────
    std::optional<Booking> getByRef(const std::string& booking_id);
    std::optional<Booking> getById(int id);
    std::vector<Booking>   getByHotel(int hotel_id,
                                       const std::string& status = "active");
    std::vector<Booking>   getByGuest(int hotel_id,
                                       const std::string& phone);

    // ── Validation ────────────────────────────────────────────────────────
    bool   isDuplicatePhone(int hotel_id, const std::string& phone,
                             int exclude_id = -1);

    // ── Cost preview (no DB write) ────────────────────────────────────────
    Booking calculateCosts(Booking booking) const;

    // ── ID generation ─────────────────────────────────────────────────────
    std::string generateBookingId(int hotel_id);   // e.g. "BK-2026-042"

private:
    std::shared_ptr<Database> db_;

    Booking       rowToBooking(const DBRow& row);
    std::string   addDays(const std::string& date_str, int days);
};
