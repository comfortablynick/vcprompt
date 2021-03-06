cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME vcprompt)

project(${PROJECT_NAME}
        VERSION 1.1
        DESCRIPTION "VCS status for your shell prompt"
        HOMEPAGE_URL "github.com/comfortablynick/vcprompt"
        LANGUAGES C)

set(BIN_DIR $ENV{HOME}/.local/bin)
include(cmake/build_safeguards.cmake)

# print cmake/system details
message(STATUS "CMake version:    ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Compiler:         ${CMAKE_CXX_COMPILER}")
message(STATUS "Operating System: ${CMAKE_SYSTEM}")
message(STATUS "Processor:        ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "Build Type:       ${CMAKE_BUILD_TYPE}")

# compile
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

include(cmake/sqlite3.cmake)

message(STATUS "Running configure")
include(cmake/configure.cmake)
configure_file("${PROJECT_SOURCE_DIR}/cmake/config.h.cmake"
               "${PROJECT_SOURCE_DIR}/config.h" @ONLY)
message(STATUS "Running configure - done")

string(APPEND CMAKE_C_FLAGS " --std=gnu11 -Wall -Wextra")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")

set(SOURCE_FILES
    src/vcprompt.c
    src/svn.h
    src/svn.c
    src/hg.h
    src/hg.c
    src/git.h
    src/git.c
    src/fossil.h
    src/fossil.c
    src/cvs.h
    src/cvs.c
    src/common.h
    src/common.c
    src/capture.h
    src/capture.c)

# renamed executable (no conflict with original vcprompt)
set(MAIN_TARGET_NAME "vctest")
configure_file("${PROJECT_SOURCE_DIR}/vcprompt-hgst" ${BIN_DIR} COPYONLY)

add_executable(${MAIN_TARGET_NAME} ${SOURCE_FILES})
target_compile_options(vctest PUBLIC -g)
target_link_libraries(vctest sqlite3)

include(cmake/iwyu.cmake)
install(TARGETS ${MAIN_TARGET_NAME} DESTINATION $ENV{HOME}/.local/bin)
