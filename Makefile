#-----------------------------------------------------------
# GOGGLES BUILD SYSTEM
#-----------------------------------------------------------
#
# The actual make file
#
#-----------------------------------------------------------

.PHONY : all plugins clean realclean cleanicons install install-desktop nls

ifeq "$(wildcard config.make)" ""
  $(error No configuration found. Run ./configure first)
endif

include config.make

# Set suffixes
.SUFFIXES:
.SUFFIXES: .cpp .h .gif .png $(OBJEXT) $(BINEXT) $(LIBEXT)

# Set Installation directories
installdir=$(DESTDIR)$(PREFIX)
bindir=$(installdir)/bin
libdir=$(DESTDIR)$(LIBDIR)
sharedir=$(installdir)/share
localefile=$(sharedir)/locale/$(tr)/LC_MESSAGES/gogglesmm.mo
pluginfile=$(libdir)/gogglesmm/$(notdir $(plugin))
mandir=$(sharedir)/man
icondir=$(sharedir)/icons


# Install Utility
INSTALL=install

# Define Targets
include build/targets.mk

# Build Targets
all: $(GAP_ALL_PLUGINS) $(GMM_NAME) 

# Get Sources and Rules
include build/gap.mk
include build/gmm.mk

# Setup Translations
TRANSLATIONS:=$(basename $(notdir $(wildcard po/*.mo)))
LINGUAS?=$(TRANSLATIONS)

# Install Variables

# Install
#----------------------------------------------------------
install: $(GMM_NAME) $(GAP_ALL_PLUGINS)

	@echo "    Installing $(bindir)/gogglesmm ..."
	@$(INSTALL) -D -m 755 src/gogglesmm $(bindir)/gogglesmm
	@$(foreach plugin,$(GAP_ALL_PLUGINS),echo "    Installing $(pluginfile) ..." ; $(INSTALL) -D -m 755  $(plugin) $(pluginfile);)

	@echo "    Installing $(sharedir)/applications/gogglesmm.desktop"
	@$(INSTALL) -D -m 644 extra/gogglesmm.desktop $(sharedir)/applications/gogglesmm.desktop

	@echo "    Installing $(sharedir)/gogglesmm/utils"
	@$(INSTALL) -D -m 644 extra/import_banshee_stats.py $(sharedir)/gogglesmm/utils/import_banshee_stats.py
	@$(INSTALL) -D -m 644 extra/import_gogglesmm12_stats.py $(sharedir)/gogglesmm/utils/import_gogglesmm12_stats.py

	@echo "    Installing $(mandir)/man1/gogglesmm.1"
	@$(INSTALL) -m 644 -D extra/gogglesmm.1 $(mandir)/man1/gogglesmm.1

# install nls if needed
ifneq (,$(findstring nls,$(OPTIONS)))
	@$(foreach tr,$(filter $(TRANSLATIONS),$(LINGUAS)),echo "    Installing $(localefile) ..." ;$(INSTALL) -m 644 -D po/$(tr).mo $(localefile); )
endif

	@echo "    Installing icons..."
	@$(INSTALL) -m 644 -D icons/gogglesmm_16.png $(icondir)/hicolor/16x16/apps/gogglesmm.png
	@$(INSTALL) -m 644 -D extra/gogglesmm_22.png $(icondir)/hicolor/22x22/apps/gogglesmm.png
	@$(INSTALL) -m 644 -D extra/gogglesmm_24.png $(icondir)/hicolor/24x24/apps/gogglesmm.png
	@$(INSTALL) -m 644 -D icons/gogglesmm_32.png $(icondir)/hicolor/32x32/apps/gogglesmm.png
	@$(INSTALL) -m 644 -D extra/gogglesmm_48.png $(icondir)/hicolor/48x48/apps/gogglesmm.png
	@$(INSTALL) -m 644 -D extra/gogglesmm.svg $(icondir)/hicolor/scalable/apps/gogglesmm.svg
	@echo "    Done."

svg2png:
	rsvg-convert -w 22  extra/gogglesmm.svg -o extra/gogglesmm_22.png
	rsvg-convert -w 24  extra/gogglesmm.svg -o extra/gogglesmm_24.png
	rsvg-convert -w 48  extra/gogglesmm.svg -o extra/gogglesmm_48.png


# Clean
#----------------------------------------------------------
clean :
	@echo "    Remove Executables ..."
	@rm -f $(GMM_NAME) $(GAP_ALL_PLUGINS)
	@echo "    Remove Objects ..."
	@rm -f src/*$(OBJEXT)
	@rm -f src/*.d
	@rm -f src/gap/*$(OBJEXT)
	@rm -f src/gap/*.d
	@rm -f src/gap/plugins/*$(OBJEXT)
	@rm -f src/gap/plugins/*.d
	@echo "    Remove Generated Files ..."
	@rm -f src/icons.cpp
	@rm -f src/icons.h
	@rm -f src/gogglesmm_xml.h
	@rm -f src/mpris1_xml.h
	@rm -f src/mpris2_xml.h
	@rm -f src/appstatus_xml.h

#----------------------------------------------------------

realclean : clean
	@echo "    Remove Configuration ..."
	@rm -f config.make
	@rm -f src/gmconfig.h
	@rm -f src/gap/ap_config.h
	@rm -f po/*.mo
  
PO_FILES=$(wildcard po/*.po)
MO_FILES=$(PO_FILES:.po=.mo)

# Make mo files
%.mo: %.po
	msgfmt $< -o $@ 

nls: $(MO_FILES)

dist: svg2png clean realclean nls
	@echo " Creating Tarbals .."
	tar --create --xz --file='../../$(TARNAME).tar.xz' --verbose --exclude-vcs --exclude='*.tar.xz' --transform='s/^./$(TARNAME)/' --show-transformed-names .

# Clean Icons
cleanicons : $(MO_FILES)
	@rm -f src/icons.*
	@rm -f include/icons.*

# Compile to object
%$(OBJEXT):	%.cpp
	@echo "    Compiling $< ..."
	@$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) -MM -o $*.d -MT $@ $<
	@$(CXX) $(CFLAGS) $(DEFS) $(CPPFLAGS) $(OUTPUTOBJ)$@ -c $<

# How to make everything else
-include $(DEPENDENCIES)
