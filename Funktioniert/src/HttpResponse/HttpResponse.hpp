#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>

class HttpResponse {
public:
    static std::string ErrorResponse(const std::string& errorCode, const std::string& errorPagePath);
	static std::string generate(const std::string& Code, const std::string& contentType, const std::string& content);
private:
    static std::string readFile(const std::string& filePath);
};

#endif // HTTPRESPONSE_HPP