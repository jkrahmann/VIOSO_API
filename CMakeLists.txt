cmake_minimum_required(VERSION 3.9)

project(VIOSOWarpBlend VERSION 1.2.4 DESCRIPTION "VIOSOWarpBlend library")

set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/VIOSOWarpBlend/3rdparty)
if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4302")
endif()
if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-multichar")
	set(OpenGL_GL_PREFERENCE LEGACY)
endif()

set(DEFAULT_FLAGS -Wall -Werror -Wno-unused-result -Wno-unknown-pragmas)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED VIOSO_API_SKIP_EXAMPLES)
	add_subdirectory(Examples)
endif()

add_subdirectory(EyePointProvider)
add_dependencies(EyePointProvider ${PROJECT_NAME})

if (WIN32)
	if (NOT DEFINED VIOSO_API_SKIP_TRACKERSIM)
		add_subdirectory(TrackerSim)
		add_dependencies(TrackerSim ${PROJECT_NAME})
	endif()
endif()

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
  message(ERROR "OPENGL not found!")
endif(NOT OPENGL_FOUND)

add_compile_definitions(VIOSOWARPBLEND_EXPORTS)

file(GLOB SOURCES "VIOSOWarpBlend/*.cpp")
file(GLOB SOURCES_DX "VIOSOWarpBlend/DX/*.cpp")
file(GLOB SOURCES_GL "VIOSOWarpBlend/GL/*.cpp")

if (WIN32)
	set(ALL_SRC ${SOURCES} ${SOURCES_DX} ${SOURCES_GL})
else()
	set(ALL_SRC ${SOURCES} ${SOURCES_GL} VIOSOWarpBlend/linux_compat.cpp)
endif()

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    set(ARCH_POSTFIX "")
		set(LIB_ARCH_PATH "x86")
		set(LIB_EXTENSION "")
	else()
		set(ARCH_POSTFIX 64)
		set(LIB_ARCH_PATH "x64")
		set(LIB_EXTENSION "64")
	endif()
endif(WIN32)

add_library(${PROJECT_NAME} SHARED ${ALL_SRC})

include_directories(
	${CMAKE_SOURCE_DIR}/Include
	${CMAKE_SOURCE_DIR}/VIOSOWarpBlend
	${CMAKE_SOURCE_DIR}/VIOSOWarpBlend/3rdparty/tinyxml2
	${THIRD_PARTY}/d3dX/include/
	)
message(${CMAKE_SOURCE_DIR}/VIOSOWarpBlend)

if (WIN32)
target_link_libraries (${PROJECT_NAME}
	${THIRD_PARTY}/d3dX/lib/${LIB_ARCH_PATH}/dxerr.lib
	)
endif()

#uncompress zipped data files
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/data/vioso3D.vwf)
	message("extracting vioso3D.zip")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xzf vioso3D.zip
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
	)
endif()

#uncompress zipped data files
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/data/vioso2d.vwf)
	message("extracting vioso2d.zip")
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E tar xzf vioso2d.zip
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data
	)
endif()
