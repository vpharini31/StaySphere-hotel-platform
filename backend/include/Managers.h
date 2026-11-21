#pragma once
// ═══════════════════════════════════════════════════════════════════════════
//  BillingManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include "Database.h"
#include "Models.h"

class BillingManager {
public:
    static constexpr double GST_RATE         = 0.05;   // 5%
    static constexpr double BREAKFAST_CHARGE = 350.0;  // ₹350 per person per day

    explicit BillingManager(std::shared_ptr<Database> db);

    Bill   generateBill(const Booking& booking, const Hotel& hotel);
    Bill   getBillByBookingRef(const std::string& booking_ref);
    double calculateActivityCost(const std::string& activities_json);

private:
    std::shared_ptr<Database> db_;
};


// ═══════════════════════════════════════════════════════════════════════════
//  CancellationManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include <optional>
#include <string>

class CancellationManager {
public:
    explicit CancellationManager(std::shared_ptr<Database> db);

    std::optional<Cancellation> cancelByRef(const std::string& booking_ref,
                                             const std::string& reason,
                                             int cancelled_by);

    std::optional<Cancellation> cancelByPhone(const std::string& phone,
                                               int hotel_id,
                                               const std::string& reason,
                                               int cancelled_by);

    std::vector<Cancellation>   getCancellations(int hotel_id);
    std::optional<Cancellation> getCancellationById(int id);

private:
    std::shared_ptr<Database> db_;
    Cancellation rowToCancellation(const DBRow& row);
};


// ═══════════════════════════════════════════════════════════════════════════
//  SearchManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include <string>

struct SearchParams {
    int         hotel_id      = 0;
    std::string query;           // free-text
    std::string filter_name;
    std::string filter_phone;
    std::string filter_booking_id;
    std::string filter_room_type;
    std::string filter_status;
    std::string date_from;
    std::string date_to;
    std::string sort_by;         // "cost", "date", "name"
    std::string sort_dir;        // "asc", "desc"
    int         page     = 1;
    int         per_page = 20;
};

struct SearchResult {
    std::vector<Booking> bookings;
    int total_count = 0;
    int page        = 1;
    int per_page    = 20;
    int total_pages = 0;

    json toJson() const {
        json arr = json::array();
        for (auto& b : bookings) arr.push_back(b.toJson());
        return {
            {"bookings",    arr},
            {"total_count", total_count},
            {"page",        page},
            {"per_page",    per_page},
            {"total_pages", total_pages}
        };
    }
};

class SearchManager {
public:
    explicit SearchManager(std::shared_ptr<Database> db);
    SearchResult search(const SearchParams& params);

private:
    std::shared_ptr<Database> db_;
};


// ═══════════════════════════════════════════════════════════════════════════
//  ReportManager.h
// ═══════════════════════════════════════════════════════════════════════════
class ReportManager {
public:
    explicit ReportManager(std::shared_ptr<Database> db);

    RevenueReport  getRevenueReport(int hotel_id,
                                     const std::string& from,
                                     const std::string& to);

    json           getRevenueByRoomType(int hotel_id,
                                         const std::string& from,
                                         const std::string& to);

    json           getMostBookedRoom(int hotel_id);
    json           getMostPopularActivity(int hotel_id);
    json           getBookingStats(int hotel_id);
    double         getOccupancyRate(int hotel_id);
    json           getRevenueTimeline(int hotel_id, int days = 30);

private:
    std::shared_ptr<Database> db_;
};


// ═══════════════════════════════════════════════════════════════════════════
//  ReceiptManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include <string>

class ReceiptManager {
public:
    explicit ReceiptManager(std::shared_ptr<Database> db,
                            const std::string& receipts_dir = "receipts/");

    std::string generateReceipt(const Bill& bill);    // returns file path
    std::string formatBillAsText(const Bill& bill);   // printable string
    std::string formatBillAsPdf(const Bill& bill);    // PDF bytes for download

private:
    std::shared_ptr<Database> db_;
    std::string               receipts_dir_;
    std::string               nowFormatted() const;
};


// ═══════════════════════════════════════════════════════════════════════════
//  BackupManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include <vector>

struct BackupEntry {
    std::string filename;
    std::string created_at;
    long        size_bytes = 0;
};

class BackupManager {
public:
    explicit BackupManager(std::shared_ptr<Database> db,
                           const std::string& db_path,
                           const std::string& backup_dir = "data/backups/");

    std::string             createBackup();            // returns backup path
    bool                    restoreBackup(const std::string& filename);
    std::vector<BackupEntry> listBackups();
    bool                    deleteBackup(const std::string& filename);
    bool                    deleteAllBookings();       // admin only; with confirmation

private:
    std::shared_ptr<Database> db_;
    std::string               db_path_;
    std::string               backup_dir_;
    std::string               timestampedName() const;
};
