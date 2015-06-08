cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)

project(GOGGLESMM VERSION 1.1.0)

option(WITH_OPENGL "Enable OpenGL Features" ON)
option(WITH_DBUS "Enable DBUS Features" ON)
option(WITH_NLS "Enable Native Language Support" ON)


TEST_BIG_ENDIAN(GOGGLESMM_BIGENDIAN)

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


# Detects reswrap version
find_program(RESWRAP reswrap)

if(RESWRAP)
    execute_process(COMMAND ${RESWRAP} -v OUTPUT_VARIABLE RESWRAP_OUTPUT)
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]" RESWRAP_VERSION "${RESWRAP_OUTPUT}")
    if(RESWRAP_VERSION VERSION_LESS "5.0")
        set(RESWRAP_H "${RESWRAP} -i -k")
        set(RESWRAP_CPP "${RESWRAP} -e -k")
        set(RESWRAP_TEXT "${RESWRAP} -t -k")
    else()
        set(RESWRAP_H ${RESWRAP} --keep-ext --header)
        set(RESWRAP_CPP ${RESWRAP} --keep-ext --source --extern)
        set(RESWRAP_TEXT ${RESWRAP} --keep-ext -t)
    endif()
else()
message(FATAL_ERROR "Unable to find reswrap")
endif()






#------------------------------------------------------------------------------

# We need C++ 11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Translation Files to share/locale
set(LOCALE_PATH ${CMAKE_INSTALL_PREFIX}/share/locale)

# Don't use CMAKE_INSTALL_PREFIX since most icons won't be installed anywhere but /usr
set(ICON_PATH /usr/share/icons)

#------------------------------------------------------------------------------

set(SOURCES
  GMAbout.cpp
  GMAnimImage.cpp
  GMApp.cpp
  GMAudioPlayer.cpp
  GMAudioScrobbler.cpp
  GMAlbumList.cpp
  GMClipboard.cpp
  GMColumnDialog.cpp
  GMCover.cpp
  GMCoverCache.cpp
  GMCoverManager.cpp
  GMDatabase.cpp
  GMDatabaseSource.cpp
  GMFilename.cpp
  GMFilter.cpp
  GMFilterEditor.cpp
  GMFilterSource.cpp
  GMFontDialog.cpp
  GMIconTheme.cpp
  GMImportDialog.cpp
  GMImageView.cpp
  GMList.cpp
  GMLocalSource.cpp
  GMPlayerManager.cpp
  GMPlayListSource.cpp
  GMPlayQueue.cpp
  GMPodcastSource.cpp
  GMPreferences.cpp
  GMPreferencesDialog.cpp
  GMPresenter.cpp
  GMRemote.cpp
  GMSession.cpp
  GMScanner.cpp
  GMSource.cpp
  GMSourceView.cpp
  GMTag.cpp
  GMTaskManager.cpp
  GMTrack.cpp
  GMTrackDatabase.cpp
  GMTrackEditor.cpp
  GMTrackList.cpp
  GMTrackItem.cpp
  GMTrackView.cpp
  GMTrayIcon.cpp
  GMStreamSource.cpp
  GMWindow.cpp
  main.cpp
  icons.cpp
  icons.h
  fxext.cpp
  gmutils.cpp
  )

