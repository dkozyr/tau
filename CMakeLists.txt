cmake_minimum_required(VERSION 3.20)
project(tau)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
add_compile_definitions(PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# dependencies
include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/build_3rdparty/include)
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/build_3rdparty/lib/cmake")
set(OPENSSL_ROOT_DIR ${PROJECT_SOURCE_DIR}/build_3rdparty)

option(GCC_SANITIZER "Enter gcc sanitizer mode" OFF)
if(GCC_SANITIZER)
  add_compile_options("-fsanitize=address")
  link_libraries("-fsanitize=address")
endif()

set(Boost_USE_STATIC_LIBS ON)

add_subdirectory("${PROJECT_SOURCE_DIR}/tau")

option(TAU_BUILD_APPS "Build applications" ON)
if(TAU_BUILD_APPS)
  add_subdirectory("${PROJECT_SOURCE_DIR}/apps")
endif()

option(TAU_BUILD_TESTS "Build tests" ON)
if(TAU_BUILD_TESTS)
  add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/3rdparty/opus")
