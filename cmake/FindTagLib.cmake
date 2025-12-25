# FindTagLib.cmake
# Find TagLib audio metadata library
#
# This module supports two discovery methods:
#   1. CMake Config - Uses TagLibConfig.cmake if available (preferred)
#   2. pkg-config - Falls back to pkg-config as hint
#
# Output Variables:
#   TagLib_FOUND        - TRUE if TagLib was found
#   TagLib_VERSION      - Version of TagLib
#
# Output Targets:
#   TagLib::TagLib      - Imported target for TagLib

# Early return if already found
if(TARGET TagLib::TagLib)
  set(TagLib_FOUND TRUE)
  return()
endif()

# Method 1: Try CMake config first (modern method)
find_package(TagLib QUIET CONFIG)

if(TagLib_FOUND)
  message(STATUS "TagLib: Found via CMake config (version ${TagLib_VERSION})")

  # Ensure target exists (some configs may not create it)
  if(NOT TARGET TagLib::TagLib AND TARGET TagLib::tag)
    add_library(TagLib::TagLib ALIAS TagLib::tag)
  endif()

  return()
endif()

# Method 2: Fallback to pkg-config
find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(TagLib QUIET taglib)

  if(TagLib_FOUND)
    message(STATUS "TagLib: Found via pkg-config (version ${TagLib_VERSION})")

    # Create imported target from pkg-config results
    if(NOT TARGET TagLib::TagLib)
      add_library(TagLib::TagLib UNKNOWN IMPORTED)

      # Find the library file
      find_library(TagLib_LIBRARY
        NAMES tag tag_c
        HINTS ${TagLib_LIBRARY_DIRS}
      )

      if(TagLib_LIBRARY)
        set_target_properties(TagLib::TagLib PROPERTIES
          IMPORTED_LOCATION "${TagLib_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${TagLib_INCLUDE_DIRS}"
          INTERFACE_COMPILE_OPTIONS "${TagLib_CFLAGS_OTHER}"
        )

        # Add link flags if present
        if(TagLib_LDFLAGS)
          set_target_properties(TagLib::TagLib PROPERTIES
            INTERFACE_LINK_LIBRARIES "${TagLib_LDFLAGS}"
          )
        endif()
      endif()
    endif()

    return()
  endif()
endif()

# Method 3: Manual search as last resort
find_path(TagLib_INCLUDE_DIR
  NAMES taglib/tag.h
  PATH_SUFFIXES taglib
)

find_library(TagLib_LIBRARY
  NAMES tag tag_c
)

# Try to extract version
if(TagLib_INCLUDE_DIR AND EXISTS "${TagLib_INCLUDE_DIR}/taglib/taglib.h")
  file(STRINGS "${TagLib_INCLUDE_DIR}/taglib/taglib.h" TagLib_VERSION_LINE
    REGEX "^#define[ \t]+TAGLIB_VERSION[ \t]+")
  if(TagLib_VERSION_LINE)
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" TagLib_VERSION "${TagLib_VERSION_LINE}")
  endif()
endif()

# Standard error handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TagLib
  REQUIRED_VARS TagLib_LIBRARY TagLib_INCLUDE_DIR
  VERSION_VAR TagLib_VERSION
)

if(TagLib_FOUND AND NOT TARGET TagLib::TagLib)
  add_library(TagLib::TagLib UNKNOWN IMPORTED)
  set_target_properties(TagLib::TagLib PROPERTIES
    IMPORTED_LOCATION "${TagLib_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${TagLib_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(TagLib_INCLUDE_DIR TagLib_LIBRARY)
