cmake_minimum_required(VERSION 3.10)

project(server VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("source dir : ${PROJECT_SOURCE_DIR}")
message("binary dir : ${PROJECT_BINARY_DIR}")

add_definitions("-Wall -g -fPIC")

add_subdirectory(message)
add_subdirectory(net_mgr)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../dependent/boost/include
    ${PROJECT_SOURCE_DIR}/../../dependent/protobuf/include
)
link_directories(${PROJECT_SOURCE_DIR}/../../dependent/boost/lib)

add_executable(server main.cpp)

target_link_libraries(server PUBLIC pthread boost_thread net_mgr)
target_include_directories(server PUBLIC "${PROJECT_SOURCE_DIR}")
