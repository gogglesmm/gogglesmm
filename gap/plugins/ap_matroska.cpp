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
#include "ap_event_private.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_input_plugin.h"
#include "ap_decoder_thread.h"


enum {
  EBML                        = 0x1a45dfa3,
  EBML_VERSION                = 0x4286,
  EBML_READ_VERSION           = 0x42f7,
  EBML_MAX_ID_LENGTH          = 0x42f2,
  EBML_MAX_SIZE_LENGTH        = 0x42f3,
  EBML_DOC_TYPE               = 0x4282,
  EBML_DOC_TYPE_VERSION       = 0x4287,
  EBML_DOC_TYPE_READ_VERSION  = 0x4285,
  SEGMENT                     = 0x18538067,
  SEEK_HEAD                   = 0x114d9b74,
  SEEK                        = 0x4dbb,
  SEEK_ID                     = 0x53ab,
  SEEK_POSITION               = 0x53ac,
  TRACK                       = 0x1654ae6b,
  TRACK_ENTRY                 = 0xae,
  TRACK_TYPE                  = 0x83,
  CODEC_ID                    = 0x86,
  AUDIO                       = 0xe1,
  AUDIO_SAMPLE_RATE           = 0xb5,
  AUDIO_CHANNELS              = 0x9f,
  };


namespace ap {

class MatroskaReader : public ReaderPlugin {
protected:
#ifdef DEBUG
  FXint indent = 0;
#endif
protected:
  ReadStatus parse(Packet * p);




  FXbool parse_element(FXuint & id,FXlong & size);
  FXbool parse_element(FXlong & container,FXuint & id,FXlong & size);
  FXbool parse_element_size(FXlong & container,FXlong & value);
  FXbool parse_element_id(FXlong & container,FXuint & value);

  FXbool parse_ebml(FXlong size);
  FXbool parse_segment(FXlong size);
  FXbool parse_seekhead(FXlong size);
  FXbool parse_seek(FXlong size);
  FXbool parse_track(FXlong size);
  FXbool parse_track_entry(FXlong size);
  FXbool parse_track_audio(FXlong size);
public:
  MatroskaReader(AudioEngine*);

  // Format
  FXuchar format() const { return Format::Matroska; };

  // Init
  FXbool init(InputPlugin*);

  // Seekable
  FXbool can_seek() const;

  // Seek
  FXbool seek(FXlong );

  // Process Packet
  ReadStatus process(Packet*);

  // Destroy
  ~MatroskaReader();
  };


ReaderPlugin * ap_matroska_reader(AudioEngine * engine) {
  return new MatroskaReader(engine);
  }



MatroskaReader::MatroskaReader(AudioEngine* e) : ReaderPlugin(e) {
  }

MatroskaReader::~MatroskaReader(){
  }

FXbool MatroskaReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  flags&=~FLAG_PARSED;
#ifdef DEBUG
  indent=0;
#endif
  return true;
  }

FXbool MatroskaReader::can_seek() const {
  return true;
  }

FXbool MatroskaReader::seek(FXlong offset){
  return false;
  }

ReadStatus MatroskaReader::process(Packet*packet) {
  packet->stream_position=-1;
  packet->stream_length=stream_length;

  if (!(flags&FLAG_PARSED)) {
    return parse(packet);
    }
  return ReadError;
  }

ReadStatus MatroskaReader::parse(Packet * packet) {
  FXuint element_type;
  FXlong element_size;

  if (!parse_element(element_type,element_size))
    return ReadError;

  if (element_type!=EBML || !parse_ebml(element_size))
    return ReadError;    

  while(parse_element(element_type,element_size)) {
    fxmessage("element type root %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {   
      case SEGMENT: if (!parse_segment(element_size)) return ReadError; break;
      default     : input->position(element_size,FXIO::Current); break;
      }
    }
  return ReadError;
  }



FXbool MatroskaReader::parse_track_audio(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type track audio %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case AUDIO_SAMPLE_RATE: break;
      case AUDIO_CHANNELS   : break;
      default       : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }
  return true;
  }

