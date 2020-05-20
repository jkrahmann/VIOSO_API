cmake_minimum_required(VERSION 3.9)

project(VIOSOWarpBlend VERSION 1.2.4 DESCRIPTION "VIOSOWarpBlend library")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../QT_GUI/libs/cmake/Modules)

set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/VIOSOWarpBlend/3rdparty)

add_subdirectory(Examples)
add_subdirectory(EyePointProvider)
add_dependencies(VWB_ExampleGL ${PROJECT_NAME})
add_dependencies(EyePointProvider ${PROJECT_NAME})

find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
  message(ERROR "OPENGL not found!")
endif(NOT OPENGL_FOUND)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4302")
set(DEFAULT_FLAGS -std=c++11 -Wall -Werror -Wno-unused-result -Wno-unknown-pragmas)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

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
#set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
#set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}${LIB_EXTENSION})

#GLEW
include_directories(${THIRD_PARTY}/GLEW/include/GL)
target_link_libraries (${PROJECT_NAME} ${THIRD_PARTY}/GLEW/lib/${LIB_ARCH_PATH}/glew32.lib)

#freeglut
include_directories(${THIRD_PARTY}/freeglut/include/GL)
target_link_libraries (${PROJECT_NAME} ${THIRD_PARTY}/freeglut/lib/${LIB_ARCH_PATH}/freeglut.lib)
#add_definitions(${GLUT_DEFINITIONS})endif(NOT GLUT_FOUND)

#speasy
set(SPEASY_ROOT ${CMAKE_SOURCE_DIR}/..)
include(speasy)

#d3dx
include_directories(${THIRD_PARTY}/d3dX/include/)
target_link_libraries (${PROJECT_NAME} ${THIRD_PARTY}/d3dX/lib/${LIB_ARCH_PATH}/d3dx9.lib)
target_link_libraries (${PROJECT_NAME} ${THIRD_PARTY}/d3dX/lib/${LIB_ARCH_PATH}/d3dx10.lib)
target_link_libraries (${PROJECT_NAME} ${THIRD_PARTY}/d3dX/lib/${LIB_ARCH_PATH}/dxerr.lib)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.dll ${CMAKE_CURRENT_BINARY_DIR}/Examples/OpenGL
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.dll ${CMAKE_CURRENT_BINARY_DIR}/../QT_GUI/WarpingTool
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.dll ${CMAKE_CURRENT_BINARY_DIR}/../QT_GUI/ProjectDesigner
)