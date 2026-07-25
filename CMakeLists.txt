# Using cmake 3.23 or higher
cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

# Set policies to match current version and future proof for 4.x
cmake_policy(VERSION 3.23...4.2)

project(gogglesmm_dist)

# Honor CMAKE_EXE_LINKER_FLAGS during tests
# (CMP0056 is included in the VERSION 3.23 policy above, but keeping explicit for clarity if desired, 
# though redundant. I will remove it as it's covered by the version policy.)

# Include standard modules
include(FeatureSummary)
include(GNUInstallDirs)

# Enable for cmake debugging
# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# libdir name to allow overriding to lib32 / lib64
# set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")

# Add cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Override default install to /usr
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX /usr CACHE STRING "Installation prefix used by install." FORCE)
endif()

# Override default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Specify the desired build type." FORCE)
endif()

# Status so far
message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

#-------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------

include(CompilerFlags)

#-------------------------------------------------------------------------------

# Find or build FOX toolkit
# By default uses bundled fox/ subdirectory
# Set FOX_USE_EXTERNAL=ON to use system FOX
# Set FOX_BUILD_TREE=/path/to/build to use FOX from a build tree
include(FindFox)

# Add Goggles Audio Player
add_subdirectory(gap)

# Add Goggles Music Manager
add_subdirectory(src)


#-------------------------------------------------------------------------------
# Install application icons in various sizes
#-------------------------------------------------------------------------------

# Define icon sizes and their source directories
set(_icon_sizes_and_dirs
  "16:icons"
  "22:extra"
  "24:extra"
  "32:icons"
  "48:extra"
  "64:extra"
)

# Install PNG icons
foreach(_entry ${_icon_sizes_and_dirs})
  string(REPLACE ":" ";" _parts "${_entry}")
  list(GET _parts 0 _size)
  list(GET _parts 1 _dir)
  install(
    FILES ${_dir}/gogglesmm_${_size}.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_size}x${_size}/apps
    RENAME gogglesmm.png
  )
endforeach()

# Install SVG icon
install(
  FILES extra/gogglesmm.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
  RENAME gogglesmm.svg
)

# Install desktop metadata
install(FILES extra/gogglesmm.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES extra/gogglesmm.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES extra/gogglesmm.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Install translations when native language support has been enabled
if(WITH_NLS)
  include(Translations)
  install_translations(
    DOMAIN gogglesmm
    PO_DIR ${CMAKE_SOURCE_DIR}/po
    LANGUAGES de es fr hu ko pt ru
  )

  # Add targets for updating translations (manual invocation)
  add_pot_update_target(
    DOMAIN gogglesmm
    POT_FILE ${CMAKE_SOURCE_DIR}/po/gogglesmm.pot
    SOURCE_FILES src/*.cpp src/*.h
    KEYWORDS tr:1 fxtr:1 notr:1 fxtrformat:1
    FLAGS fxtrformat:1:c-format
    PACKAGE_VERSION 1.3.0
    BUGS_ADDRESS s.jansen@gmail.com
    COPYRIGHT_HOLDER "Sander Jansen"
    APPEND_SOURCE_FILES
      fox/lib/FXMessageBox.cpp
      fox/lib/FXColorSelector.cpp
      fox/lib/FXDirSelector.cpp
      fox/lib/FXFileList.cpp
      fox/lib/FXFileSelector.cpp
      fox/lib/FXReplaceDialog.cpp
      fox/lib/FXSearchDialog.cpp
      fox/lib/FXStatusLine.cpp
  )
endif()

#-------------------------------------------------------------------------------
# Post-installation script to update icon cache
#-------------------------------------------------------------------------------

install(SCRIPT ${CMAKE_SOURCE_DIR}/cmake/UpdateIconCache.cmake)

#-------------------------------------------------------------------------------

# Display Feature Summary
feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Features:\n")