set(ICONS ../icons/cursor_hand.gif
../icons/about.png
../icons/gogglesmm_16.png
../icons/gogglesmm_32.png
../icons/x16_accessories_text_editor.png
../icons/x16_application_rss_xml.png
../icons/x16_applications_internet.png
../icons/x16_audio_volume_high.png
../icons/x16_audio_volume_low.png
../icons/x16_audio_volume_medium.png
../icons/x16_audio_volume_muted.png
../icons/x16_audio_x_generic.png
../icons/x16_bookmark_new.png
../icons/x16_document_open.png
../icons/x16_document_save.png
../icons/x16_edit_copy.png
../icons/x16_edit_cut.png
../icons/x16_edit_delete.png
../icons/x16_edit_find.png
../icons/x16_edit_paste.png
../icons/x16_edit_undo.png
../icons/x16_exit.png
../icons/x16_folder.png
../icons/x16_folder_open.png
../icons/x16_go_bottom.png
../icons/x16_go_home.png
../icons/x16_help_browser.png
../icons/x16_image_x_generic.png
../icons/x16_list_add.png
../icons/x16_list_remove.png
../icons/x16_media_optical.png
../icons/x16_media_playback_pause.png
../icons/x16_media_playback_start.png
../icons/x16_media_playback_stop.png
../icons/x16_media_skip_backward.png
../icons/x16_media_skip_forward.png
../icons/x16_preferences_desktop.png
../icons/x16_process_working.png
../icons/x16_stock_attach.png
../icons/x16_status_error.png
../icons/x16_system_users.png
../icons/x16_text_x_generic.png
../icons/x16_view_refresh.png
../icons/x16_view_sort_descending.png
../icons/x16_window_close.png
../icons/x16_x_office_presentation.png
../icons/x22_applications_internet.png
../icons/x22_applications_multimedia.png
../icons/x22_applications_rss_xml.png
../icons/x22_audio_volume_high.png
../icons/x22_audio_volume_low.png
../icons/x22_audio_volume_medium.png
../icons/x22_audio_volume_muted.png
../icons/x22_audio_x_generic.png
../icons/x22_document_open.png
../icons/x22_document_properties.png
../icons/x22_drive_harddisk.png
../icons/x22_folder.png
../icons/x22_image_x_generic.png
../icons/x22_media_playback_pause.png
../icons/x22_media_playback_start.png
../icons/x22_media_playback_stop.png
../icons/x22_media_skip_backward.png
../icons/x22_media_skip_forward.png
../icons/x22_preferences_system.png
../icons/x22_text_x_generic.png
../icons/x22_user_bookmarks.png
../icons/x22_user_home.png
../icons/x22_x_office_presentation.png
../icons/x128_media_optical.png
)

set_source_files_properties(icons.cpp PROPERTIES GENERATED 1)
set_source_files_properties(icons.h PROPERTIES GENERATED 1)

ADD_CUSTOM_COMMAND(
    OUTPUT icons.h icons.cpp
    COMMAND ${RESWRAP_H} -o icons.h ${ICONS}
    COMMAND ${RESWRAP_CPP} -o icons.cpp ${ICONS}
    )

if (DBUS_FOUND)

    LIST(APPEND SOURCES GMDBus.cpp
                        GMSettingsDaemon.cpp
                        GMMediaPlayerService.cpp
                        GMNotifyDaemon.cpp
                        GMAppStatusNotify.cpp
                        gogglesmm_xml.h
                        appstatus_xml.h
                        mpris1_xml.h
                        mpris2_xml.h)

    set_source_files_properties(gogglesmm_xml.h PROPERTIES GENERATED 1)
    set_source_files_properties(appstatus_xml.h PROPERTIES GENERATED 1)
    set_source_files_properties(mpris1_xml.h PROPERTIES GENERATED 1)
    set_source_files_properties(mpris2_xml.h PROPERTIES GENERATED 1)

    ADD_CUSTOM_COMMAND(
        OUTPUT gogglesmm_xml.h appstatus_xml.h mpris1_xml.h mpris2_xml.h
        COMMAND ${RESWRAP_TEXT} -o gogglesmm_xml.h gogglesmm.xml
        COMMAND ${RESWRAP_TEXT} -o appstatus_xml.h appstatus.xml dbusmenu.xml
        COMMAND ${RESWRAP_TEXT} -o mpris1_xml.h mpris.xml mpris_player.xml mpris_tracklist.xml
        COMMAND ${RESWRAP_TEXT} -o mpris2_xml.h mpris2.xml
        )

    set(HAVE_DBUS 1)
endif()

# Write gmconfig.h
configure_file("gmconfig.h.in" "gmconfig.h")



# Goggles Music Manager
add_executable(gogglesmm ${SOURCES})




# Export Symbols if GAP is not build as a shared library
if(NOT BUILD_SHARED_LIBS)
    set_target_properties(gogglesmm PROPERTIES ENABLE_EXPORTS 1)
endif()

# Include Directories
target_include_directories(gogglesmm PRIVATE
                           ${FOX_INCLUDE_DIRS}
                           ${TAGLIB_INCLUDE_DIRS}
                           ${DBUS_INCLUDE_DIRS})

# Libraries
target_link_libraries(gogglesmm PRIVATE gap
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







