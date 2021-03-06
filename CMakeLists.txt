cmake_minimum_required(VERSION 3.9)

project(VIOSOWarpBlend VERSION 1.2.4 DESCRIPTION "VIOSOWarpBlend library")

set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/VIOSOWarpBlend/3rdparty)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4302")
set(DEFAULT_FLAGS -std=c++11 -Wall -Werror -Wno-unused-result -Wno-unknown-pragmas)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (NOT DEFINED VIOSO_API_SKIP_EXAMPLES)
	add_subdirectory(Examples)
endif()

add_subdirectory(EyePointProvider)
add_dependencies(EyePointProvider ${PROJECT_NAME})

if (NOT DEFINED VIOSO_API_SKIP_TRACKERSIM)
	add_subdirectory(TrackerSim)
	add_dependencies(TrackerSim ${PROJECT_NAME})
endif()

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
  message(ERROR "OPENGL not found!")
endif(NOT OPENGL_FOUND)

add_compile_definitions(VIOSOWARPBLEND_EXPORTS)

file(GLOB_RECURSE SOURCES "VIOSOWarpBlend/*.cpp")

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    set(ARCH_POSTFIX "")
		set(LIB_ARCH_PATH "Win32")
		set(LIB_EXTENSION "")
	else()
		set(ARCH_POSTFIX 64)
		set(LIB_ARCH_PATH "x64")
		set(LIB_EXTENSION "64")
	endif()
endif(WIN32)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

include_directories(
	${CMAKE_SOURCE_DIR}/Include
	${THIRD_PARTY}/d3dX/include/
	)

target_link_libraries (${PROJECT_NAME} 
	${THIRD_PARTY}/d3dX/lib/${LIB_ARCH_PATH}/dxerr.lib
	)

#uncompress zipped data files
if (NOT DEFINED VIOSO_API_SKIP_EXAMPLES)
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
endif()