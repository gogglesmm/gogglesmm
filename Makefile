#-----------------------------------------------------------
# GOGGLES BUILD SYSTEM
#-----------------------------------------------------------
#
# The actual make file
#
#-----------------------------------------------------------

include config.make

# Set suffixes
.SUFFIXES:
.SUFFIXES: .cpp .h .gif .png $(OBJEXT) $(BINEXT) $(LIBEXT)

.PHONY : all clean realclean cleanicons install install-desktop

INSTALL=install


# Convert to Platform specific names
#----------------------------------------------------------
BINNAME=src/gogglesmm$(BINEXT)    						# XXX on Linux, X.exe on Windows

# Installation Directory
ifdef DESTDIR
INSTALLDIR=$(DESTDIR)$(PREFIX)
else
INSTALLDIR=$(PREFIX)
endif

all: $(BINNAME)

ICONS := icons/cursor_hand.gif \
icons/about.png \
icons/16x16_apps_musicmanager.png \
icons/32x32_apps_musicmanager.png

# Objects to Compile
#----------------------------------------------------------
SRCFILES := src/GMAbout.cpp \
src/GMAnimImage.cpp \
src/GMApp.cpp \
src/GMAudioPlayer.cpp \
src/GMAudioScrobbler.cpp \
src/GMAlbumList.cpp \
src/GMClipboard.cpp \
src/GMColumnDialog.cpp \
src/GMCover.cpp \
src/GMCoverThumbs.cpp \
src/GMDatabase.cpp \
src/GMDatabaseSource.cpp \
src/GMFilename.cpp \
src/GMFontDialog.cpp \
src/GMIconTheme.cpp \
src/GMImportDialog.cpp \
src/GMImageView.cpp \
src/GMList.cpp \
src/GMLocalSource.cpp \
src/GMPlayerManager.cpp \
src/GMPlayListSource.cpp \
src/GMPlayQueue.cpp \
src/GMPreferences.cpp \
src/GMPreferencesDialog.cpp \
src/GMRemote.cpp \
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
	SRCFILES += src/md5.cpp
endif

ifneq (,$(findstring dbus,$(OPTIONS)))
	SRCFILES += src/GMDBus.cpp \
	src/GMAppStatusNotify.cpp \
	src/GMNotifyDaemon.cpp \
  src/GMSettingsDaemon.cpp \
  src/GMMediaPlayerService.cpp
endif


OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(SRCFILES))
DEPENDENCIES = $(patsubst %.cpp,%.d,$(SRCFILES))

$(BINNAME): $(OBJECTS)
	@echo "    Linking $@ ..."
#	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(BINNAME) $(OBJECTS) $(LIBS)"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(BINNAME) $(OBJECTS) $(LIBS)

%$(OBJEXT):	%.cpp
	@echo "    Compiling $< ..."
#	@echo "$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) -MM -o $*.d -MT $@ $<"
	@$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) -MM -o $*.d -MT $@ $<
#	@echo "$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) $(OUTPUTOBJ)$@ -c $<"
	@$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) $(OUTPUTOBJ)$@ -c $<

ifneq (,$(findstring dbus,$(OPTIONS)))
src/gogglesmm_xml.h: src/gogglesmm.xml
	@echo "    Creating gogglesmm_xml.h ..."
	@$(RESWRAP_TEXT) -o $@ src/gogglesmm.xml
src/appstatus_xml.h:  src/appstatus.xml src/dbusmenu.xml
	@echo "    Creating appstatus_xml.h ..."
	@$(RESWRAP_TEXT) -o $@  src/appstatus.xml  src/dbusmenu.xml
#src/mpris_xml.h:  src/mpris.xml src/mpris_player.xml
#	@echo "    Creating DBUS Introspection..."
#	@$(RESWRAP_TEXT) -o $@ src/mpris.xml src/mpris_player.xml src/mpris_tracklist.xml
src/mpris2_xml.h:  src/mpris2.xml
	@echo "    Creating mpris2_xml.h ..."
	@$(RESWRAP_TEXT) -o $@ src/mpris2.xml
endif

$(OBJECTS): src/icons.h src/icons.cpp

src/icons.h: $(ICONS)
	@echo "    Creating Icon Resource Header"
	@$(RESWRAP_H) -o $@ $(ICONS)

src/icons.cpp: $(ICONS)
	@echo "    Creating Icon Resources"
	@$(RESWRAP_CPP) -o $@ $(ICONS)

