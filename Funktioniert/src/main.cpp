#include "Server/Server.hpp"
#include "Config/ConfigParser.hpp"
#include <iostream>
#include <map>
#include <vector>

void setupDefaultConfig(std::vector<ServerConfig>& servers) {
    std::cout << "Using internal default configuration" << std::endl;

    // Erstelle eine Standard-Serverkonfiguration
    ServerConfig defaultServer;
    defaultServer.host = "localhost";
    defaultServer.port = 80;
    defaultServer.serverNames = {"localhost"};
    defaultServer.clientMaxBodySize = 1024 * 1024 * 10; // 10 MB

    Route defaultRoute;
    defaultRoute.path = "/";
    defaultRoute.root = "www";
    defaultRoute.defaultFile = "default.html";
    defaultRoute.allowedMethods = {"POST", "GET"};
    defaultRoute.directoryListing = false;
    // defaultRoute.uploadDir = "uploads/";


    defaultServer.routes.push_back(defaultRoute);
    servers.push_back(defaultServer);
}

int main(int argc, char* argv[]) {
	// try block ist doch angeblich voll toll uber alles lmnao
	try {
        std::vector<ServerConfig> servers;

        if (argc > 1) {
            std::string configFile = "config/" + std::string(argv[1]);
            ConfigParser parser(configFile);
            parser.parse();
            servers = parser.getServers();
        } else {
            setupDefaultConfig(servers);
        }

        std::map<int, std::vector<Server>> serversByPort;
        
        for (const auto& config : servers) {
            serversByPort[config.port].emplace_back(config);
        }

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
    	return 0;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

} 