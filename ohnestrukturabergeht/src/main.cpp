#include "Server/Server.hpp"
#include "Config/ConfigParser.hpp"
#include <iostream>
#include <map>
#include <vector>

int main(int argc, char* argv[]) {
    try {
        std::string configFile = (argc > 1) ? argv[1] : "config/default.conf";
        
        ConfigParser parser(configFile);
        parser.parse();
        
        const auto& servers = parser.getServers();
        if (servers.empty()) {
            throw std::runtime_error("No server configuration found");
        }

        // Gruppiere Server nach Ports
        std::map<int, std::vector<Server>> serversByPort;
        
        for (const auto& config : servers) {
            serversByPort[config.port].emplace_back(config);
        }

        // Initialisiere alle Server
        for (auto& portServers : serversByPort) {
            for (auto& server : portServers.second) {
                server.setup();
            }
        }

        // Hauptloop für alle Server
        while (true) {
            for (auto& portServers : serversByPort) {
                for (auto& server : portServers.second) {
                    server.handleConnections();  // Neue non-blocking Methode
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
} 