# FindEpoxy.cmake
# Find libepoxy - OpenGL function pointer management library
#
# This module supports two discovery methods:
#   1. CMake Config - Uses epoxyConfig.cmake if available (preferred)
#   2. pkg-config - Falls back to pkg-config as hint
#
# Output Variables:
#   Epoxy_FOUND         - TRUE if Epoxy was found
#   Epoxy_VERSION       - Version of Epoxy
#
# Output Targets:
#   Epoxy::Epoxy        - Imported target for libepoxy

# Early return if already found
if(TARGET Epoxy::Epoxy)
  set(Epoxy_FOUND TRUE)
  return()
endif()

# Method 1: Try CMake config first (modern method)
find_package(epoxy QUIET CONFIG)

if(epoxy_FOUND)
  set(Epoxy_FOUND TRUE)
  set(Epoxy_VERSION ${epoxy_VERSION})
  message(STATUS "Epoxy: Found via CMake config (version ${Epoxy_VERSION})")

  # Create consistent target name
  if(NOT TARGET Epoxy::Epoxy)
    if(TARGET epoxy::epoxy)
      add_library(Epoxy::Epoxy ALIAS epoxy::epoxy)
    elseif(TARGET epoxy)
      add_library(Epoxy::Epoxy ALIAS epoxy)
    endif()
  endif()

  return()
endif()

# Method 2: Fallback to pkg-config
find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(Epoxy QUIET epoxy)

  if(Epoxy_FOUND)
    message(STATUS "Epoxy: Found via pkg-config (version ${Epoxy_VERSION})")

    # Create imported target from pkg-config results
    if(NOT TARGET Epoxy::Epoxy)
      add_library(Epoxy::Epoxy UNKNOWN IMPORTED)

      # Find the library file
      find_library(Epoxy_LIBRARY
        NAMES epoxy
        HINTS ${Epoxy_LIBRARY_DIRS}
      )

      if(Epoxy_LIBRARY)
        set_target_properties(Epoxy::Epoxy PROPERTIES
          IMPORTED_LOCATION "${Epoxy_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${Epoxy_INCLUDE_DIRS}"
          INTERFACE_COMPILE_OPTIONS "${Epoxy_CFLAGS_OTHER}"
        )

        # Add link flags if present
        if(Epoxy_LDFLAGS)
          set_target_properties(Epoxy::Epoxy PROPERTIES
            INTERFACE_LINK_LIBRARIES "${Epoxy_LDFLAGS}"
          )
        endif()
      endif()
    endif()

    return()
  endif()
endif()

# Method 3: Manual search as last resort
find_path(Epoxy_INCLUDE_DIR
  NAMES epoxy/gl.h epoxy/egl.h
)

find_library(Epoxy_LIBRARY
  NAMES epoxy
)

# Try to extract version
if(Epoxy_INCLUDE_DIR AND EXISTS "${Epoxy_INCLUDE_DIR}/epoxy/gl.h")
  file(STRINGS "${Epoxy_INCLUDE_DIR}/epoxy/gl.h" Epoxy_VERSION_LINE
    REGEX "^#define[ \t]+EPOXY_VERSION[ \t]+")
  if(Epoxy_VERSION_LINE)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" Epoxy_VERSION "${Epoxy_VERSION_LINE}")
  endif()
endif()

# Standard error handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Epoxy
  REQUIRED_VARS Epoxy_LIBRARY Epoxy_INCLUDE_DIR
  VERSION_VAR Epoxy_VERSION
)

if(Epoxy_FOUND AND NOT TARGET Epoxy::Epoxy)
  add_library(Epoxy::Epoxy UNKNOWN IMPORTED)
  set_target_properties(Epoxy::Epoxy PROPERTIES
    IMPORTED_LOCATION "${Epoxy_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Epoxy_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Epoxy_INCLUDE_DIR Epoxy_LIBRARY)
