# FindDBus.cmake
# Find D-Bus message bus system
#
# This module supports two discovery methods:
#   1. CMake Config - Uses DBusConfig.cmake if available (preferred)
#   2. pkg-config - Falls back to pkg-config as hint
#
# Output Variables:
#   DBus_FOUND          - TRUE if DBus was found
#   DBus_VERSION        - Version of DBus
#
# Output Targets:
#   DBus::DBus          - Imported target for DBus (dbus-1)

# Early return if already found
if(TARGET DBus::DBus)
  set(DBus_FOUND TRUE)
  return()
endif()

# Method 1: Try CMake config first (modern method)
find_package(DBus1 QUIET CONFIG)

if(DBus1_FOUND)
  set(DBus_FOUND TRUE)
  set(DBus_VERSION ${DBus1_VERSION})
  message(STATUS "DBus: Found via CMake config (version ${DBus_VERSION})")

  # Create consistent target name
  if(NOT TARGET DBus::DBus)
    if(TARGET dbus-1)
      add_library(DBus::DBus ALIAS dbus-1)
    endif()
  endif()

  return()
endif()

# Method 2: Fallback to pkg-config
find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(DBus QUIET dbus-1)

  if(DBus_FOUND)
    message(STATUS "DBus: Found via pkg-config (version ${DBus_VERSION})")

    # Create imported target from pkg-config results
    if(NOT TARGET DBus::DBus)
      add_library(DBus::DBus UNKNOWN IMPORTED)

      # Find the library file
      find_library(DBus_LIBRARY
        NAMES dbus-1
        HINTS ${DBus_LIBRARY_DIRS}
      )

      if(DBus_LIBRARY)
        set_target_properties(DBus::DBus PROPERTIES
          IMPORTED_LOCATION "${DBus_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${DBus_INCLUDE_DIRS}"
          INTERFACE_COMPILE_OPTIONS "${DBus_CFLAGS_OTHER}"
        )

        # Add link flags if present
        if(DBus_LDFLAGS)
          set_target_properties(DBus::DBus PROPERTIES
            INTERFACE_LINK_LIBRARIES "${DBus_LDFLAGS}"
          )
        endif()
      endif()
    endif()

    return()
  endif()
endif()

# Method 3: Manual search as last resort
find_path(DBus_INCLUDE_DIR
  NAMES dbus/dbus.h
  PATH_SUFFIXES dbus-1.0
)

# DBus also has architecture-specific includes
find_path(DBus_ARCH_INCLUDE_DIR
  NAMES dbus/dbus-arch-deps.h
  PATHS
    /usr/lib/dbus-1.0/include
    /usr/lib64/dbus-1.0/include
    /usr/lib/x86_64-linux-gnu/dbus-1.0/include
    /usr/lib/i386-linux-gnu/dbus-1.0/include
)

find_library(DBus_LIBRARY
  NAMES dbus-1
)

# Try to extract version
if(DBus_INCLUDE_DIR AND EXISTS "${DBus_INCLUDE_DIR}/dbus/dbus.h")
  file(STRINGS "${DBus_INCLUDE_DIR}/dbus/dbus.h" DBus_VERSION_MAJOR_LINE
    REGEX "^#define[ \t]+DBUS_MAJOR_VERSION[ \t]+")
  file(STRINGS "${DBus_INCLUDE_DIR}/dbus/dbus.h" DBus_VERSION_MINOR_LINE
    REGEX "^#define[ \t]+DBUS_MINOR_VERSION[ \t]+")
  file(STRINGS "${DBus_INCLUDE_DIR}/dbus/dbus.h" DBus_VERSION_MICRO_LINE
    REGEX "^#define[ \t]+DBUS_MICRO_VERSION[ \t]+")

  if(DBus_VERSION_MAJOR_LINE AND DBus_VERSION_MINOR_LINE AND DBus_VERSION_MICRO_LINE)
    string(REGEX MATCH "[0-9]+" DBus_VERSION_MAJOR "${DBus_VERSION_MAJOR_LINE}")
    string(REGEX MATCH "[0-9]+" DBus_VERSION_MINOR "${DBus_VERSION_MINOR_LINE}")
    string(REGEX MATCH "[0-9]+" DBus_VERSION_MICRO "${DBus_VERSION_MICRO_LINE}")
    set(DBus_VERSION "${DBus_VERSION_MAJOR}.${DBus_VERSION_MINOR}.${DBus_VERSION_MICRO}")
  endif()
endif()

# Standard error handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBus
  REQUIRED_VARS DBus_LIBRARY DBus_INCLUDE_DIR
  VERSION_VAR DBus_VERSION
)

if(DBus_FOUND AND NOT TARGET DBus::DBus)
  add_library(DBus::DBus UNKNOWN IMPORTED)

  # Combine include directories
  set(_dbus_includes "${DBus_INCLUDE_DIR}")
  if(DBus_ARCH_INCLUDE_DIR)
    list(APPEND _dbus_includes "${DBus_ARCH_INCLUDE_DIR}")
  endif()

  set_target_properties(DBus::DBus PROPERTIES
    IMPORTED_LOCATION "${DBus_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${_dbus_includes}"
  )
endif()

mark_as_advanced(DBus_INCLUDE_DIR DBus_ARCH_INCLUDE_DIR DBus_LIBRARY)
