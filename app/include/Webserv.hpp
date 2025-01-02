#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <map>
#include <string>
#include <list>
#include <iterator>

namespace webs
{
	/*
	A package is what we recieve from the client.
	The listener ist responsible for building these and handling sliced
	packages
	*/
	enum Http_Method {NAN1, GET, POST, NAN2, DELETE}; // NAN1 and 2 is used so it corresponds with the bitfield values of a route
	struct Package
	{
		std::string uri;                         			// URI of the request (path)
		Http_Method method;                      			// HTTP method (GET, POST, DELETE)
		std::map<std::string, std::string> query_params;	// Query parameters from the URI (e.g., ?key=value)
		std::map<std::string, std::string> headers;     	// HTTP headers (e.g., Content-Type)
		uint32_t size;                           			// Size of the package body in bytes
		std::string body;                       			// Body of the request (e.g., content of a POST request)
	};


	/*
	Route Configuration:
	- Allowed methods: Define which HTTP methods are allowed for this route (GET, POST, DELETE).
	- Redirection: Set a URL to redirect to, useful for handling errors or restructuring.
	- Directory listing: Enable or disable directory listing if the requested URL is a directory.
	- Default file: Specify a default file (e.g., index.html) to serve when the client requests a directory.
	*/
	#define ROUTE_METHOD_GET	1
	#define ROUTE_METHOD_POST	2
	#define ROUTE_METHOD_DELETE	4
	struct Route
	{
		std::string	root;				// directory to map to filesystem
		std::string	redirect;			// You can also serve static files but redirect to another route (or page) if a file is not found. WARNING: REDIRECTION LOOP NEEDS TO BE HANDLED GRACEFULLY ( maybe after like 20-30 redirections)
		std::string	default_file;		// Set a default file to answer if the request is a directory.
		char		allowed_methods;	// bitfield for allowed method. Example: ROUTE_METHOD_GET | ROUTE_METHOD_POST (= 3)
		bool		dir_listiing;		// Turn on or off directory listing.
	};

	struct ServerConfig
	{
		std::string 						root;			// Root directory for this server
		std::string						 	host;			// host of this server
		std::list<uint32_t>					ports;			// Ports this server is listening on
		std::list<std::string>				server_names;	// list of domain names the server goes by
		std::list<Route>					routes;			// list of available routes
		std::map<std::string, std::string> 	error_pages;	// default error pages for different errors...? its plural in the subject. idk....... Clarification requested. 
		uint32_t							max_body_size;	// max package body size the server accepts
	};

	/*
	Configureation of the webserver.
	name of webserver, paths for allowed cgis and so on
	*/
	class Config
	{
	private:
		std::list<ServerConfig>	server_configs_;
		bool	err; // set to true on error during initialization.
		Config();
	public:
		Config(std::string _config_file);
		~Config();

		// function to check if any server config is missing parts or is generally mallformed.
		// this will be called after initialization to check if at least one server config is present
		// and that no error occured;
		bool	Validate();

		// Iterator for later getting all server_configs
		std::list<ServerConfig>::iterator begin() { return std::list<ServerConfig>::iterator(server_configs_.begin()); }
		std::list<ServerConfig>::iterator end()	  { return std::list<ServerConfig>::iterator(server_configs_.end());   }
	};

	/*
	handles the open socket and creates packages from incoming data.
	*/
	class Listener;

	/*
	matches packages to routes and corresponding configuration. 
	*/
	class Router;

	/*
	Recieves packages and executes based on configuration
	*/
	class Executer;

	/*
	we need to determine how exactly the different parts of the configuration
	are important for different parts of the process.
	We also may need a session manager and/or an authenticator (for tokens)

	*/
}


#endif