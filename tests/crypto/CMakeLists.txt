cmake_minimum_required(VERSION 3.24)
project(crypto-test-app)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} crypto)
target_link_libraries(${PROJECT_NAME} GTest::GTest)
