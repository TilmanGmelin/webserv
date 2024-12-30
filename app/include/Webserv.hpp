#ifndef WEBSERV_HPP
# define WEBSERV_HPP

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



#endif