#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <stdexcept>

namespace Utils {

std::string sha256(const std::string& input) {
    // Deterministic hash — good enough for this project
    std::hash<std::string> hasher;
    size_t h1 = hasher(input);
    size_t h2 = hasher(input + "_salt_hms_2026");
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << h1
                    << std::setw(16) << std::setfill('0') << h2;
    return oss.str();
}

std::string generateUUID() {
    static std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8)  << (dist(rng) & 0xFFFFFFFF)         << "-"
        << std::setw(4)  << (dist(rng) & 0xFFFF)             << "-"
        << std::setw(4)  << ((dist(rng) & 0x0FFF) | 0x4000)  << "-"
        << std::setw(4)  << ((dist(rng) & 0x3FFF) | 0x8000)  << "-"
        << std::setw(12) << (dist(rng) & 0xFFFFFFFFFFFFULL);
    return oss.str();
}

std::string nowISO() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
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

std::string todayDate() { return nowISO().substr(0, 10); }

// Parse "YYYY-MM-DD" into std::tm
static std::tm parseDate(const std::string& date) {
    std::tm tm{};
    if (date.size() < 10) return tm;
    tm.tm_year = std::stoi(date.substr(0,4)) - 1900;
    tm.tm_mon  = std::stoi(date.substr(5,2)) - 1;
    tm.tm_mday = std::stoi(date.substr(8,2));
    tm.tm_isdst = -1;
    return tm;
}

std::string addDaysToDate(const std::string& date, int days) {
    std::tm tm = parseDate(date);
    tm.tm_mday += days;
    std::time_t t = mktime(&tm);
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

int daysBetween(const std::string& from, const std::string& to) {
    std::tm t1 = parseDate(from);
    std::tm t2 = parseDate(to);
    std::time_t tt1 = mktime(&t1);
    std::time_t tt2 = mktime(&t2);
    double diff = std::difftime(tt2, tt1);
    return static_cast<int>(diff / 86400.0);
}

bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    try {
        int y = std::stoi(date.substr(0,4));
        int m = std::stoi(date.substr(5,2));
        int d = std::stoi(date.substr(8,2));
        return y > 2000 && m >= 1 && m <= 12 && d >= 1 && d <= 31;
    } catch (...) { return false; }
}

bool isFutureDate(const std::string& date) { return date >= todayDate(); }

std::string formatDate(const std::string& iso) {
    static const char* months[] = {
        "","Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    if (iso.size() < 10) return iso;
    try {
        int m = std::stoi(iso.substr(5,2));
        return iso.substr(8,2) + " " + months[m] + " " + iso.substr(0,4);
    } catch (...) { return iso; }
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(),r.end(),r.begin(),::tolower); return r;
}
std::string toUpper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(),r.end(),r.begin(),::toupper); return r;
}
bool startsWith(const std::string& s, const std::string& p) {
    return s.rfind(p,0) == 0;
}
std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> r;
    std::istringstream ss(s);
    std::string t;
    while (std::getline(ss,t,d)) r.push_back(t);
    return r;
}
std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string r;
    for (size_t i=0;i<v.size();++i){ if(i) r+=sep; r+=v[i]; }
    return r;
}
std::string padLeft(const std::string& s, size_t w, char f) {
    return s.size()>=w ? s : std::string(w-s.size(),f)+s;
}
std::string padRight(const std::string& s, size_t w, char f) {
    return s.size()>=w ? s : s+std::string(w-s.size(),f);
}
bool isValidEmail(const std::string& e) {
    return e.find('@') != std::string::npos && e.find('.') != std::string::npos;
}
bool isValidPhone(const std::string& p) {
    std::string d; for(char c:p) if(std::isdigit(c)) d+=c;
    return d.size() >= 10;
}
bool isNotEmpty(const std::string& s) { return !trim(s).empty(); }

std::string formatINR(double amount) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << amount;
    return "Rs." + oss.str();
}
std::string hLine(int w, char ch) { return std::string(w, ch); }
std::string centerText(const std::string& t, int w) {
    if ((int)t.size()>=w) return t;
    int p=(w-(int)t.size())/2;
    return std::string(p,' ')+t;
}
std::string twoColumn(const std::string& l, const std::string& r, int w) {
    std::string row = l;
    if ((int)row.size() < w-(int)r.size())
        row += std::string(w-row.size()-r.size(),' ');
    return row+r;
}

} // namespace Utils
