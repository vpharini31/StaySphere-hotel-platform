#pragma once
#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
//  Enumerations
// ─────────────────────────────────────────────────────────────────────────────

enum class UserRole {
    SUPER_ADMIN,
    HOTEL_MANAGER,
    RECEPTIONIST,
    GUEST
};

inline std::string roleToString(UserRole role) {
    switch (role) {
        case UserRole::SUPER_ADMIN:   return "super_admin";
        case UserRole::HOTEL_MANAGER: return "hotel_manager";
        case UserRole::RECEPTIONIST:  return "receptionist";
        case UserRole::GUEST:         return "guest";
    }
    return "guest";
}

inline UserRole roleFromString(const std::string& role) {
    if (role == "super_admin")   return UserRole::SUPER_ADMIN;
    if (role == "hotel_manager") return UserRole::HOTEL_MANAGER;
    if (role == "manager")       return UserRole::HOTEL_MANAGER;
    if (role == "receptionist")  return UserRole::RECEPTIONIST;
    return UserRole::GUEST;
}

class PortalAccount {
public:
    PortalAccount(int id, std::string name, std::string email)
        : id_(id), name_(std::move(name)), email_(std::move(email)) {}
    virtual ~PortalAccount() = default;

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& email() const { return email_; }
    virtual UserRole role() const = 0;
    virtual std::string portalName() const = 0;

protected:
    int id_ = 0;
    std::string name_;
    std::string email_;
};

class Guest : public PortalAccount {
public:
    using PortalAccount::PortalAccount;
    UserRole role() const override { return UserRole::GUEST; }
    std::string portalName() const override { return "Guest Portal"; }
};

class HotelManagerAccount : public PortalAccount {
public:
    HotelManagerAccount(int id, std::string name, std::string email, int hotel_id)
        : PortalAccount(id, std::move(name), std::move(email)), hotel_id_(hotel_id) {}
    UserRole role() const override { return UserRole::HOTEL_MANAGER; }
    std::string portalName() const override { return "Hotel Manager Portal"; }
    int hotelId() const { return hotel_id_; }

private:
    int hotel_id_ = 0;
};

class Admin : public PortalAccount {
public:
    using PortalAccount::PortalAccount;
    UserRole role() const override { return UserRole::SUPER_ADMIN; }
    std::string portalName() const override { return "Super Admin Portal"; }
};

enum class RoomStatus {
    AVAILABLE,
    OCCUPIED,
    MAINTENANCE,
    CLEANED,
    RESERVED,
    DEACTIVATED
};

enum class BookingStatus {
    ACTIVE,
    CHECKED_IN,
    CHECKED_OUT,
    CANCELLED,
    NO_SHOW
};

enum class ActivityType {
    SPA          = 1,
    BOAT_RIDE    = 2,
    SCUBA_DIVING = 3,
    GYM          = 4,
    DJ_NIGHT     = 5
};

// ─────────────────────────────────────────────────────────────────────────────
//  User / Auth
// ─────────────────────────────────────────────────────────────────────────────

struct User {
    int         id            = 0;
    std::string username;
    std::string password_hash;   // SHA-256 hex
    std::string full_name;
    std::string email;
    std::string phone;
    UserRole    role          = UserRole::GUEST;
    int         hotel_id      = 0;   // 0 = all hotels (super admin)
    bool        is_locked     = false;
    int         failed_attempts = 0;
    std::string last_login;
    std::string created_at;

    json toJson() const {
        return {
            {"id",         id},
            {"username",   username},
            {"full_name",  full_name},
            {"email",      email},
            {"phone",      phone},
            {"role",       roleToString(role)},
            {"hotel_id",   hotel_id},
            {"is_locked",  is_locked},
            {"last_login", last_login},
            {"created_at", created_at}
        };
    }
};

struct Session {
    std::string token;       // UUID token stored in memory
    int         user_id  = 0;
    UserRole    role     = UserRole::RECEPTIONIST;
    int         hotel_id = 0;
    std::string created_at;
    std::string expires_at;
    bool        is_valid = false;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Hotel
// ─────────────────────────────────────────────────────────────────────────────

struct Hotel {
    int         id       = 0;
    std::string name;
    std::string location;
    std::string address;
    std::string phone;
    std::string email;
    std::string website;
    bool        is_active = true;
    std::string status = "approved";
    int         owner_id = 0;
    std::string description;
    std::string amenities;
    std::string image_url;
    std::string property_type = "Hotel";
    double      price_from = 0.0;
    double      star_rating = 5.0;
    double      review_score = 9.0;
    std::string created_at;

