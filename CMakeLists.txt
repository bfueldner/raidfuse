cmake_minimum_required(VERSION 3.6)

project(raidfuse C CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(FUSE_DEBUG ON)
find_package(FUSE REQUIRED)

set(HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/inc/${PROJECT_NAME}/mbr.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/inc/${PROJECT_NAME}/gpt.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/inc/${PROJECT_NAME}/partition.hpp

	${CMAKE_CURRENT_SOURCE_DIR}/inc/${PROJECT_NAME}/drive.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/inc/${PROJECT_NAME}/raid.hpp
)

set(SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/src/fuse.cpp
)

if(FUSE_FOUND)
	add_definitions(${FUSE_DEFINITIONS})
	include_directories(${FUSE_INCLUDE_DIRS})

	add_executable(${PROJECT_NAME} ${HEADER} ${SOURCE})
	target_link_libraries(${PROJECT_NAME} ${FUSE_LIBRARIES})
else()
	message(FATAL_ERROR "Fuse not found!")
endif()
