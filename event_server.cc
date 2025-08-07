#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>

using namespace std;

// Global config for the document root and port (set via command-line)
string ev_document_root = ".";
int ev_port = 8080;

// Buffers for incoming request data and outgoing response data per client FD
map<int, string> client_buffers;
map<int, string> response_buffers;

// Helper function: checks if a string ends with a given suffix (e.g., ".html")
bool ends_with(const string& str, const string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Returns appropriate MIME type based on file extension
string ev_get_mime_type(const string& filename) {
    if (ends_with(filename,".html")) return "text/html";
    if (ends_with(filename,".jpg"))  return "image/jpeg";
    if (ends_with(filename,".png"))  return "image/png";
    if (ends_with(filename,".css"))  return "text/css";
    if (ends_with(filename,".js"))   return "application/javascript";
    return "text/plain";
}

// Builds the full HTTP response string for a given path
string ev_build_response(const string& path) {
    // Translate "/" to "/index.html"
    string full_path = ev_document_root + (path == "/" ? "/index.html" : path);
    
    struct stat st;
    // If file doesn't exist or is a directory, return 404
    if (stat(full_path.c_str(), &st) != 0 || S_ISDIR(st.st_mode)) {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
    }

    ifstream file(full_path, ios::binary);
    ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Content-Type: " << ev_get_mime_type(full_path) << "\r\n"
           << "Connection: close\r\n\r\n";

    ostringstream content;
    content << header.str() << file.rdbuf();
    return content.str();  // Full HTTP response with header + file body
}

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-document_root") ev_document_root = argv[++i];
        else if (string(argv[i]) == "-port") ev_port = stoi(argv[++i]);
    }

    // Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Make server socket non-blocking
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // Allow reuse of port if program restarts quickly
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Set up address struct to bind the socket
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ev_port);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);  // Start listening with max queue length

    cout << "Event-driven server on port " << ev_port
         << ", serving from " << ev_document_root << endl;

    // FD sets for select()
    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);  // Add server socket to master set
    int fd_max = server_fd;          // Track highest FD for select()

    // Main loop
    while (true) {
        read_fds = master_set;  // Copy master set to pass to select()

        // Block until at least one FD is ready
        select(fd_max + 1, &read_fds, nullptr, nullptr, nullptr);

        // Check each FD to see if it's ready
        for (int fd = 0; fd <= fd_max; ++fd) {
            if (!FD_ISSET(fd, &read_fds)) continue;

            // Accept new connections
            if (fd == server_fd) {
                sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);

                // Make client socket non-blocking
                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                FD_SET(client_fd, &master_set);  // Add client to set

                if (client_fd > fd_max) fd_max = client_fd;  // Update max FD
            } 
            // Handle existing client connections
            else {
                char buf[4096];
                ssize_t bytes = recv(fd, buf, sizeof(buf), 0);

                // If error or client closed connection
                if (bytes <= 0) {
                    close(fd);
                    FD_CLR(fd, &master_set);
                    client_buffers.erase(fd);
                    response_buffers.erase(fd);
                } else {
                    // Append received data to buffer
                    client_buffers[fd] += string(buf, bytes);

                    // Very basic HTTP request parsing (only supports GET)
                    istringstream iss(client_buffers[fd]);
                    string method, path, version;
                    iss >> method >> path >> version;

                    if (method == "GET") {
                        response_buffers[fd] = ev_build_response(path);

                        // Send the entire response
                        send(fd, response_buffers[fd].c_str(), response_buffers[fd].size(), 0);
                    }

                    // Close connection after response (HTTP/1.0 style)
                    close(fd);
                    FD_CLR(fd, &master_set);
                    client_buffers.erase(fd);
                    response_buffers.erase(fd);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
