cmake_minimum_required(VERSION 3.22)

project(NetworkExcersise VERSION 1.0 DESCRIPTION "This is for lab10." LANGUAGES C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(server server.c)
add_executable(client client.c)

target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)
