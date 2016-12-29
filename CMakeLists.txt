cmake_minimum_required(VERSION 2.6)
project(ktail)

set(SRCS
  src/main.c
  src/utils.c
  src/config.c
  src/ktail.c
)

# checks
include(CheckFunctionExists)
include(CheckCSourceCompiles)
check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(inotify_init HAVE_INOTIFY)
check_c_source_compiles("int main(void) {
  unsigned long a, b, c;
  __builtin_umull_overflow(a, b, &c);
  return 0;
}" HAVE_BUILTIN_UMULL_OVERFLOW)

# config file
configure_file(
  "${PROJECT_SOURCE_DIR}/ktail_config.in"
  "${PROJECT_BINARY_DIR}/ktail_config.h"
  )
include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -pedantic -Wall -march=native")
set(CMAKE_BUILD_TYPE "Release")

add_executable(ktail ${SRCS})
install(TARGETS ktail DESTINATION bin COMPONENT binaries)
