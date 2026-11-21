
#include "APIRouter.h"
#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <vector>

namespace {
json dbRowsToJson(const DBResult& rows) {
    json arr = json::array();
    for (auto& row : rows) {
        json obj;
        for (auto& [k, v] : row) obj[k] = v;
        arr.push_back(obj);
    }
    return arr;
}

json dbRowToJson(const DBRow& row) {
    json obj;
    for (auto& [k, v] : row) obj[k] = v;
    return obj;
}

std::string firstParam(const crow::request& req, const char* key, const std::string& fallback = "") {
    auto v = req.url_params.get(key);
    return v ? std::string(v) : fallback;
}

std::string bodyText(const json& body, const char* key, const std::string& fallback = "") {
    if (!body.contains(key) || body[key].is_null()) return fallback;
    if (body[key].is_string()) return body.value(key, fallback);
    if (body[key].is_array()) return body[key].dump();
    if (body[key].is_boolean()) return body[key].get<bool>() ? "true" : "false";
    if (body[key].is_number_integer()) return std::to_string(body[key].get<int>());
    if (body[key].is_number_float()) return std::to_string(body[key].get<double>());
    return fallback;
}

std::string bodyArrayText(const json& body, const char* key, const std::string& fallback = "[]") {
    if (!body.contains(key) || body[key].is_null()) return fallback;
    if (body[key].is_array()) return body[key].dump();
    if (body[key].is_string()) return body.value(key, fallback);
    return fallback;
}

void updateHotelPartnerFields(Database& db, int hotelId, const json& body) {
    auto rows = db.query("SELECT * FROM hotels WHERE id=?;", {std::to_string(hotelId)});
    std::unordered_map<std::string, std::string> current;
    if (!rows.empty()) {
        for (const auto& [key, value] : rows[0]) current[key] = value;
    }
    auto text = [&](const char* key, const std::string& fallback = "") {
        auto found = current.find(key);
        return bodyText(body, key, found != current.end() ? found->second : fallback);
    };
    auto arrayText = [&](const char* key, const std::string& fallback = "[]") {
        auto found = current.find(key);
        return bodyArrayText(body, key, found != current.end() ? found->second : fallback);
    };

    db.execute(R"sql(
        UPDATE hotels SET
            country=?, state=?, city=?, postal_code=?, coordinates=?, nearby_places=?,
            business_name=?, room_count=?, weekend_price=?, seasonal_price=?, taxes_fees=?, min_stay=?,
            check_in_time=?, check_out_time=?, cancellation_policy=?, refund_rules=?,
            child_policy=?, pet_policy=?, smoking_policy=?,
            id_proof_requirements=?, extra_guest_rules=?, custom_policies=?,
            bank_account=?, tax_number=?, pan_registration=?, accepted_payments=?
        WHERE id=?;
    )sql", {
        text("country"), text("state"), text("city"), text("postal_code"), text("coordinates"), arrayText("nearby_places"),
        text("business_name"), text("room_count", "0"), text("weekend_price", "0"), text("seasonal_price", "0"),
        text("taxes_fees", "0"), text("min_stay", "1"), text("check_in_time"), text("check_out_time"),
        text("cancellation_policy"), text("refund_rules"), text("child_policy"), text("pet_policy"), text("smoking_policy"),
        text("id_proof_requirements"), text("extra_guest_rules"), text("custom_policies"),
        text("bank_account"), text("tax_number"), text("pan_registration"), arrayText("accepted_payments"),
        std::to_string(hotelId)
    });
}

void recordHotelAssets(Database& db, int hotelId, const json& body) {
    if (body.contains("hotel_images") && body["hotel_images"].is_array()) {
        bool hasPrimary = db.queryInt("SELECT COUNT(*) FROM hotel_images WHERE hotel_id=? AND is_primary=1;",
                                      {std::to_string(hotelId)}) > 0;
        for (const auto& image : body["hotel_images"]) {
            if (!image.is_string() || image.get<std::string>().empty()) continue;
            const std::string imageUrl = image.get<std::string>();
            if (db.queryInt(R"sql(
                SELECT COUNT(*) FROM hotel_images
                WHERE hotel_id=? AND COALESCE(NULLIF(image_path,''), NULLIF(image_url,''))=?;
            )sql", {std::to_string(hotelId), imageUrl}) > 0) continue;
            db.execute("INSERT INTO hotel_images (hotel_id, image_path, image_url, is_primary) VALUES (?,?,?,?);",
                       {std::to_string(hotelId), imageUrl, imageUrl,
                        hasPrimary ? "0" : "1"});
            if (!hasPrimary) {
                db.execute("UPDATE hotels SET image=?, image_url=? WHERE id=?;",
                           {imageUrl, imageUrl, std::to_string(hotelId)});
            }
            hasPrimary = true;
        }
    }
    if (body.contains("documents") && body["documents"].is_array()) {
        for (const auto& doc : body["documents"]) {
            if (!doc.is_object()) continue;
            std::string type = doc.value("document_type", "verification");
            std::string url = doc.value("document_url", "");
            if (url.empty()) continue;
            if (db.queryInt("SELECT COUNT(*) FROM hotel_documents WHERE hotel_id=? AND document_url=?;",
                            {std::to_string(hotelId), url}) > 0) continue;
            db.execute("INSERT INTO hotel_documents (hotel_id, document_type, document_url) VALUES (?,?,?);",
                       {std::to_string(hotelId), type, url});
        }
    }
}

std::string multipartField(const crow::multipart::message& msg, const std::string& name) {
    auto part = msg.part_map.find(name);
    return part == msg.part_map.end() ? "" : part->second.body;
}

std::string multipartFilename(const crow::multipart::part& part) {
    const auto& disposition = part.get_header_object("Content-Disposition");
    auto found = disposition.params.find("filename");
    return found == disposition.params.end() ? "" : found->second;
}

std::string multipartContentType(const crow::multipart::part& part) {
    return part.get_header_object("Content-Type").value;
}

bool boolParam(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower == "1" || lower == "true" || lower == "yes" || lower == "primary";
}

bool bodyBool(const json& body, const char* key, bool fallback = false) {
    if (!body.contains(key) || body[key].is_null()) return fallback;
    if (body[key].is_boolean()) return body[key].get<bool>();
    if (body[key].is_number_integer()) return body[key].get<int>() != 0;
    if (body[key].is_string()) return boolParam(body[key].get<std::string>());
    return fallback;
}

std::string roomNumberFromSequence(const std::string& prefix, int number, size_t width) {
    std::ostringstream stream;
    stream << prefix;
    if (width > 0) stream << std::setw(static_cast<int>(width)) << std::setfill('0');
    stream << number;
    return stream.str();
}

std::string sanitizeRoomPrefix(const std::string& value) {
    std::string out;
    for (unsigned char c : value) {
        if (std::isalnum(c)) out += static_cast<char>(std::toupper(c));
    }
    return out;
}

std::string defaultRoomPrefixForType(const std::string& roomTypeName) {
    std::string lower = roomTypeName;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("deluxe") != std::string::npos) return "D";
    if (lower.find("family") != std::string::npos) return "F";
    if (lower.find("penthouse") != std::string::npos) return "P";
    if (lower.find("presidential") != std::string::npos) return "P";
    if (lower.find("premium") != std::string::npos) return "P";
    if (lower.find("suite") != std::string::npos) return "S";
    if (lower.find("classic") != std::string::npos) return "C";
    if (lower.find("standard") != std::string::npos) return "S";
    for (unsigned char c : roomTypeName) {
        if (std::isalnum(c)) return std::string(1, static_cast<char>(std::toupper(c)));
    }
    return "R";
}

std::string rowValue(const DBRow& row, const std::string& key, const std::string& fallback = "") {
    for (const auto& [k, v] : row) {
        if (k == key) return v;
    }
    return fallback;
}

json roomImagesJson(Database& db, int roomTypeId) {
    return dbRowsToJson(db.query(R"sql(
        SELECT id, room_type_id, image_url, image_url AS image_path, is_primary, created_at
        FROM room_images
        WHERE room_type_id=?
        ORDER BY is_primary DESC, id ASC;
    )sql", {std::to_string(roomTypeId)}));
}

std::string primaryRoomImage(Database& db, int roomTypeId) {
    return db.queryString(R"sql(
        SELECT image_url
        FROM room_images
        WHERE room_type_id=?
        ORDER BY is_primary DESC, id ASC
        LIMIT 1;
    )sql", {std::to_string(roomTypeId)});
}

int addRoomImageRecord(Database& db, int roomTypeId, const std::string& imageUrl, bool isPrimary) {
    if (roomTypeId <= 0 || imageUrl.empty()) return 0;
    auto existing = db.query(
        "SELECT id FROM room_images WHERE room_type_id=? AND image_url=? LIMIT 1;",
        {std::to_string(roomTypeId), imageUrl});
    if (!existing.empty() && !existing[0].empty()) {
        const int existingId = std::atoi(existing[0][0].second.c_str());
        if (isPrimary) {
            db.execute("UPDATE room_images SET is_primary=0 WHERE room_type_id=?;", {std::to_string(roomTypeId)});
            db.execute("UPDATE room_images SET is_primary=1 WHERE id=?;", {std::to_string(existingId)});
        }
        return existingId;
    }

    const bool hasPrimary = db.queryInt(
        "SELECT COUNT(*) FROM room_images WHERE room_type_id=? AND is_primary=1;",
        {std::to_string(roomTypeId)}) > 0;
    const bool makePrimary = isPrimary || !hasPrimary;
    if (makePrimary) db.execute("UPDATE room_images SET is_primary=0 WHERE room_type_id=?;", {std::to_string(roomTypeId)});
    return db.executeAndGetId(
        "INSERT INTO room_images (room_type_id, image_url, is_primary) VALUES (?,?,?);",
        {std::to_string(roomTypeId), imageUrl, makePrimary ? "1" : "0"});
}

double toDoubleSafe(const std::string& value, double fallback = 0.0) {
    if (value.empty()) return fallback;
    try { return std::stod(value); } catch (...) { return fallback; }
}

int toIntSafe(const std::string& value, int fallback = 0) {
    if (value.empty()) return fallback;
    try { return std::stoi(value); } catch (...) { return fallback; }
}

std::string jsonValueText(const json& obj, const char* key, const std::string& fallback = "") {
    if (!obj.contains(key) || obj[key].is_null()) return fallback;
    if (obj[key].is_string()) return obj[key].get<std::string>();
    if (obj[key].is_number_integer()) return std::to_string(obj[key].get<int>());
    if (obj[key].is_number_float()) return std::to_string(obj[key].get<double>());
    if (obj[key].is_boolean()) return obj[key].get<bool>() ? "true" : "false";
    return fallback;
}

bool couponAppliesToHotel(const std::string& applicable, int hotelId) {
    if (hotelId <= 0 || applicable.empty() || applicable == "[]") return true;
    try {
        auto ids = json::parse(applicable);
        if (!ids.is_array()) return true;
        for (const auto& id : ids) {
            if ((id.is_number_integer() && id.get<int>() == hotelId) ||
                (id.is_string() && toIntSafe(id.get<std::string>()) == hotelId)) {
                return true;
            }
        }
        return false;
    } catch (...) {
        const std::string needle = std::to_string(hotelId);
        return applicable.find(needle) != std::string::npos;
    }
}

void attachActiveCoupon(Database& db, json& hotel) {
    const int hotelId = toIntSafe(jsonValueText(hotel, "id"));
    const double price = toDoubleSafe(jsonValueText(hotel, "price_from"), 0.0);
    if (hotelId <= 0 || price <= 0.0) return;

    auto rows = db.query(R"sql(
        SELECT *
        FROM coupons
        WHERE is_active=1
          AND (valid_to IS NULL OR valid_to='' OR valid_to >= date('now'))
          AND (usage_limit IS NULL OR usage_limit=0 OR usage_count < usage_limit)
        ORDER BY discount_value DESC, id DESC;
    )sql");

    for (const auto& row : rows) {
        if (!couponAppliesToHotel(rowValue(row, "applicable_hotels", "[]"), hotelId)) continue;

        const std::string type = rowValue(row, "discount_type", "percentage");
        const double value = toDoubleSafe(rowValue(row, "discount_value"));
        double discount = type == "percentage" ? price * (value / 100.0) : value;
        const double maxDiscount = toDoubleSafe(rowValue(row, "max_discount"), 0.0);
        if (maxDiscount > 0.0 && discount > maxDiscount) discount = maxDiscount;
        if (discount <= 0.0) continue;
        if (discount > price) discount = price;

        json coupon = {
            {"id", rowValue(row, "id")},
            {"code", rowValue(row, "code")},
            {"description", rowValue(row, "description", "Limited Time Offer")},
            {"discount_type", type},
            {"discount_value", value},
            {"discount_amount", discount},
            {"valid_to", rowValue(row, "valid_to")},
            {"old_price", price},
            {"discounted_price", price - discount}
        };
        hotel["coupon"] = coupon;
        hotel["active_coupon_code"] = coupon["code"];
        hotel["active_coupon_discount_type"] = type;
        hotel["active_coupon_discount_value"] = value;
        hotel["coupon_old_price"] = price;
        hotel["discounted_price"] = price - discount;
        break;
    }
}

struct CouponResult {
    bool valid = false;
    int id = 0;
    std::string code;
    std::string message;
    double discount = 0.0;
};

CouponResult evaluateCoupon(Database& db, int hotel_id, double subtotal, const std::string& raw_code) {
    CouponResult result;
    result.code = Utils::toUpper(Utils::trim(raw_code));
    if (result.code.empty()) {
        result.message = "No coupon code entered";
        return result;
    }

    auto rows = db.query(R"sql(
        SELECT * FROM coupons
        WHERE UPPER(code)=UPPER(?)
          AND is_active=1
          AND (valid_to IS NULL OR valid_to='' OR valid_to >= date('now'))
          AND (usage_limit IS NULL OR usage_limit=0 OR usage_count < usage_limit)
        LIMIT 1;
    )sql", {result.code});
    if (rows.empty()) {
        result.message = "Coupon is expired, inactive, or fully used";
        return result;
    }

    const DBRow& row = rows[0];
    const std::string applicable = rowValue(row, "applicable_hotels", "[]");
    if (!applicable.empty() && applicable != "[]") {
        bool applies = false;
        try {
            auto ids = json::parse(applicable);
            if (ids.is_array()) {
                for (const auto& id : ids) {
                    if ((id.is_number_integer() && id.get<int>() == hotel_id) ||
                        (id.is_string() && toIntSafe(id.get<std::string>()) == hotel_id)) {
                        applies = true;
                        break;
                    }
                }
            }
        } catch (...) {}
        if (!applies) {
            result.message = "Coupon is not valid for this hotel";
            return result;
        }
    }

    const double min_amount = toDoubleSafe(rowValue(row, "min_booking_amount"));
    if (subtotal < min_amount) {
        result.message = "Minimum booking amount is not reached";
        return result;
    }

    const std::string type = rowValue(row, "discount_type", "percentage");
    const double value = toDoubleSafe(rowValue(row, "discount_value"));
    if (type == "percentage") result.discount = subtotal * (value / 100.0);
    else result.discount = value;

    const double max_discount = toDoubleSafe(rowValue(row, "max_discount"), 0.0);
    if (max_discount > 0 && result.discount > max_discount) result.discount = max_discount;
    if (result.discount > subtotal) result.discount = subtotal;
    if (result.discount <= 0.0) {
        result.message = "Coupon has no discount value";
        return result;
    }

    result.valid = true;
    result.id = toIntSafe(rowValue(row, "id"));
    result.code = rowValue(row, "code", result.code);
    result.message = "Coupon applied";
    return result;
}

double roundMoney(double value) {
    return std::round(value * 100.0) / 100.0;
}

std::string appSetting(Database& db, const std::string& key, const std::string& fallback = "") {
    std::string value = db.queryString("SELECT value FROM app_settings WHERE key=?;", {key});
    return value.empty() ? fallback : value;
}

double commissionRate(Database& db) {
    double rate = toDoubleSafe(appSetting(db, "platform_commission_rate", "15"), 15.0);
    if (rate < 0.0) rate = 0.0;
    if (rate > 60.0) rate = 60.0;
    return rate;
}

json commissionPreview(Database& db, double totalAmount) {
    const double rate = commissionRate(db);
    const double commission = roundMoney(totalAmount * rate / 100.0);
    const double hotelAmount = roundMoney(totalAmount - commission);
    return {
        {"commission_rate", rate},
        {"commission_amount", commission},
        {"platform_fee", commission},
        {"hotel_amount", hotelAmount}
    };
}

std::string urlEncode(const std::string& value) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~' || c == '@') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ') {
            out += "%20";
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 15]);
        }
    }
    return out;
}

std::string buildUpiUri(const std::string& upiId,
                        const std::string& merchantName,
                        double amount,
                        const std::string& note) {
    std::string cleanUpi = Utils::trim(upiId);
    std::string cleanMerchant = Utils::trim(merchantName.empty() ? "StaySphere" : merchantName);
    if (cleanUpi.empty()) cleanUpi = "staysphere@upi";
    std::ostringstream amountStream;
    amountStream << std::fixed << std::setprecision(2) << roundMoney(amount);
    std::string amountText = amountStream.str();
    while (amountText.size() > 1 && amountText.back() == '0') amountText.pop_back();
    if (!amountText.empty() && amountText.back() == '.') amountText.pop_back();
    return "upi://pay?pa=" + urlEncode(cleanUpi) +
           "&pn=" + urlEncode(cleanMerchant) +
           "&am=" + urlEncode(amountText) +
           "&cu=INR" +
           "&tn=" + urlEncode(note);
}

bool isValidUpiId(const std::string& value) {
    const std::string upi = Utils::trim(value);
    const size_t at = upi.find('@');
    if (at == std::string::npos || at == 0 || at == upi.size() - 1) return false;
    if (upi.find('@', at + 1) != std::string::npos) return false;
    if (upi.size() > 80) return false;
    for (char c : upi) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '@' || c == '.' || c == '_' || c == '-') continue;
        return false;
    }
    return true;
}

void setAppSetting(Database& db, const std::string& key, const std::string& value) {
    db.execute(R"sql(
        INSERT INTO app_settings (key, value, updated_at)
        VALUES (?, ?, datetime('now','localtime'))
        ON CONFLICT(key) DO UPDATE SET value=excluded.value, updated_at=excluded.updated_at;
    )sql", {key, value});
}

int settingInt(Database& db, const std::string& key, int fallback) {
    int value = toIntSafe(appSetting(db, key, std::to_string(fallback)), fallback);
    return value < 0 ? fallback : value;
}

bool commissionCollectionEnabled(Database& db) {
    const std::string value = Utils::toLower(appSetting(db, "commission_collection_enabled", "1"));
    return value == "1" || value == "true" || value == "yes" || value == "enabled";
}

json paymentConfigJson(Database& db, double amount = 0.0, const std::string& bookingRef = "") {
    const std::string upiId = appSetting(db, "platform_upi_id", "staysphere@upi");
    const std::string merchant = appSetting(db, "platform_merchant_name", "StaySphere");
    json config = {
        {"upi_id", upiId},
        {"merchant_name", merchant},
        {"commission_rate", commissionRate(db)}
    };
    if (amount > 0.0) {
        config["amount"] = roundMoney(amount);
        config["upi_uri"] = buildUpiUri(upiId, merchant, amount,
            bookingRef.empty() ? "StaySphere booking" : "StaySphere booking " + bookingRef);
    }
    return config;
}

json platformPaymentSettingsJson(Database& db) {
    const std::string upiId = appSetting(db, "platform_upi_id", "staysphere@upi");
    const std::string merchant = appSetting(db, "platform_merchant_name", "StaySphere");
    std::string qrPayload = appSetting(db, "platform_upi_qr_payload", "");
    if (qrPayload.empty()) {
        qrPayload = buildUpiUri(upiId, merchant, 0.0, "StaySphere commission");
        setAppSetting(db, "platform_upi_qr_payload", qrPayload);
    }
    return {
        {"upi_id", upiId},
        {"merchant_name", merchant},
        {"upi_qr_payload", qrPayload},
        {"upi_uri", qrPayload},
        {"commission_collection_enabled", commissionCollectionEnabled(db)},
        {"commission_rate", commissionRate(db)},
        {"active_days", settingInt(db, "commission_active_days", 30)},
        {"grace_until_days", settingInt(db, "commission_grace_until_days", 37)},
        {"suspension_days", settingInt(db, "commission_suspension_days", 60)},
        {"notification_template", appSetting(db, "commission_notification_template",
            "Your commission invoice is due. Please pay using the platform UPI QR and upload proof for verification.")}
    };
}

