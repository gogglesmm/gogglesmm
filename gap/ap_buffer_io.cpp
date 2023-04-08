/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#include "ap_buffer_io.h"

namespace ap {

BufferIO::BufferIO(FXuval sz) : BufferBase(sz),io(nullptr),dir(DirNone) {
  }

#if FOXVERSION >= FXVERSION(1, 7, 82)
BufferIO::BufferIO(FXIO * stream,FXuval sz) : FXIO(),BufferBase(sz),io(stream),dir(DirNone) {
  }
#else
BufferIO::BufferIO(FXIO * stream,FXuval sz) : FXIO(stream->mode()),BufferBase(sz),io(stream),dir(DirNone) {
  }
#endif


BufferIO::~BufferIO() {
  close();
  }


// Attach an IO. Close and delete existing
void BufferIO::attach(FXIO * stream) {
  close();
#if FOXVERSION < FXVERSION(1, 7, 82)
  access=stream->mode();
#endif
  io=stream;
  }


// Return attached io
FXIO * BufferIO::attached() const {
  return io;
  }


// Read at least count bytes into the buffer
FXuval BufferIO::readBuffer(){
  FXival m,n;
  if(dir==DirWrite) {fxerror("BufferIO::readBuffer: wrong io direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  m=wrptr-rdptr;
  if(m){memmove(begptr,rdptr,m);}
  rdptr=begptr;
  wrptr=begptr+m;
  n=io->readBlock(wrptr,endptr-wrptr);
  if(0<n){
    wrptr+=n;
    }
  dir = (wrptr>rdptr) ? DirRead : DirNone;
  return wrptr-rdptr;
  }

// Write at least count bytes from the buffer
FXuval BufferIO::writeBuffer(){
  FXival m,n;
  if(dir==DirRead) {fxerror("BufferIO::writeBuffer: wrong io direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  m=wrptr-rdptr;
  n=io->writeBlock(rdptr,m);
  if(0<n){
    m-=n;
    if(m)
      memmove(begptr,rdptr+n,m);
    else
      dir=DirNone;
    rdptr=begptr;
    wrptr=begptr+m;
    }
  return endptr-wrptr;
  }


FXbool BufferIO::flushBuffer() {
  if (dir==DirWrite && wrptr>rdptr) {
    FXuchar*p=wrptr;
    while(wrptr>rdptr) {
      writeBuffer();
      if (p==wrptr) return false;
      p=wrptr;
      }
    }
  rdptr=wrptr=begptr;
  dir=DirNone;
  return true;
  }



FXbool BufferIO::isOpen() const {
  if (dir==DirRead)
    return (wrptr-rdptr || (io && io->isOpen()));
  else
    return (io && io->isOpen());
  }


FXbool BufferIO::isSerial() const {
  return io->isSerial();
  }


/// Get current file position
FXlong BufferIO::position() const {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  if (dir==DirWrite)
    return io->position() + (wrptr-rdptr);
  else if (dir==DirRead)
    return io->position() - (wrptr-rdptr);
  else
    return io->position();
  }

/// Change file position, returning new position from start
FXlong BufferIO::position(FXlong offset,FXuint from){
  if (dir==DirRead)
    wrptr=rdptr=begptr;
  else if (dir==DirWrite && wrptr-rdptr)
    writeBuffer();
  return io->position(offset,from);
  }


FXival BufferIO::peekBlock(void* data,FXival n) {
  if (isSerial()) {
    FXival nr=0;
    FXuval avail;

    if (endptr-begptr<n && !resize(n))
      return -1;

    while(wrptr-rdptr<n) {
      avail = wrptr-rdptr;
      if (readBuffer()>avail)
        continue;
      break;
      }

    FXuchar * pkptr = rdptr;
    FXuchar * p			= (FXuchar*)data;
    dir=DirRead;
    do{
      *p++=*pkptr++;
      nr++;
      n--;
      }
    while(0<n && rdptr<wrptr);
    return nr;
    }
  else {
    FXlong pos = position();
    FXival nr = readBlock(data,n);
    position(pos,FXIO::Begin);
    return nr;
    }
  return -1;
  }

/// Read block of bytes
FXival BufferIO::readBlock(void* data,FXival n) {
  FXASSERT(dir!=DirWrite);
  if (dir!=DirWrite) {
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    FXuchar * p=(FXuchar*)data;
    FXival nr=0;
    while(0<n){
      if(rdptr+n>wrptr && readBuffer()<1) { return nr; }
      FXASSERT(rdptr<wrptr);
      dir=DirRead;
      do{
        *p++=*rdptr++;
        nr++;
        n--;
        }
      while(0<n && rdptr<wrptr);
      }
    dir = (wrptr>rdptr) ? DirRead : DirNone;
    return nr;
    }
  return -1;
  }


/// Read block of bytes
FXival BufferIO::writeBlock(const void* data,FXival n) {
  FXASSERT(dir!=DirRead);
  if (dir!=DirRead) {
    FXASSERT(begptr<=rdptr);
    FXASSERT(rdptr<=wrptr);
    FXASSERT(wrptr<=endptr);
    FXuchar * p=(FXuchar*)data;
    FXival nr=0;
    while(0<n){
      if(wrptr+n>endptr && writeBuffer()<1){ return nr; }
      dir=DirWrite;
      FXASSERT(wrptr<endptr);
      do{
        *wrptr++=*p++;
        nr++;
        n--;
        }
      while(0<n && wrptr<endptr);
      }
    return nr;
    }
  return -1;
  }


/// Truncate file
FXlong BufferIO::truncate(FXlong sz) {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  flushBuffer();
  return io->truncate(sz);
  }

FXbool BufferIO::flush() {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  return flushBuffer();
  }

FXint BufferIO::eof() {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  if (dir==DirRead && wrptr-rdptr)
    return 0;
  return io->eof();
  }

FXlong BufferIO::size() {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  if (dir==DirWrite)
    return io->size() + (wrptr-rdptr);
  else
    return io->size();
  }

/// Close handle
FXbool BufferIO::close() {
  FXASSERT(wrptr);
  FXASSERT(rdptr);
  if (isOpen()) {
    flushBuffer();
    rdptr=begptr;
    wrptr=begptr;
    dir=DirNone;
    io->close();
    delete io;
    io=nullptr;
    return true;
    }
  return true;
  }


}
