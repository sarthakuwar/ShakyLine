cmake_minimum_required(VERSION 3.14)
project(shakyline VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Standalone Asio via FetchContent
include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)
FetchContent_MakeAvailable(asio)

# Platform-specific definitions
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# Main executable
add_executable(shakyline
    src/main.cpp
    src/Socket.cpp
    src/Buffer.cpp
    src/DelayQueue.cpp
    src/EventLoop.cpp
    src/Scheduler.cpp
    src/Session.cpp
    src/SessionManager.cpp
    src/ProxyServer.cpp
    src/AnomalyEngine.cpp
    src/Config.cpp
    src/MetricsRegistry.cpp
    src/ControlServer.cpp
    src/Logger.cpp
)

target_include_directories(shakyline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(shakyline PRIVATE ASIO_STANDALONE)

if(MSVC)
    target_compile_options(shakyline PRIVATE /W4 /WX)
else()
    target_compile_options(shakyline PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

# Install
install(TARGETS shakyline RUNTIME DESTINATION bin)
