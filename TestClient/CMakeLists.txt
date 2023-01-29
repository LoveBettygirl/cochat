cmake_minimum_required(VERSION 3.0)
project(ProxyService)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

enable_language(ASM)
set(CMAKE_ASM_COMPILER ${CMAKE_CXX_COMPILER})

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(TestClient)