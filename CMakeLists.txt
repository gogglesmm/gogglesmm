# Using cmake 3.2
cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

project(gogglesmm_dist)

# Honor CMAKE_EXE_LINKER_FLAGS during tests
cmake_policy(SET CMP0056 NEW)

# Need FeaureSummary
include(FeatureSummary)

# Enable for cmake debugging
# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# libdir name to allow overriding to lib32 / lib64
# set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")

# Build with internal FOX library, the OFF setting is not supported
option(WITH_CFOX "Enable Internal FOX" ON)

# Override default install to /usr
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX /usr CACHE STRING "Installation prefix used by install." FORCE)
endif()

# Override default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Specify the desired build type." FORCE)
endif()

# Status so far
message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

#-------------------------------------------------------------------------------

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wall                HAS_CXX_WALL)
check_cxx_compiler_flag(-Wextra              HAS_CXX_WEXTRA)
check_cxx_compiler_flag(-Wno-format          HAS_CXX_WFORMAT)
check_cxx_compiler_flag(-Wno-format-security HAS_CXX_WFORMAT_SECURITY)
check_cxx_compiler_flag(-Wfatal-errors       HAS_CXX_WFATAL)
check_cxx_compiler_flag(-Og                  HAS_CXX_OPTIMIZE_DEBUG)


# Additional compiler flags we like
if(HAS_CXX_WALL)
  add_compile_options(-Wall)
endif()

if(HAS_CXX_WEXTRA)
  add_compile_options(-Wextra)
endif()

if (HAS_CXX_WFORMAT)
  add_compile_options(-Wno-format)
endif()

if (HAS_CXX_WFORMAT_SECURITY)
  add_compile_options(-Wno-format-security)
endif()

if(HAS_CXX_WFATAL)
  add_compile_options(-Wfatal-errors)
endif()

if(HAS_CXX_OPTIMIZE_DEBUG AND CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_options(-Og)
endif()

# FIXME Doesn't work on all platforms.
#CHECK_CXX_COMPILER_FLAG(-flto HAS_CXX_OPTIMIZE_LINKTIME)
if(HAS_CXX_OPTIMIZE_LINKTIME AND NOT CMAKE_BUILD_TYPE MATCHES Debug)
  add_compile_options(-flto)
  link_libraries(-flto)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
endif()

# Suppress FXASSERT
if(CMAKE_BUILD_TYPE MATCHES None)
  add_definitions(-DNDEBUG)
endif()

#-------------------------------------------------------------------------------

# Internal FOX Library
if(WITH_CFOX)
  add_subdirectory(cfox)
endif()

# Add Goggles Audio Player
add_subdirectory(gap)

# Add Goggles Music Manager
add_subdirectory(src)


#-------------------------------------------------------------------------------

install(FILES icons/gogglesmm_16.png DESTINATION share/icons/hicolor/16x16/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_22.png DESTINATION share/icons/hicolor/22x22/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_24.png DESTINATION share/icons/hicolor/24x24/apps RENAME gogglesmm.png)
install(FILES icons/gogglesmm_32.png DESTINATION share/icons/hicolor/32x32/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_48.png DESTINATION share/icons/hicolor/48x48/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_64.png DESTINATION share/icons/hicolor/64x64/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm.svg DESTINATION share/icons/hicolor/scalable/apps RENAME gogglesmm.svg)
install(FILES extra/gogglesmm.appdata.xml DESTINATION share/metainfo)
install(FILES extra/gogglesmm.desktop DESTINATION share/applications)
install(FILES extra/gogglesmm.1 DESTINATION share/man/man1)

# Install translations when native language support has been enabled
if(WITH_NLS)
    set(LINGUAS "All" CACHE STRING "A space-seperated list of translations to compile in to gogglesmm, or \"None\".")
    if (LINGUAS STREQUAL "All")
        file(GLOB pofiles po/*.po)
        foreach(pofile ${pofiles})
            get_filename_component(lang ${pofile} NAME_WE)
            list(APPEND LANGUAGES ${lang})
        endforeach()
    else()
        if(NOT LINGUAS OR LINGUAS STREQUAL "None")
            set(LANGUAGES "")
        else()
            string(REGEX MATCHALL [a-zA-Z_@]+ LANGUAGES ${LINGUAS})
        endif()
    endif()

    foreach(lang ${LANGUAGES})
        install(FILES po/${lang}.mo DESTINATION share/locale/${lang}/LC_MESSAGES RENAME gogglesmm.mo)
    endforeach()
endif()

# Update the icon cache upon installation if needed
install(SCRIPT build/update_icons.cmake)

# Display Feature Summary
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Features:\n")
