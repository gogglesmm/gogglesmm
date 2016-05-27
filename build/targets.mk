# Main Executable
GMM_NAME=src/gogglesmm$(BINEXT)    						# XXX on Linux, X.exe on Windows

# ALSA Output Plugin
ifneq (,$(findstring alsa,$(GAP_PLUGINS)))
GAP_ALSA_NAME=libgap_alsa.so
GAP_ALSA_PLUGIN=src/$(GAP_ALSA_NAME)
endif

# OSS Output Plugin
ifneq (,$(findstring oss,$(GAP_PLUGINS)))
GAP_OSS_NAME=libgap_oss.so
GAP_OSS_PLUGIN=src/$(GAP_OSS_NAME)
endif

# Pulse Output Plugin
ifneq (,$(findstring pulse,$(GAP_PLUGINS)))
GAP_PULSE_NAME=libgap_pulse.so
GAP_PULSE_PLUGIN=src/$(GAP_PULSE_NAME)
endif

# WAV Output Plugin
GAP_WAV_NAME=libgap_wav.so
GAP_WAV_PLUGIN=src/$(GAP_WAV_NAME)

# Shortcut to mushrooms
GAP_ALL_PLUGINS:=$(GAP_ALSA_PLUGIN) $(GAP_OSS_PLUGIN) $(GAP_PULSE_PLUGIN) $(GAP_JACK_PLUGIN) $(GAP_RSOUND_PLUGIN) $(GAP_WAV_PLUGIN)

