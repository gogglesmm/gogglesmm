cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

# set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

# Test only in Debug mode for now
if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_subdirectory(test)
endif()

# Third Party Modules
add_subdirectory(lib/alac)


# Goggles Audio Player
project(GAP VERSION 1.2.5)

# Update whenever ABI changes
set(GAP_SOVERSION_CURRENT 0)    # Increase whenever an interface has been added, removed or changed.
set(GAP_SOVERSION_REVISION 0)   # Always increase the revision value. Reset to 0 when Current changes.
set(GAP_SOVERSION_AGE 0)        # Increase the age value only if the changes made to the ABI are backward compatible.


set_package_properties(FOX PROPERTIES
                           URL "http://www.fox-toolkit.org"
                           PURPOSE "\tportability library"
                           TYPE REQUIRED)

set_package_properties(expat PROPERTIES
                            PURPOSE "\txml parser"
                            TYPE REQUIRED)


set_package_properties(flac PROPERTIES
                            URL "https://xiph.org/flac"
                            PURPOSE "\tFLAC Codec"
                            TYPE RECOMMENDED)

set_package_properties(ogg PROPERTIES
                            URL "https://www.xiph.org/ogg/"
                            PURPOSE "\tOgg File Input"
                            TYPE RECOMMENDED)

set_package_properties(vorbis PROPERTIES
                            URL "http://www.vorbis.com/"
                            PURPOSE "\tVorbis Codec"
                            TYPE RECOMMENDED)

set_package_properties(opus PROPERTIES
                            URL "http://www.opus-codec.org/"
                            PURPOSE "\tOpus Codec"
                            TYPE RECOMMENDED)

set_package_properties(mad PROPERTIES
                            URL "http://www.underbit.com/products/mad/"
                            PURPOSE "\tMP3 Codec"
                            TYPE OPTIONAL)

set_package_properties(faad PROPERTIES
                            URL "http://www.audiocoding.com/faad2.html"
                            PURPOSE "\tAAC Codec"
                            TYPE OPTIONAL)

set_package_properties(tremor PROPERTIES
                            URL "https://xiph.org/vorbis/"
                            PURPOSE "\tVorbis Codec"
                            TYPE OPTIONAL)

set_package_properties(alsa PROPERTIES
                            URL "http://www.alsa-project.org/"
                            PURPOSE "\tALSA Output"
                            TYPE OPTIONAL)

set_package_properties(pulse PROPERTIES
                            URL "https://www.freedesktop.org/wiki/Software/PulseAudio"
                            PURPOSE "\tPulseAudio Output"
                            TYPE OPTIONAL)

set_package_properties(sndio PROPERTIES
                            URL "http://www.sndio.org/"
                            PURPOSE "\tSndio Output"
                            TYPE OPTIONAL)

set_package_properties(openssl PROPERTIES
                            URL "https://www.openssl.org"
                            PURPOSE "\tSecure HTTP"
                            TYPE OPTIONAL)

set_package_properties(gnutls PROPERTIES
                            URL "https://www.gnutls.org"
                            PURPOSE "\tSecure HTTP"
                            TYPE OPTIONAL)

set_package_properties(gcrypt PROPERTIES
                            URL "https://www.gnu.org/software/libgcrypt"
                            PURPOSE "\tMD5 Hashing"
                            TYPE OPTIONAL)

set_package_properties(zlib PROPERTIES
                            URL "http://www.zlib.net/"
                            PURPOSE "\tZLIB decompression for http(s)"
                            TYPE OPTIONAL)

set_package_properties(samplerate PROPERTIES
                            URL "https://libsndfile.github.io/libsamplerate/"
			    PURPOSE "\tSample Rate Converter for audio"
                            TYPE OPTIONAL)

# Allow GAP to be build as a shared library.
if(MSVC)
  set(BUILD_GAP_SHARED_LIB ON CACHE BOOL "Build libgap as a shared library" FORCE)
else()
  set(BUILD_GAP_SHARED_LIB OFF CACHE BOOL "Build libgap as a shared library")
endif()

# libdir name to allow overriding to lib32 / lib64
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")

# Make CMAKE_INSTALL_LIBDIR relative to CMAKE_INSTALL_PREFIX
if(IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
  set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
  file(RELATIVE_PATH CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_LIBDIR})
else()
  set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
endif()

# Optional flags to turn on/off compilation of specific modules
if(UNIX)

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

  option(WITH_LIBSAMPLERATE "Samplerate support" OFF)
endif()

# Endiannes
TEST_BIG_ENDIAN(GAP_BIGENDIAN)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Expat
find_package(EXPAT REQUIRED)
LIST(APPEND LIBRARIES ${EXPAT_LIBRARIES})

