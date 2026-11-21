#include "Managers.h"
#include "Utils.h"
#include <iostream>
#include <sstream>

BillingManager::BillingManager(std::shared_ptr<Database> db) : db_(db) {}

double BillingManager::calculateActivityCost(const std::string& activities_json) {
    double total = 0.0;
    try {
        auto acts = json::parse(activities_json.empty() ? "[]" : activities_json);
        for (auto& act_id : acts) {
            int id = act_id.get<int>();
            double price = db_->queryDouble(
                "SELECT price FROM activities WHERE id=?;",
                {std::to_string(id)});
            total += price;
        }
    } catch (...) {}
    return total;
}

Bill BillingManager::generateBill(const Booking& booking, const Hotel& hotel) {
    Bill bill;
    bill.booking_id   = booking.booking_id;
    bill.hotel        = hotel;
    bill.booking      = booking;
    bill.generated_at = Utils::nowISO();

    // Room cost line
    auto rtName = booking.room_type_name.empty() ? "Room" : booking.room_type_name;
    bill.items.push_back({
        rtName + " x " + std::to_string(booking.nights) + " night(s)",
        booking.room_cost
    });

    // Breakfast
    if (booking.breakfast_cost > 0)
        bill.items.push_back({"Breakfast (" + std::to_string(booking.nights) + " days)", booking.breakfast_cost});

    // Activities
    try {
        auto acts = json::parse(booking.activities_json.empty() ? "[]" : booking.activities_json);
        for (auto& act_id : acts) {
            int id = act_id.get<int>();
            auto rows = db_->query("SELECT name, price FROM activities WHERE id=?;",
                                   {std::to_string(id)});
            if (!rows.empty()) {
                std::string name;
                double price = 0;
                for (auto& [k,v] : rows[0]) {
                    if (k == "name")  name  = v;
                    if (k == "price") price = std::stod(v);
                }
                bill.items.push_back({name, price});
            }
        }
    } catch (...) {}

    if (booking.discount_amount > 0) {
        std::string label = booking.coupon_code.empty()
            ? "Coupon discount"
            : "Coupon discount (" + booking.coupon_code + ")";
        bill.items.push_back({label, -booking.discount_amount});
    }

    bill.subtotal   = booking.subtotal;
    bill.gst_rate   = 0.05;
    bill.gst_amount = booking.gst_amount;
    bill.total      = booking.total_amount;
    return bill;
}

Bill BillingManager::getBillByBookingRef(const std::string& booking_ref) {
    // Fetch booking
    auto rows = db_->query("SELECT * FROM bookings WHERE booking_id=?;", {booking_ref});
    if (rows.empty()) throw std::runtime_error("Booking not found: " + booking_ref);

    Booking b;
    for (auto& [k,v] : rows[0]) {
        if      (k=="id")              b.id             = std::stoi(v);
        else if (k=="booking_id")      b.booking_id     = v;
        else if (k=="hotel_id")        b.hotel_id       = std::stoi(v);
        else if (k=="room_id")         b.room_id        = std::stoi(v);
        else if (k=="room_type_id")    b.room_type_id   = std::stoi(v);
        else if (k=="room_number")     b.room_number    = v;
        else if (k=="room_type_name")  b.room_type_name = v;
        else if (k=="guest_name")      b.guest_name     = v;
        else if (k=="guest_email")     b.guest_email    = v;
        else if (k=="guest_phone")     b.guest_phone    = v;
        else if (k=="guest_address")   b.guest_address  = v;
        else if (k=="check_in_date")   b.check_in_date  = v;
        else if (k=="check_out_date")  b.check_out_date = v;
        else if (k=="nights")          b.nights         = std::stoi(v);
        else if (k=="breakfast")       b.breakfast      = (v=="1");
        else if (k=="activities_json") b.activities_json= v;
        else if (k=="room_cost")       b.room_cost      = std::stod(v);
        else if (k=="activity_cost")   b.activity_cost  = std::stod(v);
        else if (k=="breakfast_cost")  b.breakfast_cost = std::stod(v);
        else if (k=="subtotal")        b.subtotal       = std::stod(v);
        else if (k=="gst_amount")      b.gst_amount     = std::stod(v);
        else if (k=="total_amount")    b.total_amount   = std::stod(v);
        else if (k=="coupon_code")     b.coupon_code    = v;
        else if (k=="discount_amount") b.discount_amount= v.empty()?0:std::stod(v);
        else if (k=="payment_method")  b.payment_method = v;
        else if (k=="payment_status")  b.payment_status = v;
        else if (k=="commission_rate") b.commission_rate = v.empty()?15.0:std::stod(v);
        else if (k=="commission_amount") b.commission_amount = v.empty()?0:std::stod(v);
        else if (k=="hotel_amount")    b.hotel_amount = v.empty()?0:std::stod(v);
        else if (k=="transaction_id")  b.transaction_id = v;
    }

    // Fetch hotel
    Hotel h;
    auto hrows = db_->query("SELECT * FROM hotels WHERE id=?;",
                             {std::to_string(b.hotel_id)});
    if (!hrows.empty()) {
        for (auto& [k,v] : hrows[0]) {
            if      (k=="id")       h.id       = std::stoi(v);
            else if (k=="name")     h.name     = v;
            else if (k=="location") h.location = v;
            else if (k=="address")  h.address  = v;
            else if (k=="phone")    h.phone    = v;
            else if (k=="email")    h.email    = v;
        }
    }
    return generateBill(b, h);
}
