
cmake_minimum_required(VERSION 3.10)


project(ExtHashProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_executable(ext_client ext_client.cpp)


include_directories(/usr/include)


link_directories(/usr/lib/x86_64-linux-gnu)


target_link_libraries(ext_client pthread gflags)