    json toJson() const {
        return {
            {"id",         id},
            {"hotel_name", name},
            {"name",       name},
            {"location",   location},
            {"address",    address},
            {"phone",      phone},
            {"email",      email},
            {"website",    website},
            {"is_active",  is_active},
            {"status",     status},
            {"owner_id",   owner_id},
            {"description", description},
            {"amenities",  amenities},
            {"image",      image_url},
            {"image_path", image_url},
            {"image_url",  image_url},
            {"property_type", property_type},
            {"price",      price_from},
            {"price_from", price_from},
            {"rating",     star_rating},
            {"star_rating", star_rating},
            {"review_score", review_score},
            {"created_at", created_at}
        };
    }
};

class Payment {
public:
    Payment(std::string method, double subtotal, double gst_rate = 0.05)
        : method_(std::move(method)), subtotal_(subtotal), gst_rate_(gst_rate) {}

    virtual ~Payment() = default;
    virtual std::string provider() const { return method_; }
    virtual double gstAmount() const { return subtotal_ * gst_rate_; }
    virtual double total() const { return subtotal_ + gstAmount(); }
    json toJson() const {
        return {
            {"method", provider()},
            {"subtotal", subtotal_},
            {"gst_rate", gst_rate_},
            {"gst_amount", gstAmount()},
            {"total", total()}
        };
    }

protected:
    std::string method_;
    double subtotal_ = 0.0;
    double gst_rate_ = 0.05;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Room Type
// ─────────────────────────────────────────────────────────────────────────────

struct RoomType {
    int         id           = 0;
    int         hotel_id     = 0;
    std::string name;            // "Suite", "Deluxe", "Penthouse", etc.
    std::string description;
    std::string room_size;
    int         max_capacity  = 2;
    double      base_price    = 0.0;
    double      current_price = 0.0;  // after dynamic pricing
    int         total_rooms   = 0;
    std::string amenities;           // JSON array string
    std::string created_at;

