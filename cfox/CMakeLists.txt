cmake_minimum_required(VERSION 3.3.1)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

project(cfox VERSION 1.7.83)

option(WITH_NATIVE "Compile for native architecture" OFF)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wall                HAS_CXX_WALL)
check_cxx_compiler_flag(-Wextra              HAS_CXX_WEXTRA)
check_cxx_compiler_flag(-Wno-format          HAS_CXX_WFORMAT)
check_cxx_compiler_flag(-Wno-format-security HAS_CXX_WFORMAT_SECURITY)
check_cxx_compiler_flag(-Woverloaded-virtual HAS_CXX_WOVERLOADED_VIRTUAL)
check_cxx_compiler_flag(-Wshadow             HAS_CXX_WSHADOW)
check_cxx_compiler_flag(-Wstrict-aliasing    HAS_CXX_WSTRICT_ALIASING)
check_cxx_compiler_flag(-Wstrict-overflow    HAS_CXX_WSTRICT_OVERFLOW)
check_cxx_compiler_flag(-Wcast-qual          HAS_CXX_WCAST_QUAL)
check_cxx_compiler_flag(-Wcast-align         HAS_CXX_WCAST_ALIGN)
check_cxx_compiler_flag(-Wdouble-promotion   HAS_CXX_WDOUBLE_PROMOTION)
check_cxx_compiler_flag(-Wno-implicit-fallthrough HAX_CXX_IMPLICIT_FALLTHROUGH)

check_cxx_compiler_flag(-Wfatal-errors  HAS_CXX_WFATAL)
check_cxx_compiler_flag(-Og             HAS_CXX_OPTIMIZE_DEBUG)
check_cxx_compiler_flag(-ffast-math     HAS_CXX_FASTMATH)
check_cxx_compiler_flag(-fstrict-aliasing HAS_CXX_STRICTALIASING)
check_cxx_compiler_flag(-finline-functions HAS_CXX_INLINEFUNCTIONS)
check_cxx_compiler_flag(-fomit-frame-pointer HAS_CXX_OMITFRAMEPOINTER)
check_cxx_compiler_flag(-fstrict-overflow HAS_CXX_STRICT_OVERFLOW)
check_cxx_compiler_flag(-fstrict-aliasing HAS_CXX_STRICT_ALIASING)
check_cxx_compiler_flag(-fcf-protection=none HAS_CXX_CF_PROTECTION)
check_cxx_compiler_flag(-fno-stack-protector HAS_CXX_STACK_PROTECTOR)


if(HAS_CXX_FASTMATH)
  add_compile_options("$<$<CONFIG:Release>:-ffast-math>")
endif()

if(HAS_CXX_STRICTALIASING)
  add_compile_options("$<$<CONFIG:Release>:-fstrict-aliasing>")
endif()

if(HAS_CXX_INLINEFUNCTIONS)
  add_compile_options("$<$<CONFIG:Release>:-finline-functions>")
endif()

if(HAS_CXX_OMITFRAMEPOINTER)
  add_compile_options("$<$<CONFIG:Release>:-fomit-frame-pointer>")
endif()

if(HAS_CXX_STRICT_OVERFLOW)
  add_compile_options("$<$<CONFIG:Release>:-fstrict-overflow>")
endif()

if(HAS_CXX_STRICT_ALIASING)
  add_compile_options("$<$<CONFIG:Release>:-fstrict-aliasing>")
endif()

if(HAS_CXX_CF_PROTECTION)
  add_compile_options("$<$<CONFIG:Release>:-fcf-protection=none>")
endif()

if(HAS_CXX_STACK_PROTECTOR)
  add_compile_options("$<$<CONFIG:Release>:-fno-stack-protector>")
endif()



if(HAS_CXX_WALL)
    add_compile_options(-Wall)
endif()

if(HAS_CXX_WEXTRA)
    add_compile_options(-Wextra)
endif()

# Disable any -Wformat as FXString::format is incompatible
if(HAS_CXX_WFORMAT)
    add_compile_options(-Wno-format)
endif()

# Disable -Wformat-security as this flag won't work with passing -Wno-format
if(HAS_CXX_WFORMAT)
    add_compile_options(-Wno-format-security)
endif()

if(HAS_CXX_WFATAL)
    add_compile_options(-Wfatal-errors)
endif()

if(HAS_CXX_WSHADOW)
    add_compile_options(-Wshadow)
endif()

if(HAS_CXX_WOVERLOADED_VIRTUAL)
    add_compile_options(-Woverloaded-virtual)
endif()

if(HAX_CXX_IMPLICIT_FALLTHROUGH)
    add_compile_options(-Wno-implicit-fallthrough)
endif()

if(HAS_CXX_WSTRICT_ALIASING)
    add_compile_options(-Wstrict-aliasing)
endif()

if(HAS_CXX_WSTRICT_OVERFLOW)
    add_compile_options(-Wstrict-overflow)
endif()

if(HAS_CXX_WCAST_QUAL)
    add_compile_options(-Wcast-qual)
endif()

if(HAS_CXX_WCAST_ALIGN)
    add_compile_options(-Wcast-align)
endif()

if(HAS_CXX_WDOUBLE_PROMOTION)
    add_compile_options(-Wdouble-promotion)
endif()

if(WITH_NATIVE)
  check_cxx_compiler_flag(-march=native HAS_CXX_OPTIMIZE_NATIVE)
  if(HAS_CXX_OPTIMIZE_NATIVE)
    add_compile_options(-march=native)
  endif()
endif()

# Libraries
add_subdirectory(utils)
add_subdirectory(lib)
