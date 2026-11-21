#include "RoomManager.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>

RoomManager::RoomManager(std::shared_ptr<Database> db) : db_(db) {}

namespace {
std::string roomStatusValue(RoomStatus status) {
    switch (status) {
        case RoomStatus::AVAILABLE:   return "available";
        case RoomStatus::OCCUPIED:    return "occupied";
        case RoomStatus::MAINTENANCE: return "maintenance";
        case RoomStatus::CLEANED:     return "cleaned";
        case RoomStatus::RESERVED:    return "reserved";
        case RoomStatus::DEACTIVATED: return "deactivated";
    }
    return "available";
}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Row → Room
// ─────────────────────────────────────────────────────────────────────────────
Room RoomManager::rowToRoom(const DBRow& row) {
    Room r;
    for (auto& [k, v] : row) {
        if      (k == "id")             r.id           = v.empty() ? 0 : std::stoi(v);
        else if (k == "hotel_id")       r.hotel_id     = v.empty() ? 0 : std::stoi(v);
        else if (k == "room_type_id")   r.room_type_id = v.empty() ? 0 : std::stoi(v);
        else if (k == "room_number")    r.room_number  = v;
        else if (k == "room_type_name") r.room_type_name = v;
        else if (k == "floor")          r.floor        = v.empty() ? 1 : std::stoi(v);
        else if (k == "notes")          r.notes        = v;
        else if (k == "last_updated")   r.last_updated = v;
        else if (k == "status") {
            if      (v == "occupied")    r.status = RoomStatus::OCCUPIED;
            else if (v == "maintenance") r.status = RoomStatus::MAINTENANCE;
            else if (v == "cleaned")     r.status = RoomStatus::CLEANED;
            else if (v == "reserved")    r.status = RoomStatus::RESERVED;
            else if (v == "deactivated") r.status = RoomStatus::DEACTIVATED;
            else                         r.status = RoomStatus::AVAILABLE;
        }
    }
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Room CRUD
// ─────────────────────────────────────────────────────────────────────────────
int RoomManager::createRoom(const Room& room) {
    int id = db_->executeAndGetId(
        "INSERT INTO rooms (hotel_id, room_type_id, room_number, floor, notes) "
        "VALUES (?,?,?,?,?);",
        {std::to_string(room.hotel_id),
         std::to_string(room.room_type_id),
         room.room_number,
         std::to_string(room.floor),
         room.notes});
    db_->execute(R"sql(
        INSERT OR REPLACE INTO inventory (hotel_id, room_id, room_type_id, status, price, last_synced)
        SELECT r.hotel_id, r.id, r.room_type_id, r.status,
               COALESCE(NULLIF(rt.current_price, 0), rt.base_price, 0),
               datetime('now','localtime')
        FROM rooms r
        JOIN room_types rt ON rt.id = r.room_type_id
        WHERE r.id=?;
    )sql", {std::to_string(id)});
    return id;
}

bool RoomManager::updateRoom(const Room& room) {
    db_->execute(
        "UPDATE rooms SET room_type_id=?, room_number=?, floor=?, notes=?, "
        "last_updated=datetime('now','localtime') WHERE id=?;",
        {std::to_string(room.room_type_id),
         room.room_number,
         std::to_string(room.floor),
         room.notes,
         std::to_string(room.id)});
    db_->execute(R"sql(
        UPDATE inventory
        SET room_type_id=?,
            price=COALESCE(NULLIF((SELECT current_price FROM room_types WHERE id=?), 0), price),
            last_synced=datetime('now','localtime')
        WHERE room_id=?;
    )sql", {
        std::to_string(room.room_type_id),
        std::to_string(room.room_type_id),
        std::to_string(room.id)
    });
    return true;
}

bool RoomManager::deleteRoom(int room_id) {
    int bookingCount = db_->queryInt(
        "SELECT COUNT(*) FROM bookings WHERE room_id=?;",
        {std::to_string(room_id)});
    if (bookingCount > 0) {
        throw std::runtime_error("Room has booking history and cannot be permanently deleted");
    }
    db_->execute("DELETE FROM inventory WHERE room_id=?;", {std::to_string(room_id)});
    db_->execute("DELETE FROM rooms WHERE id=?;", {std::to_string(room_id)});
    return true;
}

