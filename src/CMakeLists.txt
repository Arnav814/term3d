cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(play3d)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

#set(HEADERS)

#set(SOURCES)

add_executable(play3d main.cpp ${HEADERS} ${SOURCES})
target_link_libraries(play3d ${CURSES_LIBRARIES})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

