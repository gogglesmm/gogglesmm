# unix.cmake
# Unix-specific configuration for GAP (Goggles Audio Player)

include(CheckIncludeFiles)

# Find Threads (required by some upstream CMake configs like FLAC)
find_package(Threads REQUIRED)

#-------------------------------------------------------------------------------
# Unix-specific Build Options
#-------------------------------------------------------------------------------

# Output Plugins
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  option(WITH_OSS "OSS Output Support" OFF)
  option(WITH_ALSA "ALSA Output Support" ON)
else()
  option(WITH_OSS "OSS Output Support" ON)
  option(WITH_ALSA "ALSA Output Support" OFF)
endif()
option(WITH_PULSE "PulseAudio Output Support" ON)
option(WITH_JACK "Jack Output Support (currently not working)" OFF)
option(WITH_WAVOUT "WAV Output Support" ON)
option(WITH_SNDIO "Sndio Output Support" ON)

# Containers
option(WITH_OGG "Ogg File Support" ON)
option(WITH_MATROSKA "Matroska File Support" ON)
option(WITH_MP4 "MP4 File Support" ON)

# Codecs
option(WITH_VORBIS "Vorbis Codec Support" ON)
option(WITH_TREMOR "Vorbis Codec (using Tremor) Support" OFF)
option(WITH_FLAC "FLAC Codec Support" ON)
option(WITH_MAD "MP3 Codec Support" ON)
option(WITH_ALAC "ALAC Codec Support" ON)
option(WITH_FAAD "AAC Codec Support" ON)
option(WITH_OPUS "Opus Codec Support" ON)
option(WITH_A52 "A52 Codec Support" OFF)
option(WITH_DCA "DCA Codec Support" OFF)

# HTTP(s) + Zlib
option(WITH_OPENSSL "OpenSSL Support" ON)
option(WITH_GNUTLS "GnuTLS Support" ON)
option(WITH_GCRYPT "libgcrypt Support" ON)
option(WITH_ZLIB "zlib Support" ON)

option(WITH_STATIC_FOX "Pull in static library dependencies for FOX" OFF)

#-------------------------------------------------------------------------------
# Find Unix-specific Dependencies
#-------------------------------------------------------------------------------

# Audio Output Libraries
if(WITH_ALSA)
  find_package(ALSA)
endif()

if(WITH_PULSE)
  find_package(PulseAudio)
endif()

if(WITH_JACK)
  find_package(Jack)
endif()

if(WITH_SNDIO)
  find_package(Sndio)
endif()

if(WITH_OSS)
  check_cxx_symbol_exists(ioctl "sys/ioctl.h" HAVE_SYS_IOCTL_H)
  if(NOT HAVE_SYS_IOCTL_H)
    check_cxx_symbol_exists(ioctl "ioctl.h" HAVE_IOCTL_H)
  endif()

  check_cxx_symbol_exists(SNDCTL_DSP_SETFMT "sys/soundcard.h" HAVE_SYS_SOUNDCARD_H)
  if(NOT HAVE_SYS_SOUNDCARD_H)
    check_cxx_symbol_exists(SNDCTL_DSP_SETFMT, "soundcard.h" HAVE_SOUNDCARD_H)
  endif()

  if((HAVE_SYS_IOCTL_H OR HAVE_IOCTL_H) AND (HAVE_SYS_SOUNDCARD_H OR HAVE_SOUNDCARD_H))
    set(OSS_FOUND TRUE)
  endif()
endif()

# Audio Codec Libraries
if(WITH_DCA)
  find_package(DCA)
endif()

if(WITH_FLAC)
  find_package(FLAC)
endif()

if(WITH_OGG)
  find_package(Ogg)
  if(WITH_TREMOR)
    find_package(Tremor)
  elseif(WITH_VORBIS)
    find_package(Vorbis)
  endif()
  if(WITH_OPUS)
    find_package(Opus)
  endif()
endif()

# Compression Libraries
if(WITH_ZLIB)
  find_package(ZLIB)
endif()

# SSL/TLS Libraries
if(WITH_OPENSSL)
  find_package(OpenSSL 1.0.1)
elseif(WITH_GNUTLS)
  find_package(GnuTLS)
elseif(WITH_GCRYPT)
  find_package(Gcrypt)
endif()

# Audio Codec Libraries (continued)
if(WITH_MAD)
  find_package(MAD)
endif()

if(WITH_FAAD)
  find_package(FAAD)
endif()

if(WITH_A52)
  find_package(A52)
endif()

#-------------------------------------------------------------------------------
# Build Unix-specific Output Plugins
#-------------------------------------------------------------------------------

# Alsa Output
if (WITH_ALSA AND ALSA_FOUND)
  add_library(gap_alsa MODULE plugins/ap_alsa.cpp)
  target_link_libraries(gap_alsa ALSA::ALSA FX::FOX)
  target_include_directories(gap_alsa PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  install(TARGETS gap_alsa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  set(HAVE_ALSA 1)
endif()

# Pulse Output
if (WITH_PULSE AND PulseAudio_FOUND)
  add_library(gap_pulse MODULE plugins/ap_pulse.cpp)
  target_link_libraries(gap_pulse PulseAudio::PulseAudio FX::FOX)
  target_include_directories(gap_pulse PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  install(TARGETS gap_pulse LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  set(HAVE_PULSE 1)
endif()

# Jack Output
if (WITH_JACK AND Jack_FOUND)
  add_library(gap_jack MODULE plugins/ap_jack.cpp)
  target_link_libraries(gap_jack Jack::Jack FX::FOX)
  target_include_directories(gap_jack PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  install(TARGETS gap_jack LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  set(HAVE_JACK 1)
endif()

# Sndio Output
if (WITH_SNDIO AND Sndio_FOUND)
  add_library(gap_sndio MODULE plugins/ap_sndio.cpp)
  target_link_libraries(gap_sndio Sndio::Sndio FX::FOX)
  target_include_directories(gap_sndio PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  install(TARGETS gap_sndio LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  set(HAVE_SNDIO 1)
endif()

# OSS Output
if(WITH_OSS AND OSS_FOUND)
  add_library(gap_oss MODULE plugins/ap_oss_plugin.cpp)

  if(HAVE_SYS_IOCTL_H)
    target_compile_definitions(gap_oss PRIVATE HAVE_SYS_IOCTL_H)
  endif()
  if(HAVE_IOCTL_H)
    target_compile_definitions(gap_oss PRIVATE HAVE_IOCTL_H)
  endif()

  if(HAVE_SYS_SOUNDCARD_H)
    target_compile_definitions(gap_oss PRIVATE HAVE_SYS_SOUNDCARD_H)
  endif()
  if(HAVE_SOUNDCARD_H)
    target_compile_definitions(gap_oss PRIVATE HAVE_SOUNDCARD_H)
  endif()

  target_include_directories(gap_oss BEFORE PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(gap_oss FX::FOX)
  install(TARGETS gap_oss LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  set(HAVE_OSS 1)
endif()
