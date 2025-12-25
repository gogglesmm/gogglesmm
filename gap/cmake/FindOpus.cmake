# FindOpus.cmake
# Modern CMake module for finding Opus audio codec
#
# Creates imported target: Opus::Opus
#
# Output variables:
#   Opus_FOUND          - True if Opus was found
#   Opus_VERSION        - Version string
#
# Output targets:
#   Opus::Opus          - Imported target for Opus library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_OPUS QUIET opus)
endif()

find_library(Opus_LIBRARY
    NAMES opus
    HINTS ${PC_OPUS_LIBRARY_DIRS}
)

find_path(Opus_INCLUDE_DIR
    NAMES opus/opus.h
    HINTS ${PC_OPUS_INCLUDE_DIRS}
)

if(PC_OPUS_VERSION)
    set(Opus_VERSION ${PC_OPUS_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Opus
    REQUIRED_VARS Opus_LIBRARY Opus_INCLUDE_DIR
    VERSION_VAR Opus_VERSION
)

if(Opus_FOUND AND NOT TARGET Opus::Opus)
    add_library(Opus::Opus UNKNOWN IMPORTED)
    set_target_properties(Opus::Opus PROPERTIES
        IMPORTED_LOCATION "${Opus_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Opus_INCLUDE_DIR}"
    )
    if(PC_OPUS_CFLAGS_OTHER)
        set_target_properties(Opus::Opus PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_OPUS_CFLAGS_OTHER}"
        )
    endif()
endif()

mark_as_advanced(Opus_LIBRARY Opus_INCLUDE_DIR)
