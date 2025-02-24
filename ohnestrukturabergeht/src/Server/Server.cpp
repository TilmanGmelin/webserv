#include "Server.hpp"

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
        buffer[bytesRead] = '\0';
        _clientBuffers[clientSocket] += std::string(buffer.get(), bytesRead);
    }

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

    std::istringstream requestStream(_clientBuffers[clientSocket]);
    std::string requestLine;
    std::getline(requestStream, requestLine);
    
    std::istringstream requestLineStream(requestLine);
    std::string method, path, protocol;
    requestLineStream >> method >> path >> protocol;

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
        }
                
        size_t headerEnd = _clientBuffers[clientSocket].find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            // Extrahiere den Body
            std::string body = _clientBuffers[clientSocket].substr(headerEnd + 4);

            // Prüfe, ob der komplette Body angekommen ist
            if (body.length() < contentLength) {
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
        response = HttpResponse::ErrorResponse("405 Method Not Allowed", _config.errorPages[405]);
        sendResponse(clientSocket, response);
        _clientBuffers[clientSocket].clear();
    }
}

void Server::sendResponse(int clientSocket, const std::string& response) {
    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
    _clientBuffers.erase(clientSocket);
    _fds.erase(std::remove_if(_fds.begin(), _fds.end(),
        [clientSocket](const struct pollfd& pfd) { return pfd.fd == clientSocket; }
    ), _fds.end());
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