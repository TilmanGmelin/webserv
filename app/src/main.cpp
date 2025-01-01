#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <map>
#include <sstream>
#include <vector>
#include <poll.h>
#include <fcntl.h>

// Send HTML content to client
void get_html(int client_socket, const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open HTML file: " << file_path << std::endl;
        return;
    }

    std::string html_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\nContent-Length: " + std::to_string(html_content.size()) + "\n\n" + html_content;

    // Non-blocking send in chunks
    size_t total_sent = 0;
    while (total_sent < response.size()) {
        ssize_t sent = send(client_socket, response.c_str() + total_sent, response.size() - total_sent, 0);
        if (sent < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue; // Retry later
            } else {
                perror("send");
                break;
            }
        }
        total_sent += sent;
    }
}


int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

	// setsocketopt SO_REUSEADDR ist fur das wiederverwenden der adresse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }


    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

	// htons() wandelt den port in das richtige format um
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    return server_fd;
}

int main(int argc, char* argv[]) {
	// Start check
    if (argc != 2) {
        std::cerr << "Required:\n\n    ./webserv <config_file>\n" << std::endl;
        exit(EXIT_FAILURE);
    }

	// Map for port to file
    std::map<int, std::string> port_to_file;
	// Map for port to file descriptor
	std::map<int, int> port_to_fd;


	/* START SIMPLES TESTING EINLESEN (config.vorerst) */ 
    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        std::cerr << "Konnte Konfigurationsdatei nicht öffnen: " << argv[1] << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(config_file, line)) {
        std::istringstream iss(line);
        int port;
        std::string file;
        if (!(iss >> port >> file)) {
            continue; // Fehlerhafte Zeile überspringen
        }
        port_to_file[port] = file;
    }
    config_file.close();

	/* ENDE SIMPLES TESTING EINLESEN */ 

    std::vector<struct pollfd> poll_fds;

	// Erstelle Server-Socket für jeden Port
    for (const auto& entry : port_to_file) {
        int port = entry.first;
        int server_fd = create_server_socket(port);
		port_to_fd[port] = server_fd;

        struct pollfd pfd;
        pfd.fd = server_fd;
        pfd.events = POLLIN;
        poll_fds.push_back(pfd);
    }

	// Warte auf eingehende Verbindungen
    while (true) {
		// Warte auf Aktivität
        int activity = poll(poll_fds.data(), poll_fds.size(), -1);

        if (activity < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

		// Für jeden Socket
        for (auto& pfd : poll_fds) {
            if (pfd.revents & POLLIN) {
                struct sockaddr_in address;
                socklen_t addrlen = sizeof(address);
                int new_socket = accept(pfd.fd, (struct sockaddr *)&address, &addrlen);

                if (new_socket < 0) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("accept");
                    }
                    continue;
                }

                // new socket to no blocking
                fcntl(new_socket, F_SETFL, O_NONBLOCK);

				// Find the port for the socket
                auto it = std::find_if(port_to_fd.begin(), port_to_fd.end(), [&pfd](const auto& entry) {
					return pfd.fd == entry.second;
				});

				// Get the file path for the port
				if (it != port_to_fd.end()) {
					int port = it->first;
					const std::string& file_path = port_to_file[port];
					get_html(new_socket, file_path);
				} else {
					std::cerr << "no corresponding socket" << std::endl;
				}

                close(new_socket);
            }
        }
    }

	// Close all sockets
    for (auto& pfd : poll_fds) {
        close(pfd.fd);
    }

    return EXIT_SUCCESS;
}




/*
A package is what we recieve from the client
*/
struct package;

/*
Configureation of the webserver.
name of webserver, paths for allowed cgis and so on
*/
class config;

/*
handles the open socket and creates packages from incoming data.
*/
class Listener;


/*
Recieves packages and executes based on configuration
*/
class Executer;

/*
we need to determine how exactly the different parts of the configuration
are important for different parts of the process.
We also may need a session manager and/or an authenticator (for tokens)

*/