void recordCommissionAudit(Database& db,
                           const std::string& entityType,
                           const std::string& entityId,
                           int hotelId,
                           int actorId,
                           const std::string& action,
                           const json& payload) {
    db.execute(R"sql(
        INSERT INTO commission_audit_log (
            entity_type, entity_id, hotel_id, actor_id, action, event_payload
        ) VALUES (?, ?, ?, ?, ?, ?);
    )sql", {
        entityType,
        entityId,
        std::to_string(hotelId),
        std::to_string(actorId),
        action,
        payload.dump()
    });
}

void refreshCommissionDues(Database& db) {
    const double defaultRate = commissionRate(db);
    db.execute("UPDATE monthly_commission_ledger SET gross_revenue=0, commission_amount=0, pending_amount=0 WHERE status!='paid';");
    auto rows = db.query(R"sql(
        SELECT
            b.hotel_id AS hotel_id,
            b.booking_id AS booking_ref,
            strftime('%Y-%m', COALESCE(NULLIF(b.check_out_date,''), b.created_at)) AS period,
            CAST(strftime('%Y', COALESCE(NULLIF(b.check_out_date,''), b.created_at)) AS INTEGER) AS year,
            COALESCE(b.total_amount,0) AS gross_revenue,
            COALESCE(CASE
                WHEN b.commission_amount > 0 THEN b.commission_amount
                ELSE b.total_amount * COALESCE(NULLIF(b.commission_rate,0), ?) / 100.0
            END,0) AS commission_amount,
            COALESCE(b.commission_rate, ?) AS commission_rate
        FROM bookings b
        WHERE b.status!='cancelled';
    )sql", {std::to_string(defaultRate), std::to_string(defaultRate)});

    for (const auto& row : rows) {
        const int hotelId = toIntSafe(rowValue(row, "hotel_id"));
        const std::string bookingRef = rowValue(row, "booking_ref");
        const std::string period = rowValue(row, "period");
        if (hotelId <= 0 || bookingRef.empty() || period.empty()) continue;

        const std::string ledgerId = "COM-" + bookingRef;
        const double gross = roundMoney(toDoubleSafe(rowValue(row, "gross_revenue")));
        const double commission = roundMoney(toDoubleSafe(rowValue(row, "commission_amount")));
        const double rate = toDoubleSafe(rowValue(row, "commission_rate"), defaultRate);
        const int year = toIntSafe(rowValue(row, "year"));
        const std::string month = period.substr(5, 2);
        const std::string dueDate = db.queryString("SELECT date(? || '-01','+1 month','-1 day');", {period});
        const double paid = db.queryDouble(
            "SELECT COALESCE(paid_amount,0) FROM monthly_commission_ledger WHERE ledger_id=?;",
            {ledgerId});
        const double pending = roundMoney(std::max(0.0, commission - paid));
        const bool overdue = db.queryInt("SELECT CASE WHEN date(?) < date('now') THEN 1 ELSE 0 END;", {dueDate}) == 1;
        const std::string status = pending <= 0.0 ? "paid" : (overdue ? "overdue" : "pending");

        db.execute(R"sql(
            INSERT INTO monthly_commission_ledger (
                ledger_id, hotel_id, month, year, gross_revenue,
                commission_rate, commission_amount, pending_amount,
                paid_amount, status, due_date
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ON CONFLICT(ledger_id) DO UPDATE SET
                gross_revenue=excluded.gross_revenue,
                commission_rate=excluded.commission_rate,
                commission_amount=excluded.commission_amount,
                pending_amount=excluded.pending_amount,
                paid_amount=excluded.paid_amount,
                status=excluded.status,
                due_date=excluded.due_date,
                updated_at=datetime('now','localtime');
        )sql", {
            ledgerId,
            std::to_string(hotelId),
            month,
            std::to_string(year),
            std::to_string(gross),
            std::to_string(rate),
            std::to_string(commission),
            std::to_string(pending),
            std::to_string(paid),
            status,
            dueDate
        });
    }

    db.execute(R"sql(
        UPDATE monthly_commission_ledger
        SET status=CASE
                WHEN pending_amount <= 0 THEN 'paid'
                WHEN date(due_date) < date('now') THEN 'overdue'
                ELSE 'pending'
            END,
            updated_at=datetime('now','localtime')
        WHERE status!='paid';
    )sql");

    db.execute("DELETE FROM monthly_commission_ledger WHERE gross_revenue=0 AND commission_amount=0 AND paid_amount=0;");

    const bool enabled = commissionCollectionEnabled(db);
    const int graceUntilDays = settingInt(db, "commission_grace_until_days", 37);
    const int suspensionDays = settingInt(db, "commission_suspension_days", 60);
    auto hotels = db.query("SELECT id FROM hotels;");
    for (const auto& row : hotels) {
        const int hotelId = toIntSafe(rowValue(row, "id"));
        const double due = roundMoney(db.queryDouble(
            "SELECT COALESCE(SUM(pending_amount),0) FROM monthly_commission_ledger WHERE hotel_id=?;",
            {std::to_string(hotelId)}));
        const std::string dueDate = db.queryString(R"sql(
            SELECT COALESCE(MIN(due_date), '')
            FROM monthly_commission_ledger
            WHERE hotel_id=? AND pending_amount > 0;
        )sql", {std::to_string(hotelId)});
        int daysOverdue = 0;
        if (!dueDate.empty()) {
            daysOverdue = std::max(0, db.queryInt(
                "SELECT CAST(julianday(date('now')) - julianday(date(?)) AS INTEGER);",
                {dueDate}));
        }

        std::string commissionStatus = "Paid";
        std::string hotelStatus = "Active";
        if (due > 0.0) {
            commissionStatus = daysOverdue > 0 ? "Overdue" : "Pending";
            if (enabled) {
                if (daysOverdue >= suspensionDays) hotelStatus = "Suspended";
                else if (daysOverdue > graceUntilDays) hotelStatus = "Restricted";
            }
        }

        db.execute(R"sql(
            UPDATE hotels
            SET commission_due=?, due_date=?, days_overdue=?,
                commission_status=?, hotel_status=?, updated_at=datetime('now','localtime')
            WHERE id=?;
        )sql", {
            std::to_string(due),
            dueDate,
            std::to_string(daysOverdue),
            commissionStatus,
            hotelStatus,
            std::to_string(hotelId)
        });
    }
}

std::string hotelPaymentRestriction(Database& db, int hotelId) {
    refreshCommissionDues(db);
    return db.queryString("SELECT hotel_status FROM hotels WHERE id=?;", {std::to_string(hotelId)});
}

