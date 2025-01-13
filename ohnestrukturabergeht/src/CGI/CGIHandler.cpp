#include "CGIHandler.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdexcept>
#include <iostream>

CGIHandler::CGIHandler(const std::string& scriptPath) : _scriptPath(scriptPath) {}

void CGIHandler::setupEnvironment(const std::string& method, 
                                const std::string& queryString,
                                const std::string& contentType,
                                size_t contentLength) {
    _env["REQUEST_METHOD"] = method;
    _env["QUERY_STRING"] = queryString;
    _env["CONTENT_TYPE"] = contentType;
    _env["CONTENT_LENGTH"] = std::to_string(contentLength);
    _env["SCRIPT_FILENAME"] = _scriptPath;
    _env["REDIRECT_STATUS"] = "200";
}

std::string CGIHandler::executeCGI(const std::string& input) {
    int inPipe[2];  // Parent -> Child
    int outPipe[2]; // Child -> Parent
    
    if (pipe(inPipe) < 0 || pipe(outPipe) < 0) {
        throw std::runtime_error("Failed to create pipes");
    }
    
    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Fork failed");
    }
    
    if (pid == 0) { // Child process
        close(inPipe[1]);
        close(outPipe[0]);
        
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        
        // Set environment variables
        for (const auto& env : _env) {
            setenv(env.first.c_str(), env.second.c_str(), 1);
        }
        
        execl(_scriptPath.c_str(), _scriptPath.c_str(), nullptr);
        exit(1);
    }
    
    // Parent process
    close(inPipe[0]);
    close(outPipe[1]);
    
    // Write input to CGI script
    write(inPipe[1], input.c_str(), input.length());
    close(inPipe[1]);
    
    // Read CGI output
    std::string output;
    char buffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, bytesRead);
    }
    
    close(outPipe[0]);
    
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("CGI script failed");
    }
    
    return output;
}

std::string CGIHandler::handleRequest(const std::string& method,
                                    const std::string& queryString,
                                    const std::string& contentType,
                                    const std::string& body) {
    setupEnvironment(method, queryString, contentType, body.length());
    return executeCGI(body);
} 