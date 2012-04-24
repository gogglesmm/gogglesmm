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
#include "ap_buffer.h"

#define ROUNDVAL    16
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

namespace ap {

MemoryBuffer::MemoryBuffer(FXival cap) :
  buffer(NULL),
  buffersize(0),
  rdptr(NULL),
  wrptr(NULL) {
  reserve(cap);
  }

MemoryBuffer::MemoryBuffer(const MemoryBuffer & other) :
  buffer(NULL),
  buffersize(0),
  rdptr(NULL),
  wrptr(NULL) {
  reserve(other.capacity());
  append(other.data(),other.size());
  }

MemoryBuffer::~MemoryBuffer() {
  freeElms(buffer);
  buffersize=0;
  wrptr=rdptr=NULL;
  }

// Assignment operator
MemoryBuffer& MemoryBuffer::operator=(const MemoryBuffer& other) {
  clear();
  reserve(other.capacity());
  append(other.data(),other.size());
  return *this;
  }

// Append operator
MemoryBuffer& MemoryBuffer::operator+=(const MemoryBuffer& other) {
  reserve(other.capacity());
  append(other.data(),other.size());
  return *this;
  }


void MemoryBuffer::adopt(MemoryBuffer & other) {

  // Take over
  buffer=other.buffer;
  buffersize=other.buffersize;
  rdptr=other.rdptr;
  wrptr=other.wrptr;

  // Reset Other
  other.buffer=other.rdptr=other.wrptr=NULL;
  other.buffersize=0;
  }


// Clear
void MemoryBuffer::clear() {
  wrptr=rdptr=buffer;
  }

// Clear and reset
void MemoryBuffer::reset(FXival nbytes) {
  clear();
  if (buffersize!=nbytes) {
    buffersize=nbytes;
    resizeElms(buffer,buffersize);
    wrptr=rdptr=buffer;
    }
  }


// Make room for needed bytes
void MemoryBuffer::reserve(FXival needed) {
  if (needed>0) {
    if (buffer) {
      FXival avail = space();

      /// Check if we can move back rdptr/wrptr
      if (avail<needed) {
        if (rdptr>buffer) {
          if (wrptr>rdptr) {
            memmove(buffer,rdptr,wrptr-rdptr);
            wrptr-=(rdptr-buffer);
            rdptr = buffer;
            avail = space();
            }
          else {
            wrptr=rdptr=buffer;
            avail=buffersize;
            }
          }
        }

      /// Still not enough space, resize buffer
      if (avail<needed) {
        buffersize=ROUNDUP(buffersize+(needed-avail));
        avail=wrptr-buffer;
        resizeElms(buffer,buffersize);
        wrptr=rdptr=buffer;
        wrptr+=avail;
        }
      }
    else {
      buffersize=needed;
      allocElms(buffer,buffersize);
      wrptr=rdptr=buffer;
      }
    }
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
  //FXASSERT(nbytes<size());
  nbytes=FXMIN(size(),nbytes);
  memcpy(b,rdptr,nbytes);
  readBytes(nbytes);
  return nbytes;
  }

FXival MemoryBuffer::peek(void * b, FXival nbytes) {
//  FXASSERT(nbytes<size());
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
