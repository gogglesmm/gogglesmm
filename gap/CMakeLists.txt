cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

include(TestBigEndian)
include(CheckIncludeFiles)
include(FindEXPAT)

project(GAP VERSION 1.0.0)

# Endiannes
TEST_BIG_ENDIAN(GAP_BIGENDIAN)


# libdir name to allow overriding to lib32 / lib64
set(CMAKE_INSTALL_LIBDIR "lib" CACHE STRING "Specify the name of the directory where libraries are meant to be installed")


# Update whenever ABI changes
set(GAP_SOVERSION_CURRENT 0)    # Increase whenever an interface has been added, removed or changed.
set(GAP_SOVERSION_REVISION 0)   # Always increase the revision value. Reset to 0 when Current changes.
set(GAP_SOVERSION_AGE 0)        # Increase the age value only if the changes made to the ABI are backward compatible.


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


find_package(PkgConfig)

# Expat is Required
find_package(EXPAT REQUIRED)
LIST(APPEND LIBRARIES ${EXPAT_LIBRARIES})

# We need
pkg_check_modules(FOX fox17>=1.7.48 REQUIRED)

pkg_check_modules(ALSA alsa)
pkg_check_modules(PULSE libpulse)
pkg_check_modules(FLAC flac)
pkg_check_modules(VORBIS vorbis)
pkg_check_modules(OGG ogg)
pkg_check_modules(OPUS opus)
pkg_check_modules(SAMPLERATE samplerate)
#pkg_check_modules(JACK jack)
#pkg_check_modules(CDDA libcdio_cdda)

# Check for libmad
check_include_files(mad.h HAVE_MAD_H)
if (HAVE_MAD_H)
   set(LIBMAD_HEADERS_FOUND TRUE)
endif (HAVE_MAD_H)

if (LIBMAD_HEADERS_FOUND)
   find_library(MAD_LIBRARIES NAMES mad )
endif (LIBMAD_HEADERS_FOUND)
if (MAD_LIBRARIES)
   set(MAD_FOUND TRUE)
   message(STATUS "libmad found: ${MAD_LIBRARIES}")
endif()

# Check for faad
check_include_files(faad.h HAVE_FAAD_H)
if (HAVE_FAAD_H)
   set(LIBFAAD_HEADERS_FOUND TRUE)
endif (HAVE_FAAD_H)

if (LIBFAAD_HEADERS_FOUND)
   find_library(FAAD_LIBRARIES NAMES faad )
endif (LIBFAAD_HEADERS_FOUND)
if (FAAD_LIBRARIES)
   set(FAAD_FOUND TRUE)
   message(STATUS "libfaad found: ${FAAD_LIBRARIES}")
endif()


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${FOX_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS})

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

set(PLUGIN_SOURCES plugins/ap_aiff_plugin.cpp
                   plugins/ap_file_plugin.cpp
                   plugins/ap_http_plugin.cpp
                   plugins/ap_id3v2.cpp
                   plugins/ap_m3u_plugin.cpp
                   plugins/ap_pcm_plugin.cpp
                   plugins/ap_pls_plugin.cpp
                   plugins/ap_wav_plugin.cpp
                   plugins/ap_xspf_plugin.cpp
                   )

if (FLAC_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_flac_plugin.cpp)
  LIST(APPEND LIBRARIES ${FLAC_LIBRARIES})
  set(HAVE_FLAC 1)
endif()

if (VORBIS_FOUND AND OGG_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis_plugin.cpp)
  LIST(APPEND LIBRARIES ${VORBIS_LIBRARIES})
  set(HAVE_VORBIS 1)
endif()

if (OGG_FOUND AND (FLAC_FOUND OR OPUS_FOUND OR VORBIS_FOUND))
  LIST(APPEND PLUGIN_SOURCES plugins/ap_ogg_plugin.cpp plugins/ap_ogg_decoder.cpp)
  LIST(APPEND LIBRARIES ${OGG_LIBRARIES})
  set(HAVE_OGG 1)
endif()

