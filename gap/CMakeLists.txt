cmake_minimum_required(VERSION 3.3.1 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)

# Third Party Modules
add_subdirectory(lib/alac)

# Goggles Audio Player
project(GAP VERSION 1.1.1)

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
                            PURPOSE "\tflac playback support"
                            TYPE RECOMMENDED)

set_package_properties(ogg PROPERTIES
                            URL "https://www.xiph.org/ogg/"
                            PURPOSE "\togg container support (used by vorbis, opus and flac codecs)"
                            TYPE RECOMMENDED)

set_package_properties(vorbis PROPERTIES
                            URL "http://www.vorbis.com/"
                            PURPOSE "\togg vorbis playback support"
                            TYPE RECOMMENDED)

set_package_properties(opus PROPERTIES
                            URL "http://www.opus-codec.org/"
                            PURPOSE "\topus playback support"
                            TYPE RECOMMENDED)

set_package_properties(mad PROPERTIES
                            URL "http://www.underbit.com/products/mad/"
                            PURPOSE "\tmp3 playback support"
                            TYPE OPTIONAL)

set_package_properties(faad PROPERTIES
                            URL "http://www.audiocoding.com/faad2.html"
                            PURPOSE "\tmp4 / aac playback support"
                            TYPE OPTIONAL)

set_package_properties(tremor PROPERTIES
                            URL "https://xiph.org/vorbis/"
                            PURPOSE "\togg vorbis playback support"
                            TYPE OPTIONAL)

set_package_properties(alsa PROPERTIES
                            URL "http://www.alsa-project.org/"
                            PURPOSE "\talsa output support"
                            TYPE OPTIONAL)

set_package_properties(pulse PROPERTIES
                            URL "https://www.freedesktop.org/wiki/Software/PulseAudio"
                            PURPOSE "\tpulseaudio output support"
                            TYPE OPTIONAL)

set_package_properties(oss PROPERTIES
                            PURPOSE "\toss output support"
                            TYPE OPTIONAL)


# Allow GAP to be build as a shared library.
if(WIN32 AND MSVC)
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
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(WITH_OSS "OSS Output Support" OFF)
    option(WITH_ALSA "ALSA Output Support" ON)
  else()
    option(WITH_OSS "OSS Output Support" ON)
    option(WITH_ALSA "ALSA Output Support" OFF)
  endif()
  option(WITH_TREMOR "Tremor Support" OFF)
endif()

# Endiannes
TEST_BIG_ENDIAN(GAP_BIGENDIAN)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


# Set Global Debug Flags
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS $<$<OR:$<CONFIG:DEBUG>,$<CONFIG:RELWITHDEBINFO>>:DEBUG>)


# Expat
find_package(EXPAT REQUIRED)
LIST(APPEND LIBRARIES ${EXPAT_LIBRARIES})

# Third Party Libraries
if(WIN32)

  # FOX
  find_library(FOX_LIBRARIES NAMES FOXDLL-1.7)
  find_path(FOX_INCLUDE_DIRS NAMES fx.h PATH_SUFFIXES fox-1.7)

  add_definitions(-DXML_STATIC -DFOXDLL)	


  list(APPEND LIBRARIES ${FOX_LIBRARIES} ws2_32.lib)	
else()
  find_package(PkgConfig REQUIRED QUIET)
  pkg_check_modules(FOX fox17>=1.7.50 REQUIRED)
  pkg_check_modules(ALSA alsa>=1.0)
  pkg_check_modules(PULSE libpulse>=0.9.21)
  pkg_check_modules(FLAC flac)
  pkg_check_modules(VORBIS vorbis>=1.2)
  pkg_check_modules(OGG ogg>=1.0)
  pkg_check_modules(OPUS opus)
  pkg_check_modules(TREMOR vorbisidec>=1.2)
  #pkg_check_modules(SAMPLERATE samplerate)
  # We need
  #pkg_check_modules(JACK jack)
  #pkg_check_modules(CDDA libcdio_cdda)
  #pkg_check_modules(SMB smbclient)
endif()


# Check for libmad
find_path(HAVE_MAD_H NAMES mad.h)
message(STATUS "mad.h ${HAVE_MAD_H}")
#check_include_files(mad.h HAVE_MAD_H)
if (HAVE_MAD_H)
  find_library(MAD_LIBRARIES NAMES mad libmad.lib)
  if (MAD_LIBRARIES)
    set(MAD_FOUND TRUE)
  endif()
