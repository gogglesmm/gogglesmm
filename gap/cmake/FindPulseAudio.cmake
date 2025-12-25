# FindPulseAudio.cmake
# Modern CMake module for finding PulseAudio
#
# Creates imported target: PulseAudio::PulseAudio
#
# Output variables:
#   PulseAudio_FOUND          - True if PulseAudio was found
#   PulseAudio_VERSION        - Version string (e.g. "16.1")
#
# Output targets:
#   PulseAudio::PulseAudio    - Imported target for PulseAudio library

# Strategy:
# 1. Try PulseAudio's official CMake config (if installed)
# 2. Fall back to pkg-config search for older systems
# 3. Create imported target if not already present

# First, try PulseAudio's official CMake config package
# This is provided by newer PulseAudio installations (typically in /usr/lib/cmake/PulseAudio)
find_package(PulseAudio QUIET CONFIG)

# If official config was found, it sets PULSEAUDIO_* variables (uppercase)
# Map them to our preferred PascalCase naming for consistency
if(PULSEAUDIO_FOUND)
    set(PulseAudio_LIBRARY ${PULSEAUDIO_LIBRARY})
    set(PulseAudio_INCLUDE_DIR ${PULSEAUDIO_INCLUDE_DIR})
    set(PulseAudio_VERSION ${PULSEAUDIO_VERSION})
    set(PulseAudio_FOUND TRUE)
    message(STATUS "Found PulseAudio via CMake config: ${PulseAudio_VERSION}")
else()
    # Fallback to pkg-config search for older systems without CMake config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_PULSEAUDIO QUIET libpulse)
    endif()

    # Find the library
    find_library(PulseAudio_LIBRARY
        NAMES pulse
        HINTS ${PC_PULSEAUDIO_LIBRARY_DIRS}
    )

    # Find the include directory
    find_path(PulseAudio_INCLUDE_DIR
        NAMES pulse/pulseaudio.h
        HINTS ${PC_PULSEAUDIO_INCLUDE_DIRS}
    )

    # Extract version from pkg-config if available
    if(PC_PULSEAUDIO_VERSION)
        set(PulseAudio_VERSION ${PC_PULSEAUDIO_VERSION})
    endif()
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PulseAudio
    REQUIRED_VARS PulseAudio_LIBRARY PulseAudio_INCLUDE_DIR
    VERSION_VAR PulseAudio_VERSION
)

# Create imported target
if(PulseAudio_FOUND AND NOT TARGET PulseAudio::PulseAudio)
    add_library(PulseAudio::PulseAudio UNKNOWN IMPORTED)
    set_target_properties(PulseAudio::PulseAudio PROPERTIES
        IMPORTED_LOCATION "${PulseAudio_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PulseAudio_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_PULSEAUDIO_CFLAGS_OTHER)
        set_target_properties(PulseAudio::PulseAudio PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_PULSEAUDIO_CFLAGS_OTHER}"
        )
    endif()

    # Add library dependencies
    if(PC_PULSEAUDIO_LIBRARIES)
        set_target_properties(PulseAudio::PulseAudio PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PC_PULSEAUDIO_LDFLAGS}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(PulseAudio_LIBRARY PulseAudio_INCLUDE_DIR)
