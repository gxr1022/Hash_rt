# Install script for directory: /mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/aal")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/ds")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/override")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/backend")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/mem")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE DIRECTORY FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/pal")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc/test" TYPE FILE FILES
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/test/measuretime.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/test/opt.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/test/setup.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/test/usage.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/test/xoroshiro.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/snmalloc" TYPE FILE FILES
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/snmalloc.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/snmalloc_core.h"
    "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-src/src/snmalloc/snmalloc_front.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/snmalloc/snmalloc-config.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/snmalloc/snmalloc-config.cmake"
         "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-build/CMakeFiles/Export/share/snmalloc/snmalloc-config.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/snmalloc/snmalloc-config-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/snmalloc/snmalloc-config.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/snmalloc" TYPE FILE FILES "/mnt/nvme0/home/gxr/hash_rt/build/_deps/snmalloc-build/CMakeFiles/Export/share/snmalloc/snmalloc-config.cmake")
endif()

