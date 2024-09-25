cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(atcpp)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

set(HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/boardArea.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/commandArea.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/common.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/statusArea.hpp
)

set(SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/boardArea.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/commandArea.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/common.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/statusArea.cpp
)

add_executable(atcpp main.cpp ${HEADERS} ${SOURCES})
target_link_libraries(atcpp ${CURSES_LIBRARIES})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

