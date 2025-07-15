#include "../include/UDPAnomalyClient.hpp"
#include <getopt.h>
#include <iostream>
using namespace std;


void usage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "  -l <loss_rate>      Packet loss rate (0.0 to 1.0)\n"
         << "  -c                  Corrupt data\n"
         << "  -d                  Duplicate packets\n"
         << "  -t <ms>             Delay in milliseconds\n"
         << "  -h <host>           Server hostname (default: 127.0.0.1)\n"
         << "  -p <port>           Server port (default: 12345)\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    float loss = 0.0;
    bool corrupt = false, dup = false;
    int delay = 0;
    const char* host = "127.0.0.1";
    int port = 12345;

    int opt;
    while ((opt = getopt(argc, argv, "l:cdt:h:p:")) != -1) {
        switch (opt) {
            case 'l': loss = atof(optarg); break;
            case 'c': corrupt = true; break;
            case 'd': dup = true; break;
            case 't': delay = atoi(optarg); break;
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            default: usage(argv[0]);
        }
    }

    UDPAnomalyClient client(host, port);
    client.configure(loss, corrupt, dup, delay);
    client.runTest({"Hello", "UDP Anomaly", "Test Complete"});
    return 0;
}
