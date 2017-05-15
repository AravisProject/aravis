#===============================================================================
# Set default build type for single-configuration
#-------------------------------------------------------------------------------
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type. Default: Release")
endif()

#===============================================================================
# CMake project configuration for dlm
#-------------------------------------------------------------------------------
project(aravis)

#===============================================================================
# Require a CMake version that supports object libraries
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)

# if (UNIX)
# 	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
# endif (UNIX)

#find_package(GTK2 2.6 REQUIRED)
include_directories(${GTK2_INCLUDE_DIRS})

find_path(LIBUSB1_INCLUDE_DIR libusb.h PATH_SUFFIXES libusb-1.0)
include_directories(${LIBUSB1_INCLUDE_DIR})
mark_as_advanced(LIBUSB1_INCLUDE_DIR)
find_library(LIBUSB1_LIBRARY libusb-1.0.so)
mark_as_advanced(LIBUSB1_LIBRARY)

find_library(LIBCAP_NG_LIBRARY libcap-ng.so)

#find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
mark_as_advanced(LIBXML2_INCLUDE_DIR)
mark_as_advanced(LIBXML2_LIBRARIES)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}")

file(GLOB_RECURSE aravis_srcs "src/*.c")

list(REMOVE_ITEM aravis_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/arvtool.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/arvtest.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/arvfakegvcamera.c")


#message(STATUS "${aravis_srcs}")

#SET(CMAKE_C_CREATE_SHARED_LIBRARY ON)

option(ARAVIS_ENABLE_USB		1)
option(ARAVIS_ENABLE_PACKET_SOCKET	1)

if (ARAVIS_ENABLE_USB AND LIBUSB1_LIBRARY AND LIBUSB1_INCLUDE_DIR)
	set(ARAVIS_HAS_USB	1)
else()
	set(ARAVIS_HAS_USB	0)
endif()

if (ARAVIS_ENABLE_PACKET_SOCKET AND LIBCAP_NG_LIBRARY)
	set(ARAVIS_HAS_PACKET_SOCKET	1)
else()
	set(ARAVIS_HAS_PACKET_SOCKET	0)
endif()


set(ARAVIS_MAJOR_VERSION "0")
set(ARAVIS_MINOR_VERSION "5")
set(ARAVIS_MICRO_VERSION "8")

add_definitions(-DARAVIS_API_VERSION="0.6" -DARAVIS_COMPILATION -DARAVIS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
#add_definitions(-DARAVIS_COMPILATION)

configure_file("src/arvversion.h.in" "arvversion.h")
configure_file("src/arvfeatures.h.in" "arvfeatures.h")

add_library(aravis SHARED ${aravis_srcs})
add_library(aravis-static STATIC ${aravis_srcs})

add_executable(arv-tool "${CMAKE_CURRENT_SOURCE_DIR}/src/arvtool.c")
target_link_libraries(arv-tool aravis-static)
target_link_libraries(arv-tool ${GTK2_LIBRARIES} ${LIBXML2_LIBRARIES} ${LIBUSB1_LIBRARY} ${LIBCAP_NG_LIBRARY} z m)

add_executable(arv-viewer "viewer/arvviewer.c" "viewer/main.c")
target_link_libraries(arv-viewer aravis-static)
target_link_libraries(arv-tool ${GTK2_LIBRARIES} ${LIBXML2_LIBRARIES} ${LIBUSB1_LIBRARY} ${LIBCAP_NG_LIBRARY} z m)

if (UNIX AND CMAKE_BUILD_TYPE STREQUAL "Release")
	add_custom_command(TARGET aravis POST_BUILD COMMAND ${CMAKE_STRIP}  --strip-unneeded  -R .comment -R .note -R .note.ABI-tag ${CMAKE_CURRENT_BINARY_DIR}/libaravis.so)
else()
	message(STATUS "buildtype ${CMAKE_BUILD_TYPE}")
endif()

target_link_libraries(aravis ${GTK2_LIBRARIES} ${LIBXML2_LIBRARIES} ${LIBUSB1_LIBRARY} ${LIBCAP_NG_LIBRARY})

#add_executable(arvtool ${CMAKE_CURRENT_SOURCE_DIR}/src/arvtool.c ${GTK2_LIBRARIES})
#target_link_libraries(arvtool aravis)

#add_executable(arvfakegvcamera ${CMAKE_CURRENT_SOURCE_DIR}/src/arvfakegvcamera.c ${GTK2_LIBRARIES})
#target_link_libraries(arvfakegvcamera aravis)

#add_executable(arvviewer ${CMAKE_CURRENT_SOURCE_DIR}/viewer/arvviewer.c)
#target_link_libraries(arvviewer aravis ${GTK2_LIBRARIES})
#SET_TARGET_PROPERTIES(aravis PROPERTIES LINKER_LANGUAGE C)
