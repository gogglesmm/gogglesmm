# FindFLAC.cmake
# Modern CMake module for finding FLAC (Free Lossless Audio Codec)
#
# Creates imported target: FLAC::FLAC
#
# Output variables:
#   FLAC_FOUND          - True if FLAC was found
#   FLAC_VERSION        - Version string
#
# Output targets:
#   FLAC::FLAC          - Imported target for FLAC library

# Strategy:
# 1. Try FLAC's official CMake config (if installed)
# 2. Fall back to pkg-config search for older systems
# 3. Use existing FLAC::FLAC target from config, or create if using pkg-config

# First, try FLAC's official CMake config package
# This is provided by newer FLAC installations (typically in /usr/lib/cmake/FLAC)
find_package(FLAC QUIET CONFIG)

# If official config was found, it creates FLAC::FLAC target and sets component found variables
if(TARGET FLAC::FLAC)
    set(FLAC_FOUND TRUE)

    # Extract version from FLAC include directory if available
    get_target_property(_flac_include_dir FLAC::FLAC INTERFACE_INCLUDE_DIRECTORIES)
    if(_flac_include_dir AND EXISTS "${_flac_include_dir}/FLAC/format.h")
        file(STRINGS "${_flac_include_dir}/FLAC/format.h" _flac_version_lines
             REGEX "^#define[ \\t]+FLAC__VERSION_STRING[ \\t]+")
        if(_flac_version_lines)
            string(REGEX MATCH "\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _ "${_flac_version_lines}")
            set(FLAC_VERSION "${CMAKE_MATCH_1}")
        endif()
        unset(_flac_version_lines)
    endif()
    unset(_flac_include_dir)

    message(STATUS "Found FLAC via CMake config: ${FLAC_VERSION}")
else()
    # Fallback to pkg-config search for older systems without CMake config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_FLAC QUIET flac)
    endif()

    find_library(FLAC_LIBRARY
        NAMES FLAC flac
        HINTS ${PC_FLAC_LIBRARY_DIRS}
    )

    find_path(FLAC_INCLUDE_DIR
        NAMES FLAC/stream_decoder.h
        HINTS ${PC_FLAC_INCLUDE_DIRS}
    )

    if(PC_FLAC_VERSION)
        set(FLAC_VERSION ${PC_FLAC_VERSION})
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(FLAC
        REQUIRED_VARS FLAC_LIBRARY FLAC_INCLUDE_DIR
        VERSION_VAR FLAC_VERSION
    )

    # Create imported target
    if(FLAC_FOUND AND NOT TARGET FLAC::FLAC)
        add_library(FLAC::FLAC UNKNOWN IMPORTED)
        set_target_properties(FLAC::FLAC PROPERTIES
            IMPORTED_LOCATION "${FLAC_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIR}"
        )
        if(PC_FLAC_CFLAGS_OTHER)
            set_target_properties(FLAC::FLAC PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${PC_FLAC_CFLAGS_OTHER}"
            )
        endif()
    endif()

    mark_as_advanced(FLAC_LIBRARY FLAC_INCLUDE_DIR)
endif()
