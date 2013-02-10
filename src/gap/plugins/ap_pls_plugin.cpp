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
#include "ap_common.h"
#include "ap_pipe.h"
#include "ap_format.h"
#include "ap_device.h"
#include "ap_event.h"
#include "ap_event_private.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_buffer.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_reader_plugin.h"
#include "ap_decoder_plugin.h"
#include "ap_thread.h"
#include "ap_input_thread.h"
#include "ap_buffer.h"
#include "ap_decoder_thread.h"
#include "ap_output_thread.h"

namespace ap {

void ap_parse_pls(const FXString & data,FXStringList & mrl) {
  FXint start=0,end=0,pos,next;
  for (FXint i=0;i<data.length();i++) {
    if (data[i]=='\n') {
      end=i;
      next=i+1;

      /// Skip white space
      while(start<end && Ascii::isSpace(data[start])) start++;

      /// Skip white space
      while(end>start && Ascii::isSpace(data[end])) end--;

      /// Parse the actual line.
      if ((end-start)>6) {
        if (compare(&data[start],"File",4)==0) {
          pos = data.find('=',start+4);
          if (pos==-1) continue;
          pos++;
          if (end-pos>0) {
            mrl.append(data.mid(pos,1+end-pos));
            }
          }
        }
      start=next;
      }
    }
  }



class PLSReader : public TextReader {
protected:
  FXStringList uri;
public:
  PLSReader(AudioEngine*);
  ReadStatus process(Packet*);
  FXbool init(InputPlugin*);
  FXuchar format() const { return Format::PLS; };
  FXbool redirect(FXStringList & u) { u=uri; return true; }
  virtual ~PLSReader();
  };



PLSReader::PLSReader(AudioEngine*e) : TextReader(e) {
  }

PLSReader::~PLSReader(){
  }

FXbool PLSReader::init(InputPlugin*plugin) {
  TextReader::init(plugin);
  uri.clear();
  return true;
  }

ReadStatus PLSReader::process(Packet*packet) {
  if (TextReader::process(packet)==ReadDone) {
    ap_parse_pls(textbuffer,uri);
    if (uri.no())
      return ReadRedirect;
    else
      return ReadDone;
    }
  return ReadError;
  }

ReaderPlugin * ap_pls_reader(AudioEngine * engine) {
  return new PLSReader(engine);
  }

}
