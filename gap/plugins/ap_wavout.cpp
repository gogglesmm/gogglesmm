/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2015 by Sander Jansen. All Rights Reserved      *
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
#include "ap_output_plugin.h"


using namespace ap;

namespace ap {

class WavOutput : public OutputPlugin {
protected:
  FXFile file;
  FXlong data_pos;
public:
  WavOutput(OutputThread * output);

  FXchar type() const { return DeviceWav; }

  FXbool configure(const AudioFormat &);

  FXbool write(const void*, FXuint);

  void drop(){}
  void drain(){}
  void pause(FXbool) {}

  void close();

  virtual ~WavOutput();
  };

WavOutput::WavOutput(OutputThread * out) : OutputPlugin(out),data_pos(0){
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
  GM_DEBUG_PRINT("[wav] failed to open output file: %s\n",path.text());
  return false;
  }


//FIXME make sure data fits within 4GB.
FXbool WavOutput::write(const void * data,FXuint nframes) {
  FXlong duration = ((FXlong)nframes*1000000000)/af.rate;
  FXThread::sleep(duration);
  if (!file.isOpen() || file.writeBlock(data,af.framesize()*nframes)!=af.framesize()*nframes)
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
  af.reset();
  }

}


extern "C" GMAPI OutputPlugin * ap_load_plugin(OutputThread * output) {
  return new WavOutput(output);
  }

extern "C" GMAPI void ap_free_plugin(OutputPlugin* plugin) {
  delete plugin;
  }

FXuint GMAPI ap_version = AP_VERSION(GAP_VERSION_MAJOR,GAP_VERSION_MINOR,GAP_VERSION_PATCH);

