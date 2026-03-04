cmake_minimum_required(VERSION 3.16)
project(t2-installer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

file(GLOB HEADERS "include/*.h")

file(GLOB_RECURSE SOURCES "src/*.cpp")

include_directories(include)

find_program(CLANG_FORMAT_EXE clang-format)

add_executable(t2-installer ${SOURCES} ${HEADERS}) 

target_link_libraries(t2-installer Qt6::Widgets)
