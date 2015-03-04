# This Makefile defines all sources

ICONS := icons/cursor_hand.gif \
icons/about.png \
icons/gogglesmm_16.png \
icons/gogglesmm_32.png \
icons/x16_accessories_text_editor.png \
icons/x16_application_rss_xml.png \
icons/x16_applications_internet.png \
icons/x16_audio_volume_high.png \
icons/x16_audio_volume_low.png \
icons/x16_audio_volume_medium.png \
icons/x16_audio_volume_muted.png \
icons/x16_audio_x_generic.png \
icons/x16_bookmark_new.png \
icons/x16_document_open.png \
icons/x16_document_save.png \
icons/x16_edit_copy.png \
icons/x16_edit_cut.png \
icons/x16_edit_delete.png \
icons/x16_edit_find.png \
icons/x16_edit_paste.png \
icons/x16_edit_undo.png \
icons/x16_exit.png \
icons/x16_folder.png \
icons/x16_folder_open.png \
icons/x16_go_bottom.png \
icons/x16_go_home.png \
icons/x16_help_browser.png \
icons/x16_image_x_generic.png \
icons/x16_list_add.png \
icons/x16_list_remove.png \
icons/x16_media_optical.png \
icons/x16_media_playback_pause.png \
icons/x16_media_playback_start.png \
icons/x16_media_playback_stop.png \
icons/x16_media_skip_backward.png \
icons/x16_media_skip_forward.png \
icons/x16_preferences_desktop.png \
icons/x16_process_working.png \
icons/x16_stock_attach.png \
icons/x16_status_error.png \
icons/x16_system_users.png \
icons/x16_text_x_generic.png \
icons/x16_view_refresh.png \
icons/x16_view_sort_descending.png \
icons/x16_window_close.png \
icons/x16_x_office_presentation.png \
icons/x22_applications_internet.png \
icons/x22_applications_multimedia.png \
icons/x22_applications_rss_xml.png \
icons/x22_audio_volume_high.png \
icons/x22_audio_volume_low.png \
icons/x22_audio_volume_medium.png \
icons/x22_audio_volume_muted.png \
icons/x22_audio_x_generic.png \
icons/x22_document_open.png \
icons/x22_document_properties.png \
icons/x22_drive_harddisk.png \
icons/x22_folder.png \
icons/x22_image_x_generic.png \
icons/x22_media_playback_pause.png \
icons/x22_media_playback_start.png \
icons/x22_media_playback_stop.png \
icons/x22_media_skip_backward.png \
icons/x22_media_skip_forward.png \
icons/x22_preferences_system.png \
icons/x22_text_x_generic.png \
icons/x22_user_bookmarks.png \
icons/x22_user_home.png \
icons/x22_x_office_presentation.png \
icons/x128_media_optical.png

GMM_SRC := src/GMAbout.cpp \
src/GMAnimImage.cpp \
src/GMApp.cpp \
src/GMAudioPlayer.cpp \
src/GMAudioScrobbler.cpp \
src/GMAlbumList.cpp \
src/GMClipboard.cpp \
src/GMColumnDialog.cpp \
src/GMCover.cpp \
src/GMCoverCache.cpp \
src/GMCoverManager.cpp \
src/GMDatabase.cpp \
src/GMDatabaseSource.cpp \
src/GMFilename.cpp \
src/GMFilter.cpp \
src/GMFilterEditor.cpp \
src/GMFilterSource.cpp \
src/GMFontDialog.cpp \
src/GMIconTheme.cpp \
src/GMImportDialog.cpp \
src/GMImageView.cpp \
src/GMList.cpp \
src/GMLocalSource.cpp \
src/GMPlayerManager.cpp \
src/GMPlayListSource.cpp \
src/GMPlayQueue.cpp \
src/GMPodcastSource.cpp \
src/GMPreferences.cpp \
src/GMPreferencesDialog.cpp \
src/GMPresenter.cpp \
src/GMRemote.cpp \
src/GMSession.cpp \
src/GMScanner.cpp \
src/GMSource.cpp \
src/GMSourceView.cpp \
src/GMTag.cpp \
src/GMTaskManager.cpp \
src/GMTrack.cpp \
src/GMTrackDatabase.cpp \
src/GMTrackEditor.cpp \
src/GMTrackList.cpp \
src/GMTrackItem.cpp \
src/GMTrackView.cpp \
src/GMTrayIcon.cpp \
src/GMStreamSource.cpp \
src/GMWindow.cpp \
src/main.cpp \
src/icons.cpp \
src/fxext.cpp \
src/gmutils.cpp

