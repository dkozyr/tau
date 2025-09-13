cmake_minimum_required(VERSION 3.20)
project(tau-crypto)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

set(OPENSSL_USE_STATIC_LIBS FALSE)
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIRS})
  message(STATUS "Using OpenSSL ${OPENSSL_VERSION}, include: ${OPENSSL_INCLUDE_DIRS}")
else()
  error("Error: OpenSSL no found!")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} tau-common)
target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto OpenSSL::SSL dl)