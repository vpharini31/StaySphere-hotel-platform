#include "Database.h"
#include "Utils.h"
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <cctype>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
Database::Database(const std::string& db_path) : db_path_(db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw DatabaseException("Cannot open database: " + err);
    }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;",    nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;",     nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;",  nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA cache_size=-8000;",    nullptr, nullptr, nullptr);
    std::cout << "[DB] Opened: " << db_path << "\n";
}

namespace { // anonymous namespace for file-local helpers
    std::string slugify(const std::string& input) {
        std::string result;
        for (char c : input) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else if (std::isspace(static_cast<unsigned char>(c)) && !result.empty() && result.back() != '-') {
                result += '-';
            }
        }
        if (!result.empty() && result.back() == '-') {
            result.pop_back();
        }
        return result;
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        std::cout << "[DB] Closed.\n";
    }
}

bool        Database::isOpen()   const { return db_ != nullptr; }
std::string Database::lastError()const { return db_ ? sqlite3_errmsg(db_) : "no db"; }
int         Database::lastInsertRowId() const {
    return db_ ? static_cast<int>(sqlite3_last_insert_rowid(db_)) : -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Param binding helper
// ─────────────────────────────────────────────────────────────────────────────
void Database::bindParams(sqlite3_stmt* stmt,
                           const std::vector<std::string>& params) {
    for (int i = 0; i < static_cast<int>(params.size()); ++i)
        sqlite3_bind_text(stmt, i + 1,
                          params[i].c_str(), -1, SQLITE_TRANSIENT);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Core query interface
// ─────────────────────────────────────────────────────────────────────────────
DBResult Database::query(const std::string& sql,
                          const std::vector<std::string>& params) {
    DBResult result;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Prepare: " + lastError() + "\nSQL: " + sql);

    bindParams(stmt, params);
    int cols = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DBRow row;
        for (int i = 0; i < cols; ++i) {
            const char* val = reinterpret_cast<const char*>(
                                  sqlite3_column_text(stmt, i));
            row.push_back({sqlite3_column_name(stmt, i), val ? val : ""});
        }
        result.push_back(row);
    }
    sqlite3_finalize(stmt);
    return result;
}

void Database::execute(const std::string& sql,
                        const std::vector<std::string>& params) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException("Prepare: " + lastError() + "\nSQL: " + sql);
    bindParams(stmt, params);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        throw DatabaseException("Execute: " + lastError());
}

int Database::executeAndGetId(const std::string& sql,
                               const std::vector<std::string>& params) {
    execute(sql, params);
    return lastInsertRowId();
}

int Database::queryInt(const std::string& sql,
                        const std::vector<std::string>& params) {
    auto rows = query(sql, params);
    if (!rows.empty() && !rows[0].empty())
        try { return std::stoi(rows[0][0].second); } catch (...) {}
    return 0;
}

double Database::queryDouble(const std::string& sql,
                              const std::vector<std::string>& params) {
    auto rows = query(sql, params);
    if (!rows.empty() && !rows[0].empty())
        try { return std::stod(rows[0][0].second); } catch (...) {}
    return 0.0;
}

