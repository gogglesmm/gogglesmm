# FindSndio.cmake
# Modern CMake module for finding sndio (OpenBSD audio/MIDI framework)
#
# Creates imported target: Sndio::Sndio
#
# Output variables:
#   Sndio_FOUND          - True if Sndio was found
#   Sndio_VERSION        - Version string (if available)
#
# Output targets:
#   Sndio::Sndio         - Imported target for Sndio library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_SNDIO QUIET sndio)
endif()

# Find the library
find_library(Sndio_LIBRARY
    NAMES sndio
    HINTS ${PC_SNDIO_LIBRARY_DIRS}
)

# Find the include directory
find_path(Sndio_INCLUDE_DIR
    NAMES sndio.h
    HINTS ${PC_SNDIO_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_SNDIO_VERSION)
    set(Sndio_VERSION ${PC_SNDIO_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sndio
    REQUIRED_VARS Sndio_LIBRARY Sndio_INCLUDE_DIR
    VERSION_VAR Sndio_VERSION
)

# Create imported target
if(Sndio_FOUND AND NOT TARGET Sndio::Sndio)
    add_library(Sndio::Sndio UNKNOWN IMPORTED)
    set_target_properties(Sndio::Sndio PROPERTIES
        IMPORTED_LOCATION "${Sndio_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Sndio_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_SNDIO_CFLAGS_OTHER)
        set_target_properties(Sndio::Sndio PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_SNDIO_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(Sndio_LIBRARY Sndio_INCLUDE_DIR)
