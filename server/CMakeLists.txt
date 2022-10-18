cmake_minimum_required(VERSION 3.10)

project(server VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("SOURCE DIR : ${PROJECT_SOURCE_DIR}")
message("BINARY DIR : ${PROJECT_BINARY_DIR}")
message("PROJECTNAME: ${PROJECT_NAME}")

add_definitions("-Wall -g -fPIC")

add_subdirectory(message)
add_subdirectory(net_mgr)

include_directories(
    ${PROJECT_SOURCE_DIR}/../../dependent/boost/include
    ${PROJECT_SOURCE_DIR}/../../dependent/protobuf/include
)
link_directories(
    ${PROJECT_SOURCE_DIR}/../../dependent/boost/lib
    ${PROJECT_SOURCE_DIR}/../../dependent/protobuf/lib
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC pthread boost_thread protobuf net_mgr message)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}")
