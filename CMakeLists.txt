cmake_minimum_required(VERSION 3.14)

# Read version from VERSION file
file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" SHAKYLINE_VERSION_STRING
     LIMIT_COUNT 1 REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+")
if(NOT SHAKYLINE_VERSION_STRING)
    set(SHAKYLINE_VERSION_STRING "0.0.0")
endif()
message(STATUS "ShakyLine version: ${SHAKYLINE_VERSION_STRING}")

project(shakyline VERSION ${SHAKYLINE_VERSION_STRING} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
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

target_compile_definitions(shakyline PRIVATE
    ASIO_STANDALONE
    SHAKYLINE_VERSION="${SHAKYLINE_VERSION_STRING}"
)

if(MSVC)
    target_compile_options(shakyline PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(shakyline PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

# Install
install(TARGETS shakyline RUNTIME DESTINATION bin)
install(FILES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/CHANGELOG.md"
    DESTINATION .
)

# CPack packaging
set(CPACK_PACKAGE_NAME "ShakyLine")
set(CPACK_PACKAGE_VENDOR "sarthakuwar")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Programmable Network Fault Injection Proxy")
set(CPACK_PACKAGE_VERSION "${SHAKYLINE_VERSION_STRING}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README  "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ShakyLine")
set(CPACK_PACKAGE_EXECUTABLES "shakyline" "ShakyLine")

if(WIN32)
    # ZIP archive: extract and run.
    set(CPACK_GENERATOR "ZIP;NSIS")

    # NSIS Windows installer settings
    set(CPACK_NSIS_DISPLAY_NAME "ShakyLine ${SHAKYLINE_VERSION_STRING}")
    set(CPACK_NSIS_PACKAGE_NAME "ShakyLine")
    set(CPACK_NSIS_CONTACT       "https://github.com/sarthakuwar/ShakyLine/issues")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/sarthakuwar/ShakyLine")
    set(CPACK_NSIS_HELP_LINK     "https://github.com/sarthakuwar/ShakyLine/blob/main/README.md")
    # Add the install bin dir to PATH so `shakyline` works from any terminal
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "shakyline.exe")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
