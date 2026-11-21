#pragma once
#include "Database.h"
#include "Models.h"
#include <vector>
#include <optional>

class RoomManager {
public:
    explicit RoomManager(std::shared_ptr<Database> db);

    // ── CRUD ──────────────────────────────────────────────────────────────
    int                   createRoom(const Room& room);
    bool                  updateRoom(const Room& room);
    bool                  deleteRoom(int room_id);
    std::optional<Room>   getRoomById(int id);
    std::vector<Room>     getRoomsByHotel(int hotel_id);
    std::vector<Room>     getRoomsByType(int hotel_id, int room_type_id);

    // ── Availability ──────────────────────────────────────────────────────
    std::vector<Room>     getAvailableRooms(int hotel_id,
                                             int room_type_id,
                                             const std::string& check_in,
                                             const std::string& check_out);

    std::optional<Room>   findFirstAvailable(int hotel_id,
                                              int room_type_id,
                                              const std::string& check_in,
                                              const std::string& check_out);

    // ── Status management ─────────────────────────────────────────────────
    bool setStatus(int room_id, RoomStatus status, const std::string& notes = "");
    bool markMaintenance(int room_id, const std::string& reason);
    bool releaseRoom(int room_id);

    // ── Stats ─────────────────────────────────────────────────────────────
    json getRoomStats(int hotel_id);
    int  countByStatus(int hotel_id, RoomStatus status);

private:
    std::shared_ptr<Database> db_;
    Room rowToRoom(const DBRow& row);
};
