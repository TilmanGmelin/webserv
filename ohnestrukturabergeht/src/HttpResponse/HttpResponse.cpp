#include "HttpResponse.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string HttpResponse::ErrorResponse(const std::string& errorCode, const std::string& errorPagePath) {
   try {
		std::string errorContent = readFile(errorPagePath);
		return "HTTP/1.1" + errorCode + "\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: " + std::to_string(errorContent.length()) + "\r\n"
				"\r\n" + errorContent;
	} catch (const std::exception& e) {
		std::string defaultError = "<html><body><h1>"+ errorCode + "</h1></body></html>";
		return "HTTP/1.1" + errorCode + "\r\n"
				"Content-Type: text/html\r\n"
				"Content-Length: " + std::to_string(defaultError.length()) + "\r\n"
				"\r\n" + defaultError;
    }
}

std::string HttpResponse::readFile(const std::string& path) {
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

std::string HttpResponse::generate(const std::string& Code, const std::string& contentType, const std::string& content) {
    return "HTTP/1.1 " + Code + "\r\n"
           "Content-Type: " + contentType + "\r\n"
           "Content-Length: " + std::to_string(content.length()) + "\r\n"
           "\r\n" + content;
}

// "Access-Control-Allow-Origin: *\r\n"
// --> config setting wurde be cool sein