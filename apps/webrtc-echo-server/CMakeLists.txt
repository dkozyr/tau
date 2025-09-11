cmake_minimum_required(VERSION 3.20)
project(webrtc-echo-server-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

find_package(Boost REQUIRED program_options)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} net ws webrtc)
target_link_libraries(${PROJECT_NAME} Boost::program_options)