std::string Database::queryString(const std::string& sql,
                                   const std::vector<std::string>& params) {
    auto rows = query(sql, params);
    if (!rows.empty() && !rows[0].empty()) return rows[0][0].second;
    return "";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Transactions
// ─────────────────────────────────────────────────────────────────────────────
void Database::beginTransaction() { execute("BEGIN TRANSACTION;"); in_tx_ = true;  }
void Database::commit()           { execute("COMMIT;");            in_tx_ = false; }
void Database::rollback()         { execute("ROLLBACK;");          in_tx_ = false; }

// ─────────────────────────────────────────────────────────────────────────────
//  Schema — CREATE TABLE statements
// ─────────────────────────────────────────────────────────────────────────────
void Database::createTables() {
    beginTransaction();
    try {

    // ── users ──────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS users (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            username        TEXT    NOT NULL UNIQUE,
            password_hash   TEXT    NOT NULL,
            full_name       TEXT    NOT NULL,
            email           TEXT    NOT NULL DEFAULT '',
            phone           TEXT    NOT NULL DEFAULT '',
            role            TEXT    NOT NULL DEFAULT 'receptionist',
            hotel_id        INTEGER NOT NULL DEFAULT 0,
            is_locked       INTEGER NOT NULL DEFAULT 0,
            loyalty_points  INTEGER NOT NULL DEFAULT 0,
            failed_attempts INTEGER NOT NULL DEFAULT 0,
            last_login      TEXT,
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);");

    // ── hotels ─────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS hotels (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_name  TEXT    NOT NULL DEFAULT '',
            name        TEXT    NOT NULL,
            location    TEXT    NOT NULL,
            address     TEXT    NOT NULL DEFAULT '',
            phone       TEXT    NOT NULL DEFAULT '',
            email       TEXT    NOT NULL DEFAULT '',
            website     TEXT    NOT NULL DEFAULT '',
            owner_id    INTEGER NOT NULL DEFAULT 0,
            description TEXT    NOT NULL DEFAULT '',
            amenities   TEXT    NOT NULL DEFAULT '[]',
            image       TEXT    NOT NULL DEFAULT '',
            image_url   TEXT    NOT NULL DEFAULT '',
            property_type TEXT  NOT NULL DEFAULT 'Hotel',
            price       REAL    NOT NULL DEFAULT 0.0,
            price_from  REAL    NOT NULL DEFAULT 0.0,
            rating      REAL    NOT NULL DEFAULT 5.0,
            star_rating REAL    NOT NULL DEFAULT 5.0,
            review_score REAL   NOT NULL DEFAULT 9.0,
            status      TEXT    NOT NULL DEFAULT 'pending',
            is_active   INTEGER NOT NULL DEFAULT 1,
            country     TEXT    NOT NULL DEFAULT '',
            state       TEXT    NOT NULL DEFAULT '',
            city        TEXT    NOT NULL DEFAULT '',
            postal_code TEXT    NOT NULL DEFAULT '',
            coordinates TEXT    NOT NULL DEFAULT '',
            business_name TEXT  NOT NULL DEFAULT '',
            room_count  INTEGER NOT NULL DEFAULT 0,
            weekend_price REAL  NOT NULL DEFAULT 0.0,
            seasonal_price REAL NOT NULL DEFAULT 0.0,
            taxes_fees  REAL    NOT NULL DEFAULT 0.0,
            min_stay    INTEGER NOT NULL DEFAULT 1,
            check_in_time TEXT  NOT NULL DEFAULT '',
            check_out_time TEXT NOT NULL DEFAULT '',
            cancellation_policy TEXT NOT NULL DEFAULT '',
            refund_rules TEXT NOT NULL DEFAULT '',
            child_policy TEXT  NOT NULL DEFAULT '',
            pet_policy   TEXT  NOT NULL DEFAULT '',
            smoking_policy TEXT NOT NULL DEFAULT '',
            id_proof_requirements TEXT NOT NULL DEFAULT '',
            extra_guest_rules TEXT NOT NULL DEFAULT '',
            custom_policies TEXT NOT NULL DEFAULT '',
            bank_account TEXT  NOT NULL DEFAULT '',
            tax_number   TEXT  NOT NULL DEFAULT '',
            pan_registration TEXT NOT NULL DEFAULT '',
            accepted_payments TEXT NOT NULL DEFAULT '[]',
            upi_id TEXT NOT NULL DEFAULT '',
            upi_qr_code TEXT NOT NULL DEFAULT '',
            commission_due REAL NOT NULL DEFAULT 0.0,
            due_date TEXT NOT NULL DEFAULT '',
            days_overdue INTEGER NOT NULL DEFAULT 0,
            commission_status TEXT NOT NULL DEFAULT 'Paid',
            hotel_status TEXT NOT NULL DEFAULT 'Active',
            created_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    auto hasColumn = [&](const std::string& table, const std::string& column) {
        auto info = query("PRAGMA table_info(" + table + ");");
        for (auto& row : info) {
            for (auto& [key, value] : row) {
                if (key == "name" && value == column) return true;
            }
        }
        return false;
    };

    auto addColumnIfMissing = [&](const std::string& table,
                                  const std::string& column,
                                  const std::string& ddl) {
        if (!hasColumn(table, column)) execute("ALTER TABLE " + table + " ADD COLUMN " + ddl + ";");
    };

    addColumnIfMissing("hotels", "owner_id",      "owner_id INTEGER NOT NULL DEFAULT 0");
    addColumnIfMissing("hotels", "hotel_name",    "hotel_name TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "description",   "description TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "amenities",     "amenities TEXT NOT NULL DEFAULT '[]'");
    addColumnIfMissing("hotels", "image",         "image TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "image_url",     "image_url TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "property_type", "property_type TEXT NOT NULL DEFAULT 'Hotel'");
    addColumnIfMissing("hotels", "price",         "price REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "price_from",    "price_from REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "rating",        "rating REAL NOT NULL DEFAULT 5.0");
    addColumnIfMissing("hotels", "star_rating",   "star_rating REAL NOT NULL DEFAULT 5.0");
    addColumnIfMissing("hotels", "review_score",  "review_score REAL NOT NULL DEFAULT 9.0");
    addColumnIfMissing("hotels", "status",        "status TEXT NOT NULL DEFAULT 'approved'");
    addColumnIfMissing("hotels", "is_active",     "is_active INTEGER NOT NULL DEFAULT 1");
    addColumnIfMissing("hotels", "nearby_places", "nearby_places TEXT NOT NULL DEFAULT '[]'");
    addColumnIfMissing("hotels", "country",       "country TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "state",         "state TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "city",          "city TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "postal_code",   "postal_code TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "coordinates",   "coordinates TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "business_name", "business_name TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "room_count",    "room_count INTEGER NOT NULL DEFAULT 0");
    addColumnIfMissing("hotels", "weekend_price", "weekend_price REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "seasonal_price", "seasonal_price REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "taxes_fees",    "taxes_fees REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "min_stay",      "min_stay INTEGER NOT NULL DEFAULT 1");
    addColumnIfMissing("hotels", "check_in_time", "check_in_time TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "check_out_time", "check_out_time TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "cancellation_policy", "cancellation_policy TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "refund_rules", "refund_rules TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "child_policy",  "child_policy TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "pet_policy",    "pet_policy TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "smoking_policy", "smoking_policy TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "id_proof_requirements", "id_proof_requirements TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "extra_guest_rules", "extra_guest_rules TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "custom_policies", "custom_policies TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "bank_account",  "bank_account TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "tax_number",    "tax_number TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "pan_registration", "pan_registration TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "accepted_payments", "accepted_payments TEXT NOT NULL DEFAULT '[]'");
    addColumnIfMissing("hotels", "upi_id", "upi_id TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "upi_qr_code", "upi_qr_code TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "commission_due", "commission_due REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("hotels", "due_date", "due_date TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotels", "days_overdue", "days_overdue INTEGER NOT NULL DEFAULT 0");
    addColumnIfMissing("hotels", "commission_status", "commission_status TEXT NOT NULL DEFAULT 'Paid'");
    addColumnIfMissing("hotels", "hotel_status", "hotel_status TEXT NOT NULL DEFAULT 'Active'");
    addColumnIfMissing("hotels", "updated_at", "updated_at TEXT NOT NULL DEFAULT ''");

    // ── monthly_commission_ledger ──────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS monthly_commission_ledger (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            ledger_id           TEXT    NOT NULL UNIQUE,
            hotel_id            INTEGER NOT NULL DEFAULT 0,
            month               TEXT    NOT NULL DEFAULT '',
            year                INTEGER NOT NULL DEFAULT 0,
            gross_revenue       REAL    NOT NULL DEFAULT 0.0,
            commission_rate     REAL    NOT NULL DEFAULT 15.0,
            commission_amount   REAL    NOT NULL DEFAULT 0.0,
            pending_amount      REAL    NOT NULL DEFAULT 0.0,
            paid_amount         REAL    NOT NULL DEFAULT 0.0,
            status              TEXT    NOT NULL DEFAULT 'pending',
            due_date            TEXT    NOT NULL DEFAULT '',
            paid_at             TEXT    NOT NULL DEFAULT '',
            transaction_ref     TEXT    NOT NULL DEFAULT '',
            notes               TEXT    NOT NULL DEFAULT '',
            created_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_commission_ledger_hotel ON monthly_commission_ledger(hotel_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_commission_ledger_period ON monthly_commission_ledger(year, month);");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS commission_payment_proofs (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            proof_id            TEXT    NOT NULL UNIQUE,
            ledger_id           TEXT    NOT NULL DEFAULT '',
            hotel_id            INTEGER NOT NULL DEFAULT 0,
            amount              REAL    NOT NULL DEFAULT 0.0,
            upi_id              TEXT    NOT NULL DEFAULT '',
            payment_proof_url   TEXT    NOT NULL DEFAULT '',
            transaction_ref     TEXT    NOT NULL DEFAULT '',
            status              TEXT    NOT NULL DEFAULT 'pending',
            remarks             TEXT    NOT NULL DEFAULT '',
            submitted_by        INTEGER NOT NULL DEFAULT 0,
            reviewed_by         INTEGER NOT NULL DEFAULT 0,
            reviewed_at         TEXT    NOT NULL DEFAULT '',
            created_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at          TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_commission_proofs_ledger ON commission_payment_proofs(ledger_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_commission_proofs_status ON commission_payment_proofs(status);");
    execute("CREATE UNIQUE INDEX IF NOT EXISTS idx_commission_proofs_pending_unique ON commission_payment_proofs(ledger_id) WHERE status='pending';");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS commission_audit_log (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            entity_type     TEXT    NOT NULL DEFAULT 'commission',
            entity_id       TEXT    NOT NULL DEFAULT '',
            hotel_id        INTEGER NOT NULL DEFAULT 0,
            actor_id        INTEGER NOT NULL DEFAULT 0,
            action          TEXT    NOT NULL DEFAULT '',
            event_payload   TEXT    NOT NULL DEFAULT '{}',
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_commission_audit_hotel ON commission_audit_log(hotel_id);");

    // ── room_types ──────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS room_types (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id        INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            name            TEXT    NOT NULL,
            description     TEXT    NOT NULL DEFAULT '',
            room_size       TEXT    NOT NULL DEFAULT '',
            max_capacity    INTEGER NOT NULL DEFAULT 2,
            base_price      REAL    NOT NULL DEFAULT 0.0,
            current_price   REAL    NOT NULL DEFAULT 0.0,
            total_rooms     INTEGER NOT NULL DEFAULT 0,
            amenities       TEXT    NOT NULL DEFAULT '[]',
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_room_types_hotel ON room_types(hotel_id);");
    addColumnIfMissing("room_types", "room_size", "room_size TEXT NOT NULL DEFAULT ''");

    // ── rooms ───────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS rooms (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id        INTEGER NOT NULL REFERENCES hotels(id)      ON DELETE CASCADE,
            room_type_id    INTEGER NOT NULL REFERENCES room_types(id)  ON DELETE CASCADE,
            room_number     TEXT    NOT NULL,
            status          TEXT    NOT NULL DEFAULT 'available',
            floor           INTEGER NOT NULL DEFAULT 1,
            notes           TEXT    NOT NULL DEFAULT '',
            last_updated    TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            UNIQUE(hotel_id, room_number)
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_rooms_hotel   ON rooms(hotel_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_rooms_status  ON rooms(hotel_id, status);");
    execute("CREATE INDEX IF NOT EXISTS idx_rooms_type    ON rooms(room_type_id);");

    // Inventory is a denormalised operations view of rooms. The rooms table remains
    // the source of truth; this table gives the manager portal a stable surface for
    // filtering, pagination, and status analytics.
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS inventory (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id        INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            room_id         INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
            room_type_id    INTEGER NOT NULL REFERENCES room_types(id) ON DELETE CASCADE,
            status          TEXT    NOT NULL DEFAULT 'available',
            price           REAL    NOT NULL DEFAULT 0.0,
            last_synced     TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            UNIQUE(room_id)
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_inventory_hotel_status ON inventory(hotel_id, status);");

    // ── activities ──────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS activities (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT    NOT NULL UNIQUE,
            price       REAL    NOT NULL DEFAULT 0.0,
            description TEXT    NOT NULL DEFAULT ''
        );
    )sql");

    // ── bookings ────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS bookings (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_id      TEXT    NOT NULL UNIQUE,
            hotel_id        INTEGER NOT NULL REFERENCES hotels(id) ON DELETE RESTRICT,
            room_id         INTEGER NOT NULL REFERENCES rooms(id)  ON DELETE RESTRICT,
            room_type_id    INTEGER NOT NULL DEFAULT 0,
            room_number     TEXT    NOT NULL DEFAULT '',
            room_type_name  TEXT    NOT NULL DEFAULT '',

            guest_name      TEXT    NOT NULL,
            guest_email     TEXT    NOT NULL DEFAULT '',
            guest_phone     TEXT    NOT NULL,
            guest_address   TEXT    NOT NULL DEFAULT '',

            check_in_date   TEXT    NOT NULL,
            check_out_date  TEXT    NOT NULL,
            nights          INTEGER NOT NULL DEFAULT 1,

            breakfast       INTEGER NOT NULL DEFAULT 0,
            activities_json TEXT    NOT NULL DEFAULT '[]',

            room_cost       REAL    NOT NULL DEFAULT 0.0,
            activity_cost   REAL    NOT NULL DEFAULT 0.0,
            breakfast_cost  REAL    NOT NULL DEFAULT 0.0,
            subtotal        REAL    NOT NULL DEFAULT 0.0,
            gst_amount      REAL    NOT NULL DEFAULT 0.0,
            total_amount    REAL    NOT NULL DEFAULT 0.0,
            coupon_code     TEXT    NOT NULL DEFAULT '',
            discount_amount REAL    NOT NULL DEFAULT 0.0,
            payment_method  TEXT    NOT NULL DEFAULT 'Pay at hotel',
            payment_status  TEXT    NOT NULL DEFAULT 'pending',
            commission_rate REAL    NOT NULL DEFAULT 15.0,
            commission_amount REAL  NOT NULL DEFAULT 0.0,
            hotel_amount    REAL    NOT NULL DEFAULT 0.0,
            transaction_id  TEXT    NOT NULL DEFAULT '',

            status          TEXT    NOT NULL DEFAULT 'active',
            created_by      INTEGER NOT NULL DEFAULT 1,
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            updated_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_bookings_hotel    ON bookings(hotel_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_bookings_ref      ON bookings(booking_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_bookings_phone    ON bookings(hotel_id, guest_phone);");
    execute("CREATE INDEX IF NOT EXISTS idx_bookings_status   ON bookings(hotel_id, status);");
    execute("CREATE INDEX IF NOT EXISTS idx_bookings_checkin  ON bookings(check_in_date);");
    execute("CREATE INDEX IF NOT EXISTS idx_bookings_checkout ON bookings(check_out_date);");
    addColumnIfMissing("bookings", "coupon_code", "coupon_code TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("bookings", "discount_amount", "discount_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("bookings", "payment_method", "payment_method TEXT NOT NULL DEFAULT 'Pay at hotel'");
    addColumnIfMissing("bookings", "payment_status", "payment_status TEXT NOT NULL DEFAULT 'pending'");
    addColumnIfMissing("bookings", "commission_rate", "commission_rate REAL NOT NULL DEFAULT 15.0");
    addColumnIfMissing("bookings", "commission_amount", "commission_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("bookings", "hotel_amount", "hotel_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("bookings", "transaction_id", "transaction_id TEXT NOT NULL DEFAULT ''");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS payments (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_id      INTEGER NOT NULL REFERENCES bookings(id) ON DELETE CASCADE,
            booking_ref     TEXT    NOT NULL DEFAULT '',
            hotel_id        INTEGER NOT NULL DEFAULT 0,
            user_id         INTEGER NOT NULL DEFAULT 0,
            provider        TEXT    NOT NULL DEFAULT '',
            payment_method  TEXT    NOT NULL DEFAULT '',
            amount          REAL    NOT NULL DEFAULT 0.0,
            status          TEXT    NOT NULL DEFAULT 'pending',
            transaction_ref TEXT    NOT NULL DEFAULT '',
            razorpay_order_id TEXT  NOT NULL DEFAULT '',
            upi_id          TEXT    NOT NULL DEFAULT '',
            merchant_name   TEXT    NOT NULL DEFAULT 'StaySphere',
            commission_rate REAL    NOT NULL DEFAULT 15.0,
            commission_amount REAL  NOT NULL DEFAULT 0.0,
            hotel_amount    REAL    NOT NULL DEFAULT 0.0,
            transaction_date TEXT   NOT NULL DEFAULT (datetime('now','localtime')),
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_payments_booking ON payments(booking_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_payments_status ON payments(status);");
    addColumnIfMissing("payments", "payment_method", "payment_method TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("payments", "upi_id", "upi_id TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("payments", "merchant_name", "merchant_name TEXT NOT NULL DEFAULT 'StaySphere'");
    addColumnIfMissing("payments", "commission_rate", "commission_rate REAL NOT NULL DEFAULT 15.0");
    addColumnIfMissing("payments", "commission_amount", "commission_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("payments", "hotel_amount", "hotel_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("payments", "transaction_date", "transaction_date TEXT NOT NULL DEFAULT (datetime('now','localtime'))");
    addColumnIfMissing("payments", "payment_proof_url", "payment_proof_url TEXT NOT NULL DEFAULT ''");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS app_settings (
            key        TEXT PRIMARY KEY,
            value      TEXT NOT NULL DEFAULT '',
            updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('platform_commission_rate', '15');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('platform_upi_id', 'staysphere@upi');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('platform_merchant_name', 'StaySphere');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('platform_upi_qr_payload', 'upi://pay?pa=staysphere@upi&pn=StaySphere&am=0&cu=INR&tn=StaySphere%20commission');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('commission_collection_enabled', '1');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('commission_active_days', '30');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('commission_grace_until_days', '37');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('commission_suspension_days', '60');");
    execute("INSERT OR IGNORE INTO app_settings (key, value) VALUES ('commission_notification_template', 'Your commission invoice is due. Please pay using the platform UPI QR and upload proof for verification.');");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS hotel_payouts (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            payout_id         TEXT    NOT NULL UNIQUE,
            hotel_id          INTEGER NOT NULL DEFAULT 0,
            booking_id        INTEGER NOT NULL DEFAULT 0,
            booking_ref       TEXT    NOT NULL DEFAULT '',
            gross_amount      REAL    NOT NULL DEFAULT 0.0,
            commission_amount REAL    NOT NULL DEFAULT 0.0,
            payout_amount     REAL    NOT NULL DEFAULT 0.0,
            status            TEXT    NOT NULL DEFAULT 'pending',
            scheduled_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime','+7 days')),
            paid_at           TEXT    NOT NULL DEFAULT '',
            transaction_ref   TEXT    NOT NULL DEFAULT '',
            created_at        TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_payouts_hotel ON hotel_payouts(hotel_id);");
    execute("CREATE INDEX IF NOT EXISTS idx_payouts_status ON hotel_payouts(status);");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS refund_requests (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            refund_id       TEXT    NOT NULL UNIQUE,
            booking_id      INTEGER NOT NULL DEFAULT 0,
            booking_ref     TEXT    NOT NULL DEFAULT '',
            payment_id      INTEGER NOT NULL DEFAULT 0,
            user_id         INTEGER NOT NULL DEFAULT 0,
            hotel_id        INTEGER NOT NULL DEFAULT 0,
            amount          REAL    NOT NULL DEFAULT 0.0,
            reason          TEXT    NOT NULL DEFAULT '',
            status          TEXT    NOT NULL DEFAULT 'requested',
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            processed_at    TEXT    NOT NULL DEFAULT ''
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_refunds_booking ON refund_requests(booking_id);");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS wallet_credits (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id     INTEGER NOT NULL DEFAULT 0,
            booking_id  INTEGER NOT NULL DEFAULT 0,
            amount      REAL    NOT NULL DEFAULT 0.0,
            entry_type  TEXT    NOT NULL DEFAULT 'credit',
            notes       TEXT    NOT NULL DEFAULT '',
            created_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    execute(R"sql(
        CREATE TABLE IF NOT EXISTS booking_history (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_id      INTEGER NOT NULL REFERENCES bookings(id) ON DELETE CASCADE,
            booking_ref     TEXT    NOT NULL DEFAULT '',
            user_id         INTEGER NOT NULL DEFAULT 0,
            hotel_id        INTEGER NOT NULL DEFAULT 0,
            event_type      TEXT    NOT NULL DEFAULT 'created',
            event_payload   TEXT    NOT NULL DEFAULT '{}',
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    // ── cancellations ───────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS cancellations (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            booking_id      INTEGER NOT NULL REFERENCES bookings(id) ON DELETE RESTRICT,
            booking_ref     TEXT    NOT NULL,
            reason          TEXT    NOT NULL DEFAULT '',
            cancelled_by    INTEGER NOT NULL DEFAULT 1,
            refund_amount   REAL    NOT NULL DEFAULT 0.0,
            cancelled_at    TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_cancel_booking ON cancellations(booking_id);");

    // ── sessions ────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS sessions (
            token       TEXT PRIMARY KEY,
            user_id     INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            hotel_id    INTEGER NOT NULL DEFAULT 0,
            role        TEXT    NOT NULL DEFAULT 'receptionist',
            created_at  TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            expires_at  TEXT    NOT NULL
        );
    )sql");

    // Staff profiles hang off users so login remains centralized, while the
    // portal can store hotel-specific staff metadata separately.
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS staff (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id         INTEGER NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
            hotel_id        INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            role_label      TEXT    NOT NULL DEFAULT 'Receptionist',
            shift           TEXT    NOT NULL DEFAULT 'Front desk',
            status          TEXT    NOT NULL DEFAULT 'active',
            avatar_url      TEXT    NOT NULL DEFAULT '',
            created_at      TEXT    NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_staff_hotel ON staff(hotel_id, status);");

    // ── guest_history ────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS guest_history (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id        INTEGER NOT NULL,
            guest_phone     TEXT    NOT NULL,
            guest_name      TEXT    NOT NULL,
            total_visits    INTEGER NOT NULL DEFAULT 1,
            total_spent     REAL    NOT NULL DEFAULT 0.0,
            last_visit      TEXT    NOT NULL DEFAULT (datetime('now','localtime')),
            preferred_room  TEXT    NOT NULL DEFAULT '',
            notes           TEXT    NOT NULL DEFAULT '',
            UNIQUE(hotel_id, guest_phone)
        );
    )sql");

    execute("CREATE INDEX IF NOT EXISTS idx_guest_history_phone ON guest_history(hotel_id, guest_phone);");

    // ── wishlists ────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS wishlists (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
            hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
            UNIQUE(user_id, hotel_id)
        );
    )sql");

    // ── reviews ─────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS reviews (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL REFERENCES users(id),
            hotel_id INTEGER NOT NULL REFERENCES hotels(id),
            rating INTEGER NOT NULL DEFAULT 5,
            comment TEXT NOT NULL DEFAULT '',
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    
    addColumnIfMissing("reviews", "manager_reply", "manager_reply TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("reviews", "replied_at", "replied_at TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("reviews", "images", "images TEXT NOT NULL DEFAULT '[]'");

    // ── hotel_images ────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS hotel_images (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            image_path TEXT NOT NULL DEFAULT '',
            image_url TEXT NOT NULL DEFAULT '',
            is_primary INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    addColumnIfMissing("hotel_images", "image_path", "image_path TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotel_images", "image_url", "image_url TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("hotel_images", "is_primary", "is_primary INTEGER NOT NULL DEFAULT 0");
    execute("CREATE INDEX IF NOT EXISTS idx_hotel_images_hotel ON hotel_images(hotel_id);");

    // ── room_images ─────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS room_images (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_type_id INTEGER NOT NULL REFERENCES room_types(id) ON DELETE CASCADE,
            image_url TEXT NOT NULL,
            is_primary INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");

    // ── hotel_documents ─────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS hotel_documents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
            document_type TEXT NOT NULL,
            document_url TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'pending',
            reviewed_by INTEGER NOT NULL DEFAULT 0,
            reviewed_at TEXT NOT NULL DEFAULT '',
            uploaded_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
        );
    )sql");
    addColumnIfMissing("hotel_documents", "status", "status TEXT NOT NULL DEFAULT 'pending'");
    addColumnIfMissing("hotel_documents", "reviewed_by", "reviewed_by INTEGER NOT NULL DEFAULT 0");
    addColumnIfMissing("hotel_documents", "reviewed_at", "reviewed_at TEXT NOT NULL DEFAULT ''");

    // ── coupons ─────────────────────────────────────────────────────────────
    execute(R"sql(
        CREATE TABLE IF NOT EXISTS coupons (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT NOT NULL UNIQUE,
            description TEXT NOT NULL DEFAULT '',
            discount_type TEXT NOT NULL DEFAULT 'percentage', -- 'percentage' or 'fixed'
            discount_value REAL NOT NULL DEFAULT 0.0,
            min_booking_amount REAL NOT NULL DEFAULT 0.0,
            max_discount REAL,
            valid_to TEXT,
            is_active INTEGER NOT NULL DEFAULT 1,
            usage_limit INTEGER,
            usage_count INTEGER NOT NULL DEFAULT 0
        );
    )sql");
    execute("CREATE INDEX IF NOT EXISTS idx_coupons_code ON coupons(code);");
    addColumnIfMissing("coupons", "description", "description TEXT NOT NULL DEFAULT ''");
    addColumnIfMissing("coupons", "discount_type", "discount_type TEXT NOT NULL DEFAULT 'percentage'");
    addColumnIfMissing("coupons", "discount_value", "discount_value REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("coupons", "min_booking_amount", "min_booking_amount REAL NOT NULL DEFAULT 0.0");
    addColumnIfMissing("coupons", "max_discount", "max_discount REAL");
    addColumnIfMissing("coupons", "valid_to", "valid_to TEXT");
    addColumnIfMissing("coupons", "is_active", "is_active INTEGER NOT NULL DEFAULT 1");
    addColumnIfMissing("coupons", "usage_limit", "usage_limit INTEGER");
    addColumnIfMissing("coupons", "usage_count", "usage_count INTEGER NOT NULL DEFAULT 0");
    addColumnIfMissing("coupons", "applicable_hotels", "applicable_hotels TEXT DEFAULT '[]'");

    execute(R"sql(
        INSERT OR IGNORE INTO inventory (hotel_id, room_id, room_type_id, status, price)
        SELECT r.hotel_id, r.id, r.room_type_id, r.status, rt.current_price
        FROM rooms r
        JOIN room_types rt ON rt.id = r.room_type_id;
    )sql");
    execute(R"sql(
        INSERT OR IGNORE INTO staff (user_id, hotel_id, role_label, shift, status)
        SELECT id, hotel_id,
               CASE WHEN role='hotel_manager' THEN 'Hotel Manager' ELSE 'Receptionist' END,
               CASE WHEN role='hotel_manager' THEN 'Operations' ELSE 'Front desk' END,
               'active'
        FROM users
        WHERE hotel_id > 0 AND role IN ('hotel_manager', 'receptionist');
    )sql");

    commit();
    std::cout << "[DB] Schema created / verified.\n";

    } catch (...) {
        rollback();
        throw;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Seed default data
// ─────────────────────────────────────────────────────────────────────────────
void Database::seedDefaultData() {
    // ── Default super admin (only if no users exist) ──────────────────────
    int userCount = queryInt("SELECT COUNT(*) FROM users;");
    if (userCount == 0) {
        // Password: Admin@1234  (sha256 placeholder — Phase 3 uses real hash)
        std::string hash = Utils::sha256("Admin@1234");
        execute(R"sql(
            INSERT INTO users (username, password_hash, full_name, role, hotel_id)
            VALUES (?, ?, 'Super Administrator', 'super_admin', 0);
        )sql", {"admin", hash});
        std::cout << "[DB] Default admin created — username: admin | password: Admin@1234\n";
    }

    // ── Activities (fixed catalogue) ────────────────────────────────────────
    /*
    if (queryInt("SELECT COUNT(*) FROM users WHERE username='guest';") == 0) {
        execute(R"sql(
            INSERT INTO users (username, password_hash, full_name, email, phone, role, hotel_id, loyalty_points)
            VALUES (?, ?, 'Demo Guest', 'guest@example.com', '+91-9876543210', 'guest', 0, 1250);
        )sql", {"guest", Utils::sha256("Guest@1234")});
        std::cout << "[DB] Demo guest created - username: guest | password: Guest@1234\n";
    }
    */

    int actCount = queryInt("SELECT COUNT(*) FROM activities;");
    if (actCount == 0) {
        const std::vector<std::tuple<std::string,double,std::string>> acts = {
            {"Spa",          1000.0, "Full body spa and relaxation"},
            {"Boat Ride",    1500.0, "Guided boat ride on lake/sea"},
            {"Scuba Diving", 3000.0, "Professional scuba diving with instructor"},
            {"Gym",           500.0, "Gym access for full stay duration"},
            {"DJ Night",     2000.0, "Evening DJ night and entertainment"}
        };
        for (auto& [name, price, desc] : acts) {
            execute("INSERT INTO activities (name, price, description) VALUES (?,?,?);",
                    {name, std::to_string(price), desc});
        }
        std::cout << "[DB] Activities seeded (" << acts.size() << " entries).\n";
    }

    const std::vector<std::string> gallery_standard_room = {
        "https://images.unsplash.com/photo-1590490360182-c33d57733427?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1582719478250-c89cae4dc85b?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1596394516093-501ba68a0ba6?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_deluxe_room = {
        "https://images.unsplash.com/photo-1598035411571-b9f7a4254a43?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1618773928121-c32242e63f39?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1560185893-a55de8537e4b?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_suite = {
        "https://images.unsplash.com/photo-1611892440504-42a792e24d32?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1560448204-e02f11c3d0e2?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1594563703937-fdc640497dcd?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_penthouse = {
        "https://images.unsplash.com/photo-1600210492493-4161435a25f1?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1604514257246-17a41844c3c2?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1522771739844-6a9f6d5f14af?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_family = {
        "https://images.unsplash.com/photo-1578898887233-475465559419?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1567767292278-a4f21aa2d36e?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_villa = {
        "https://images.unsplash.com/photo-1584132967334-10e028bd69f7?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1561501900-3701fa6a0864?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1613977257363-31b5a15e5896?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_cabin = {
        "https://images.unsplash.com/photo-1616047006789-b7af5afb8c20?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1559743349-343d818583a9?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1505873242700-f289a29e1e0f?auto=format&fit=crop&w=1200&q=80"
    };
    const std::vector<std::string> gallery_heritage_suite = {
        "https://images.unsplash.com/photo-1629592259698-35d163a13b43?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1617542234828-43a734d53327?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1631049307264-da0ec9d70304?auto=format&fit=crop&w=1200&q=80"
    };

    // ── Sample hotel (only if none exist) ───────────────────────────────────
    int hotelCount = queryInt("SELECT COUNT(*) FROM hotels;");
    if (hotelCount == 0) {
        int hotelId = executeAndGetId(R"sql(
            INSERT INTO hotels (
                name, location, address, phone, email, website, status, is_active,
                description, amenities, image_url, property_type, price_from, star_rating, review_score, nearby_places
            )
            VALUES (?, ?, ?, ?, ?, ?, 'approved', 1, ?, ?, ?, 'Hotel', ?, ?, ?, ?);
        )sql", {
            "Grand Palace Hotel",
            "Mumbai, Maharashtra",
            "123 Marine Drive, Mumbai - 400001",
            "+91-22-12345678",
            "info@grandpalace.com",
            "https://grandpalace.example.com",
            "A premium seafront city hotel with polished rooms, rooftop dining, spa access, and fast access to South Mumbai landmarks.",
            "[\"WiFi\",\"Pool\",\"Parking\",\"Breakfast\",\"AC\",\"Spa\"]",
            "https://images.unsplash.com/photo-1566073771259-6a8506099945?auto=format&fit=crop&w=1400&q=80",
            "15000",
            "5",
            "9.2",
            "[\"Gateway of India (2.5 km)\", \"Marine Drive (1.2 km)\", \"Colaba Causeway (3 km)\"]"
        });

        const std::vector<std::string> gallery = {
            "https://images.unsplash.com/photo-1566073771259-6a8506099945?auto=format&fit=crop&w=1400&q=80",
            "https://images.unsplash.com/photo-1582719478250-c89cae4dc85b?auto=format&fit=crop&w=1200&q=80",
            "https://images.unsplash.com/photo-1590490360182-c33d57733427?auto=format&fit=crop&w=1200&q=80"
        };
        for (size_t i = 0; i < gallery.size(); ++i) {
            execute("INSERT INTO hotel_images (hotel_id, image_url, is_primary) VALUES (?,?,?);",
                    {std::to_string(hotelId), gallery[i], i == 0 ? "1" : "0"});
        }

        // ── Room types for the sample hotel ──────────────────────────────────
        const std::vector<std::tuple<std::string,std::string,int,double,int>> rtypes = {
            {"Standard",   "Comfortable standard room with all basic amenities", 2,  15000.0, 10},
            {"Deluxe",     "Spacious deluxe room with premium furnishings",       2,  22000.0,  8},
            {"Suite",      "Luxury suite with separate living area and minibar",  4,  35000.0,  4},
            {"Penthouse",  "Top-floor penthouse with panoramic city views",       6, 75000.0,  2},
            {"Family Room","Large family room with bunk beds and play area",      6,  25000.0,  4}
        };

        // Type prefix + floor-number format keeps room numbers professional.
        // e.g. D101, D102, F201, P501
        const std::vector<std::tuple<std::string,int>> typePrefix = {
            {"S", 1}, {"D", 1}, {"S", 3}, {"P", 5}, {"F", 2}
        };
        int typeIdx = 0;

        for (auto& [name, desc, cap, price, total] : rtypes) {
            int rtId = executeAndGetId(R"sql(
                INSERT INTO room_types
                    (hotel_id, name, description, max_capacity, base_price, current_price, total_rooms)
                VALUES (?,?,?,?,?,?,?);
            )sql", {
                std::to_string(hotelId), name, desc,
                std::to_string(cap),
                std::to_string(price), std::to_string(price),
                std::to_string(total)
            });

            auto& [prefix, floor] = typePrefix[typeIdx++];

            for (int r = 1; r <= total; ++r) {
                std::string roomNum = prefix + std::to_string(floor * 100 + r);
                execute(R"sql(
                    INSERT INTO rooms (hotel_id, room_type_id, room_number, floor)
                    VALUES (?,?,?,?);
                )sql", {
                    std::to_string(hotelId),
                    std::to_string(rtId),
                    roomNum,
                    std::to_string(floor)
                });
            }
        }
        std::cout << "[DB] Sample hotel seeded: Grand Palace Hotel (Mumbai).\n";
    }

    auto seedMarketplaceHotel = [&](const std::string& name,
                                    const std::string& location,
                                    const std::string& address,
                                    const std::vector<std::string>& images,
                                    const std::string& property_type,
                                    const std::string& price,
                                    const std::string& rating,
                                    const std::string& score,
                                    const std::string& description,
                                    const std::string& amenities,
                                    const std::string& nearby_places) {
        if (queryInt("SELECT COUNT(*) FROM hotels WHERE name=?;", {name}) > 0) return 0;

        std::vector<std::string> params = {
            name,
            location,
            address,
            "hello@" + name.substr(0, name.find(' ')) + ".example.com",
            "https://" + slugify(name) + ".example.com",
            description,
            amenities,
            images.empty() ? "" : images[0],
            property_type,
            price,
            rating,
            score,
            nearby_places
        };

        int hotelId = executeAndGetId(R"sql(
            INSERT INTO hotels (
                name, location, address, phone, email, website, status, is_active,
                description, amenities, image_url, property_type, price_from, star_rating, review_score, nearby_places
            )
            VALUES (?, ?, ?, '+91-22-55550000', ?, ?, 'approved', 1, ?, ?, ?, ?, ?, ?, ?, ?);
        )sql", params);

        const std::vector<std::tuple<std::string,std::string,int,double,int>> rtypes = {
            {"Classic", "Elegant base room with WiFi, AC, and city views", 2, std::stod(price), 8},
            {"Premium", "Larger room with breakfast, lounge seating, and premium bath amenities", 3, std::stod(price) * 1.45, 6},
            {"Signature Suite", "Suite with living area, work desk, and upgraded views", 4, std::stod(price) * 2.2, 3}
        };
        const std::vector<std::tuple<std::string,int>> prefixes = {{"C", 1}, {"P", 2}, {"S", 3}};
        int idx = 0;
        for (auto& [typeName, desc, cap, roomPrice, total] : rtypes) {
            int rtId = executeAndGetId(R"sql(
                INSERT INTO room_types
                    (hotel_id, name, description, max_capacity, base_price, current_price, total_rooms, amenities)
                VALUES (?,?,?,?,?,?,?,?);
            )sql", {
                std::to_string(hotelId), typeName, desc,
                std::to_string(cap), std::to_string(roomPrice), std::to_string(roomPrice),
                std::to_string(total), amenities
            });

            const std::vector<std::string>* current_gallery = &gallery_standard_room;
            if (name == "Deluxe") current_gallery = &gallery_deluxe_room;
            else if (name == "Suite") current_gallery = &gallery_suite;
            else if (name == "Penthouse") current_gallery = &gallery_penthouse;
            else if (name == "Family Room") current_gallery = &gallery_family;

            for (size_t i = 0; i < current_gallery->size(); ++i) {
                execute("INSERT INTO room_images (room_type_id, image_url, is_primary) VALUES (?,?,?);",
                        {std::to_string(rtId), (*current_gallery)[i], i == 0 ? "1" : "0"});
            }

            auto& [prefix, floor] = prefixes[idx++];
            for (int r = 1; r <= total; ++r) {
                execute("INSERT INTO rooms (hotel_id, room_type_id, room_number, floor) VALUES (?,?,?,?);",
                        {std::to_string(hotelId), std::to_string(rtId),
                         prefix + std::to_string(floor * 100 + r), std::to_string(floor)});
            }
        }
        for (size_t i = 0; i < images.size(); ++i) {
            execute("INSERT INTO hotel_images (hotel_id, image_url, is_primary) VALUES (?,?,?);",
                    {std::to_string(hotelId), images[i], i == 0 ? "1" : "0"});
        }
        return hotelId;
    };

    seedMarketplaceHotel(
        "Aravali Sky Retreat",
        "Udaipur, Rajasthan",
        "Lake Pichola Road, Udaipur - 313001",
        {"https://images.unsplash.com/photo-1542314831-068cd1dbfeeb?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1582719508461-905c673771fd?auto=format&fit=crop&w=1200&q=80",
         "https://images.unsplash.com/photo-1564501049412-61c2a3083791?auto=format&fit=crop&w=1200&q=80"},
        "Luxury Resort",
        "22000",
        "5",
        "9.5",
        "A lakeside retreat with palace-inspired rooms, courtyards, infinity pool views, and private dining decks.",
        "[\"WiFi\",\"Pool\",\"Parking\",\"Breakfast\",\"AC\",\"Lake view\",\"Spa\"]",
        "[\"City Palace (1.5 km)\", \"Lake Pichola (0 km)\", \"Jag Mandir (2 km)\"]");

    seedMarketplaceHotel(
        "Coastal Nest Villas",
        "Goa",
        "Candolim Beach Lane, Goa - 403515",
        {"https://images.unsplash.com/photo-1571896349842-33c89424de2d?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1584132967334-10e028bd69f7?auto=format&fit=crop&w=1200&q=80",
         "https://images.unsplash.com/photo-1561501900-3701fa6a0864?auto=format&fit=crop&w=1200&q=80"},
        "Villa Stay",
        "12000",
        "4.8",
        "9.1",
        "Airbnb-style private villas near the beach with patios, kitchenettes, pet-friendly rooms, and long-stay offers.",
        "[\"WiFi\",\"Parking\",\"Breakfast\",\"AC\",\"Pet friendly\",\"Beach access\"]",
        "[\"Candolim Beach (0.5 km)\", \"Aguada Fort (4 km)\", \"Baga Beach (3 km)\"]");

    seedMarketplaceHotel(
        "City Comfort Inn",
        "Mumbai, Maharashtra",
        "45 Andheri East, Mumbai",
        {"https://images.unsplash.com/photo-1551882547-ff40c63fe5fa?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1618773928121-c32242e63f39?auto=format&fit=crop&w=1200&q=80"},
        "Hotel",
        "2500",
        "3",
        "7.5",
        "A budget-friendly transit hotel offering clean rooms, free breakfast, and easy airport access.",
        "[\"WiFi\",\"AC\",\"Breakfast\"]",
        "[\"Mumbai Airport (3 km)\", \"Juhu Beach (6 km)\"]");

    seedMarketplaceHotel(
        "Backpackers Haven",
        "Goa",
        "Anjuna Beach Road, Goa",
        {"https://images.unsplash.com/photo-1520250497591-112f2f40a3f4?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1596422846543-75c6fc197f07?auto=format&fit=crop&w=1200&q=80"},
        "Hostel",
        "1500",
        "3",
        "8.2",
        "Vibrant hostel and budget hotel with a communal lounge, bar, and walking distance to the beach.",
        "[\"WiFi\",\"Bar\",\"AC\"]",
        "[\"Anjuna Beach (1 km)\", \"Curlies (1.5 km)\"]");

    seedMarketplaceHotel(
        "Himalayan Pines",
        "Manali, Himachal Pradesh",
        "Old Manali Road, Manali - 175131",
        {"https://images.unsplash.com/photo-1605649487212-47bdab064df7?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1616047006789-b7af5afb8c20?auto=format&fit=crop&w=1200&q=80",
         "https://images.unsplash.com/photo-1611892440504-42a792e24d32?auto=format&fit=crop&w=1200&q=80"},
        "Cabin",
        "8500",
        "4.5",
        "8.8",
        "Cozy timber cabins surrounded by snow-capped peaks and pine forests.",
        "[\"WiFi\",\"Parking\",\"Breakfast\",\"Heater\",\"Mountain view\"]",
        "[\"Solang Valley (12 km)\", \"Hidimba Temple (10 km)\", \"Mall Road (2 km)\"]");

    seedMarketplaceHotel(
        "Kerala Backwaters Resort",
        "Alleppey, Kerala",
        "Punnamada, Alleppey - 688006",
        {"https://images.unsplash.com/photo-1593693397690-362cb9666c89?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1582719478250-c89cae4dc85b?auto=format&fit=crop&w=1200&q=80",
         "https://images.unsplash.com/photo-1590490360182-c33d57733427?auto=format&fit=crop&w=1200&q=80"},
        "Resort",
        "18000",
        "4.9",
        "9.4",
        "Traditional houseboat experiences and luxury resort rooms overlooking tranquil backwaters.",
        "[\"WiFi\",\"Pool\",\"Breakfast\",\"AC\",\"Spa\",\"River view\"]",
        "[\"Alleppey Beach (5 km)\", \"Kumarakom Bird Sanctuary (15 km)\", \"Marari Beach (12 km)\"]");

    seedMarketplaceHotel(
        "Royal Heritage Palace",
        "Jaipur, Rajasthan",
        "Amer Road, Jaipur - 302001",
        {"https://images.unsplash.com/photo-1477587458883-47135dfdb56f?auto=format&fit=crop&w=1400&q=80",
         "https://images.unsplash.com/photo-1629592259698-35d163a13b43?auto=format&fit=crop&w=1200&q=80",
         "https://images.unsplash.com/photo-1617542234828-43a734d53327?auto=format&fit=crop&w=1200&q=80"},
        "Luxury Hotel",
        "28000",
        "5",
        "9.7",
        "A breathtaking heritage palace offering royal suites, traditional courtyards, and majestic elephant rides.",
        "[\"WiFi\",\"Pool\",\"Breakfast\",\"AC\",\"Spa\",\"Heritage\"]",
        "[\"Amer Fort (2 km)\", \"Hawa Mahal (4 km)\", \"City Palace (5 km)\"]");

    /*
    int managerHotelId = queryInt("SELECT id FROM hotels WHERE name='Grand Palace Hotel' LIMIT 1;");
    if (queryInt("SELECT COUNT(*) FROM users WHERE username='manager';") == 0) {
        execute(R"sql(
            INSERT INTO users (username, password_hash, full_name, email, phone, role, hotel_id)
            VALUES (?, ?, 'Demo Hotel Manager', 'manager@example.com', '+91-9876500000', 'hotel_manager', ?);
        )sql", {"manager", Utils::sha256("Manager@1234"), std::to_string(managerHotelId)});
        std::cout << "[DB] Demo manager created - username: manager | password: Manager@1234\n";
    }
    */

    // ── Seed Dummy Guest Reviews ─────────────────────────────────────────────
    /*
    if (queryInt("SELECT COUNT(*) FROM reviews;") == 0) {
        auto getGuestId = [&](const std::string& name) {
            std::string username = "guest_" + slugify(name);
            int userId = queryInt("SELECT id FROM users WHERE username=?;", {username});
            if (userId == 0) {
                userId = executeAndGetId(R"sql(
                    INSERT INTO users (username, password_hash, full_name, role)
                    VALUES (?, ?, ?, 'guest');
                )sql", {username, Utils::sha256("Guest@1234"), name});
            }
            return userId;
        };

        auto addRev = [&](const std::string& guestName, const std::string& hName, int r, const std::string& c) {
            int guestId = getGuestId(guestName);
            int hId = queryInt("SELECT id FROM hotels WHERE name=?;", {hName});
            if (hId > 0 && guestId > 0) {
                execute("INSERT INTO reviews (user_id, hotel_id, rating, comment) VALUES (?,?,?,?);",
                        {std::to_string(guestId), std::to_string(hId), std::to_string(r), c});
            }
        };
        addRev("Aarav Sharma", "Grand Palace Hotel", 5, "Absolutely luxurious! The penthouse views are breathtaking.");
        addRev("Priya Patel", "Grand Palace Hotel", 4, "Great service and location, but quite expensive.");
        addRev("Rohan Mehta", "Aravali Sky Retreat", 5, "Felt like royalty. The lakeside dinner was magical.");
        addRev("Saanvi Singh", "Coastal Nest Villas", 5, "Perfect private getaway for our family and pet.");
        addRev("Vikram Reddy", "City Comfort Inn", 4, "Good for a quick layover. Clean and practical.");
        addRev("Anika Gupta", "Backpackers Haven", 5, "Great vibe, amazing people, very affordable!");
        addRev("Aditya Kumar", "Himalayan Pines", 5, "The mountain views from the cabin were surreal. Excellent heater!");
        addRev("Diya Joshi", "Kerala Backwaters Resort", 5, "The houseboat cruise was a once-in-a-lifetime experience.");
        addRev("Kabir Desai", "Royal Heritage Palace", 5, "Truly felt like a Maharaja. Impeccable hospitality.");
        addRev("Ishaan Nair", "Grand Palace Hotel", 4, "Amazing food, but the check-in took a bit too long.");
        addRev("Myra Choudhary", "Coastal Nest Villas", 4, "Loved the private BBQ, though the WiFi was spotty at times.");
    }
    */

    execute("UPDATE hotels SET hotel_name=name WHERE hotel_name='';");
    execute("UPDATE hotels SET image=COALESCE(NULLIF(image,''), NULLIF(image_url,''), '') WHERE image='';");
    execute("UPDATE hotels SET image_url=image WHERE image_url='' AND image!='';");
    execute("UPDATE hotels SET price=price_from WHERE price=0 AND price_from>0;");
    execute("UPDATE hotels SET rating=star_rating WHERE rating=5.0 AND star_rating!=5.0;");
    execute("UPDATE hotel_images SET image_path=image_url WHERE image_path='' AND image_url!='';");
    execute("UPDATE hotel_images SET image_url=image_path WHERE image_url='' AND image_path!='';");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public initialise — called once from main()
// ─────────────────────────────────────────────────────────────────────────────
void Database::initialize() {
    createTables();
    seedDefaultData();

    // Backfill missing payment records for bookings
    try {
        auto bookings = query("SELECT id, booking_id, hotel_id, created_by, total_amount, payment_status, payment_method, commission_rate, commission_amount, hotel_amount FROM bookings;");
        for (auto& row : bookings) {
            auto getVal = [](const DBRow& r, const std::string& key) {
                for (auto& p : r) {
                    if (p.first == key) return p.second;
                }
                return std::string("");
            };
            std::string b_id = getVal(row, "id");
            std::string booking_ref = getVal(row, "booking_id");
            int count = queryInt("SELECT COUNT(*) FROM payments WHERE booking_ref=?;", {booking_ref});
            if (count == 0) {
                std::string hotel_id = getVal(row, "hotel_id");
                std::string user_id = getVal(row, "created_by");
                std::string amount = getVal(row, "total_amount");
                std::string status = getVal(row, "payment_status");
                std::string payment_method = getVal(row, "payment_method");
                std::string commission_rate = getVal(row, "commission_rate");
                std::string commission_amount = getVal(row, "commission_amount");
                std::string hotel_amount = getVal(row, "hotel_amount");

                auto hotelInfoRows = query("SELECT name, upi_id FROM hotels WHERE id=?;", {hotel_id});
                std::string hotelUpiId = "";
                std::string hotelName = "";
                if (!hotelInfoRows.empty()) {
                    hotelUpiId = getVal(hotelInfoRows[0], "upi_id");
                    hotelName = getVal(hotelInfoRows[0], "name");
                }

                std::string dummyProof = "";
                if (payment_method == "Pay Now") {
                    dummyProof = "https://images.unsplash.com/photo-1554224155-8d04cb21cd6c?auto=format&fit=crop&w=600&q=80";
                }

                execute(R"sql(
                    INSERT INTO payments (
                        booking_id, booking_ref, hotel_id, user_id,
                        provider, payment_method, amount, status, transaction_ref,
                        upi_id, merchant_name,
                        commission_rate, commission_amount, hotel_amount, payment_proof_url
                    )
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
                )sql", {
                    b_id, booking_ref, hotel_id, user_id,
                    payment_method, payment_method, amount, status, "PROOF-SEED-" + booking_ref,
                    hotelUpiId, hotelName,
                    commission_rate, commission_amount, hotel_amount, dummyProof
                });
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB] Warning: could not backfill payments: " << e.what() << "\n";
    }

    std::cout << "[DB] Database initialised and ready.\n";
}
