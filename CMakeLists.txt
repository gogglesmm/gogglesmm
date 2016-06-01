# Using cmake 3.2
cmake_minimum_required(VERSION 3.4 FATAL_ERROR)

# Need FeaureSummary
include(FeatureSummary)

# Enable for cmake debugging
# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# libdir name to allow overriding to lib32 / lib64
set(CMAKE_INSTALL_LIBDIR
    "lib"
    CACHE STRING
    "Specify the name of the directory where libraries are meant to be installed")


# Override default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel
        CACHE STRING
        "Specify the desired build type." FORCE)
endif()

# Display the build type
message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")


# Override default install to /usr
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX /usr CACHE STRING "Installation prefix used by install." FORCE)
endif()

# Display the build type
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

#-------------------------------------------------------------------------------

include(CheckCXXCompilerFlag)


# Additional compiler flags we like
CHECK_CXX_COMPILER_FLAG(-Wall       HAS_CXX_WALL)
CHECK_CXX_COMPILER_FLAG(-Wextra     HAS_CXX_WEXTRA)
CHECK_CXX_COMPILER_FLAG(-Wformat=2  HAS_CXX_WFORMAT)
CHECK_CXX_COMPILER_FLAG(-Wfatal-errors  HAS_CXX_WFATAL)
CHECK_CXX_COMPILER_FLAG(-Og         HAS_CXX_OPTIMIZE_DEBUG)
# FIXME Doesn't work on all platforms.
#CHECK_CXX_COMPILER_FLAG(-flto       HAS_CXX_OPTIMIZE_LINKTIME)

if(HAS_CXX_WALL)
    add_compile_options(-Wall)
endif()

if(HAS_CXX_WEXTRA)
    add_compile_options(-Wextra)
endif()

if(HAS_CXX_WFORMAT)
    add_compile_options(-Wformat=2)
endif()

if(HAS_CXX_WFATAL)
    add_compile_options(-Wfatal-errors)
endif()


if(HAS_CXX_OPTIMIZE_DEBUG AND CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-Og)
endif()

if(HAS_CXX_OPTIMIZE_LINKTIME AND NOT CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-flto)
    link_libraries(-flto)
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG)
endif()

#-------------------------------------------------------------------------------


# Add Goggles Audio Player
add_subdirectory(src/gap)

# Add Goggles Music Manager
add_subdirectory(src)


#-------------------------------------------------------------------------------

# Extra Installation Files
install(PROGRAMS extra/import_banshee_stats.py
                 extra/import_gogglesmm12.py
                 extra/import_gogglesmm12_stats.py
        DESTINATION
                 share/gogglesmm)

install(FILES icons/gogglesmm_16.png DESTINATION share/icons/hicolor/16x16/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_22.png DESTINATION share/icons/hicolor/22x22/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_24.png DESTINATION share/icons/hicolor/24x24/apps RENAME gogglesmm.png)
install(FILES icons/gogglesmm_32.png DESTINATION share/icons/hicolor/32x32/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm_48.png DESTINATION share/icons/hicolor/48x48/apps RENAME gogglesmm.png)
install(FILES extra/gogglesmm.svg DESTINATION share/icons/hicolor/scalable/apps RENAME gogglesmm.svg)
install(FILES extra/gogglesmm.appdata.xml DESTINATION share/appdata)
install(FILES extra/gogglesmm.desktop DESTINATION share/applications)
install(FILES extra/gogglesmm.1 DESTINATION share/man/man1)

# Install translations when native language support has been enabled
if(WITH_NLS)
    set(LINGUAS "All" CACHE STRING "A space-seperated list of translations to compile in to gogglesmm, or \"None\".")
    if (LINGUAS STREQUAL "All")
        file(GLOB pofiles po/*.po)
        foreach(pofile ${pofiles})
            get_filename_component(lang ${pofile} NAME_WE)
            list(APPEND LANGUAGES ${lang})
        endforeach()
    else()
        if(NOT LINGUAS OR LINGUAS STREQUAL "None")
            set(LANGUAGES "")
        else()
            string(REGEX MATCHALL [a-zA-Z_@]+ LANGUAGES ${LINGUAS})
        endif()
    endif()

    foreach(lang ${LANGUAGES})
        install(FILES po/${lang}.mo DESTINATION share/locale/${lang}/LC_MESSAGES RENAME gogglesmm.mo)
    endforeach()
endif()


feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Features:\n")


