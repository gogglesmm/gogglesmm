# GAP CMake Modules

This directory contains CMake find modules specific to the **GAP (Goggles Audio Player)** library.

## Audio-Specific Find Modules

GAP requires various audio libraries for playback and codec support. These modules provide modern imported targets:

### Audio Output Libraries

| Module | Target | Description |
|--------|--------|-------------|
| FindPulseAudio.cmake | `PulseAudio::PulseAudio` | PulseAudio sound server |
| FindJack.cmake | `Jack::Jack` | JACK Audio Connection Kit (low-latency) |

### Audio Codec Libraries

| Module | Target | Description |
|--------|--------|-------------|
| FindFLAC.cmake | `FLAC::FLAC` | Free Lossless Audio Codec |
| FindOgg.cmake | `Ogg::Ogg` | Ogg container format |
| FindVorbis.cmake | `Vorbis::Vorbis`<br>`Vorbis::VorbisFile`<br>`Vorbis::VorbisEnc` | Vorbis audio codec |
| FindTremor.cmake | `Tremor::Tremor` | Tremor (fixed-point Vorbis decoder) |
| FindOpus.cmake | `Opus::Opus` | Opus audio codec |
| FindMAD.cmake | `MAD::MAD` | MPEG Audio Decoder (MP3) |
| FindFAAD.cmake | `FAAD::FAAD` | Freeware Advanced Audio Decoder (AAC) |
| FindA52.cmake | `A52::A52` | ATSC A/52 stream decoder |
| FindDCA.cmake | `DCA::DCA` | DTS Coherent Acoustics decoder |
| FindSndio.cmake | `Sndio::Sndio` | OpenBSD audio/MIDI framework |

### Security Libraries

| Module | Target | Description |
|--------|--------|-------------|
| FindGnuTLS.cmake | `GnuTLS::GnuTLS` | GNU Transport Layer Security Library |
| FindGcrypt.cmake | `Gcrypt::Gcrypt` | GNU cryptographic library |

## Built-in CMake Modules

GAP also uses these built-in CMake modules (no custom module needed):

| Library | Target | Description |
|---------|--------|-------------|
| ALSA | `ALSA::ALSA` | Advanced Linux Sound Architecture |
| ZLIB | `ZLIB::ZLIB` | Compression library |
| OpenSSL | `OpenSSL::SSL`, `OpenSSL::Crypto` | SSL/TLS for secure HTTP |

## Usage

These modules are automatically available when building GAP because `gap/CMakeLists.txt` adds this directory to `CMAKE_MODULE_PATH`:

```cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
```

### Example

```cmake
# In gap/CMakeLists.txt

# Built-in module
find_package(ALSA)

# Custom module from gap/cmake/
find_package(PulseAudio)
find_package(FLAC)

# Target-based linking
if(ALSA_FOUND)
  target_link_libraries(gap_alsa PRIVATE ALSA::ALSA)
endif()

if(PulseAudio_FOUND)
  target_link_libraries(gap_pulse PRIVATE PulseAudio::PulseAudio)
endif()
```

## Module Design

All modules follow a two-tier discovery pattern:

### Tier 1: Prefer Upstream CMake Configs

Several audio libraries now provide official CMake config files:

| Library | Upstream Config | Target Created | Our Module Behavior |
|---------|----------------|----------------|---------------------|
| **PulseAudio** | ✅ `/usr/lib/cmake/PulseAudio` | ❌ (sets variables only) | Use config for discovery, create `PulseAudio::PulseAudio` target |
| **Ogg** | ✅ `/usr/lib/cmake/Ogg` | ✅ `Ogg::ogg` | Use config's target, create `Ogg::Ogg` alias for consistency |
| **FLAC** | ✅ `/usr/lib/cmake/FLAC` | ✅ `FLAC::FLAC` | Use config's target directly (name matches!) |
| **Vorbis** | ❌ | ❌ | Use pkg-config + manual target creation |
| **Opus** | ❌ | ❌ | Use pkg-config + manual target creation |
| **Jack** | ❌ | ❌ | Use pkg-config + manual target creation |

### Tier 2: Fallback to pkg-config

When upstream configs aren't available or fail, modules fall back to:

1. **Use pkg-config as hint** - Helps find library/include paths
2. **Find library independently** - Works even without pkg-config
3. **Extract version** - From pkg-config if available
4. **Standard error handling** - Uses `find_package_handle_standard_args()`
5. **Create imported target** - Modern `Namespace::Component` pattern
6. **Mark as advanced** - Clean CMake cache

### Benefits of Preferring Upstream Configs

1. **Better maintained** - Updated by library developers
2. **Proper dependencies** - FLAC's config automatically finds Ogg
3. **Correct flags** - Platform-specific compile/link flags handled upstream
4. **Version info** - More reliable version detection
5. **Forward compatible** - New features automatically available

## Scope

These modules are **GAP-specific** because:

- They're only needed by the audio player library
- Not used by the main gogglesmm application
- Keeps separation between GAP and gogglesmm concerns

Project-wide modules (like FindFox.cmake) live in the root `cmake/` directory.

## Adding New Audio Libraries

When adding support for new audio formats or output backends:

1. Check if CMake provides a built-in module first
2. If not, create a Find module here following the existing pattern
3. Add it to this README
4. Use the imported target in gap/CMakeLists.txt

## See Also

- **../CMakeLists.txt** - GAP build configuration
- **../../cmake/** - Project-wide CMake modules (FindFox, etc.)
