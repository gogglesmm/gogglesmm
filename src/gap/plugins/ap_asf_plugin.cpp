/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2016 by Sander Jansen. All Rights Reserved      *
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
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_memory_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_input_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_buffer.h"
#include "ap_output_thread.h"
#include "ap_reader_plugin.h"

namespace ap {

typedef FXuchar FXguid[16];

static void print_guid(FXguid& guid){
  for (FXint i=0;i<16;i++) {
    fxmessage("%x",guid[i]);
    }
  fxmessage("\n");
  }


struct asf_data {
  FXguid    id;
  FXulong   npackets;
  FXushort  reserved;
  };

struct asf_header {
  FXguid  guid;
  FXulong size;
  };

struct asf_file_header {
  FXguid   id;
  FXulong  size;
  FXulong  create;
  FXulong  npackets;
  FXulong  play_duration;
  FXulong  send_duration;
  FXulong  preroll;
  FXuint  flags;
  FXuint  minsize;
  FXuint  maxsize;
  FXuint  maxrate;


  void debug() {
    fxmessage("id: "); print_guid(id);
    fxmessage("size: %ld\n",size);
    fxmessage("create: %ld\n",create);
    fxmessage("npackets: %ld\n",npackets);
    fxmessage("play_duration: %ld\n",play_duration);
    fxmessage("send_duration: %ld\n",send_duration);
    fxmessage("preroll: %ld\n",preroll);
    fxmessage("flags: %d\n",flags);
    fxmessage("minsize: %d\n",minsize);
    fxmessage("maxsize: %d\n",maxsize);
    fxmessage("maxrate: %d\n",maxrate);
    }
  };


struct asf_stream_header {
  FXguid    stream_type;
  FXguid    correction_type;
  FXulong   time_offset;
  FXuint    data_len;
  FXuint    correction_len;
  FXushort  flags;
  FXuint    reserved;
  FXchar*   data;
  FXchar*   error;


  void debug() {
    fxmessage("stream_type: "); print_guid(stream_type);
    fxmessage("correction_type: "); print_guid(correction_type);
    fxmessage("time_offset: %ld\n",time_offset);
    fxmessage("data_len: %d\n",data_len);
    fxmessage("correction_len: %d\n",correction_len);
    fxmessage("flags: %hd\n",flags);
    fxmessage("reserved: %d\n",reserved);
    }
  };


const FXguid asf_guid_header = {
  0x30,0x26,0xB2,0x75,0x8E,0x66,0xCF,0x11,
  0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C
  };

const FXguid asf_guid_file_header = {
  0xA1,0xDC,0xAB,0x8C,0x47,0xA9,0xCF,0x11,
  0x8E,0xE4,0x00,0xC0,0x0C,0x20,0x53,0x65
  };

const FXguid asf_guid_stream_header = {
  0x91,0x07,0xDC,0xB7,0xB7,0xA9,0xCF,0x11,
  0x8E,0xE6,0x00,0xC0,0x0C,0x20,0x53,0x65
  };

const FXguid asf_guid_data = {
  0x36,0x26,0xb2,0x75,0x8E,0x66,0xCF,0x11,
  0xa6,0xd9,0x00,0xaa,0x00,0x62,0xce,0x6c
  };


enum {
  ASF_GUID_INVALID      = 0,
  ASF_GUID_HEADER       = 1,
  ASF_GUID_FILE_HEADER  = 2,
  ASF_GUID_STREAM_HEADER =3,
  ASF_GUID_DATA
  };

static FXuint get_guid_id(FXguid & g){
  if (memcmp(g,asf_guid_header,16)==0) {
    GM_DEBUG_PRINT("found asf header\n");
    return ASF_GUID_HEADER;
    }
  else if (memcmp(g,asf_guid_file_header,16)==0) {
    GM_DEBUG_PRINT("found asf file header\n");
    return ASF_GUID_FILE_HEADER;
    }
  else if (memcmp(g,asf_guid_stream_header,16)==0) {
    GM_DEBUG_PRINT("found asf stream header\n");
    return ASF_GUID_STREAM_HEADER;
    }
  else if (memcmp(g,asf_guid_data,16)==0) {
    GM_DEBUG_PRINT("found asf data\n");
    return ASF_GUID_DATA;
    }
  else {
    return ASF_GUID_INVALID;
    }
  }



class ASFReader : public ReaderPlugin {
protected:
  FXbool read_guid(FXguid & guid);
  FXbool readLong(FXulong &);
  FXbool readInt(FXuint &);
  FXbool readShort(FXushort &);
  ReadStatus parse_header(Packet*);
  ReadStatus parse_file_header(Packet*);
  ReadStatus parse_stream_header(Packet*);
  ReadStatus parse_data(Packet*);
public:
  ASFReader(AudioEngine*);
  FXbool init(InputPlugin*);
  ReadStatus process(Packet*);

