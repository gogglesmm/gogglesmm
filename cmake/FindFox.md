# FindFox.cmake Usage Guide

The `FindFox.cmake` module provides flexible FOX Toolkit discovery with three modes of operation.

## Modes of Operation

### 1. Bundled (Default)
Uses the `fox/` subdirectory (symlink or submodule) to build FOX alongside your project.

```bash
# Default behavior - uses fox/ subdirectory
cmake -B build
```

**Requirements:**
- `fox/` directory must exist (symlink or git submodule)
- Contains FOX source with CMakeLists.txt

**Advantages:**
- Same FOX version for all developers
- No system FOX installation required
- Controlled build configuration
- Optimized: Only builds FOX library + reswrap (skips apps, tests, chart)

**Build Optimization:**
When using bundled mode, FindFox automatically sets:
- `FOX_BUILD_APPS=OFF` - Skips adie, pathfinder, shutterbug, calculator, controlpanel
- `FOX_BUILD_TESTS=OFF` - Skips test programs and glviewer
- `FOX_BUILD_CHART=OFF` - Skips chart library

This significantly reduces build time since gogglesmm only needs the core FOX library and reswrap utility.

### 2. Build Tree
Uses FOX from an existing build tree without installing it.

```bash
# Use FOX from a specific build tree
cmake -B build -DFOX_BUILD_TREE=/home/sxj/Development/fox/build/release-native
```

**Requirements:**
- FOX must be already configured and built
- Build tree must contain `fox-config.cmake`

**Advantages:**
- Rapid iteration when developing both FOX and your app
- No need to install FOX
- Can test different FOX build configurations

**Common build trees:**
- `../fox/build/release-native` - Native optimized build
- `../fox/build/release-shared` - Shared library build
- `../fox/build/dev` - Development build with debug symbols

### 3. External (System-Installed)
Uses FOX installed on the system via package manager or manual installation.

```bash
# Use system-installed FOX
cmake -B build -DFOX_USE_EXTERNAL=ON
```

**Discovery Order:**
1. CMake config file (`fox-config.cmake`) - Modern method
2. pkg-config (`fox-1.7.pc`) - Fallback for older installations

**Advantages:**
- Smaller build times (FOX already compiled)
- Shared between multiple projects
- Managed by package manager

## CMake Variables

### Input Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `FOX_USE_EXTERNAL` | BOOL | OFF | Use external FOX instead of bundled |
| `FOX_BUILD_TREE` | PATH | - | Path to FOX build tree |

### Output Variables

| Variable | Description |
|----------|-------------|
| `FOX_FOUND` | TRUE if FOX was found or built |
| `FOX_RESWRAP` | Path to reswrap utility executable |
| `FOX_RESWRAP_H` | reswrap command for header generation (with correct flags) |
| `FOX_RESWRAP_CPP` | reswrap command for source generation (with correct flags) |
| `FOX_RESWRAP_TEXT` | reswrap command for text resources (with correct flags) |
| `FOX_VERSION` | FOX version (when using external) |

### Output Targets

| Target | Description |
|--------|-------------|
| `FX::FOX` | Main FOX library (link against this) |
| `FX::FOX_XINCS` | Platform definitions (optional, for xincs.h usage) |
| `FX::reswrap` | reswrap utility executable |

## Usage Examples

### Basic Usage
```cmake
# In your CMakeLists.txt
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindFox)

# Link your target
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE FX::FOX)
```

### Using reswrap
```cmake
# Generate resources using FOX_RESWRAP command variables
# FindFox automatically detects reswrap version and sets correct flags

# Generate header
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icons.h
    COMMAND ${FOX_RESWRAP_H} -o ${CMAKE_CURRENT_BINARY_DIR}/icons.h icon1.gif icon2.gif
    DEPENDS icon1.gif icon2.gif
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Generate source
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icons.cpp
    COMMAND ${FOX_RESWRAP_CPP} -o ${CMAKE_CURRENT_BINARY_DIR}/icons.cpp icon1.gif icon2.gif
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/icons.h icon1.gif icon2.gif
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(myapp main.cpp ${CMAKE_CURRENT_BINARY_DIR}/icons.cpp)
target_link_libraries(myapp PRIVATE FX::FOX)

# Or use the target directly (modern CMake)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp
    COMMAND FX::reswrap --keep-ext --source -o ${CMAKE_CURRENT_BINARY_DIR}/resources.cpp logo.png
    DEPENDS logo.png
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
```

### Using Platform Definitions (xincs.h)
```cmake
# If your code uses xincs.h for platform-specific features
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE FX::FOX FX::FOX_XINCS)
```

## Common Scenarios

### Development Workflow
```bash
# Day-to-day development - use bundled FOX
cmake -B build
cmake --build build

# Testing with native optimizations
cmake -B build -DFOX_BUILD_TREE=../fox/build/release-native
cmake --build build

# Testing against system FOX
cmake -B build -DFOX_USE_EXTERNAL=ON
cmake --build build
```

### CI/CD Pipeline
```bash
# Use system FOX for faster builds
cmake -B build -DFOX_USE_EXTERNAL=ON
cmake --build build
cmake --install build
```

### Package Maintainers
```bash
# Distributions should always use external FOX
cmake -B build \
    -DFOX_USE_EXTERNAL=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build
cmake --install build
```

## Git Submodule Setup

To use FOX as a git submodule instead of a symlink:

```bash
# Add FOX as a submodule
cd gogglesmm
git submodule add https://github.com/franko/fox.git fox

# Clone project with submodules
git clone --recursive https://github.com/user/gogglesmm.git

# Or initialize submodules after cloning
git submodule update --init --recursive
```

## Troubleshooting

### "FOX: Bundled fox/ subdirectory not found"
- Ensure `fox/` symlink or submodule exists: `ls -la fox/`
- For submodules: `git submodule update --init`

### "FOX: External FOX requested but not found"
- Install FOX: `sudo apt install libfox-1.7-dev` (Debian/Ubuntu)
- Or build and install FOX manually
- Or use bundled: `cmake -DFOX_USE_EXTERNAL=OFF`

### "FOX: Build tree specified but fox-config.cmake not found"
- Ensure FOX build tree is configured: `cd ../fox && cmake --preset release-native`
- Check path is correct: `ls ../fox/build/release-native/lib/fox-config.cmake`

### reswrap not found
- For bundled: reswrap is built automatically
- For external: Ensure FOX was installed with development tools
- For build tree: Build FOX with utilities: `cmake --build build --target reswrap`
