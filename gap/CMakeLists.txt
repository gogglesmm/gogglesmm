cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
cmake_policy(VERSION 3.23...4.2)

# Goggles Audio Player
project(GAP VERSION 1.3.1 LANGUAGES CXX)

# Windows requires shared library for plugin loading
# (static export from executable doesn't work on Windows)
if(WIN32)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
endif()

# set(CMAKE_VERBOSE_MAKEFILE TRUE)

# Add GAP's cmake modules to module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TestBigEndian)
include(CheckIncludeFiles)
include(FeatureSummary)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

#-------------------------------------------------------------------------------
# Find Required Dependencies
#-------------------------------------------------------------------------------

# Find FOX if not already found (allows GAP to build standalone)
if(NOT TARGET FX::FOX)
  # When building standalone, point to bundled fox in parent directory
  set(FOX_BUNDLED_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../fox")
  find_package(Fox REQUIRED)
endif()

# Find EXPAT (required dependency)
# When cross-compiling for Windows, build expat from source using FetchContent
if(CMAKE_CROSSCOMPILING AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(STATUS "Cross-compiling for Windows - building EXPAT from source")
  include(FetchContent)

  FetchContent_Declare(
    expat
    URL https://github.com/libexpat/libexpat/releases/download/R_2_6_4/expat-2.6.4.tar.xz
    URL_HASH SHA256=a695629dae047055b37d50a0ff4776d1d45d0a4c842cf4ccee158441f55ff7ee
  )

  # Configure expat build options
  set(EXPAT_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(EXPAT_BUILD_DOCS OFF CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(expat)

  # Create EXPAT::EXPAT alias target for compatibility
  if(NOT TARGET EXPAT::EXPAT)
    add_library(EXPAT::EXPAT ALIAS expat)
  endif()
else()
  # Native builds: use system expat
  find_package(EXPAT REQUIRED)
endif()

#-------------------------------------------------------------------------------
# Third Party Modules
#-------------------------------------------------------------------------------

add_subdirectory(lib/alac)

add_subdirectory(test)

# Update whenever ABI changes
set(GAP_SOVERSION_CURRENT 0)    # Increase whenever an interface has been added, removed or changed.
set(GAP_SOVERSION_REVISION 0)   # Always increase the revision value. Reset to 0 when Current changes.
set(GAP_SOVERSION_AGE 0)        # Increase the age value only if the changes made to the ABI are backward compatible.

# Calculate library version numbers (libtool versioning scheme)
math(EXPR GAP_SOVERSION "${GAP_SOVERSION_CURRENT} - ${GAP_SOVERSION_AGE}")
set(GAP_LIB_VERSION "${GAP_SOVERSION_CURRENT}.${GAP_SOVERSION_AGE}.${GAP_SOVERSION_REVISION}")


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


#-------------------------------------------------------------------------------
# GAP Library Build Configuration
#-------------------------------------------------------------------------------
#
# GAP can be built as either a static or shared library. This affects how
# plugins load and how symbols are exported:
#
# STATIC BUILD (Linux default):
#   - libgap.a linked into gogglesmm executable
#   - Output plugins (gap_alsa.so, gap_pulse.so, etc.) are shared modules
#   - Plugins depend on symbols from GAP library
#   - gogglesmm executable MUST export symbols with ENABLE_EXPORTS
#   - Works on Linux/Unix
#
# SHARED BUILD (Windows required):
#   - libgap.so/.dll is a shared library
#   - GAP exports its own symbols (via GAP_DLL define)
#   - Plugins link against GAP and load symbols from GAP DLL
#   - Required on Windows (static export doesn't work, or FOX linking conflict)
#   - Optional on Linux/Unix
#
# Control via BUILD_SHARED_LIBS=ON (standard CMake variable)
# Windows automatically forces shared build regardless of BUILD_SHARED_LIBS
#
#-------------------------------------------------------------------------------

# Status message for build type
if(BUILD_SHARED_LIBS)
  if(WIN32)
    message(STATUS "GAP: Building as SHARED library (required on Windows)")
  else()
    message(STATUS "GAP: Building as SHARED library (BUILD_SHARED_LIBS=ON)")
  endif()
else()
  message(STATUS "GAP: Building as STATIC library (BUILD_SHARED_LIBS=OFF)")
endif()

# Platform-specific configuration
if(WIN32)
  include(windows.cmake)
elseif(UNIX)
  include(unix.cmake)
endif()

# Endiannes
TEST_BIG_ENDIAN(GAP_BIGENDIAN)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Third Party Libraries
#-------------------------------------------------------------------------------
# Dependencies are found at the top of this file (FOX, EXPAT)
# Platform-specific dependencies are found in unix.cmake or windows.cmake
#-------------------------------------------------------------------------------

# Include source and header file lists
include(sources.cmake)
include(headers.cmake)

# GAP_PLUGIN_LINK_TARGET is set in windows.cmake for Windows builds

# FOX is linked via FX::FOX target (see target_link_libraries below)

# Buildin Alac Decoder
if(WITH_ALAC AND WITH_MP4)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_alac.cpp)
  LIST(APPEND GAP_TARGET_OBJECTS $<TARGET_OBJECTS:gap_alac>)
  set(HAVE_ALAC 1)
endif()

if(TARGET FLAC::FLAC)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_flac.cpp)
  LIST(APPEND PLUGIN_LINK_LIBRARIES FLAC::FLAC)
  set(HAVE_FLAC 1)
endif()

# Vorbis and Opus Decoders only with Ogg or Matroska.
if(WITH_OGG AND TARGET Ogg::Ogg)

  if(WITH_TREMOR AND TARGET Tremor::Tremor)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_vorbis.h)
    LIST(APPEND PLUGIN_LINK_LIBRARIES Tremor::Tremor)
    set(HAVE_TREMOR 1)
  elseif(WITH_VORBIS AND TARGET Vorbis::Vorbis)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_vorbis.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_vorbis.h)
    LIST(APPEND PLUGIN_LINK_LIBRARIES Vorbis::Vorbis Vorbis::VorbisFile)
    set(HAVE_VORBIS 1)
  endif()

  if(WITH_OPUS AND TARGET Opus::Opus)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_opus.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_opus.h)
    LIST(APPEND PLUGIN_LINK_LIBRARIES Opus::Opus)
    set(HAVE_OPUS 1)
  endif()

  if(HAVE_TREMOR OR HAVE_VORBIS OR HAVE_FLAC OR HAVE_OPUS)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_ogg.cpp plugins/ap_ogg_decoder.cpp)
    LIST(APPEND PLUGIN_HEADERS plugins/ap_ogg_decoder.h)
    LIST(APPEND PLUGIN_LINK_LIBRARIES Ogg::Ogg)
    set(HAVE_OGG 1)
  endif()
