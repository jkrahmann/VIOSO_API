cmake_minimum_required(VERSION 3.9)

project(Example_DirectX9 VERSION 1.2.4 DESCRIPTION "VIOSOWarpBlend OpenGL Example")

set(THIRD_PARTY ${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty)

set(DEFAULT_FLAGS "-Wall -Werror -Wno-unused-result -Wno-unknown-pragmas")

file(GLOB_RECURSE SOURCES "*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
target_link_libraries(${PROJECT_NAME} VIOSOWarpBlend)

include_directories(${CMAKE_SOURCE_DIR}/Include)

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    set(ARCH_POSTFIX "")
		set(LIB_ARCH_PATH "x86")
	else()
		set(ARCH_POSTFIX 64)
		set(LIB_ARCH_PATH "x64")
	endif()
endif(WIN32)


link_directories(${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty/d3dX/lib/${LIB_ARCH_PATH})

#d3dx
include_directories(${THIRD_PARTY}/d3dX/include/)
target_link_libraries (${PROJECT_NAME} 
	${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty/d3dX/lib/${LIB_ARCH_PATH}/dxerr.lib
	VIOSOWarpBlend
	EyePointProvider)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/VIOSOWarpBlend.dll ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/EyePointProvider/EyePointProvider.dll ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty/d3dX/lib/${LIB_ARCH_PATH}/d3dx9.lib ${CMAKE_CURRENT_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Include
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Include ${CMAKE_BINARY_DIR}/Include
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/../../VIOSOWarpBlend/3rdparty
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty ${CMAKE_BINARY_DIR}/../../VIOSOWarpBlend/3rdparty
)
