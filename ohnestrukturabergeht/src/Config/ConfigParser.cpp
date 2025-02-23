#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

ConfigParser::ConfigParser(const std::string& configPath) 
    : _configPath(configPath) {}

void ConfigParser::parse() {
    std::ifstream file(_configPath);
    if (!file.is_open()) {
        throw ConfigError("Cannot open config file: " + _configPath);
    }

    std::string line;
    size_t lineNum = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        // Entferne Kommentare und führende/nachfolgende Leerzeichen
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Entferne Leerzeichen am Anfang und Ende
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (line == "server {") {
            ServerConfig server;
            parseServerBlock(file, server);
            _servers.push_back(server);
        } else {
            throw ConfigError("Unexpected line outside server block at line " + 
                            std::to_string(lineNum));
        }
    }
    
    validateConfig();
}

void ConfigParser::parseServerBlock(std::istream& file, ServerConfig& server) {
    std::string line;
    
    while (std::getline(file, line)) {
        // Bereinige die Zeile
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        if (line == "}") return;
        
        if (line.substr(0, 8) == "location") {
            Route route;
            size_t pathStart = line.find('/');
            if (pathStart == std::string::npos) {
                throw ConfigError("Invalid location format");
            }
            size_t pathEnd = line.find('{');
            if (pathEnd == std::string::npos) {
                throw ConfigError("Missing { in location block");
            }
            route.path = line.substr(pathStart, pathEnd - pathStart - 1);
            parseRouteBlock(file, route);
            server.routes.push_back(route);
            continue;
        }
        
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;
        
        if (directive == "listen") {
            std::string hostPort;
            iss >> hostPort;
            
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                server.host = hostPort.substr(0, colonPos);
                server.port = std::stoi(hostPort.substr(colonPos + 1));
            } else {
                server.port = std::stoi(hostPort);
            }
        }
        else if (directive == "server_name") {
            std::string name;
            while (iss >> name) {
                server.serverNames.push_back(name);
            }
        }
        else if (directive == "error_page") {
            int code;
            std::string path;
            iss >> code >> path;
            server.errorPages[code] = path;
        }
        else if (directive == "client_max_body_size") {
            std::string size;
            iss >> size;
            char unit = size.back();
            size.pop_back();
            size_t value = std::stoul(size);
            
            switch (unit) {
                case 'K': value *= 1024; break;
                case 'M': value *= 1024 * 1024; break;
                case 'G': value *= 1024 * 1024 * 1024; break;
                default: throw ConfigError("Invalid size unit in client_max_body_size");
            }
            server.clientMaxBodySize = value;
        }
    }
    
    throw ConfigError("Unexpected end of file in server block");
}

void ConfigParser::parseRouteBlock(std::istream& file, Route& route) {
    std::string line;
    
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        if (line == "}") return;
        
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;
        
        if (directive == "methods") {
            std::string method;
            while (iss >> method) {
                route.allowedMethods.insert(method);
            }
        }
        else if (directive == "root") {
            iss >> route.root;
        }
        else if (directive == "return") {
            iss >> route.redirect;
        }
        else if (directive == "autoindex") {
            std::string value;
            iss >> value;
            route.directoryListing = (value == "on");
        }
        else if (directive == "index") {
            iss >> route.defaultFile;
        }
        else if (directive == "upload_store") {
            iss >> route.uploadDir;
            std::cout << "DEBUG: Setting upload_store path to: " << route.uploadDir << std::endl;
            
            // Stelle sicher, dass der Pfad mit einem Slash endet
            if (!route.uploadDir.empty() && route.uploadDir.back() != '/') {
                route.uploadDir += '/';
            }
            
            // Erstelle das Verzeichnis, falls es nicht existiert
            try {
                std::filesystem::create_directories(route.uploadDir);
                std::cout << "Created upload directory: " << route.uploadDir << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not create upload directory: " << e.what() << std::endl;
            }
        }
    }
    
    throw ConfigError("Unexpected end of file in location block");
}

void ConfigParser::validateConfig() const {
    if (_servers.empty()) {
        throw ConfigError("No server blocks found in config");
    }
    
    std::map<std::pair<std::string, int>, int> portBindings;
    
    for (const auto& server : _servers) {
        auto binding = std::make_pair(server.host, server.port);
        if (++portBindings[binding] > 1) {
            throw ConfigError("Duplicate host:port binding: " + 
                            server.host + ":" + std::to_string(server.port));
        }
        
        for (const auto& route : server.routes) {
            // Nur Warnung ausgeben, wenn Verzeichnisse nicht existieren
            if (!route.root.empty() && !std::filesystem::exists(route.root)) {
                std::cerr << "Warning: Root directory does not exist: " << route.root << std::endl;
                // Versuche das Verzeichnis zu erstellen
                std::filesystem::create_directories(route.root);
            }
            
            if (!route.uploadDir.empty() && !std::filesystem::exists(route.uploadDir)) {
                std::cerr << "Warning: Upload directory does not exist: " << route.uploadDir << std::endl;
                // Versuche das Verzeichnis zu erstellen
                std::filesystem::create_directories(route.uploadDir);
				std::cout << "Created upload directory: " << route.uploadDir << std::endl;
            }
            
            for (const auto& method : route.allowedMethods) {
                if (method != "GET" && method != "POST" && method != "DELETE") {
                    throw ConfigError("Invalid HTTP method: " + method);
                }
            }
        }
    }
}