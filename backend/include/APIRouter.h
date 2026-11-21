#pragma once
#include "crow.h"
#include "Database.h"
#include "Models.h"
#include "AuthManager.h"
#include "HotelManager.h"
#include "ImageManager.h"
#include "UploadManager.h"
#include "RoomManager.h"
#include "BookingManager.h"
#include "Managers.h"
#include "AIManager.h"
#include <memory>
#include <optional>

class APIRouter {
public:
    APIRouter(crow::SimpleApp& app, std::shared_ptr<Database> db);
    void registerAll();

private:
    crow::SimpleApp&                     app_;
    std::shared_ptr<Database>            db_;
    std::shared_ptr<AuthManager>         auth_;
    std::shared_ptr<HotelManager>        hotels_;
    std::shared_ptr<ImageManager>        images_;
    std::shared_ptr<UploadManager>       uploads_;
    std::shared_ptr<RoomManager>         rooms_;
    std::shared_ptr<BookingManager>      bookings_;
    std::shared_ptr<BillingManager>      billing_;
    std::shared_ptr<CancellationManager> cancellations_;
    std::shared_ptr<SearchManager>       search_;    // Phase 8
    std::shared_ptr<ReportManager>       reports_;   // Phase 8
    std::shared_ptr<ReceiptManager>      receipts_;  // Phase 9
    std::shared_ptr<BackupManager>       backup_;    // Phase 11
    std::shared_ptr<AIManager>           ai_;        // Phase 11

    void registerCorsHeaders();
    void registerPublicRoutes();
    void registerAuthRoutes();
    void registerHotelRoutes();
    void registerRoomRoutes();
    void registerBookingRoutes();
    void registerPaymentRoutes();
    void registerBillingRoutes();
    void registerCancellationRoutes();
    void registerSearchRoutes();
    void registerReportRoutes();
    void registerReceiptRoutes();
    void registerBackupRoutes();
    void registerAIRoutes();        // Phase 11
    void registerUserRoutes();      // Phase 11
    void registerStaticRoutes();

    crow::response         jsonOk (const json& data, const std::string& msg = "Success");
    crow::response         jsonErr(const std::string& msg, int status = 400);
    std::optional<Session> getSession (const crow::request& req);
    bool requireAuth (const crow::request& req, crow::response& res);
    bool requireAuth (const crow::request& req);
    bool requireAdmin(const crow::request& req, crow::response& res);
    bool requireAdmin(const crow::request& req);
    bool canManageHotel(const Session& session, int hotel_id) const;
};
