#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <string>
#include <list>
#include <iterator>
#include <poll.h>
#include <map>
#include <vector>

namespace webs
{
#define MAX_SERVERNAME_LEN 32
	/*
	A package is what we recieve from the client.
	The listener ist responsible for building these and handling sliced
	packages
	*/
	enum Http_Method {NAN1, GET, POST, NAN2, DELETE}; // NAN1 and 2 is used so it corresponds with the bitfield values of a route
	struct Package
	{
		std::string							uri;            // URI of the request (path)
		Http_Method 						method;         // HTTP method (GET, POST, DELETE)
		std::map<std::string, std::string>	query_params;	// Query parameters from the URI (e.g., ?key=value)
		std::map<std::string, std::string>	headers;     	// HTTP headers (e.g., Content-Type)
		std::string 						body;           // Body of the request (e.g., content of a POST request)
		uint32_t							size;           // Size of the package body in bytes
		uint16_t							fd;				// fd of the connection the package was recieved on
	};

	/*
	Route Configuration:
	- Allowed methods: Define which HTTP methods are allowed for this route (GET, POST, DELETE).
	- Redirection: Set a URL to redirect to, useful for handling errors or restructuring.
	- Directory listing: Enable or disable directory listing if the requested URL is a directory.
	- Default file: Specify a default file (e.g., index.html) to serve when the client requests a directory.
	*/
	#define ROUTE_METHOD_GET			1
	#define ROUTE_METHOD_POST			2
	#define ROUTE_METHOD_DELETE			4
	#define ROUTE_METHOD_DIR_LISTING	8
	struct Route
	{
		std::string	root;				// directory to map to filesystem
		std::string	redirect;			// You can also serve static files but redirect to another route (or page) if a file is not found. WARNING: REDIRECTION LOOP NEEDS TO BE HANDLED GRACEFULLY ( maybe after like 20-30 redirections)
		std::string	default_file;		// Set a default file to answer if the request is a directory.
		char		allowed_methods;	// bitfield for allowed method. Example: ROUTE_METHOD_GET | ROUTE_METHOD_POST (= 3)
	};

	struct ServerConfig
	{
		std::string 						root;			// Root directory for this server
		std::vector<uint16_t>				ports;			// Ports this server is listening on
		std::vector<std::string>			server_names;	// list of domain names the server goes by
		std::vector<Route>					routes;			// list of available routes
		std::map<std::string, std::string> 	error_pages;	// default error pages for different errors...? its plural in the subject. idk....... Clarification requested. 
		uint32_t							max_body_size;	// max package body size the server accepts
	};

	/*
	Configuration of the webserver.
	name of webserver, paths for allowed cgis and so on
	*/
	class Config
	{
	private:
		std::list<ServerConfig>	server_configs_;
		bool					err_; 				// set to true on error during initialixation.
		Config();
	public:
		Config(const std::string& _config_file);
		~Config();

		// function to check if any server config is missing parts or is generally mallformed.
		// this will be called after initialization to check if at least one server config is present
		// and that no error occured;
		bool	Validate();

		// Iterator for getting all server_configs later
		std::list<ServerConfig>::iterator begin() { return (server_configs_.begin()); }
		std::list<ServerConfig>::iterator end()	  { return (server_configs_.end());   }

		// Print function for debugging. 
		void debug_print();
	};
		// std::string	root;				// directory to map to filesystem
		// std::string	redirect;			// You can also serve static files but redirect to another route (or page) if a file is not found. WARNING: REDIRECTION LOOP NEEDS TO BE HANDLED GRACEFULLY ( maybe after like 20-30 redirections)
		// std::string	default_file;		// Set a default file to answer if the request is a directory.
		// char		allowed_methods;		// bitfield for allowed method. Example: ROUTE_METHOD_GET | ROUTE_METHOD_POST (= 3)
		// bool		dir_listing;			// Turn on or off directory listing.

	class Server
	{
	private:
		// Configuration
		std::string 				root_;
		std::vector<std::string> 	server_names_;
		uint32_t					max_client_body_size_;

		// Routes
		std::vector<std::string>	route_path_;
		std::vector<std::string>	route_redirect_;
		std::vector<std::string>	route_default_file_;
		std::vector<char>			route_allowed_methods_;