# Third Party Libraries
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

  # FOX
  find_library(FOX_LIBRARIES NAMES FOX-1.7)
  find_path(FOX_INCLUDE_DIRS NAME fx.h PATH_SUFFIXES fox-1.7)

else()
  find_package(PkgConfig REQUIRED QUIET)

  if (WITH_CFOX)
    set(FOX_LIBRARIES cfox)
    set(FOX_INCLUDE_DIRS $<TARGET_PROPERTY:cfox,INTERFACE_INCLUDE_DIRECTORIES>)
    set_property(TARGET cfox PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  else()
    pkg_check_modules(FOX fox17>=1.7.50 REQUIRED)
  endif()

  if(WITH_ALSA)
    pkg_check_modules(ALSA alsa>=1.0)
  endif()

  if(WITH_PULSE)
    pkg_check_modules(PULSE libpulse>=0.9.21)
  endif()

  if(WITH_JACK)
    pkg_check_modules(JACK jack)
  endif()

  if(WITH_SNDIO)
    check_include_files(sndio.h HAVE_SNDIO_H)
    if(HAVE_SNDIO_H)
        find_library(SNDIO_LIBRARIES NAMES sndio)
        if(SNDIO_LIBRARIES)
            set(SNDIO_FOUND 1)
        endif()
    endif()
  endif()

  if(WITH_DCA)
    pkg_check_modules(DCA libdca)
  endif()

  if(WITH_FLAC)
    pkg_check_modules(FLAC flac)
  endif()

  if(WITH_OGG)
    pkg_check_modules(OGG ogg>=1.0)
    if(WITH_TREMOR)
      pkg_check_modules(TREMOR vorbisidec>=1.2)
    elseif(WITH_VORBIS)
      pkg_check_modules(VORBIS vorbis>=1.2)
    endif()
    if(WITH_OPUS)
      pkg_check_modules(OPUS opus)
    endif()
  endif()

  if(WITH_ZLIB)
    pkg_check_modules(ZLIB zlib)
  endif()

  if(WITH_OPENSSL)
    pkg_check_modules(OPENSSL openssl>=1.0.1)
  elseif(WITH_GNUTLS)
    pkg_check_modules(GNUTLS gnutls>=3.4.0)
  elseif(WITH_GCRYPT)
    check_include_files(gcrypt.h HAVE_GCRYPT_H)
    if (HAVE_GCRYPT_H)
      find_library(LIBGCRYPT_LIBRARIES NAMES gcrypt )
      if (LIBGCRYPT_LIBRARIES)
        set(GCRYPT_FOUND 1)
      endif()
    endif()
  endif()
  if(WITH_LIBSAMPLERATE)
    pkg_check_modules(SAMPLERATE samplerate)
  endif()
endif()


# Check for libgrypt


# Check for libmad
if(WITH_MAD)
  check_include_files(mad.h HAVE_MAD_H)
  if (HAVE_MAD_H)
    find_library(MAD_LIBRARIES NAMES mad )
    if (MAD_LIBRARIES)
      set(MAD_FOUND TRUE)
    endif()
  endif()
endif()


# Check for faad
if(WITH_FAAD)
  check_include_files(faad.h HAVE_FAAD_H)
  if (HAVE_FAAD_H)
    find_library(FAAD_LIBRARIES NAMES faad )
    if (FAAD_LIBRARIES)
      set(FAAD_FOUND TRUE)
    endif()
  endif()
endif()


# Check for a52
if(WITH_A52)
  find_path(HAVE_A52_H a52dec/a52.h)
  if (HAVE_A52_H)
      find_library(A52_LIBRARIES NAMES a52)
      if (A52_LIBRARIES)
          set(A52_FOUND TRUE)
          message(STATUS "liba52dec found ${A52_LIBRARIES}")
      endif()
  endif()
endif()

#-------------------------------------------------------------------------------

set(SOURCES ap_app_queue.cpp
            ap_buffer.cpp
            ap_buffer_io.cpp
            ap_connect.cpp
            ap_convert.cpp
            ap_crossfader.cpp
            ap_decoder_plugin.cpp
            ap_decoder_thread.cpp
            ap_device.cpp
            ap_engine.cpp
            ap_event.cpp
            ap_format.cpp
            ap_http_client.cpp
            ap_http_response.cpp
            ap_input_plugin.cpp
            ap_input_thread.cpp
            ap_output_thread.cpp
            ap_packet.cpp
            ap_player.cpp
            ap_reactor.cpp
            ap_reader_plugin.cpp
            ap_signal.cpp
            ap_socket.cpp
            ap_thread.cpp
            ap_thread_queue.cpp
            ap_utils.cpp
            ap_xml_parser.cpp
            )

set(HEADERS ap_buffer.h
            ap_config.h
            ap_connect.h
            ap_convert.h
            ap_crossfader.h
            ap_decoder_plugin.h
            ap_decoder_thread.h
            ap_defs.h
            ap_engine.h
            ap_event_private.h
            ap_format.h
            ap_input_plugin.h
            ap_input_thread.h
            ap_output_plugin.h
            ap_output_thread.h
            ap_packet.h
            ap_reactor.h
            ap_reader_plugin.h
            ap_signal.h
            ap_socket.h
            ap_thread.h
            ap_thread_queue.h
            ap_utils.h
            )

set(PUBLIC_HEADERS include/ap.h
                   include/ap_app_queue.h
                   include/ap_buffer_base.h
                   include/ap_buffer_io.h
                   include/ap_common.h
                   include/ap_device.h
                   include/ap_event.h
                   include/ap_event_queue.h
                   include/ap_http.h
                   include/ap_http_client.h
                   include/ap_http_response.h
                   include/ap_player.h
                   include/ap_xml_parser.h
                   )


set(PLUGIN_SOURCES plugins/ap_aiff.cpp
                   plugins/ap_file.cpp
                   plugins/ap_http.cpp
                   plugins/ap_m3u.cpp
                   plugins/ap_pcm.cpp
                   plugins/ap_pls.cpp
                   plugins/ap_wav.cpp
                   plugins/ap_xspf.cpp
                   )

set(OUTPUT_SOURCES plugins/ap_alsa.cpp
                   plugins/ap_jack.cpp
                   plugins/ap_oss_plugin.cpp
                   plugins/ap_pulse.cpp
                   plugins/ap_sndio.cpp
                   plugins/ap_wavout.cpp)

#if(WIN32)
  if(BUILD_GAP_SHARED_LIB)
    add_definitions(-DGAP_DLL)
    set(GAP_PLUGIN_LINK_TARGET gap)
  else()
    set(GAP_PLUGIN_LINK_TARGET gogglesmm)
  endif()
#endif()

# Link to FOX during shared library build
if(BUILD_GAP_SHARED_LIB)
  LIST(APPEND LIBRARIES ${FOX_LIBRARIES})
endif()

# Buildin Alac Decoder
if(WITH_ALAC AND WITH_MP4)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_alac.cpp)
  LIST(APPEND GAP_TARGET_OBJECTS $<TARGET_OBJECTS:gap_alac>)
  set(HAVE_ALAC 1)
