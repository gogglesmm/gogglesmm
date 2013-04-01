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
#include "ap_config.h"
#include "ap_defs.h"
#include "ap_utils.h"
#include "ap_pipe.h"
#include "ap_event.h"
#include "ap_format.h"
#include "ap_buffer.h"
#include "ap_input_plugin.h"
#include "ap_event_queue.h"
#include "ap_thread_queue.h"
#include "ap_packet.h"
#include "ap_engine.h"
#include "ap_thread.h"
#include "ap_thread_queue.h"
#include "ap_input_thread.h"

using namespace ap;

namespace ap {


//InputPlugin::InputPlugin(InputThread * i,FXival sz) : input(i), buffer(sz) {
//  }

InputPlugin::InputPlugin(InputThread * i) : input(i){
  }

InputPlugin::~InputPlugin() {
  }
/*
FXival InputPlugin::io_buffer(FXival count) {
  register FXival nread=0;
  register FXival n;
  buffer.reserve(count);
  while(nread<count) {
    n=io_read(buffer.ptr(),count-nread);
    if (__likely(n>0)){
      buffer.wroteBytes(n);
      nread+=n;
      }
    else if (n==0){
      return nread;
      }
    else if (n==AP_IO_BLOCK) {
      // Only block if we haven't received any bytes yet.
      // This prevents us from locking up if we don't know how many bytes we'll receive.
      if (nread)
        return nread;
      else if (!io_wait_read())
        return AP_IO_BLOCK;
      }
    else {
      return AP_IO_ERROR;
      }
    }
  return nread;
  }



FXival InputPlugin::io_read_block(void*ptr,FXival count) {
  FXchar * buf = static_cast<FXchar*>(ptr);
  register FXival nread=0;
  register FXival n;
  while(nread<count) {
    n=io_read(buf+nread,count-nread);
    if (__likely(n>0)){
      nread+=n;
      }
    else if (n==0){
      return nread;
      }
    else if (n==AP_IO_BLOCK) {
      if (!io_wait_read())
        return AP_IO_BLOCK;
      }
    else {
      return AP_IO_ERROR;
      }
    }
  return nread;
  }


FXival InputPlugin::io_write_block(const void*ptr,FXival count) {
  const FXchar * buf = static_cast<const FXchar*>(ptr);
  register FXival nwrite=0;
  register FXival n;
  while(nwrite<count) {
    n=io_write(buf+nwrite,count-nwrite);
    if (__likely(n>0)){
      nwrite+=n;
      }
    else if (n==0){
      return nwrite;
      }
    else if (n==AP_IO_BLOCK) {
      if (!io_wait_write())
        return AP_IO_BLOCK;
      }
    else {
      return AP_IO_ERROR;
      }
    }
  return nwrite;
  }



FXbool InputPlugin::io_wait_read() {
  do {
    FXuint x = ap_wait_read(input->getFifoHandle(),io_handle());
    switch(x) {
       case WIO_TIMEOUT      : return false; break;
       case WIO_HANDLE       : return true; break;
       default               :

          if (input->aborted()){
            return false;
            }
          else if (x==WIO_BOTH)
            return true;
          else
            continue;
      }
    }
  while(1);
  }


FXbool InputPlugin::io_wait_write() {
  do {
    FXuint x = ap_wait_write(input->getFifoHandle(),io_handle());
    switch(x) {
       case WIO_TIMEOUT      : return false; break;
       case WIO_HANDLE       : return true; break;
       default               :

          if (input->aborted()){
            return false;
            }
          else if (x==WIO_BOTH)
            return true;
          else
            continue;
      }
    }
  while(1);
  }


FXival InputPlugin::preview(void*data,FXival count) {
  if (serial() || buffer.size()) {
    if (buffer.size()<count) {
      buffer.reserve(count-buffer.size());
      FXival n=InputPlugin::io_read_block(buffer.ptr(),count-buffer.size());
      if (n>0)
        buffer.wroteBytes(n);
      else if (n<0 && buffer.size()==0)
        return n;
      }
    return buffer.peek(data,count);
    }
  else { // no need to buffer if we have non-serial streams
    FXlong readpos = position();
    FXival n = InputPlugin::io_read_block(data,count);
    position(readpos,FXIO::Begin);
    return n;
    }
  }

FXival InputPlugin::read(void * d,FXival count){
  if (__unlikely(buffer.size()>0)) {
    FXchar * data = (FXchar*)d;
    FXival nbuffer = buffer.read(data,count);
    if (nbuffer==count) return nbuffer;
    FXival nblock = InputPlugin::io_read_block(data+nbuffer,count-nbuffer);
    if (nblock<0) return nblock;
    return nbuffer+nblock;
    }
  else {
    return InputPlugin::io_read_block(d,count);
    }
  }
*/
}
