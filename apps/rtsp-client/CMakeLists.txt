cmake_minimum_required(VERSION 3.20)
project(tau-rtsp-client)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} tau-net)
target_link_libraries(${PROJECT_NAME} tau-rtp-session)
target_link_libraries(${PROJECT_NAME} tau-rtp-packetization)
target_link_libraries(${PROJECT_NAME} tau-sdp)
target_link_libraries(${PROJECT_NAME} tau-rtsp)
target_link_libraries(${PROJECT_NAME} tau-video)
