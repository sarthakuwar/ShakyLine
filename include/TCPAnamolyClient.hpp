#ifndef TCP_ANOMALY_CLIENT_HPP
#define TCP_ANOMALY_CLIENT_HPP

#include <vector>
#include <string>
#include <netinet/in.h>

class TCPAnomalyClient {
public:
    TCPAnomalyClient(const char* host, int port);
    void configure(float loss, bool corrupt, bool dup, bool outOfOrder, int delay, bool handshakeFail);
    void runTest(const std::vector<std::string>& messages);

private:
    int sock;
    struct sockaddr_in serverAddr;

    float packetLossRate;
    bool corruptData, duplicatePackets, sendOutOfOrder, handshakeOverride;
    int delayMs;

    void connectToServer();
    void performHandshake();
    void sendWithAnomalies(const std::string& data);
    std::string corrupt(const std::string& data);
};

#endif
