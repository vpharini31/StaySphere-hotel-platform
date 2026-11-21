#include "UploadManager.h"
#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool hasJpegSignature(const std::string& body) {
    return body.size() >= 3 &&
           static_cast<unsigned char>(body[0]) == 0xff &&
           static_cast<unsigned char>(body[1]) == 0xd8 &&
           static_cast<unsigned char>(body[2]) == 0xff;
}

bool hasPngSignature(const std::string& body) {
    return body.size() >= 8 &&
           static_cast<unsigned char>(body[0]) == 0x89 &&
           body[1] == 'P' && body[2] == 'N' && body[3] == 'G' &&
           static_cast<unsigned char>(body[4]) == 0x0d &&
           static_cast<unsigned char>(body[5]) == 0x0a &&
           static_cast<unsigned char>(body[6]) == 0x1a &&
           static_cast<unsigned char>(body[7]) == 0x0a;
}

bool hasPdfSignature(const std::string& body) {
    return body.size() >= 5 && body.rfind("%PDF-", 0) == 0;
}
}

UploadManager::UploadManager(std::string upload_dir, std::size_t max_bytes)
    : upload_dir_(std::move(upload_dir)), max_bytes_(max_bytes) {}

std::string UploadManager::extensionFor(const std::string& original_name,
                                        const std::string& body) const {
    std::string ext;
    auto dot = original_name.find_last_of('.');
    if (dot != std::string::npos) ext = lowerCopy(original_name.substr(dot));

    if (ext == ".jpeg") ext = ".jpg";
    if (ext == ".jpg" || ext == ".png") return ext;
    if (hasJpegSignature(body)) return ".jpg";
    if (hasPngSignature(body)) return ".png";
    return "";
}

std::string UploadManager::documentExtensionFor(const std::string& original_name,
                                                const std::string& body) const {
    std::string ext;
    auto dot = original_name.find_last_of('.');
    if (dot != std::string::npos) ext = lowerCopy(original_name.substr(dot));
    if (ext == ".jpeg") ext = ".jpg";
    if (ext == ".jpg" || ext == ".png" || ext == ".pdf") return ext;
    if (hasJpegSignature(body)) return ".jpg";
    if (hasPngSignature(body)) return ".png";
    if (hasPdfSignature(body)) return ".pdf";
    return "";
}

bool UploadManager::isAllowedImage(const std::string& original_name,
                                   const std::string& content_type,
                                   const std::string& body) const {
    if (body.empty() || body.size() > max_bytes_) return false;

    const std::string ext = extensionFor(original_name, body);
    if (ext != ".jpg" && ext != ".png") return false;

    const std::string type = lowerCopy(content_type);
    const bool type_ok = type.empty() ||
                         type.find("image/jpeg") != std::string::npos ||
                         type.find("image/jpg") != std::string::npos ||
                         type.find("image/png") != std::string::npos;
    if (!type_ok) return false;

    return (ext == ".jpg" && hasJpegSignature(body)) ||
           (ext == ".png" && hasPngSignature(body));
}

bool UploadManager::isAllowedDocument(const std::string& original_name,
                                      const std::string& content_type,
                                      const std::string& body) const {
    if (body.empty() || body.size() > max_bytes_) return false;

    const std::string ext = documentExtensionFor(original_name, body);
    if (ext == ".jpg" || ext == ".png") return isAllowedImage(original_name, content_type, body);
    if (ext != ".pdf" || !hasPdfSignature(body)) return false;

    const std::string type = lowerCopy(content_type);
    return type.empty() ||
           type.find("application/pdf") != std::string::npos ||
           type.find("application/octet-stream") != std::string::npos;
}

UploadResult UploadManager::saveHotelImage(const std::string& original_name,
                                           const std::string& content_type,
                                           const std::string& body) const {
    if (body.empty()) throw std::runtime_error("Uploaded image is empty");
    if (body.size() > max_bytes_) throw std::runtime_error("Image exceeds 5 MB limit");
    if (!isAllowedImage(original_name, content_type, body)) {
        throw std::runtime_error("Only JPG, JPEG, and PNG images are allowed");
    }

    std::filesystem::create_directories(upload_dir_);

    const std::string ext = extensionFor(original_name, body);
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    const std::string filename = "hotel_" + std::to_string(now) + "_" +
                                 Utils::generateUUID().substr(0, 8) + ext;
    const std::filesystem::path target = std::filesystem::path(upload_dir_) / filename;

    std::ofstream out(target, std::ios::binary);
    if (!out) throw std::runtime_error("Unable to save uploaded image");
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    if (!out) throw std::runtime_error("Failed while writing uploaded image");

    UploadResult result;
    result.original_name = original_name;
    result.filename = filename;
    result.public_url = "/uploads/" + filename;
    result.content_type = content_type;
    result.size = body.size();
    return result;
}

UploadResult UploadManager::saveDocument(const std::string& original_name,
                                         const std::string& content_type,
                                         const std::string& body) const {
    if (body.empty()) throw std::runtime_error("Uploaded document is empty");
    if (body.size() > max_bytes_) throw std::runtime_error("Document exceeds 5 MB limit");
    if (!isAllowedDocument(original_name, content_type, body)) {
        throw std::runtime_error("Only PDF, JPG, JPEG, and PNG documents are allowed");
    }

    std::filesystem::create_directories(upload_dir_);

    const std::string ext = documentExtensionFor(original_name, body);
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    const std::string filename = "document_" + std::to_string(now) + "_" +
                                 Utils::generateUUID().substr(0, 8) + ext;
    const std::filesystem::path target = std::filesystem::path(upload_dir_) / filename;

    std::ofstream out(target, std::ios::binary);
    if (!out) throw std::runtime_error("Unable to save uploaded document");
    out.write(body.data(), static_cast<std::streamsize>(body.size()));
    if (!out) throw std::runtime_error("Failed while writing uploaded document");

    UploadResult result;
    result.original_name = original_name;
    result.filename = filename;
    result.public_url = "/uploads/" + filename;
    result.content_type = content_type;
    result.size = body.size();
    return result;
}
