cmake_minimum_required(VERSION 3.24)
project(sdp-test-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} sdp)
target_link_libraries(${PROJECT_NAME} tests-lib)
target_link_libraries(${PROJECT_NAME} GTest::GTest)
