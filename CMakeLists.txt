cmake_minimum_required(VERSION 3.10.0)
project(HashIndexProject CXX)
set(CMAKE_CXX_STANDARD 17)

# 添加选项来禁用测试
set(VERONA_RT_ONLY_HEADER_LIBRARY ON CACHE BOOL "Only build header library")

# 添加 verona-rt
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/verona-rt)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GFLAGS_INCLUDE_DIRS}
)

set(SOURCES util/util.hpp src/hash_main.cpp)
# set(SOURCES util/util.hpp src/hash_rt.cpp)

find_package(gflags REQUIRED)

add_executable(hash_rt ${SOURCES})

target_link_libraries(hash_rt PUBLIC verona_rt PUBLIC ${GFLAGS_LIBRARIES} INTERFACE snmalloc-new-override)


set(INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include/verona_rt/src/rt)
subdirlist(INCLUDE_CATEGORIES ${INCLUDEDIR})
foreach(INCLUDE_CATEGORY ${INCLUDE_CATEGORIES})
    target_include_directories(hash_rt PUBLIC ${INCLUDEDIR}/${INCLUDE_CATEGORY})
endforeach()

