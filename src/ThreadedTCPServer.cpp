#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <mutex>

std::mutex cout_mutex;

void handle_client(int client_socket) {
    char buffer[1024] = {0};

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[+] New client thread started\n";
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(client_socket, buffer, sizeof(buffer), 0);
        if (valread <= 0) {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "[-] Client disconnected\n";
            break;
        }

        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[<] Received: " << buffer << std::endl;

        if (strcmp(buffer, "SYN") == 0) {
            const char* response = "SYN-CUSTACK";
            send(client_socket, response, strlen(response), 0);
        }
    }

    close(client_socket);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    std::cout << "🚀 Threaded TCP Server listening on port 12345...\n";

    std::vector<std::thread> threads;

    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            std::cerr << "[x] Failed to accept client\n";
            continue;
        }

        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[+] Client connected!" << std::endl;

        threads.emplace_back(std::thread(handle_client, new_socket));
        threads.back().detach();  // Or keep them joinable if you want to control lifetime
    }

    close(server_fd);
    return 0;
}
