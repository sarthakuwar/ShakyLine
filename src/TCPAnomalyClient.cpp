#include "../include/TCPAnamolyClient.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <cstdlib>
#include <ctime>

TCPAnomalyClient::TCPAnomalyClient(const char* host, int port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
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

void TCPAnomalyClient::configure(float loss, bool corrupt, bool dup, bool outOfOrder, int delay, bool handshakeFail) {
    packetLossRate = loss;
    corruptData = corrupt;
    duplicatePackets = dup;
    sendOutOfOrder = outOfOrder;
    delayMs = delay;
    handshakeOverride = handshakeFail;
}

void TCPAnomalyClient::connectToServer() {
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "[+] Connected to server." << std::endl;
}

std::string TCPAnomalyClient::corrupt(const std::string& data) {
    std::string modified = data;
    if (!modified.empty()) {
        int index = rand() % modified.size();
        modified[index] = '!';
    }
    return modified;
}

void TCPAnomalyClient::sendWithAnomalies(const std::string& data) {
    if (delayMs > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

    if (((float) rand() / RAND_MAX) < packetLossRate) {
        std::cout << "[!] Packet dropped (simulated): " << data << std::endl;
        return;
    }

    std::string payload = corruptData ? corrupt(data) : data;
    send(sock, payload.c_str(), payload.length(), 0);
    std::cout << "[>] Sent: " << payload << std::endl;

    if (duplicatePackets) {
        std::cout << "[!] Duplicate packet sent" << std::endl;
        send(sock, payload.c_str(), payload.length(), 0);
    }
}

void TCPAnomalyClient::performHandshake() {
    if (handshakeOverride) {
        sendWithAnomalies("ACK-CUSTOM");
        return;
    }

    sendWithAnomalies("SYN");
    char buffer[1024];
    recv(sock, buffer, sizeof(buffer), 0);
    std::cout << "[<] Server: " << buffer << std::endl;
    sendWithAnomalies("ACK-CUSTOM");
}

void TCPAnomalyClient::runTest(const std::vector<std::string>& messages) {
    connectToServer();
    performHandshake();

    if (sendOutOfOrder) {
        for (const auto& msg : messages) {
            std::thread([this, msg]() {
                sendWithAnomalies(msg);
            }).detach();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        for (const auto& msg : messages) {
            sendWithAnomalies(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    close(sock);
}