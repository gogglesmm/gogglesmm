# FindGcrypt.cmake
# Modern CMake module for finding libgcrypt (GNU cryptographic library)
#
# Creates imported target: Gcrypt::Gcrypt
#
# Output variables:
#   Gcrypt_FOUND          - True if Gcrypt was found
#   Gcrypt_VERSION        - Version string (if available)
#
# Output targets:
#   Gcrypt::Gcrypt        - Imported target for Gcrypt library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GCRYPT QUIET libgcrypt)
endif()

# Find the library
find_library(Gcrypt_LIBRARY
    NAMES gcrypt
    HINTS ${PC_GCRYPT_LIBRARY_DIRS}
)

# Find the include directory
find_path(Gcrypt_INCLUDE_DIR
    NAMES gcrypt.h
    HINTS ${PC_GCRYPT_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_GCRYPT_VERSION)
    set(Gcrypt_VERSION ${PC_GCRYPT_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gcrypt
    REQUIRED_VARS Gcrypt_LIBRARY Gcrypt_INCLUDE_DIR
    VERSION_VAR Gcrypt_VERSION
)

# Create imported target
if(Gcrypt_FOUND AND NOT TARGET Gcrypt::Gcrypt)
    add_library(Gcrypt::Gcrypt UNKNOWN IMPORTED)
    set_target_properties(Gcrypt::Gcrypt PROPERTIES
        IMPORTED_LOCATION "${Gcrypt_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Gcrypt_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_GCRYPT_CFLAGS_OTHER)
        set_target_properties(Gcrypt::Gcrypt PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GCRYPT_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(Gcrypt_LIBRARY Gcrypt_INCLUDE_DIR)
