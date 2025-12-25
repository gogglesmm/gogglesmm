# FindGnuTLS.cmake
# Modern CMake module for finding GnuTLS (GNU Transport Layer Security Library)
#
# Creates imported target: GnuTLS::GnuTLS
#
# Output variables:
#   GnuTLS_FOUND          - True if GnuTLS was found
#   GnuTLS_VERSION        - Version string (if available)
#
# Output targets:
#   GnuTLS::GnuTLS        - Imported target for GnuTLS library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_GNUTLS QUIET gnutls>=3.4.0)
endif()

# Find the library
find_library(GnuTLS_LIBRARY
    NAMES gnutls
    HINTS ${PC_GNUTLS_LIBRARY_DIRS}
)

# Find the include directory
find_path(GnuTLS_INCLUDE_DIR
    NAMES gnutls/gnutls.h
    HINTS ${PC_GNUTLS_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_GNUTLS_VERSION)
    set(GnuTLS_VERSION ${PC_GNUTLS_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GnuTLS
    REQUIRED_VARS GnuTLS_LIBRARY GnuTLS_INCLUDE_DIR
    VERSION_VAR GnuTLS_VERSION
)

# Create imported target
if(GnuTLS_FOUND AND NOT TARGET GnuTLS::GnuTLS)
    add_library(GnuTLS::GnuTLS UNKNOWN IMPORTED)
    set_target_properties(GnuTLS::GnuTLS PROPERTIES
        IMPORTED_LOCATION "${GnuTLS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GnuTLS_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_GNUTLS_CFLAGS_OTHER)
        set_target_properties(GnuTLS::GnuTLS PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GNUTLS_CFLAGS_OTHER}"
        )
    endif()

    # Add library dependencies from pkg-config
    if(PC_GNUTLS_LDFLAGS)
        set_target_properties(GnuTLS::GnuTLS PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PC_GNUTLS_LDFLAGS}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(GnuTLS_LIBRARY GnuTLS_INCLUDE_DIR)
