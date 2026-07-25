# FindFox.cmake
# Find or build the FOX Toolkit library
#
# This module supports three modes, listed in the order they are tried:
#   1. Build Tree - Uses FOX from an existing build tree via FOX_BUILD_TREE
#   2. External - Finds installed FOX via find_package() or pkg-config
#   3. Bundled (default) - Builds the fox/ subdirectory via add_subdirectory()
#
# Options:
#   FOX_USE_EXTERNAL    - Set to ON to use external FOX instead of bundled (default: OFF)
#   FOX_BUILD_TREE      - Path to FOX build tree (e.g., /path/to/fox/build/release-native)
#                         Takes precedence over FOX_USE_EXTERNAL when it exists.
#   FOX_BUNDLED_SOURCE_DIR
#                       - Override the bundled FOX source location (default: ${CMAKE_CURRENT_SOURCE_DIR}/fox)
#                         Only used by the bundled mode.
#
# Output Variables:
#   FOX_FOUND           - TRUE if FOX was found
#   FOX_RESWRAP         - Path to reswrap utility
#
# Output Targets:
#   FX::FOX             - Main FOX library target
#   FX::FOX_XINCS       - Platform definitions (if needed by application)
#   FX::reswrap         - reswrap utility executable target (if available)

# Option to use external FOX instead of bundled
option(FOX_USE_EXTERNAL "Use external FOX library instead of bundled fox/ subdirectory" OFF)

# Early return if already found
if(TARGET FX::FOX)
    set(FOX_FOUND TRUE)
    if(TARGET FX::reswrap)
        get_target_property(FOX_RESWRAP FX::reswrap IMPORTED_LOCATION)
    endif()
    return()
endif()

# Mode 1: Build Tree - Highest priority if specified
if(DEFINED FOX_BUILD_TREE AND EXISTS "${FOX_BUILD_TREE}")
    message(STATUS "FOX: Using build tree at ${FOX_BUILD_TREE}")

    # Find the fox-config.cmake in the build tree
    # Build tree structure: build/lib/fox-config.cmake
    find_package(FOX QUIET CONFIG
        PATHS "${FOX_BUILD_TREE}" "${FOX_BUILD_TREE}/lib"
        NO_DEFAULT_PATH
    )

    if(FOX_FOUND)
        message(STATUS "FOX: Loaded from build tree")
        if(DEFINED FOX_RESWRAP_EXECUTABLE)
            set(FOX_RESWRAP "${FOX_RESWRAP_EXECUTABLE}" CACHE FILEPATH "Path to reswrap utility")
        endif()
        return()
    else()
        message(WARNING "FOX: Build tree specified but fox-config.cmake not found in ${FOX_BUILD_TREE}")
    endif()
endif()

# Mode 2: External - Use system-installed FOX
if(FOX_USE_EXTERNAL)
    message(STATUS "FOX: Searching for external installation")

    # Try CMake config first (modern method)
    find_package(FOX QUIET CONFIG)

    if(FOX_FOUND)
        message(STATUS "FOX: Found via CMake config (version ${FOX_VERSION})")
        if(DEFINED FOX_RESWRAP_EXECUTABLE)
            set(FOX_RESWRAP "${FOX_RESWRAP_EXECUTABLE}" CACHE FILEPATH "Path to reswrap utility")
        endif()
        return()
    endif()

    # Fallback to pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        # Try different FOX version names
        foreach(_version 1.7 1.6)
            pkg_check_modules(FOX QUIET fox-${_version})
            if(FOX_FOUND)
                message(STATUS "FOX: Found via pkg-config (fox-${_version})")

                # Create imported target from pkg-config results
                if(NOT TARGET FX::FOX)
                    add_library(FX::FOX UNKNOWN IMPORTED)

                    # Find the library file
                    find_library(FOX_LIBRARY
                        NAMES FOX-${_version} fox-${_version} FOX fox
                        HINTS ${FOX_LIBRARY_DIRS}
                    )

                    if(FOX_LIBRARY)
                        set_target_properties(FX::FOX PROPERTIES
                            IMPORTED_LOCATION "${FOX_LIBRARY}"
                            INTERFACE_INCLUDE_DIRECTORIES "${FOX_INCLUDE_DIRS}"
                            INTERFACE_COMPILE_OPTIONS "${FOX_CFLAGS_OTHER}"
                        )

                        # Parse and add library dependencies
                        if(FOX_LDFLAGS)
                            set_target_properties(FX::FOX PROPERTIES
                                INTERFACE_LINK_LIBRARIES "${FOX_LDFLAGS}"
                            )
                        endif()
                    endif()
                endif()

                # Find reswrap
                find_program(FOX_RESWRAP
                    NAMES reswrap reswrap-${_version}
                    HINTS ${FOX_PREFIX}/bin
                    PATH_SUFFIXES bin
                )

                if(FOX_RESWRAP)
                    message(STATUS "FOX: Found reswrap: ${FOX_RESWRAP}")
                    if(NOT TARGET FX::reswrap)
                        add_executable(FX::reswrap IMPORTED)
                        set_target_properties(FX::reswrap PROPERTIES
                            IMPORTED_LOCATION "${FOX_RESWRAP}"
                        )
                    endif()
                endif()

                return()
            endif()
        endforeach()
    endif()

    # Not found
    message(FATAL_ERROR "FOX: External FOX requested but not found. Install FOX or set FOX_USE_EXTERNAL=OFF to use bundled fox/")
