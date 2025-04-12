cmake_minimum_required(VERSION 3.24)
project(srtp)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

find_package(OpenSSL REQUIRED)
find_package(libSRTP REQUIRED srtp3)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} memory)
target_link_libraries(${PROJECT_NAME} libSRTP::srtp3)