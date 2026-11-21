#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>

namespace Utils {

// ── Hashing (simple SHA-256 via platform libs or bundled impl) ───────────────
std::string sha256(const std::string& input);

// ── UUID / token generation ───────────────────────────────────────────────────
std::string generateUUID();

// ── Date helpers ──────────────────────────────────────────────────────────────
std::string nowISO();                              // "2026-04-23 14:30:00"
std::string todayDate();                           // "2026-04-23"
std::string addDaysToDate(const std::string& date, int days);
int         daysBetween(const std::string& from, const std::string& to);
bool        isValidDate(const std::string& date); // "YYYY-MM-DD"
bool        isFutureDate(const std::string& date);
std::string formatDate(const std::string& iso_date); // "23 Apr 2026"

// ── String helpers ────────────────────────────────────────────────────────────
std::string  trim(const std::string& s);
std::string  toLower(const std::string& s);
std::string  toUpper(const std::string& s);
bool         startsWith(const std::string& s, const std::string& prefix);
std::vector<std::string> split(const std::string& s, char delim);
std::string  join(const std::vector<std::string>& v, const std::string& sep);
std::string  padLeft(const std::string& s, size_t width, char fill = ' ');
std::string  padRight(const std::string& s, size_t width, char fill = ' ');

// ── Validation ────────────────────────────────────────────────────────────────
bool isValidEmail(const std::string& email);
bool isValidPhone(const std::string& phone);   // 10-digit Indian mobile
bool isNotEmpty(const std::string& s);

// ── Currency formatting ───────────────────────────────────────────────────────
std::string formatINR(double amount);  // "₹1,500.00"

// ── Receipt line helpers ──────────────────────────────────────────────────────
std::string hLine(int width = 60, char ch = '-');
std::string centerText(const std::string& text, int width = 60);
std::string twoColumn(const std::string& left,
                       const std::string& right,
                       int width = 60);

} // namespace Utils
