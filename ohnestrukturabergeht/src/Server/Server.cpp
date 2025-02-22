#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>

Server::Server(const ServerConfig& config) 
    : _host(config.host), _port(config.port), _config(config) {}

Server::~Server() {
    stop();
}

void Server::setNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

void Server::setup() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Failed to set socket options");
    }

    setNonBlocking(_serverSocket);

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(_host.c_str());
    if (_host == "localhost" || _host.empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    }
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)) + 
                               " (Port: " + std::to_string(_port) + 
                               ", Host: " + _host + ")");
    }

    if (listen(_serverSocket, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    // // std::cout << "Server listening on " << _host << ":" << _port << std::endl;

    struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;
    _fds.push_back(serverPollFd);
}

void Server::handleConnections() {
    int ready = poll(_fds.data(), _fds.size(), 0);  // Non-blocking poll
    if (ready < 0) {
        if (errno == EINTR) return;
        throw std::runtime_error("Poll failed");
    }

    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].revents & POLLIN) {
            if (_fds[i].fd == _serverSocket) {
                handleNewConnection();
            } else {
                handleClientData(_fds[i].fd);
            }
        }
    }
}

void Server::handleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSocket < 0) {
        return;
    }

    setNonBlocking(clientSocket);
    
    struct pollfd clientPollFd;
    clientPollFd.fd = clientSocket;
    clientPollFd.events = POLLIN;
    _fds.push_back(clientPollFd);
}

void Server::handleClientData(int clientSocket) {
    _currentClientSocket = clientSocket;  // Speichere den aktuellen Socket
    const size_t bufferSize = 50000000;
    std::unique_ptr<char[]> buffer(new char[bufferSize]);
    ssize_t bytesRead;

    if ((bytesRead = recv(clientSocket, buffer.get(), bufferSize - 1, 0)) > 0) {
		// // std::cout << "Bytes read: " << bytesRead << std::endl;
        buffer[bytesRead] = '\0';
        _clientBuffers[clientSocket] += std::string(buffer.get(), bytesRead);
		// std::cout << "12_clientBuffers[clientSocket]: " << _clientBuffers[clientSocket] << std::endl;
		// std::cout << "12_Buffer: " << buffer.get() << std::endl;
		 // std::cout << "Bytes read: " << bytesRead << std::endl;
    }
	// std::cout << "_clientBuffers[clientSocket]: " << _clientBuffers[clientSocket] << "\n\n\n\n\n\n\n\n\n\n" << std::endl;

    if (bytesRead < 0 && errno != EWOULDBLOCK) {
        close(clientSocket);
        _clientBuffers.erase(clientSocket);
        _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
            [clientSocket](const struct pollfd& pfd) { return pfd.fd == clientSocket; }
        ), _fds.end());
        return;
    }

    if (bytesRead == 0) {
        close(clientSocket);
        _clientBuffers.erase(clientSocket);
        _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
            [clientSocket](const struct pollfd& pfd) { return pfd.fd == clientSocket; }
        ), _fds.end());
        return;
    }

    // std::cout << "Buffer: " << _clientBuffers[clientSocket] << std::endl;
	// std::cout << "2_clientBuffers[clientSocket]: " << _clientBuffers[clientSocket] << std::endl;

    // Parse HTTP Request
    std::istringstream requestStream(_clientBuffers[clientSocket]);
    std::string requestLine;
    std::getline(requestStream, requestLine);
    
    std::istringstream requestLineStream(requestLine);
    std::string method, path, protocol;
    requestLineStream >> method >> path >> protocol;

    // std::cout << "\n=== Request Debug ===\n";
    // std::cout << "Method: " << method << std::endl;
    // std::cout << "Path: " << path << std::endl;
    // std::cout << "Raw request: " << _clientBuffers[clientSocket] << std::endl;

    std::string response;

    if (method == "POST") {
        std::string contentType;
        size_t contentLength = 0;
        std::string line;
        
        // Parse headers
        while (std::getline(requestStream, line) && line != "\r" && line != "") {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            if (line.substr(0, 14) == "Content-Type: ") {
                contentType = line.substr(14);
            } else if (line.substr(0, 16) == "Content-Length: ") {
                contentLength = std::stoul(line.substr(16));
            }
            // std::cout << "Header: " << line << std::endl;
        }
        
        // std::cout << "Content-Length: " << contentLength << std::endl;
        
        size_t headerEnd = _clientBuffers[clientSocket].find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Extrahiere den Body
            std::string body = _clientBuffers[clientSocket].substr(headerEnd + 4);

            // std::cout << "Current body length: " << body.length() << ", expected: " << contentLength << std::endl;

            // Prüfe, ob der komplette Body angekommen ist
            if (body.length() < contentLength) {
                // std::cout << "Waiting for more data. buffereaded: " << bytesRead << ". Current body length: " << body.length() << ", expected: " << contentLength << std::endl;
                return; // Warte auf den nächsten Empfang
            }

            // Vollständiger Body ist da → Bearbeite die POST-Anfrage
            body = body.substr(0, contentLength);
            response = handlePOSTRequest(path, contentType, body);
            sendResponse(clientSocket, response);

            // Buffer für diesen Client aufräumen
            _clientBuffers[clientSocket].clear();
        } else {
            // std::cout << "Header not fully received yet. Waiting..." << std::endl;
        }
    } else if (method == "GET") {
        response = handleGETRequest(path);
        sendResponse(clientSocket, response);
        _clientBuffers[clientSocket].clear();
    } else {
        response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        sendResponse(clientSocket, response);
        _clientBuffers[clientSocket].clear();
    }
}

