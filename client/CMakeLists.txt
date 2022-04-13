cmake_minimum_required(VERSION 3.10)

project(client VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("SOURCE DIR : ${PROJECT_SOURCE_DIR}")
message("BINARY DIR : ${PROJECT_BINARY_DIR}")

add_definitions("-Wall -g -fPIC")

add_subdirectory(message)

include_directories(${PROJECT_SOURCE_DIR}/../../dependent/boost/include)
link_directories(${PROJECT_SOURCE_DIR}/../../dependent/boost/lib)

add_executable(client main.cpp)

target_link_libraries(client PUBLIC pthread boost_thread)
