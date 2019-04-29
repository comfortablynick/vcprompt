# create config.h 

include(CheckIncludeFiles)
include(CheckTypeSize)
include(CheckFunctionExists)

# check system includes
check_include_file(arpa/inet.h HAVE_ARPA_INET_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(vfork.h HAVE_VFORK_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sqlite3.h HAVE_SQLITE3_H)

# check types
check_type_size(mode_t MODE_T)
check_type_size(pid_t PID_T)
check_type_size(size_t SIZE_T)
check_type_size(ssize_t SSIZE_T)
check_type_size(uint32_t UINT32_T)

# check functions
check_function_exists(malloc HAVE_MALLOC)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(fork HAVE_FORK)
check_function_exists(vfork HAVE_VFORK)

# define other vars in config.h
set(PACKAGE_BUGREPORT "comfortablynick@gmail.com")
set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PACKAGE_TARNAME ${PROJECT_NAME})
set(PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
set(PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