endif()

if(TARGET MAD::MAD)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_mad.cpp)
  LIST(APPEND PLUGIN_LINK_LIBRARIES MAD::MAD)
  set(HAVE_MAD 1)
endif()

if(TARGET FAAD::FAAD)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_aac.cpp)
  LIST(APPEND PLUGIN_LINK_LIBRARIES FAAD::FAAD)
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

  if(TARGET DCA::DCA)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_dca.cpp)
    LIST(APPEND PLUGIN_LINK_LIBRARIES DCA::DCA)
    set(HAVE_DCA 1)
  endif()

  if(TARGET A52::A52)
    LIST(APPEND PLUGIN_SOURCES plugins/ap_a52.cpp)
    LIST(APPEND PLUGIN_LINK_LIBRARIES A52::A52)
    set(HAVE_A52 1)
  endif()

endif()

if(HAVE_FLAC OR HAVE_MAD)
  LIST(APPEND PLUGIN_SOURCES plugins/ap_id3v2.cpp)
  LIST(APPEND PLUGIN_HEADERS plugins/ap_id3v2.h)
endif()

if(WITH_ZLIB AND TARGET ZLIB::ZLIB)
  LIST(APPEND PLUGIN_LINK_LIBRARIES ZLIB::ZLIB)
  set(HAVE_ZLIB 1)
endif()