endif()

if(FLAC_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_flac.cpp)
  LIST(APPEND LIBRARIES ${FLAC_LIBRARIES})
  set(HAVE_FLAC 1)
endif()

# Vorbis and Opus Decoders only with Ogg or Matroska.
if(WITH_OGG AND OGG_FOUND)

  if(WITH_TREMOR AND TREMOR_FOUND)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_vorbis.h)
    LIST(APPEND LIBRARIES ${TREMOR_LIBRARIES})
    set(HAVE_TREMOR 1)
  elseif(WITH_VORBIS AND VORBIS_FOUND)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_vorbis.h)
    LIST(APPEND LIBRARIES ${VORBIS_LIBRARIES})
    set(HAVE_VORBIS 1)
  endif()

  if(WITH_OPUS AND OPUS_FOUND)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_opus.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_opus.h)
    LIST(APPEND LIBRARIES ${OPUS_LIBRARIES})
    set(HAVE_OPUS 1)
  endif()

  if((WITH_OGG AND OGG_FOUND) AND (HAVE_TREMOR OR HAVE_VORBIS OR HAVE_FLAC OR HAVE_OPUS))
    LIST(APPEND PLUGIN_SOURCES plugins/ap_ogg.cpp plugins/ap_ogg_decoder.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_ogg_decoder.h)
    LIST(APPEND LIBRARIES ${OGG_LIBRARIES})
    set(HAVE_OGG 1)
  endif()
endif()

if(MAD_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_mad.cpp)
  LIST(APPEND LIBRARIES ${MAD_LIBRARIES})
  set(HAVE_MAD 1)
endif()

if(FAAD_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_aac.cpp)
  LIST(APPEND LIBRARIES ${FAAD_LIBRARIES})
  set(HAVE_FAAD 1)
endif()

# MP4 only supports AAC and ALAC
if(WITH_MP4 AND HAVE_FAAD OR HAVE_ALAC)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_mp4.cpp)
  set(HAVE_MP4 1)