[[maybe_unused]] void updateBookingCommission(Database& db,
                             int bookingId,
                             const std::string& bookingRef,
                             int hotelId,
                             double totalAmount,
                             double rate,
                             const std::string& transactionRef) {
    const double commission = roundMoney(totalAmount * rate / 100.0);
    const double hotelAmount = roundMoney(totalAmount - commission);
    db.execute(R"sql(
        UPDATE bookings
        SET commission_rate=?, commission_amount=?, hotel_amount=?,
            transaction_id=?, updated_at=datetime('now','localtime')
        WHERE id=?;
    )sql", {
        std::to_string(rate),
        std::to_string(commission),
        std::to_string(hotelAmount),
        transactionRef,
        std::to_string(bookingId)
    });

    const int payoutExists = db.queryInt(
        "SELECT COUNT(*) FROM hotel_payouts WHERE booking_id=?;",
        {std::to_string(bookingId)});
    if (payoutExists == 0) {
        db.execute(R"sql(
            INSERT INTO hotel_payouts (
                payout_id, hotel_id, booking_id, booking_ref,
                gross_amount, commission_amount, payout_amount,
                status, transaction_ref
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, 'pending', ?);
        )sql", {
            "PO-" + bookingRef,
            std::to_string(hotelId),
            std::to_string(bookingId),
            bookingRef,
            std::to_string(totalAmount),
            std::to_string(commission),
            std::to_string(hotelAmount),
            transactionRef
        });
    } else {
        db.execute(R"sql(
            UPDATE hotel_payouts
            SET gross_amount=?, commission_amount=?, payout_amount=?,
                transaction_ref=?
            WHERE booking_id=?;
        )sql", {
            std::to_string(totalAmount),
            std::to_string(commission),
            std::to_string(hotelAmount),
            transactionRef,
            std::to_string(bookingId)
        });
    }
}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
APIRouter::APIRouter(crow::SimpleApp& app,
                     std::shared_ptr<Database>  db)
    : app_(app), db_(db) {
    auth_          = std::make_shared<AuthManager>(db_);
    hotels_        = std::make_shared<HotelManager>(db_);
    images_        = std::make_shared<ImageManager>(db_);
    uploads_       = std::make_shared<UploadManager>("backend/uploads");
    rooms_         = std::make_shared<RoomManager>(db_);
    bookings_      = std::make_shared<BookingManager>(db_);
    billing_       = std::make_shared<BillingManager>(db_);
    cancellations_ = std::make_shared<CancellationManager>(db_);
    search_        = std::make_shared<SearchManager>(db_);
    reports_       = std::make_shared<ReportManager>(db_);
    receipts_      = std::make_shared<ReceiptManager>(db_, "receipts/");
    backup_        = std::make_shared<BackupManager>(db_, "data/hotel.db", "data/backups/");
    ai_            = std::make_shared<AIManager>(db_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Response helpers
// ─────────────────────────────────────────────────────────────────────────────
crow::response APIRouter::jsonOk(const json& data, const std::string& msg) {
    crow::response res(200, successResponse(data, msg).dump());
    res.add_header("Content-Type", "application/json");
    res.add_header("Access-Control-Allow-Origin",  "*");
    res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
    return res;
}
crow::response APIRouter::jsonErr(const std::string& msg, int status) {
    crow::response res(status, errorResponse(msg, status).dump());
    res.add_header("Content-Type", "application/json");
    res.add_header("Access-Control-Allow-Origin",  "*");
    res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    res.add_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
    return res;
}

std::optional<Session> APIRouter::getSession(const crow::request& req) {
    std::string auth = req.get_header_value("Authorization");
    if (auth.size() <= 7 || auth.substr(0,7) != "Bearer ") return std::nullopt;
    return auth_->validateToken(auth.substr(7));
}

bool APIRouter::requireAuth(const crow::request& /*req*/, crow::response& /*res*/) {
    // Authentication disabled - always return true
    return true;
}
bool APIRouter::requireAuth(const crow::request& /*req*/) {
    // Authentication disabled - always return true
    return true;
}
bool APIRouter::requireAdmin(const crow::request& /*req*/, crow::response& /*res*/) {
    // Admin authentication disabled - always return true
    return true;
}
bool APIRouter::requireAdmin(const crow::request& /*req*/) {
    // Admin authentication disabled - always return true
    return true;
}

bool APIRouter::canManageHotel(const Session& session, int hotel_id) const {
    if (hotel_id <= 0) return false;
    if (session.role == UserRole::SUPER_ADMIN) return true;
    if (session.role != UserRole::HOTEL_MANAGER && session.role != UserRole::RECEPTIONIST) return false;
    if (session.hotel_id == hotel_id) return true;
    if (session.role == UserRole::HOTEL_MANAGER) {
        int ownerId = db_->queryInt(
            "SELECT owner_id FROM hotels WHERE id=?;",
            {std::to_string(hotel_id)});
        return ownerId > 0 && ownerId == session.user_id;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
void APIRouter::registerAll() {
    registerCorsHeaders();
    registerPublicRoutes();
    registerAuthRoutes();
    registerHotelRoutes();
    registerRoomRoutes();
    registerBookingRoutes();
    registerPaymentRoutes();
    registerBillingRoutes();
    registerCancellationRoutes();
    registerSearchRoutes();
    registerReportRoutes();
    registerReceiptRoutes();
    registerBackupRoutes();
    registerAIRoutes();
    registerUserRoutes();
    registerStaticRoutes();
    std::cout << "[API] All routes registered.\n";
}

void APIRouter::registerCorsHeaders() {
    CROW_ROUTE(app_, "/api/<path>")
    .methods(crow::HTTPMethod::Options)
    ([](const crow::request&, const std::string&) {
        crow::response res(204);
        res.add_header("Access-Control-Allow-Origin",  "*");
        res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
        return res;
    });
}

// ═════════════════════════════════════════════════════════════════════════════
//  PHASE 3 — AUTH ROUTES
// ═════════════════════════════════════════════════════════════════════════════
void APIRouter::registerPublicRoutes() {
    CROW_ROUTE(app_, "/api/public/hotels")
    ([this](const crow::request& req) {
        refreshCommissionDues(*db_);
        std::string q = firstParam(req, "q");
        std::string location = firstParam(req, "location");
        std::string min_price = firstParam(req, "min_price");
        std::string max_price = firstParam(req, "max_price");
        std::string rating = firstParam(req, "rating");

        std::string sql =
            "SELECT h.*, "
            "COALESCE((SELECT COALESCE(NULLIF(hi.image_path,''), NULLIF(hi.image_url,'')) "
            "FROM hotel_images hi WHERE hi.hotel_id=h.id ORDER BY hi.is_primary DESC, hi.id ASC LIMIT 1), "
            "NULLIF(h.image,''), NULLIF(h.image_url,''), '') AS image_path, "
            "COALESCE(NULLIF(h.price_from,0),(SELECT MIN(current_price) FROM room_types WHERE hotel_id=h.id),0) AS price_from, "
            "COALESCE((SELECT ROUND(AVG(rating),1) FROM reviews WHERE hotel_id=h.id), h.review_score) AS review_score, "
            "(SELECT COUNT(*) FROM reviews WHERE hotel_id=h.id) AS review_count "
            "FROM hotels h WHERE h.is_active=1 AND h.status='approved' "
            "AND COALESCE(h.hotel_status,'Active')!='Suspended'";
        std::vector<std::string> params;

        if (!q.empty()) {
            sql += " AND (LOWER(h.name) LIKE LOWER(?) OR LOWER(h.location) LIKE LOWER(?) OR LOWER(h.description) LIKE LOWER(?))";
            params.push_back("%" + q + "%");
            params.push_back("%" + q + "%");
            params.push_back("%" + q + "%");
        }
        if (!location.empty()) {
            sql += " AND LOWER(h.location) LIKE LOWER(?)";
            params.push_back("%" + location + "%");
        }
        if (!min_price.empty()) {
            sql += " AND COALESCE(NULLIF(h.price_from,0),(SELECT MIN(current_price) FROM room_types WHERE hotel_id=h.id),0) >= ?";
            params.push_back(min_price);
        }
        if (!max_price.empty()) {
            sql += " AND COALESCE(NULLIF(h.price_from,0),(SELECT MIN(current_price) FROM room_types WHERE hotel_id=h.id),0) <= ?";
            params.push_back(max_price);
        }
        if (!rating.empty()) {
            sql += " AND h.star_rating >= ?";
            params.push_back(rating);
        }
        sql += " ORDER BY h.review_score DESC, h.name ASC LIMIT 50;";

        try {
            json hotels = dbRowsToJson(db_->query(sql, params));
            for (auto& hotel : hotels) {
                images_->hydrateHotelJson(hotel);
                attachActiveCoupon(*db_, hotel);
            }
            return jsonOk(hotels);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    CROW_ROUTE(app_, "/getHotels")
    ([this]() {
        try {
            refreshCommissionDues(*db_);
            json hotels = dbRowsToJson(db_->query(R"sql(
                SELECT h.*,
                       COALESCE((SELECT COALESCE(NULLIF(hi.image_path,''), NULLIF(hi.image_url,''))
                                 FROM hotel_images hi
                                 WHERE hi.hotel_id=h.id
                                 ORDER BY hi.is_primary DESC, hi.id ASC LIMIT 1),
                                NULLIF(h.image,''), NULLIF(h.image_url,''), '') AS image_path,
                       COALESCE(NULLIF(h.price_from,0),(SELECT MIN(current_price) FROM room_types WHERE hotel_id=h.id),0) AS price_from,
                       COALESCE((SELECT ROUND(AVG(rating),1) FROM reviews WHERE hotel_id=h.id), h.review_score) AS review_score
                FROM hotels h
                WHERE h.is_active=1 AND h.status='approved'
                  AND COALESCE(h.hotel_status,'Active')!='Suspended'
                ORDER BY h.review_score DESC, h.name ASC;
            )sql"));
            for (auto& hotel : hotels) {
                images_->hydrateHotelJson(hotel);
                attachActiveCoupon(*db_, hotel);
            }
            return jsonOk(hotels);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    CROW_ROUTE(app_, "/getHotelImages/<int>")
    ([this](int id) {
        return jsonOk(images_->getHotelImagesJson(id));
    });

    CROW_ROUTE(app_, "/api/public/hotels/<int>")
    ([this](int id) {
        try {
            refreshCommissionDues(*db_);
            auto rows = db_->query(
                "SELECT h.*, "
                "COALESCE(NULLIF(h.price_from,0),(SELECT MIN(current_price) FROM room_types WHERE hotel_id=h.id),0) AS price_from, "
                "COALESCE((SELECT ROUND(AVG(rating),1) FROM reviews WHERE hotel_id=h.id), h.review_score) AS review_score "
                "FROM hotels h WHERE h.id=? AND h.is_active=1 AND h.status='approved' "
                "AND COALESCE(h.hotel_status,'Active')!='Suspended';",
                {std::to_string(id)});
            if (rows.empty()) return jsonErr("Hotel not found", 404);

            json hotel = dbRowToJson(rows[0]);
            images_->hydrateHotelJson(hotel);
            attachActiveCoupon(*db_, hotel);
            hotel["rooms"] = dbRowsToJson(db_->query(
                "SELECT rt.*, "
                "(SELECT image_url FROM room_images ri WHERE ri.room_type_id=rt.id ORDER BY is_primary DESC LIMIT 1) AS image_url, "
                "(SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='available') AS available_count "
                "FROM room_types rt WHERE rt.hotel_id=? ORDER BY rt.current_price ASC;",
                {std::to_string(id)}));
            for (auto& room : hotel["rooms"]) {
                room["images"] = roomImagesJson(*db_, toIntSafe(jsonValueText(room, "id")));
            }
            hotel["reviews"] = dbRowsToJson(db_->query(
                "SELECT r.id, r.rating, r.comment, r.images, r.manager_reply, r.replied_at, r.created_at, u.full_name AS author "
                "FROM reviews r LEFT JOIN users u ON r.user_id=u.id "
                "WHERE r.hotel_id=? ORDER BY r.created_at DESC LIMIT 8;",
                {std::to_string(id)}));
            hotel["similar"] = dbRowsToJson(db_->query(
                "SELECT id,name,location,image,image_url,price_from,review_score,star_rating "
                "FROM hotels WHERE id!=? AND status='approved' AND is_active=1 "
                "AND COALESCE(hotel_status,'Active')!='Suspended' "
                "ORDER BY review_score DESC LIMIT 3;",
                {std::to_string(id)}));
            for (auto& similar : hotel["similar"]) images_->hydrateHotelJson(similar);
        hotel["activities_list"] = dbRowsToJson(db_->query(
            "SELECT id, name, price, description FROM activities ORDER BY id;"
        ));
            return jsonOk(hotel);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    CROW_ROUTE(app_, "/api/public/destinations")
    ([this]() {
        refreshCommissionDues(*db_);
        auto rows = db_->query(
            "SELECT location, COUNT(*) AS stays, MIN(price_from) AS price_from, "
            "MAX(COALESCE(NULLIF(image,''), NULLIF(image_url,''), '')) AS image_url FROM hotels "
            "WHERE is_active=1 AND status='approved' AND COALESCE(hotel_status,'Active')!='Suspended' "
            "GROUP BY location ORDER BY stays DESC LIMIT 8;");
        return jsonOk(dbRowsToJson(rows));
    });

    // GET /api/activities — public catalogue of paid add-ons (Spa, Boat Ride, etc.)
    CROW_ROUTE(app_, "/api/activities")
    ([this]() {
        auto rows = db_->query(
            "SELECT id, name, price, description FROM activities ORDER BY id;");
        return jsonOk(dbRowsToJson(rows));
    });
}

void APIRouter::registerAuthRoutes() {

    // GET /api/health
    CROW_ROUTE(app_, "/api/health")
    ([this]() {
        return jsonOk({{"status","ok"},{"phase","3+4"},{"message","Hotel MS running!"}});
    });

    // POST /api/auth/login
    CROW_ROUTE(app_, "/api/auth/login")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            std::string username = body.value("username","");
            std::string password = body.value("password","");
            if (username.empty() || password.empty())
                return jsonErr("Username and password required", 400);

            auto session = auth_->login(username, password);
            if (!session) return jsonErr("Invalid credentials", 401);

            auto rows = db_->query(
                "SELECT id,username,full_name,email,phone,role,hotel_id,loyalty_points FROM users WHERE username=?;",
                {username});
            json user = {};
            if (!rows.empty()) for (auto& [k,v] : rows[0]) user[k] = v;

            return jsonOk({
                {"token",      session->token},
                {"expires_at", session->expires_at},
                {"user",       user}
            }, "Login successful");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // POST /api/auth/register
    CROW_ROUTE(app_, "/api/auth/register")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            std::string roleName = body.value("role", "guest");
            if (roleName == "manager") roleName = "hotel_manager";
            if (roleName != "guest" && roleName != "hotel_manager")
                return jsonErr("Public registration supports guest or hotel_manager", 400);

            std::string password = body.value("password", "");
            User u;
            u.username  = body.value("username", "");
            u.full_name = body.value("full_name", "");
            u.email     = body.value("email", "");
            u.phone     = body.value("phone", "");
            u.role      = roleFromString(roleName);
            u.hotel_id  = 0;

            if (u.username.empty() || password.empty() || u.full_name.empty())
                return jsonErr("username, full_name and password required", 400);
            if (password.size() < 6)
                return jsonErr("Password must be at least 6 characters", 400);
            if (u.role == UserRole::HOTEL_MANAGER && body.value("location", "").empty())
                return jsonErr("location required for hotel manager registration", 400);
            if (!auth_->createUser(u, password))
                return jsonErr("Username already exists", 409);

            int userId = db_->queryInt("SELECT id FROM users WHERE username=?;", {u.username});
            json result = {{"user_id", userId}, {"role", roleName}};

            if (u.role == UserRole::HOTEL_MANAGER) {
                Hotel h;
                h.name = body.value("hotel_name", body.value("company", "New Partner Stay"));
                h.location = body.value("location", "");
                h.address = body.value("address", "");
                h.phone = u.phone;
                h.email = u.email;
                h.owner_id = userId;
                h.status = "pending";
                h.is_active = true;
                h.description = body.value("description", "Partner hotel submitted for admin verification.");
                h.amenities = bodyArrayText(body, "amenities", "[\"WiFi\",\"AC\"]");
                h.image_url = body.value("image_path", body.value("image", body.value("image_url", "")));
                h.property_type = body.value("property_type", "Hotel");
                h.price_from = body.value("price_from", body.value("price", 3500.0));
                h.star_rating = body.value("star_rating", body.value("rating", 4.5));
                h.review_score = 0.0;
                int hotelId = hotels_->createHotel(h);
                updateHotelPartnerFields(*db_, hotelId, body);
                recordHotelAssets(*db_, hotelId, body);
                db_->execute("UPDATE users SET hotel_id=? WHERE id=?;", {std::to_string(hotelId), std::to_string(userId)});
                result["hotel_id"] = hotelId;
                result["verification_status"] = "pending";
            }

            return jsonOk(result, "Registration successful");
        } catch (const std::exception& e) {
            return jsonErr(std::string("Invalid registration: ") + e.what(), 400);
        }
    });

    // POST /api/auth/logout
    CROW_ROUTE(app_, "/api/auth/logout")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.size() > 7) auth_->logout(auth.substr(7));
        return jsonOk({}, "Logged out");
    });

    // GET /api/auth/me
    CROW_ROUTE(app_, "/api/auth/me")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        auto rows = db_->query(
            "SELECT id,username,full_name,email,phone,role,hotel_id,loyalty_points,last_login FROM users WHERE id=?;",
            {std::to_string(s->user_id)});
        if (rows.empty()) return jsonErr("User not found", 404);
        json u = {};
        for (auto& [k,v] : rows[0]) u[k] = v;
        return jsonOk(u);
    });

    // PUT /api/auth/profile
    CROW_ROUTE(app_, "/api/auth/profile")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        try {
            auto body = json::parse(req.body);
            std::string fn = body.value("full_name", "");
            std::string em = body.value("email", "");
            std::string ph = body.value("phone", "");
            db_->execute("UPDATE users SET full_name=?, email=?, phone=? WHERE id=?;",
                {fn, em, ph, std::to_string(s->user_id)});
            return jsonOk({}, "Profile updated successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // GET /api/auth/loyalty
    CROW_ROUTE(app_, "/api/auth/loyalty")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        int points = db_->queryInt("SELECT loyalty_points FROM users WHERE id=?;", {std::to_string(s->user_id)});
        return jsonOk({{"loyalty_points", points}});
    });

    // POST /api/auth/change-password
    CROW_ROUTE(app_, "/api/auth/change-password")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        try {
            auto body   = json::parse(req.body);
            std::string old_pw = body.value("old_password","");
            std::string new_pw = body.value("new_password","");
            if (old_pw.empty() || new_pw.empty())
                return jsonErr("Both old_password and new_password required", 400);
            if (new_pw.size() < 6)
                return jsonErr("New password must be at least 6 characters", 400);
            if (!auth_->changePassword(s->user_id, old_pw, new_pw))
                return jsonErr("Old password incorrect", 401);
            return jsonOk({}, "Password changed");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // GET /api/admin/users
    CROW_ROUTE(app_, "/api/admin/users")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        auto rows = db_->query(
            "SELECT id,username,full_name,role,hotel_id,"
            "is_locked,failed_attempts,last_login,created_at FROM users;");
        json arr = json::array();
        for (auto& row : rows) {
            json u; for (auto& [k,v]:row) u[k]=v;
            arr.push_back(u);
        }
        return jsonOk(arr);
    });

    // POST /api/admin/users
    CROW_ROUTE(app_, "/api/admin/users")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            User u;
            u.username  = body.value("username","");
            u.full_name = body.value("full_name","");
            u.hotel_id  = body.value("hotel_id", 0);
            u.role      = roleFromString(body.value("role","receptionist"));
            std::string pw = body.value("password","");
            if (u.username.empty()||pw.empty()||u.full_name.empty())
                return jsonErr("username, full_name, password required", 400);
            if (!auth_->createUser(u, pw))
                return jsonErr("Username already exists", 409);
            return jsonOk({}, "User created");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // POST /api/admin/users/unlock
    CROW_ROUTE(app_, "/api/admin/users/unlock")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string username = body.value("username","");
            if (username.empty()) return jsonErr("username required", 400);
            auth_->unlockUser(username);
            return jsonOk({}, "User unlocked");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });
}

// ═════════════════════════════════════════════════════════════════════════════
//  PHASE 4 — HOTEL ROUTES
// ═════════════════════════════════════════════════════════════════════════════
void APIRouter::registerHotelRoutes() {

    auto uploadHotelHandler = [this](const crow::request& req) {
        auto session = getSession(req);
        if (!session) return jsonErr("Unauthorized", 401);

        try {
            crow::multipart::message msg(req);
            std::string hotel_id_text = multipartField(msg, "hotel_id");
            if (hotel_id_text.empty()) hotel_id_text = firstParam(req, "hotel_id");

            int hotel_id = hotel_id_text.empty() ? session->hotel_id : std::atoi(hotel_id_text.c_str());
            if (hotel_id <= 0) return jsonErr("hotel_id required", 400);
            if (!canManageHotel(*session, hotel_id)) return jsonErr("Forbidden", 403);
            if (!hotels_->getHotelById(hotel_id)) return jsonErr("Hotel not found", 404);

            const bool requested_primary = boolParam(multipartField(msg, "is_primary"));
            json uploaded = json::array();
            int saved_count = 0;

            for (const auto& part : msg.parts) {
                const std::string original_name = multipartFilename(part);
                if (original_name.empty()) continue;

                UploadResult saved = uploads_->saveHotelImage(
                    original_name,
                    multipartContentType(part),
                    part.body);

                const bool make_primary = requested_primary && saved_count == 0;
                const int image_id = images_->addHotelImage(hotel_id, saved.public_url, make_primary);
                json item = saved.toJson();
                item["id"] = image_id;
                uploaded.push_back(item);
                ++saved_count;
            }

            if (uploaded.empty()) return jsonErr("No image files found in upload", 400);

            return jsonOk({
                {"hotel_id", hotel_id},
                {"uploaded", uploaded},
                {"images", images_->getHotelImagesJson(hotel_id)}
            }, "Hotel image uploaded successfully");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    };

    CROW_ROUTE(app_, "/uploadHotel")
    .methods(crow::HTTPMethod::Post)
    (uploadHotelHandler);

    CROW_ROUTE(app_, "/api/uploadHotel")
    .methods(crow::HTTPMethod::Post)
    (uploadHotelHandler);

    auto uploadRoomHandler = [this](const crow::request& req) {
        auto session = getSession(req);
        if (!session) return jsonErr("Unauthorized", 401);

        try {
            crow::multipart::message msg(req);
            std::string room_type_id_text = multipartField(msg, "room_type_id");
            if (room_type_id_text.empty()) room_type_id_text = firstParam(req, "room_type_id");
            int room_type_id = room_type_id_text.empty() ? 0 : std::atoi(room_type_id_text.c_str());
            if (room_type_id <= 0) return jsonErr("room_type_id required", 400);

            int hotel_id = db_->queryInt("SELECT hotel_id FROM room_types WHERE id=?;", {std::to_string(room_type_id)});
            if (hotel_id <= 0) return jsonErr("Room type not found", 404);
            if (!canManageHotel(*session, hotel_id)) return jsonErr("Forbidden", 403);

            const bool requested_primary = boolParam(multipartField(msg, "is_primary"));
            json uploaded = json::array();
            int saved_count = 0;

            for (const auto& part : msg.parts) {
                const std::string original_name = multipartFilename(part);
                if (original_name.empty()) continue;

                UploadResult saved = uploads_->saveHotelImage(
                    original_name,
                    multipartContentType(part),
                    part.body);

                const bool make_primary = requested_primary && saved_count == 0;
                const int image_id = addRoomImageRecord(*db_, room_type_id, saved.public_url, make_primary);
                json item = saved.toJson();
                item["id"] = image_id;
                uploaded.push_back(item);
                ++saved_count;
            }

            if (uploaded.empty()) return jsonErr("No image files found in upload", 400);

            return jsonOk({
                {"hotel_id", hotel_id},
                {"room_type_id", room_type_id},
                {"uploaded", uploaded},
                {"images", roomImagesJson(*db_, room_type_id)}
            }, "Room image uploaded successfully");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    };

    CROW_ROUTE(app_, "/uploadRoom")
    .methods(crow::HTTPMethod::Post)
    (uploadRoomHandler);

    CROW_ROUTE(app_, "/api/uploadRoom")
    .methods(crow::HTTPMethod::Post)
    (uploadRoomHandler);

    // GET /api/hotels
    CROW_ROUTE(app_, "/api/hotels")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto hotels = hotels_->getAllHotels();
        json arr = json::array();
        for (auto& h : hotels) {
            json hotel = h.toJson();
            images_->hydrateHotelJson(hotel);
            arr.push_back(hotel);
        }
        return jsonOk(arr);
    });

    // GET /api/hotels/:id
    CROW_ROUTE(app_, "/api/hotels/<int>")
    ([this](const crow::request& req, int id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto rows = db_->query("SELECT * FROM hotels WHERE id=?;", {std::to_string(id)});
        if (rows.empty()) return jsonErr("Hotel not found", 404);
        json hotel = dbRowToJson(rows[0]);
        images_->hydrateHotelJson(hotel);
        hotel["documents"] = dbRowsToJson(db_->query(
            "SELECT id, document_type, document_url, status, uploaded_at, reviewed_at FROM hotel_documents WHERE hotel_id=? ORDER BY uploaded_at DESC;",
            {std::to_string(id)}));
        return jsonOk(hotel);
    });

    // GET /api/hotels/:id/dashboard
    CROW_ROUTE(app_, "/api/hotels/<int>/dashboard")
    ([this](const crow::request& req, int id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(hotels_->getDashboardSummary(id));
    });

    // GET /api/hotels/:id/occupancy
    CROW_ROUTE(app_, "/api/hotels/<int>/occupancy")
    ([this](const crow::request& req, int id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(hotels_->getOccupancyStats(id).toJson());
    });

    // POST /api/hotels  (admin only)
    CROW_ROUTE(app_, "/api/hotels")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        crow::response res;
        if (!requireAdmin(req, res)) return res;
        try {
            auto body = json::parse(req.body);
            Hotel h;
            h.name     = body.value("name","");
            h.location = body.value("location","");
            h.address  = body.value("address","");
            h.phone    = body.value("phone","");
            h.email    = body.value("email","");
            h.website  = body.value("website","");
            h.description = body.value("description", "");
            h.amenities = bodyArrayText(body, "amenities", "[]");
            h.image_url = body.value("image_path", body.value("image", body.value("image_url", "")));
            h.price_from = body.value("price_from", body.value("price", 0.0));
            h.star_rating = body.value("star_rating", body.value("rating", 5.0));
            if (h.name.empty() || h.location.empty())
                return jsonErr("name and location required", 400);
            int id = hotels_->createHotel(h);
            h.id = id;
            recordHotelAssets(*db_, id, body);
            return jsonOk(h.toJson(), "Hotel created");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // PUT /api/hotels/:id  (admin only)
    CROW_ROUTE(app_, "/api/hotels/<int>")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, id)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            auto existing = hotels_->getHotelById(id);
            if (!existing) return jsonErr("Hotel not found", 404);
            existing->name     = body.value("name",     existing->name);
            existing->location = body.value("location", existing->location);
            existing->address  = body.value("address",  existing->address);
            existing->phone    = body.value("phone",    existing->phone);
            existing->email    = body.value("email",    existing->email);
            existing->website  = body.value("website",  existing->website);
            existing->description = body.value("description", existing->description);
            existing->amenities = bodyArrayText(body, "amenities", existing->amenities);
            existing->image_url = body.value("image_path", body.value("image", body.value("image_url", existing->image_url)));
            existing->property_type = body.value("property_type", existing->property_type);
            existing->price_from = body.value("price_from", existing->price_from);
            existing->star_rating = body.value("star_rating", existing->star_rating);
            hotels_->updateHotel(*existing);
            updateHotelPartnerFields(*db_, id, body);

            // Handle UPI configuration
            if (body.contains("upi_id")) {
                std::string upi_id = body["upi_id"].is_string() ? body["upi_id"].get<std::string>() : "";
                db_->execute("UPDATE hotels SET upi_id=? WHERE id=?;", {upi_id, std::to_string(id)});
            }
            if (body.contains("upi_qr_code")) {
                std::string upi_qr_code = body["upi_qr_code"].is_string() ? body["upi_qr_code"].get<std::string>() : "";
                db_->execute("UPDATE hotels SET upi_qr_code=? WHERE id=?;", {upi_qr_code, std::to_string(id)});
            }

            recordHotelAssets(*db_, id, body);
            return jsonOk(existing->toJson(), "Hotel updated");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // DELETE /api/hotels/:id  (admin only)
    CROW_ROUTE(app_, "/api/hotels/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int id) {
        crow::response res;
        if (!requireAdmin(req, res)) return res;
        hotels_->deleteHotel(id);
        return jsonOk({}, "Hotel deactivated");
    });

    // PUT /api/admin/hotels/:id/status (Approve/Reject Workflow)
    CROW_ROUTE(app_, "/api/admin/hotels/<int>/status")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req, int id) {
        crow::response res;
        if (!requireAdmin(req, res)) return res;
        try {
            auto body = json::parse(req.body);
            std::string status = body.value("status", "approved"); // pending, approved, rejected
            if (status != "pending" && status != "approved" && status != "rejected")
                return jsonErr("Invalid status", 400);
            db_->execute("UPDATE hotels SET status=? WHERE id=?;", {status, std::to_string(id)});
            auto session = getSession(req);
            if (status == "approved" || status == "rejected") {
                db_->execute(R"sql(
                    UPDATE hotel_documents
                    SET status=?, reviewed_by=?, reviewed_at=datetime('now','localtime')
                    WHERE hotel_id=?;
                )sql", {
                    status,
                    std::to_string(session ? session->user_id : 0),
                    std::to_string(id)
                });
            }
            return jsonOk({}, "Hotel status updated to: " + status);
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    CROW_ROUTE(app_, "/api/admin/hotels")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        std::string status = firstParam(req, "status", "all");
        std::string sql =
            "SELECT h.*, u.full_name AS owner_name, u.email AS owner_email, u.phone AS owner_phone, "
            "(SELECT COUNT(*) FROM hotel_documents d WHERE d.hotel_id=h.id) AS document_count "
            "FROM hotels h LEFT JOIN users u ON h.owner_id=u.id";
        std::vector<std::string> params;
        if (status != "all") {
            sql += " WHERE h.status=?";
            params.push_back(status);
        }
        sql += " ORDER BY h.created_at DESC;";
        json hotels = dbRowsToJson(db_->query(sql, params));
        for (auto& hotel : hotels) {
            images_->hydrateHotelJson(hotel);
            const int hotel_id = toIntSafe(hotel.value("id", "0"));
            hotel["documents"] = dbRowsToJson(db_->query(R"sql(
                SELECT id, document_type, document_url, status, uploaded_at, reviewed_at
                FROM hotel_documents
                WHERE hotel_id=?
                ORDER BY uploaded_at DESC, id DESC;
            )sql", {std::to_string(hotel_id)}));
        }
        return jsonOk(hotels);
    });

    CROW_ROUTE(app_, "/api/admin/analytics")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        refreshCommissionDues(*db_);
        json data = {
            {"total_hotels", db_->queryInt("SELECT COUNT(*) FROM hotels WHERE is_active=1;")},
            {"pending_hotels", db_->queryInt("SELECT COUNT(*) FROM hotels WHERE status='pending';")},
            {"total_users", db_->queryInt("SELECT COUNT(*) FROM users;")},
            {"total_bookings", db_->queryInt("SELECT COUNT(*) FROM bookings;")},
            {"monthly_bookings", db_->queryInt("SELECT COUNT(*) FROM bookings WHERE created_at >= date('now','start of month');")},
            {"revenue", db_->queryDouble("SELECT COALESCE(SUM(total_amount),0) FROM bookings WHERE status!='cancelled';")},
            {"platform_revenue", db_->queryDouble("SELECT COALESCE(SUM(commission_amount),0) FROM bookings WHERE payment_status='paid' AND status!='cancelled';")},
            {"commission_rate", commissionRate(*db_)},
            {"pending_payouts", db_->queryDouble("SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE status='pending';")},
            {"failed_transactions", db_->queryInt("SELECT COUNT(*) FROM payments WHERE status='failed';")},
            {"occupancy", db_->queryDouble(
                "SELECT CASE WHEN COUNT(*)=0 THEN 0 ELSE "
                "ROUND(SUM(CASE WHEN status='occupied' THEN 1 ELSE 0 END)*100.0/COUNT(*),1) END FROM rooms;")}
        };
        data["top_hotels"] = dbRowsToJson(db_->query(
            "SELECT h.name, h.location, COALESCE(SUM(b.total_amount),0) AS revenue, COUNT(b.id) AS bookings "
            "FROM hotels h LEFT JOIN bookings b ON b.hotel_id=h.id "
            "GROUP BY h.id ORDER BY revenue DESC LIMIT 5;"));
        return jsonOk(data);
    });

    CROW_ROUTE(app_, "/api/manager/hotels")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s || (s->role != UserRole::HOTEL_MANAGER && s->role != UserRole::RECEPTIONIST))
            return jsonErr("Manager login required", 403);
        try {
            auto body = json::parse(req.body);
            Hotel h;
            h.name = body.value("name", "");
            h.location = body.value("location", "");
            h.address = body.value("address", "");
            h.phone = body.value("phone", "");
            h.email = body.value("email", "");
            h.website = body.value("website", "");
            h.owner_id = s->user_id;
            h.status = "pending";
            h.description = body.value("description", "");
            h.amenities = bodyArrayText(body, "amenities", "[\"WiFi\",\"AC\"]");
            h.image_url = body.value("image_path", body.value("image", body.value("image_url", "")));
            h.property_type = body.value("property_type", "Hotel");
            h.price_from = body.value("price_from", body.value("price", 0.0));
            h.star_rating = body.value("star_rating", body.value("rating", 4.5));
            if (h.name.empty() || h.location.empty())
                return jsonErr("name and location required", 400);
            int hotelId = hotels_->createHotel(h);
            updateHotelPartnerFields(*db_, hotelId, body);
            recordHotelAssets(*db_, hotelId, body);
            db_->execute("UPDATE users SET hotel_id=? WHERE id=?;",
                         {std::to_string(hotelId), std::to_string(s->user_id)});
            h.id = hotelId;
            return jsonOk(h.toJson(), "Hotel submitted for admin approval");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    CROW_ROUTE(app_, "/api/manager/analytics")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s || (s->role != UserRole::HOTEL_MANAGER && s->role != UserRole::RECEPTIONIST && s->role != UserRole::SUPER_ADMIN))
            return jsonErr("Manager login required", 403);
        int hotel_id = std::atoi(firstParam(req, "hotel_id", std::to_string(s->hotel_id)).c_str());
        if (!canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        auto summary = hotels_->getDashboardSummary(hotel_id);
        summary["today_revenue"] = db_->queryDouble(
            "SELECT COALESCE(SUM(total_amount),0) FROM bookings WHERE hotel_id=? AND date(created_at)=date('now','localtime') AND status!='cancelled';",
            {std::to_string(hotel_id)});
        summary["monthly_revenue"] = db_->queryDouble(
            "SELECT COALESCE(SUM(total_amount),0) FROM bookings WHERE hotel_id=? AND created_at >= date('now','start of month') AND status!='cancelled';",
            {std::to_string(hotel_id)});
        summary["monthly_earnings"] = db_->queryDouble(
            "SELECT COALESCE(SUM(hotel_amount),0) FROM bookings WHERE hotel_id=? AND payment_status='paid' AND created_at >= date('now','start of month') AND status!='cancelled';",
            {std::to_string(hotel_id)});
        summary["pending_payouts"] = db_->queryDouble(
            "SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE hotel_id=? AND status='pending';",
            {std::to_string(hotel_id)});
        summary["completed_payouts"] = db_->queryDouble(
            "SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE hotel_id=? AND status='completed';",
            {std::to_string(hotel_id)});
        summary["pending_checkins"] = db_->queryInt(
            "SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status='active' AND check_in_date>=date('now');",
            {std::to_string(hotel_id)});
        return jsonOk(summary);
    });

    // POST /api/wishlist
    CROW_ROUTE(app_, "/api/wishlist")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        try {
            auto body = json::parse(req.body);
            int hotel_id = body.value("hotel_id", 0);
            db_->execute("INSERT OR IGNORE INTO wishlists (user_id, hotel_id) VALUES (?,?);",
                {std::to_string(user_id), std::to_string(hotel_id)});
            return jsonOk({}, "Added to wishlist");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // GET /api/wishlist
    CROW_ROUTE(app_, "/api/wishlist")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        auto rows = db_->query("SELECT h.* FROM hotels h JOIN wishlists w ON h.id = w.hotel_id WHERE w.user_id=?;", {std::to_string(user_id)});
        json arr = json::array();
        for(auto& row : rows) {
            json obj;
            for(auto& [k,v]: row) obj[k]=v;
            images_->hydrateHotelJson(obj);
            attachActiveCoupon(*db_, obj);
            arr.push_back(obj);
        }
        return jsonOk(arr);
    });

    // DELETE /api/wishlist/:hotel_id
    CROW_ROUTE(app_, "/api/wishlist/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        db_->execute("DELETE FROM wishlists WHERE user_id=? AND hotel_id=?;", {std::to_string(user_id), std::to_string(hotel_id)});
        return jsonOk({}, "Removed from wishlist");
    });

    // POST /api/hotels/:id/reviews
    CROW_ROUTE(app_, "/api/hotels/<int>/reviews")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        try {
            auto body = json::parse(req.body);
            int rating = body.value("rating", 5);
            std::string comment = body.value("comment", "");
            std::string images = bodyArrayText(body, "images", "[]");
            db_->execute("INSERT INTO reviews (user_id, hotel_id, rating, comment, images) VALUES (?,?,?,?,?);",
                {std::to_string(user_id), std::to_string(hotel_id), std::to_string(rating), comment, images});
            
            // Reward the user with Loyalty Points for leaving a review!
            db_->execute("UPDATE users SET loyalty_points = loyalty_points + 50 WHERE id=?;", {std::to_string(user_id)});
            return jsonOk({}, "Review posted! You earned 50 Loyalty Points.");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // GET /api/hotels/:id/reviews
    CROW_ROUTE(app_, "/api/hotels/<int>/reviews")
    ([this](const crow::request& /*req*/, int hotel_id) {
        auto rows = db_->query("SELECT r.id, r.rating, r.comment, r.images, r.manager_reply, r.replied_at, r.created_at, u.full_name as author FROM reviews r JOIN users u ON r.user_id = u.id WHERE r.hotel_id=? ORDER BY r.created_at DESC;", {std::to_string(hotel_id)});
        json arr = json::array();
        for(auto& row : rows) { json obj; for(auto& [k,v]: row) obj[k]=v; arr.push_back(obj); }
        return jsonOk(arr);
    });

    // GET /api/me/reviews
    CROW_ROUTE(app_, "/api/me/reviews")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        auto rows = db_->query(
            "SELECT r.id, r.hotel_id, h.name AS hotel_name, h.location AS hotel_location, "
            "COALESCE((SELECT COALESCE(NULLIF(hi.image_path,''), NULLIF(hi.image_url,'')) "
            "FROM hotel_images hi WHERE hi.hotel_id=h.id ORDER BY hi.is_primary DESC, hi.id ASC LIMIT 1), "
            "NULLIF(h.image,''), NULLIF(h.image_url,''), '') AS hotel_image, "
            "r.rating, r.comment, r.images, r.manager_reply, r.replied_at, r.created_at "
            "FROM reviews r "
            "JOIN hotels h ON h.id=r.hotel_id "
            "WHERE r.user_id=? "
            "ORDER BY r.created_at DESC;",
            {std::to_string(user_id)});
        return jsonOk(dbRowsToJson(rows));
    });
    
    // POST /api/reviews/:id/reply (Manager replies to a review)
    CROW_ROUTE(app_, "/api/reviews/<int>/reply")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int review_id) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        int hotel_id = db_->queryInt("SELECT hotel_id FROM reviews WHERE id=?;", {std::to_string(review_id)});
        if (hotel_id == 0) return jsonErr("Review not found", 404);
        if (!canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string reply = body.value("reply", "");
            db_->execute("UPDATE reviews SET manager_reply=?, replied_at=datetime('now','localtime') WHERE id=?;", {reply, std::to_string(review_id)});
            return jsonOk({}, "Reply posted successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // POST /api/hotels/:id/images (Upload image info)
    CROW_ROUTE(app_, "/api/hotels/<int>/images")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string image_url = body.value("image_path", body.value("image", body.value("image_url", "")));
            bool is_primary = false;
            if (body.contains("is_primary")) {
                if (body["is_primary"].is_boolean()) is_primary = body["is_primary"].get<bool>();
                else if (body["is_primary"].is_number_integer()) is_primary = body["is_primary"].get<int>() != 0;
                else if (body["is_primary"].is_string()) is_primary = boolParam(body["is_primary"].get<std::string>());
            }
            if (image_url.empty()) return jsonErr("image_url required", 400);
            int id = images_->addHotelImage(hotel_id, image_url, is_primary);
            return jsonOk({{"id", id}, {"image_path", image_url}, {"images", images_->getHotelImagesJson(hotel_id)}},
                          "Hotel image recorded successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    CROW_ROUTE(app_, "/api/hotels/<int>/images")
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        return jsonOk(images_->getHotelImagesJson(hotel_id));
    });

    CROW_ROUTE(app_, "/api/hotels/images/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int image_id) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        int hotel_id = db_->queryInt("SELECT hotel_id FROM hotel_images WHERE id=?;", {std::to_string(image_id)});
        if (hotel_id <= 0) return jsonErr("Image not found", 404);
        if (!canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        if (!images_->deleteHotelImage(image_id)) return jsonErr("Image not found", 404);
        return jsonOk({{"hotel_id", hotel_id}, {"images", images_->getHotelImagesJson(hotel_id)}},
                      "Hotel image deleted");
    });

    // POST /api/hotels/:id/documents (Upload hotel documents for approval)
    CROW_ROUTE(app_, "/api/hotels/<int>/documents")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id))
            return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string doc_type = body.value("document_type", "license");
            std::string doc_url  = body.value("document_url", "");
            if (doc_url.empty()) return jsonErr("document_url required", 400);
            db_->execute(R"sql(
                INSERT INTO hotel_documents (hotel_id, document_type, document_url)
                SELECT ?, ?, ?
                WHERE NOT EXISTS (
                    SELECT 1 FROM hotel_documents WHERE hotel_id=? AND document_url=?
                );
            )sql", {std::to_string(hotel_id), doc_type, doc_url, std::to_string(hotel_id), doc_url});
            db_->execute("UPDATE hotels SET status='pending' WHERE id=? AND status='rejected';",
                         {std::to_string(hotel_id)});
            return jsonOk({}, "Hotel document uploaded successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // POST /api/hotels/:id/staff (Manager adds staff to their hotel)
    CROW_ROUTE(app_, "/api/hotels/<int>/staff")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        // Only Super Admin OR the Manager of THIS hotel can add staff
        if (!s || !canManageHotel(*s, hotel_id))
            return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            User u;
            u.username  = body.value("username","");
            u.full_name = body.value("full_name","");
            u.email     = body.value("email","");
            u.phone     = body.value("phone","");
            u.hotel_id  = hotel_id;
            u.role      = UserRole::RECEPTIONIST; 
            std::string pw = body.value("password","");
            if (u.username.empty() || pw.empty() || u.full_name.empty())
                return jsonErr("username, full_name, password required", 400);
            if (!auth_->createUser(u, pw))
                return jsonErr("Username already exists", 409);
            int user_id = db_->queryInt("SELECT id FROM users WHERE username=?;", {u.username});
            if (user_id > 0) {
                db_->execute(R"sql(
                    INSERT OR IGNORE INTO staff (user_id, hotel_id, role_label, shift, status)
                    VALUES (?, ?, ?, ?, 'active');
                )sql", {
                    std::to_string(user_id),
                    std::to_string(hotel_id),
                    bodyText(body, "role_label", "Receptionist"),
                    bodyText(body, "shift", "Front desk")
                });
            }
            return jsonOk({}, "Staff account created successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // ── Room types ────────────────────────────────────────────────────────

    CROW_ROUTE(app_, "/api/hotels/<int>/staff")
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        return jsonOk(dbRowsToJson(db_->query(R"sql(
            SELECT u.id AS user_id, COALESCE(st.id, 0) AS staff_id,
                   u.username, u.full_name, u.email, u.phone, u.role, u.hotel_id,
                   COALESCE(NULLIF(st.role_label,''), CASE WHEN u.role='hotel_manager' THEN 'Hotel Manager' ELSE 'Receptionist' END) AS role_label,
                   COALESCE(NULLIF(st.shift,''), CASE WHEN u.role='hotel_manager' THEN 'Operations' ELSE 'Front desk' END) AS shift,
                   COALESCE(NULLIF(st.status,''), 'active') AS staff_status,
                   COALESCE(st.avatar_url, '') AS avatar_url,
                   u.created_at
            FROM users u
            LEFT JOIN staff st ON st.user_id=u.id
            WHERE u.hotel_id=? AND u.role IN ('hotel_manager', 'receptionist')
            ORDER BY CASE WHEN u.role='hotel_manager' THEN 0 ELSE 1 END, u.full_name;
        )sql", {std::to_string(hotel_id)})));
    });

    CROW_ROUTE(app_, "/api/staff/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int user_id) {
        auto s = getSession(req);
        if (!s) return jsonErr("Unauthorized", 401);
        auto rows = db_->query("SELECT hotel_id, role FROM users WHERE id=?;", {std::to_string(user_id)});
        if (rows.empty()) return jsonErr("Staff user not found", 404);
        const int hotel_id = toIntSafe(rowValue(rows[0], "hotel_id"));
        const std::string role = rowValue(rows[0], "role");
        if (!canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        if (role == "hotel_manager" && s->role != UserRole::SUPER_ADMIN)
            return jsonErr("Managers cannot delete the owner account", 403);
        db_->execute("DELETE FROM users WHERE id=?;", {std::to_string(user_id)});
        return jsonOk({}, "Staff account deleted");
    });

    // GET /api/hotels/:id/room-types
    CROW_ROUTE(app_, "/api/hotels/<int>/room-types")
    ([this](const crow::request& req, int hotel_id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto rows = db_->query(R"sql(
            SELECT rt.*,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id) AS room_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='available') AS available_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='occupied') AS occupied_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='reserved') AS reserved_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='maintenance') AS maintenance_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='cleaned') AS cleaning_count,
                   (SELECT COUNT(*) FROM rooms r WHERE r.room_type_id=rt.id AND r.status='deactivated') AS deactivated_count,
                   COALESCE((SELECT image_url FROM room_images ri WHERE ri.room_type_id=rt.id ORDER BY is_primary DESC, id ASC LIMIT 1), '') AS image_url
            FROM room_types rt
            WHERE rt.hotel_id=?
            ORDER BY rt.base_price;
        )sql", {std::to_string(hotel_id)});
        json arr = dbRowsToJson(rows);
        for (auto& item : arr) {
            const int room_type_id = toIntSafe(item.value("id", "0"));
            item["images"] = roomImagesJson(*db_, room_type_id);
            item["image_path"] = item.value("image_url", "");
        }
        return jsonOk(arr);
    });

    // POST /api/hotels/:id/room-types  (admin only)
    CROW_ROUTE(app_, "/api/hotels/<int>/room-types")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        
        auto hotel = hotels_->getHotelById(hotel_id);
        if (!hotel || hotel->status != "approved") {
            return jsonErr("Your hotel must be approved before adding room types.", 403);
        }
        const std::string paymentStatus = hotelPaymentRestriction(*db_, hotel_id);
        if (paymentStatus == "Restricted" || paymentStatus == "Suspended") {
            return jsonErr("Room creation is disabled until overdue commission dues are paid.", 403);
        }

        try {
            auto body = json::parse(req.body);
            RoomType rt;
            rt.hotel_id      = hotel_id;
            rt.name          = body.value("name","");
            rt.description   = body.value("description","");
            rt.room_size     = bodyText(body, "room_size", "");
            rt.max_capacity  = body.value("max_capacity", 2);
            rt.base_price    = body.value("base_price",   0.0);
            rt.current_price = rt.base_price;
            rt.total_rooms   = body.value("total_rooms",  0);
            rt.amenities     = bodyArrayText(body, "amenities", "[]");
            if (rt.name.empty()) return jsonErr("name required", 400);

            const bool autoGenerate = bodyBool(body, "auto_generate_room_numbers", rt.total_rooms > 0);
            const std::string providedPrefix = sanitizeRoomPrefix(Utils::trim(bodyText(body, "room_prefix", "")));
            const std::string prefix = providedPrefix.empty() ? defaultRoomPrefixForType(rt.name) : providedPrefix;
            const std::string startingText = Utils::trim(bodyText(body, "starting_room_number", "101"));
            const bool numericStart = !startingText.empty() && std::all_of(
                startingText.begin(), startingText.end(),
                [](unsigned char c) { return std::isdigit(c); });
            if (autoGenerate && rt.total_rooms > 0 && !numericStart) {
                return jsonErr("Starting room number must be numeric", 400);
            }

            int generatedCount = 0;
            json generatedRooms = json::array();
            db_->beginTransaction();
            rt.id = hotels_->createRoomType(rt);
            if (autoGenerate && rt.total_rooms > 0) {
                const int startNumber = toIntSafe(startingText, 101);
                const size_t numberWidth = startingText.size();
                const int explicitFloor = toIntSafe(bodyText(body, "floor", "0"), 0);
                for (int i = 0; i < rt.total_rooms; ++i) {
                    const int roomNumberValue = startNumber + i;
                    Room room;
                    room.hotel_id = hotel_id;
                    room.room_type_id = rt.id;
                    room.room_number = roomNumberFromSequence(prefix, roomNumberValue, numberWidth);
                    room.floor = explicitFloor > 0 ? explicitFloor : std::max(1, roomNumberValue / 100);
                    if (db_->queryInt(
                        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND room_number=?;",
                        {std::to_string(hotel_id), room.room_number}) > 0) {
                        db_->rollback();
                        return jsonErr("Room number already exists: " + room.room_number, 400);
                    }
                    room.id = rooms_->createRoom(room);
                    generatedRooms.push_back(room.toJson());
                    ++generatedCount;
                }
            }
            std::string single_image = body.value("image_path", body.value("image_url", ""));
            if (!single_image.empty()) {
                addRoomImageRecord(*db_, rt.id, single_image, true);
            }
            if (body.contains("room_images") && body["room_images"].is_array()) {
                bool first = single_image.empty();
                for (const auto& image : body["room_images"]) {
                    if (!image.is_string() || image.get<std::string>().empty()) continue;
                    addRoomImageRecord(*db_, rt.id, image.get<std::string>(), first);
                    first = false;
                }
            }
            db_->commit();
            json out = rt.toJson();
            out["image_url"] = primaryRoomImage(*db_, rt.id);
            out["image_path"] = out["image_url"];
            out["images"] = roomImagesJson(*db_, rt.id);
            out["generated_count"] = generatedCount;
            out["generated_rooms"] = generatedRooms;
            return jsonOk(out, "Room type created");
        } catch (const std::exception& e) {
            try { db_->rollback(); } catch (...) {}
            return jsonErr(e.what(), 400);
        } catch (...) {
            try { db_->rollback(); } catch (...) {}
            return jsonErr("Invalid JSON", 400);
        }
    });

    // PUT /api/room-types/:id  (admin only)
    CROW_ROUTE(app_, "/api/room-types/<int>")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        int hotel_id = db_->queryInt("SELECT hotel_id FROM room_types WHERE id=?;", {std::to_string(id)});
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        const std::string paymentStatus = hotelPaymentRestriction(*db_, hotel_id);
        if (paymentStatus == "Restricted" || paymentStatus == "Suspended") {
            return jsonErr("Room edits and price updates are disabled until overdue commission dues are paid.", 403);
        }
        try {
            auto body = json::parse(req.body);
            auto existing = hotels_->getRoomTypeById(id);
            if (!existing) return jsonErr("Room type not found", 404);
            existing->name          = body.value("name",         existing->name);
            existing->description   = body.value("description",  existing->description);
            existing->room_size     = bodyText(body, "room_size", existing->room_size);
            existing->max_capacity  = body.value("max_capacity", existing->max_capacity);
            existing->base_price    = body.value("base_price",   existing->base_price);
            existing->current_price = body.value("current_price",existing->current_price);
            existing->total_rooms   = body.value("total_rooms",  existing->total_rooms);
            existing->amenities     = bodyArrayText(body, "amenities", existing->amenities);
            hotels_->updateRoomType(*existing);
            return jsonOk(existing->toJson(), "Room type updated");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // DELETE /api/room-types/:id
    CROW_ROUTE(app_, "/api/room-types/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        int hotel_id = db_->queryInt("SELECT hotel_id FROM room_types WHERE id=?;", {std::to_string(id)});
        if (hotel_id <= 0) return jsonErr("Room type not found", 404);
        auto deny = [&](const std::string& reason, int status) {
            std::cerr << "[AuthZ] DELETE /api/room-types/" << id << " denied: " << reason << "\n";
            return jsonErr(reason, status);
        };
        if (!s) return deny("Authentication token missing or expired. Please sign in again.", 401);
        if (s->role != UserRole::SUPER_ADMIN && s->role != UserRole::HOTEL_MANAGER)
            return deny("Only hotel managers can delete room types.", 403);
        if (!canManageHotel(*s, hotel_id))
            return deny("Room type belongs to a different hotel.", 403);
        const std::string paymentStatus = hotelPaymentRestriction(*db_, hotel_id);
        if (paymentStatus == "Restricted" || paymentStatus == "Suspended") {
            return jsonErr("Room type deletion is disabled until overdue commission dues are paid.", 403);
        }
        try {
            int roomCount = db_->queryInt(
                "SELECT COUNT(*) FROM rooms WHERE room_type_id=?;",
                {std::to_string(id)});
            int bookingCount = db_->queryInt(R"sql(
                SELECT COUNT(*)
                FROM bookings
                WHERE room_type_id=?
                   OR room_id IN (SELECT id FROM rooms WHERE room_type_id=?);
            )sql", {std::to_string(id), std::to_string(id)});
            if (bookingCount > 0) {
                return jsonErr("Room type cannot be deleted because booking history exists. Deactivate rooms instead to preserve reports and guest history.", 409);
            }
            db_->beginTransaction();
            db_->execute("DELETE FROM inventory WHERE room_type_id=?;", {std::to_string(id)});
            db_->execute("DELETE FROM rooms WHERE room_type_id=?;", {std::to_string(id)});
            hotels_->deleteRoomType(id);
            db_->commit();
            return jsonOk({
                {"deleted_rooms", roomCount},
                {"stats", rooms_->getRoomStats(hotel_id)}
            }, "Room type and associated inventory deleted");
        } catch (const std::exception& e) {
            try { db_->rollback(); } catch (...) {}
            return jsonErr(e.what(), 400);
        }
    });

    // POST /api/room-types/:id/images (Upload room images)
    CROW_ROUTE(app_, "/api/room-types/<int>/images")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int room_type_id) {
        auto s = getSession(req);
        int hotel_id = db_->queryInt("SELECT hotel_id FROM room_types WHERE id=?;", {std::to_string(room_type_id)});
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string image_url = body.value("image_path", body.value("image", body.value("image_url", "")));
            int is_primary = body.value("is_primary", 0);
            if (image_url.empty()) return jsonErr("image_url required", 400);
            int image_id = addRoomImageRecord(*db_, room_type_id, image_url, is_primary != 0);
            return jsonOk({{"id", image_id}, {"image_path", image_url}, {"images", roomImagesJson(*db_, room_type_id)}},
                          "Room image recorded successfully");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });
}

