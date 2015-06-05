cmake_minimum_required(VERSION 2.8)
project(GOGGLESMM)

# We need C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)


if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_definitions(-DDEBUG)
endif()


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
   message(STATUS "Libgcrypt found: ${LIBGCRYPT_LIBRARIES}")
endif()


#pkg_check_modules(FOX fox17>1.7.48)
find_package(PkgConfig)

#pkg_check_modules(GCRYPT gcrypt REQUIRED)


pkg_check_modules(FOX fox17>=1.7.48)
pkg_check_modules(DBUS dbus-1)
pkg_check_modules(TAGLIB taglib)
pkg_check_modules(SQLITE sqlite3)
pkg_check_modules(SM sm)
pkg_check_modules(ICE ice )
pkg_check_modules(X11 x11 )
pkg_check_modules(GLU glu )
pkg_check_modules(EPOXY epoxy)




#include_directories(src/gap/include ${FOX_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})

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
endif()


add_subdirectory(src/gap)

add_executable(gogglesmm ${SOURCES})
target_include_directories(gogglesmm PUBLIC ${PROJECT_SOURCE_DIR}/src/gap/include ${FOX_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS} ${DBUS_INCLUDE_DIRS} )
target_link_libraries(gogglesmm  PUBLIC gap ${FOX_LIBRARIES} ${SQLITE_LIBRARIES} ${TAGLIB_LIBRARIES} ${DBUS_LIBRARIES} ${GLU_LIBRARIES}
${EPOXY_LIBRARIES} ${X11_LIBRARIES} ${SM_LIBRARIES} ${ICE_LIBRARIES} ${LIBGCRYPT_LIBRARIES})


install(TARGETS gogglesmm
        RUNTIME DESTINATION bin)
#add_dependencies(gogglesmm gap)

#

