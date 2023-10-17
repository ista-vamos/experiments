cmake_minimum_required(VERSION 3.9)
project(fase23-experiments)

set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (vamos_DIR)
	if (NOT vamos-buffers_DIR)
		set(vamos-buffers_DIR ${vamos_DIR}/vamos-buffers/cmake/vamos-buffers)
	endif()
	if (NOT vamos-compiler_DIR)
		set(vamos-compiler_DIR ${vamos_DIR}/vamos-compiler/cmake/vamos-compiler)
	endif()
	if (NOT vamos-sources_DIR)
		set(vamos-sources_DIR ${vamos_DIR}/vamos-sources/cmake/vamos-sources)
	endif()
endif()

find_package(vamos-buffers REQUIRED)

# make sure the paths are aboslute
get_filename_component(vamos-buffers_DIR ${vamos-buffers_DIR} ABSOLUTE)
get_filename_component(vamos-buffers_SRCDIR ${vamos-buffers_DIR}/../.. ABSOLUTE)

include_directories(${vamos-buffers_INCLUDE_DIRS})

get_target_property(vamos-buffers_vamos-buffers-lib vamos-buffers-lib LOCATION)
get_target_property(vamos-buffers_vamos-buffers-ringbuf vamos-buffers-ringbuf LOCATION)
get_target_property(vamos-buffers_vamos-buffers-shmbuf vamos-buffers-shmbuf LOCATION)
get_target_property(vamos-buffers_vamos-buffers-streams vamos-buffers-streams LOCATION)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_lib ${vamos-buffers_vamos-buffers-lib} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_core ${vamos-buffers_vamos-buffers-ringbuf} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_shmbuf ${vamos-buffers_vamos-buffers-shmbuf} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_streams ${vamos-buffers_vamos-buffers-streams} DIRECTORY)

if (vamos-compiler_DIR)
	# make sure the user-provided path is aboslute
	get_filename_component(vamos-compiler_DIR ${vamos-compiler_DIR} ABSOLUTE)
	get_filename_component(vamos-compiler_SRCDIR ${vamos-compiler_DIR}/../.. ABSOLUTE)
	message(STATUS "vamos-compiler_DIR=${vamos-compiler_DIR}")
else()
	message(FATAL_ERROR "Need to set vamos-compiler_DIR variable")
endif()

if (vamos-sources_DIR)
	# make sure the user-provided path is aboslute
	get_filename_component(vamos-sources_DIR ${vamos-sources_DIR} ABSOLUTE)
	get_filename_component(vamos-sources_SRCDIR ${vamos-sources_DIR}/../.. ABSOLUTE)
	message(STATUS "vamos-sources_DIR=${vamos-sources_DIR}")
else()
	message(FATAL_ERROR "Need to set vamos-sources_DIR variable")
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Makefile.config.in
	       ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.config)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup-vars.sh.in
	       ${CMAKE_CURRENT_SOURCE_DIR}/setup-vars.sh)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.py.in
	       ${CMAKE_CURRENT_SOURCE_DIR}/config.py)

add_custom_target(gen-readme
		  COMMAND pandoc -f markdown -t HTML -o README.html README.md
		  COMMAND pandoc -f markdown -t pdf -o README.pdf README.md)
