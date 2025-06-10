cmake_minimum_required(VERSION 3.10)
project(GamblingProject2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create executable
add_executable(server
        main.cpp
        deps/sqlite-amalgamation-3500100/sqlite3.c
)

# Include path for Asio standalone
target_include_directories(server PRIVATE
        ${CMAKE_SOURCE_DIR}/deps/asio/asio/include
        ${CMAKE_SOURCE_DIR}/deps/sqlite-amalgamation-3500100
        ${CMAKE_SOURCE_DIR}/deps/jwt-cpp-v0.7.1/include
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32 mswsock)
elseif(APPLE)
    target_link_libraries(server PRIVATE pthread)
    # Optional: Add framework if needed
    # target_link_libraries(server PRIVATE "-framework CoreFoundation")
elseif(UNIX)
    target_link_libraries(server PRIVATE pthread)
endif()

# Define ASIO_STANDALONE to use standalone version
target_compile_definitions(server PRIVATE ASIO_STANDALONE)

# file(COPY ${CMAKE_SOURCE_DIR}/static DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(TARGET server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/static
        ${CMAKE_BINARY_DIR}/static
)

find_package(OpenSSL REQUIRED)

target_include_directories(server PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)

