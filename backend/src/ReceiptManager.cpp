#include "Managers.h"
#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace {
std::string pdfEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c == '(' || c == ')' || c == '\\') out.push_back('\\');
        if (c == '\r' || c == '\n') out.push_back(' ');
        else out.push_back(c);
    }
    return out;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}
}

ReceiptManager::ReceiptManager(std::shared_ptr<Database> db,
                               const std::string& receipts_dir)
    : db_(db), receipts_dir_(receipts_dir) {
    // Ensure the receipts directory exists
    try {
        fs::create_directories(receipts_dir_);
    } catch (...) {}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Format a Bill as a printable plain-text receipt string
// ─────────────────────────────────────────────────────────────────────────────
std::string ReceiptManager::formatBillAsText(const Bill& bill) {
    const int W = 60;
    std::ostringstream ss;

    // Header
    ss << Utils::hLine(W, '=') << "\n";
    ss << Utils::centerText(bill.hotel.name, W) << "\n";
    if (!bill.hotel.address.empty())
        ss << Utils::centerText(bill.hotel.address, W) << "\n";
    if (!bill.hotel.phone.empty())
        ss << Utils::centerText("Tel: " + bill.hotel.phone, W) << "\n";
    if (!bill.hotel.email.empty())
        ss << Utils::centerText(bill.hotel.email, W) << "\n";
    ss << Utils::hLine(W, '=') << "\n";
    ss << Utils::centerText("GUEST RECEIPT", W) << "\n";
    ss << Utils::hLine(W, '=') << "\n";

    // Booking details
    ss << Utils::twoColumn("Booking ID:", bill.booking.booking_id, W) << "\n";
    ss << Utils::twoColumn("Guest Name:", bill.booking.guest_name, W) << "\n";
    if (!bill.booking.guest_phone.empty())
        ss << Utils::twoColumn("Phone:", bill.booking.guest_phone, W) << "\n";
    if (!bill.booking.guest_email.empty())
        ss << Utils::twoColumn("Email:", bill.booking.guest_email, W) << "\n";
    if (!bill.booking.guest_address.empty())
        ss << Utils::twoColumn("Address:", bill.booking.guest_address, W) << "\n";
    ss << Utils::hLine(W) << "\n";

    // Room info
    ss << Utils::twoColumn("Room Number:", bill.booking.room_number, W) << "\n";
    ss << Utils::twoColumn("Room Type:", bill.booking.room_type_name, W) << "\n";
    ss << Utils::twoColumn("Check-in:",
        Utils::formatDate(bill.booking.check_in_date), W) << "\n";
    ss << Utils::twoColumn("Check-out:",
        Utils::formatDate(bill.booking.check_out_date), W) << "\n";
    ss << Utils::twoColumn("Nights:",
        std::to_string(bill.booking.nights), W) << "\n";
    ss << Utils::hLine(W) << "\n";

    // Line items
    ss << Utils::centerText("CHARGES", W) << "\n";
    ss << Utils::hLine(W) << "\n";
    for (auto& item : bill.items) {
        ss << Utils::twoColumn(item.description,
                               Utils::formatINR(item.amount), W) << "\n";
    }
    ss << Utils::hLine(W) << "\n";

    // Totals
    ss << Utils::twoColumn("Subtotal:", Utils::formatINR(bill.subtotal), W) << "\n";

    // GST rate as percentage string
    std::ostringstream gst_label;
    gst_label << "GST (" << std::fixed << std::setprecision(0)
              << (bill.gst_rate * 100) << "%):";
    ss << Utils::twoColumn(gst_label.str(),
                           Utils::formatINR(bill.gst_amount), W) << "\n";
    ss << Utils::hLine(W, '=') << "\n";
    ss << Utils::twoColumn("TOTAL AMOUNT:", Utils::formatINR(bill.total), W) << "\n";
    ss << Utils::hLine(W, '=') << "\n";

    // Footer
    ss << Utils::twoColumn("Generated:", bill.generated_at, W) << "\n";
    ss << "\n";
    ss << Utils::centerText("Thank you for staying with us!", W) << "\n";
    ss << Utils::centerText("We hope to see you again.", W) << "\n";
    ss << Utils::hLine(W, '=') << "\n";

    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generate a receipt file and return the file path
// ─────────────────────────────────────────────────────────────────────────────
std::string ReceiptManager::generateReceipt(const Bill& bill) {
    std::string text = formatBillAsText(bill);

    // Filename: RECEIPT-BK-2026-001.txt
    std::string filename = receipts_dir_ + "RECEIPT-" + bill.booking.booking_id + ".txt";

    std::ofstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot create receipt file: " + filename);

    file << text;
    file.close();

    std::cout << "[Receipt] Generated: " << filename << "\n";
    return filename;
}

std::string ReceiptManager::formatBillAsPdf(const Bill& bill) {
    auto lines = splitLines(formatBillAsText(bill));
    if (lines.empty()) lines.push_back("HotelMS Invoice");

    std::ostringstream stream;
    stream << "BT\n"
           << "/F1 10 Tf\n"
           << "14 TL\n"
           << "1 0 0 1 52 790 Tm\n";
    for (size_t i = 0; i < lines.size() && i < 52; ++i) {
        stream << "(" << pdfEscape(lines[i]) << ") Tj\n";
        if (i + 1 < lines.size()) stream << "T*\n";
    }
    stream << "ET\n";
    const std::string contents = stream.str();

    std::vector<std::string> objects;
    objects.push_back("1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
    objects.push_back("2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
    objects.push_back("3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>\nendobj\n");
    objects.push_back("4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Courier >>\nendobj\n");
    objects.push_back("5 0 obj\n<< /Length " + std::to_string(contents.size()) + " >>\nstream\n" +
                      contents + "endstream\nendobj\n");

    std::ostringstream pdf;
    pdf << "%PDF-1.4\n";
    std::vector<long> offsets;
    for (const auto& object : objects) {
        offsets.push_back(static_cast<long>(pdf.tellp()));
        pdf << object;
    }
    const long xref = static_cast<long>(pdf.tellp());
    pdf << "xref\n0 " << (objects.size() + 1) << "\n"
        << "0000000000 65535 f \n";
    for (long offset : offsets) {
        pdf << std::setw(10) << std::setfill('0') << offset << " 00000 n \n";
    }
    pdf << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\n"
        << "startxref\n" << xref << "\n%%EOF\n";
    return pdf.str();
}
