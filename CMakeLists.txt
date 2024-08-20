cmake_minimum_required(VERSION 3.10.0)
project(HashIndexProject CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/verona-rt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES src/hashmap.cpp)

add_executable(hash_rt ${SOURCES})

target_link_libraries(hash_rt verona_rt)

set(INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include/verona_rt/src/rt)
subdirlist(INCLUDE_CATEGORIES ${INCLUDEDIR})
foreach(INCLUDE_CATEGORY ${INCLUDE_CATEGORIES})
    target_include_directories(hash_rt PUBLIC ${INCLUDEDIR}/${INCLUDE_CATEGORY})
endforeach()