endif()

if(WITH_MATROSKA)

  # Buildin Matroska Reader: supports raw PCM, so always include.
  LIST(APPEND PLUGIN_SOURCES plugins/ap_matroska.cpp)
  set(HAVE_MATROSKA 1)

  if(DCA_FOUND)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_dca.cpp)
    LIST(APPEND LIBRARIES ${DCA_LIBRARIES})
    set(HAVE_DCA 1)
  endif()

  if(A52_FOUND)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_a52.cpp)
    LIST(APPEND LIBRARIES ${A52_LIBRARIES})
    set(HAVE_A52 1)
  endif()

endif()

if(HAVE_FLAC OR HAVE_MAD)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_id3v2.cpp)
  LIST(APPEND PLUGIN_HEADERS plugins/ap_id3v2.h)
endif()

if(WITH_ZLIB AND ZLIB_FOUND)
  LIST(APPEND LIBRARIES ${ZLIB_LIBRARIES})
  set(HAVE_ZLIB 1)
endif()

if(WITH_OPENSSL AND OPENSSL_FOUND)
  LIST(APPEND LIBRARIES ${OPENSSL_LIBRARIES})
  set(HAVE_OPENSSL 1 CACHE INTERNAL "" FORCE)
elseif(WITH_GNUTLS AND GNUTLS_FOUND)
  LIST(APPEND LIBRARIES ${GNUTLS_LIBRARIES})
  set(HAVE_GNUTLS 1)
elseif(WITH_GCRYPT AND GCRYPT_FOUND)
  LIST(APPEND LIBRARIES ${LIBGCRYPT_LIBRARIES})
  set(HAVE_GCRYPT 1)
else()
  add_subdirectory(lib/md5)
  LIST(APPEND GAP_TARGET_OBJECTS $<TARGET_OBJECTS:gap_md5>)
  set(MD5_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/md5)
endif()

if(WITH_LIBSAMPLERATE AND SAMPLERATE_FOUND)
  LIST(APPEND LIBRARIES ${SAMPLERATE_LIBRARIES})
  set(HAVE_SAMPLERATE 1)
endif()


#-------------------------------------------------------------------------------

