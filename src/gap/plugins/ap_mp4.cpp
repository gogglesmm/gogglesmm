/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_reactor.h"
#include "ap_event_private.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {

class MP4Reader : public ReaderPlugin {
protected:
  ReadStatus parse();
public:
  MP4Reader(AudioEngine*);

  // Format
  FXuchar format() const { return Format::MP4; };

  // Init
  FXbool init(InputPlugin*);

  // Seekable
  FXbool can_seek() const;

  // Seek
  FXbool seek(FXdouble);

  // Process Packet
  ReadStatus process(Packet*); 

  // Destroy
  ~MP4Reader();
  };


ReaderPlugin * ap_mp4_reader(AudioEngine * engine) {
  return new MP4Reader(engine);
  }



MP4Reader::MP4Reader(AudioEngine* e) : ReaderPlugin(e) {
  }

MP4Reader::~MP4Reader(){
  }

FXbool MP4Reader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  flags&=~FLAG_PARSED;
  return true;
  }


FXbool MP4Reader::can_seek() const {
  return true;
  }

FXbool MP4Reader::seek(FXdouble pos){
  return true;
  }

ReadStatus MP4Reader::process(Packet*p) {
  /*
  packet=p;
  packet->stream_position=-1;
  packet->stream_length=stream_length;
  packet->flags=AAC_FLAG_FRAME;
  */

  if (!(flags&FLAG_PARSED)) {
    return parse();
    }
  return ReadError;
  }



ReadStatus MP4Reader::parse() {

  return ReadError;
  }



}


