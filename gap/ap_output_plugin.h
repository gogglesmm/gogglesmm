/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef OUTPUT_PLUGIN_H
#define OUTPUT_PLUGIN_H

#include "ap_device.h"  // for OutputConfig
#include "ap_format.h"  // for AudioFormat
#include "ap_reactor.h" // for Reactor

namespace ap {


#define AP_IMPLEMENT_PLUGIN(PluginName)\
 extern "C" {\
  GMPLUGINAPI OutputPlugin * ap_load_plugin(OutputContext* output) {\
    return new PluginName(output);\
    }\
  GMPLUGINAPI void ap_free_plugin(OutputPlugin * plugin) {\
    delete plugin;\
    plugin=nullptr;\
    }\
  FXuint GMPLUGINAPI ap_version = AP_VERSION(GAP_VERSION_MAJOR,GAP_VERSION_MINOR,GAP_VERSION_PATCH);\
  }

/* Interface to OutputThread */
class GMAPI OutputContext {
public:
  virtual void notify_disable_volume()=0;

  virtual void notify_volume(FXfloat value)=0;

  virtual void wait_plugin_events()=0;

  virtual Reactor & getReactor()=0;
  };


class GMAPI OutputPlugin {
public:
  OutputContext * context = nullptr;
  AudioFormat     af;
private:
  OutputPlugin(){}
public:
  /// Constructor
  OutputPlugin(OutputContext * ctx) : context(ctx) {}

  /// Output Plugin Type
  virtual FXchar type() const=0;

  /// Set Device Configuration
  virtual FXbool setOutputConfig(const OutputConfig &) { return false; }

  /// Configure the device for the requested format. If requested
  /// format is not available it should return something similar. Only
  /// return false if no format is available at all.
  virtual FXbool configure(const AudioFormat &)=0;

  /// Write frames to playback buffer
  virtual FXbool write(const void*, FXuint)=0;

  /// Return delay in no. of frames
  virtual FXint delay() { return 0; }

  /// Empty Playback Buffer Immediately
  virtual void drop()=0;

  /// Wait until playback buffer is emtpy.
  virtual void drain()=0;

  virtual void start() {}

  /// Pause Playback
  virtual void pause(FXbool t)=0;

  /// Change Volume
  virtual void volume(FXfloat) {}

  /// Get Volume
  virtual FXfloat volume() {return 1.0f;}

  /// Close Output
  virtual void close() {}

  /// Destructor
  virtual ~OutputPlugin() {}
  };


// Plugin API
typedef OutputPlugin*  (*ap_load_plugin_t)(OutputContext*);
typedef void           (*ap_free_plugin_t)(OutputPlugin*);

}
#endif
