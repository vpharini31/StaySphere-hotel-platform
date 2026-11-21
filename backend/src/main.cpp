#include "crow.h"
#include "Database.h"
#include "APIRouter.h"
#include <iostream>
#include <filesystem>
#include <csignal>
#include <memory>

static constexpr uint16_t   DEFAULT_PORT    = 8081;
static const    std::string DEFAULT_DB_PATH = "data/hotel.db";

static crow::SimpleApp* g_app = nullptr;
void signalHandler(int sig) {
    std::cout << "\n[HMS] Shutting down (signal " << sig << ")...\n";
    if (g_app) g_app->stop();
}

int main(int argc, char* argv[]) {
    uint16_t port = DEFAULT_PORT;
    if (argc > 1) {
        try { port = static_cast<uint16_t>(std::stoi(argv[1])); }
        catch (...) {}
    }

    std::cout << "\n";
    std::cout << "==============================================\n";
    std::cout << "   Hotel Management System  v1.0\n";
    std::cout << "   Backend  : C++17 + Crow (MinGW)\n";
    std::cout << "   Database : SQLite3\n";
    std::cout << "   Port     : " << port << "\n";
    std::cout << "==============================================\n\n";

    // ── Create runtime directories ────────────────────────────────────────
    try {
        std::filesystem::create_directories("data/backups");
        std::filesystem::create_directories("receipts");
        std::filesystem::create_directories("backend/uploads");
    } catch (const std::exception& e) {
        std::cerr << "[HMS] Warning: could not create dirs: " << e.what() << "\n";
    }

    // ── Open + initialise database ────────────────────────────────────────
    std::shared_ptr<Database> db;
    try {
        db = std::make_shared<Database>(DEFAULT_DB_PATH);
        db->initialize();
    } catch (const DatabaseException& e) {
        std::cerr << "[HMS] FATAL: " << e.what() << "\n";
        return 1;
    }

    // ── Create Crow app ───────────────────────────────────────────────────
    crow::SimpleApp app;
    g_app = &app;

    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    // ── Register all routes ───────────────────────────────────────────────
    APIRouter router(app, db);
    router.registerAll();

    std::cout << "[HMS] Server running at http://localhost:" << port << "\n";
    std::cout << "[HMS] API health check : http://localhost:" << port << "/api/health\n";
    std::cout << "[HMS] Press Ctrl+C to stop.\n\n";

    app.port(port).multithreaded().run();

    std::cout << "[HMS] Server stopped.\n";
    return 0;
}
