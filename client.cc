#include <iostream>
#include <fstream>
#include "httplib.h"

void download_html(const std::string& host, int port) {
    httplib::Client cli(host, port);
    if (auto res = cli.Get("/")) {
        std::cout << "✅ HTML Response Code: " << res->status << "\n";
        std::ofstream out("received_index.html");
        out << res->body;
        out.close();
        std::cout << "✅ Saved as 'received_index.html'\n";
    } else {
        std::cerr << "❌ Failed to get HTML. No response from server.\n";
    }
}

void download_image(const std::string& host, int port, const std::string& image_path) {
    httplib::Client cli(host, port);
    if (auto res = cli.Get(image_path.c_str())) {
        std::cout << "✅ Image Response Code: " << res->status << "\n";
        if (res->status == 200) {
            std::ofstream out("received_image.jpg", std::ios::binary);
            out << res->body;
            out.close();
            std::cout << "✅ Image saved as 'received_image.jpg'\n";
        } else {
            std::cerr << "❌ Failed to get image. Status: " << res->status << "\n";
        }
    } else {
        std::cerr << "❌ Failed to connect to server or no response.\n";
    }
}

int main() {
    std::string host = "localhost";
    int port = 8888;

    std::cout << "🌐 Connecting to " << host << ":" << port << "...\n";

    download_html(host, port);
    download_image(host, port, "/image.jpg");  // Adjust if your image path is different

    return 0;
}
