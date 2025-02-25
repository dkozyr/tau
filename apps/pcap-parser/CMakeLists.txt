cmake_minimum_required(VERSION 3.24)
project(pcap-parser-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} memory)
target_link_libraries(${PROJECT_NAME} rtp)
target_link_libraries(${PROJECT_NAME} pcap)