// ═════════════════════════════════════════════════════════════════════════════
//  PHASE 4 — ROOM ROUTES
// ═════════════════════════════════════════════════════════════════════════════
void APIRouter::registerRoomRoutes() {

    // GET /api/hotels/:id/rooms
    CROW_ROUTE(app_, "/api/hotels/<int>/rooms")
    ([this](const crow::request& req, int hotel_id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(dbRowsToJson(db_->query(R"sql(
            SELECT r.*, rt.name AS room_type_name,
                   COALESCE(i.status, r.status) AS status,
                   COALESCE(NULLIF(i.price, 0), rt.current_price, rt.base_price, 0) AS price,
                   rt.current_price AS current_price,
                   COALESCE((SELECT image_url FROM room_images ri WHERE ri.room_type_id=rt.id ORDER BY is_primary DESC, id ASC LIMIT 1), '') AS image_url,
                   COALESCE(i.status, r.status) AS inventory_status,
                   COALESCE((SELECT b.guest_name FROM bookings b WHERE b.room_id=r.id AND b.status IN ('active', 'checked_in') ORDER BY b.created_at DESC LIMIT 1), '') AS guest_name,
                   COALESCE((SELECT b.booking_id FROM bookings b WHERE b.room_id=r.id AND b.status IN ('active', 'checked_in') ORDER BY b.created_at DESC LIMIT 1), '') AS booking_ref
            FROM rooms r
            JOIN room_types rt ON r.room_type_id = rt.id
            LEFT JOIN inventory i ON i.room_id = r.id
            WHERE r.hotel_id=?
            ORDER BY r.floor, r.room_number;
        )sql", {std::to_string(hotel_id)})));
    });

    CROW_ROUTE(app_, "/api/hotels/<int>/inventory")
    ([this](const crow::request& req, int hotel_id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        std::string status = firstParam(req, "status", "all");
        std::string type = firstParam(req, "room_type_id", "0");
        int page = std::max(1, toIntSafe(firstParam(req, "page", "1"), 1));
        int per_page = std::min(50, std::max(5, toIntSafe(firstParam(req, "per_page", "12"), 12)));
        int offset = (page - 1) * per_page;

        std::string sql = R"sql(
            SELECT r.id AS room_id, r.room_number, r.floor, COALESCE(i.status, r.status) AS status,
                   rt.id AS room_type_id, rt.name AS room_type_name,
                   COALESCE(NULLIF(i.price, 0), rt.current_price, rt.base_price, 0) AS price,
                   COALESCE((SELECT image_url FROM room_images ri WHERE ri.room_type_id=rt.id ORDER BY is_primary DESC, id ASC LIMIT 1), '') AS image_url,
                   COALESCE((SELECT b.guest_name FROM bookings b WHERE b.room_id=r.id AND b.status IN ('active', 'checked_in') ORDER BY b.created_at DESC LIMIT 1), '') AS guest_name,
                   COALESCE((SELECT b.booking_id FROM bookings b WHERE b.room_id=r.id AND b.status IN ('active', 'checked_in') ORDER BY b.created_at DESC LIMIT 1), '') AS booking_ref,
                   r.last_updated
            FROM rooms r
            JOIN room_types rt ON r.room_type_id=rt.id
            LEFT JOIN inventory i ON i.room_id=r.id
            WHERE r.hotel_id=?
        )sql";
        std::vector<std::string> params = {std::to_string(hotel_id)};
        if (status != "all" && !status.empty()) {
            if (status == "cleaning") status = "cleaned";
            sql += " AND COALESCE(i.status, r.status)=?";
            params.push_back(status);
        }
        if (type != "0" && !type.empty()) {
            sql += " AND r.room_type_id=?";
            params.push_back(type);
        }
        sql += " ORDER BY r.floor, r.room_number LIMIT ? OFFSET ?;";
        params.push_back(std::to_string(per_page));
        params.push_back(std::to_string(offset));
        return jsonOk({
            {"page", page},
            {"per_page", per_page},
            {"items", dbRowsToJson(db_->query(sql, params))}
        });
    });

    // GET /api/hotels/:id/rooms/stats
    CROW_ROUTE(app_, "/api/hotels/<int>/rooms/stats")
    ([this](const crow::request& req, int hotel_id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(rooms_->getRoomStats(hotel_id));
    });

    // GET /api/hotels/:id/rooms/available?check_in=&check_out=&room_type_id=
    CROW_ROUTE(app_, "/api/hotels/<int>/rooms/available")
    ([this](const crow::request& req, int hotel_id) {
        // Publicly accessible so users can check dates before booking
        std::string check_in     = req.url_params.get("check_in")     ? req.url_params.get("check_in")     : "";
        std::string check_out    = req.url_params.get("check_out")    ? req.url_params.get("check_out")    : "";
        std::string rt_str       = req.url_params.get("room_type_id") ? req.url_params.get("room_type_id") : "0";
        int room_type_id = 0;
        try { room_type_id = std::stoi(rt_str); } catch (...) {}

        if (check_in.empty() || check_out.empty())
            return jsonErr("check_in and check_out query params required", 400);
        const std::string paymentStatus = hotelPaymentRestriction(*db_, hotel_id);
        if (paymentStatus == "Suspended")
            return jsonErr("This hotel is temporarily unavailable for new bookings.", 403);

        auto rooms = rooms_->getAvailableRooms(hotel_id, room_type_id, check_in, check_out);
        json arr = json::array();
        for (auto& r : rooms) arr.push_back(r.toJson());
        return jsonOk(arr);
    });

    // GET /api/rooms/:id
    CROW_ROUTE(app_, "/api/rooms/<int>")
    ([this](const crow::request& req, int id) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto r = rooms_->getRoomById(id);
        if (!r) return jsonErr("Room not found", 404);
        return jsonOk(r->toJson());
    });

    // PUT /api/rooms/:id
    CROW_ROUTE(app_, "/api/rooms/<int>")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        auto existing = rooms_->getRoomById(id);
        if (!existing) return jsonErr("Room not found", 404);
        if (!s) return jsonErr("Authentication token missing or expired. Please sign in again.", 401);
        if (!canManageHotel(*s, existing->hotel_id)) return jsonErr("Room belongs to a different hotel.", 403);
        try {
            auto body = json::parse(req.body);
            Room r = *existing;
            r.room_type_id = body.value("room_type_id", r.room_type_id);
            r.room_number = Utils::trim(body.value("room_number", r.room_number));
            r.floor = body.value("floor", r.floor);
            r.notes = body.value("notes", r.notes);
            if (r.room_number.empty() || r.room_type_id <= 0)
                return jsonErr("room_number and room_type_id required", 400);

            int typeHotelId = db_->queryInt(
                "SELECT hotel_id FROM room_types WHERE id=?;",
                {std::to_string(r.room_type_id)});
            if (typeHotelId != r.hotel_id)
                return jsonErr("Room type belongs to a different hotel.", 403);

            int duplicate = db_->queryInt(
                "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND room_number=? AND id<>?;",
                {std::to_string(r.hotel_id), r.room_number, std::to_string(r.id)});
            if (duplicate > 0) return jsonErr("Room number already exists: " + r.room_number, 409);

            rooms_->updateRoom(r);
            double room_price = body.contains("price")
                ? toDoubleSafe(bodyText(body, "price"), 0.0)
                : toDoubleSafe(bodyText(body, "price_per_night"), 0.0);
            if (room_price > 0.0) {
                db_->execute("UPDATE inventory SET price=?, last_synced=datetime('now','localtime') WHERE room_id=?;",
                             {std::to_string(room_price), std::to_string(r.id)});
            }
            auto updated = rooms_->getRoomById(id);
            return jsonOk(updated ? updated->toJson() : r.toJson(), "Room updated");
        } catch (...) {
            return jsonErr("Invalid JSON", 400);
        }
    });

    // POST /api/hotels/:id/rooms  (admin only)
    CROW_ROUTE(app_, "/api/hotels/<int>/rooms")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int hotel_id) {
        auto s = getSession(req);
        if (!s || !canManageHotel(*s, hotel_id)) return jsonErr("Forbidden", 403);

        auto hotel = hotels_->getHotelById(hotel_id);
        if (!hotel || hotel->status != "approved") {
            return jsonErr("Your hotel must be approved before adding rooms.", 403);
        }
        const std::string paymentStatus = hotelPaymentRestriction(*db_, hotel_id);
        if (paymentStatus == "Restricted" || paymentStatus == "Suspended") {
            return jsonErr("Room creation is disabled until overdue commission dues are paid.", 403);
        }

        try {
            auto body = json::parse(req.body);
            Room r;
            r.hotel_id     = hotel_id;
            r.room_type_id = body.value("room_type_id", 0);
            r.room_number  = Utils::trim(body.value("room_number",""));
            r.floor        = body.value("floor", 1);
            r.notes        = body.value("notes","");
            if (r.room_number.empty() || r.room_type_id == 0)
                return jsonErr("room_number and room_type_id required", 400);
            int typeHotelId = db_->queryInt(
                "SELECT hotel_id FROM room_types WHERE id=?;",
                {std::to_string(r.room_type_id)});
            if (typeHotelId != hotel_id)
                return jsonErr("Room type belongs to a different hotel.", 403);
            int duplicate = db_->queryInt(
                "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND room_number=?;",
                {std::to_string(hotel_id), r.room_number});
            if (duplicate > 0) return jsonErr("Room number already exists: " + r.room_number, 409);
            r.id = rooms_->createRoom(r);
            double room_price = body.contains("price")
                ? toDoubleSafe(bodyText(body, "price"), 0.0)
                : toDoubleSafe(bodyText(body, "price_per_night"), 0.0);
            if (room_price > 0.0) {
                db_->execute("UPDATE inventory SET price=?, last_synced=datetime('now','localtime') WHERE room_id=?;",
                             {std::to_string(room_price), std::to_string(r.id)});
            }
            return jsonOk(r.toJson(), "Room created");
        } catch (const std::exception& e) { return jsonErr(e.what(), 400); }
        catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // PUT /api/rooms/:id/status  (set available/maintenance/etc)
    CROW_ROUTE(app_, "/api/rooms/<int>/status")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        auto room = rooms_->getRoomById(id);
        if (!room) return jsonErr("Room not found", 404);
        if (!s || !canManageHotel(*s, room->hotel_id)) return jsonErr("Forbidden", 403);
        try {
            auto body   = json::parse(req.body);
            std::string status = body.value("status","");
            std::string notes  = body.value("notes","");
            RoomStatus rs = RoomStatus::AVAILABLE;
            if      (status == "occupied")    rs = RoomStatus::OCCUPIED;
            else if (status == "maintenance") rs = RoomStatus::MAINTENANCE;
            else if (status == "cleaned" || status == "cleaning") rs = RoomStatus::CLEANED;
            else if (status == "reserved")    rs = RoomStatus::RESERVED;
            else if (status == "deactivated") rs = RoomStatus::DEACTIVATED;
            else if (status == "empty")       rs = RoomStatus::AVAILABLE;
            else if (status != "available")
                return jsonErr("Invalid status. Use: empty/cleaned/cleaning/occupied/available/maintenance/reserved/deactivated", 400);
            rooms_->setStatus(id, rs, notes);
            const std::string inventory_status = status == "empty" ? "available" : (status == "cleaning" ? "cleaned" : status);
            db_->execute("UPDATE inventory SET status=?, last_synced=datetime('now','localtime') WHERE room_id=?;",
                         {inventory_status, std::to_string(id)});
            return jsonOk({}, "Room status updated");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    // DELETE /api/rooms/:id  (admin only)
    CROW_ROUTE(app_, "/api/rooms/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        auto room = rooms_->getRoomById(id);
        if (!room) return jsonErr("Room not found", 404);
        auto deny = [&](const std::string& reason, int status) {
            std::cerr << "[AuthZ] DELETE /api/rooms/" << id << " denied: " << reason << "\n";
            return jsonErr(reason, status);
        };
        if (!s) return deny("Authentication token missing or expired. Please sign in again.", 401);
        if (s->role != UserRole::SUPER_ADMIN)
            return deny("Permanent room deletion is restricted to super admins. Use Deactivate for normal operations.", 403);
        int bookingCount = db_->queryInt(
            "SELECT COUNT(*) FROM bookings WHERE room_id=?;",
            {std::to_string(id)});
        if (bookingCount > 0) {
            return jsonErr("Room cannot be permanently deleted because booking history exists. Deactivate it instead.", 409);
        }
        try {
            db_->beginTransaction();
            rooms_->deleteRoom(id);
            db_->commit();
            return jsonOk({{"deleted_room_id", id}}, "Room permanently deleted");
        } catch (const std::exception& e) {
            try { db_->rollback(); } catch (...) {}
            return jsonErr(e.what(), 400);
        }
    });
}