if(WITH_OPENSSL AND TARGET OpenSSL::SSL)
  LIST(APPEND PLUGIN_LINK_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
  set(HAVE_OPENSSL 1 CACHE INTERNAL "" FORCE)
elseif(WITH_GNUTLS AND TARGET GnuTLS::GnuTLS)
  LIST(APPEND PLUGIN_LINK_LIBRARIES GnuTLS::GnuTLS)
  set(HAVE_GNUTLS 1)
elseif(WITH_GCRYPT AND TARGET Gcrypt::Gcrypt)
  LIST(APPEND PLUGIN_LINK_LIBRARIES Gcrypt::Gcrypt)
  set(HAVE_GCRYPT 1)
else()
  add_subdirectory(lib/md5)
  LIST(APPEND GAP_TARGET_OBJECTS $<TARGET_OBJECTS:gap_md5>)
  set(MD5_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/md5)
endif()



#-------------------------------------------------------------------------------

# GAP Plugins Library
add_library(gap_plugins OBJECT ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})
target_include_directories(gap_plugins BEFORE PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(gap_plugins PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/lib/alac)
target_link_libraries(gap_plugins PRIVATE FX::FOX EXPAT::EXPAT ${PLUGIN_LINK_LIBRARIES})

if(BUILD_SHARED_LIBS)
  set_property(TARGET gap_alac PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  set_property(TARGET gap_plugins PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  if(TARGET gap_md5)
    set_property(TARGET gap_md5 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  endif()
endif()


#-------------------------------------------------------------------------------


# Wav File output (common for all platforms)
if(WITH_WAVOUT)
  add_library(gap_wav MODULE plugins/ap_wavout.cpp)
  target_include_directories(gap_wav PRIVATE ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(gap_wav FX::FOX)
  # Windows needs additional linking to the main GAP target for plugins
  if(WIN32)
    target_link_libraries(gap_wav ${GAP_PLUGIN_LINK_TARGET})
  endif()
  install(TARGETS gap_wav LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/gogglesmm)
endif()

# Platform-specific output plugins are built in unix.cmake or windows.cmake


#-------------------------------------------------------------------------------
# Feature Summary
#-------------------------------------------------------------------------------

# Audio Output Plugins
add_feature_info("PulseAudio" HAVE_PULSE "PulseAudio output - ${PulseAudio_VERSION}")
add_feature_info("ALSA" HAVE_ALSA "ALSA output - ${ALSA_VERSION}")
add_feature_info("Jack" HAVE_JACK "Jack audio output - ${Jack_VERSION}")
add_feature_info("OSS" HAVE_OSS "OSS output")
add_feature_info("Sndio" HAVE_SNDIO "Sndio output - ${Sndio_VERSION}")
add_feature_info("WAV" WITH_WAVOUT "WAV file output")

# Container Formats
add_feature_info("Ogg" HAVE_OGG "Ogg container - ${Ogg_VERSION}")
add_feature_info("MP4" HAVE_MP4 "MP4/M4A container")
add_feature_info("Matroska" HAVE_MATROSKA "Matroska/MKA/WebM container")

# Audio Codecs
add_feature_info("FLAC" HAVE_FLAC "FLAC lossless codec - ${FLAC_VERSION}")
add_feature_info("Vorbis" HAVE_VORBIS "Vorbis codec - ${Vorbis_VERSION}")
add_feature_info("Tremor" HAVE_TREMOR "Tremor fixed-point Vorbis - ${Tremor_VERSION}")
add_feature_info("Opus" HAVE_OPUS "Opus codec - ${Opus_VERSION}")
add_feature_info("MP3" HAVE_MAD "MP3 via libmad - ${MAD_VERSION}")
add_feature_info("AAC" HAVE_FAAD "AAC via libfaad - ${FAAD_VERSION}")
add_feature_info("ALAC" HAVE_ALAC "Apple Lossless codec")
add_feature_info("DCA" HAVE_DCA "DTS Coherent Acoustics - ${DCA_VERSION}")
add_feature_info("A52" HAVE_A52 "Dolby Digital (AC-3) - ${A52_VERSION}")

# Network & Compression
add_feature_info("ZLIB" HAVE_ZLIB "HTTP compression - ${ZLIB_VERSION}")
add_feature_info("OpenSSL" HAVE_OPENSSL "HTTPS via OpenSSL - ${OPENSSL_VERSION}")
add_feature_info("GnuTLS" HAVE_GNUTLS "HTTPS via GnuTLS - ${GnuTLS_VERSION}")
add_feature_info("libgcrypt" HAVE_GCRYPT "MD5 via libgcrypt - ${Gcrypt_VERSION}")

# Print feature summary (only when GAP is built standalone)
if(PROJECT_IS_TOP_LEVEL)
  feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES)
endif()

set(AP_PLUGIN_PATH ${CMAKE_INSTALL_FULL_LIBDIR}/gogglesmm)

configure_file(ap_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/ap_config.h)

#-------------------------------------------------------------------------------
#
# Workaround to compile with earlier versions of FOX, something
# that cannot be easily handled using the preprocessor
#
if (DEFINED FOX_VERSION AND FOX_VERSION VERSION_LESS "1.7.80")
  foreach(sourcefile IN LISTS PLUGIN_SOURCES OUTPUT_SOURCES SOURCES)
    file(READ ${sourcefile} content)
    string(REPLACE "FXString::compare" "FX::compare" updated "${content}")
    string(REPLACE "comparenatural" "compareversion" updated "${updated}")
    file(WRITE ${sourcefile} "${updated}")
  endforeach()
endif()

#-------------------------------------------------------------------------------

# Gap Library
if(BUILD_SHARED_LIBS)
  add_library(gap SHARED
    ${SOURCES}
    ${HEADERS}
    $<TARGET_OBJECTS:gap_plugins>
    ${GAP_TARGET_OBJECTS}
  )
  target_sources(gap PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${PUBLIC_HEADERS}
  )
else()
  add_library(gap STATIC
    ${SOURCES}
    ${HEADERS}
    $<TARGET_OBJECTS:gap_plugins>
    ${GAP_TARGET_OBJECTS}
  )
  target_sources(gap PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${PUBLIC_HEADERS}
  )
endif()

# Windows DLL export/import handling
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(gap PRIVATE BUILDING_GAP_DLL)
endif()

# ENABLE_EXPORTS is CRITICAL for plugin loading:
# - SHARED builds: GAP DLL exports symbols for plugins
# - STATIC builds: Consumer executable must export symbols for plugins
# Without this, plugins will fail to load with undefined symbol errors
if(BUILD_SHARED_LIBS)
  # Shared library exports its own symbols
  set_target_properties(gap PROPERTIES ENABLE_EXPORTS 1)
else()
  # Static library: set INTERFACE property to tell consumers they must export symbols
  set_target_properties(gap PROPERTIES
    INTERFACE_GAP_REQUIRES_CONSUMER_EXPORTS TRUE
  )
endif()

target_include_directories(gap PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${MD5_INCLUDE_DIRS})
target_include_directories(gap INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/gap>
)

target_link_libraries(gap PRIVATE FX::FOX EXPAT::EXPAT ${PLUGIN_LINK_LIBRARIES})

# Propagate FOX to gap consumers (gap is a library)
target_link_libraries(gap INTERFACE FX::FOX)


if(BUILD_SHARED_LIBS)

  if(WIN32)
    target_compile_definitions(gap INTERFACE -DGAP_DLL)
  endif()

  # Set library version (creates libgap.so.0.0.0 and symlinks)
  set_target_properties(gap PROPERTIES
    VERSION ${GAP_LIB_VERSION}
    SOVERSION ${GAP_SOVERSION}
  )

  # Install library, headers, and export targets
  install(TARGETS gap
    EXPORT gap-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gap
  )

  # Install exported targets file
  install(EXPORT gap-targets
    FILE gap-targets.cmake
    NAMESPACE GAP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gap
  )

  # Generate and install package config file
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gap-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/gap-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gap
  )

  # Generate and install package version file
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/gap-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Install config files
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/gap-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/gap-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gap
  )
endif()
