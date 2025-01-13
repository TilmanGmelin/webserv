#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <map>

class CGIHandler {
private:
    std::string _scriptPath;
    std::map<std::string, std::string> _env;
  
    std::string executeCGI(const std::string& input);

public:
    CGIHandler(const std::string& scriptPath);
	void setupEnvironment(const std::string& method, 
                         const std::string& queryString,
                         const std::string& contentType,
                         size_t contentLength);
    
    std::string handleRequest(const std::string& method,
                            const std::string& queryString,
                            const std::string& contentType,
                            const std::string& body);
};

#endif 