  FXuchar format() const { return Format::ASF; };
  virtual ~ASFReader();
  };


ASFReader::ASFReader(AudioEngine*e) : ReaderPlugin(e){
  }

ASFReader::~ASFReader(){
  }

FXbool ASFReader::init(InputPlugin*plugin){
  ReaderPlugin::init(plugin);
  return true;
  }

ReaderPlugin * ap_asf_reader(AudioEngine * engine) {
  return new ASFReader(engine);
  }

FXbool ASFReader::read_guid(FXguid & guid) {
  if (engine->input->read(guid,16)!=16)
    return false;
  else
    return true;
  }

FXbool ASFReader::readLong(FXulong & value) {
  if (engine->input->read(&value,8)!=8)
    return false;
  else
    return true;
  }

FXbool ASFReader::readInt(FXuint & value) {
  if (engine->input->read(&value,4)!=4)
    return false;
  else
    return true;
  }
FXbool ASFReader::readShort(FXushort & value) {
  if (engine->input->read(&value,2)!=2)
    return false;
  else
    return true;
  }

ReadStatus ASFReader::parse_data(Packet*) {
  asf_data d;

  if (!read_guid(d.id)) return ReadError;
  if (!readLong(d.npackets)) return ReadError;
  if (!readShort(d.reserved)) return ReadError;

  GM_DEBUG_PRINT("got %d packets\n",d.npackets);


  FXuchar flags;
  if (engine->input->read(&flags,1)!=1)
    return ReadError;

  if (flags&(1<<7)) {
    GM_DEBUG_PRINT("got error block\n");
    }

  GM_DEBUG_PRINT("len: %d\n",flags&0xF);

  engine->input->position(2,FXIO::Current);




//  fxmessage("flags: %x\n",flags);

  return ReadError;
  }

ReadStatus ASFReader::parse_stream_header(Packet*) {
  asf_stream_header h;
  if (!read_guid(h.stream_type))   return ReadError;
  if (!read_guid(h.correction_type))   return ReadError;
  if (!readLong(h.time_offset))   return ReadError;
  if (!readInt(h.data_len))   return ReadError;
  if (!readInt(h.correction_len))   return ReadError;
  if (!readShort(h.flags))   return ReadError;
  if (!readInt(h.reserved))   return ReadError;

  if (h.data_len+h.correction_len) engine->input->position(h.data_len+h.correction_len,FXIO::Current);
  h.debug();
  }



ReadStatus ASFReader::parse_file_header(Packet*) {
  asf_file_header h;
  if (!read_guid(h.id))   return ReadError;
  if (!readLong(h.size)) return ReadError;
  if (!readLong(h.create)) return ReadError;
  if (!readLong(h.npackets)) return ReadError;
  if (!readLong(h.play_duration)) return ReadError;
  if (!readLong(h.send_duration)) return ReadError;
  if (!readLong(h.preroll)) return ReadError;
  if (!readInt(h.flags)) return ReadError;
  if (!readInt(h.minsize)) return ReadError;
  if (!readInt(h.maxsize)) return ReadError;
  if (!readInt(h.maxrate)) return ReadError;
  h.debug();
  }

ReadStatus ASFReader::parse_header(Packet*) {
  FXuint nobjects;
  FXchar dummy[2];

  if (!readInt(nobjects)) return ReadError;

  /// Skip 2 bytes
  if (engine->input->read(&dummy,2)!=2)

  GM_DEBUG_PRINT("got %d objects\n",nobjects);
  return ReadOk;
  }




ReadStatus ASFReader::process(Packet*p) {
  asf_header header;

  while(!engine->input->eof()) {

    if (!read_guid(header.guid)) return ReadError;
    if (!readLong(header.size)) return ReadError;
    GM_DEBUG_PRINT("id: "); print_guid(header.guid);
    GM_DEBUG_PRINT("size: %ld\n",header.size-24);

    if (header.size<=24)
      return ReadError;


    switch(get_guid_id(header.guid)) {
      case ASF_GUID_HEADER        : parse_header(p);          break;
      case ASF_GUID_FILE_HEADER   : parse_file_header(p);          break;
      case ASF_GUID_STREAM_HEADER : parse_stream_header(p);                 break;
      case ASF_GUID_DATA          : return parse_data(p);                 break;
      default             : if (header.size>24) engine->input->position(header.size-24,FXIO::Current); break;
      }
    }


  return ReadOk;
  }






}