endif()


# Check for faad
check_include_files(faad.h HAVE_FAAD_H)
if (HAVE_FAAD_H)
  find_library(FAAD_LIBRARIES NAMES faad )
  if (FAAD_LIBRARIES)
    set(FAAD_FOUND TRUE)
  endif()
endif()

#-------------------------------------------------------------------------------

set(SOURCES ap_app_queue.cpp
            ap_buffer.cpp
            ap_buffer_io.cpp
            ap_connect.cpp
            ap_convert.cpp
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
            ap_pipe.cpp
            ap_player.cpp
            ap_reactor.cpp
            ap_reader_plugin.cpp
            ap_socket.cpp
            ap_thread.cpp
            ap_thread_queue.cpp
            ap_utils.cpp
            ap_wait_io.cpp
            ap_xml_parser.cpp
            )

set(HEADERS ap_buffer.h
            ap_config.h
            ap_connect.h
            ap_convert.h
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
            ap_pipe.h
            ap_reactor.h
            ap_reader_plugin.h
            ap_socket.h
            ap_thread.h
            ap_thread_queue.h
            ap_utils.h
            ap_wait_io.h
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
                   plugins/ap_alac.cpp
                   plugins/ap_file.cpp
                   plugins/ap_http.cpp
                   plugins/ap_id3v2.cpp
                   plugins/ap_m3u.cpp
                   plugins/ap_mp4.cpp
                   plugins/ap_pcm.cpp
                   plugins/ap_pls.cpp
                   plugins/ap_wav.cpp
                   plugins/ap_xspf.cpp
                   )

set(HAVE_ALAC 1)
set(HAVE_MP4 1)

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


if(FLAC_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_flac.cpp)
  LIST(APPEND LIBRARIES ${FLAC_LIBRARIES})
  set(HAVE_FLAC 1)
endif()

if(TREMOR_FOUND AND OGG_FOUND AND WITH_TREMOR)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
  LIST(APPEND LIBRARIES ${TREMOR_LIBRARIES})
  set(HAVE_TREMOR 1)
endif()

if(VORBIS_FOUND AND OGG_FOUND AND NOT WITH_TREMOR)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
  LIST(APPEND LIBRARIES ${VORBIS_LIBRARIES})
  set(HAVE_VORBIS 1)
endif()


if(OGG_FOUND AND (FLAC_FOUND OR OPUS_FOUND OR VORBIS_FOUND OR TREMOR_FOUND))
  LIST(APPEND PLUGIN_SOURCES plugins/ap_ogg.cpp plugins/ap_ogg_decoder.cpp)
  LIST(APPEND LIBRARIES ${OGG_LIBRARIES})
  set(HAVE_OGG 1)
endif()


if(OPUS_FOUND AND OGG_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_opus.cpp)
  LIST(APPEND LIBRARIES ${OPUS_LIBRARIES})
  set(HAVE_OPUS 1)
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

if(CDDA_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_cdda.cpp)
  LIST(APPEND LIBRARIES ${CDDA_LIBRARIES})
  set(HAVE_CDDA 1)
endif()

if(SMB_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_smb.cpp)
  LIST(APPEND LIBRARIES ${SMB_LIBRARIES})
  set(HAVE_SMB 1)
endif()

if(SAMPLERATE_FOUND)
  LIST(APPEND LIBRARIES ${SAMPLERATE_LIBRARIES})
  set(HAVE_SAMPLERATE 1)
endif()


#-------------------------------------------------------------------------------

