#include "Server.hpp"

std::string Server::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << std::endl;  // Debug-Ausgabe
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    if (!file) {
        std::cerr << "Failed to read file: " << path << std::endl;  // Debug-Ausgabe
        throw std::runtime_error("Cannot read file: " + path);
    }
    return buffer.str();
}

std::string Server::extractFilename(const std::string& contentDisposition) {
    size_t fnPos = contentDisposition.find("filename=\"");
    if (fnPos == std::string::npos) return "";
    
    fnPos += 10;
    size_t fnEnd = contentDisposition.find("\"", fnPos);
    if (fnEnd == std::string::npos) return "";
    
    return contentDisposition.substr(fnPos, fnEnd - fnPos);
}

std::string Server::getBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) {
        throw std::runtime_error("No boundary found in content type");
    }
    return contentType.substr(pos + 9);
}

std::string Server::getContentType(const std::string& path) {
    std::string extension = path.substr(path.find_last_of('.') + 1);
    
    if (extension == "html") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "json") return "application/json";
    if (extension == "pdf") return "application/pdf";
    if (extension == "zip") return "application/zip";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "webp") return "image/webp";
    if (extension == "mp3") return "audio/mpeg";
    if (extension == "wav") return "audio/wav";
    if (extension == "mp4") return "video/mp4";
    
    return "application/octet-stream"; // Default for unknown types
}

void Server::saveUploadedFile(const std::string& uploadDir, const std::string& filename, const std::string& content) {
    if (filename.empty()) {
        throw std::runtime_error("No filename provided");
    }

    std::filesystem::create_directories(uploadDir);

    std::string safeFilename = std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) 
                              + "_" + filename;

    std::string fullPath = uploadDir + "/" + safeFilename;
    std::ofstream file(fullPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + fullPath);
    }

    file.write(content.data(), content.size());
    if (!file) {
        throw std::runtime_error("Failed to write to file: " + fullPath);
    }
}