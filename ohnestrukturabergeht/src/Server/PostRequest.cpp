#include "Server.hpp"

std::string Server::handlePOSTRequest(const std::string& path, const std::string& contentType, const std::string& body) {

    const Route* matchedRoute = nullptr;
    
    for (const auto& route : _config.routes) {
        if (path.substr(0, route.path.length()) == route.path) {
            matchedRoute = &route;
            // std::cout << "DEBUG: Matched route: " << matchedRoute->uploadDir << std::endl;
            break;
        }
    }

    if (!matchedRoute) {
		return HttpResponse::ErrorResponse("404 Not Found", _config.errorPages[404]);
    }

    if (matchedRoute->allowedMethods.find("POST") == matchedRoute->allowedMethods.end()) {
		return HttpResponse::ErrorResponse("405 Method Not Allowed", _config.errorPages[405]);
    }

    std::string uploadPath = matchedRoute->uploadDir;
    // std::cout << "Using upload path: " << uploadPath << std::endl;

    if (uploadPath.empty()) {
        std::cerr << "No upload path configured for this route" << std::endl;
		return HttpResponse::generate("500 Internal Server Error", "text/plain", "No upload path configured");
    }

    try {
        std::filesystem::create_directories(uploadPath);

        if (contentType.find("multipart/form-data") != std::string::npos) {
            std::string boundary = getBoundary(contentType);
            auto parts = parseMultipartFormData(body, boundary);
            
            int filesUploaded = 0;
            for (const auto& part : parts) {
                auto it = part.headers.find("Content-Disposition");
                if (it != part.headers.end()) {
                    std::string filename = extractFilename(it->second);
					// std::cout << "Extracted filename: " << filename << std::endl;

					// std::cout << "1 Saving file: " << filename << std::endl;
                    if (!filename.empty()) {
						// std::cout << "1 Saving file: " << filename << std::endl;
                        saveUploadedFile(uploadPath, filename, part.body);
                        filesUploaded++;
                    }
                }
				else {
					// std::cout << "+No filename found" << std::endl;
				}
            }
            
            std::string message = "Successfully uploaded " + std::to_string(filesUploaded) + " file(s)";
			return HttpResponse::generate("201 Created", "text/plain", message);
        } else {
			// Andere Post-Requests
			/// --------------------
            std::string filename = "post_" + 
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt";
            saveUploadedFile(uploadPath, filename, body);
            
			return HttpResponse::generate("201 Created", "text/plain", "Created");
        }
    } catch (const std::exception& e) {
        std::string error = "Upload failed: " + std::string(e.what());
		return HttpResponse::generate("500 Internal Server Error", "text/plain", error);
    }
}