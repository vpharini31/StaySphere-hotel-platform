#include "Managers.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

BackupManager::BackupManager(std::shared_ptr<Database> db,
                             const std::string& db_path,
                             const std::string& backup_dir)
    : db_(db), db_path_(db_path), backup_dir_(backup_dir)
{
    try { fs::create_directories(backup_dir_); } catch (...) {}
}

// ── createBackup ─────────────────────────────────────────────────────────────
std::string BackupManager::createBackup() {
    // Build timestamped filename
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y%m%d_%H%M%S");
    std::string dest = backup_dir_ + "backup_" + ts.str() + ".db";

    // Copy the SQLite file
    try {
        fs::copy_file(db_path_, dest, fs::copy_options::overwrite_existing);
        std::cout << "[Backup] Created: " << dest << "\n";
        return dest;
    } catch (const std::exception& e) {
        throw std::runtime_error("Backup failed: " + std::string(e.what()));
    }
}

// ── restoreBackup ─────────────────────────────────────────────────────────────
bool BackupManager::restoreBackup(const std::string& filename) {
    std::string src = backup_dir_ + filename;
    if (!fs::exists(src)) return false;
    try {
        fs::copy_file(src, db_path_, fs::copy_options::overwrite_existing);
        std::cout << "[Backup] Restored from: " << src << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

// ── listBackups ──────────────────────────────────────────────────────────────
std::vector<BackupEntry> BackupManager::listBackups() {
    std::vector<BackupEntry> list;
    if (!fs::exists(backup_dir_)) return list;
    for (auto& entry : fs::directory_iterator(backup_dir_)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".db") continue;
        BackupEntry be;
        be.filename   = entry.path().filename().string();
        be.size_bytes = static_cast<long>(entry.file_size());
        // Format creation time from filename timestamp (backup_YYYYMMDD_HHMMSS.db)
        std::string name = be.filename;
        if (name.size() >= 22) {
            // backup_20260512_143022.db
            std::string dt = name.substr(7, 15); // 20260512_143022
            be.created_at = dt.substr(0,4) + "-" + dt.substr(4,2) + "-" +
                            dt.substr(6,2) + " " + dt.substr(9,2) + ":" +
                            dt.substr(11,2) + ":" + dt.substr(13,2);
        } else {
            be.created_at = "unknown";
        }
        list.push_back(be);
    }
    // Sort newest first
    std::sort(list.begin(), list.end(), [](const BackupEntry& a, const BackupEntry& b){
        return a.created_at > b.created_at;
    });
    return list;
}

// ── deleteBackup ─────────────────────────────────────────────────────────────
bool BackupManager::deleteBackup(const std::string& filename) {
    // Safety: only allow files in backup dir, no path traversal
    if (filename.find('/') != std::string::npos ||
        filename.find('\\') != std::string::npos) return false;
    std::string path = backup_dir_ + filename;
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

// ── deleteAllBookings ────────────────────────────────────────────────────────
bool BackupManager::deleteAllBookings() {
    try {
        db_->execute("DELETE FROM bookings");
        db_->execute("DELETE FROM cancellations");
        db_->execute("UPDATE rooms SET status='available'");
        std::cout << "[Backup] All bookings cleared and rooms reset to available.\n";
        return true;
    } catch (...) {
        return false;
    }
}
