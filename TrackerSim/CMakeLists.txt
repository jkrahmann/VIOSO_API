cmake_minimum_required(VERSION 3.9)
project(TrackerSim VERSION 1.2.4 DESCRIPTION "TrackerSim")

set(THIRD_PARTY ${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty)
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MFC_FLAG 2)
add_definitions(-D_AFXDLL)

add_compile_definitions(TRACKERSIM_EXPORTS)

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_link_libraries(${PROJECT_NAME} VIOSOWarpBlend)
include_directories(${CMAKE_SOURCE_DIR}/Include)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/VIOSOWarpBlend.dll ${CMAKE_CURRENT_BINARY_DIR}/.
)