ifneq (,$(findstring md5,$(OPTIONS)))
	GMM_SRC += src/md5.cpp
endif

ifneq (,$(findstring dbus,$(OPTIONS)))
	GMM_SRC += src/GMDBus.cpp \
	src/GMAppStatusNotify.cpp \
	src/GMNotifyDaemon.cpp \
  src/GMSettingsDaemon.cpp \
  src/GMMediaPlayerService.cpp
endif


#----------------------------------------------------------

src/icons.h: $(ICONS)
	@echo "    Creating Icon Resource Header"
	@$(RESWRAP_H) -o $@ $(ICONS)

src/icons.cpp: $(ICONS)
	@echo "    Creating Icon Resources"
	@$(RESWRAP_CPP) -o $@ $(ICONS)


ifneq (,$(findstring dbus,$(OPTIONS)))
src/gogglesmm_xml.h: src/gogglesmm.xml
	@echo "    Creating gogglesmm_xml.h ..."
	@$(RESWRAP_TEXT) -o $@ src/gogglesmm.xml
src/appstatus_xml.h:  src/appstatus.xml src/dbusmenu.xml
	@echo "    Creating appstatus_xml.h ..."
	@$(RESWRAP_TEXT) -o $@  src/appstatus.xml  src/dbusmenu.xml
ifneq (,$(findstring mpris1,$(OPTIONS)))
src/mpris1_xml.h:  src/mpris.xml
	@echo "    Creating mpris1_xml.h ..."
	@$(RESWRAP_TEXT) -o $@ src/mpris.xml src/mpris_player.xml src/mpris_tracklist.xml
endif
ifneq (,$(findstring mpris2,$(OPTIONS)))
src/mpris2_xml.h:  src/mpris2.xml
	@echo "    Creating mpris2_xml.h ..."
	@$(RESWRAP_TEXT) -o $@ src/mpris2.xml
endif
endif

ifneq (,$(findstring dbus,$(OPTIONS)))
src/GMPlayerManager.cpp: src/gogglesmm_xml.h
src/GMAppStatusNotify.cpp: src/appstatus_xml.h
ifneq (,$(findstring mpris1,$(OPTIONS)))
ifneq (,$(findstring mpris2,$(OPTIONS)))
src/GMMediaPlayerService.cpp: src/mpris1_xml.h src/mpris2_xml.h
else
src/GMMediaPlayerService.cpp: src/mpris1_xml.h
endif
else
ifneq (,$(findstring mpris2,$(OPTIONS)))
src/GMMediaPlayerService.cpp: src/mpris2_xml.h
endif
endif
endif

#----------------------------------------------------------

GMM_OBJECTS := $(GMM_SRC:.cpp=$(OBJEXT))

$(GMM_OBJECTS): CPPFLAGS+=-Isrc/gap/include $(GMM_CPPFLAGS)
$(GMM_OBJECTS): CFLAGS+=$(GMM_CFLAGS)
$(GMM_NAME): LIBS+=$(ALL_LIBS)
$(GMM_NAME): LDFLAGS+=$(ALL_LDFLAGS)

$(GMM_OBJECTS): src/icons.h src/icons.cpp

# Link Libraries
$(GMM_NAME): $(GAP_OBJECTS) $(GMM_OBJECTS)
	@echo "    Linking $@ ..."
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$@ $^ $(LIBS)