# GAP Plugins Library
add_library(gap_plugins OBJECT ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
target_include_directories(gap_plugins BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(gap_plugins PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib/alac)
if(BUILD_GAP_SHARED_LIB)
  set_property(TARGET gap_alac PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  set_property(TARGET gap_plugins PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  if(TARGET gap_md5)
    set_property(TARGET gap_md5 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  endif()
endif()


#-------------------------------------------------------------------------------


# Wav File output
if(WITH_WAVOUT)
  add_library(gap_wav MODULE plugins/ap_wavout.cpp)
  target_include_directories(gap_wav PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
  install(TARGETS gap_wav LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
  if(MSVC)
    target_link_libraries(gap_wav ${FOX_LIBRARIES} ${GAP_PLUGIN_LINK_TARGET})
  endif()
endif()

if(NOT WIN32)

  # Alsa Output
  if (WITH_ALSA AND ALSA_FOUND)
    add_library(gap_alsa MODULE plugins/ap_alsa.cpp)
    target_link_libraries(gap_alsa ${ALSA_LIBRARIES})
    target_include_directories(gap_alsa PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_alsa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_ALSA 1)
  endif()

  # Pulse Output
  if (WITH_PULSE AND PULSE_FOUND)
    add_library(gap_pulse MODULE plugins/ap_pulse.cpp)
    target_link_libraries(gap_pulse ${PULSE_LIBRARIES})
    target_include_directories(gap_pulse PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_pulse LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_PULSE 1)
  endif()

  # Jack Output
  if (WITH_JACK AND JACK_FOUND)
    add_library(gap_jack MODULE plugins/ap_jack.cpp)
    target_link_libraries(gap_jack ${PULSE_LIBRARIES})
    target_include_directories(gap_jack PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_jack LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_JACK 1)
  endif()

  # Sndio Output
  if (WITH_SNDIO AND SNDIO_FOUND)
    add_library(gap_sndio MODULE plugins/ap_sndio.cpp)
    target_link_libraries(gap_sndio ${SNDIO_LIBRARIES})
    target_include_directories(gap_sndio PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_sndio LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_SNDIO 1)
  endif()
endif()


if(UNIX AND WITH_OSS)
    add_library(gap_oss MODULE plugins/ap_oss_plugin.cpp)
    target_compile_options(gap_oss PRIVATE -idirafter /usr/include/sys)
    target_include_directories(gap_oss BEFORE PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_oss LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_OSS 1)
endif()


#-------------------------------------------------------------------------------

# Output Plugins
add_feature_info(pulse HAVE_PULSE "${PULSE_VERSION}")
add_feature_info(alsa HAVE_ALSA "${ALSA_VERSION}")
add_feature_info(jack HAVE_JACK "${JACK_VERSION}")
add_feature_info(oss HAVE_OSS "OSS Output")
add_feature_info(sndio HAVE_SNDIO "Sndio Output")
add_feature_info(wav WITH_WAVOUT "WAV File Output")

# Containers
add_feature_info(ogg HAVE_OGG "${OGG_VERSION}")
add_feature_info(mp4 HAVE_MP4 "MP4 File Input")
add_feature_info(matroska HAVE_MATROSKA "Matroska File Input")

# Codecs
add_feature_info(vorbis HAVE_VORBIS "${VORBIS_VERSION}")
add_feature_info(tremor HAVE_TREMOR "${TREMOR_VERSION}")
add_feature_info(opus HAVE_OPUS "${OPUS_VERSION}")
add_feature_info(mad HAVE_MAD "")
add_feature_info(faad HAVE_FAAD "")
add_feature_info(flac HAVE_FLAC "${FLAC_VERSION}")
add_feature_info(alac HAVE_ALAC "ALAC Codec")
add_feature_info(dca HAVE_DCA "DTS Coherent Acoustics codec")
add_feature_info(a52 HAVE_A52 "ATSC A/52 stream codec")

# Http
add_feature_info(zlib HAVE_ZLIB "${ZLIB_VERSION}")
add_feature_info(openssl HAVE_OPENSSL "${OPENSSL_VERSION}")
add_feature_info(gnutls HAVE_GNUTLS "${GNUTLS_VERSION}")
add_feature_info(gcrypt HAVE_GCRYPT "")

add_feature_info(samplerate HAVE_SAMPLERATE "${SAMPLERATE_VERSION}")

set(AP_PLUGIN_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/gogglesmm)

configure_file(ap_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/ap_config.h)

#-------------------------------------------------------------------------------
#
# Workaround to compile with earlier versions of FOX, something
# that cannot be easily handled using the preprocessor#
#
if (NOT WITH_CFOX AND FOX_VERSION VERSION_LESS "1.7.80")
  foreach(sourcefile IN LISTS PLUGIN_SOURCES OUTPUT_SOURCES SOURCES)
    file(READ ${sourcefile} content)
    string(REPLACE "FXString::compare" "FX::compare" updated "${content}")
    string(REPLACE "comparenatural" "compareversion" updated "${updated}")
    file(WRITE ${sourcefile} "${updated}")
  endforeach()
endif()

#-------------------------------------------------------------------------------

# Gap Library
if(BUILD_GAP_SHARED_LIB)
  add_library(gap SHARED ${SOURCES} ${HEADERS} ${PUBLIC_HEADERS} $<TARGET_OBJECTS:gap_plugins> ${GAP_TARGET_OBJECTS})
else()
  add_library(gap STATIC ${SOURCES} ${HEADERS} ${PUBLIC_HEADERS} $<TARGET_OBJECTS:gap_plugins> ${GAP_TARGET_OBJECTS})
endif()
set_target_properties(gap PROPERTIES ENABLE_EXPORTS 1)
target_include_directories(gap PRIVATE ${FOX_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include ${MD5_INCLUDE_DIRS})
target_include_directories(gap INTERFACE ${FOX_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(gap PRIVATE ${LIBRARIES})

if(NOT WITH_CFOX)
  if(WITH_STATIC_FOX)
    target_link_libraries(gap INTERFACE ${FOX_STATIC_LDFLAGS})
  else()
    target_link_libraries(gap INTERFACE ${FOX_LDFLAGS})
  endif()
else()
  target_link_libraries(gap INTERFACE cfox)
endif()


if(BUILD_GAP_SHARED_LIB)

  if(WIN32)
    target_compile_definitions(gap INTERFACE -DGAP_DLL)
  endif()

  #set_target_properties(gap PROPERTIES VERSION ${GAP_VERSION} SOVERSION ${GAP_SOVERSION})
  install(TARGETS gap LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES include/ap.h
                include/ap_app_queue.h
                include/ap_buffer_base.h
                include/ap_buffer_io.h
                include/ap_common.h
                include/ap_device.h
                include/ap_event.h
                include/ap_event_queue.h
                include/ap_http.h
                include/ap_http_client.h
                include/ap_http_response.h
                include/ap_player.h
                include/ap_xml_parser.h DESTINATION include/gap)
endif()
