# Compiler Flags Configuration
#
# This module sets up compiler flags for the gogglesmm project.
# It checks for compiler support and enables appropriate warning
# and optimization flags.

include(CheckCXXCompilerFlag)

# Check for supported compiler flags
check_cxx_compiler_flag(-Wall                HAS_CXX_WALL)
check_cxx_compiler_flag(-Wextra              HAS_CXX_WEXTRA)
check_cxx_compiler_flag(-Wno-format          HAS_CXX_WFORMAT)
check_cxx_compiler_flag(-Wno-format-security HAS_CXX_WFORMAT_SECURITY)
check_cxx_compiler_flag(-Wfatal-errors       HAS_CXX_WFATAL)
check_cxx_compiler_flag(-Og                  HAS_CXX_OPTIMIZE_DEBUG)

# Warning flags
if(HAS_CXX_WALL)
  add_compile_options(-Wall)
endif()

if(HAS_CXX_WEXTRA)
  add_compile_options(-Wextra)
endif()

if(HAS_CXX_WFORMAT)
  add_compile_options(-Wno-format)
endif()

if(HAS_CXX_WFORMAT_SECURITY)
  add_compile_options(-Wno-format-security)
endif()

if(HAS_CXX_WFATAL)
  add_compile_options(-Wfatal-errors)
endif()

# Debug optimization flag
if(HAS_CXX_OPTIMIZE_DEBUG AND CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_options(-Og)
endif()

# Link-time optimization (LTO)
# FIXME: Doesn't work on all platforms
# check_cxx_compiler_flag(-flto HAS_CXX_OPTIMIZE_LINKTIME)
if(HAS_CXX_OPTIMIZE_LINKTIME AND NOT CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_options(-flto)
  link_libraries(-flto)
endif()

# Build type specific definitions
if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
endif()

# Suppress FXASSERT for None build type
if(CMAKE_BUILD_TYPE MATCHES None)
  add_definitions(-DNDEBUG)
endif()
