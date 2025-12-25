# FindMAD.cmake
# Modern CMake module for finding libmad (MPEG Audio Decoder)
#
# Creates imported target: MAD::MAD
#
# Output variables:
#   MAD_FOUND          - True if MAD was found
#   MAD_VERSION        - Version string (if available)
#
# Output targets:
#   MAD::MAD           - Imported target for MAD library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_MAD QUIET mad)
endif()

# Find the library
find_library(MAD_LIBRARY
    NAMES mad
    HINTS ${PC_MAD_LIBRARY_DIRS}
)

# Find the include directory
find_path(MAD_INCLUDE_DIR
    NAMES mad.h
    HINTS ${PC_MAD_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_MAD_VERSION)
    set(MAD_VERSION ${PC_MAD_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MAD
    REQUIRED_VARS MAD_LIBRARY MAD_INCLUDE_DIR
    VERSION_VAR MAD_VERSION
)

# Create imported target
if(MAD_FOUND AND NOT TARGET MAD::MAD)
    add_library(MAD::MAD UNKNOWN IMPORTED)
    set_target_properties(MAD::MAD PROPERTIES
        IMPORTED_LOCATION "${MAD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MAD_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_MAD_CFLAGS_OTHER)
        set_target_properties(MAD::MAD PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_MAD_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(MAD_LIBRARY MAD_INCLUDE_DIR)
