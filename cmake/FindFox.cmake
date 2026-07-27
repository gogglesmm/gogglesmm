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

# ============================================================================
# Detect reswrap version and set the command variables consumers use.
#
# Every mode below returns as soon as it has found FOX, so this has to be
# called explicitly on each of those paths - it cannot live at the end of the
# file. Without it FOX_RESWRAP_H/CPP/TEXT stay empty and the generated build
# rules start with "-o", failing at build time with "-o: command not found".
# ============================================================================
macro(_fox_setup_reswrap)
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
endmacro()

# Early return if already found
if(TARGET FX::FOX)
    set(FOX_FOUND TRUE)
    if(TARGET FX::reswrap)
        get_target_property(FOX_RESWRAP FX::reswrap IMPORTED_LOCATION)
    endif()
    _fox_setup_reswrap()
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
        _fox_setup_reswrap()
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
        _fox_setup_reswrap()
        return()
    endif()

    # Fallback to pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        # Try different FOX version names
        # Note: the .pc files are named fox17 / fox16, not fox-1.7 / fox-1.6
        foreach(_version 17 16)
            # GLOBAL is required so the imported target can be aliased below and
            # used from the src/ and gap/ subdirectories
            pkg_check_modules(FOX QUIET IMPORTED_TARGET GLOBAL fox${_version})
            if(FOX_FOUND)
                message(STATUS "FOX: Found via pkg-config (fox${_version}, version ${FOX_VERSION})")

                # pkg_check_modules(IMPORTED_TARGET) already provides a fully
                # populated target: library, include dirs and compile options.
                if(NOT TARGET FX::FOX)
                    add_library(FX::FOX ALIAS PkgConfig::FOX)
                endif()

                # You really shouldn't be using xincs.h when using pkg-config,
                # But people insist anyway... perhaps in the future this needs
                # extra logic to determine which additional libraries are included
                # and find headers to go along with them.
                if(NOT TARGET FX::FOX_XINCS)
                    add_library(FOX_XINCS INTERFACE)
                    target_include_directories(FOX_XINCS INTERFACE ${FOX_INCLUDE_DIRS})
                    add_library(FX::FOX_XINCS ALIAS FOX_XINCS)
                endif()

                # Find reswrap. pkg_check_modules does not set FOX_PREFIX, so ask
                # pkg-config for it directly.
                pkg_get_variable(FOX_PREFIX fox${_version} prefix)
                find_program(FOX_RESWRAP
                    NAMES reswrap reswrap-1.7 reswrap-1.6
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

                _fox_setup_reswrap()
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

# Bundled mode is the only path that reaches here; the others return above
_fox_setup_reswrap()
