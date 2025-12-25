# FindVorbis.cmake
# Modern CMake module for finding Vorbis audio codec
#
# Creates imported targets: Vorbis::Vorbis, Vorbis::VorbisFile, Vorbis::VorbisEnc
#
# Output variables:
#   Vorbis_FOUND          - True if Vorbis was found
#   Vorbis_VERSION        - Version string
#
# Output targets:
#   Vorbis::Vorbis        - Imported target for vorbis library
#   Vorbis::VorbisFile    - Imported target for vorbisfile library
#   Vorbis::VorbisEnc     - Imported target for vorbisenc library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_VORBIS QUIET vorbis)
    pkg_check_modules(PC_VORBISFILE QUIET vorbisfile)
    pkg_check_modules(PC_VORBISENC QUIET vorbisenc)
endif()

# Find vorbis library
find_library(Vorbis_LIBRARY
    NAMES vorbis
    HINTS ${PC_VORBIS_LIBRARY_DIRS}
)

find_path(Vorbis_INCLUDE_DIR
    NAMES vorbis/codec.h
    HINTS ${PC_VORBIS_INCLUDE_DIRS}
)

# Find vorbisfile library (optional)
find_library(VorbisFile_LIBRARY
    NAMES vorbisfile
    HINTS ${PC_VORBISFILE_LIBRARY_DIRS}
)

# Find vorbisenc library (optional)
find_library(VorbisEnc_LIBRARY
    NAMES vorbisenc
    HINTS ${PC_VORBISENC_LIBRARY_DIRS}
)

if(PC_VORBIS_VERSION)
    set(Vorbis_VERSION ${PC_VORBIS_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis
    REQUIRED_VARS Vorbis_LIBRARY Vorbis_INCLUDE_DIR
    VERSION_VAR Vorbis_VERSION
)

# Create main Vorbis target
if(Vorbis_FOUND AND NOT TARGET Vorbis::Vorbis)
    add_library(Vorbis::Vorbis UNKNOWN IMPORTED)
    set_target_properties(Vorbis::Vorbis PROPERTIES
        IMPORTED_LOCATION "${Vorbis_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
    )
    if(PC_VORBIS_CFLAGS_OTHER)
        set_target_properties(Vorbis::Vorbis PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_VORBIS_CFLAGS_OTHER}"
        )
    endif()
endif()

# Create VorbisFile target
if(VorbisFile_LIBRARY AND NOT TARGET Vorbis::VorbisFile)
    add_library(Vorbis::VorbisFile UNKNOWN IMPORTED)
    set_target_properties(Vorbis::VorbisFile PROPERTIES
        IMPORTED_LOCATION "${VorbisFile_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "Vorbis::Vorbis"
    )
endif()

# Create VorbisEnc target
if(VorbisEnc_LIBRARY AND NOT TARGET Vorbis::VorbisEnc)
    add_library(Vorbis::VorbisEnc UNKNOWN IMPORTED)
    set_target_properties(Vorbis::VorbisEnc PROPERTIES
        IMPORTED_LOCATION "${VorbisEnc_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "Vorbis::Vorbis"
    )
endif()

mark_as_advanced(Vorbis_LIBRARY Vorbis_INCLUDE_DIR VorbisFile_LIBRARY VorbisEnc_LIBRARY)
