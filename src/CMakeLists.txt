cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

project(GOGGLESMM VERSION 1.2.3)


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

find_package(PkgConfig)

if(NOT WITH_CFOX)
  pkg_check_modules(FOX REQUIRED fox17>=1.7.50 )
else()
  set(CFOX cfox)
endif()

pkg_check_modules(TAGLIB REQUIRED taglib>=1.9.0)
pkg_check_modules(SQLITE REQUIRED sqlite3>=3.6.19)

pkg_check_modules(SM sm)
pkg_check_modules(ICE ice)
pkg_check_modules(X11 x11)

if(WITH_DBUS)
  pkg_check_modules(DBUS dbus-1)
endif()

if(WITH_OPENGL)
  pkg_check_modules(EPOXY REQUIRED epoxy)
  pkg_check_modules(GLU REQUIRED glu)
  set(HAVE_OPENGL 1)
endif()

if(WITH_NLS)
  find_package(Intl REQUIRED)
  set(HAVE_NLS 1)
endif()


if(WITH_CFOX)
  set(RESWRAP $<TARGET_FILE:reswrap>)
  set(RESWRAP_H ${RESWRAP} --keep-ext --header)
  set(RESWRAP_CPP ${RESWRAP} --keep-ext --source --extern)
  set(RESWRAP_TEXT ${RESWRAP} --keep-ext -t)
else()
  find_program(RESWRAP NAMES reswrap-1.7 reswrap fox-reswrap)
  if(RESWRAP)
    message(STATUS "Found reswrap: ${RESWRAP}")
    execute_process(COMMAND ${RESWRAP} -v OUTPUT_VARIABLE RESWRAP_OUTPUT)
    string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]" RESWRAP_VERSION "${RESWRAP_OUTPUT}")
    if(RESWRAP_VERSION VERSION_LESS "5.0")
      set(RESWRAP_H ${RESWRAP} -i -k)
      set(RESWRAP_CPP ${RESWRAP} -e -k)
      set(RESWRAP_TEXT ${RESWRAP} -t -k)
    else()
      set(RESWRAP_H ${RESWRAP} --keep-ext --header)
      set(RESWRAP_CPP ${RESWRAP} --keep-ext --source --extern)
      set(RESWRAP_TEXT ${RESWRAP} --keep-ext -t)
    endif()
  else()
    message(FATAL_ERROR "Unable to find reswrap")
  endif()
endif()


#------------------------------------------------------------------------------

set(SOURCES GMAbout.cpp
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
            GMLyrics.cpp
            GMPlayerManager.cpp
            GMPlayListSource.cpp
            GMPlayQueue.cpp
            GMPodcastSource.cpp
            GMPreferences.cpp
            GMPreferencesDialog.cpp
            GMPresenter.cpp
            GMRemote.cpp
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
            fxext.cpp
            gmutils.cpp)

set(HEADERS GMAbout.h
            GMAnimImage.h
            GMApp.h
            GMAudioPlayer.h
            GMAudioScrobbler.h
            GMAlbumList.h
            GMClipboard.h
            GMColumnDialog.h
            GMCover.h
            GMCoverCache.h
            GMCoverManager.h
            GMDatabase.h
            GMDatabaseSource.h
            GMFilename.h
            GMFilter.h
            GMFilterEditor.h
            GMFilterSource.h
            GMFontDialog.h
            GMIconTheme.h
            GMImportDialog.h
            GMImageView.h
            GMList.h
            GMLocalSource.h
            GMLyrics.h
            GMPlayerManager.h
            GMPlayListSource.h
            GMPlayQueue.h
            GMPodcastSource.h
            GMPreferences.h
            GMPreferencesDialog.h
            GMPresenter.h
            GMRemote.h
            GMSession.h
            GMScanner.h
            GMSource.h
            GMSourceView.h
            GMTag.h
            GMTaskManager.h
            GMTrack.h
            GMTrackDatabase.h
            GMTrackEditor.h
            GMTrackList.h
            GMTrackItem.h
            GMTrackView.h
            GMTrayIcon.h
            GMStreamSource.h
            GMWindow.h
            icons.h
            fxext.h
            gmdefs.h
            gmutils.h)

