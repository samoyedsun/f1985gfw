cmake_minimum_required(VERSION 3.10)

project(server VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

message("source dir : ${PROJECT_SOURCE_DIR}")
message("binary dir : ${PROJECT_BINARY_DIR}")

add_definitions("-Wall -g -fPIC")

include_directories(${PROJECT_SOURCE_DIR}/../../boost_1_78_0)
link_directories(${PROJECT_SOURCE_DIR}/../../boost_1_78_0/stage/lib)

add_library(net_mgr net_mgr.cpp)
target_link_libraries(net_mgr PUBLIC pthread)

add_executable(server main.cpp)

target_link_libraries(server PUBLIC pthread net_mgr)
target_include_directories(server PUBLIC
                            "${PROJECT_SOURCE_DIR}"
                        )
