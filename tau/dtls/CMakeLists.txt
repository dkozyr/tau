cmake_minimum_required(VERSION 3.24)
project(dtls)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} common)
target_link_libraries(${PROJECT_NAME} crypto)
