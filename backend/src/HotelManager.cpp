#include "HotelManager.h"
#include "Utils.h"
#include <iostream>

HotelManager::HotelManager(std::shared_ptr<Database> db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Row → Hotel
// ─────────────────────────────────────────────────────────────────────────────
Hotel HotelManager::rowToHotel(const DBRow& row) {
    Hotel h;
    for (auto& [k, v] : row) {
        if      (k == "id")         h.id        = v.empty() ? 0 : std::stoi(v);
        else if (k == "hotel_name" && h.name.empty()) h.name = v;
        else if (k == "name")       h.name      = v;
        else if (k == "location")   h.location  = v;
        else if (k == "address")    h.address   = v;
        else if (k == "phone")      h.phone     = v;
        else if (k == "email")      h.email     = v;
        else if (k == "website")    h.website   = v;
        else if (k == "is_active")  h.is_active = (v == "1");
        else if (k == "status")     h.status    = v;
        else if (k == "owner_id")   h.owner_id  = v.empty() ? 0 : std::stoi(v);
        else if (k == "description") h.description = v;
        else if (k == "amenities")  h.amenities = v;
        else if (k == "image" && h.image_url.empty()) h.image_url = v;
        else if (k == "image_url")  h.image_url = v;
        else if (k == "property_type") h.property_type = v;
        else if (k == "price" && h.price_from == 0) h.price_from = v.empty() ? 0 : std::stod(v);
        else if (k == "price_from") h.price_from = v.empty() ? 0 : std::stod(v);
        else if (k == "rating" && h.star_rating == 5.0) h.star_rating = v.empty() ? 5.0 : std::stod(v);
        else if (k == "star_rating") h.star_rating = v.empty() ? 0 : std::stod(v);
        else if (k == "review_score") h.review_score = v.empty() ? 0 : std::stod(v);
        else if (k == "created_at") h.created_at= v;
    }
    return h;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Row → RoomType
// ─────────────────────────────────────────────────────────────────────────────
RoomType HotelManager::rowToRoomType(const DBRow& row) {
    RoomType rt;
    for (auto& [k, v] : row) {
        if      (k == "id")            rt.id            = v.empty() ? 0 : std::stoi(v);
        else if (k == "hotel_id")      rt.hotel_id      = v.empty() ? 0 : std::stoi(v);
        else if (k == "name")          rt.name          = v;
        else if (k == "description")   rt.description   = v;
        else if (k == "room_size")     rt.room_size     = v;
        else if (k == "max_capacity")  rt.max_capacity  = v.empty() ? 2 : std::stoi(v);
        else if (k == "base_price")    rt.base_price    = v.empty() ? 0 : std::stod(v);
        else if (k == "current_price") rt.current_price = v.empty() ? 0 : std::stod(v);
        else if (k == "total_rooms")   rt.total_rooms   = v.empty() ? 0 : std::stoi(v);
        else if (k == "amenities")     rt.amenities     = v;
        else if (k == "created_at")    rt.created_at    = v;
    }
    return rt;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hotel CRUD
// ─────────────────────────────────────────────────────────────────────────────
int HotelManager::createHotel(const Hotel& h) {
    return db_->executeAndGetId(
        "INSERT INTO hotels (hotel_name,name,location,address,phone,email,website,owner_id,"
        "description,amenities,image,image_url,property_type,price,price_from,rating,star_rating,review_score,status,is_active) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);",
        {h.name, h.name, h.location, h.address, h.phone, h.email, h.website,
         std::to_string(h.owner_id), h.description, h.amenities, h.image_url,
         h.image_url, h.property_type, std::to_string(h.price_from), std::to_string(h.price_from),
         std::to_string(h.star_rating), std::to_string(h.star_rating),
         std::to_string(h.review_score), h.status, h.is_active ? "1" : "0"});
}

bool HotelManager::updateHotel(const Hotel& h) {
    db_->execute(
        "UPDATE hotels SET hotel_name=?,name=?,location=?,address=?,phone=?,email=?,website=?,"
        "description=?,amenities=?,image=?,image_url=?,property_type=?,price=?,price_from=?,rating=?,star_rating=?,review_score=?,status=?,is_active=? "
        "WHERE id=?;",
        {h.name, h.name, h.location, h.address, h.phone, h.email, h.website,
         h.description, h.amenities, h.image_url, h.image_url, h.property_type,
         std::to_string(h.price_from), std::to_string(h.price_from),
         std::to_string(h.star_rating), std::to_string(h.star_rating),
         std::to_string(h.review_score), h.status, h.is_active ? "1" : "0",
         std::to_string(h.id)});
    return true;
}

bool HotelManager::deleteHotel(int hotel_id) {
    db_->execute("UPDATE hotels SET is_active=0, status='inactive' WHERE id=?;",
                 {std::to_string(hotel_id)});
    return true;
}

std::optional<Hotel> HotelManager::getHotelById(int id) {
    auto rows = db_->query("SELECT * FROM hotels WHERE id=?;",
                           {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return rowToHotel(rows[0]);
}

std::vector<Hotel> HotelManager::getAllHotels(bool active_only) {
    std::string sql = active_only
        ? "SELECT * FROM hotels WHERE is_active=1 AND status='approved' ORDER BY name;"
        : "SELECT * FROM hotels ORDER BY name;";
    auto rows = db_->query(sql);
    std::vector<Hotel> out;
    for (auto& r : rows) out.push_back(rowToHotel(r));
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Room type CRUD
// ─────────────────────────────────────────────────────────────────────────────
int HotelManager::createRoomType(const RoomType& rt) {
    return db_->executeAndGetId(
        "INSERT INTO room_types "
        "(hotel_id,name,description,room_size,max_capacity,base_price,current_price,total_rooms,amenities) "
        "VALUES (?,?,?,?,?,?,?,?,?);",
        {std::to_string(rt.hotel_id), rt.name, rt.description, rt.room_size,
         std::to_string(rt.max_capacity),
         std::to_string(rt.base_price),
         std::to_string(rt.current_price),
         std::to_string(rt.total_rooms),
         rt.amenities.empty() ? "[]" : rt.amenities});
}

bool HotelManager::updateRoomType(const RoomType& rt) {
    db_->execute(
        "UPDATE room_types SET name=?,description=?,room_size=?,max_capacity=?,"
        "base_price=?,current_price=?,total_rooms=?,amenities=? WHERE id=?;",
        {rt.name, rt.description, rt.room_size,
         std::to_string(rt.max_capacity),
         std::to_string(rt.base_price),
         std::to_string(rt.current_price),
         std::to_string(rt.total_rooms),
         rt.amenities,
         std::to_string(rt.id)});
    return true;
}

bool HotelManager::deleteRoomType(int rt_id) {
    db_->execute("DELETE FROM room_types WHERE id=?;", {std::to_string(rt_id)});
    return true;
}

std::optional<RoomType> HotelManager::getRoomTypeById(int id) {
    auto rows = db_->query("SELECT * FROM room_types WHERE id=?;",
                           {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return rowToRoomType(rows[0]);
}

std::vector<RoomType> HotelManager::getRoomTypesByHotel(int hotel_id) {
    auto rows = db_->query(
        "SELECT rt.*, "
        "(SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='available') AS available_count "
        "FROM room_types rt WHERE rt.hotel_id=? ORDER BY rt.base_price;",
        {std::to_string(hotel_id)});
    std::vector<RoomType> out;
    for (auto& r : rows) out.push_back(rowToRoomType(r));
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stats
// ─────────────────────────────────────────────────────────────────────────────
OccupancyStats HotelManager::getOccupancyStats(int hotel_id) {
    OccupancyStats s;
    s.hotel_id = hotel_id;

    auto hopt = getHotelById(hotel_id);
    if (hopt) s.hotel_name = hopt->name;

    s.total_rooms    = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=?;",
        {std::to_string(hotel_id)});
    s.occupied_rooms = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status='occupied';",
        {std::to_string(hotel_id)});
    s.available      = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status='available';",
        {std::to_string(hotel_id)});
    s.maintenance    = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status='maintenance';",
        {std::to_string(hotel_id)});
    s.occupancy_pct  = s.total_rooms > 0
        ? (static_cast<double>(s.occupied_rooms) / s.total_rooms) * 100.0
        : 0.0;
    return s;
}

json HotelManager::getDashboardSummary(int hotel_id) {
    auto occ = getOccupancyStats(hotel_id);
    std::string hid = std::to_string(hotel_id);

    double revenue = db_->queryDouble(
        "SELECT COALESCE(SUM(total_amount),0) FROM bookings "
        "WHERE hotel_id=? AND status='active';", {hid});

    int active_bookings = db_->queryInt(
        "SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status='active';", {hid});

    int todays_checkins = db_->queryInt(
        "SELECT COUNT(*) FROM bookings "
        "WHERE hotel_id=? AND check_in_date=date('now','localtime') AND status='active';",
        {hid});

    int todays_checkouts = db_->queryInt(
        "SELECT COUNT(*) FROM bookings "
        "WHERE hotel_id=? AND check_out_date=date('now','localtime') AND status='active';",
        {hid});

    return {
        {"hotel_id",          hotel_id},
        {"hotel_name",        occ.hotel_name},
        {"total_rooms",       occ.total_rooms},
        {"occupied_rooms",    occ.occupied_rooms},
        {"available_rooms",   occ.available},
        {"maintenance_rooms", occ.maintenance},
        {"occupancy_pct",     occ.occupancy_pct},
        {"total_revenue",     revenue},
        {"active_bookings",   active_bookings},
        {"todays_checkins",   todays_checkins},
        {"todays_checkouts",  todays_checkouts}
    };
}
