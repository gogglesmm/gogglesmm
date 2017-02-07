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

// Building a plugin
#define GAP_PLUGIN 1

#include "ap_defs.h"
#include "ap_output_plugin.h"

using namespace ap;

namespace ap {

class WavOutput : public OutputPlugin {
protected:
  FXFile file;
  FXlong data_pos = 0;
public:
  WavOutput(OutputContext * ctx);

  FXchar type() const override { return DeviceWav; }

  FXbool configure(const AudioFormat &) override;

  FXbool write(const void*, FXuint) override;

  void drop() override {}
  void drain() override {}
  void pause(FXbool) override {}

  void close() override;

  virtual ~WavOutput();
  };

WavOutput::WavOutput(OutputContext * ctx) : OutputPlugin(ctx) {
  }

WavOutput::~WavOutput() {
  close();
  }


enum {
  WAV_FORMAT_PCM        = 0x0001,
  WAV_FORMAT_FLOAT      = 0x0003,
  //WAV_FORMAT_EXTENSIBLE = 0xFFFE
  };


///FIXME perhaps support extensible wav format
FXbool WavOutput::configure(const AudioFormat & fmt) {
  FXushort format;

  // Can't handle big endian yet, neither does the output thread handle byteorder swaps
  if (fmt.byteorder() != Format::Little)
    return false;

  // Extensible Wav Format not yet supported
  if (fmt.channels>2)
    return false;

  // Determine format
  switch(fmt.datatype()) {
    case Format::Unsigned :
    case Format::Signed   : format = WAV_FORMAT_PCM;  break;
    case Format::Float    : format = WAV_FORMAT_FLOAT; break;
    default               : return false; break;
    }

  FXString path=FXPath::unique("gap.wav");
  if (file.open(path,FXIO::Writing)) {
    GM_DEBUG_PRINT("[wav] opened output file: %s\n",path.text());
    af=fmt;
    FXuint chunksize=0;
    FXlong ldata=0;

    /// riff chunk
    file.writeBlock("RIFF",4);
    file.writeBlock(&chunksize,4); // empty for now
    file.writeBlock("WAVE",4);

    /// junk chunk
    chunksize=28;
    file.writeBlock("JUNK",4);
    file.writeBlock(&chunksize,4);
    file.writeBlock(&ldata,8);
    file.writeBlock(&ldata,8);
    file.writeBlock(&ldata,8);
    chunksize=0;
    file.writeBlock(&chunksize,4);

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
    chunksize=0xFFFFFFFF;
    data_pos=file.position();
    file.writeBlock(&chunksize,4);
    return true;
    }
  GM_DEBUG_PRINT("[wav] failed to open output file: %s\n",path.text());
  return false;
  }


FXbool WavOutput::write(const void * data,FXuint nframes) {
  FXlong duration = (nframes*NANOSECONDS_PER_SECOND) / af.rate;
  FXThread::sleep(duration);
  if (!file.isOpen() || file.writeBlock(data,af.framesize()*nframes)!=af.framesize()*nframes)
    return false;
  else
    return true;
  }

void WavOutput::close() {
  if (file.isOpen()) {
    GM_DEBUG_PRINT("[wav] closed output\n");
    FXulong end=file.position();
    FXulong size;
    FXuint size32=0xFFFFFFFF;

    size=end-8;
    if (end>0xFFFFFFFF) {

      // RIFF Chunk
      file.position(0);
      file.writeBlock("RF64",4);
      file.writeBlock(&size32,4);

      // DS64 Chunk
      file.position(12);
      file.writeBlock("ds64",4);
      file.position(20);
      file.writeBlock(&size,8);
      // Data Chunk
      if (data_pos) {
        size=end-data_pos-4;
        file.writeBlock(&size,8);
        size=0;
        file.writeBlock(&size,8);
        }
      }
    else {

      /// RIFF chunksize
      size32=size;
      file.position(4);
      file.writeBlock(&size32,4);

      // Data Chunksize
      if (data_pos) {
        file.position(data_pos);
        size=end-data_pos-4;
        file.writeBlock(&size,4);
        }
      }
    file.close();
    }
  af.reset();
  }

}


AP_IMPLEMENT_PLUGIN(WavOutput);

