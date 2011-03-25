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
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

GAP_NAME=libgap-$(GAP_MAJOR).so
GAP_SONAME=$(GAP_NAME).$(GAP_ABI)
GAP_LIB=$(GAP_SONAME).$(GAP_MINOR)
GAP_LIBRARY=src/$(GAP_LIB)
GAP_INTERFACE=gap-$(GAP_MAJOR)

TARNAME=libgap-$(GAP_MAJOR).$(GAP_MINOR)

ifneq (,$(findstring alsa,$(PLUGINS)))
GAP_ALSA_NAME=libgap_plugin_alsa.so
GAP_ALSA_PLUGIN=src/$(GAP_ALSA_NAME)
$(GAP_ALSA_PLUGIN): CFLAGS+=$(ALSA_CFLAGS)
$(GAP_ALSA_PLUGIN): CPPFLAGS+=$(ALSA_CPPFLAGS)
endif

ifneq (,$(findstring oss,$(PLUGINS)))
GAP_OSS_NAME=libgap_plugin_oss.so
GAP_OSS_PLUGIN=src/$(GAP_OSS_NAME)
$(GAP_OSS_PLUGIN): CPPFLAGS+=$(OSS_CPPFLAGS)
$(GAP_OSS_PLUGIN): CFLAGS+=$(OSS_CFLAGS)
endif

ifneq (,$(findstring pulse,$(PLUGINS)))
GAP_PULSE_NAME=libgap_plugin_pulse.so
GAP_PULSE_PLUGIN=src/$(GAP_PULSE_NAME)
$(GAP_PULSE_PLUGIN): CPPFLAGS+=$(PULSE_CPPFLAGS)
$(GAP_PULSE_PLUGIN): CFLAGS+=$(PULSE_CFLAGS)
endif

ifneq (,$(findstring jack,$(PLUGINS)))
GAP_JACK_NAME=libgap_plugin_jack.so
GAP_JACK_PLUGIN=src/$(GAP_JACK_NAME)
$(GAP_JACK_PLUGIN): CPPFLAGS+=$(JACK_CPPFLAGS)
$(GAP_JACK_PLUGIN): CFLAGS+=$(JACK_CFLAGS)
endif

ifneq (,$(findstring rsound,$(PLUGINS)))
GAP_RSOUND_NAME=libgap_plugin_rsound.so
GAP_RSOUND_PLUGIN=src/$(GAP_RSOUND_NAME)
$(GAP_RSOUND_PLUGIN): CPPFLAGS+=$(RSOUND_CPPFLAGS)
$(GAP_RSOUND_PLUGIN): CFLAGS+=$(RSOUND_CFLAGS)
endif

GAP_WAV_NAME=libgap_plugin_wav.so
GAP_WAV_PLUGIN=src/$(GAP_WAV_NAME)
$(GAP_WAV_PLUGIN): CPPFLAGS+=$(WAV_CPPFLAGS)
$(GAP_WAV_PLUGIN): CFLAGS+=$(WAV_CFLAGS)


all: $(GAP_LIBRARY) $(GAP_ALSA_PLUGIN) $(GAP_OSS_PLUGIN) $(GAP_PULSE_PLUGIN) $(GAP_JACK_PLUGIN) $(GAP_RSOUND_PLUGIN) $(GAP_WAV_PLUGIN)
plugins: $(GAP_ALSA_PLUGIN) $(GAP_OSS_PLUGIN) $(GAP_PULSE_PLUGIN) $(GAP_JACK_PLUGIN) $(GAP_RSOUND_PLUGIN) $(GAP_WAV_PLUGIN)
library: $(GAP_LIBRARY)

libdir=$(DESTDIR)$(LIBDIR)
includedir=$(DESTDIR)$(INCLUDEDIR)


