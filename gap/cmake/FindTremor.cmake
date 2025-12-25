# FindTremor.cmake
# Modern CMake module for finding Tremor (fixed-point Vorbis decoder)
#
# Creates imported target: Tremor::Tremor
#
# Output variables:
#   Tremor_FOUND          - True if Tremor was found
#   Tremor_VERSION        - Version string (if available)
#
# Output targets:
#   Tremor::Tremor        - Imported target for Tremor library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_TREMOR QUIET vorbisidec>=1.2)
endif()

# Find the library
find_library(Tremor_LIBRARY
    NAMES vorbisidec
    HINTS ${PC_TREMOR_LIBRARY_DIRS}
)

# Find the include directory
find_path(Tremor_INCLUDE_DIR
    NAMES tremor/ivorbiscodec.h
    HINTS ${PC_TREMOR_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_TREMOR_VERSION)
    set(Tremor_VERSION ${PC_TREMOR_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tremor
    REQUIRED_VARS Tremor_LIBRARY Tremor_INCLUDE_DIR
    VERSION_VAR Tremor_VERSION
)

# Create imported target
if(Tremor_FOUND AND NOT TARGET Tremor::Tremor)
    add_library(Tremor::Tremor UNKNOWN IMPORTED)
    set_target_properties(Tremor::Tremor PROPERTIES
        IMPORTED_LOCATION "${Tremor_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Tremor_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_TREMOR_CFLAGS_OTHER)
        set_target_properties(Tremor::Tremor PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_TREMOR_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(Tremor_LIBRARY Tremor_INCLUDE_DIR)
