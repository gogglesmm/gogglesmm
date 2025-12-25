# CMake Modules for Gogglesmm

This directory contains **project-wide** CMake modules used across the entire gogglesmm project.

## Directory Structure

```
cmake/                     # Project-wide modules (this directory)
├── FindFox.cmake         # FOX Toolkit GUI library
├── FindFox.md           # FOX module documentation
└── README.md            # This file

gap/cmake/                # GAP (audio library) specific modules
├── FindPulseAudio.cmake
├── FindJack.cmake
├── FindFLAC.cmake
├── FindOgg.cmake
├── FindVorbis.cmake
├── FindOpus.cmake
└── README.md
```

## Project-Wide Modules

### FindFox.cmake

Modern CMake module for the FOX Toolkit GUI library.

**Provides:**
- `FX::FOX` - Main FOX library target
- `FX::FOX_XINCS` - Platform definitions (optional, for xincs.h usage)
- `FX::reswrap` - Resource compiler utility

**Modes:**
1. **Bundled** (default) - Uses `fox/` subdirectory
2. **Build Tree** - Uses FOX from existing build via `FOX_BUILD_TREE`
3. **External** - Uses system-installed FOX via `FOX_USE_EXTERNAL=ON`

See **FindFox.md** for complete documentation.

## GAP Audio Modules

Audio-specific modules (PulseAudio, Jack, FLAC, Ogg, Vorbis, Opus) are in **`gap/cmake/`** since they're only used by the GAP audio library module.

See **`gap/cmake/README.md`** for details on audio modules.

## Built-in CMake Modules

These libraries use standard CMake find modules (no custom module needed):

| Library | Target | CMake Version | Used By |
|---------|--------|---------------|---------|
| ALSA | `ALSA::ALSA` | 3.12+ | GAP |
| EXPAT | `EXPAT::EXPAT` | 3.0+ | GAP, src |
| ZLIB | `ZLIB::ZLIB` | 3.1+ | GAP |
| OpenSSL | `OpenSSL::SSL`<br>`OpenSSL::Crypto` | 3.4+ | GAP |

## Usage Example

### Old Way (Variable-Based)
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(FLAC REQUIRED flac)

include_directories(${ALSA_INCLUDE_DIRS} ${FLAC_INCLUDE_DIRS})
target_link_libraries(myapp ${ALSA_LIBRARIES} ${FLAC_LIBRARIES})
```

### New Way (Target-Based)
```cmake
# Built-in modules work automatically (no special cmake module path needed)
find_package(ALSA REQUIRED)

# Custom modules use CMAKE_MODULE_PATH (set in root CMakeLists.txt)
find_package(FLAC REQUIRED)

# Clean, modern linking - no include_directories needed!
target_link_libraries(myapp PRIVATE ALSA::ALSA FLAC::FLAC)
```

**Note:** Built-in CMake modules (ALSA, EXPAT, ZLIB, OpenSSL) are always available. Custom modules require `CMAKE_MODULE_PATH` to be set to this directory.

## Benefits of Target-Based Approach

1. **Automatic include directories** - No need for `include_directories()` or `target_include_directories()`
2. **Transitive dependencies** - Dependencies automatically propagate
3. **Proper visibility** - PUBLIC/PRIVATE/INTERFACE keywords work correctly
4. **Better encapsulation** - No global pollution
5. **Modern CMake** - Follows CMake 3.x/4.x best practices
6. **Type safety** - Linking to non-existent targets fails at configure time

## Module Design Principles

### Prefer Built-in CMake Modules

Always check if CMake provides a built-in module before creating a custom one:
1. Better maintained by CMake developers
2. Tested across more platforms
3. Updated with new CMake versions
4. No maintenance burden for us

Use `cmake --help-module Find<Name>` to check if a module exists and what it provides.

### Custom Module Pattern

When CMake doesn't provide a module (or doesn't provide modern targets), our custom modules follow this pattern:

1. Use pkg-config as a hint (not requirement)
2. Find library and include directory independently
3. Extract version from pkg-config if available
4. Use `find_package_handle_standard_args()` for standard error handling
5. Create imported target with proper properties
6. Mark variables as advanced

This ensures consistency and reliability across different systems.

### Adding Fallbacks

If you need to support older CMake versions that don't have certain built-in targets, you can add fallback logic:

```cmake
find_package(ALSA REQUIRED)

# Fallback for CMake < 3.12 that doesn't provide ALSA::ALSA
if(ALSA_FOUND AND NOT TARGET ALSA::ALSA)
    add_library(ALSA::ALSA UNKNOWN IMPORTED)
    set_target_properties(ALSA::ALSA PROPERTIES
        IMPORTED_LOCATION "${ALSA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ALSA_INCLUDE_DIRS}"
    )
endif()
```

However, since gogglesmm requires CMake 4.0, such fallbacks are not needed for ALSA, EXPAT, ZLIB, or OpenSSL.

## Future Improvements

Additional modules that could be created:

- FindSampleRate.cmake (for libsamplerate)
- FindTagLib.cmake (for TagLib)
- FindSQLite3.cmake (for SQLite)
- FindDbus.cmake (for D-Bus)

These currently use pkg-config but could benefit from proper find modules with imported targets.
