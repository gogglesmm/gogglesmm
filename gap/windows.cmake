# Copyright (C) 2010-2026 by Sander Jansen. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
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
# Note: Symbol export/import is handled per-target via compile definitions

# Plugins link to GAP library on Windows
set(GAP_PLUGIN_LINK_TARGET gap)
