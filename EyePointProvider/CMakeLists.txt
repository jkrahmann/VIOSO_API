cmake_minimum_required(VERSION 3.9)
project(EyePointProvider VERSION 1.2.4 DESCRIPTION "EyeProvider")

set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/../VIOSOWarpBlend/3rdparty)
set(CMAKE_CXX_STANDARD 11)
add_compile_definitions(EYEPOINTPROVIDER_EXPORTS)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} VIOSOWarpBlend)

include_directories(${CMAKE_SOURCE_DIR}/Include)