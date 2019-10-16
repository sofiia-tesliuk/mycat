cmake_minimum_required(VERSION 3.13)
project(mycat)

set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

set(ICU_MAJOR_VER "57")
set(ICU_FULL_VERSION "${ICU_MAJOR_VER}.1")

add_executable(mycat main.cpp)

find_package(Boost COMPONENTS program_options REQUIRED)

target_link_libraries(mycat ${Boost_LIBRARIES} ${ICU_LIBRARIES})