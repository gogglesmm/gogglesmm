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
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_memory_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"
#include "ap_xml_parser.h"

namespace ap {


class ASXParser : public XMLStream {
public:
  FXStringList files;
protected:
  FXint begin(const FXchar *,const FXchar**);
  void end(const FXchar *);
protected:
  FXint        elem;
public:
  enum {
    Elem_None,
    Elem_ASX,
    Elem_ASX_Entry,
    Elem_ASX_Entry_Ref,
    };
public:
  ASXParser();
  ~ASXParser();
  };


ASXParser::ASXParser() : elem(Elem_None) {
  }

ASXParser::~ASXParser(){
  }

FXint ASXParser::begin(const FXchar * element,const FXchar ** attributes){
  switch(elem) {
    case Elem_None:
      {
        if (comparecase(element,"asx")==0) {
          elem=Elem_ASX;
          return 1;
          }
      } break;
    case Elem_ASX:
      {
        if (comparecase(element,"entry")==0) {
          elem=Elem_ASX_Entry;
          return 1;
          }
      } break;
    case Elem_ASX_Entry:
      {
        if (comparecase(element,"ref")==0) {
          elem=Elem_ASX_Entry_Ref;
          for (FXint i=0;attributes[i];i+=2){
            if (comparecase(attributes[i],"href")==0) {
              files.append(FXString(attributes[i+1]));
              }
            }
          return 1;
          }
      } break;
    default: return 0; // skip
    }
  return 0;
  }

void ASXParser::end(const FXchar*) {
  switch(elem){
    case Elem_ASX_Entry_Ref     : elem=Elem_ASX_Entry; break;
    case Elem_ASX_Entry         : elem=Elem_ASX; break;
    case Elem_ASX               : elem=Elem_None; break;
    default                     : elem=Elem_None; break;
    }
  }

static void gm_parse_asx(FXString & data,FXStringList & mrl) {
  ASXParser asx;


  /// FIXME replace with nifty regex
  data.substitute("&","&amp;");

  if (asx.parse(data)) {
    mrl=asx.files;
    }
  }


class ASXReader : public TextReader {
protected:
  FXStringList uri;
public:
  ASXReader(AudioEngine*);
  ReadStatus process(Packet*) override;
  FXbool init(InputPlugin) override;
  FXuchar format() const override { return Format::ASX; };
  FXbool redirect(FXStringList & u) override { u=uri; return true; }
  virtual ~ASXReader();
  };


ASXReader::ASXReader(AudioEngine*e) : TextReader(e) {
  }

ASXReader::~ASXReader(){
  }

FXbool ASXReader::init(InputPlugin*plugin) {
  TextReader::init(plugin);
  uri.clear();
  return true;
  }

ReadStatus ASXReader::process(Packet*packet) {
  if (TextReader::process(packet)==ReadDone) {
    gm_parse_asx(textbuffer,uri);
    if (uri.no())
      return ReadRedirect;
    else
      return ReadDone;
    }
  return ReadError;
  }

ReaderPlugin * ap_asx_reader(AudioEngine * engine) {
  return new ASXReader(engine);
  }


}
