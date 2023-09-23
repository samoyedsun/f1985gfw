cmake_minimum_required(VERSION 3.10)

project(net_mgr)

FILE(GLOB ALL_CXX_SRCS
./*.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/../../../dependent/boost/include)
link_directories(${PROJECT_SOURCE_DIR}/../../../dependent/boost/lib)

add_library(${PROJECT_NAME} STATIC ${ALL_CXX_SRCS})

target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
