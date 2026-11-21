#pragma once
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <memory>
#include "sqlite3.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Row = map of column name → string value
using DBRow    = std::vector<std::pair<std::string, std::string>>;
using DBResult = std::vector<DBRow>;

// ─────────────────────────────────────────────────────────────────────────────
//  Custom exception
// ─────────────────────────────────────────────────────────────────────────────
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg)
        : std::runtime_error("DB Error: " + msg) {}
};

// ─────────────────────────────────────────────────────────────────────────────
//  Database — thin RAII wrapper around sqlite3*
// ─────────────────────────────────────────────────────────────────────────────
class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    // No copy
    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // ── Lifecycle ────────────────────────────────────────────────────────────
    void initialize();          // create tables + seed default admin
    bool isOpen() const;

    // ── Core query interface ──────────────────────────────────────────────────
    DBResult    query(const std::string& sql,
                      const std::vector<std::string>& params = {});

    void        execute(const std::string& sql,
                        const std::vector<std::string>& params = {});

    int         executeAndGetId(const std::string& sql,
                                const std::vector<std::string>& params = {});

    int         queryInt(const std::string& sql,
                         const std::vector<std::string>& params = {});

    double      queryDouble(const std::string& sql,
                            const std::vector<std::string>& params = {});

    std::string queryString(const std::string& sql,
                            const std::vector<std::string>& params = {});

    // ── Transactions ──────────────────────────────────────────────────────────
    void beginTransaction();
    void commit();
    void rollback();

    // ── Utils ─────────────────────────────────────────────────────────────────
    sqlite3*    handle() { return db_; }   // raw handle if ever needed
    std::string lastError() const;
    int         lastInsertRowId() const;

private:
    sqlite3*    db_      = nullptr;
    std::string db_path_;
    bool        in_tx_   = false;

    // ── Schema creation (called by initialize) ────────────────────────────────
    void createTables();
    void seedDefaultData();

    // ── Helper: bind params to stmt ───────────────────────────────────────────
    void bindParams(sqlite3_stmt* stmt,
                    const std::vector<std::string>& params);
};
