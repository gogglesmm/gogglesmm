cmake_policy(VERSION 3.2)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Set the buildtype")

project(GOGGLESMM VERSION 1.0.4)

# Allow building without opengl
option(WITH_OPENGL "Enable OpenGL Features" ON)
option(WITH_DBUS "Enable DBUS Features" ON)
option(WITH_NLS "Enable Native Language Support" ON)

find_package(PkgConfig)

pkg_check_modules(FOX fox17>=1.7.48 REQUIRED)
pkg_check_modules(TAGLIB taglib REQUIRED)
pkg_check_modules(SQLITE sqlite3 REQUIRED)

pkg_check_modules(SM sm)
pkg_check_modules(ICE ice)
pkg_check_modules(X11 x11)

if(WITH_DBUS)
    pkg_check_modules(DBUS dbus-1)
endif()

if(WITH_OPENGL)
    pkg_check_modules(EPOXY epoxy REQUIRED)
    pkg_check_modules(GLU glu REQUIRED)
    set(HAVE_OPENGL 1)
endif()

if(WITH_NLS)
    set(HAVE_NLS 1)
endif()

if(SM_FOUND AND ICE_FOUND)
    set(HAVE_SESSION 1)
endif()

# libgrypt
include(CheckIncludeFiles)
check_include_files(gcrypt.h HAVE_GCRYPT_H)
if (HAVE_GCRYPT_H)
   set(LIBGCRYPT_HEADERS_FOUND TRUE)
endif (HAVE_GCRYPT_H)

if (LIBGCRYPT_HEADERS_FOUND)
   find_library(LIBGCRYPT_LIBRARIES NAMES gcrypt )
endif (LIBGCRYPT_HEADERS_FOUND)
if (LIBGCRYPT_LIBRARIES)
   set(LIBGCRYPT_FOUND TRUE)
   set(HAVE_GCRYPT true)
   message(STATUS "libgcrypt found: ${LIBGCRYPT_LIBRARIES}")
endif()


#------------------------------------------------------------------------------

# We need C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wformat=2")

set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_definitions(-DDEBUG)
endif()


# Translation Files to share/locale
set(LOCALE_PATH ${CMAKE_INSTALL_PREFIX}/share/locale)

# Don't use CMAKE_INSTALL_PREFIX since most icons won't be installed anywhere but /usr
set(ICON_PATH /usr/share/icons)

# Write src/gmconfig.h
configure_file("src/gmconfig.h.in" "src/gmconfig.h")

#------------------------------------------------------------------------------

add_subdirectory(src/gap)

#------------------------------------------------------------------------------

set(SOURCES
  src/GMAbout.cpp
  src/GMAnimImage.cpp
  src/GMApp.cpp
  src/GMAudioPlayer.cpp
  src/GMAudioScrobbler.cpp
  src/GMAlbumList.cpp
  src/GMClipboard.cpp
  src/GMColumnDialog.cpp
  src/GMCover.cpp
  src/GMCoverCache.cpp
  src/GMCoverManager.cpp
  src/GMDatabase.cpp
  src/GMDatabaseSource.cpp
  src/GMFilename.cpp
  src/GMFilter.cpp
  src/GMFilterEditor.cpp
  src/GMFilterSource.cpp
  src/GMFontDialog.cpp
  src/GMIconTheme.cpp
  src/GMImportDialog.cpp
  src/GMImageView.cpp
  src/GMList.cpp
  src/GMLocalSource.cpp
  src/GMPlayerManager.cpp
  src/GMPlayListSource.cpp
  src/GMPlayQueue.cpp
  src/GMPodcastSource.cpp
  src/GMPreferences.cpp
  src/GMPreferencesDialog.cpp
  src/GMPresenter.cpp
  src/GMRemote.cpp
  src/GMSession.cpp
  src/GMScanner.cpp
  src/GMSource.cpp
  src/GMSourceView.cpp
  src/GMTag.cpp
  src/GMTaskManager.cpp
  src/GMTrack.cpp
  src/GMTrackDatabase.cpp
  src/GMTrackEditor.cpp
  src/GMTrackList.cpp
  src/GMTrackItem.cpp
  src/GMTrackView.cpp
  src/GMTrayIcon.cpp
  src/GMStreamSource.cpp
  src/GMWindow.cpp
  src/main.cpp
  src/icons.cpp
  src/fxext.cpp
  src/gmutils.cpp
  )

if (DBUS_FOUND)
  LIST(APPEND SOURCES src/GMDBus.cpp src/GMSettingsDaemon.cpp src/GMMediaPlayerService.cpp src/GMNotifyDaemon.cpp src/GMAppStatusNotify.cpp)
  set(HAVE_DBUS 1)
endif()


# Goggles Music Manager
add_executable(gogglesmm ${SOURCES})


set_target_properties(gogglesmm PROPERTIES ENABLE_EXPORTS 1)


# Include Directories
target_include_directories(gogglesmm PUBLIC
                           ${PROJECT_SOURCE_DIR}/src/gap/include
                           ${FOX_INCLUDE_DIRS}
                           ${TAGLIB_INCLUDE_DIRS}
                           ${DBUS_INCLUDE_DIRS})

# Libraries
target_link_libraries(gogglesmm  PUBLIC gap
                      ${FOX_LIBRARIES}
                      ${SQLITE_LIBRARIES}
                      ${TAGLIB_LIBRARIES}
                      ${DBUS_LIBRARIES}
                      ${GLU_LIBRARIES}
                      ${EPOXY_LIBRARIES}
                      ${X11_LIBRARIES}
                      ${SM_LIBRARIES}
                      ${ICE_LIBRARIES}
                      ${LIBGCRYPT_LIBRARIES})

install(TARGETS gogglesmm
        RUNTIME DESTINATION bin)







