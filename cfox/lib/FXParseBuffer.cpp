/********************************************************************************
*                                                                               *
*                            P a r s e - B u f f e r                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXException.h"
#include "FXParseBuffer.h"

/*
  Notes:

  - When reading, the read pointer (rptr) represents the oldest character we intend
    to keep when the buffer is being filled.  The scan pointer (sptr) represents the
    next character about to be parsed; it should not catch up with the write pointer
    (wptr) which is where new data is being read from storage (when reading).
  - When writing, the write pointer is where new data will be dropped into the
    buffer, and the read pointer is pointing to the first byte not yet saved to
    storage.
  - One may read from memory as if it were a file, simply by passing a non-0 length;
    the size of the buffer must be at least equal to length, but may be the same.
  - End-of-file is signified as a partially filled buffer; thus, subclasses must
    initialize rptr, wptr, and sptr to endptr at the start.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Initialize parse buffer
FXParseBuffer::FXParseBuffer():begptr(nullptr),endptr(nullptr),wptr(nullptr),rptr(nullptr),sptr(nullptr),dir(Stop){
  }


// Initialize parse buffer with given size and direction
FXParseBuffer::FXParseBuffer(FXchar* buffer,FXuval sz,Direction d):begptr(nullptr),endptr(nullptr),wptr(nullptr),rptr(nullptr),sptr(nullptr),dir(Stop){
  open(buffer,sz,d);
  }


// Open parse buffer with given size and direction
FXbool FXParseBuffer::open(FXchar* buffer,FXuval sz,Direction d){
  if((dir==Stop) && (d!=Stop) && (0<sz) && buffer){
    begptr=buffer;
    endptr=buffer+sz;
    wptr=(d==Load)?endptr:begptr;
    rptr=begptr;
    sptr=begptr;
    dir=d;
    return true;
    }
  return false;
  }


// Read at least count bytes into buffer; return bytes available, or -1 for error
FXival FXParseBuffer::fill(FXival){
  return wptr-sptr;
  }


// Write at least count bytes from buffer; return space available, or -1 for error
FXival FXParseBuffer::flush(FXival){
  return endptr-wptr;
  }


// Read characters into buffer when fewer than count remain.
// If previous fill() left the buffer fully populated, call fill()
// again; if previous fill() left buffer partially populated, we're
// at the end of the file and will stop reading.
FXbool FXParseBuffer::need(FXival count){
  FXASSERT(dir==Load);
  if(sptr+count>wptr){
    if(wptr==endptr){
      if(fill(count)<0) return false;
      }
    return sptr<wptr;
    }
  return true;
  }


// Write characters to buffer.
// While characters remain, append as much as we can to the end of the
// buffer, calling flush whenever the buffer fills up.
FXbool FXParseBuffer::emit(FXchar ch,FXint count){
  FXival num;
  FXASSERT(dir==Save);
  while(0<count){
    if(wptr>=endptr){
      if(flush(count)<=0) return false;
      }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    fillElms(wptr,ch,num);
    wptr+=num;
    count-=num;
    }
  return true;
  }


// Write string to buffer.
// While characters remain, append as much as we can to the end of the
// buffer, calling flush whenever the buffer fills up.
FXbool FXParseBuffer::emit(const FXchar* str,FXint count){
  FXival num;
  FXASSERT(dir==Save);
  while(0<count){
    if(wptr>=endptr){
      if(flush(count)<=0) return false;
      }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    copyElms(wptr,str,num);
    wptr+=num;
    str+=num;
    count-=num;
    }
  return true;
  }


// Close parse buffer
FXbool FXParseBuffer::close(){
  if(dir!=Stop){
    if((dir==Load) || 0<=flush(0)){     // Error during final flush is possible
      begptr=nullptr;
      endptr=nullptr;
      wptr=nullptr;
      rptr=nullptr;
      sptr=nullptr;
      dir=Stop;
      return true;
      }
    begptr=nullptr;
    endptr=nullptr;
    wptr=nullptr;
    rptr=nullptr;
    sptr=nullptr;
    dir=Stop;
    }
  return false;
  }


// Clean up and close buffer
FXParseBuffer::~FXParseBuffer(){
  close();
  begptr=(FXchar*)-1L;
  endptr=(FXchar*)-1L;
  wptr=(FXchar*)-1L;
  rptr=(FXchar*)-1L;
  begptr=(FXchar*)-1L;
  sptr=(FXchar*)-1L;
  }

}
