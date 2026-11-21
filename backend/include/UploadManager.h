#pragma once

#include <cstddef>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct UploadResult {
    std::string original_name;
    std::string filename;
    std::string public_url;
    std::string content_type;
    std::size_t size = 0;

    json toJson() const {
        return {
            {"original_name", original_name},
            {"filename", filename},
            {"url", public_url},
            {"image_path", public_url},
            {"content_type", content_type},
            {"size", size}
        };
    }
};

class UploadManager {
public:
    explicit UploadManager(std::string upload_dir = "backend/uploads",
                           std::size_t max_bytes = 5 * 1024 * 1024);

    UploadResult saveHotelImage(const std::string& original_name,
                                const std::string& content_type,
                                const std::string& body) const;

    UploadResult saveDocument(const std::string& original_name,
                              const std::string& content_type,
                              const std::string& body) const;

    bool isAllowedImage(const std::string& original_name,
                        const std::string& content_type,
                        const std::string& body) const;

    bool isAllowedDocument(const std::string& original_name,
                           const std::string& content_type,
                           const std::string& body) const;

    std::size_t maxBytes() const { return max_bytes_; }
    const std::string& uploadDir() const { return upload_dir_; }

private:
    std::string upload_dir_;
    std::size_t max_bytes_;

    std::string extensionFor(const std::string& original_name,
                             const std::string& body) const;
    std::string documentExtensionFor(const std::string& original_name,
                                     const std::string& body) const;
};
