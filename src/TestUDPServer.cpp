#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sockfd;
    char buffer[1024];
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(12345);

    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    std::cout << "UDP Server listening on port 12345..." << std::endl;

    while (true) {
        socklen_t len = sizeof(cliaddr);
        memset(buffer, 0, sizeof(buffer));
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&cliaddr, &len);
        buffer[n] = '\0';
        std::cout << "[UDP <] Received: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}