std::string Server::handleGETRequest(const std::string& path) {
    
    // Finde die passende Route
    const Route* matchedRoute = nullptr;
    std::string filePath;
    
    for (const auto& route : _config.routes) {
        if (path.substr(0, route.path.length()) == route.path) {
            matchedRoute = &route;
			
            // Relativen Pfad zum Root-Verzeichnis der Route hinzufügen
            filePath = route.root + "/" + path.substr(route.path.length());
            if (filePath.back() == '/') {
                filePath += matchedRoute->defaultFile;
            }
            break;
        }
    }

    if (!matchedRoute) {
        // Versuche die 404 Error Page zu laden
        try {
            std::string errorContent = readFile(_config.errorPages[404]);
            return "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + std::to_string(errorContent.length()) + "\r\n"
                   "\r\n" + errorContent;
        } catch (const std::exception& e) {
            // Fallback wenn die Error Page nicht geladen werden kann
            std::string defaultError = "<html><body><h1>404 Not Found</h1></body></html>";
            return "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + std::to_string(defaultError.length()) + "\r\n"
                   "\r\n" + defaultError;
        }
    }

    // Prüfe, ob es sich um ein Verzeichnis handelt
    if (std::filesystem::is_directory(filePath)) {
        if (!matchedRoute->defaultFile.empty()) {
            filePath += "/" + matchedRoute->defaultFile;
        } else if (matchedRoute->directoryListing) {
            // TODO: Implementiere Directory Listing
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: 25\r\n"
                   "\r\n"
                   "Directory listing enabled\n";
        }
    }

    // Versuche die Datei zu lesen
    try {
        std::string content = readFile(filePath);
        std::string contentType = getContentType(filePath);
        
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: " + contentType + "\r\n"
               "Content-Length: " + std::to_string(content.length()) + "\r\n"
               "\r\n" + content;
    } catch (const std::exception&) {
        // Versuche die 404 Error Page zu laden
        try {
            std::string errorContent = readFile(_config.errorPages[404]);
            return "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + std::to_string(errorContent.length()) + "\r\n"
                   "\r\n" + errorContent;
        } catch (const std::exception& e) {
            // Fallback wenn die Error Page nicht geladen werden kann
            std::string defaultError = "<html><body><h1>404 Not Found</h1></body></html>";
            return "HTTP/1.1 404 Not Found\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " + std::to_string(defaultError.length()) + "\r\n"
                   "\r\n" + defaultError;
        }
    }
}