if (OPUS_FOUND AND OGG_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_opus_plugin.cpp)
  LIST(APPEND LIBRARIES ${OPUS_LIBRARIES})
  set(HAVE_OPUS 1)
endif()

if (MAD_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_mad_plugin.cpp)
  LIST(APPEND LIBRARIES ${MAD_LIBRARIES})
  set(HAVE_MAD 1)
endif()

if (FAAD_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_aac_plugin.cpp plugins/ap_mp4.cpp)
  LIST(APPEND LIBRARIES ${FAAD_LIBRARIES})
  set(HAVE_FAAD 1)
endif()

if (CDDA_FOUND)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_cdda_plugin.cpp)
  LIST(APPEND LIBRARIES ${CDDA_LIBRARIES})
  set(HAVE_CDDA 1)
endif()

if (SAMPLERATE_FOUND)
  LIST(APPEND LIBRARIES ${SAMPLERATE_LIBRARIES})
  set(HAVE_SAMPLERATE 1)
endif()

add_library(gap_plugins OBJECT ${PLUGIN_SOURCES})
target_include_directories(gap_plugins BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
if(BUILD_SHARED_LIBS)
  set_property(TARGET gap_plugins PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()


# Alsa Output
if (ALSA_FOUND)
  add_library(gap_alsa MODULE plugins/ap_alsa_plugin.cpp)
  target_link_libraries(gap_alsa ${ALSA_LIBRARIES})
  target_include_directories(gap_alsa BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
  install(TARGETS gap_alsa LIBRARY DESTINATION lib/gogglesmm)
  set(HAVE_ALSA 1)
endif()

# Pulse Output
if (PULSE_FOUND)
  add_library(gap_pulse MODULE plugins/ap_pulse_plugin.cpp)
  target_link_libraries(gap_pulse ${PULSE_LIBRARIES})
  target_include_directories(gap_pulse BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
  install(TARGETS gap_pulse LIBRARY DESTINATION lib/gogglesmm)
  set(HAVE_PULSE 1)
endif()

# Jack Output
if (JACK_FOUND)
  add_library(gap_jack MODULE plugins/ap_jack_plugin.cpp)
  target_link_libraries(gap_jack ${PULSE_LIBRARIES})
  target_include_directories(gap_jack BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
  install(TARGETS gap_jack LIBRARY DESTINATION lib/gogglesmm)
  set(HAVE_JACK 1)
endif()

# Wav File output
add_library(gap_wav MODULE plugins/ap_wavout_plugin.cpp)
target_include_directories(gap_wav BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
install(TARGETS gap_wav LIBRARY DESTINATION lib/gogglesmm)


if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(gap_oss MODULE plugins/ap_oss_plugin.cpp)
    target_compile_options(gap_oss PRIVATE -idirafter /usr/include/sys)
    target_include_directories(gap_oss BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
    install(TARGETS gap_oss LIBRARY DESTINATION lib/gogglesmm)
    set(HAVE_OSS 1)
endif()


set(AP_PLUGIN_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/gogglesmm)



configure_file("${PROJECT_SOURCE_DIR}/ap_config.h.in" "${PROJECT_BINARY_DIR}/ap_config.h")

add_library(gap ${SOURCES} $<TARGET_OBJECTS:gap_plugins>)
target_link_libraries(gap ${LIBRARIES})


set_target_properties(gap PROPERTIES ENABLE_EXPORTS 1)

target_include_directories(gap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(BUILD_SHARED_LIBS)
  #set_target_properties(gap PROPERTIES VERSION ${GAP_VERSION} SOVERSION ${GAP_SOVERSION})
  install(TARGETS gap LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(FILES include/ap.h
                include/ap_app_queue.h
                include/ap_buffer_base.h
                include/ap_buffer_io.h
                include/ap_common.h
                include/ap_device.h
                include/ap_dsp.h
                include/ap_event.h
                include/ap_event_queue.h
                include/ap_http.h
                include/ap_http_client.h
                include/ap_http_response.h
                include/ap_player.h
                include/ap_xml_parser.h DESTINATION include/gap)
endif()

