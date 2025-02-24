
#include "Server.hpp"

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

    struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;
    _fds.push_back(serverPollFd);
}

void Server::stop() {
    for (const auto& fd : _fds) {
        close(fd.fd);
    }
    _fds.clear();
    _clientBuffers.clear();
}