    json toJson() const {
        return {
            {"id",            id},
            {"hotel_id",      hotel_id},
            {"name",          name},
            {"description",   description},
            {"room_size",     room_size},
            {"max_capacity",  max_capacity},
            {"base_price",    base_price},
            {"current_price", current_price},
            {"total_rooms",   total_rooms},
            {"amenities",     amenities},
            {"created_at",    created_at}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Room
// ─────────────────────────────────────────────────────────────────────────────

struct Room {
    int         id           = 0;
    int         hotel_id     = 0;
    int         room_type_id = 0;
    std::string room_number;
    std::string room_type_name;  // denormalised for convenience
    RoomStatus  status       = RoomStatus::AVAILABLE;
    int         floor        = 0;
    std::string notes;
    std::string last_updated;

    std::string statusString() const {
        switch (status) {
            case RoomStatus::AVAILABLE:   return "available";
            case RoomStatus::OCCUPIED:    return "occupied";
            case RoomStatus::MAINTENANCE: return "maintenance";
            case RoomStatus::CLEANED:     return "cleaned";
            case RoomStatus::RESERVED:    return "reserved";
            case RoomStatus::DEACTIVATED: return "deactivated";
        }
        return "unknown";
    }

    json toJson() const {
        return {
            {"id",             id},
            {"hotel_id",       hotel_id},
            {"room_type_id",   room_type_id},
            {"room_number",    room_number},
            {"room_type_name", room_type_name},
            {"status",         statusString()},
            {"floor",          floor},
            {"notes",          notes},
            {"last_updated",   last_updated}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Activity
// ─────────────────────────────────────────────────────────────────────────────

struct Activity {
    int         id     = 0;
    std::string name;
    double      price  = 0.0;
    std::string description;

    json toJson() const {
        return {{"id", id}, {"name", name}, {"price", price}};
    }
};

// Predefined activity prices (₹)
inline const std::vector<Activity> PREDEFINED_ACTIVITIES = {
    {1, "Spa",          1000.0, "Full body spa and relaxation"},
    {2, "Boat Ride",    1500.0, "Guided boat ride on lake/sea"},
    {3, "Scuba Diving", 3000.0, "Professional scuba diving with instructor"},
    {4, "Gym",           500.0, "Gym access for full stay"},
    {5, "DJ Night",     2000.0, "Evening DJ night and entertainment"}
};

// ─────────────────────────────────────────────────────────────────────────────
//  Booking
// ─────────────────────────────────────────────────────────────────────────────

struct Booking {
    int         id              = 0;
    std::string booking_id;         // "BK-2026-001"
    int         hotel_id        = 0;
    int         room_id         = 0;
    int         room_type_id    = 0;
    std::string room_number;        // denormalised
    std::string room_type_name;     // denormalised

    // Guest details
    std::string guest_name;
    std::string guest_email;
    std::string guest_phone;
    std::string guest_address;

    // Dates
    std::string check_in_date;
    std::string check_out_date;
    int         nights          = 1;

    // Options
    bool        breakfast       = false;
    std::string activities_json;    // JSON array of activity IDs

    // Pricing
    double      room_cost       = 0.0;
    double      activity_cost   = 0.0;
    double      breakfast_cost  = 0.0;
    double      subtotal        = 0.0;
    double      gst_amount      = 0.0;
    double      total_amount    = 0.0;
    std::string coupon_code;
    double      discount_amount = 0.0;
    std::string payment_method  = "Pay at hotel";
    std::string payment_status  = "pending";
    double      commission_rate = 15.0;
    double      commission_amount = 0.0;
    double      hotel_amount = 0.0;
    std::string transaction_id;

    // Meta
    BookingStatus status        = BookingStatus::ACTIVE;
    int         created_by      = 0;   // user_id
    std::string created_at;
    std::string updated_at;

    std::string statusString() const {
        switch (status) {
            case BookingStatus::ACTIVE:      return "active";
            case BookingStatus::CHECKED_IN:  return "checked_in";
            case BookingStatus::CHECKED_OUT: return "checked_out";
            case BookingStatus::CANCELLED:   return "cancelled";
            case BookingStatus::NO_SHOW:     return "no_show";
        }
        return "unknown";
    }

    json toJson() const {
        return {
            {"id",              id},
            {"booking_id",      booking_id},
            {"hotel_id",        hotel_id},
            {"room_id",         room_id},
            {"room_number",     room_number},
            {"room_type_name",  room_type_name},
            {"guest_name",      guest_name},
            {"guest_email",     guest_email},
            {"guest_phone",     guest_phone},
            {"guest_address",   guest_address},
            {"check_in_date",   check_in_date},
            {"check_out_date",  check_out_date},
            {"nights",          nights},
            {"breakfast",       breakfast},
            {"activities_json", activities_json},
            {"room_cost",       room_cost},
            {"activity_cost",   activity_cost},
            {"breakfast_cost",  breakfast_cost},
            {"subtotal",        subtotal},
            {"gst_amount",      gst_amount},
            {"total_amount",    total_amount},
            {"coupon_code",     coupon_code},
            {"discount_amount", discount_amount},
            {"payment_method",  payment_method},
            {"payment_status",  payment_status},
            {"commission_rate", commission_rate},
            {"commission_amount", commission_amount},
            {"hotel_amount", hotel_amount},
            {"transaction_id", transaction_id},
            {"status",          statusString()},
            {"created_at",      created_at},
            {"updated_at",      updated_at}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Cancellation
// ─────────────────────────────────────────────────────────────────────────────

struct Cancellation {
    int         id          = 0;
    int         booking_id  = 0;      // FK → bookings.id
    std::string booking_ref;          // "BK-2026-001"
    std::string reason;
    int         cancelled_by = 0;    // user_id
    std::string cancelled_at;
    double      refund_amount = 0.0;

    json toJson() const {
        return {
            {"id",            id},
            {"booking_id",    booking_id},
            {"booking_ref",   booking_ref},
            {"reason",        reason},
            {"cancelled_by",  cancelled_by},
            {"cancelled_at",  cancelled_at},
            {"refund_amount", refund_amount}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Bill / Receipt
// ─────────────────────────────────────────────────────────────────────────────

struct BillItem {
    std::string description;
    double      amount = 0.0;
};

struct Bill {
    std::string booking_id;
    Hotel       hotel;
    Booking     booking;
    std::vector<BillItem> items;
    double      subtotal    = 0.0;
    double      gst_rate    = 0.05;   // 5 %
    double      gst_amount  = 0.0;
    double      total       = 0.0;
    std::string generated_at;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Report DTOs
// ─────────────────────────────────────────────────────────────────────────────

struct OccupancyStats {
    int    hotel_id       = 0;
    std::string hotel_name;
    int    total_rooms    = 0;
    int    occupied_rooms = 0;
    int    available      = 0;
    int    maintenance    = 0;
    double occupancy_pct  = 0.0;

    json toJson() const {
        return {
            {"hotel_id",       hotel_id},
            {"hotel_name",     hotel_name},
            {"total_rooms",    total_rooms},
            {"occupied_rooms", occupied_rooms},
            {"available",      available},
            {"maintenance",    maintenance},
            {"occupancy_pct",  occupancy_pct}
        };
    }
};

struct RevenueReport {
    std::string period;
    double      total_revenue      = 0.0;
    double      room_revenue       = 0.0;
    double      activity_revenue   = 0.0;
    double      breakfast_revenue  = 0.0;
    int         total_bookings     = 0;
    int         cancellations      = 0;
    double      avg_stay_nights    = 0.0;

    json toJson() const {
        return {
            {"period",             period},
            {"total_revenue",      total_revenue},
            {"room_revenue",       room_revenue},
            {"activity_revenue",   activity_revenue},
            {"breakfast_revenue",  breakfast_revenue},
            {"total_bookings",     total_bookings},
            {"cancellations",      cancellations},
            {"avg_stay_nights",    avg_stay_nights}
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  API Response helpers
// ─────────────────────────────────────────────────────────────────────────────

inline json successResponse(const json& data, const std::string& message = "Success") {
    return {{"success", true}, {"message", message}, {"data", data}};
}

inline json errorResponse(const std::string& message, int code = 400) {
    return {{"success", false}, {"message", message}, {"code", code}};
}
