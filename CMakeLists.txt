cmake_minimum_required(VERSION 3.10)
project(anomaly_client)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(anomaly_tcp
    src/TCPAnomalyClient.cpp
    src/main_tcp.cpp
)

add_executable(anomaly_udp
    src/UDPAnomalyClient.cpp
    src/main_udp.cpp
)

add_executable(anomaly_tcp_server src/TestServer.cpp)
add_executable(anomaly_udp_server src/TestUDPServer.cpp)
add_executable(anomaly_threaded_tcp_server src/ThreadedTCPServer.cpp)

install(TARGETS anomaly_tcp anomaly_udp anomaly_tcp_server anomaly_udp_server anomaly_threaded_tcp_server
    RUNTIME DESTINATION bin
)