ifneq (,$(findstring dbus,$(OPTIONS)))
src/GMPlayerManager.cpp: src/gogglesmm_xml.h
src/GMAppStatusNotify.cpp: src/appstatus_xml.h
src/GMMediaPlayerService.cpp: src/mpris2_xml.h
endif


TRANSLATIONS:=$(basename $(notdir $(wildcard po/*.mo)))
LINGUAS?=$(TRANSLATIONS)

# Install
#----------------------------------------------------------
install: $(BINNAME)
	@echo "    Installing $(INSTALLDIR)/bin/gogglesmm ..."
	@$(INSTALL) -d $(INSTALLDIR)/bin
	@$(INSTALL) -m 755 src/gogglesmm $(INSTALLDIR)/bin/gogglesmm
	@echo "    Installing $(INSTALLDIR)/share/applications/gogglesmm.desktop"
	@$(INSTALL) -d $(INSTALLDIR)/share/applications
	@$(INSTALL) -m 644 extra/gogglesmm.desktop $(INSTALLDIR)/share/applications/gogglesmm.desktop
	@echo "    Installing Icons"
	$(INSTALL) -m 644 -D icons/gogglesmm_16.png $(INSTALL_DIR)/share/icons/hicolor/16x16/apps/gogglesmm.png
	$(INSTALL) -m 644 -D extra/gogglesmm_22.png $(INSTALL_DIR)/share/icons/hicolor/22x22/apps/gogglesmm.png
	$(INSTALL) -m 644 -D extra/gogglesmm_24.png $(INSTALL_DIR)/share/icons/hicolor/24x24/apps/gogglesmm.png
	$(INSTALL) -m 644 -D icons/gogglesmm_32.png $(INSTALL_DIR)/share/icons/hicolor/32x32/apps/gogglesmm.png
	$(INSTALL) -m 644 -D extra/gogglesmm_48.png $(INSTALL_DIR)/share/icons/hicolor/48x48/apps/gogglesmm.png
	$(INSTALL) -m 644 -D extra/gogglesmm.svg $(INSTALL_DIR)/share/icons/hicolor/scalable/apps/gogglesmm.svg
ifneq (,$(findstring nls,$(OPTIONS)))
	@echo "    Installing Translations"
	@linguas='$(filter $(TRANSLATIONS),$(LINGUAS))'; \
	for tr in $$linguas ; do \
    echo "    Installing $(INSTALLDIR)/share/locale/$$tr/LC_MESSAGES/gogglesmm.mo" ;\
  	$(INSTALL) -m 644 -D po/$$tr.mo -T $(INSTALLDIR)/share/locale/$$tr/LC_MESSAGES/gogglesmm.mo ; \
	done;
endif

# Clean
#----------------------------------------------------------
clean :
	@echo "    Remove Executables ..."
	@rm -f $(BINNAME)
	@echo "    Remove Objects ..."
	@rm -f src/*$(OBJEXT)
	@rm -f src/*.d
	@echo "    Remove Generated Files ..."
	@rm -f src/icons.cpp
	@rm -f src/icons.h
	@rm -f src/gogglesmm_xml.h
	@rm -f src/mpris2_xml.h
	@rm -f src/appstatus_xml.h

#----------------------------------------------------------

realclean :
	@echo "    Remove Configuration ..."
	@rm -f config.make
	@rm -f src/gmconfig.h

dist: clean realclean
	sh build/makemo
	rm po/fi.mo
	rsvg-convert -w 22  extra/gogglesmm.svg -o extra/gogglesmm_22.png
	rsvg-convert -w 24  extra/gogglesmm.svg -o extra/gogglesmm_24.png
	rsvg-convert -w 48  extra/gogglesmm.svg -o extra/gogglesmm_48.png
	@echo " Creating Tarbals .."
	tar --create --xz --file='../../$(TARNAME).tar.xz' --verbose --exclude-vcs --exclude='*.tar.xz' --transform='s/^./$(TARNAME)/' --show-transformed-names .
	tar --create --bzip2 --file='../../$(TARNAME).tar.bz2' --verbose --exclude-vcs --exclude='*.tar.bz2' --transform='s/^./$(TARNAME)/' --show-transformed-names .


# Clean Icons
#----------------------------------------------------------
cleanicons :
	@rm -f src/icons.*
	@rm -f include/icons.*
#----------------------------------------------------------

# How to make everything else
-include $(DEPENDENCIES)
