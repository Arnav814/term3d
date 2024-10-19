cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(play3d)

set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

find_package(Boost 1.83.0 REQUIRED)

file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hpp)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

include(CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T LANGUAGE CXX)
if (NOT ${HAVE_SIZE_T} EQUAL TRUE)
	message(FATAL_ERROR "Could not get size of std::size_t")
endif()

add_executable(play3d main.cpp ${HEADERS} ${SOURCES})
target_compile_definitions(play3d PRIVATE SIZEOF_SIZE_T=${SIZEOF_SIZE_T})
target_link_libraries(play3d ${CURSES_LIBRARIES})
target_link_libraries(play3d Boost::headers)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