set(ICONS cursor_hand.gif
          about.png
          gogglesmm_16.png
          gogglesmm_32.png
          x16_accessories_text_editor.png
          x16_application_rss_xml.png
          x16_applications_internet.png
          x16_audio_volume_high.png
          x16_audio_volume_low.png
          x16_audio_volume_medium.png
          x16_audio_volume_muted.png
          x16_audio_x_generic.png
          x16_bookmark_new.png
          x16_document_open.png
          x16_document_save.png
          x16_edit_copy.png
          x16_edit_cut.png
          x16_edit_delete.png
          x16_edit_find.png
          x16_edit_paste.png
          x16_edit_undo.png
          x16_exit.png
          x16_folder.png
          x16_folder_open.png
          x16_go_bottom.png
          x16_go_home.png
          x16_help_browser.png
          x16_image_x_generic.png
          x16_list_add.png
          x16_list_remove.png
          x16_media_optical.png
          x16_media_playback_pause.png
          x16_media_playback_start.png
          x16_media_playback_stop.png
          x16_media_skip_backward.png
          x16_media_skip_forward.png
          x16_preferences_desktop.png
          x16_process_working.png
          x16_stock_attach.png
          x16_status_error.png
          x16_system_users.png
          x16_text_x_generic.png
          x16_view_refresh.png
          x16_view_sort_descending.png
          x16_window_close.png
          x16_x_office_presentation.png
          x22_applications_internet.png
          x22_applications_multimedia.png
          x22_applications_rss_xml.png
          x22_audio_volume_high.png
          x22_audio_volume_low.png
          x22_audio_volume_medium.png
          x22_audio_volume_muted.png
          x22_audio_x_generic.png
          x22_document_open.png
          x22_document_properties.png
          x22_drive_harddisk.png
          x22_folder.png
          x22_image_x_generic.png
          x22_media_playback_pause.png
          x22_media_playback_start.png
          x22_media_playback_stop.png
          x22_media_skip_backward.png
          x22_media_skip_forward.png
          x22_preferences_system.png
          x22_text_x_generic.png
          x22_user_bookmarks.png
          x22_user_home.png
          x22_x_office_presentation.png
          x128_media_optical.png)



if(WITH_SESSION AND SM_FOUND AND ICE_FOUND)
  list(APPEND SOURCES GMSession.cpp)
  set(HAVE_SESSION 1)
endif()



add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/icons.h ${CMAKE_CURRENT_SOURCE_DIR}/icons.cpp
  COMMAND ${RESWRAP_H} -o ${CMAKE_CURRENT_SOURCE_DIR}/icons.h ${ICONS}
  COMMAND ${RESWRAP_CPP} -o ${CMAKE_CURRENT_SOURCE_DIR}/icons.cpp ${ICONS}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/icons
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

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/mpris1_xml.h ${CMAKE_CURRENT_SOURCE_DIR}/mpris2_xml.h
    COMMAND ${RESWRAP_TEXT} -o mpris1_xml.h mpris.xml mpris_player.xml mpris_tracklist.xml
    COMMAND ${RESWRAP_TEXT} -o mpris2_xml.h mpris2.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
    )
  set(HAVE_DBUS 1)
endif()

#-------------------------------------------------------------------------------
#
# Workaround to compile with earlier versions of FOX, something
# that cannot be easily handled using the preprocessor#
#
if (NOT WITH_CFOX AND FOX_VERSION VERSION_LESS "1.7.80")
  message("Replacing occurences of FXString::compare")
  foreach(sourcefile IN LISTS PLUGIN_SOURCES SOURCES)
    file(READ ${sourcefile} content)
    string(REPLACE "FXString::compare" "FX::compare" updated "${content}")
    string(REPLACE "comparenatural" "compareversion" updated "${updated}")
    file(WRITE ${sourcefile} "${updated}")
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

# GAP outplut plugins need symbols from the gap library. If we build GAP as a static library,
# gogglesmm needs to export those symbols instead.
if(NOT BUILD_GAP_SHARED_LIB)
  message(STATUS "gogglesmm: enable exports (libgap static library)")
  set_target_properties(gogglesmm PROPERTIES ENABLE_EXPORTS 1)
endif()

# Include Directories
target_include_directories(gogglesmm PRIVATE
                           ${FOX_INCLUDE_DIRS}
                           ${TAGLIB_INCLUDE_DIRS}
                           ${DBUS_INCLUDE_DIRS})

# Libraries
target_link_libraries(gogglesmm PRIVATE gap ${CFOX}
                      ${SQLITE_LIBRARIES}
                      ${TAGLIB_LIBRARIES}
                      ${DBUS_LIBRARIES}
                      ${GLU_LIBRARIES}
                      ${EPOXY_LIBRARIES}
                      ${X11_LIBRARIES}
                      ${SM_LIBRARIES}
                      ${ICE_LIBRARIES}
                      ${Intl_LIBRARIES})

install(TARGETS gogglesmm RUNTIME DESTINATION bin)