cmake_minimum_required(VERSION 3.20)
project(tau-srtp-test-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} tau-srtp tau-crypto tau-rtp)
target_link_libraries(${PROJECT_NAME} GTest::GTest)
