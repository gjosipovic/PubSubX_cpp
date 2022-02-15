cmake_minimum_required(VERSION 3.0.0)
project(PubSubX_cpp VERSION 0.1.0)

find_package (Threads)

include(CTest)
enable_testing()

add_executable(PubSubX_cpp main.cpp Client.cpp)

target_link_libraries (PubSubX_cpp ${CMAKE_THREAD_LIBS_INIT})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
