# FindDCA.cmake
# Modern CMake module for finding libdca (DTS Coherent Acoustics decoder)
#
# Creates imported target: DCA::DCA
#
# Output variables:
#   DCA_FOUND          - True if DCA was found
#   DCA_VERSION        - Version string (if available)
#
# Output targets:
#   DCA::DCA           - Imported target for DCA library

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_DCA QUIET libdca)
endif()

# Find the library
find_library(DCA_LIBRARY
    NAMES dca
    HINTS ${PC_DCA_LIBRARY_DIRS}
)

# Find the include directory
find_path(DCA_INCLUDE_DIR
    NAMES dca.h
    HINTS ${PC_DCA_INCLUDE_DIRS}
)

# Extract version from pkg-config if available
if(PC_DCA_VERSION)
    set(DCA_VERSION ${PC_DCA_VERSION})
endif()

# Standard find_package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DCA
    REQUIRED_VARS DCA_LIBRARY DCA_INCLUDE_DIR
    VERSION_VAR DCA_VERSION
)

# Create imported target
if(DCA_FOUND AND NOT TARGET DCA::DCA)
    add_library(DCA::DCA UNKNOWN IMPORTED)
    set_target_properties(DCA::DCA PROPERTIES
        IMPORTED_LOCATION "${DCA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${DCA_INCLUDE_DIR}"
    )

    # Add any extra compiler flags from pkg-config
    if(PC_DCA_CFLAGS_OTHER)
        set_target_properties(DCA::DCA PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_DCA_CFLAGS_OTHER}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(DCA_LIBRARY DCA_INCLUDE_DIR)