// ── Stub route groups — filled in Phases 5-12 ─────────────────────────────────

void APIRouter::registerBillingRoutes() {
    // TODO: Implement Billing routes
}

void APIRouter::registerCancellationRoutes() {

    // POST /api/cancellations
    CROW_ROUTE(app_, "/api/cancellations")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);

        try {
            auto body = json::parse(req.body);
            std::string booking_ref = body.value("booking_ref", "");
            std::string reason = body.value("reason", "Guest requested cancellation");

            if (booking_ref.empty())
                return jsonErr("booking_ref is required", 400);

            // Verify user can cancel this booking (admin or booking owner)
            auto booking = bookings_->getByRef(booking_ref);
            if (!booking)
                return jsonErr("Booking not found", 404);

            // If a session is present, enforce role ownership. With auth disabled
            // (no session), we still allow the request — the cancellation
            // window and refund rules in CancellationManager remain the gate.
            if (s && s->role != UserRole::SUPER_ADMIN && s->role != UserRole::HOTEL_MANAGER &&
                s->user_id != booking->created_by)
                return jsonErr("You can only cancel your own bookings", 403);

            int user_id = s ? s->user_id : 1;
            auto cancellation = cancellations_->cancelByRef(booking_ref, reason, user_id);
            if (!cancellation)
                return jsonErr("Booking cannot be cancelled. It may not be active or past cancellation window.", 400);

            json response = cancellation->toJson();
            response["booking"] = booking->toJson();

            // Update payment status
            db_->execute("UPDATE payments SET status='refunded' WHERE booking_ref=?;", {booking_ref});

            return jsonOk(response, "Booking cancelled successfully. Refund will be processed.");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    // POST /api/cancellations/by-phone
    CROW_ROUTE(app_, "/api/cancellations/by-phone")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);

        try {
            auto body = json::parse(req.body);
            int hotel_id = body.value("hotel_id", 0);
            std::string phone = body.value("phone", "");
            std::string reason = body.value("reason", "Cancelled by hotel staff");

            if (hotel_id == 0 || phone.empty())
                return jsonErr("hotel_id and phone are required", 400);

            if (s && !canManageHotel(*s, hotel_id))
                return jsonErr("You can only cancel bookings for your hotel", 403);

            int user_id = s ? s->user_id : 1;
            auto cancellation = cancellations_->cancelByPhone(phone, hotel_id, reason, user_id);
            if (!cancellation)
                return jsonErr("No active booking found for this phone number", 404);

            // Get booking details
            auto booking = bookings_->getByRef(cancellation->booking_ref);
            json response = cancellation->toJson();
            if (booking) {
                response["booking"] = booking->toJson();
                // Update payment status
                db_->execute("UPDATE payments SET status='refunded' WHERE booking_ref=?;", {cancellation->booking_ref});
            }

            return jsonOk(response, "Booking cancelled successfully");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    // GET /api/cancellations?hotel_id=
    CROW_ROUTE(app_, "/api/cancellations")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);

        int hotel_id = 0;
        try {
            hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0");
        } catch(...) {}

        if (hotel_id == 0)
            return jsonErr("hotel_id query parameter required", 400);

        if (s && !canManageHotel(*s, hotel_id))
            return jsonErr("You can only view cancellations for your hotel", 403);

        auto cancellations = cancellations_->getCancellations(hotel_id);
        json arr = json::array();
        for (auto& c : cancellations) arr.push_back(c.toJson());

        return jsonOk(arr);
    });

    // GET /api/cancellations/:id
    CROW_ROUTE(app_, "/api/cancellations/<int>")
    ([this](const crow::request& req, int id) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);

        auto cancellation = cancellations_->getCancellationById(id);
        if (!cancellation)
            return jsonErr("Cancellation not found", 404);

        // Verify hotel ownership
        auto booking = bookings_->getByRef(cancellation->booking_ref);
        if (s && booking && !canManageHotel(*s, booking->hotel_id))
            return jsonErr("You can only view cancellations for your hotel", 403);

        json response = cancellation->toJson();
        if (booking) {
            response["booking"] = booking->toJson();
        }

        return jsonOk(response);
    });
}

