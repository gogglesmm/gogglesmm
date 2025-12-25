# FindFAAD.cmake
# Modern CMake module for finding libfaad (Freeware Advanced Audio Decoder)
#
# Creates imported target: FAAD::FAAD
#
# Output variables:
#   FAAD_FOUND          - True if FAAD was found
#   FAAD_VERSION        - Version string (if available)
#
# Output targets:
#   FAAD::FAAD          - Imported target for FAAD library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_FAAD QUIET faad2)
endif()

# Find the library
find_library(FAAD_LIBRARY
    NAMES faad
    HINTS ${PC_FAAD_LIBRARY_DIRS}
)

# Find the include directory
find_path(FAAD_INCLUDE_DIR
    NAMES faad.h
    HINTS ${PC_FAAD_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_FAAD_VERSION)
    set(FAAD_VERSION ${PC_FAAD_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FAAD
    REQUIRED_VARS FAAD_LIBRARY FAAD_INCLUDE_DIR
    VERSION_VAR FAAD_VERSION
)

# Create imported target
if(FAAD_FOUND AND NOT TARGET FAAD::FAAD)
    add_library(FAAD::FAAD UNKNOWN IMPORTED)
    set_target_properties(FAAD::FAAD PROPERTIES
        IMPORTED_LOCATION "${FAAD_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FAAD_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_FAAD_CFLAGS_OTHER)
        set_target_properties(FAAD::FAAD PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_FAAD_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(FAAD_LIBRARY FAAD_INCLUDE_DIR)
