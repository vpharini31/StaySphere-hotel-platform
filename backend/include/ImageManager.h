#pragma once

#include "Database.h"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ImageManager {
public:
    explicit ImageManager(std::shared_ptr<Database> db);

    int addHotelImage(int hotel_id, const std::string& image_path, bool is_primary = false);
    bool deleteHotelImage(int image_id);
    json getHotelImagesJson(int hotel_id);
    std::string primaryHotelImage(int hotel_id);
    void hydrateHotelJson(json& hotel);

private:
    std::shared_ptr<Database> db_;

    void refreshHotelCover(int hotel_id);
};