endif()

# Mode 3: Bundled - Default, use fox/ subdirectory
# FOX_BUNDLED_SOURCE_DIR can be set to override the default location
if(FOX_BUNDLED_SOURCE_DIR)
    set(_fox_source_dir "${FOX_BUNDLED_SOURCE_DIR}")
else()
    set(_fox_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/fox")
endif()

if(EXISTS "${_fox_source_dir}/CMakeLists.txt")
    message(STATUS "FOX: Using bundled fox/ subdirectory")

    # Configure FOX to only build what we need (library + reswrap)
    # Skip apps, tests, and chart library to speed up builds
    set(FOX_BUILD_APPS OFF CACHE BOOL "Build FOX applications" FORCE)
    set(FOX_BUILD_TESTS OFF CACHE BOOL "Build FOX tests" FORCE)
    set(FOX_BUILD_CHART OFF CACHE BOOL "Build FOX chart library" FORCE)

    # Add the bundled FOX as a subdirectory
    # This will create FX::FOX, FX::FOX_XINCS, and FX::reswrap targets
    add_subdirectory("${_fox_source_dir}" "${CMAKE_CURRENT_BINARY_DIR}/fox-build" EXCLUDE_FROM_ALL)

    set(FOX_FOUND TRUE)

    # Extract FOX_VERSION from bundled fox/CMakeLists.txt
    if(NOT DEFINED FOX_VERSION)
        file(STRINGS "${_fox_source_dir}/CMakeLists.txt" FOX_VERSION_LINE
             REGEX "^project\\(FOX VERSION"
             LIMIT_COUNT 1)
        if(FOX_VERSION_LINE)
            string(REGEX MATCH "VERSION ([0-9]+\\.[0-9]+\\.[0-9]+)" _ "${FOX_VERSION_LINE}")
            set(FOX_VERSION "${CMAKE_MATCH_1}" CACHE STRING "FOX Toolkit version")
            message(STATUS "FOX: Bundled version ${FOX_VERSION}")
        endif()
    endif()

    # The reswrap target should already be available as FX::reswrap
    # But also set the variable for compatibility
    if(TARGET reswrap)
        set(FOX_RESWRAP "$<TARGET_FILE:reswrap>")
    elseif(TARGET FX::reswrap)
        get_target_property(FOX_RESWRAP FX::reswrap IMPORTED_LOCATION)
        if(NOT FOX_RESWRAP)
            # It's not imported, it's a real target
            set(FOX_RESWRAP "$<TARGET_FILE:FX::reswrap>")
        endif()
    endif()

    message(STATUS "FOX: Bundled build configured")
else()
    message(FATAL_ERROR "FOX: Bundled fox/ subdirectory not found. Either:\n"
                        "  1. Create fox/ symlink: ln -s ../fox fox\n"
                        "  2. Use external FOX: cmake -DFOX_USE_EXTERNAL=ON\n"
                        "  3. Use build tree: cmake -DFOX_BUILD_TREE=/path/to/fox/build")
endif()

# ============================================================================
# Detect reswrap version and set command variables
# ============================================================================

if(FOX_RESWRAP)
    # Detect reswrap version to use correct flags
    # Note: Skip version detection for generator expressions (build tree targets)
    if(NOT FOX_RESWRAP MATCHES "\\$<")
        execute_process(
            COMMAND ${FOX_RESWRAP} -v
            OUTPUT_VARIABLE RESWRAP_OUTPUT
            ERROR_QUIET
        )
        string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" RESWRAP_VERSION "${RESWRAP_OUTPUT}")
    endif()

    # Set reswrap command variables based on version
    if(RESWRAP_VERSION AND RESWRAP_VERSION VERSION_LESS "5.0")
        # Old reswrap < 5.0 uses short flags
        set(FOX_RESWRAP_H "${FOX_RESWRAP}" -i -k CACHE STRING "reswrap command for header generation")
        set(FOX_RESWRAP_CPP "${FOX_RESWRAP}" -e -k CACHE STRING "reswrap command for source generation")
        set(FOX_RESWRAP_TEXT "${FOX_RESWRAP}" -t -k CACHE STRING "reswrap command for text resources")
        message(STATUS "FOX: reswrap version ${RESWRAP_VERSION} (using short flags)")
    else()
        # Modern reswrap >= 5.0 uses long flags (or unknown version, assume modern)
        set(FOX_RESWRAP_H "${FOX_RESWRAP}" --keep-ext --header CACHE STRING "reswrap command for header generation")
        set(FOX_RESWRAP_CPP "${FOX_RESWRAP}" --keep-ext --source --extern CACHE STRING "reswrap command for source generation")
        set(FOX_RESWRAP_TEXT "${FOX_RESWRAP}" --keep-ext -t CACHE STRING "reswrap command for text resources")
        if(RESWRAP_VERSION)
            message(STATUS "FOX: reswrap version ${RESWRAP_VERSION} (using long flags)")
        else()
            message(STATUS "FOX: reswrap version unknown (assuming modern, using long flags)")
        endif()
    endif()

    message(STATUS "FOX: reswrap utility: ${FOX_RESWRAP}")
endif()