# Objects to Compile
#----------------------------------------------------------
GAP_SRCFILES := src/ap_event.cpp \
src/ap_packet.cpp \
src/ap_thread_queue.cpp \
src/ap_app_queue.cpp \
src/ap_pipe.cpp \
src/ap_utils.cpp \
src/ap_convert.cpp \
src/ap_format.cpp \
src/ap_device.cpp \
src/ap_decoder_plugin.cpp \
src/ap_thread.cpp \
src/ap_input_plugin.cpp \
src/ap_input_thread.cpp \
src/ap_decoder_thread.cpp \
src/ap_output_thread.cpp \
src/ap_engine.cpp \
src/ap_player.cpp \
src/ap_memory_buffer.cpp \
src/plugins/ap_wav_plugin.cpp \
src/plugins/ap_pcm_plugin.cpp

ifneq (,$(findstring musepack,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_musepack_plugin.cpp
endif

ifneq (,$(findstring vorbis,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_vorbis_plugin.cpp
endif

ifneq (,$(findstring ogg,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_ogg_plugin.cpp
endif

ifneq (,$(findstring mad,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_mad_plugin.cpp
endif

ifneq (,$(findstring flac,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_flac_plugin.cpp
endif

ifneq (,$(findstring aac,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_aac_plugin.cpp
endif

ifneq (,$(findstring cdda,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_cdda_plugin.cpp
endif


ifneq (,$(findstring avcodec,$(PLUGINS)))
GAP_SRCFILES+=src/plugins/ap_avc_plugin.cpp
endif

$(GAP_LIBRARY): CFLAGS+=$(PLUGIN_CFLAGS)
$(GAP_LIBRARY): CPPFLAGS+=$(PLUGIN_CPPFLAGS)
$(GAP_LIBRARY): LIBS+=$(PLUGIN_LIBS)

GAP_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(GAP_SRCFILES))
GAP_DEPENDENCIES = $(patsubst %.cpp,%.d,$(GAP_SRCFILES))

ALSA_SRCFILES=src/plugins/ap_alsa_plugin.cpp
ALSA_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(ALSA_SRCFILES))
ALSA_DEPENDENCIES = $(patsubst %.cpp,%.d,$(ALSA_SRCFILES))

OSS_SRCFILES=src/plugins/ap_oss_plugin.cpp
OSS_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(OSS_SRCFILES))
OSS_DEPENDENCIES = $(patsubst %.cpp,%.d,$(OSS_SRCFILES))

PULSE_SRCFILES=src/plugins/ap_pulse_plugin.cpp
PULSE_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(PULSE_SRCFILES))
PULSE_DEPENDENCIES = $(patsubst %.cpp,%.d,$(PULSE_SRCFILES))

JACK_SRCFILES=src/plugins/ap_jack_plugin.cpp
JACK_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(JACK_SRCFILES))
JACK_DEPENDENCIES = $(patsubst %.cpp,%.d,$(JACK_SRCFILES))

RSOUND_SRCFILES=src/plugins/ap_rsound_plugin.cpp
RSOUND_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(RSOUND_SRCFILES))
RSOUND_DEPENDENCIES = $(patsubst %.cpp,%.d,$(RSOUND_SRCFILES))

WAV_SRCFILES=src/plugins/ap_wavout_plugin.cpp
WAV_OBJECTS := $(patsubst %.cpp,%$(OBJEXT),$(WAV_SRCFILES))
WAV_DEPENDENCIES = $(patsubst %.cpp,%.d,$(WAV_SRCFILES))

%$(OBJEXT):	%.cpp
#	@echo "    Compiling $< ..."
#	@echo "$(CXX) $(CFLAGS) $(DEFS)  $(CPPFLAGS) -MM -o $*.d -MT $@ $<"
	@$(CXX) $(CFLAGS) $(CPPFLAGS) -MM -o $*.d -MT $@ $<
	@echo "$(CXX) $(strip $(CFLAGS)) $(strip $(CPPFLAGS)) $(OUTPUTOBJ)$@ -c $<"
	@$(CXX) $(strip $(CFLAGS)) $(strip $(CPPFLAGS)) $(OUTPUTOBJ)$@ -c $<

$(GAP_LIBRARY): $(GAP_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS)  $(SONAME)$(GAP_SONAME) $(OUTPUTBIN)$(GAP_LIBRARY)  $(strip $(GAP_OBJECTS)) $(strip $(LIBS))"
	@$(LINK) $(LDFLAGS) $(SONAME)$(GAP_SONAME) $(OUTPUTBIN)$(GAP_LIBRARY) $(strip $(GAP_OBJECTS)) $(strip $(LIBS))

$(GAP_ALSA_PLUGIN): $(ALSA_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_ALSA_PLUGIN) $(strip $(ALSA_OBJECTS)) $(strip $(ALSA_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_ALSA_PLUGIN) $(strip $(ALSA_OBJECTS)) $(strip $(ALSA_LIBS))

$(GAP_OSS_PLUGIN): $(OSS_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_OSS_PLUGIN) $(strip $(OSS_OBJECTS))  $(strip $(OSS_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_OSS_PLUGIN) $(strip $(OSS_OBJECTS))  $(strip $(OSS_LIBS))

$(GAP_PULSE_PLUGIN): $(PULSE_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_PULSE_PLUGIN) $(strip $(PULSE_OBJECTS)) $(strip $(PULSE_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_PULSE_PLUGIN) $(strip $(PULSE_OBJECTS)) $(strip $(PULSE_LIBS))

$(GAP_JACK_PLUGIN): $(JACK_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_JACK_PLUGIN) $(strip $(JACK_OBJECTS)) $(strip $(JACK_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_JACK_PLUGIN) $(strip $(JACK_OBJECTS)) $(strip $(JACK_LIBS))

$(GAP_RSOUND_PLUGIN): $(RSOUND_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_RSOUND_PLUGIN) $(strip $(RSOUND_OBJECTS)) $(strip $(RSOUND_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_RSOUND_PLUGIN) $(strip $(RSOUND_OBJECTS)) $(strip $(RSOUND_LIBS))

$(GAP_WAV_PLUGIN): $(WAV_OBJECTS)
	@echo "    Linking $@ ..."
	@echo "$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_WAV_PLUGIN) $(strip $(WAV_OBJECTS)) $(strip $(WAV_LIBS))"
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$(GAP_WAV_PLUGIN) $(strip $(WAV_OBJECTS)) $(strip $(WAV_LIBS))

# Force Dependency on config.make
$(GAP_OBJECTS) $(ALSA_OBJECTS) $(OSS_OBJECTS) $(PULSE_OBJECTS) $(JACK_OBJECTS) $(RSOUND_OBJECTS) $(WAV_OBJECTS): config.make


# Install
#----------------------------------------------------------
install: $(GAP_LIBRARY) $(GAP_ALSA_PLUGIN)  $(GAP_OSS_PLUGIN) $(GAP_PULSE_PLUGIN) $(GAP_JACK_PLUGIN) $(GAP_RSOUND_PLUGIN) $(GAP_WAV_PLUGIN)
	@echo "  Installing Library ..."
	@$(INSTALL) -d $(libdir)
	@$(INSTALL) $(GAP_LIBRARY) $(libdir)/$(GAP_LIB)

# Not sure if this belongs in make install
	@echo "  Installing Library Symbolic Links ..."
	@ldconfig -n $(libdir)
	@cd $(libdir) && ln --symbolic --force $(GAP_SONAME) $(GAP_NAME)

	@echo "  Installing Plugins ..."
	@$(INSTALL) -d $(libdir)/$(GAP_INTERFACE)
ifneq (,$(findstring alsa,$(PLUGINS)))
	@$(INSTALL) $(GAP_ALSA_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_ALSA_NAME)
endif
ifneq (,$(findstring oss,$(PLUGINS)))
	@$(INSTALL) $(GAP_OSS_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_OSS_NAME)
endif
ifneq (,$(findstring pulse,$(PLUGINS)))
	@$(INSTALL) $(GAP_PULSE_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_PULSE_NAME)
endif
ifneq (,$(findstring jack,$(PLUGINS)))
	@$(INSTALL) $(GAP_JACK_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_JACK_NAME)
endif
ifneq (,$(findstring rsound,$(PLUGINS)))
	@$(INSTALL) $(GAP_RSOUND_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_RSOUND_NAME)
endif
	@$(INSTALL) $(GAP_WAV_PLUGIN) $(libdir)/$(GAP_INTERFACE)/$(GAP_WAV_NAME)
	@echo "  Installing Headers ..."
	@$(INSTALL) -d $(includedir)/$(GAP_INTERFACE)
	@$(INSTALL) -m 644 include/ap.h $(includedir)/$(GAP_INTERFACE)/ap.h
	@$(INSTALL) -m 644 include/ap_event.h $(includedir)/$(GAP_INTERFACE)/ap_event.h
	@$(INSTALL) -m 644 include/ap_event_queue.h $(includedir)/$(GAP_INTERFACE)/ap_event_queue.h
	@$(INSTALL) -m 644 include/ap_app_queue.h $(includedir)/$(GAP_INTERFACE)/ap_app_queue.h
	@$(INSTALL) -m 644 include/ap_player.h $(includedir)/$(GAP_INTERFACE)/ap_player.h
	@$(INSTALL) -m 644 include/ap_version.h $(includedir)/$(GAP_INTERFACE)/ap_version.h
	@$(INSTALL) -m 644 include/ap_device.h $(includedir)/$(GAP_INTERFACE)/ap_device.h
	@echo "  Installing pkg-config ..."
	@$(INSTALL) -d $(libdir)/pkgconfig
	@$(INSTALL) -m 644 src/libgap-$(GAP_MAJOR).pc $(libdir)/pkgconfig/libgap-$(GAP_MAJOR).pc
	@echo "  Done!"

uninstall:
	@echo "  Removing Library ..."
	rm -f $(libdir)/$(GAP_LIB)
	rm -f $(libdir)/$(GAP_NAME)
	rm -f $(libdir)/$(GAP_SONAME)
	@echo "  Removing Plugins ..."
ifneq (,$(findstring alsa,$(PLUGINS)))
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_ALSA_NAME)
endif
ifneq (,$(findstring oss,$(PLUGINS)))
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_OSS_NAME)
endif
ifneq (,$(findstring pulse,$(PLUGINS)))
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_PULSE_NAME)
endif
ifneq (,$(findstring jack,$(PLUGINS)))
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_JACK_NAME)
endif
ifneq (,$(findstring rsound,$(PLUGINS)))
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_RSOUND_NAME)
endif
	rm -f $(libdir)/$(GAP_INTERFACE)/$(GAP_WAV_NAME)
	rmdir $(libdir)/$(GAP_INTERFACE)
	@echo "  Removing Headers ..."
	rm $(includedir)/$(GAP_INTERFACE)/*.h
	rmdir $(includedir)/$(GAP_INTERFACE)
	@echo "Removing pkg-config ..."
	rm $(libdir)/pkgconfig/libgap-$(GAP_MAJOR).pc
	rmdir $(libdir)/pkgconfig


# Clean
#----------------------------------------------------------
clean:
	@echo "  Remove Executables ..."
	@rm -f $(GAP_LIBRARY)
	@rm -f $(GAP_ALSA_PLUGIN)
	@rm -f $(GAP_OSS_PLUGIN)
	@rm -f $(GAP_PULSE_PLUGIN)
	@rm -f $(GAP_JACK_PLUGIN)
	@rm -f $(GAP_RSOUND_PLUGIN)
	@rm -f $(GAP_WAV_PLUGIN)

	@echo "  Remove Objects ..."
	@rm -f src/plugins/*$(OBJEXT)
	@rm -f src/plugins/*.d
	@rm -f src/*$(OBJEXT)
	@rm -f src/*.d
#----------------------------------------------------------

realclean:
	@echo "  Remove Configuration ..."
	@rm -f config.make
	@rm -f include/ap_version.h
	@rm -f src/ap_config.h
	@rm -f src/libgap-$(GAP_MAJOR).pc


dist: clean realclean
	@echo " Creating Tarbal .."
	tar --create --xz --file='../../$(TARNAME).tar.xz' --verbose --exclude-vcs --exclude='*.tar.xz' --transform='s/^./$(TARNAME)/' --show-transformed-names .


# How to make everything else
-include $(GAP_DEPENDENCIES) $(ALSA_DEPENDENCIES) $(OSS_DEPENDENCIES) $(PULSE_DEPENDENCIES) $(RSOUND_DEPENDENCIES) $(JACK_DEPENDENCIES) $(WAV_DEPENDENCIES)
