cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(play3d)

include(FetchContent)

# I don't want to deal with this
#FetchContent_Declare(
#	notcurses
#	GIT_REPOSITORY https://github.com/dankamongmen/notcurses.git
#	GIT_TAG        bfb65c252e0764796e379595ad6e089dcb573ffe # v3.0.11
#)
#set(USE_MULTIMEDIA none)
#FetchContent_MakeAvailable(notcurses)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG        bf71a834948186f4097caa076cd2663c69a10e1e # v1.0.1
)

FetchContent_MakeAvailable(glm)

find_package(Boost 1.83.0 REQUIRED)

# TODO: make this part more robust (it works on my system!)
find_path(
	NOTCURSES_CONFIG_DIR
	"NotcursesCoreConfig.cmake"
	"NotcursesCoreConfigVersion.cmake"
	HINTS /usr/lib/x86_64-linux-gnu/cmake/NotcursesCore
	REQUIRED
)
# TODO: make version check work
#set(PACKAGE_FIND_VERSION "3.0.0") # somewhat arbitrary
#include(${NOTCURSES_VERSION_CHECK}/NotcursesCoreConfigVersion.cmake)
#if (NOT PACKAGE_VERSION_COMPATIBLE)
#	message(FATAL_ERROR "Couldn't find a compatible Notcurses library")
#endif()
include(${NOTCURSES_CONFIG_DIR}/NotcursesCoreConfig.cmake)

file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hpp)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

include(CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T LANGUAGE CXX)
if (NOT ${HAVE_SIZE_T} EQUAL TRUE)
	message(FATAL_ERROR "Could not get size of std::size_t")
endif()

add_executable(play3d main.cpp ${HEADERS} ${SOURCES})
target_compile_definitions(play3d PRIVATE SIZEOF_SIZE_T=${SIZEOF_SIZE_T})
target_include_directories(play3d PRIVATE ${Notcurses_INCLUDE_DIRS})
target_link_libraries(play3d PRIVATE ${Notcurses_LIBRARIES})
target_link_libraries(play3d PRIVATE Boost::headers)
target_link_libraries(play3d PRIVATE glm::glm)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

