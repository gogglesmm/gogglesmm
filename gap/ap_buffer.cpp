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
#include "ap_buffer.h"

#define ROUNDVAL    16
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

namespace ap {

BufferBase::BufferBase(FXival n) {
  allocElms(begptr,ROUNDUP(n));
  endptr=begptr+n;
  wrptr=begptr;
  rdptr=begptr;
  }

BufferBase::~BufferBase(){
  freeElms(begptr);
  }

void BufferBase::clear() {
  wrptr=rdptr=begptr;
  }

FXbool BufferBase::resize(FXival n) {
  FXASSERT(n>0);
  if(begptr+n!=endptr){
    FXuchar *oldbegptr=begptr;

    // Resize the buffer
    if(!resizeElms(begptr,ROUNDUP(n))) return false;

    // Adjust pointers, buffer may have moved
    endptr=begptr+n;
    wrptr=begptr+(wrptr-oldbegptr);
    rdptr=begptr+(rdptr-oldbegptr);
    if(wrptr>endptr) wrptr=endptr;
    if(rdptr>endptr) rdptr=endptr;
    }
  return true;
  }

FXbool BufferBase::reserve(FXival n) {
  FXASSERT(n>0);
  if (n>(endptr-wrptr)) {
    if (rdptr>begptr) {
      if (rdptr<wrptr) {
        memmove(begptr,rdptr,wrptr-rdptr);
        wrptr-=(rdptr-begptr);
        rdptr=begptr;
        }
      else {
        rdptr=wrptr=begptr;
        }
      if (n<=endptr-wrptr)
        return true;
      }
    return resize((endptr-begptr)+(n-(endptr-wrptr)));
    }
  return true;
  }



//----------------------------------------------

MemoryBuffer::MemoryBuffer(FXival cap) : BufferBase(cap) {
  }

MemoryBuffer::~MemoryBuffer() {
  }

void MemoryBuffer::readBytes(FXival nbytes) {
  FXASSERT(nbytes<=size());
  rdptr+=nbytes;
  }

void MemoryBuffer::wroteBytes(FXival nbytes) {
  FXASSERT(nbytes<=space());
  wrptr+=nbytes;
  }

void MemoryBuffer::append(const void * b,FXival nbytes) {
  if (nbytes) {
    reserve(nbytes);
    memcpy(wrptr,b,nbytes);
    wrptr+=nbytes;
    }
  }

void MemoryBuffer::append(const FXchar c,FXival nbytes/*=1*/) {
  FXASSERT(nbytes>=1);
  reserve(nbytes);
  while(nbytes--) *wrptr++=c;
  }


FXival MemoryBuffer::read(void * b, FXival nbytes) {
  nbytes=FXMIN(size(),nbytes);
  memcpy(b,rdptr,nbytes);
  readBytes(nbytes);
  return nbytes;
  }

FXival MemoryBuffer::peek(void * b, FXival nbytes) {
  nbytes=FXMIN(size(),nbytes);
  memcpy(b,rdptr,nbytes);
  return nbytes;
  }

void MemoryBuffer::trimBegin(FXival nbytes) {
  readBytes(nbytes);
  }

void MemoryBuffer::trimEnd(FXival nbytes) {
  FXASSERT(nbytes<=size());
  wrptr-=nbytes;
  }

}
