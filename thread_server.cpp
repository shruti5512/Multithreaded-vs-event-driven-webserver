#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <mutex>    
#include <atomic>

using namespace std;

string document_root = ".";
int port = 8080;
atomic<int> active_connections(0);
mutex cout_mutex;

void send_error(int client_fd, int code, const string& message) {
    ostringstream response;
    response << "HTTP/1.1 " << code << " " << message << "\r\n"
             << "Content-Length: " << message.length() << "\r\n"    
             << "Connection: close\r\n\r\n"
             << message;
    send(client_fd, response.str().c_str(), response.str().length(), 0);
}

bool ends_with(const string& str, const string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


string get_mime_type(const string& filename) {
    if (ends_with(filename,".html")) return "text/html";
    if (ends_with(filename,".jpg")) return "image/jpeg";
    if (ends_with(filename,".png")) return "image/png";
    if (ends_with(filename,".css")) return "text/css";
    if (ends_with(filename,".js")) return "application/javascript";
    return "text/plain";
}

void handle_client(int client_fd) {
    active_connections++;
    char buffer[8192];
    ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);

    if (received <= 0) {
        close(client_fd);
        active_connections--;
        return;
    }

    istringstream request(string(buffer, received));
    string method, path, version;
    request >> method >> path >> version;

    if (method != "GET") {
        send_error(client_fd, 405, "Method Not Allowed");
        close(client_fd);
        active_connections--;
        return;
    }

    if (path == "/") path = "/index.html";
    string full_path = document_root + path;

    struct stat st;
    if (stat(full_path.c_str(), &st) != 0 || S_ISDIR(st.st_mode)) {
        send_error(client_fd, 404, "Not Found");
        close(client_fd);
        active_connections--;
        return;
    }

    ifstream file(full_path, ios::binary);
    if (!file.is_open()) {
        send_error(client_fd, 403, "Forbidden");
        close(client_fd);
        active_connections--;
        return;
    }

    ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Content-Type: " << get_mime_type(full_path) << "\r\n"
           << "Connection: close\r\n\r\n";
    send(client_fd, header.str().c_str(), header.str().length(), 0);

    char file_buf[8192];
    while (file.read(file_buf, sizeof(file_buf))) {
        send(client_fd, file_buf, file.gcount(), 0);
    }
    send(client_fd, file_buf, file.gcount(), 0);

    close(client_fd);
    active_connections--;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (string(argv[i]) == "-document_root") document_root = argv[++i];
        else if (string(argv[i]) == "-port") port = stoi(argv[++i]);
    }

// Create a socket with (domain = Internet Domain, type = sock_stream, protocol = default protocol), returns socket handle (a file descriptor)
// Set socketoption to reuse local address
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//  Create a structure for handling internet addresses
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

//  Bind and listen to the port
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Multithreaded server on port " << port<< ", serving from " << document_root << endl;
    }

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        thread(handle_client, client_fd).detach();
    }

    close(server_fd);
    return 0;
}