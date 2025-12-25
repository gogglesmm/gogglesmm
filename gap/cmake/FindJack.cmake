# FindJack.cmake
# Modern CMake module for finding JACK Audio Connection Kit
#
# Creates imported target: Jack::Jack
#
# Output variables:
#   Jack_FOUND          - True if JACK was found
#   Jack_VERSION        - Version string
#
# Output targets:
#   Jack::Jack          - Imported target for JACK library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_JACK QUIET jack)
endif()

find_library(Jack_LIBRARY
    NAMES jack
    HINTS ${PC_JACK_LIBRARY_DIRS}
)

find_path(Jack_INCLUDE_DIR
    NAMES jack/jack.h
    HINTS ${PC_JACK_INCLUDE_DIRS}
)

if(PC_JACK_VERSION)
    set(Jack_VERSION ${PC_JACK_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jack
    REQUIRED_VARS Jack_LIBRARY Jack_INCLUDE_DIR
    VERSION_VAR Jack_VERSION
)

if(Jack_FOUND AND NOT TARGET Jack::Jack)
    add_library(Jack::Jack UNKNOWN IMPORTED)
    set_target_properties(Jack::Jack PROPERTIES
        IMPORTED_LOCATION "${Jack_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Jack_INCLUDE_DIR}"
    )
    if(PC_JACK_CFLAGS_OTHER)
        set_target_properties(Jack::Jack PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_JACK_CFLAGS_OTHER}"
        )
    endif()
endif()

mark_as_advanced(Jack_LIBRARY Jack_INCLUDE_DIR)
