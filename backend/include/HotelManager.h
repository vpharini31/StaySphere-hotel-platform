#pragma once
#include "Database.h"
#include "Models.h"
#include <vector>
#include <optional>

class HotelManager {
public:
    explicit HotelManager(std::shared_ptr<Database> db);

    // ── CRUD ──────────────────────────────────────────────────────────────
    int                    createHotel(const Hotel& hotel);
    bool                   updateHotel(const Hotel& hotel);
    bool                   deleteHotel(int hotel_id);          // soft-delete
    std::optional<Hotel>   getHotelById(int id);
    std::vector<Hotel>     getAllHotels(bool active_only = true);

    // ── Room types per hotel ──────────────────────────────────────────────
    int                    createRoomType(const RoomType& rt);
    bool                   updateRoomType(const RoomType& rt);
    bool                   deleteRoomType(int rt_id);
    std::optional<RoomType> getRoomTypeById(int id);
    std::vector<RoomType>  getRoomTypesByHotel(int hotel_id);

    // ── Stats ─────────────────────────────────────────────────────────────
    OccupancyStats         getOccupancyStats(int hotel_id);
    json                   getDashboardSummary(int hotel_id);

private:
    std::shared_ptr<Database> db_;

    Hotel    rowToHotel(const DBRow& row);
    RoomType rowToRoomType(const DBRow& row);
};
