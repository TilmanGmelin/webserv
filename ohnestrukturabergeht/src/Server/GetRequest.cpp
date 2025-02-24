#include "Server.hpp"

std::string Server::handleGETRequest(const std::string& path) {
    
    // Finde die passende Route
    const Route* matchedRoute = nullptr;
    std::string filePath;

    // Sortiere die Routen nach der Länge ihres Pfads in absteigender Reihenfolge
    std::vector<Route> sortedRoutes = _config.routes;
    std::sort(sortedRoutes.begin(), sortedRoutes.end(), [](const Route& a, const Route& b) {
        return a.path.length() > b.path.length();
    });
    
    for (const auto& route : sortedRoutes) {
        if (path.substr(0, route.path.length()) == route.path) {
            matchedRoute = &route;
			
            // Relativen Pfad zum Root-Verzeichnis der Route hinzufügen
            filePath = route.root + "/" + path.substr(route.path.length());
			 if (filePath.back() == '/' && !matchedRoute->defaultFile.empty()) {
				try
				{
					readFile(filePath + matchedRoute->defaultFile);
					filePath += matchedRoute->defaultFile;
				}
				catch(const std::exception& e)
				{
					//
				}
            }
			std::cout << "filePath: " << filePath << std::endl;
            break;
        }
    }

	filePath.erase(std::unique(filePath.begin(), filePath.end(), [](char a, char b) {
		return a == '/' && b == '/';
	}), filePath.end());
	std::cout << "filePath: " << filePath << std::endl;

    if (!matchedRoute) {
        // Versuche die 404 Error Page zu laden
		std::cout << "404 Not Found" << std::endl;
        return HttpResponse::ErrorResponse("404 Not Found", _config.errorPages[404]);
    }

	std::cout << "DirectoryListin" << matchedRoute->directoryListing << std::endl;

	if (std::filesystem::is_directory(filePath) && !matchedRoute->directoryListing)
	{
		// 403 Forbidden
		HttpResponse::ErrorResponse("403 Forbidden", _config.errorPages[403]);
	}

    // Prüfe, ob es sich um ein Verzeichnis handelt
    if (std::filesystem::is_directory(filePath)) {
    	if (matchedRoute->directoryListing) {
            // Erstelle Directory Listing
            std::string listing = "<ul>";
            for (const auto& entry : std::filesystem::directory_iterator(filePath)) {
                std::string relativePath = path + (path.back() == '/' ? "" : "/") + entry.path().filename().string();
                if (entry.is_directory()) {
                    listing += "<li class='directory'><a href='" + relativePath + "/'>" + entry.path().filename().string() + "/</a></li>";
                } else {
                    listing += "<li class='file'><a href='" + relativePath + "'>" + entry.path().filename().string() + "</a></li>";
                }
            }
            listing += "</ul>";

			return HttpResponse::generate("200 OK", "text/html", listing);
        }
    }

    // Versuche die Datei zu lesen
    try {
        std::string content = readFile(filePath);
        std::string contentType = getContentType(filePath);
        
		return HttpResponse::generate("200 OK", contentType, content);
    } catch (const std::exception&) {
        // Versuche die 404 Error Page zu laden
		return HttpResponse::ErrorResponse("404 Not Found", _config.errorPages[404]);
    }
}