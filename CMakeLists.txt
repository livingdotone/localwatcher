cmake_minimum_required(VERSION 3.30)
project(localwatcher)

set(CMAKE_CXX_STANDARD 20)

include_directories("include")
include_directories("src")

find_package(spdlog REQUIRED)

add_executable(localwatcher src/main.cpp
        include/process_collector.h
        src/process_collector.cpp
        include/logger.h
        src/logger.cpp
)

target_link_libraries(localwatcher PRIVATE spdlog::spdlog)
