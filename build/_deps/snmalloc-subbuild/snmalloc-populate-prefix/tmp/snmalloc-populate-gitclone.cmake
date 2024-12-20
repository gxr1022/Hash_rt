
if(NOT "/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitinfo.txt" IS_NEWER_THAN "/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitclone-lastrun.txt")
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: '/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/users/Xuran/hash_rt/build/_deps/snmalloc-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/users/Xuran/hash_rt/build/_deps/snmalloc-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"  clone --no-checkout --config "advice.detachedHead=false" "https://github.com/microsoft/snmalloc" "snmalloc-src"
    WORKING_DIRECTORY "/users/Xuran/hash_rt/build/_deps"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/microsoft/snmalloc'")
endif()

execute_process(
  COMMAND "/usr/bin/git"  checkout d8f174c717500a834229da5f1f6bfe888442e81a --
  WORKING_DIRECTORY "/users/Xuran/hash_rt/build/_deps/snmalloc-src"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'd8f174c717500a834229da5f1f6bfe888442e81a'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git"  submodule update --recursive --init 
    WORKING_DIRECTORY "/users/Xuran/hash_rt/build/_deps/snmalloc-src"
    RESULT_VARIABLE error_code
    )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/users/Xuran/hash_rt/build/_deps/snmalloc-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitinfo.txt"
    "/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/users/Xuran/hash_rt/build/_deps/snmalloc-subbuild/snmalloc-populate-prefix/src/snmalloc-populate-stamp/snmalloc-populate-gitclone-lastrun.txt'")
endif()

