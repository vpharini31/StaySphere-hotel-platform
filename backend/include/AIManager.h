#pragma once
#include "Database.h"
#include "Models.h"
#include <vector>
#include <string>
#include <memory>

// ─── Recommendation result ────────────────────────────────────────────────────
struct RoomRecommendation {
    int         room_type_id;
    std::string room_type_name;
    double      base_price;
    double      dynamic_price;      // after occupancy-based adjustment
    double      match_score;        // 0.0 – 100.0
    std::string reason;             // human-readable why this was recommended
    std::vector<std::string> amenities;
};

// ─── Dynamic price result ─────────────────────────────────────────────────────
struct DynamicPriceResult {
    int         room_type_id;
    std::string room_type_name;
    double      base_price;
    double      adjusted_price;
    double      occupancy_rate;     // 0.0 – 1.0
    double      multiplier;         // e.g. 1.25 → +25%
    std::string tier;               // "low" | "normal" | "high" | "peak"
};

// ─── Activity suggestion ──────────────────────────────────────────────────────
struct ActivitySuggestion {
    int         activity_id;
    std::string name;
    double      price;
    int         priority;           // 1 = top pick
    std::string reason;
};

// ─── Occupancy forecast (one data point per day) ──────────────────────────────
struct OccupancyForecast {
    std::string date;
    double      predicted_rate;     // 0.0 – 1.0
    int         predicted_bookings;
    std::string label;              // "Low" | "Normal" | "High" | "Peak"
};

// ─── Guest intelligence card ──────────────────────────────────────────────────
struct GuestIntelligence {
    std::string guest_name;
    std::string guest_phone;
    int         total_stays;
    double      total_spent;
    double      avg_stay_nights;
    std::string preferred_room_type;
    std::string tier;               // "New" | "Returning" | "Loyal" | "VIP"
    std::string personalised_offer; // e.g. "10% off Suites"
};

// ─────────────────────────────────────────────────────────────────────────────
class AIManager {
public:
    explicit AIManager(std::shared_ptr<Database> db);

    // Smart room recommendation for a guest query
    std::vector<RoomRecommendation> recommendRooms(
        int hotel_id,
        double budget_per_night,
        int    guests,
        int    nights,
        bool   wants_breakfast,
        const  std::vector<std::string>& preferred_amenities);

    // Dynamic pricing for all room types in a hotel
    std::vector<DynamicPriceResult> getDynamicPrices(int hotel_id);

    // Activity suggestions based on stay length and season
    std::vector<ActivitySuggestion> suggestActivities(
        int hotel_id,
        int nights,
        const std::string& checkin_date);

    // Occupancy forecast for next N days
    std::vector<OccupancyForecast> forecastOccupancy(
        int hotel_id,
        int days = 14);

    // Guest intelligence from booking history
    std::optional<GuestIntelligence> getGuestIntelligence(
        int hotel_id,
        const std::string& phone_or_email);

private:
    std::shared_ptr<Database> db_;

    double computeOccupancy(int hotel_id);
    double pricingMultiplier(double occupancy_rate);
    std::string pricingTier(double multiplier);
    std::string datePlusDays(const std::string& date, int days);
    int dayOfWeek(const std::string& date);   // 0=Sun … 6=Sat
};