std::string Server::handlePOSTRequest(const std::string& path, const std::string& contentType, const std::string& body) {
    // std::cout << "Handling POST request to: " << path << std::endl;
    // std::cout << "Content-Type: " << contentType << std::endl;
    // std::cout << "Body size: " << body.length() << std::endl;

    const Route* matchedRoute = nullptr;
    
    for (const auto& route : _config.routes) {
        if (path.substr(0, route.path.length()) == route.path) {
            matchedRoute = &route;
            // std::cout << "DEBUG: Matched route: " << matchedRoute->uploadDir << std::endl;
            break;
        }
    }

    if (!matchedRoute) {
        return "HTTP/1.1 404 Not Found\r\n\r\n";
    }

    if (matchedRoute->allowedMethods.find("POST") == matchedRoute->allowedMethods.end()) {
        return "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
    }

    std::string uploadPath = matchedRoute->uploadDir;
    // std::cout << "Using upload path: " << uploadPath << std::endl;

    if (uploadPath.empty()) {
        std::cerr << "No upload path configured for this route" << std::endl;
        return "HTTP/1.1 500 Internal Server Error\r\n"
               "Content-Type: text/plain\r\n"
               "Content-Length: 29\r\n"
               "\r\n"
               "No upload path configured\r\n";
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
            return "HTTP/1.1 201 Created\r\n"
                   "Content-Type: text/plain\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Content-Length: " + std::to_string(message.length()) + "\r\n"
                   "\r\n" + message;
        } else {
            std::string filename = "post_" + 
                std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".txt";
			// std::cout << "2 Saving file: " << filename << std::endl;
            saveUploadedFile(uploadPath, filename, body);
            
            return "HTTP/1.1 201 Created\r\n"
                   "Content-Type: text/plain\r\n"
                   "Access-Control-Allow-Origin: *\r\n"
                   "Content-Length: 7\r\n"
                   "\r\n"
                   "Created";
        }
    } catch (const std::exception& e) {
        std::string error = "Upload failed: " + std::string(e.what());
        return "HTTP/1.1 500 Internal Server Error\r\n"
               "Content-Type: text/plain\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Content-Length: " + std::to_string(error.length()) + "\r\n"
               "\r\n" + error;
    }
}

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

std::string Server::getContentType(const std::string& path) {
    std::string extension = path.substr(path.find_last_of('.') + 1);
    
    if (extension == "html") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    
    return "text/plain";
}

void Server::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
    _clientBuffers.erase(clientSocket);
    _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
        [clientSocket](const struct pollfd& pfd) { return pfd.fd == clientSocket; }
    ), _fds.end());
}

void Server::stop() {
    for (const auto& fd : _fds) {
        close(fd.fd);
    }
    _fds.clear();
    _clientBuffers.clear();
}

std::string Server::getBoundary(const std::string& contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) {
        throw std::runtime_error("No boundary found in content type");
    }
    return contentType.substr(pos + 9);
}

std::vector<MultipartPart> Server::parseMultipartFormData(const std::string& body, const std::string& boundary) {
    std::vector<MultipartPart> parts;
    size_t pos = 0;
    std::string delimiter = "--" + boundary;

    while (true) {
        size_t partStart = body.find(delimiter, pos);
        if (partStart == std::string::npos) break;
        partStart += delimiter.length();
        if (body.substr(partStart, 2) == "--") break; // Ende des Multipart-Dokuments
        partStart = body.find("\r\n", partStart) + 2;

        size_t partEnd = body.find(delimiter, partStart);
        if (partEnd == std::string::npos) break;
        partEnd -= 2; // Überspringe \r\n vor Boundary

        MultipartPart part;
        size_t headerEnd = body.find("\r\n\r\n", partStart);
        if (headerEnd == std::string::npos || headerEnd > partEnd) break;

        std::string headers = body.substr(partStart, headerEnd - partStart);
        size_t headerPos = 0;
        while (headerPos < headers.length()) {
            size_t lineEnd = headers.find("\r\n", headerPos);
            if (lineEnd == std::string::npos) lineEnd = headers.length();

            std::string line = headers.substr(headerPos, lineEnd - headerPos);
            size_t colonPos = line.find(": ");
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 2);
                part.headers[name] = value;
            }

            headerPos = lineEnd + 2;
        }

        part.body = body.substr(headerEnd + 4, partEnd - (headerEnd + 4));
        parts.push_back(part);

        pos = partEnd + 2;
    }

    return parts;
}

std::string Server::extractFilename(const std::string& contentDisposition) {
    size_t fnPos = contentDisposition.find("filename=\"");
    if (fnPos == std::string::npos) return "";
    
    fnPos += 10;
    size_t fnEnd = contentDisposition.find("\"", fnPos);
    if (fnEnd == std::string::npos) return "";
    
    return contentDisposition.substr(fnPos, fnEnd - fnPos);
}

void Server::saveUploadedFile(const std::string& uploadDir, const std::string& filename, const std::string& content) {
    if (filename.empty()) {
        throw std::runtime_error("No filename provided");
    }

    // std::cout << "Saving file to directory: " << uploadDir << std::endl;

    std::filesystem::create_directories(uploadDir);

    std::string safeFilename = std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) 
                              + "_" + filename;

    std::string fullPath = uploadDir + "/" + safeFilename;
    // std::cout << "Full path: " << fullPath << std::endl;

    std::ofstream file(fullPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + fullPath);
    }

    file.write(content.data(), content.size());
    if (!file) {
        throw std::runtime_error("Failed to write to file: " + fullPath);
    }

    // std::cout << "File saved successfully: " << fullPath << std::endl;
}