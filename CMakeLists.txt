cmake_minimum_required(VERSION 3.10.0)
project(rtmp_assistant VERSION 0.1.0 LANGUAGES C CXX)

# Specify MSVC UTF-8 encoding   
# add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
set(CMAKE_CXX_STANDARD 17)

set(3rd_Library ${CMAKE_CURRENT_LIST_DIR}/3rdparty/wpcap)

include_directories(${3rd_Library}/.. ${3rd_Library}/include)
link_directories(${3rd_Library}/lib)

add_library(get_rtmp rtmp.h rtmp.cpp)
target_include_directories(get_rtmp PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(get_rtmp PUBLIC iphlpapi wpcap)

add_executable(rtmp_assistant main.cpp)
target_link_libraries(rtmp_assistant get_rtmp)

add_subdirectory(ui)