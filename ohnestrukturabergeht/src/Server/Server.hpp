#ifndef SERVER_HPP
#define SERVER_HPP

#include "../Config/ConfigParser.hpp"
#include <vector>
#include <string>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

struct MultipartPart {
    std::map<std::string, std::string> headers;
    std::string body;
};

class Server {
private:
    std::vector<struct pollfd> _fds;
    std::map<int, std::string> _clientBuffers;
    std::string _host;
    int _port;
    int _serverSocket;
    int _currentClientSocket;
    ServerConfig _config;
    
    void setNonBlocking(int socket);
    void handleNewConnection();
    void handleClientData(int clientSocket);
    void sendResponse(int clientSocket, const std::string& response);
    std::string handleGETRequest(const std::string& path);
    std::string handlePOSTRequest(const std::string& path, const std::string& contentType, 
                                 const std::string& body);
    std::string readFile(const std::string& path);
    std::string getContentType(const std::string& path);
    std::string getBoundary(const std::string& contentType);
    std::vector<MultipartPart> parseMultipartFormData(const std::string& body, const std::string& boundary);
    std::string extractFilename(const std::string& contentDisposition);
    void saveUploadedFile(const std::string& uploadDir, const std::string& filename, const std::string& content);
    std::string getDefaultFile(const std::string& directoryPath, const std::string& defaultFile);

public:
    Server(const ServerConfig& config);
    ~Server();
    
    void setup();
    void handleConnections();
    void stop();
};

#endif