std::optional<Room> RoomManager::getRoomById(int id) {
    auto rows = db_->query(
        "SELECT r.*, rt.name AS room_type_name FROM rooms r "
        "JOIN room_types rt ON r.room_type_id = rt.id WHERE r.id=?;",
        {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return rowToRoom(rows[0]);
}

std::vector<Room> RoomManager::getRoomsByHotel(int hotel_id) {
    auto rows = db_->query(
        "SELECT r.*, rt.name AS room_type_name FROM rooms r "
        "JOIN room_types rt ON r.room_type_id = rt.id "
        "WHERE r.hotel_id=? ORDER BY r.floor, r.room_number;",
        {std::to_string(hotel_id)});
    std::vector<Room> out;
    for (auto& r : rows) out.push_back(rowToRoom(r));
    return out;
}

std::vector<Room> RoomManager::getRoomsByType(int hotel_id, int room_type_id) {
    auto rows = db_->query(
        "SELECT r.*, rt.name AS room_type_name FROM rooms r "
        "JOIN room_types rt ON r.room_type_id = rt.id "
        "WHERE r.hotel_id=? AND r.room_type_id=? ORDER BY r.room_number;",
        {std::to_string(hotel_id), std::to_string(room_type_id)});
    std::vector<Room> out;
    for (auto& r : rows) out.push_back(rowToRoom(r));
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Availability — find rooms not booked in date range
// ─────────────────────────────────────────────────────────────────────────────
std::vector<Room> RoomManager::getAvailableRooms(int hotel_id,
                                                   int room_type_id,
                                                   const std::string& check_in,
                                                   const std::string& check_out) {
    std::string sql = R"sql(
        SELECT r.*, rt.name AS room_type_name
        FROM rooms r
        JOIN room_types rt ON r.room_type_id = rt.id
        WHERE r.hotel_id = ?
          AND r.status   = 'available'
          AND (? = 0 OR r.room_type_id = ?)
          AND r.id NOT IN (
              SELECT b.room_id FROM bookings b
              WHERE b.hotel_id = ?
                AND b.status   = 'active'
                AND b.check_in_date  < ?
                AND b.check_out_date > ?
          )
        ORDER BY r.floor, r.room_number;
    )sql";
    std::string rtStr = std::to_string(room_type_id);
    auto rows = db_->query(sql, {
        std::to_string(hotel_id),
        rtStr, rtStr,
        std::to_string(hotel_id),
        check_out, check_in
    });
    std::vector<Room> out;
    for (auto& r : rows) out.push_back(rowToRoom(r));
    return out;
}

std::optional<Room> RoomManager::findFirstAvailable(int hotel_id,
                                                      int room_type_id,
                                                      const std::string& check_in,
                                                      const std::string& check_out) {
    auto rooms = getAvailableRooms(hotel_id, room_type_id, check_in, check_out);
    if (rooms.empty()) return std::nullopt;
    return rooms[0];
}

// ─────────────────────────────────────────────────────────────────────────────
//  Status management
// ─────────────────────────────────────────────────────────────────────────────
bool RoomManager::setStatus(int room_id, RoomStatus status,
                             const std::string& notes) {
    std::string s = roomStatusValue(status);
    db_->execute(
        "UPDATE rooms SET status=?, notes=?, "
        "last_updated=datetime('now','localtime') WHERE id=?;",
        {s, notes, std::to_string(room_id)});
    db_->execute(R"sql(
        INSERT INTO inventory (hotel_id, room_id, room_type_id, status, price, last_synced)
        SELECT r.hotel_id, r.id, r.room_type_id, r.status,
               COALESCE(NULLIF(rt.current_price, 0), rt.base_price, 0),
               datetime('now','localtime')
        FROM rooms r
        JOIN room_types rt ON rt.id = r.room_type_id
        WHERE r.id=?
        ON CONFLICT(room_id) DO UPDATE SET
            room_type_id=excluded.room_type_id,
            status=excluded.status,
            price=COALESCE(NULLIF(inventory.price, 0), excluded.price),
            last_synced=excluded.last_synced;
    )sql", {std::to_string(room_id)});
    return true;
}

bool RoomManager::markMaintenance(int room_id, const std::string& reason) {
    return setStatus(room_id, RoomStatus::MAINTENANCE, reason);
}

bool RoomManager::releaseRoom(int room_id) {
    return setStatus(room_id, RoomStatus::AVAILABLE, "");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stats
// ─────────────────────────────────────────────────────────────────────────────
int RoomManager::countByStatus(int hotel_id, RoomStatus status) {
    std::string s = roomStatusValue(status);
    return db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status=?;",
        {std::to_string(hotel_id), s});
}

json RoomManager::getRoomStats(int hotel_id) {
    std::string hid = std::to_string(hotel_id);
    int total       = db_->queryInt("SELECT COUNT(*) FROM rooms WHERE hotel_id=?;", {hid});
    int available   = countByStatus(hotel_id, RoomStatus::AVAILABLE);
    int occupied    = countByStatus(hotel_id, RoomStatus::OCCUPIED);
    int maintenance = countByStatus(hotel_id, RoomStatus::MAINTENANCE);
    int cleaned     = countByStatus(hotel_id, RoomStatus::CLEANED);
    int reserved    = countByStatus(hotel_id, RoomStatus::RESERVED);
    int deactivated = countByStatus(hotel_id, RoomStatus::DEACTIVATED);

    // Per room-type breakdown
    auto rows = db_->query(
        "SELECT rt.name, "
        "COUNT(r.id) AS total, "
        "SUM(CASE WHEN r.status='available'   THEN 1 ELSE 0 END) AS available, "
        "SUM(CASE WHEN r.status='occupied'    THEN 1 ELSE 0 END) AS occupied, "
        "SUM(CASE WHEN r.status='maintenance' THEN 1 ELSE 0 END) AS maintenance, "
        "SUM(CASE WHEN r.status='cleaned'     THEN 1 ELSE 0 END) AS cleaned, "
        "SUM(CASE WHEN r.status='reserved'    THEN 1 ELSE 0 END) AS reserved, "
        "SUM(CASE WHEN r.status='deactivated' THEN 1 ELSE 0 END) AS deactivated "
        "FROM room_types rt "
        "LEFT JOIN rooms r ON r.room_type_id=rt.id AND r.hotel_id=rt.hotel_id "
        "WHERE rt.hotel_id=? GROUP BY rt.id ORDER BY rt.name;",
        {hid});

    json breakdown = json::array();
    for (auto& row : rows) {
        json item;
        for (auto& [k,v] : row) item[k] = v;
        breakdown.push_back(item);
    }

    return {
        {"total",       total},
        {"available",   available},
        {"occupied",    occupied},
        {"maintenance", maintenance},
        {"cleaned",     cleaned},
        {"reserved",    reserved},
        {"deactivated", deactivated},
        {"occupancy_pct", total > 0 ? (occupied * 100.0 / total) : 0.0},
        {"by_type",     breakdown}
    };
}
