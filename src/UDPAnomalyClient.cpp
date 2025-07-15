#include "../include/UDPAnomalyClient.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <cstdlib>
#include <ctime>

UDPAnomalyClient::UDPAnomalyClient(const char* host, int port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    srand(time(0));

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &serverAddr.sin_addr);
}

void UDPAnomalyClient::configure(float loss, bool corrupt, bool dup, int delay) {
    packetLossRate = loss;
    corruptData = corrupt;
    duplicatePackets = dup;
    delayMs = delay;
}

std::string UDPAnomalyClient::corrupt(const std::string& data) {
    std::string modified = data;
    if (!modified.empty()) {
        int index = rand() % modified.size();
        modified[index] = '!';
    }
    return modified;
}

void UDPAnomalyClient::sendWithAnomalies(const std::string& data) {
    if (delayMs > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

    if (((float) rand() / RAND_MAX) < packetLossRate) {
        std::cout << "[UDP] Packet dropped (simulated): " << data << std::endl;
        return;
    }

    std::string payload = corruptData ? corrupt(data) : data;
    sendto(sock, payload.c_str(), payload.length(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "[UDP] Sent: " << payload << std::endl;

    if (duplicatePackets) {
        std::cout << "[UDP] Duplicate packet sent" << std::endl;
        sendto(sock, payload.c_str(), payload.length(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }
}

void UDPAnomalyClient::runTest(const std::vector<std::string>& messages) {
    for (const auto& msg : messages) {
        sendWithAnomalies(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    close(sock);
}