FXbool MatroskaReader::parse_track_entry(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type track entry %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case TRACK_TYPE : 
        {
          FXuchar track_type;
          input->read(&track_type,1);
          fxmessage("track_type %hhd\n",track_type);
          break;
        }
      case CODEC_ID :
        {
          FXString codec;
          codec.length(element_size);
          input->read(codec.text(),element_size);
          fxmessage("track codec %s\n",codec.text());
          break;
        }
      case AUDIO    : parse_track_audio(element_size); break;
      default       : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }
  return true;
  }


FXbool MatroskaReader::parse_track(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type track %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case TRACK_ENTRY: parse_track_entry(element_size); break;
      default       : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }
  return true;
  }



FXbool MatroskaReader::parse_segment(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  fxmessage("segment size %ld\n",container);
  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type segment %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case SEEK_HEAD: parse_seekhead(element_size); break;
      case TRACK    : parse_track(element_size); break;   
      default       : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    fxmessage("segment bytes left %ld\n",container);
    }
  return true;
  }


FXbool MatroskaReader::parse_seekhead(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type seekhead %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case SEEK   : parse_seek(element_size); break;   
      default     : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }
  return true;
  }

FXbool MatroskaReader::parse_seek(FXlong container) {
  FXuint element_type;
  FXlong element_size;
  FXuchar id[4];
  FXuint pos;

  while(parse_element(container,element_type,element_size)) {
    fxmessage("element type seek %x of %ld bytes\n",element_type,element_size);    
    switch(element_type) {
      case SEEK_ID      : input->read(&id,element_size); fxmessage("seek id: %hhx%hhx%hhx%hhx\n",id[0],id[1],id[2],id[3]); break;
      case SEEK_POSITION: 
          {
            if (element_size==2) {
              FXushort v;
              input->read_uint16_be(v);
              pos=v;  
              }
            else if (element_size==4) {
              input->read_uint32_be(pos);
              }
            else {
              input->position(element_size,FXIO::Current);
              //return false;
              }            
            //fxmessage("seek pos: %u\n",pos);
          } break;
      default           : input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }
  return true;
  }








FXbool MatroskaReader::parse_element(FXuint & id,FXlong & size) {
  FXlong container = 12;

  if (!parse_element_id(container,id))
    return false;

  if (!parse_element_size(container,size))
    return false;

  return true;
  }

FXbool MatroskaReader::parse_element(FXlong & container,FXuint & id,FXlong & size) {
  if (container > 2) {

    if (!parse_element_id(container,id))
      return false;

    if (!parse_element_size(container,size))
      return false;

    return true;
    }
  return false;
  }


FXbool MatroskaReader::parse_ebml(FXlong container) {
  FXuint element_type;
  FXlong element_size;

  FXString doctype;

  while(parse_element(container,element_type,element_size)) {   
    fxmessage("element type %x\n",element_type);  
    switch(element_type) {
      case EBML_DOC_TYPE             : doctype.length(element_size); 
                                       input->read(doctype.text(),element_size);
                                       break; 
      case EBML_VERSION              :
      case EBML_READ_VERSION         :
      case EBML_MAX_ID_LENGTH        :
      case EBML_MAX_SIZE_LENGTH      : 
      case EBML_DOC_TYPE_VERSION     :
      case EBML_DOC_TYPE_READ_VERSION:
      default: input->position(element_size,FXIO::Current); break;
      }
    container-=element_size;
    }

  if (doctype!="matroska")
    return false;

  return true;
  }






FXbool MatroskaReader::parse_element_id(FXlong & container,FXuint & value) {
  FXuchar buffer[4];
  if (input->read(&buffer[0],1)!=1) return false;
  if (buffer[0]>=0x80) {
    value = buffer[0];
    container-=1;
    }
  else if (buffer[0]>=0x40) {
    if (input->read(&buffer[1],1)!=1) return false;
    value = (static_cast<FXuint>(buffer[0])<<8) | static_cast<FXuint>(buffer[1]);
    container-=2;
    }
  else if (buffer[0]>=0x20) {
    if (input->read(&buffer[1],2)!=2) return false;
    value = (static_cast<FXuint>(buffer[0])<<16) | (static_cast<FXuint>(buffer[1])<<8) | static_cast<FXuint>(buffer[2]);
    container-=3;
    }
  else if (buffer[0]>=0x10) {
    if (input->read(&buffer[1],3)!=3) return false;
    container-=4;
    value = (static_cast<FXuint>(buffer[0])<<24) | 
            (static_cast<FXuint>(buffer[1])<<16) | 
            (static_cast<FXuint>(buffer[2])<<8) | 
            (static_cast<FXuint>(buffer[3]));
    }
  else {
    fxmessage("ooops got %x\n",buffer[0]);
    return false;
    }
  //fxmessage("raw element %hhx %hhx %hhx %hhx\n",buffer[0],buffer[1],buffer[2],buffer[3]);
  return true;
  }


FXbool MatroskaReader::parse_element_size(FXlong & container,FXlong & value) {
  FXuchar buffer[8];
  if (input->read(&buffer[0],1)!=1) return false;
  if (buffer[0]>=0x80) {
    value = buffer[0]&0x7f;
    container-=1;
    }
  else if (buffer[0]>=0x40) {
    if (input->read(&buffer[1],1)!=1) return false;
    value = (static_cast<FXulong>(buffer[0]&0x3F)<<8) | 
            (static_cast<FXulong>(buffer[1]));
    container-=2;
    }
  else if (buffer[0]>=0x20) {

    if (input->read(&buffer[1],2)!=2) return false;
    value = (static_cast<FXulong>(buffer[0]&0x1F)<<16) | 
            (static_cast<FXulong>(buffer[1])<<8) |
            (static_cast<FXulong>(buffer[2]));
    container-=3;
    }
  else if (buffer[0]>=0x10) {
    if (input->read(&buffer[1],3)!=3) return false;
    value = (static_cast<FXulong>(buffer[0]&0xF)<<24) | 
            (static_cast<FXulong>(buffer[1])<<16) | 
            (static_cast<FXulong>(buffer[2])<<8) | 
            (static_cast<FXulong>(buffer[3]));
    container-=4;
    }
  else if (buffer[0]>=0x8) {

    if (input->read(&buffer[1],4)!=4) return false;
    value = (static_cast<FXulong>(buffer[0]&0x7)<<32) | 
            (static_cast<FXulong>(buffer[1])<<24) | 
            (static_cast<FXulong>(buffer[2])<<16) | 
            (static_cast<FXulong>(buffer[3])<<8) | 
            (static_cast<FXulong>(buffer[4]));
    container-=5;
    }
  else if (buffer[0]>=0x4) {
    fxmessage("check %hhx\n",buffer[0]);

    if (input->read(&buffer[1],5)!=5) return false;
    value = (static_cast<FXulong>(buffer[0]&0x3)<<40) | 
            (static_cast<FXulong>(buffer[1])<<32) | 
            (static_cast<FXulong>(buffer[2])<<24) | 
            (static_cast<FXulong>(buffer[3])<<16) | 
            (static_cast<FXulong>(buffer[4])<<8) | 
            (static_cast<FXulong>(buffer[5]));
    fxmessage("value %lu\n",value);

    container-=6;
    }
  else if (buffer[0]>=0x2) {
    if (input->read(&buffer[1],6)!=6) return false;
    value = (static_cast<FXulong>(buffer[0]&0x1)<<48) | 
            (static_cast<FXulong>(buffer[1])<<40) | 
            (static_cast<FXulong>(buffer[2])<<32) | 
            (static_cast<FXulong>(buffer[3])<<24) | 
            (static_cast<FXulong>(buffer[4])<<16) | 
            (static_cast<FXulong>(buffer[5])<<8) | 
            (static_cast<FXulong>(buffer[6]));
    container-=7;
    }
  else if (buffer[0]>=0x1) {
    if (input->read(&buffer[1],7)!=7) return false;
    value = (static_cast<FXulong>(buffer[1])<<48) | 
            (static_cast<FXulong>(buffer[2])<<40) | 
            (static_cast<FXulong>(buffer[3])<<32) | 
            (static_cast<FXulong>(buffer[4])<<24) | 
            (static_cast<FXulong>(buffer[5])<<16) | 
            (static_cast<FXulong>(buffer[6])<<8) | 
            (static_cast<FXulong>(buffer[7]));
    container-=8;
    }
  else {
    return false;
    }
  return true;
  }







  



}



