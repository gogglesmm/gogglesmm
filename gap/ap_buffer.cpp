/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#include "ap_defs.h"
#include "ap_buffer.h"

#include <openssl/err.h>

#define CACHE_ALIGN 64

namespace ap {

BufferBase::BufferBase(FXival n) : rawptr(nullptr) {
  const FXival nbytes = n + (CACHE_ALIGN - 1);
  if (allocElms(rawptr, nbytes)) {
    const auto raw_addr = reinterpret_cast<FXuval>(rawptr);
    const FXuval aligned_addr = (raw_addr + (CACHE_ALIGN - 1)) & ~(static_cast<FXuval>(CACHE_ALIGN - 1));
    begptr = reinterpret_cast<FXuchar*>(aligned_addr);
    endptr = begptr + n;
    wrptr  = begptr;
    rdptr  = begptr;

    // zero out areas beyond requested buffer size
    if (begptr > rawptr)
      memset(rawptr, 0, begptr - rawptr);
    const FXuchar * bufptr = rawptr + nbytes;
    if (bufptr > endptr)
      memset(endptr, 0, bufptr - endptr);
    }
  }

BufferBase::~BufferBase(){
  freeElms(rawptr);
  rawptr = nullptr;
  begptr = nullptr;
  endptr = nullptr;
  wrptr = nullptr;
  rdptr = nullptr;
  }

void BufferBase::adopt(BufferBase & other) {
  freeElms(rawptr);
  rawptr = other.rawptr;
  begptr = other.begptr;
  endptr = other.endptr;
  wrptr  = other.wrptr;
  rdptr  = other.rdptr;
  other.rawptr = nullptr;
  other.begptr = nullptr;
  other.endptr = nullptr;
  other.wrptr = nullptr;
  other.rdptr = nullptr;
  }

void BufferBase::clear() {
  wrptr=rdptr=begptr;
  }

FXbool BufferBase::resize(FXival n) {
  FXASSERT(n>0);
  const FXival nbytes = n + (CACHE_ALIGN - 1);
  if(begptr+n!=endptr) {

    // Old buffer pointers
    FXuchar *oldrawptr=rawptr;
    FXuchar *oldwrptr=wrptr;
    FXuchar *oldrdptr=rdptr;

    rawptr = nullptr;
    if (!allocElms(rawptr, nbytes)) {
      rawptr = oldrawptr;
      return false;
    }
    const auto raw_addr = reinterpret_cast<FXuval>(rawptr);
    const FXuval aligned_addr = (raw_addr + (CACHE_ALIGN - 1)) & ~(static_cast<FXuval>(CACHE_ALIGN - 1));

    begptr = reinterpret_cast<FXuchar*>(aligned_addr);
    endptr = begptr + n;

    // zero out areas beyond requested buffer size
    if (begptr > rawptr)
      memset(rawptr, 0, begptr - rawptr);
    const FXuchar * bufptr = rawptr + nbytes;
    if (bufptr > endptr)
      memset(endptr, 0, bufptr - endptr);

    FXival avail = (oldwrptr > oldrdptr) ? (oldwrptr - oldrdptr) : 0;
    if (avail > 0) {
      FXival to_copy = (avail < n) ? avail : n;
      copyElms(begptr, oldrdptr, to_copy);
      wrptr = begptr + to_copy;
      rdptr = begptr;
      }
    else {
      wrptr = begptr;
      rdptr = begptr;
      }
    freeElms(oldrawptr);
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


void BufferBase::align() {
  if (rdptr > begptr) {
    if (wrptr > rdptr) {
      memmove(begptr, rdptr, wrptr - rdptr);
      wrptr -= (rdptr - begptr);
      rdptr = begptr;
      }
    else {
      rdptr=wrptr=begptr;
      }
    }
  }



//----------------------------------------------

MemoryBuffer::MemoryBuffer(FXival cap) : BufferBase(cap) {
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

FXival MemoryBuffer::peek(void * b, FXival nbytes) const {
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
