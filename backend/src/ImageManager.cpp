#include "ImageManager.h"

#include <cstdlib>
#include <utility>
#include <unordered_set>

namespace {
std::string jsonText(const json& obj, const char* key) {
    if (!obj.contains(key) || obj[key].is_null()) return "";
    if (obj[key].is_string()) return obj[key].get<std::string>();
    if (obj[key].is_number_integer()) return std::to_string(obj[key].get<int>());
    if (obj[key].is_number_float()) return std::to_string(obj[key].get<double>());
    return "";
}

int jsonInt(const json& obj, const char* key) {
    const std::string text = jsonText(obj, key);
    return text.empty() ? 0 : std::atoi(text.c_str());
}
}

ImageManager::ImageManager(std::shared_ptr<Database> db) : db_(std::move(db)) {}

json ImageManager::getHotelImagesJson(int hotel_id) {
    auto rows = db_->query(R"sql(
        SELECT id, hotel_id,
               COALESCE(NULLIF(image_path,''), NULLIF(image_url,'')) AS image_path,
               COALESCE(NULLIF(image_path,''), NULLIF(image_url,'')) AS image_url,
               is_primary, created_at
        FROM hotel_images
        WHERE hotel_id=?
        ORDER BY is_primary DESC, id ASC;
    )sql", {std::to_string(hotel_id)});

    json arr = json::array();
    std::unordered_set<std::string> seen;
    for (const auto& row : rows) {
        json item;
        for (const auto& [key, value] : row) item[key] = value;
        item["url"] = item.value("image_path", "");
        const std::string url = item.value("image_path", "");
        if (url.empty() || seen.count(url)) continue;
        seen.insert(url);
        arr.push_back(item);
    }
    return arr;
}

std::string ImageManager::primaryHotelImage(int hotel_id) {
    std::string image = db_->queryString(R"sql(
        SELECT COALESCE(NULLIF(image_path,''), NULLIF(image_url,''))
        FROM hotel_images
        WHERE hotel_id=?
        ORDER BY is_primary DESC, id ASC
        LIMIT 1;
    )sql", {std::to_string(hotel_id)});
    if (!image.empty()) return image;

    return db_->queryString(
        "SELECT COALESCE(NULLIF(image,''), NULLIF(image_url,'')) FROM hotels WHERE id=?;",
        {std::to_string(hotel_id)});
}

void ImageManager::refreshHotelCover(int hotel_id) {
    const std::string cover = primaryHotelImage(hotel_id);
    db_->execute("UPDATE hotels SET image=?, image_url=? WHERE id=?;",
                 {cover, cover, std::to_string(hotel_id)});
}

int ImageManager::addHotelImage(int hotel_id, const std::string& image_path, bool is_primary) {
    if (hotel_id <= 0 || image_path.empty()) return 0;

    auto existing = db_->query(R"sql(
        SELECT id FROM hotel_images
        WHERE hotel_id=? AND COALESCE(NULLIF(image_path,''), NULLIF(image_url,''))=?
        LIMIT 1;
    )sql", {std::to_string(hotel_id), image_path});
    if (!existing.empty() && !existing[0].empty()) {
        int existing_id = std::atoi(existing[0][0].second.c_str());
        if (is_primary) {
            db_->execute("UPDATE hotel_images SET is_primary=0 WHERE hotel_id=?;",
                         {std::to_string(hotel_id)});
            db_->execute("UPDATE hotel_images SET is_primary=1 WHERE id=?;",
                         {std::to_string(existing_id)});
            refreshHotelCover(hotel_id);
        }
        return existing_id;
    }

    const bool has_primary = db_->queryInt(
        "SELECT COUNT(*) FROM hotel_images WHERE hotel_id=? AND is_primary=1;",
        {std::to_string(hotel_id)}) > 0;
    const bool make_primary = is_primary || !has_primary;

    if (make_primary) {
        db_->execute("UPDATE hotel_images SET is_primary=0 WHERE hotel_id=?;",
                     {std::to_string(hotel_id)});
    }

    int id = db_->executeAndGetId(
        "INSERT INTO hotel_images (hotel_id, image_path, image_url, is_primary) VALUES (?,?,?,?);",
        {std::to_string(hotel_id), image_path, image_path, make_primary ? "1" : "0"});

    if (make_primary) {
        refreshHotelCover(hotel_id);
    } else {
        const std::string current = db_->queryString(
            "SELECT COALESCE(NULLIF(image,''), NULLIF(image_url,'')) FROM hotels WHERE id=?;",
            {std::to_string(hotel_id)});
        if (current.empty()) refreshHotelCover(hotel_id);
    }
    return id;
}

bool ImageManager::deleteHotelImage(int image_id) {
    auto rows = db_->query("SELECT hotel_id, is_primary FROM hotel_images WHERE id=?;",
                           {std::to_string(image_id)});
    if (rows.empty()) return false;

    int hotel_id = 0;
    bool was_primary = false;
    for (const auto& [key, value] : rows[0]) {
        if (key == "hotel_id") hotel_id = std::atoi(value.c_str());
        if (key == "is_primary") was_primary = value == "1";
    }

    db_->execute("DELETE FROM hotel_images WHERE id=?;", {std::to_string(image_id)});

    if (was_primary && hotel_id > 0) {
        auto next = db_->query("SELECT id FROM hotel_images WHERE hotel_id=? ORDER BY id ASC LIMIT 1;",
                               {std::to_string(hotel_id)});
        if (!next.empty() && !next[0].empty()) {
            db_->execute("UPDATE hotel_images SET is_primary=1 WHERE id=?;", {next[0][0].second});
        }
    }
    if (hotel_id > 0) refreshHotelCover(hotel_id);
    return true;
}

void ImageManager::hydrateHotelJson(json& hotel) {
    const int hotel_id = jsonInt(hotel, "id");
    if (hotel_id <= 0) return;

    json images = getHotelImagesJson(hotel_id);
    std::string primary = images.empty() ? "" : images.front().value("image_path", "");
    if (primary.empty()) {
        primary = jsonText(hotel, "image_path");
        if (primary.empty()) primary = jsonText(hotel, "image");
        if (primary.empty()) primary = jsonText(hotel, "image_url");
    }

    hotel["images"] = images;
    hotel["image_path"] = primary;
    hotel["image"] = primary;
    hotel["image_url"] = primary;
}
