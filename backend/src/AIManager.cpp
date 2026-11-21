#include "AIManager.h"
#include "Utils.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>

AIManager::AIManager(std::shared_ptr<Database> db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────────────────────────────────────
double AIManager::computeOccupancy(int hotel_id) {
    int total = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=?",
        {std::to_string(hotel_id)});
    int occupied = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=? AND status='occupied'",
        {std::to_string(hotel_id)});
    if (total == 0) return 0.0;
    return static_cast<double>(occupied) / total;
}

double AIManager::pricingMultiplier(double occ) {
    if (occ < 0.30) return 0.85;   // discount — low demand
    if (occ < 0.55) return 1.00;   // base price
    if (occ < 0.75) return 1.15;   // mild surge
    if (occ < 0.90) return 1.30;   // high demand
    return 1.50;                    // peak — near full
}

std::string AIManager::pricingTier(double m) {
    if (m < 0.90) return "low";
    if (m < 1.10) return "normal";
    if (m < 1.35) return "high";
    return "peak";
}

std::string AIManager::datePlusDays(const std::string& date, int days) {
    return Utils::addDaysToDate(date, days);
}

int AIManager::dayOfWeek(const std::string& date) {
    // date = "YYYY-MM-DD"
    if (date.size() < 10) return 0;
    std::tm tm{};
    std::istringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    std::mktime(&tm);
    return tm.tm_wday; // 0=Sun, 6=Sat
}