// =============================================================================
//  PHASE 8 — SEARCH ROUTES
// =============================================================================
void APIRouter::registerSearchRoutes() {

    // GET /api/search?hotel_id=1&q=John&status=active&page=1&per_page=20
    //     &sort_by=cost&sort_dir=desc&date_from=2026-01-01&date_to=2026-12-31
    CROW_ROUTE(app_, "/api/search")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);

        auto gp = [&](const char* k, const std::string& def = "") -> std::string {
            auto v = req.url_params.get(k);
            return v ? std::string(v) : def;
        };

        SearchParams sp;
        try { sp.hotel_id = std::stoi(gp("hotel_id", "0")); } catch(...) {}
        sp.query             = gp("q");
        sp.filter_name       = gp("name");
        sp.filter_phone      = gp("phone");
        sp.filter_booking_id = gp("booking_id");
        sp.filter_room_type  = gp("room_type");
        sp.filter_status     = gp("status");
        sp.date_from         = gp("date_from");
        sp.date_to           = gp("date_to");
        sp.sort_by           = gp("sort_by", "date");
        sp.sort_dir          = gp("sort_dir", "desc");
        try { sp.page     = std::stoi(gp("page",     "1"));  } catch(...) {}
        try { sp.per_page = std::stoi(gp("per_page", "20")); } catch(...) {}

        if (sp.hotel_id == 0) return jsonErr("hotel_id is required", 400);

        try {
            auto result = search_->search(sp);
            return jsonOk(result.toJson());
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/search/today?hotel_id=1 — today's arrivals and departures
    CROW_ROUTE(app_, "/api/search/today")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        std::string hid = std::to_string(hotel_id);
        auto arrivals = db_->query(
            "SELECT b.*, r.room_number, rt.name AS room_type_name "
            "FROM bookings b "
            "LEFT JOIN rooms r ON b.room_id=r.id "
            "LEFT JOIN room_types rt ON b.room_type_id=rt.id "
            "WHERE b.hotel_id=? AND b.check_in_date=DATE('now') AND b.status IN ('active', 'checked_in') "
            "ORDER BY b.guest_name;", {hid});

        auto departures = db_->query(
            "SELECT b.*, r.room_number, rt.name AS room_type_name "
            "FROM bookings b "
            "LEFT JOIN rooms r ON b.room_id=r.id "
            "LEFT JOIN room_types rt ON b.room_type_id=rt.id "
            "WHERE b.hotel_id=? AND b.check_out_date=DATE('now') AND b.status IN ('active', 'checked_in') "
            "ORDER BY b.guest_name;", {hid});

        auto toJson = [](const DBResult& rows) {
            json arr = json::array();
            for (auto& row : rows) {
                json obj;
                for (auto& [k,v] : row) obj[k] = v;
                arr.push_back(obj);
            }
            return arr;
        };

        return jsonOk({
            {"arrivals",   toJson(arrivals)},
            {"departures", toJson(departures)},
            {"date",       "today"}
        });
    });

    // GET /api/search/guest?hotel_id=1&phone=9876543210 — quick guest lookup
    CROW_ROUTE(app_, "/api/search/guest")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        std::string phone = req.url_params.get("phone") ? req.url_params.get("phone") : "";
        std::string name  = req.url_params.get("name")  ? req.url_params.get("name")  : "";
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);
        if (phone.empty() && name.empty()) return jsonErr("Provide phone or name", 400);

        std::string hid = std::to_string(hotel_id);
        std::vector<std::string> binds = {hid};
        std::string cond;
        if (!phone.empty()) { cond = "AND b.guest_phone LIKE ?"; binds.push_back("%" + phone + "%"); }
        else                { cond = "AND b.guest_name  LIKE ?"; binds.push_back("%" + name  + "%"); }

        auto rows = db_->query(
            "SELECT b.booking_id, b.guest_name, b.guest_phone, b.guest_email, "
            "b.check_in_date, b.check_out_date, b.nights, b.total_amount, b.status, "
            "r.room_number, rt.name AS room_type_name "
            "FROM bookings b "
            "LEFT JOIN rooms r ON b.room_id=r.id "
            "LEFT JOIN room_types rt ON b.room_type_id=rt.id "
            "WHERE b.hotel_id=? " + cond + " "
            "ORDER BY b.created_at DESC LIMIT 20;", binds);

        json arr = json::array();
        for (auto& row : rows) {
            json obj; for (auto& [k,v] : row) obj[k] = v;
            arr.push_back(obj);
        }
        return jsonOk(arr);
    });
}

// =============================================================================
//  PHASE 8 — REPORT ROUTES
// =============================================================================
void APIRouter::registerReportRoutes() {

    // GET /api/reports/revenue?hotel_id=1&from=2026-01-01&to=2026-12-31
    CROW_ROUTE(app_, "/api/reports/revenue")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        std::string from = req.url_params.get("from") ? req.url_params.get("from") : "2000-01-01";
        std::string to   = req.url_params.get("to")   ? req.url_params.get("to")   : "2099-12-31";

        try {
            auto report = reports_->getRevenueReport(hotel_id, from, to);
            return jsonOk(report.toJson());
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/revenue/by-room-type?hotel_id=1&from=2026-01-01&to=2026-12-31
    CROW_ROUTE(app_, "/api/reports/revenue/by-room-type")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        std::string from = req.url_params.get("from") ? req.url_params.get("from") : "2000-01-01";
        std::string to   = req.url_params.get("to")   ? req.url_params.get("to")   : "2099-12-31";

        try {
            auto data = reports_->getRevenueByRoomType(hotel_id, from, to);
            return jsonOk(data);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/stats?hotel_id=1 — dashboard stats block
    CROW_ROUTE(app_, "/api/reports/stats")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        try {
            auto stats = reports_->getBookingStats(hotel_id);
            double occ = reports_->getOccupancyRate(hotel_id);
            stats["occupancy_rate"] = occ;
            return jsonOk(stats);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/timeline?hotel_id=1&days=30
    CROW_ROUTE(app_, "/api/reports/timeline")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0, days = 30;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        try { days     = std::stoi(req.url_params.get("days")     ? req.url_params.get("days")     : "30"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        try {
            auto timeline = reports_->getRevenueTimeline(hotel_id, days);
            return jsonOk(timeline);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/popular-rooms?hotel_id=1
    CROW_ROUTE(app_, "/api/reports/popular-rooms")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        try {
            auto data = reports_->getMostBookedRoom(hotel_id);
            return jsonOk(data);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/popular-activities?hotel_id=1
    CROW_ROUTE(app_, "/api/reports/popular-activities")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        try {
            auto data = reports_->getMostPopularActivity(hotel_id);
            return jsonOk(data);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/reports/occupancy?hotel_id=1
    CROW_ROUTE(app_, "/api/reports/occupancy")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        if (hotel_id == 0) return jsonErr("hotel_id required", 400);

        try {
            double rate = reports_->getOccupancyRate(hotel_id);
            return jsonOk({{"hotel_id", hotel_id}, {"occupancy_rate", rate}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });
}

void APIRouter::registerReceiptRoutes() {

    // GET /api/receipts/:booking_ref — get receipt as JSON (with text body)
    CROW_ROUTE(app_, "/api/receipts/<string>")
    ([this](const crow::request& req, const std::string& booking_ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            // Reuse billing to get the Bill object
            BillingManager bm(db_);
            Bill bill = bm.getBillByBookingRef(booking_ref);
            std::string text = receipts_->formatBillAsText(bill);

            return jsonOk({
                {"booking_id",   bill.booking.booking_id},
                {"guest_name",   bill.booking.guest_name},
                {"hotel_name",   bill.hotel.name},
                {"total",        bill.total},
                {"generated_at", bill.generated_at},
                {"receipt_text", text}
            });
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 404);
        }
    });

    // POST /api/receipts/:booking_ref/save — save receipt to disk, return path
    CROW_ROUTE(app_, "/api/receipts/<string>/save").methods("POST"_method)
    ([this](const crow::request& req, const std::string& booking_ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            BillingManager bm(db_);
            Bill bill = bm.getBillByBookingRef(booking_ref);
            std::string path = receipts_->generateReceipt(bill);

            return jsonOk({
                {"booking_id", bill.booking.booking_id},
                {"file_path",  path},
                {"message",    "Receipt saved to " + path}
            }, "Receipt generated");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/receipts/:booking_ref/download — serve receipt as plain text file
    CROW_ROUTE(app_, "/api/receipts/<string>/download")
    ([this](const crow::request& req, const std::string& booking_ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            BillingManager bm(db_);
            Bill bill = bm.getBillByBookingRef(booking_ref);
            std::string text = receipts_->formatBillAsText(bill);

            crow::response res(200, text);
            res.add_header("Content-Type", "text/plain; charset=utf-8");
            res.add_header("Content-Disposition",
                "attachment; filename=\"RECEIPT-" + bill.booking.booking_id + ".txt\"");
            return res;
        } catch (const std::exception& e) {
            crow::response res(404, e.what());
            return res;
        }
    });

    // GET /api/receipts/:booking_ref/pdf — serve receipt as PDF
    CROW_ROUTE(app_, "/api/receipts/<string>/pdf")
    ([this](const crow::request& req, const std::string& booking_ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            BillingManager bm(db_);
            Bill bill = bm.getBillByBookingRef(booking_ref);
            std::string pdf = receipts_->formatBillAsPdf(bill);
            crow::response res(200);
            res.body = pdf;
            res.add_header("Content-Type", "application/pdf");
            res.add_header("Content-Disposition",
                "attachment; filename=\"INVOICE-" + bill.booking.booking_id + ".pdf\"");
            return res;
        } catch (const std::exception& e) { return crow::response(404, e.what()); }
    });
}
// ─────────────────────────────────────────────────────────────────────────────
//  PHASE 11 — BACKUP ROUTES
// ─────────────────────────────────────────────────────────────────────────────
void APIRouter::registerBackupRoutes() {

    // GET /api/backups — list all backups
    CROW_ROUTE(app_, "/api/backups")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        try {
            auto list = backup_->listBackups();
            json arr = json::array();
            for (auto& b : list) {
                arr.push_back({
                    {"filename",   b.filename},
                    {"created_at", b.created_at},
                    {"size_bytes", b.size_bytes},
                    {"size_kb",    b.size_bytes / 1024}
                });
            }
            return jsonOk({{"backups", arr}, {"count", (int)list.size()}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // POST /api/backups — create a new backup
    CROW_ROUTE(app_, "/api/backups").methods("POST"_method)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        try {
            std::string path = backup_->createBackup();
            // Extract just the filename
            size_t pos = path.find_last_of("/\\");
            std::string fname = (pos != std::string::npos) ? path.substr(pos+1) : path;
            return jsonOk({{"file_path", path}, {"filename", fname}}, "Backup created");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // POST /api/backups/:filename/restore — restore a backup
    CROW_ROUTE(app_, "/api/backups/<string>/restore").methods("POST"_method)
    ([this](const crow::request& req, const std::string& filename) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        bool ok = backup_->restoreBackup(filename);
        if (!ok) return jsonErr("Restore failed — file not found", 404);
        return jsonOk({{"filename", filename}}, "Backup restored successfully");
    });

    // DELETE /api/backups/:filename — delete a backup
    CROW_ROUTE(app_, "/api/backups/<string>").methods("DELETE"_method)
    ([this](const crow::request& req, const std::string& filename) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        bool ok = backup_->deleteBackup(filename);
        if (!ok) return jsonErr("Delete failed", 404);
        return jsonOk({{"filename", filename}}, "Backup deleted");
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  PHASE 11 — AI / SMART FEATURE ROUTES
// ─────────────────────────────────────────────────────────────────────────────
void APIRouter::registerAIRoutes() {

    // GET /api/ai/recommend?hotel_id=1&budget=5000&guests=2&nights=3&amenities=WiFi,AC
    CROW_ROUTE(app_, "/api/ai/recommend")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto params = req.url_params;
        int    hotel_id = std::atoi(params.get("hotel_id") ? params.get("hotel_id") : "1");
        double budget   = std::atof(params.get("budget")   ? params.get("budget")   : "0");
        int    guests   = std::atoi(params.get("guests")   ? params.get("guests")   : "1");
        int    nights   = std::atoi(params.get("nights")   ? params.get("nights")   : "1");
        bool   breakfast= params.get("breakfast") && std::string(params.get("breakfast")) == "true";

        std::vector<std::string> amenities;
        if (params.get("amenities")) {
            std::istringstream ss(params.get("amenities"));
            std::string a;
            while (std::getline(ss, a, ',')) if (!a.empty()) amenities.push_back(a);
        }
        try {
            auto recs = ai_->recommendRooms(hotel_id, budget, guests, nights, breakfast, amenities);
            json arr = json::array();
            for (auto& r : recs) {
                json a = json::array();
                for (auto& am : r.amenities) a.push_back(am);
                arr.push_back({
                    {"room_type_id",   r.room_type_id},
                    {"room_type_name", r.room_type_name},
                    {"base_price",     r.base_price},
                    {"dynamic_price",  r.dynamic_price},
                    {"match_score",    r.match_score},
                    {"reason",         r.reason},
                    {"amenities",      a}
                });
            }
            return jsonOk({{"recommendations", arr}, {"count", (int)recs.size()}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/ai/pricing?hotel_id=1
    CROW_ROUTE(app_, "/api/ai/pricing")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = std::atoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "1");
        try {
            auto prices = ai_->getDynamicPrices(hotel_id);
            json arr = json::array();
            for (auto& p : prices) {
                arr.push_back({
                    {"room_type_id",   p.room_type_id},
                    {"room_type_name", p.room_type_name},
                    {"base_price",     p.base_price},
                    {"adjusted_price", p.adjusted_price},
                    {"occupancy_rate", p.occupancy_rate},
                    {"multiplier",     p.multiplier},
                    {"tier",           p.tier}
                });
            }
            double occ = prices.empty() ? 0.0 : prices[0].occupancy_rate;
            std::string tier = prices.empty() ? "normal" : prices[0].tier;
            return jsonOk({
                {"prices",       arr},
                {"occupancy_rate", occ},
                {"pricing_tier", tier}
            });
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/ai/activities?hotel_id=1&nights=3&checkin=2026-05-15
    CROW_ROUTE(app_, "/api/ai/activities")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto params = req.url_params;
        int hotel_id = std::atoi(params.get("hotel_id") ? params.get("hotel_id") : "1");
        int nights   = std::atoi(params.get("nights")   ? params.get("nights")   : "1");
        std::string checkin = params.get("checkin") ? params.get("checkin") : "";
        try {
            auto sugg = ai_->suggestActivities(hotel_id, nights, checkin);
            json arr = json::array();
            for (auto& s : sugg) {
                arr.push_back({
                    {"activity_id", s.activity_id},
                    {"name",        s.name},
                    {"price",       s.price},
                    {"priority",    s.priority},
                    {"reason",      s.reason}
                });
            }
            return jsonOk({{"suggestions", arr}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/ai/forecast?hotel_id=1&days=14
    CROW_ROUTE(app_, "/api/ai/forecast")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto params = req.url_params;
        int hotel_id = std::atoi(params.get("hotel_id") ? params.get("hotel_id") : "1");
        int days     = std::atoi(params.get("days")     ? params.get("days")     : "14");
        if (days < 1 || days > 90) days = 14;
        try {
            auto fc = ai_->forecastOccupancy(hotel_id, days);
            json arr = json::array();
            for (auto& f : fc) {
                arr.push_back({
                    {"date",              f.date},
                    {"predicted_rate",    f.predicted_rate},
                    {"predicted_bookings",f.predicted_bookings},
                    {"label",             f.label}
                });
            }
            return jsonOk({{"forecast", arr}, {"days", days}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // GET /api/ai/guest-intelligence?hotel_id=1&q=9876543210
    CROW_ROUTE(app_, "/api/ai/guest-intelligence")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto params  = req.url_params;
        int hotel_id = std::atoi(params.get("hotel_id") ? params.get("hotel_id") : "1");
        std::string q = params.get("q") ? params.get("q") : "";
        if (q.empty()) return jsonErr("Provide phone or email as 'q'", 400);
        try {
            auto gi = ai_->getGuestIntelligence(hotel_id, q);
            if (!gi) return jsonErr("No history found for this guest", 404);
            return jsonOk({
                {"guest_name",          gi->guest_name},
                {"guest_phone",         gi->guest_phone},
                {"total_stays",         gi->total_stays},
                {"total_spent",         gi->total_spent},
                {"avg_stay_nights",     gi->avg_stay_nights},
                {"preferred_room_type", gi->preferred_room_type},
                {"tier",                gi->tier},
                {"personalised_offer",  gi->personalised_offer}
            });
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  PHASE 11 — USER MANAGEMENT ROUTES  (admin only)
// ─────────────────────────────────────────────────────────────────────────────
void APIRouter::registerUserRoutes() {

    // GET /api/users
    CROW_ROUTE(app_, "/api/users")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        try {
            auto rows = db_->query(
                "SELECT id, username, role, created_at FROM users ORDER BY id");
            json arr = json::array();
            for (auto& row : rows) {
                json obj;
                for (auto& [col, val] : row) obj[col] = val;
                arr.push_back(obj);
            }
            return jsonOk({{"users", arr}});
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // POST /api/users — create user  { username, password, role }
    CROW_ROUTE(app_, "/api/users").methods("POST"_method)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        try {
            auto body = json::parse(req.body);
            User u;
            u.username = body.value("username", "");
            u.full_name = body.value("full_name", u.username);
            u.email = body.value("email", "");
            u.phone = body.value("phone", "");
            u.role = roleFromString(body.value("role", "receptionist"));
            u.hotel_id = body.value("hotel_id", 0);
            std::string pass = body.value("password", "");
            if (u.username.empty() || pass.empty()) return jsonErr("username and password required");
            // Simple SHA-256-ish placeholder — reuse AuthManager's create if available
            // For now store hashed using same pattern as seeded admin
            if (!auth_->createUser(u, pass)) return jsonErr("Username already exists", 409);
            int uid = db_->queryInt("SELECT id FROM users WHERE username=?;", {u.username});
            return jsonOk({{"id", uid}, {"username", u.username}, {"role", roleToString(u.role)}}, "User created");
        } catch (const json::exception& e) {
            return jsonErr("Invalid JSON: " + std::string(e.what()));
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });

    // DELETE /api/users/:id
    CROW_ROUTE(app_, "/api/users/<int>").methods("DELETE"_method)
    ([this](const crow::request& req, int uid) {
        if (!requireAdmin(req)) return jsonErr("Admin only", 403);
        try {
            db_->execute("DELETE FROM users WHERE id=? AND username != 'admin'",
                         {std::to_string(uid)});
            return jsonOk({{"id", uid}}, "User deleted");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 500);
        }
    });
}


void APIRouter::registerStaticRoutes() {

    // Serve index.html at "/"
    CROW_ROUTE(app_, "/")
    ([]() {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/index.html");
        return res;
    });

    CROW_ROUTE(app_, "/login.html")
    ([]() {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/login.html");
        return res;
    });

    CROW_ROUTE(app_, "/register.html")
    ([]() {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/register.html");
        return res;
    });

    // Serve all static frontend files: css/, js/, pages/
    CROW_ROUTE(app_, "/css/<path>")
    ([](const std::string& filename) {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/css/" + filename);
        return res;
    });

    CROW_ROUTE(app_, "/js/<path>")
    ([](const std::string& filename) {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/js/" + filename);
        return res;
    });

    CROW_ROUTE(app_, "/images/<path>")
    ([](const std::string& filename) {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/images/" + filename);
        return res;
    });

    // POST /api/upload
    CROW_ROUTE(app_, "/api/upload")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        try {
            crow::multipart::message msg(req);
            for (const auto& part : msg.parts) {
                const std::string original_name = multipartFilename(part);
                if (original_name.empty()) continue;
                UploadResult saved = uploads_->saveDocument(
                    original_name,
                    multipartContentType(part),
                    part.body);
                return jsonOk(saved.toJson(), "File uploaded successfully");
            }
            return jsonErr("No file part in request", 400);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/uploads/<path>")
    ([](const std::string& path) {
        crow::response res;
        if (path.find("..") != std::string::npos) return crow::response(400);
        const std::filesystem::path backend_path = std::filesystem::path("backend/uploads") / path;
        if (std::filesystem::exists(backend_path)) {
            res.set_static_file_info_unsafe(backend_path.string());
        } else {
            res.set_static_file_info_unsafe((std::filesystem::path("uploads") / path).string());
        }
        return res;
    });

    CROW_ROUTE(app_, "/pages/<path>")
    ([](const std::string& filename) {
        crow::response res;
        res.set_static_file_info_unsafe("frontend/pages/" + filename);
        return res;
    });
}


// ═════════════════════════════════════════════════════════════════════════════
//  PHASE 5 — BOOKING ROUTES
// ═════════════════════════════════════════════════════════════════════════════
void APIRouter::registerBookingRoutes() {

    CROW_ROUTE(app_, "/api/me/bookings")
    ([this](const crow::request& req) {
        auto s = getSession(req);
        if (!s && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = s ? s->user_id : 1;
        auto rows = db_->query(
            "SELECT b.*, h.name AS hotel_name, h.location AS hotel_location, h.image_url AS hotel_image "
            "FROM bookings b LEFT JOIN hotels h ON b.hotel_id=h.id "
            "WHERE b.created_by=? ORDER BY b.created_at DESC;",
            {std::to_string(user_id)});
        return jsonOk(dbRowsToJson(rows));
    });

    // POST /api/bookings/preview  — cost estimate, no DB write
    CROW_ROUTE(app_, "/api/bookings/preview")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            auto body = json::parse(req.body);
            Booking b;
            b.hotel_id       = body.value("hotel_id",      0);
            b.room_type_id   = body.value("room_type_id",  0);
            b.check_in_date  = body.value("check_in_date", "");
            b.nights         = body.value("nights",        1);
            b.breakfast      = body.value("breakfast",     false);
            b.activities_json= body.value("activities",    "[]");
            b.check_out_date = Utils::addDaysToDate(b.check_in_date, b.nights);
            if (b.room_type_id == 0 || b.check_in_date.empty())
                return jsonErr("room_type_id and check_in_date required", 400);
            b = bookings_->calculateCosts(b);
            
            std::string coupon_code = body.value("coupon_code", "");
            CouponResult coupon;
            if (!coupon_code.empty()) {
                coupon = evaluateCoupon(*db_, b.hotel_id, b.subtotal, coupon_code);
            }
            
            if (coupon.discount > b.total_amount) coupon.discount = b.total_amount;
            b.total_amount -= coupon.discount;
            json res = b.toJson();
            res["coupon_code"] = coupon.valid ? coupon.code : "";
            res["coupon_valid"] = coupon.valid;
            res["coupon_message"] = coupon.message;
            res["discount_amount"] = coupon.discount;
            res.update(commissionPreview(*db_, b.total_amount));
            return jsonOk(res, "Cost preview");
        } catch (...) { return jsonErr("Invalid request", 400); }
    });

    // POST /api/bookings — create booking
    CROW_ROUTE(app_, "/api/bookings")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        int user_id = sess ? sess->user_id : 1;
        try {
            auto body = json::parse(req.body);
            Booking b;
            b.hotel_id        = body.value("hotel_id",       0);
            b.room_type_id    = body.value("room_type_id",   0);
            b.guest_name      = body.value("guest_name",     "");
            b.guest_email     = body.value("guest_email",    "");
            b.guest_phone     = body.value("guest_phone",    "");
            b.guest_address   = body.value("guest_address",  "");
            
            std::string special_requests = body.value("special_requests", "");
            if (!special_requests.empty()) {
                if (!b.guest_address.empty()) {
                    b.guest_address += " | ";
                }
                b.guest_address += "Special Requests: " + special_requests;
            }

            b.check_in_date   = body.value("check_in_date",  "");
            b.nights          = body.value("nights",         1);
            b.breakfast       = body.value("breakfast",      false);
            b.activities_json = body.value("activities",     "[]");
            b.created_by      = user_id;
            b.payment_method  = body.value("payment_method", "Pay at hotel");
            b.payment_status  = "pending";
            b.check_out_date  = Utils::addDaysToDate(b.check_in_date, b.nights);

            if (b.hotel_id==0 || b.room_type_id==0)
                return jsonErr("hotel_id and room_type_id required", 400);
            const std::string paymentStatus = hotelPaymentRestriction(*db_, b.hotel_id);
            if (paymentStatus == "Suspended")
                return jsonErr("This hotel is temporarily unavailable for new bookings.", 403);
            if (b.guest_name.empty() || b.guest_phone.empty())
                return jsonErr("guest_name and guest_phone required", 400);
            if (!Utils::isValidDate(b.check_in_date))
                return jsonErr("Invalid check_in_date (YYYY-MM-DD)", 400);

            auto result = bookings_->createBooking(b);
            if (!result) return jsonErr("Failed to create booking", 500);

            db_->execute("UPDATE bookings SET payment_method=?, payment_status=? WHERE id=?;",
                         {b.payment_method, b.payment_status, std::to_string(result->id)});
            result->payment_method = b.payment_method;
            result->payment_status = b.payment_status;

            // Securely apply valid coupon discounts to the final record
            std::string coupon_code = body.value("coupon_code", "");
            if (!coupon_code.empty()) {
                CouponResult coupon = evaluateCoupon(*db_, result->hotel_id, result->subtotal, coupon_code);
                if (coupon.valid) {
                    if (coupon.discount > result->total_amount) coupon.discount = result->total_amount;
                    result->total_amount -= coupon.discount;
                    result->coupon_code = coupon.code;
                    result->discount_amount = coupon.discount;
                    db_->execute(R"sql(
                        UPDATE bookings
                        SET total_amount=?, coupon_code=?, discount_amount=?
                        WHERE id=?;
                    )sql", {
                        std::to_string(result->total_amount),
                        coupon.code,
                        std::to_string(coupon.discount),
                        std::to_string(result->id)
                    });
                    db_->execute("UPDATE coupons SET usage_count = usage_count + 1 WHERE id=?;",
                                 {std::to_string(coupon.id)});
                }
            }

            const double rate = commissionRate(*db_);
            result->commission_rate = rate;
            result->commission_amount = roundMoney(result->total_amount * rate / 100.0);
            result->hotel_amount = roundMoney(result->total_amount - result->commission_amount);
            db_->execute(R"sql(
                UPDATE bookings
                SET commission_rate=?, commission_amount=?, hotel_amount=?
                WHERE id=?;
            )sql", {
                std::to_string(result->commission_rate),
                std::to_string(result->commission_amount),
                std::to_string(result->hotel_amount),
                std::to_string(result->id)
            });
            
            const std::string transactionRef = "PENDING-" + result->booking_id;

            // Create an automatic payout ledger entry for this booking
            if (b.payment_method == "Pay Now") {
                db_->execute(R"sql(
                    INSERT INTO hotel_payouts (
                        payout_id, hotel_id, booking_id, booking_ref,
                        gross_amount, commission_amount, payout_amount,
                        status, transaction_ref
                    )
                    VALUES (?, ?, ?, ?, ?, ?, ?, 'pending', ?);
                )sql", {
                    "PO-" + result->booking_id,
                    std::to_string(result->hotel_id),
                    std::to_string(result->id),
                    result->booking_id,
                    std::to_string(result->total_amount),
                    std::to_string(result->commission_amount),
                    std::to_string(result->hotel_amount),
                    transactionRef
                });
            }

            const std::string orderId = "order_" + Utils::generateUUID().substr(0, 8);
            // Get hotel's UPI ID
            auto hotelInfoRows = db_->query("SELECT name, upi_id FROM hotels WHERE id=?;", {std::to_string(result->hotel_id)});
            std::string hotelUpiId = "";
            std::string hotelName = "";
            if (!hotelInfoRows.empty()) {
                hotelUpiId = rowValue(hotelInfoRows[0], "upi_id");
                hotelName = rowValue(hotelInfoRows[0], "name");
            }

            db_->execute(R"sql(
                INSERT INTO payments (
                    booking_id, booking_ref, hotel_id, user_id,
                    provider, payment_method, amount, status, transaction_ref,
                    upi_id, merchant_name,
                    commission_rate, commission_amount, hotel_amount
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
            )sql", {
                std::to_string(result->id),
                result->booking_id,
                std::to_string(result->hotel_id),
                std::to_string(user_id),
                result->payment_method,
                result->payment_method,
                std::to_string(result->total_amount),
                result->payment_status,
                transactionRef,
                hotelUpiId,
                hotelName,
                std::to_string(result->commission_rate),
                std::to_string(result->commission_amount),
                std::to_string(result->hotel_amount)
            });

            json historyPayload = result->toJson();
            historyPayload["payment_method"] = result->payment_method;
            historyPayload["payment_status"] = result->payment_status;
            historyPayload["transaction_ref"] = transactionRef;
            db_->execute(R"sql(
                INSERT INTO booking_history (
                    booking_id, booking_ref, user_id, hotel_id,
                    event_type, event_payload
                )
                VALUES (?, ?, ?, ?, ?, ?);
            )sql", {
                std::to_string(result->id),
                result->booking_id,
                std::to_string(user_id),
                std::to_string(result->hotel_id),
                "created",
                historyPayload.dump()
            });

            json response = result->toJson();
            response["razorpay_order_id"] = orderId;
            response["payment_status"] = result->payment_status;
            response.update(paymentConfigJson(*db_, result->total_amount, result->booking_id));
            return jsonOk(response, "Booking created: " + result->booking_id);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    // GET /api/bookings?hotel_id=1&status=active
    CROW_ROUTE(app_, "/api/bookings")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        int hotel_id = 0;
        try { hotel_id = std::stoi(req.url_params.get("hotel_id") ? req.url_params.get("hotel_id") : "0"); } catch(...) {}
        std::string status = req.url_params.get("status") ? req.url_params.get("status") : "active";
        if (hotel_id == 0) return jsonErr("hotel_id query param required", 400);

        auto list = bookings_->getByHotel(hotel_id, status);
        json arr = json::array();
        for (auto& b : list) arr.push_back(b.toJson());
        return jsonOk(arr);
    });

    // GET /api/bookings/:ref
    CROW_ROUTE(app_, "/api/bookings/<string>")
    ([this](const crow::request& req, const std::string& ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto b = bookings_->getByRef(ref);
        if (!b) return jsonErr("Booking not found", 404);
        return jsonOk(b->toJson());
    });

    // POST /api/bookings/:ref/checkin
    CROW_ROUTE(app_, "/api/bookings/<string>/checkin")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, const std::string& ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        auto b = bookings_->getByRef(ref);
        if (!b) return jsonErr("Booking not found", 404);
        if (b->status != BookingStatus::ACTIVE) return jsonErr("Booking is not active", 400);

        // Update booking status to checked_in, room status to occupied, and ensure inventory matches
        db_->execute("UPDATE bookings SET status='checked_in', updated_at=? WHERE booking_id=?;", {Utils::nowISO(), ref});
        db_->execute("UPDATE rooms SET status='occupied' WHERE id=?;", {std::to_string(b->room_id)});
        db_->execute("UPDATE inventory SET status='occupied', last_synced=datetime('now','localtime') WHERE room_id=?;", {std::to_string(b->room_id)});
        
        return jsonOk({}, "Checked in: " + ref);
    });

    // POST /api/bookings/:ref/checkout
    CROW_ROUTE(app_, "/api/bookings/<string>/checkout")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, const std::string& ref) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        if (!bookings_->checkOut(ref))
            return jsonErr("Checkout failed — booking not active or not found", 400);
        return jsonOk({}, "Checked out: " + ref);
    });

    // GET /api/coupons
    CROW_ROUTE(app_, "/api/coupons")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(dbRowsToJson(db_->query(
            "SELECT id, code, description, discount_type, discount_value, min_booking_amount, max_discount, applicable_hotels "
            "FROM coupons "
            "WHERE is_active=1 AND (valid_to IS NULL OR valid_to='' OR valid_to >= date('now')) "
            "ORDER BY id DESC;"
        )));
    });

    // GET /api/admin/coupons
    CROW_ROUTE(app_, "/api/admin/coupons")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        return jsonOk(dbRowsToJson(db_->query("SELECT * FROM coupons ORDER BY id DESC;")));
    });

    // POST /api/admin/coupons
    CROW_ROUTE(app_, "/api/admin/coupons")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body);
            std::string code = Utils::toUpper(Utils::trim(bodyText(body, "code")));
            if (code.empty()) return jsonErr("Coupon code required", 400);
            db_->execute(R"sql(
                INSERT INTO coupons (
                    code, description, discount_type, discount_value,
                    min_booking_amount, max_discount, valid_to,
                    usage_limit, is_active
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
            )sql", {
                code,
                bodyText(body, "description"),
                bodyText(body, "discount_type", "percentage"),
                bodyText(body, "discount_value", "0"),
                bodyText(body, "min_booking_amount", "0"),
                bodyText(body, "max_discount", "0"),
                bodyText(body, "valid_to"),
                bodyText(body, "usage_limit", "0"),
                "1"
            });
            return jsonOk({}, "Coupon created");
        } catch (const std::exception& e) { return jsonErr(e.what(), 400); }
    });

    // DELETE /api/admin/coupons/:id
    CROW_ROUTE(app_, "/api/admin/coupons/<int>").methods(crow::HTTPMethod::Delete)
    ([this](const crow::request& req, int id) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        db_->execute("DELETE FROM coupons WHERE id=?;", {std::to_string(id)});
        return jsonOk({}, "Coupon deleted");
    });
}

