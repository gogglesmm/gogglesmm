# windows.cmake
# Windows-specific configuration for GAP (Goggles Audio Player)

#-------------------------------------------------------------------------------
# Windows-specific Build Options
#-------------------------------------------------------------------------------

# WAV output is available on Windows
option(WITH_WAVOUT "WAV Output Support" ON)

#-------------------------------------------------------------------------------
# Windows-specific Configuration
#-------------------------------------------------------------------------------

# Windows always builds GAP as shared library (required for plugin system)
# Define GAP_DLL for symbol export/import
add_definitions(-DGAP_DLL)

# Plugins link to GAP library on Windows
set(GAP_PLUGIN_LINK_TARGET gap)
