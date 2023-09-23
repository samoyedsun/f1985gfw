cmake_minimum_required(VERSION 3.10)

project(message)

FILE(GLOB ALL_CXX_SRCS
./*.cc
)

include_directories(${PROJECT_SOURCE_DIR}/../../../dependent/protobuf/include)
link_directories(${PROJECT_SOURCE_DIR}/../../../dependent/protobuf/lib)

add_library(${PROJECT_NAME} STATIC ${ALL_CXX_SRCS})