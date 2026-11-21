#include "AuthManager.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
AuthManager::AuthManager(std::shared_ptr<Database> db) : db_(db) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Time helpers
// ─────────────────────────────────────────────────────────────────────────────
std::string AuthManager::nowISO() const {
    return Utils::nowISO();
}

std::string AuthManager::expiresISO(int minutes) const {
    auto now = std::chrono::system_clock::now();
    auto exp = now + std::chrono::minutes(minutes);
    auto t   = std::chrono::system_clock::to_time_t(exp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool AuthManager::isExpired(const Session& s) const {
    return nowISO() >= s.expires_at;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Password hashing
// ─────────────────────────────────────────────────────────────────────────────
std::string AuthManager::hashPassword(const std::string& plain) const {
    return Utils::sha256(plain);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Token generation
// ─────────────────────────────────────────────────────────────────────────────
std::string AuthManager::generateToken() const {
    return Utils::generateUUID();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Login
// ─────────────────────────────────────────────────────────────────────────────
std::optional<Session> AuthManager::login(const std::string& username,
                                           const std::string& password) {
    // 1. Fetch user row
    auto rows = db_->query(
        "SELECT id, username, password_hash, role, hotel_id, is_locked, failed_attempts "
        "FROM users WHERE username = ?;",
        {username});

    if (rows.empty()) {
        std::cout << "[Auth] Login failed: unknown user '" << username << "'\n";
        return std::nullopt;
    }

    auto& row = rows[0];
    auto getCol = [&](const std::string& key) -> std::string {
        for (auto& [k, v] : row) if (k == key) return v;
        return "";
    };

    int  userId     = std::stoi(getCol("id"));
    bool isLocked   = getCol("is_locked") == "1";
    int  attempts   = std::stoi(getCol("failed_attempts"));
    std::string storedHash = getCol("password_hash");
    std::string role       = getCol("role");
    int  hotelId    = std::stoi(getCol("hotel_id"));

    // 2. Check lockout
    if (isLocked) {
        std::cout << "[Auth] Login blocked: account '" << username << "' is locked.\n";
        return std::nullopt;
    }

    // 3. Verify password
    std::string inputHash = hashPassword(password);
    if (inputHash != storedHash) {
        int newAttempts = attempts + 1;
        bool lockNow = newAttempts >= MAX_ATTEMPTS;

        db_->execute(
            "UPDATE users SET failed_attempts = ?, is_locked = ? WHERE id = ?;",
            {std::to_string(newAttempts),
             lockNow ? "1" : "0",
             std::to_string(userId)});

        std::cout << "[Auth] Wrong password for '" << username
                  << "' (" << newAttempts << "/" << MAX_ATTEMPTS << ")";
        if (lockNow) std::cout << " — ACCOUNT LOCKED";
        std::cout << "\n";
        return std::nullopt;
    }

    // 4. Reset failed attempts + update last_login
    db_->execute(
        "UPDATE users SET failed_attempts = 0, is_locked = 0, last_login = ? WHERE id = ?;",
        {nowISO(), std::to_string(userId)});

    // 5. Build session
    Session session;
    session.token      = generateToken();
    session.user_id    = userId;
    session.role       = roleFromString(role);
    session.hotel_id   = hotelId;
    session.created_at = nowISO();
    session.expires_at = expiresISO(SESSION_TIMEOUT_MIN);
    session.is_valid   = true;

    // Store in DB for persistence across server restarts
    db_->execute("INSERT INTO sessions (token, user_id, hotel_id, role, created_at, expires_at) VALUES (?,?,?,?,?,?);",
                 {session.token, std::to_string(userId), std::to_string(hotelId), role, session.created_at, session.expires_at});

    // 6. Store in memory map
    {
        std::lock_guard<std::mutex> lock(sessions_mtx_);
        cleanExpiredSessions();
        sessions_[session.token] = session;
    }

    std::cout << "[Auth] Login OK: '" << username
              << "' role=" << role << " token=" << session.token.substr(0,8) << "...\n";
    return session;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Logout
// ─────────────────────────────────────────────────────────────────────────────
void AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessions_mtx_);
    sessions_.erase(token);
    db_->execute("DELETE FROM sessions WHERE token=?;", {token});
    std::cout << "[Auth] Logged out token=" << token.substr(0,8) << "...\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Validate token
// ─────────────────────────────────────────────────────────────────────────────
std::optional<Session> AuthManager::validateToken(const std::string& token) {
    {
        std::lock_guard<std::mutex> lock(sessions_mtx_);
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            if (isExpired(it->second)) {
                sessions_.erase(it);
                db_->execute("DELETE FROM sessions WHERE token=?;", {token});
                return std::nullopt;
            }
            it->second.expires_at = expiresISO(SESSION_TIMEOUT_MIN);
            db_->execute("UPDATE sessions SET expires_at=? WHERE token=?;", {it->second.expires_at, token});
            return it->second;
        }
    }
    
    // Fallback to DB if session isn't in memory (e.g., after a C++ server restart)
    auto rows = db_->query("SELECT user_id, role, hotel_id, created_at, expires_at FROM sessions WHERE token=?;", {token});
    if (!rows.empty()) {
        Session s;
        s.token = token;
        s.is_valid = true;
        for (auto& [k, v] : rows[0]) {
            if (k == "user_id") s.user_id = std::stoi(v);
            else if (k == "role") s.role = roleFromString(v);
            else if (k == "hotel_id") s.hotel_id = std::stoi(v);
            else if (k == "created_at") s.created_at = v;
            else if (k == "expires_at") s.expires_at = v;
        }
        if (isExpired(s)) {
            db_->execute("DELETE FROM sessions WHERE token=?;", {token});
            return std::nullopt;
        }
        s.expires_at = expiresISO(SESSION_TIMEOUT_MIN);
        db_->execute("UPDATE sessions SET expires_at=? WHERE token=?;", {s.expires_at, token});
        
        std::lock_guard<std::mutex> lock(sessions_mtx_);
        sessions_[token] = s;
        return s;
    }
    
    return std::nullopt;
}

bool AuthManager::hasRole(const std::string& token, UserRole required) {
    auto s = validateToken(token);
    if (!s) return false;
    // Super admin can do everything
    if (s->role == UserRole::SUPER_ADMIN) return true;
    return s->role == required;
}

// ─────────────────────────────────────────────────────────────────────────────
//  User management
// ─────────────────────────────────────────────────────────────────────────────
bool AuthManager::createUser(const User& user, const std::string& plain_password) {
    try {
        std::string hash = hashPassword(plain_password);
        std::string role = roleToString(user.role);
        db_->execute(
            "INSERT INTO users (username, password_hash, full_name, email, phone, role, hotel_id) "
            "VALUES (?,?,?,?,?,?,?);",
            {user.username, hash, user.full_name, user.email, user.phone, role,
             std::to_string(user.hotel_id)});
        return true;
    } catch (const DatabaseException& e) {
        std::cerr << "[Auth] createUser failed: " << e.what() << "\n";
        return false;
    }
}

bool AuthManager::changePassword(int user_id,
                                  const std::string& old_pw,
                                  const std::string& new_pw) {
    auto rows = db_->query(
        "SELECT password_hash FROM users WHERE id = ?;",
        {std::to_string(user_id)});
    if (rows.empty()) return false;

    std::string stored = rows[0][0].second;
    if (hashPassword(old_pw) != stored) return false;

    db_->execute(
        "UPDATE users SET password_hash = ? WHERE id = ?;",
        {hashPassword(new_pw), std::to_string(user_id)});
    return true;
}

bool AuthManager::lockUser(const std::string& username) {
    db_->execute("UPDATE users SET is_locked = 1 WHERE username = ?;", {username});
    return true;
}

bool AuthManager::unlockUser(const std::string& username) {
    db_->execute(
        "UPDATE users SET is_locked = 0, failed_attempts = 0 WHERE username = ?;",
        {username});
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Cleanup expired sessions
// ─────────────────────────────────────────────────────────────────────────────
void AuthManager::cleanExpiredSessions() {
    std::string now = nowISO();
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (it->second.expires_at <= now)
            it = sessions_.erase(it);
        else
            ++it;
    }
}