		// internal data structures
		std::vector<Package*>		working_set;
	public:
		Server(const ServerConfig& _config);
		~Server();

		void HandlePackage(Package* _package);
		void HandleIOOperationComplete(uint32_t _operation_id, uint32_t _err_code);

		//TODO
	};

	/*
		maps a ports and server ids.
		for example:
		ports 		= {80, 80, 8008}
		server_ids 	= { 0,	1,    0}

		this structure is exclusively used for mapping a package to a server.
	*/
	struct	PortServerMapping
	{
		std::vector<uint16_t>					ports;
		std::vector<char[MAX_SERVERNAME_LEN]>	name;	
		std::vector<uint8_t>					server_ids;
	};

	class ServerController
	{
	private:
		std::vector<Server*>	servers_;
		PortServerMapping		ports_to_servers_;

		ServerController() = delete;
	public:
		ServerController(Config _config);
		void Dispatch(Package* _package, uint32_t _fd);
		void SignalFileOpComplete(int _err_code, uint8_t _server_id, uint32_t _operation_id);
		std::vector<uint16_t> GetWantedPorts();
		void DebugPrint();
	};

	struct Response
	{
		uint32_t size;
		uint32_t bytes_send;
		char*	 data;
	};

	//ASYNC IOINTERFACE
	class IOInterface
	{
	private:
		std::vector<struct pollfd> open_fds_;			// vector with 5 regions: listened_sockets | open_read_connections | open_write_connections | read_file | write_file
		std::vector<uint32_t>	   awaited_revent_;
		uint16_t				   socket_count_;		//start with 0
		uint16_t				   read_con_count_;		//start with 0
		uint16_t				   write_con_count_;	//start with 0
		uint16_t				   read_file_count_;	//start with 0
		uint16_t				   write_file_count_;	//start with 0

		// data for open sockets
		std::vector<uint16_t>	port_;

		// data for open_read_connections
		std::vector<uint16_t>		connection_port_;
		std::vector<std::string>	recieved_data_;

		// data for open_write_connections
		std::vector<Response>	responses_;

		// data for read file operations
		std::vector<std::string>	read_filepaths_;		//path to file 
		std::vector<std::string*>	read_data_outs_;
		std::vector<uint32_t>		read_operation_id_;
		std::vector<uint8_t>		read_server_id_;

		// data for write file operations
		std::vector<std::string>	write_filepaths_;
		std::vector<std::string*>	write_data_ins_;
		std::vector<uint32_t>		write_op_bytes_written_;
		std::vector<uint32_t>		write_operation_id_;
		std::vector<uint8_t>		write_server_id_;

		// erase functions for local datastructures (inserts seem to be basially the same as register.)
		void EraseReadConnetion(uint32_t _index);
		void EraseWriteConnection(uint32_t _index);
		void EraseReadFileOperation(uint32_t _index);
		void EraseWriteFileOperation(uint32_t _index);

		// internal functions to handle fds
		inline void Listen(uint32_t& _index); 		// calls RegisterRecieveData
		inline void RecieveData(uint32_t& _index); 	// recv incoming data and form a Package once done. calls ServerController.Dispatch(package) when entire package is recieved. 
		inline void SendData(uint32_t& _index);		// sends data to client
		inline void ReadFile(uint32_t& _index);		// reads a file and calls ServerController.SignalFileOpComplete() once done
		inline void WriteFile(uint32_t& _index);	// writes a file and calls ServerController.SignalFileOpComplete() once done

		void RegisterRecieveData(uint32_t _connection_fd);
	public:
		IOInterface();
		IOInterface(std::vector<uint16_t> _listen_ports);
		~IOInterface();

		void RegisterListen(uint16_t _port);
		void RegisterSendData(uint32_t _fd, Response _response);
		void RegisterReadFile(std::string& _filepath, std::string* _data_out, uint32_t _operation_id, uint8_t _server_id);
		void RegisterWriteFile(std::string& _filepath, std::string* _data, uint32_t _operation_id, uint8_t _server_id); //duno if char* or 

		// Function that will be called in a loop.
		void Dispatch();

		void DebugPrint();
	};
};



#endif