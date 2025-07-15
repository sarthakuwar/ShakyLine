#ifndef UDP_ANOMALY_CLIENT_HPP
#define UDP_ANOMALY_CLIENT_HPP

#include <string>
#include <vector>
#include <netinet/in.h>

class UDPAnomalyClient {
public:
    UDPAnomalyClient(const char* host, int port);

    // Make sure this matches exactly
    void configure(float loss, bool corrupt, bool dup, int delay);

    void runTest(const std::vector<std::string>& messages);

private:
    int sock;
    struct sockaddr_in serverAddr;

    float packetLossRate;
    bool corruptData, duplicatePackets;
    int delayMs;

    void sendWithAnomalies(const std::string& data);
    std::string corrupt(const std::string& data);
};

#endif
