cmake_minimum_required(VERSION 4.0 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

project(GOGGLESMM VERSION 1.2.5)


option(WITH_OPENGL "Enable OpenGL Features" ON)
option(WITH_DBUS "Enable DBUS Features" ON)
option(WITH_NLS "Enable Native Language Support" ON)
option(WITH_SESSION "Enable X Session Support" ON)

# Translation Files to share/locale
set(LOCALE_PATH ${CMAKE_INSTALL_PREFIX}/share/locale)

# Don't use CMAKE_INSTALL_PREFIX since most icons won't be installed anywhere but /usr
set(ICON_PATH /usr/share/icons)

# We need C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Display the build type
message(STATUS "Icon Path: ${ICON_PATH}")
message(STATUS "Locale Path: ${LOCALE_PATH}")


#------------------------------------------------------------------------------

TEST_BIG_ENDIAN(GOGGLESMM_BIGENDIAN)

# FOX is found at top-level via FindFox.cmake, FX::FOX and FOX_RESWRAP are already available

#-------------------------------------------------------------------------------
# Find Required Dependencies
#-------------------------------------------------------------------------------

# TagLib - Audio metadata library (required, >=1.9.0)
find_package(TagLib 1.9.0 REQUIRED)

# SQLite3 - Database engine (required, >=3.6.19)
find_package(SQLite3 3.6.19 REQUIRED)


if(NOT TARGET SQLite3::SQLite3) # CMake < 4.3
  add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()


#-------------------------------------------------------------------------------
# Find Optional Dependencies
#-------------------------------------------------------------------------------

# X11 Session Management
find_package(X11)
if(X11_FOUND)
  # X11_SM_FOUND and X11_ICE_FOUND are set by FindX11
  if(X11_SM_FOUND)
    message(STATUS "Found X11 SM: ${X11_SM_LIB}")
  endif()
  if(X11_ICE_FOUND)
    message(STATUS "Found X11 ICE: ${X11_ICE_LIB}")
  endif()
endif()

# D-Bus message bus
if(WITH_DBUS)
  find_package(DBus)
endif()

# OpenGL + libepoxy
if(WITH_OPENGL)
  find_package(OpenGL REQUIRED COMPONENTS OpenGL)
  find_package(Epoxy REQUIRED)
  set(HAVE_OPENGL 1)
endif()

# Native Language Support (gettext/Intl)
if(WITH_NLS)
  find_package(Intl REQUIRED)
  set(HAVE_NLS 1)
endif()

# Use reswrap command variables from FindFox.cmake
# FindFox sets: FOX_RESWRAP, FOX_RESWRAP_H, FOX_RESWRAP_CPP, FOX_RESWRAP_TEXT
if(NOT FOX_RESWRAP)
  message(FATAL_ERROR "reswrap not found - FOX_RESWRAP should be set by FindFox.cmake")
endif()

set(RESWRAP ${FOX_RESWRAP})
set(RESWRAP_H ${FOX_RESWRAP_H})
set(RESWRAP_CPP ${FOX_RESWRAP_CPP})
set(RESWRAP_TEXT ${FOX_RESWRAP_TEXT})


#------------------------------------------------------------------------------
# Source and Header File Lists
#------------------------------------------------------------------------------

# Include source, header, and icon file lists
include(sources.cmake)
include(headers.cmake)
include(icons.cmake)



if(WITH_SESSION AND SM_FOUND AND ICE_FOUND)
  list(APPEND SOURCES GMSession.cpp)
  set(HAVE_SESSION 1)
endif()



#-------------------------------------------------------------------------------
# Generate icon resources using reswrap
#-------------------------------------------------------------------------------

# Get full paths to icon files for proper dependency tracking
set(_icon_files "")
foreach(_icon ${ICONS})
  list(APPEND _icon_files "${CMAKE_SOURCE_DIR}/icons/${_icon}")
endforeach()

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons.h ${CMAKE_CURRENT_SOURCE_DIR}/icons.cpp
  COMMAND ${RESWRAP_H} -o ${CMAKE_CURRENT_SOURCE_DIR}/icons.h ${ICONS}
  COMMAND ${RESWRAP_CPP} -o ${CMAKE_CURRENT_SOURCE_DIR}/icons.cpp ${ICONS}
  DEPENDS ${_icon_files}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/icons
  COMMENT "Generating icon resources with reswrap"
  VERBATIM
)

