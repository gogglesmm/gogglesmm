# FindOgg.cmake
# Modern CMake module for finding Ogg container format library
#
# Creates imported target: Ogg::Ogg
#
# Output variables:
#   Ogg_FOUND          - True if Ogg was found
#   Ogg_VERSION        - Version string
#
# Output targets:
#   Ogg::Ogg           - Imported target for Ogg library

# Strategy:
# 1. Try Ogg's official CMake config (if installed)
# 2. Fall back to pkg-config search for older systems
# 3. Create Ogg::Ogg target (alias to Ogg::ogg if CONFIG found, or create new if not)

# First, try Ogg's official CMake config package
# This is provided by newer Ogg installations (typically in /usr/lib/cmake/Ogg)
find_package(Ogg QUIET CONFIG)

# If official config was found, it creates Ogg::ogg target and sets OGG_FOUND
# Map to our preferred naming for consistency
if(OGG_FOUND AND TARGET Ogg::ogg)
    set(Ogg_FOUND TRUE)

    # Extract version from OGG_INCLUDE_DIRS if available
    get_target_property(_ogg_include_dir Ogg::ogg INTERFACE_INCLUDE_DIRECTORIES)
    if(_ogg_include_dir AND EXISTS "${_ogg_include_dir}/ogg/ogg.h")
        file(STRINGS "${_ogg_include_dir}/ogg/ogg.h" _ogg_version_lines
             REGEX "^#define[ \\t]+OGG_VERSION[ \\t]+")
        if(_ogg_version_lines)
            string(REGEX MATCH "\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _ "${_ogg_version_lines}")
            set(Ogg_VERSION "${CMAKE_MATCH_1}")
        endif()
        unset(_ogg_version_lines)
    endif()
    unset(_ogg_include_dir)

    # Create PascalCase alias for consistency with our naming convention
    if(NOT TARGET Ogg::Ogg)
        add_library(Ogg::Ogg ALIAS Ogg::ogg)
    endif()

    message(STATUS "Found Ogg via CMake config: ${Ogg_VERSION}")
else()
    # Fallback to pkg-config search for older systems without CMake config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_OGG QUIET ogg)
    endif()

    find_library(Ogg_LIBRARY
        NAMES ogg
        HINTS ${PC_OGG_LIBRARY_DIRS}
    )

    find_path(Ogg_INCLUDE_DIR
        NAMES ogg/ogg.h
        HINTS ${PC_OGG_INCLUDE_DIRS}
    )

    if(PC_OGG_VERSION)
        set(Ogg_VERSION ${PC_OGG_VERSION})
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Ogg
        REQUIRED_VARS Ogg_LIBRARY Ogg_INCLUDE_DIR
        VERSION_VAR Ogg_VERSION
    )

    # Create imported target
    if(Ogg_FOUND AND NOT TARGET Ogg::Ogg)
        add_library(Ogg::Ogg UNKNOWN IMPORTED)
        set_target_properties(Ogg::Ogg PROPERTIES
            IMPORTED_LOCATION "${Ogg_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Ogg_INCLUDE_DIR}"
        )
        if(PC_OGG_CFLAGS_OTHER)
            set_target_properties(Ogg::Ogg PROPERTIES
                INTERFACE_COMPILE_OPTIONS "${PC_OGG_CFLAGS_OTHER}"
            )
        endif()
    endif()

    mark_as_advanced(Ogg_LIBRARY Ogg_INCLUDE_DIR)
endif()