// ─────────────────────────────────────────────────────────────────────────────
//  1. ROOM RECOMMENDATION
// ─────────────────────────────────────────────────────────────────────────────
std::vector<RoomRecommendation> AIManager::recommendRooms(
    int hotel_id,
    double budget_per_night,
    int    guests,
    int    nights,
    bool   wants_breakfast /* used in future phases */,
    const  std::vector<std::string>& preferred_amenities)
{
    (void)wants_breakfast;
    double occ = computeOccupancy(hotel_id);
    double mult = pricingMultiplier(occ);

    auto rows = db_->query(
        "SELECT rt.id, rt.name, rt.base_price, rt.max_capacity, "
        "GROUP_CONCAT(a.name, ',') AS amenities "
        "FROM room_types rt "
        "LEFT JOIN room_type_amenities a ON a.room_type_id = rt.id "
        "WHERE rt.hotel_id=? "
        "GROUP BY rt.id",
        {std::to_string(hotel_id)});

    // Fallback: amenities may be in a different schema — try basic query
    if (rows.empty()) {
        rows = db_->query(
            "SELECT id, name, base_price, max_capacity FROM room_types WHERE hotel_id=?",
            {std::to_string(hotel_id)});
    }

    std::vector<RoomRecommendation> results;

    for (auto& row : rows) {
        RoomRecommendation rec;
        for (auto& [col, val] : row) {
            if (col == "id")            rec.room_type_id   = std::stoi(val.empty()?"0":val);
            if (col == "name")          rec.room_type_name = val;
            if (col == "base_price")    rec.base_price     = std::stod(val.empty()?"0":val);
            if (col == "amenities" && !val.empty()) {
                std::istringstream ss(val);
                std::string a;
                while (std::getline(ss, a, ',')) if (!a.empty()) rec.amenities.push_back(a);
            }
        }
        rec.dynamic_price = std::round(rec.base_price * mult);

        // ── Scoring ─────────────────────────────────────────────────────────
        double score = 50.0; // base

        // Budget fit: prefer rooms at 60–100% of budget; penalise over-budget
        if (budget_per_night > 0) {
            double ratio = rec.dynamic_price / budget_per_night;
            if (ratio <= 1.0 && ratio >= 0.6)  score += 25.0 * (1.0 - std::abs(ratio - 0.85));
            else if (ratio > 1.0)               score -= 40.0 * (ratio - 1.0);
            else                                score -= 10.0; // too cheap for budget
        }

        // Guest capacity (check room_type max_capacity from DB when available)
        // Simple heuristic: suites/luxury fit more guests
        std::string lower = rec.room_type_name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (guests >= 3 && (lower.find("suite") != std::string::npos ||
                            lower.find("luxury") != std::string::npos ||
                            lower.find("penthouse") != std::string::npos)) score += 20.0;
        if (guests <= 2 && lower.find("standard") != std::string::npos)   score += 10.0;

        // Stay length bonus
        if (nights >= 5 && rec.base_price >= 5000) score += 8.0;
        if (nights <= 2 && lower.find("standard") != std::string::npos)    score += 5.0;

        // Amenity match
        for (auto& pref : preferred_amenities) {
            for (auto& a : rec.amenities) {
                if (a.find(pref) != std::string::npos ||
                    pref.find(a)  != std::string::npos) score += 8.0;
            }
        }

        // Cap score
        score = std::max(0.0, std::min(100.0, score));
        rec.match_score = std::round(score * 10.0) / 10.0;

        // ── Reason text ──────────────────────────────────────────────────────
        std::ostringstream reason;
        if (rec.dynamic_price <= budget_per_night)
            reason << "Within your budget. ";
        else
            reason << "Slightly above budget — worth the upgrade. ";
        if (nights >= 5) reason << "Great value for longer stays. ";
        if (guests >= 3 && score > 70) reason << "Ideal capacity for your group. ";
        if (occ > 0.75) reason << "High demand — book soon. ";
        rec.reason = reason.str();

        results.push_back(rec);
    }

    // Sort by match score descending
    std::sort(results.begin(), results.end(),
        [](const RoomRecommendation& a, const RoomRecommendation& b){
            return a.match_score > b.match_score;
        });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
//  2. DYNAMIC PRICING
// ─────────────────────────────────────────────────────────────────────────────
std::vector<DynamicPriceResult> AIManager::getDynamicPrices(int hotel_id) {
    double occ  = computeOccupancy(hotel_id);
    double mult = pricingMultiplier(occ);
    std::string tier = pricingTier(mult);

    auto rows = db_->query(
        "SELECT id, name, base_price FROM room_types WHERE hotel_id=?",
        {std::to_string(hotel_id)});

    std::vector<DynamicPriceResult> results;
    for (auto& row : rows) {
        DynamicPriceResult r;
        r.occupancy_rate = occ;
        r.multiplier     = mult;
        r.tier           = tier;
        for (auto& [col, val] : row) {
            if (col == "id")         r.room_type_id   = std::stoi(val.empty()?"0":val);
            if (col == "name")       r.room_type_name = val;
            if (col == "base_price") r.base_price     = std::stod(val.empty()?"0":val);
        }
        r.adjusted_price = std::round(r.base_price * mult / 100.0) * 100.0; // round to ₹100
        results.push_back(r);
    }
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
//  3. ACTIVITY SUGGESTIONS
// ─────────────────────────────────────────────────────────────────────────────
std::vector<ActivitySuggestion> AIManager::suggestActivities(
    int hotel_id,
    int nights,
    const std::string& checkin_date)
{
    (void)hotel_id;
    int dow = checkin_date.empty() ? 0 : dayOfWeek(checkin_date);
    bool isWeekend = (dow == 0 || dow == 5 || dow == 6);

    auto rows = db_->query("SELECT id, name, price FROM activities");
    std::vector<ActivitySuggestion> results;

    for (auto& row : rows) {
        ActivitySuggestion s;
        s.priority = 5;
        for (auto& [col, val] : row) {
            if (col == "id")    s.activity_id = std::stoi(val.empty()?"0":val);
            if (col == "name")  s.name        = val;
            if (col == "price") s.price       = std::stod(val.empty()?"0":val);
        }

        // Heuristic priority
        std::string lower = s.name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (nights >= 3) {
            if (lower.find("spa") != std::string::npos ||
                lower.find("yoga") != std::string::npos)   s.priority = 1;
            if (lower.find("tour") != std::string::npos)   s.priority = 2;
        }
        if (isWeekend) {
            if (lower.find("pool") != std::string::npos ||
                lower.find("dining") != std::string::npos) s.priority = 1;
        }
        if (nights == 1) {
            if (lower.find("dining") != std::string::npos) s.priority = 1;
        }

        // Reason
        std::ostringstream reason;
        if (s.priority == 1 && nights >= 3)  reason << "Perfect for longer stays. ";
        if (s.priority == 1 && isWeekend)    reason << "Popular on weekends. ";
        if (s.priority == 1 && nights == 1)  reason << "A great add-on for short stays. ";
        if (reason.str().empty())            reason << "Recommended for all guests.";
        s.reason = reason.str();

        results.push_back(s);
    }

    std::sort(results.begin(), results.end(),
        [](const ActivitySuggestion& a, const ActivitySuggestion& b){
            return a.priority < b.priority;
        });

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
//  4. OCCUPANCY FORECAST
// ─────────────────────────────────────────────────────────────────────────────
std::vector<OccupancyForecast> AIManager::forecastOccupancy(int hotel_id, int days) {
    // Base from current occupancy
    double baseOcc = computeOccupancy(hotel_id);

    // Count bookings per day for the next N days using existing booking data
    std::string today = Utils::todayDate();
    int totalRooms = db_->queryInt(
        "SELECT COUNT(*) FROM rooms WHERE hotel_id=?",
        {std::to_string(hotel_id)});
    if (totalRooms == 0) totalRooms = 1;

    std::vector<OccupancyForecast> forecast;

    for (int i = 0; i < days; i++) {
        std::string date = datePlusDays(today, i);
        int dow = dayOfWeek(date);
        bool isWeekend = (dow == 0 || dow == 5 || dow == 6);

        // Count active bookings that overlap this date
        int activeOnDay = db_->queryInt(
            "SELECT COUNT(*) FROM bookings "
            "WHERE hotel_id=? AND status IN ('active', 'checked_in') "
            "AND check_in_date <= ? AND check_out_date > ?",
            {std::to_string(hotel_id), date, date});

        // Blend actual data with heuristic weekend/weekday adjustment
        double dataRate = static_cast<double>(activeOnDay) / totalRooms;
        double weekendBoost = isWeekend ? 0.12 : 0.0;
        // Gentle regression toward base occupancy with weekend factor
        double predicted = (dataRate * 0.6) + (baseOcc * 0.4) + weekendBoost;
        predicted = std::max(0.0, std::min(1.0, predicted));

        OccupancyForecast f;
        f.date = date;
        f.predicted_rate = std::round(predicted * 1000.0) / 1000.0;
        f.predicted_bookings = static_cast<int>(predicted * totalRooms);
        if (predicted < 0.40)      f.label = "Low";
        else if (predicted < 0.65) f.label = "Normal";
        else if (predicted < 0.85) f.label = "High";
        else                       f.label = "Peak";

        forecast.push_back(f);
    }
    return forecast;
}

// ─────────────────────────────────────────────────────────────────────────────
//  5. GUEST INTELLIGENCE
// ─────────────────────────────────────────────────────────────────────────────
std::optional<GuestIntelligence> AIManager::getGuestIntelligence(
    int hotel_id,
    const std::string& phone_or_email)
{
    auto rows = db_->query(
        "SELECT b.guest_name, b.guest_phone, b.nights, b.total_amount, "
        "rt.name AS room_type_name "
        "FROM bookings b "
        "LEFT JOIN rooms r  ON r.id  = b.room_id "
        "LEFT JOIN room_types rt ON rt.id = r.room_type_id "
        "WHERE b.hotel_id=? "
        "AND (b.guest_phone=? OR b.guest_email=?) "
        "AND b.status IN ('active','checked_in','checked_out') "
        "ORDER BY b.id",
        {std::to_string(hotel_id), phone_or_email, phone_or_email});

    if (rows.empty()) return std::nullopt;

    GuestIntelligence gi;
    gi.guest_phone  = phone_or_email;
    gi.total_stays  = static_cast<int>(rows.size());
    gi.total_spent  = 0;
    double totalNights = 0;
    std::map<std::string, int> roomTypeCounts;

    for (auto& row : rows) {
        for (auto& [col, val] : row) {
            if (col == "guest_name")     gi.guest_name = val;
            if (col == "total_amount")   gi.total_spent += std::stod(val.empty()?"0":val);
            if (col == "nights")         totalNights    += std::stod(val.empty()?"0":val);
            if (col == "room_type_name" && !val.empty()) roomTypeCounts[val]++;
        }
    }

    gi.avg_stay_nights = gi.total_stays > 0
        ? std::round((totalNights / gi.total_stays) * 10.0) / 10.0 : 0;

    // Preferred room type
    if (!roomTypeCounts.empty()) {
        auto best = std::max_element(roomTypeCounts.begin(), roomTypeCounts.end(),
            [](auto& a, auto& b){ return a.second < b.second; });
        gi.preferred_room_type = best->first;
    }

    // Tier
    if      (gi.total_stays == 1)  gi.tier = "New";
    else if (gi.total_stays <= 3)  gi.tier = "Returning";
    else if (gi.total_stays <= 7)  gi.tier = "Loyal";
    else                           gi.tier = "VIP";

    // Personalised offer
    if (gi.tier == "VIP")       gi.personalised_offer = "20% off next stay + complimentary breakfast";
    else if (gi.tier == "Loyal") gi.personalised_offer = "15% off " + gi.preferred_room_type + " rooms";
    else if (gi.tier == "Returning") gi.personalised_offer = "10% loyalty discount on next booking";
    else                         gi.personalised_offer = "Welcome offer: free room upgrade on first night";

    return gi;
}