# Enable DBUS specific features
if (WITH_DBUS AND DBUS_FOUND)
  list(APPEND SOURCES GMDBus.cpp
                      GMSettingsDaemon.cpp
                      GMMediaPlayerService.cpp
                      GMNotifyDaemon.cpp)

  list(APPEND HEADERS GMDBus.h
                      GMSettingsDaemon.h
                      GMMediaPlayerService.h
                      GMNotifyDaemon.h
                      mpris1_xml.h
                      mpris2_xml.h)

  # Generate MPRIS XML resources using reswrap
  set(_mpris1_sources mpris.xml mpris_player.xml mpris_tracklist.xml)
  set(_mpris2_sources mpris2.xml)

  # Convert to full paths for dependency tracking
  set(_mpris1_files "")
  set(_mpris2_files "")
  foreach(_xml ${_mpris1_sources})
    list(APPEND _mpris1_files "${CMAKE_CURRENT_SOURCE_DIR}/${_xml}")
  endforeach()
  foreach(_xml ${_mpris2_sources})
    list(APPEND _mpris2_files "${CMAKE_CURRENT_SOURCE_DIR}/${_xml}")
  endforeach()

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/mpris1_xml.h ${CMAKE_CURRENT_SOURCE_DIR}/mpris2_xml.h
    COMMAND ${RESWRAP_TEXT} -o mpris1_xml.h ${_mpris1_sources}
    COMMAND ${RESWRAP_TEXT} -o mpris2_xml.h ${_mpris2_sources}
    DEPENDS ${_mpris1_files} ${_mpris2_files}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating MPRIS XML resources with reswrap"
    VERBATIM
  )
  set(HAVE_DBUS 1)
endif()

#-------------------------------------------------------------------------------
#
# Workaround to compile with earlier versions of FOX, something
# that cannot be easily handled using the preprocessor
#
if (DEFINED FOX_VERSION AND FOX_VERSION VERSION_LESS "1.7.80")
  message("Replacing occurences of FXString::compare")
  foreach(sourcefile IN LISTS SOURCES)
    if(EXISTS ${sourcefile})
      file(READ ${sourcefile} content)
      string(REPLACE "FXString::compare" "FX::compare" updated "${content}")
      string(REPLACE "comparenatural" "compareversion" updated "${updated}")
      file(WRITE ${sourcefile} "${updated}")
    endif()
  endforeach()
endif()

#-------------------------------------------------------------------------------

# Features
add_feature_info(dbus HAVE_DBUS "D-Bus")
add_feature_info(nls WITH_NLS "Native Language Support")
add_feature_info(opengl HAVE_OPENGL "OpenGL Rendering")
add_feature_info(session HAVE_SESSION "X11 Session Management")

# Write gmconfig.h
configure_file(gmconfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/gmconfig.h)

# Goggles Music Manager
add_executable(gogglesmm ${SOURCES} ${HEADERS})

# GAP output plugins need symbols from the gap library. If GAP is static,
# it sets an INTERFACE property telling us we need to export symbols.
get_target_property(_gap_requires_exports gap INTERFACE_GAP_REQUIRES_CONSUMER_EXPORTS)
if(_gap_requires_exports)
  message(STATUS "gogglesmm: enabling symbol exports (required by static GAP)")
  set_target_properties(gogglesmm PROPERTIES ENABLE_EXPORTS TRUE)
endif()

#-------------------------------------------------------------------------------
# Precompiled Headers - external library headers only
#-------------------------------------------------------------------------------

# Precompile stable external headers that rarely change
# gmdefs.h is included in 55/56 source files and pulls in fx.h
# Only precompile fx.h (not fx3d.h/OpenGL) to avoid X11 macro pollution (Bool, etc.)
# which conflicts with TagLib and other libraries
target_precompile_headers(gogglesmm PRIVATE
  <fx.h>
)

# Link libraries using modern imported targets
# Imported targets automatically provide include directories
target_link_libraries(gogglesmm PRIVATE
  # Core dependencies
  gap
  FX::FOX
  FX::FOX_XINCS
  TagLib::TagLib
  SQLite3::SQLite3
)

# Optional dependencies
if(WITH_DBUS AND DBus_FOUND)
  target_link_libraries(gogglesmm PRIVATE DBus::DBus)
endif()

if(WITH_OPENGL)
  target_link_libraries(gogglesmm PRIVATE
    OpenGL::OpenGL
    Epoxy::Epoxy
  )
endif()

if(WITH_NLS AND Intl_FOUND)
  target_link_libraries(gogglesmm PRIVATE ${Intl_LIBRARIES})
  # Intl may need include directories
  if(Intl_INCLUDE_DIRS)
    target_include_directories(gogglesmm PRIVATE ${Intl_INCLUDE_DIRS})
  endif()
endif()

# X11 Session Management (uses FindX11 variables, not modern targets)
if(X11_SM_FOUND)
  target_link_libraries(gogglesmm PRIVATE ${X11_SM_LIB})
endif()

if(X11_ICE_FOUND)
  target_link_libraries(gogglesmm PRIVATE ${X11_ICE_LIB})
endif()

if(X11_FOUND)
  target_link_libraries(gogglesmm PRIVATE ${X11_LIBRARIES})
endif()

install(TARGETS gogglesmm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
