/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#include "ap_defs.h"
#include "ap_config.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_format.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_input_plugin.h"
#include "ap_output_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_decoder_thread.h"

using namespace ap;

namespace ap {

class WavOutput : public OutputPlugin {
protected:
  FXFile file;
  FXlong header_pos;
  FXlong data_pos;
public:
  WavOutput();

  FXchar type() const { return DeviceWav; }

  FXbool configure(const AudioFormat &);

  FXbool write(const void*, FXuint);

  void drop(){}
  void drain(){}
  void pause(FXbool) {}

  void close();

  virtual ~WavOutput();
  };

WavOutput::WavOutput() {
  }

WavOutput::~WavOutput() {
  close();
  }


///FIXME perhaps support extensible wav format
FXbool WavOutput::configure(const AudioFormat & fmt) {
  FXString path=FXPath::unique("gap.wav");
  if (file.open(path,FXIO::Writing)) {
    GM_DEBUG_PRINT("[wav] opened output file: %s\n",path.text());
    af=fmt;
    FXuint chunksize=0;
    FXushort format=1;

    /// riff chunk
    file.writeBlock("RIFF",4);
    file.writeBlock(&chunksize,4); // empty for now
    file.writeBlock("WAVE",4);

    /// fmt
    file.writeBlock("fmt ",4);
    chunksize=16;
    file.writeBlock(&chunksize,4);
    file.writeBlock(&format,2);

    FXushort channels=fmt.channels;
    FXuint   rate=fmt.rate;
    FXuint   byterate=fmt.rate*fmt.channels*fmt.packing();
    FXushort blockalign=fmt.framesize();
    FXushort bitspersample=fmt.bps();

    file.writeBlock(&channels,2);
    file.writeBlock(&rate,4);
    file.writeBlock(&byterate,4);
    file.writeBlock(&blockalign,2);
    file.writeBlock(&bitspersample,2);

    file.writeBlock("data",4);
    chunksize=0;
    data_pos=file.position();
    file.writeBlock(&chunksize,4);
    return true;
    }
  return false;
  }


//FIXME make sure data fits within 4GB.
FXbool WavOutput::write(const void * data,FXuint nframes) {
  FXlong duration = ((FXlong)nframes*1000000000)/af.rate;
  FXThread::sleep(duration);
  if (file.writeBlock(data,af.framesize()*nframes)!=af.framesize()*nframes)
    return false;
  else
    return true;
  }

void WavOutput::close() {
  if (file.isOpen()) {
    GM_DEBUG_PRINT("[wav] closed output\n");
    FXuint end=file.position();
    FXuint size;

    /// RIFF chunksize
    file.position(4);
    size=end-8;
    file.writeBlock(&size,4);

    /// data chunksize
    if (data_pos) {
      file.position(data_pos);
      size=end-data_pos-4;
      file.writeBlock(&size,4);
      }
    file.close();
    }
  }

}


extern "C" GMAPI OutputPlugin * ap_load_plugin() {
  return new WavOutput();
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }
