cmake_minimum_required(VERSION 3.20)
project(tau-pcap-parser-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

find_package(Boost REQUIRED program_options)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} tau-memory)
target_link_libraries(${PROJECT_NAME} tau-rtp tau-rtp-packetization tau-rtp-session)
target_link_libraries(${PROJECT_NAME} Boost::program_options)
target_link_libraries(${PROJECT_NAME} pcap)
