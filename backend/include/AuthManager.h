#pragma once
// ═══════════════════════════════════════════════════════════════════════════
//  AuthManager.h
// ═══════════════════════════════════════════════════════════════════════════
#include "Database.h"
#include "Models.h"
#include <unordered_map>
#include <mutex>
#include <chrono>

class AuthManager {
public:
    explicit AuthManager(std::shared_ptr<Database> db);

    // ── Login / Logout ────────────────────────────────────────────────────
    std::optional<Session>  login(const std::string& username,
                                  const std::string& password);
    void                    logout(const std::string& token);

    // ── Session validation ────────────────────────────────────────────────
    std::optional<Session>  validateToken(const std::string& token);
    bool                    hasRole(const std::string& token, UserRole required);

    // ── User management ───────────────────────────────────────────────────
    bool createUser(const User& user, const std::string& plain_password);
    bool changePassword(int user_id,
                        const std::string& old_pw,
                        const std::string& new_pw);
    bool lockUser(const std::string& username);
    bool unlockUser(const std::string& username);

    // ── Helpers ───────────────────────────────────────────────────────────
    std::string hashPassword(const std::string& plain) const;
    std::string generateToken()                         const;

    static constexpr int    MAX_ATTEMPTS       = 3;
    static constexpr int    SESSION_TIMEOUT_MIN = 60; // minutes

private:
    std::shared_ptr<Database>           db_;
    std::unordered_map<std::string, Session> sessions_;  // token → session
    std::mutex                          sessions_mtx_;

    void        cleanExpiredSessions();
    bool        isExpired(const Session& s) const;
    std::string nowISO() const;
    std::string expiresISO(int minutes) const;
};
