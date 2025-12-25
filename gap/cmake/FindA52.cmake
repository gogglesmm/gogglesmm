# FindA52.cmake
# Modern CMake module for finding liba52 (ATSC A/52 stream decoder)
#
# Creates imported target: A52::A52
#
# Output variables:
#   A52_FOUND          - True if A52 was found
#   A52_VERSION        - Version string (if available)
#
# Output targets:
#   A52::A52           - Imported target for A52 library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_A52 QUIET liba52)
endif()

# Find the library
find_library(A52_LIBRARY
    NAMES a52
    HINTS ${PC_A52_LIBRARY_DIRS}
)

# Find the include directory
# Note: a52 uses a52dec/a52.h path
find_path(A52_INCLUDE_DIR
    NAMES a52dec/a52.h
    HINTS ${PC_A52_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_A52_VERSION)
    set(A52_VERSION ${PC_A52_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(A52
    REQUIRED_VARS A52_LIBRARY A52_INCLUDE_DIR
    VERSION_VAR A52_VERSION
)

# Create imported target
if(A52_FOUND AND NOT TARGET A52::A52)
    add_library(A52::A52 UNKNOWN IMPORTED)
    set_target_properties(A52::A52 PROPERTIES
        IMPORTED_LOCATION "${A52_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${A52_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_A52_CFLAGS_OTHER)
        set_target_properties(A52::A52 PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_A52_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(A52_LIBRARY A52_INCLUDE_DIR)
