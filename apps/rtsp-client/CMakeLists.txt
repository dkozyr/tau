cmake_minimum_required(VERSION 3.20)
project(rtsp-client-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} net)
target_link_libraries(${PROJECT_NAME} rtp-session)
target_link_libraries(${PROJECT_NAME} rtp-packetization)
target_link_libraries(${PROJECT_NAME} sdp)
target_link_libraries(${PROJECT_NAME} rtsp)
target_link_libraries(${PROJECT_NAME} video)