# GAP Plugins Library
add_library(gap_plugins OBJECT ${PLUGIN_SOURCES})
target_include_directories(gap_plugins BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(gap_plugins PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/lib/alac)

if(BUILD_GAP_SHARED_LIB)
  set_property(TARGET gap_alac PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  set_property(TARGET gap_plugins PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()


#add_executable(gogglesmm IMPORTED GLOBAL)
#set_target_properties(gogglesmm PROPERTIES ENABLE_EXPORTS 1)

# Wav File output
add_library(gap_wav MODULE plugins/ap_wavout.cpp)
target_include_directories(gap_wav BEFORE PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})

# Depend on symbls in main executable
if(MSVC)
  target_link_libraries(gap_wav ${FOX_LIBRARIES} ${GAP_PLUGIN_LINK_TARGET})
endif()
install(TARGETS gap_wav LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)

if(WIN32)
add_library(gap_directx MODULE plugins/ap_directx.cpp)
target_include_directories(gap_directx BEFORE PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
target_link_libraries(gap_directx PRIVATE ${FOX_LIBRARIES} dsound.lib dxguid.lib ${GAP_PLUGIN_LINK_TARGET})
install(TARGETS gap_directx LIBRARY DESTINATION bin)


add_library(gap_wmm MODULE plugins/ap_wmm.cpp)
target_include_directories(gap_wmm BEFORE PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
target_link_libraries(gap_wmm PRIVATE ${FOX_LIBRARIES} winmm.lib ${GAP_PLUGIN_LINK_TARGET})
install(TARGETS gap_wmm LIBRARY DESTINATION bin)
endif()


#-------------------------------------------------------------------------------

if(NOT WIN32)

  # Alsa Output
  if (ALSA_FOUND AND WITH_ALSA)
    add_library(gap_alsa MODULE plugins/ap_alsa.cpp)
    target_link_libraries(gap_alsa ${ALSA_LIBRARIES})
    target_include_directories(gap_alsa PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_alsa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_ALSA 1)
  endif()

  # Pulse Output
  if (PULSE_FOUND)
    add_library(gap_pulse MODULE plugins/ap_pulse.cpp)
    target_link_libraries(gap_pulse ${PULSE_LIBRARIES})
    target_include_directories(gap_pulse PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_pulse LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_PULSE 1)
  endif()

  # Jack Output
  if (JACK_FOUND)
    add_library(gap_jack MODULE plugins/ap_jack.cpp)
    target_link_libraries(gap_jack ${PULSE_LIBRARIES})
    target_include_directories(gap_jack PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS})
    install(TARGETS gap_jack LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
    set(HAVE_JACK 1)
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

add_feature_info(wav ON "Wav File Ouput")
add_feature_info(pulse HAVE_PULSE "PulseAudio")
add_feature_info(alsa HAVE_ALSA "Advanced Linux Sound Architecture")
add_feature_info(oss HAVE_OSS "OSS Output")
add_feature_info(jack HAVE_JACK "Jack Output")
add_feature_info(vorbis HAVE_VORBIS "Vorbis Decoder")
add_feature_info(tremor HAVE_TREMOR "Vorbis Decoder")
add_feature_info(ogg HAVE_OGG "Ogg Parser")
add_feature_info(faad HAVE_FAAD "MPEG-4 and MPEG-2 AAC decoder")
add_feature_info(mad HAVE_MAD "MPEG audio decoder")
add_feature_info(opus HAVE_OPUS "Opus Decoder")
add_feature_info(alac HAVE_ALAC "ALAC Decoder")
add_feature_info(mp4 HAVE_MP4 "MP4 Parser")
add_feature_info(flac HAVE_FLAC "Free Lossless Audio Codec")

set(AP_PLUGIN_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/gogglesmm)

configure_file(ap_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/ap_config.h)


# Gap Library
if(BUILD_GAP_SHARED_LIB)
  add_library(gap SHARED ${SOURCES} ${HEADERS} ${PUBLIC_HEADERS} $<TARGET_OBJECTS:gap_plugins> $<TARGET_OBJECTS:gap_alac>)
else()
  add_library(gap STATIC ${SOURCES} ${HEADERS} ${PUBLIC_HEADERS} $<TARGET_OBJECTS:gap_plugins> $<TARGET_OBJECTS:gap_alac>)
endif()
set_target_properties(gap PROPERTIES ENABLE_EXPORTS 1)


target_include_directories(gap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS})
target_include_directories(gap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(gap PRIVATE ${LIBRARIES})
target_compile_definitions(gap PRIVATE GAP_BUILD)

if(BUILD_GAP_SHARED_LIB)

  if(WIN32)
    target_compile_definitions(gap INTERFACE -DGAP_DLL)
  endif()

  #set_target_properties(gap PROPERTIES VERSION ${GAP_VERSION} SOVERSION ${GAP_SOVERSION})
  install(TARGETS gap RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
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