// =============================================================================
//  PAYMENT, COMMISSION, PAYOUT, AND REFUND ROUTES
// =============================================================================
void APIRouter::registerPaymentRoutes() {

    CROW_ROUTE(app_, "/api/payments/verify")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess && !requireAuth(req)) return jsonErr("Unauthorized", 401);
        try {
            auto body = json::parse(req.body);
            std::string booking_ref = body.value("booking_ref", "");
            std::string payment_method = body.value("payment_method", "");
            std::string status = body.value("status", "pending");
            std::string transaction_id = body.value("transaction_id", "");
            std::string payment_proof_url = body.value("payment_proof_url", "");

            if (booking_ref.empty()) return jsonErr("booking_ref required", 400);

            db_->execute("UPDATE bookings SET payment_method=?, payment_status=?, transaction_id=?, updated_at=datetime('now','localtime') WHERE booking_id=?;",
                {payment_method, status, transaction_id, booking_ref});

            int count = db_->queryInt("SELECT COUNT(*) FROM payments WHERE booking_ref=?;", {booking_ref});
            if (count == 0) {
                auto bookingRows = db_->query(
                    "SELECT id, hotel_id, user_id, total_amount, commission_rate, commission_amount, hotel_amount FROM bookings WHERE booking_id=?;",
                    {booking_ref});
                if (!bookingRows.empty()) {
                    auto& row = bookingRows[0];
                    std::string b_id = rowValue(row, "id");
                    std::string hotel_id = rowValue(row, "hotel_id");
                    std::string user_id = rowValue(row, "user_id");
                    std::string amount = rowValue(row, "total_amount");
                    std::string commission_rate = rowValue(row, "commission_rate");
                    std::string commission_amount = rowValue(row, "commission_amount");
                    std::string hotel_amount = rowValue(row, "hotel_amount");

                    auto hotelInfoRows = db_->query("SELECT name, upi_id FROM hotels WHERE id=?;", {hotel_id});
                    std::string hotelUpiId = "";
                    std::string hotelName = "";
                    if (!hotelInfoRows.empty()) {
                        hotelUpiId = rowValue(hotelInfoRows[0], "upi_id");
                        hotelName = rowValue(hotelInfoRows[0], "name");
                    }

                    db_->execute(R"sql(
                        INSERT INTO payments (
                            booking_id, booking_ref, hotel_id, user_id,
                            provider, payment_method, amount, status, transaction_ref,
                            upi_id, merchant_name,
                            commission_rate, commission_amount, hotel_amount, payment_proof_url
                        )
                        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
                    )sql", {
                        b_id, booking_ref, hotel_id, user_id,
                        payment_method, payment_method, amount, status, transaction_id,
                        hotelUpiId, hotelName,
                        commission_rate, commission_amount, hotel_amount, payment_proof_url
                    });
                }
            } else {
                db_->execute("UPDATE payments SET payment_method=?, status=?, transaction_ref=?, payment_proof_url=? WHERE booking_ref=?;",
                    {payment_method, status, transaction_id, payment_proof_url, booking_ref});
            }

            return jsonOk({{"status", status}}, "Payment verified");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    CROW_ROUTE(app_, "/api/payments/config")
    ([this](const crow::request& req) {
        if (!requireAuth(req)) return jsonErr("Unauthorized", 401);
        return jsonOk(paymentConfigJson(*db_));
    });

    CROW_ROUTE(app_, "/api/admin/platform-payments")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        refreshCommissionDues(*db_);
        json data = platformPaymentSettingsJson(*db_);
        data["analytics"] = {
            {"total_commission_earned", db_->queryDouble("SELECT COALESCE(SUM(paid_amount),0) FROM monthly_commission_ledger;")},
            {"pending_commission_amount", db_->queryDouble("SELECT COALESCE(SUM(pending_amount),0) FROM monthly_commission_ledger;")},
            {"paid_commission_amount", db_->queryDouble("SELECT COALESCE(SUM(paid_amount),0) FROM monthly_commission_ledger;")},
            {"pending_proofs", db_->queryInt("SELECT COUNT(*) FROM commission_payment_proofs WHERE status='pending';")},
            {"restricted_hotels", db_->queryInt("SELECT COUNT(*) FROM hotels WHERE hotel_status='Restricted';")},
            {"suspended_hotels", db_->queryInt("SELECT COUNT(*) FROM hotels WHERE hotel_status='Suspended';")}
        };
        data["ledgers"] = dbRowsToJson(db_->query(R"sql(
            SELECT l.*, h.name AS hotel_name, h.location, h.hotel_status,
                   h.commission_status, h.days_overdue,
                   (SELECT COUNT(*) FROM commission_payment_proofs p
                    WHERE p.ledger_id=l.ledger_id AND p.status='pending') AS pending_proofs
            FROM monthly_commission_ledger l
            LEFT JOIN hotels h ON h.id=l.hotel_id
            ORDER BY l.due_date DESC, l.id DESC
            LIMIT 150;
        )sql"));
        data["payments"] = dbRowsToJson(db_->query(R"sql(
            SELECT p.*, h.name AS hotel_name, h.location
            FROM commission_payment_proofs p
            LEFT JOIN hotels h ON h.id=p.hotel_id
            ORDER BY p.id DESC
            LIMIT 150;
        )sql"));
        data["hotel_reports"] = dbRowsToJson(db_->query(R"sql(
            SELECT h.id, h.name, h.location, h.hotel_status, h.commission_status,
                   h.commission_due, h.due_date, h.days_overdue,
                   COALESCE(SUM(l.commission_amount),0) AS commission_billed,
                   COALESCE(SUM(l.paid_amount),0) AS commission_paid,
                   COALESCE(SUM(l.pending_amount),0) AS commission_pending
            FROM hotels h
            LEFT JOIN monthly_commission_ledger l ON l.hotel_id=h.id
            GROUP BY h.id
            ORDER BY commission_pending DESC, h.name ASC;
        )sql"));
        data["monthly_revenue"] = dbRowsToJson(db_->query(R"sql(
            SELECT strftime('%Y-%m', COALESCE(NULLIF(reviewed_at,''), created_at)) AS month,
                   COALESCE(SUM(amount),0) AS commission
            FROM commission_payment_proofs
            WHERE status='approved'
            GROUP BY month
            ORDER BY month ASC
            LIMIT 24;
        )sql"));
        data["yearly_revenue"] = dbRowsToJson(db_->query(R"sql(
            SELECT strftime('%Y', COALESCE(NULLIF(reviewed_at,''), created_at)) AS year,
                   COALESCE(SUM(amount),0) AS commission
            FROM commission_payment_proofs
            WHERE status='approved'
            GROUP BY year
            ORDER BY year ASC;
        )sql"));
        data["audit"] = dbRowsToJson(db_->query(R"sql(
            SELECT a.*, h.name AS hotel_name
            FROM commission_audit_log a
            LEFT JOIN hotels h ON h.id=a.hotel_id
            ORDER BY a.id DESC
            LIMIT 80;
        )sql"));
        return jsonOk(data);
    });

    CROW_ROUTE(app_, "/api/admin/platform-payments/settings")
    .methods(crow::HTTPMethod::Put)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body.empty() ? "{}" : req.body);
            const std::string currentUpi = appSetting(*db_, "platform_upi_id", "staysphere@upi");
            const std::string currentMerchant = appSetting(*db_, "platform_merchant_name", "StaySphere");
            std::string upiId = Utils::trim(bodyText(body, "upi_id", currentUpi));
            std::string merchant = Utils::trim(bodyText(body, "merchant_name", currentMerchant));
            if (merchant.empty()) merchant = "StaySphere";
            if (!isValidUpiId(upiId)) return jsonErr("Enter a valid UPI ID, for example platform@upi", 400);

            double rate = body.contains("commission_rate")
                ? toDoubleSafe(bodyText(body, "commission_rate"), commissionRate(*db_))
                : commissionRate(*db_);
            if (rate < 0.0 || rate > 60.0) return jsonErr("Commission rate must be between 0 and 60", 400);

            const int activeDays = body.contains("active_days")
                ? toIntSafe(bodyText(body, "active_days"), settingInt(*db_, "commission_active_days", 30))
                : settingInt(*db_, "commission_active_days", 30);
            const int graceUntilDays = body.contains("grace_until_days")
                ? toIntSafe(bodyText(body, "grace_until_days"), settingInt(*db_, "commission_grace_until_days", 37))
                : settingInt(*db_, "commission_grace_until_days", 37);
            const int suspensionDays = body.contains("suspension_days")
                ? toIntSafe(bodyText(body, "suspension_days"), settingInt(*db_, "commission_suspension_days", 60))
                : settingInt(*db_, "commission_suspension_days", 60);
            if (activeDays < 0 || graceUntilDays < activeDays || suspensionDays <= graceUntilDays)
                return jsonErr("Overdue rules must follow active <= grace < suspension days", 400);

            const bool enabled = body.contains("commission_collection_enabled")
                ? boolParam(bodyText(body, "commission_collection_enabled", "false"))
                : commissionCollectionEnabled(*db_);
            const std::string notificationTemplate = bodyText(
                body,
                "notification_template",
                appSetting(*db_, "commission_notification_template",
                    "Your commission invoice is due. Please pay using the platform UPI QR and upload proof for verification."));
            const std::string qrPayload = buildUpiUri(upiId, merchant, 0.0, "StaySphere commission");

            db_->beginTransaction();
            try {
                setAppSetting(*db_, "platform_upi_id", upiId);
                setAppSetting(*db_, "platform_merchant_name", merchant);
                setAppSetting(*db_, "platform_upi_qr_payload", qrPayload);
                setAppSetting(*db_, "platform_commission_rate", std::to_string(rate));
                setAppSetting(*db_, "commission_collection_enabled", enabled ? "1" : "0");
                setAppSetting(*db_, "commission_active_days", std::to_string(activeDays));
                setAppSetting(*db_, "commission_grace_until_days", std::to_string(graceUntilDays));
                setAppSetting(*db_, "commission_suspension_days", std::to_string(suspensionDays));
                setAppSetting(*db_, "commission_notification_template", notificationTemplate);
                auto session = getSession(req);
                recordCommissionAudit(*db_, "settings", "platform-payments", 0, session ? session->user_id : 0,
                    "settings_updated", {
                        {"upi_id", upiId},
                        {"commission_rate", rate},
                        {"enabled", enabled},
                        {"active_days", activeDays},
                        {"grace_until_days", graceUntilDays},
                        {"suspension_days", suspensionDays}
                    });
                db_->commit();
            } catch (...) {
                db_->rollback();
                throw;
            }
            refreshCommissionDues(*db_);
            return jsonOk(platformPaymentSettingsJson(*db_), "Platform payment settings saved");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/api/manager/commission")
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess) return jsonErr("Unauthorized", 401);
        int hotelId = toIntSafe(firstParam(req, "hotel_id", std::to_string(sess->hotel_id)));
        if (hotelId <= 0) return jsonErr("hotel_id required", 400);
        if (!canManageHotel(*sess, hotelId)) return jsonErr("Forbidden", 403);
        refreshCommissionDues(*db_);

        auto hotelRows = db_->query(R"sql(
            SELECT id, name, location, commission_due, due_date, days_overdue,
                   commission_status, hotel_status
            FROM hotels
            WHERE id=?;
        )sql", {std::to_string(hotelId)});
        if (hotelRows.empty()) return jsonErr("Hotel not found", 404);

        json data;
        data["hotel"] = dbRowToJson(hotelRows[0]);
        data["ledgers"] = dbRowsToJson(db_->query(R"sql(
            SELECT *
            FROM monthly_commission_ledger
            WHERE hotel_id=?
            ORDER BY due_date DESC, id DESC
            LIMIT 36;
        )sql", {std::to_string(hotelId)}));
        data["payments"] = dbRowsToJson(db_->query(R"sql(
            SELECT *
            FROM commission_payment_proofs
            WHERE hotel_id=?
            ORDER BY id DESC
            LIMIT 80;
        )sql", {std::to_string(hotelId)}));
        data["history"] = data["payments"];
        const double amountDue = db_->queryDouble(
            "SELECT COALESCE(SUM(pending_amount),0) FROM monthly_commission_ledger WHERE hotel_id=?;",
            {std::to_string(hotelId)});
        data["commission_due"] = roundMoney(amountDue);
        json platform = platformPaymentSettingsJson(*db_);
        platform["amount"] = roundMoney(amountDue);
        platform["upi_uri"] = amountDue > 0.0
            ? buildUpiUri(platform.value("upi_id", "staysphere@upi"),
                          platform.value("merchant_name", "StaySphere"),
                          amountDue,
                          "StaySphere commission hotel " + std::to_string(hotelId))
            : platform.value("upi_qr_payload", "");
        data["platform_payment"] = platform;
        data["summary"] = {
            {"commission_due", roundMoney(amountDue)},
            {"pending_ledgers", db_->queryInt("SELECT COUNT(*) FROM monthly_commission_ledger WHERE hotel_id=? AND pending_amount>0;", {std::to_string(hotelId)})},
            {"paid_commission", db_->queryDouble("SELECT COALESCE(SUM(paid_amount),0) FROM monthly_commission_ledger WHERE hotel_id=?;", {std::to_string(hotelId)})},
            {"approved_payments", db_->queryInt("SELECT COUNT(*) FROM commission_payment_proofs WHERE hotel_id=? AND status='approved';", {std::to_string(hotelId)})}
        };
        return jsonOk(data);
    });

    CROW_ROUTE(app_, "/api/manager/earnings")
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess) return jsonErr("Unauthorized", 401);
        int hotelId = toIntSafe(firstParam(req, "hotel_id", std::to_string(sess->hotel_id)));
        if (hotelId <= 0) return jsonErr("hotel_id required", 400);
        if (!canManageHotel(*sess, hotelId)) return jsonErr("Forbidden", 403);

        // Clean up any errant payouts created for 'Pay at Property' bookings
        db_->execute(R"sql(
            DELETE FROM hotel_payouts
            WHERE hotel_id=? AND booking_id IN (
                SELECT id FROM bookings WHERE payment_method != 'Pay Now'
            );
        )sql", {std::to_string(hotelId)});

        // Auto-backfill missing payouts for older bookings
        db_->execute(R"sql(
            INSERT INTO hotel_payouts (payout_id, hotel_id, booking_id, booking_ref, gross_amount, commission_amount, payout_amount, status, transaction_ref)
            SELECT 'PO-' || booking_id, hotel_id, id, booking_id, total_amount, commission_amount, hotel_amount, 'pending', 'PENDING-' || booking_id
            FROM bookings b
            WHERE hotel_id=? AND status!='cancelled' AND payment_method='Pay Now'
              AND NOT EXISTS (SELECT 1 FROM hotel_payouts p WHERE p.booking_id = b.id);
        )sql", {std::to_string(hotelId)});

        json data;
        data["total_bookings"] = db_->queryInt("SELECT COUNT(*) FROM bookings WHERE hotel_id=? AND status!='cancelled';", {std::to_string(hotelId)});
        data["gross_revenue"] = db_->queryDouble("SELECT COALESCE(SUM(total_amount),0) FROM bookings WHERE hotel_id=? AND status!='cancelled';", {std::to_string(hotelId)});
        data["commission_deducted"] = db_->queryDouble("SELECT COALESCE(SUM(commission_amount),0) FROM bookings WHERE hotel_id=? AND status!='cancelled';", {std::to_string(hotelId)});
        data["earnings_after_commission"] = db_->queryDouble("SELECT COALESCE(SUM(hotel_amount),0) FROM bookings WHERE hotel_id=? AND status!='cancelled';", {std::to_string(hotelId)});
        
        data["pending_payouts"] = db_->queryDouble("SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE hotel_id=? AND status='pending';", {std::to_string(hotelId)});
        data["completed_payouts"] = db_->queryDouble("SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE hotel_id=? AND status='completed';", {std::to_string(hotelId)});

        data["revenue_analytics"] = dbRowsToJson(db_->query(R"sql(
            SELECT strftime('%Y-%m', created_at) AS month, COALESCE(SUM(total_amount),0) AS earnings
            FROM bookings
            WHERE hotel_id=? AND status!='cancelled'
            GROUP BY month
            ORDER BY month ASC
            LIMIT 12;
        )sql", {std::to_string(hotelId)}));

        data["bookings"] = dbRowsToJson(db_->query(R"sql(
            SELECT b.booking_id, b.guest_name, b.total_amount, b.commission_amount, b.commission_rate, b.hotel_amount, b.payment_status, b.payment_method,
                   (SELECT payment_proof_url FROM payments p WHERE p.booking_ref = b.booking_id ORDER BY p.id DESC LIMIT 1) AS payment_proof_url
            FROM bookings b
            WHERE b.hotel_id=? AND b.status!='cancelled'
            ORDER BY created_at DESC
            LIMIT 50;
        )sql", {std::to_string(hotelId)}));

        data["payouts"] = dbRowsToJson(db_->query(R"sql(
            SELECT p.payout_id, p.booking_ref, b.guest_name, p.payout_amount, p.status, p.scheduled_at, p.paid_at
            FROM hotel_payouts p
            LEFT JOIN bookings b ON p.booking_id = b.id
            WHERE p.hotel_id=?
            ORDER BY p.created_at DESC
            LIMIT 50;
        )sql", {std::to_string(hotelId)}));

        return jsonOk(data);
    });

    CROW_ROUTE(app_, "/api/manager/verify-guest-payment")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess) return jsonErr("Unauthorized", 401);
        try {
            auto body = json::parse(req.body.empty() ? "{}" : req.body);
            std::string booking_ref = body.value("booking_ref", "");
            if (booking_ref.empty()) return jsonErr("booking_ref required", 400);

            int hotelId = db_->queryInt("SELECT hotel_id FROM bookings WHERE booking_id=?;", {booking_ref});
            if (hotelId <= 0) return jsonErr("Booking not found", 404);
            if (!canManageHotel(*sess, hotelId)) return jsonErr("Forbidden", 403);

            db_->execute("UPDATE bookings SET payment_status='paid', updated_at=datetime('now','localtime') WHERE booking_id=?;", {booking_ref});
            db_->execute("UPDATE payments SET status='paid' WHERE booking_ref=?;", {booking_ref});
            db_->execute("UPDATE hotel_payouts SET status='completed', paid_at=datetime('now','localtime') WHERE booking_ref=?;", {booking_ref});

            return jsonOk({}, "Payment verified");
        } catch (...) { return jsonErr("Invalid JSON", 400); }
    });

    CROW_ROUTE(app_, "/api/manager/commission-payments")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        auto sess = getSession(req);
        if (!sess) return jsonErr("Unauthorized", 401);
        try {
            auto body = json::parse(req.body.empty() ? "{}" : req.body);
            std::string ledgerId = bodyText(body, "ledger_id");
            std::string proofUrl = bodyText(body, "payment_proof_url");
            std::string transactionRef = bodyText(body, "transaction_ref");
            if (ledgerId.empty()) return jsonErr("ledger_id required", 400);
            if (proofUrl.empty()) return jsonErr("Payment proof screenshot is required", 400);
            auto ledgerRows = db_->query("SELECT * FROM monthly_commission_ledger WHERE ledger_id=?;", {ledgerId});
            if (ledgerRows.empty()) return jsonErr("Commission invoice not found", 404);
            const int hotelId = toIntSafe(rowValue(ledgerRows[0], "hotel_id"));
            if (!canManageHotel(*sess, hotelId)) return jsonErr("Forbidden", 403);
            const double pending = toDoubleSafe(rowValue(ledgerRows[0], "pending_amount"));
            double amount = body.contains("amount") ? toDoubleSafe(bodyText(body, "amount"), pending) : pending;
            amount = roundMoney(amount);
            if (pending <= 0.0) return jsonErr("This commission invoice is already paid", 400);
            if (amount <= 0.0 || amount > pending + 0.01)
                return jsonErr("Payment amount must be greater than zero and not exceed the pending due", 400);
            if (db_->queryInt("SELECT COUNT(*) FROM commission_payment_proofs WHERE ledger_id=? AND status='pending';", {ledgerId}) > 0)
                return jsonErr("A payment proof for this invoice is already awaiting review", 409);
            if (transactionRef.empty()) transactionRef = "COM-" + Utils::generateUUID().substr(0, 8);

            const std::string proofId = "CPF-" + Utils::generateUUID().substr(0, 10);
            const std::string upiId = appSetting(*db_, "platform_upi_id", "staysphere@upi");
            db_->beginTransaction();
            try {
                db_->execute(R"sql(
                    INSERT INTO commission_payment_proofs (
                        proof_id, ledger_id, hotel_id, amount, upi_id,
                        payment_proof_url, transaction_ref, status, submitted_by
                    ) VALUES (?, ?, ?, ?, ?, ?, ?, 'pending', ?);
                )sql", {
                    proofId,
                    ledgerId,
                    std::to_string(hotelId),
                    std::to_string(amount),
                    upiId,
                    proofUrl,
                    transactionRef,
                    std::to_string(sess->user_id)
                });
                db_->execute("UPDATE monthly_commission_ledger SET status='pending', updated_at=datetime('now','localtime') WHERE ledger_id=?;", {ledgerId});
                db_->commit();
            } catch (...) {
                db_->rollback();
                throw;
            }
            refreshCommissionDues(*db_);
            return jsonOk({{"proof_id", proofId}}, "Payment proof uploaded for review");
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/api/admin/commission-payments/review")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        try {
            auto body = json::parse(req.body.empty() ? "{}" : req.body);
            const int proofDbId = toIntSafe(bodyText(body, "proof_id"));
            const std::string action = bodyText(body, "action"); // approved, rejected
            if (proofDbId <= 0 || (action != "approved" && action != "rejected"))
                return jsonErr("Valid proof_id and action (approved/rejected) required", 400);
            
            auto rows = db_->query("SELECT * FROM commission_payment_proofs WHERE id=?;", {std::to_string(proofDbId)});
            if (rows.empty()) return jsonErr("Proof not found", 404);
            const std::string currentStatus = rowValue(rows[0], "status");
            if (currentStatus != "pending") return jsonErr("Proof is already " + currentStatus, 400);

            const std::string ledgerId = rowValue(rows[0], "ledger_id");
            const int hotelId = toIntSafe(rowValue(rows[0], "hotel_id"));
            const double amount = toDoubleSafe(rowValue(rows[0], "amount"));
            
            auto sess = getSession(req);
            const int adminId = sess ? sess->user_id : 0;
            
            db_->beginTransaction();
            try {
                db_->execute(R"sql(
                    UPDATE commission_payment_proofs
                    SET status=?, remarks=?, reviewed_by=?, reviewed_at=datetime('now','localtime'), updated_at=datetime('now','localtime')
                    WHERE id=?;
                )sql", {action, bodyText(body, "remarks"), std::to_string(adminId), std::to_string(proofDbId)});

                if (action == "approved") {
                    db_->execute(R"sql(
                        UPDATE monthly_commission_ledger
                        SET paid_amount = paid_amount + ?,
                            pending_amount = CASE WHEN pending_amount - ? < 0 THEN 0 ELSE pending_amount - ? END,
                            status = CASE WHEN pending_amount - ? <= 0 THEN 'paid' ELSE 'pending' END,
                            paid_at = CASE WHEN pending_amount - ? <= 0 THEN datetime('now','localtime') ELSE paid_at END,
                            updated_at = datetime('now','localtime')
                        WHERE ledger_id=?;
                    )sql", {
                        std::to_string(amount), std::to_string(amount), std::to_string(amount),
                        std::to_string(amount), std::to_string(amount), ledgerId
                    });
                } else {
                    db_->execute(R"sql(
                        UPDATE monthly_commission_ledger
                        SET status = CASE
                            WHEN pending_amount <= 0 THEN 'paid'
                            WHEN date(due_date) < date('now') THEN 'overdue'
                            ELSE 'pending' END,
                            updated_at=datetime('now','localtime')
                        WHERE ledger_id=?;
                    )sql", {ledgerId});
                }

                recordCommissionAudit(*db_, "payment_proof", ledgerId, hotelId, adminId,
                    "proof_" + action, {
                        {"proof_id", rowValue(rows[0], "proof_id")},
                        {"amount", amount},
                        {"remarks", bodyText(body, "remarks")}
                    });

                db_->commit();
            } catch (...) {
                db_->rollback();
                throw;
            }
            refreshCommissionDues(*db_);
            return jsonOk({}, "Payment proof " + action);
        } catch (const std::exception& e) {
            return jsonErr(e.what(), 400);
        }
    });

    CROW_ROUTE(app_, "/api/admin/revenue")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        json data;
        data["total_platform_revenue"] = db_->queryDouble("SELECT COALESCE(SUM(total_amount),0) FROM bookings WHERE status!='cancelled';");
        data["total_commissions_earned"] = db_->queryDouble("SELECT COALESCE(SUM(commission_amount),0) FROM bookings WHERE payment_status='paid' AND status!='cancelled';");
        data["gross_revenue"] = data["total_platform_revenue"];
        data["pending_payouts"] = db_->queryDouble("SELECT COALESCE(SUM(payout_amount),0) FROM hotel_payouts WHERE status='pending';");
        data["failed_transactions"] = db_->queryInt("SELECT COUNT(*) FROM payments WHERE status='failed';");
        data["refund_requests"] = db_->queryInt("SELECT COUNT(*) FROM refund_requests WHERE status='requested';");
        data["top_hotels"] = dbRowsToJson(db_->query(R"sql(
            SELECT h.name, h.location, h.upi_id, h.commission_status,
                   COALESCE(SUM(b.total_amount),0) AS gross, COALESCE(SUM(b.commission_amount),0) AS commission, COUNT(b.id) AS bookings 
            FROM hotels h LEFT JOIN bookings b ON b.hotel_id=h.id AND b.status!='cancelled'
            GROUP BY h.id ORDER BY gross DESC LIMIT 10;
        )sql"));
        data["hotel_dues"] = dbRowsToJson(db_->query(R"sql(
            SELECT id, name, location, upi_id, commission_due, due_date, commission_status
            FROM hotels
            ORDER BY CASE WHEN commission_status='Overdue' THEN 1 WHEN commission_status='Pending' THEN 2 ELSE 3 END, commission_due DESC LIMIT 10;
        )sql"));
        data["monthly_revenue"] = dbRowsToJson(db_->query(R"sql(
            SELECT strftime('%Y-%m', created_at) AS month, COALESCE(SUM(commission_amount),0) AS commission
            FROM bookings WHERE payment_status='paid' AND status!='cancelled'
            GROUP BY month ORDER BY month ASC LIMIT 12;
        )sql"));
        data["booking_trends"] = dbRowsToJson(db_->query(R"sql(
            SELECT date(created_at) AS day, COUNT(id) AS bookings
            FROM bookings WHERE status!='cancelled' GROUP BY day ORDER BY day DESC LIMIT 14;
        )sql"));
        return jsonOk(data);
    });

    CROW_ROUTE(app_, "/api/admin/payouts")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        return jsonOk(dbRowsToJson(db_->query("SELECT p.*, h.name AS hotel_name FROM hotel_payouts p LEFT JOIN hotels h ON p.hotel_id = h.id ORDER BY p.created_at DESC;")));
    });

    CROW_ROUTE(app_, "/api/admin/payouts/<int>/complete")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int payout_id) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        db_->execute("UPDATE hotel_payouts SET status='completed', paid_at=datetime('now','localtime') WHERE id=?;", {std::to_string(payout_id)});
        return jsonOk({}, "Payout marked as completed");
    });

    CROW_ROUTE(app_, "/api/admin/transactions")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        return jsonOk(dbRowsToJson(db_->query("SELECT p.*, h.name AS hotel_name FROM payments p LEFT JOIN hotels h ON p.hotel_id = h.id ORDER BY p.created_at DESC LIMIT 100;")));
    });

    CROW_ROUTE(app_, "/api/admin/refunds")
    ([this](const crow::request& req) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        return jsonOk(dbRowsToJson(db_->query("SELECT r.*, u.full_name AS guest_name FROM refund_requests r LEFT JOIN users u ON r.user_id=u.id ORDER BY r.created_at DESC;")));
    });

    CROW_ROUTE(app_, "/api/admin/refunds/<int>/process")
    .methods(crow::HTTPMethod::Post)
    ([this](const crow::request& req, int refund_id) {
        if (!requireAdmin(req)) return jsonErr("Forbidden", 403);
        db_->execute("UPDATE refund_requests SET status='processed', processed_at=datetime('now','localtime') WHERE id=?;", {std::to_string(refund_id)});
        return jsonOk({}, "Refund processed successfully");
    });
}
// Trigger build
 
