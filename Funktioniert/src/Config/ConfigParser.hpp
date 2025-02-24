#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>


struct Route {
    std::string path;
    std::set<std::string> allowedMethods;
    std::string root;
    std::string redirect;
    bool directoryListing;
    std::string defaultFile;
    std::string uploadDir;
    
    Route() : directoryListing(false) {}
};

struct ServerConfig {
    std::string host;
    int port;
    std::vector<std::string> serverNames;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::vector<Route> routes;
    
    ServerConfig() : port(80), clientMaxBodySize(1024 * 1024) {} // Default 1MB
};

class ConfigParser {
private:
    std::string _configPath;
    std::vector<ServerConfig> _servers;
    
    void parseLine(const std::string& line, size_t lineNum);
    void parseServerBlock(std::istream& file, ServerConfig& server);
    void parseRouteBlock(std::istream& file, Route& route);
    void validateConfig() const;
    
public:
    explicit ConfigParser(const std::string& configPath);
    
    void parse();
    const std::vector<ServerConfig>& getServers() const { return _servers; }
    
    class ConfigError : public std::runtime_error {
    public:
        explicit ConfigError(const std::string& msg) : std::runtime_error(msg) {}
    };
};

#endif 