# Goggles Audio Player
#----------------------------------------------------------

GAP_SRC = src/gap/ap_event.cpp \
src/gap/ap_wait_io.cpp \
src/gap/ap_buffer_io.cpp \
src/gap/ap_packet.cpp \
src/gap/ap_thread_queue.cpp \
src/gap/ap_app_queue.cpp \
src/gap/ap_reactor.cpp \
src/gap/ap_pipe.cpp \
src/gap/ap_socket.cpp \
src/gap/ap_connect.cpp \
src/gap/ap_utils.cpp \
src/gap/ap_convert.cpp \
src/gap/ap_format.cpp \
src/gap/ap_device.cpp \
src/gap/ap_decoder_plugin.cpp \
src/gap/ap_thread.cpp \
src/gap/ap_input_plugin.cpp \
src/gap/ap_input_thread.cpp \
src/gap/ap_decoder_thread.cpp \
src/gap/ap_output_thread.cpp \
src/gap/ap_engine.cpp \
src/gap/ap_reader_plugin.cpp \
src/gap/ap_player.cpp \
src/gap/ap_buffer.cpp \
src/gap/ap_xml_parser.cpp \
src/gap/ap_http_response.cpp \
src/gap/ap_http_client.cpp \
src/gap/plugins/ap_m3u_plugin.cpp \
src/gap/plugins/ap_pls_plugin.cpp \
src/gap/plugins/ap_xspf_plugin.cpp \
src/gap/plugins/ap_wav_plugin.cpp \
src/gap/plugins/ap_aiff_plugin.cpp \
src/gap/plugins/ap_pcm_plugin.cpp \
src/gap/plugins/ap_file_plugin.cpp \
src/gap/plugins/ap_http_plugin.cpp \
src/gap/plugins/ap_id3v2.cpp

ifneq (,$(findstring musepack,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_musepack_plugin.cpp
endif

ifneq (,$(findstring vorbis,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_vorbis_plugin.cpp
endif

ifneq (,$(findstring tremor,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_vorbis_plugin.cpp
endif

ifneq (,$(findstring opus,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_opus_plugin.cpp
endif

ifneq (,$(findstring ogg,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_ogg_plugin.cpp \
src/gap/plugins/ap_ogg_decoder.cpp
endif

ifneq (,$(findstring mad,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_mad_plugin.cpp
src/gap/plugins/ap_mad_plugin$(OBJEXT): CPPFLAGS+=$(MAD_CPPFLAGS)
endif

ifneq (,$(findstring wavpack,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_wavpack_plugin.cpp
endif

ifneq (,$(findstring flac,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_flac_plugin.cpp
src/gap/plugins/ap_flac_plugin$(OBJEXT): CPPFLAGS+=$(FLAC_CPPFLAGS)
src/gap/plugins/ap_flac_plugin$(OBJEXT): CFLAGS+=$(FLAC_CFLAGS)
endif

ifneq (,$(findstring mp4,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_mp4.cpp
endif

ifneq (,$(findstring aac,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_aac_plugin.cpp
endif

ifneq (,$(findstring cdda,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_cdda_plugin.cpp
endif

ifneq (,$(findstring mms,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_mms_plugin.cpp
endif

ifneq (,$(findstring smb,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_smb_plugin.cpp
endif

ifneq (,$(findstring avcodec,$(GAP_PLUGINS)))
GAP_SRC+=src/gap/plugins/ap_avc_plugin.cpp src/plugins/ap_asf_plugin.cpp src/plugins/ap_asx_plugin.cpp
endif

#----------------------------------------------------------

# Default Include Search Path
GAP_INCS=-Isrc/gap -Isrc/gap/include

src/gap/ap_xml_parser$(OBJEXT): CPPFLAGS+=$(XML_CPPFLAGS)
src/gap/ap_xml_parser$(OBJEXT): CFLAGS+=$(XML_CFLAGS)

GAP_OBJECTS:=$(GAP_SRC:.cpp=$(OBJEXT))
$(GAP_OBJECTS): CPPFLAGS+=$(GAP_INCS) $(GAP_CPPFLAGS)

$(GAP_ALSA_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(ALSA_CPPFLAGS)
$(GAP_ALSA_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(ALSA_CFLAGS)
$(GAP_ALSA_PLUGIN): LDFLAGS+=$(SO_LDFLAGS) $(ALSA_LDFLAGS)
$(GAP_ALSA_PLUGIN): LIBS+=$(ALSA_LIBS)

$(GAP_OSS_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(OSS_CPPFLAGS)
$(GAP_OSS_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(OSS_CFLAGS)
$(GAP_OSS_PLUGIN): LDFLAGS+=$(SO_LDFLAGS) $(OSS_LDFLAGS)
$(GAP_OSS_PLUGIN): LIBS+=$(OSS_LIBS)

$(GAP_PULSE_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(PULSE_CPPFLAGS)
$(GAP_PULSE_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(PULSE_CFLAGS)
$(GAP_PULSE_PLUGIN): LDFLAGS+=$(SO_LDFLAGS) $(PULSE_LDFLAGS)
$(GAP_PULSE_PLUGIN): LIBS+=$(PULSE_LIBS)

$(GAP_JACK_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(JACK_CPPFLAGS)
$(GAP_JACK_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(JACK_CFLAGS)
$(GAP_JACK_PLUGIN): LDFLAGS+=$(SO_LDFLAGS) $(JACK_LDFLAGS)
$(GAP_JACK_PLUGIN): LIBS+=$(JACK_LIBS)

$(GAP_RSOUND_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(RSOUND_CPPFLAGS)
$(GAP_RSOUND_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(RSOUND_CFLAGS)
$(GAP_RSOUND_PLUGIN): LDFLAGS+=$(SO_LDFLAGS) $(RSOUND_LDFLAGS)
$(GAP_RSOUND_PLUGIN): LIBS+=$(RSOUND_LIBS)

$(GAP_WAV_PLUGIN): CPPFLAGS+=$(GAP_INCS) $(WAV_CPPFLAGS)
$(GAP_WAV_PLUGIN): CFLAGS+=$(SO_CFLAGS) $(WAV_CFLAGS)
$(GAP_WAV_PLUGIN): LDFLAGS+=$(SO_LDFLAGS)

ALSA_SRC=src/gap/plugins/ap_alsa_plugin.cpp
OSS_SRC=src/gap/plugins/ap_oss_plugin.cpp
PULSE_SRC=src/gap/plugins/ap_pulse_plugin.cpp
JACK_SRC=src/gap/plugins/ap_jack_plugin.cpp
RSOUND_SRC=src/gap/plugins/ap_rsound_plugin.cpp
WAV_SRC=src/gap/plugins/ap_wavout_plugin.cpp

$(GAP_ALSA_PLUGIN): $(ALSA_SRC:.cpp=$(OBJEXT))
$(GAP_OSS_PLUGIN): $(OSS_SRC:.cpp=$(OBJEXT))
$(GAP_PULSE_PLUGIN): $(PULSE_SRC:.cpp=$(OBJEXT))
$(GAP_JACK_PLUGIN): $(JACK_SRC:.cpp=$(OBJEXT))
$(GAP_RSOUND_PLUGIN): $(RSOUND_SRC:.cpp=$(OBJEXT))
$(GAP_WAV_PLUGIN): $(WAV_SRC:.cpp=$(OBJEXT))

# Link Libraries
%.so:
	@echo "    Linking $@ ..."
	@$(LINK) $(LDFLAGS) $(OUTPUTBIN)$@ $^ $(LIBS)
