/********************************************************************************
*                                                                               *
*                     G Z F i l e S t r e a m   C l a s s e s                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Sander Jansen.   All Rights Reserved.              *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXElement.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXObject.h"
#include "FXFile.h"
#include "FXGZFileStream.h"

#ifdef HAVE_ZLIB_H
#include "zlib.h"

/*
  Notes:
  - Very basic compressed file I/O only.
  - Updated for new stream classes 2003/07/08.
  - Updated for FXFile 2005/09/03.
*/

#define BUFFERSIZE 8192

/*******************************************************************************/

namespace FX {


// Used during compression
struct ZBlock {
  z_stream stream;
  Bytef    buffer[BUFFERSIZE];
  };


// Create GZIP compressed file stream
FXGZFileStream::FXGZFileStream(const FXObject* cont):FXFileStream(cont),gz(nullptr),ac(0){
  }


// Create and open GZIP compressed file stream
FXGZFileStream::FXGZFileStream(const FXString& filename,FXStreamDirection save_or_load,FXuval size):gz(nullptr),ac(0){
  open(filename,save_or_load,size);
  }


// Save to a file
FXuval FXGZFileStream::writeBuffer(FXuval){
  FXival m,n; int zerror;
  if(dir!=FXStreamSave){fxerror("FXGZFileStream::writeBuffer: wrong stream direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  while(rdptr<wrptr || ac==Z_FINISH || ac==Z_SYNC_FLUSH){
    gz->stream.next_in=(Bytef*)rdptr;
    gz->stream.avail_in=wrptr-rdptr;
    gz->stream.next_out=gz->buffer;
    gz->stream.avail_out=BUFFERSIZE;
    zerror=deflate(&gz->stream,ac);
    if(zerror<Z_OK) break;                              // Error occurred
    m=gz->stream.next_out-gz->buffer;
    n=file.writeBlock(gz->buffer,m);
    if(n<m) break;                                      // Failed to write data
    rdptr=(FXuchar*)gz->stream.next_in;
    if(zerror==Z_STREAM_END) break;                     // Flushed or finished all data
    }
  if(rdptr<wrptr){memmove(begptr,rdptr,wrptr-rdptr);}
  wrptr=begptr+(wrptr-rdptr);
  rdptr=begptr;
  return endptr-wrptr;
  }


// Load from file
FXuval FXGZFileStream::readBuffer(FXuval){
  FXival n; int zerror;
  if(dir!=FXStreamLoad){fxerror("FXGZFileStream::readBuffer: wrong stream direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  if(rdptr<wrptr){memmove(begptr,rdptr,wrptr-rdptr);}
  wrptr=begptr+(wrptr-rdptr);
  rdptr=begptr;
  while(wrptr<endptr){
    if(gz->stream.avail_in<=0){                         // Read more input
      n=file.readBlock(gz->buffer,BUFFERSIZE);
      if(n<=0) break;
      gz->stream.next_in=gz->buffer;
      gz->stream.avail_in=n;
      }
    gz->stream.next_out=(Bytef*)wrptr;
    gz->stream.avail_out=endptr-wrptr;
    zerror=inflate(&gz->stream,Z_NO_FLUSH);
    if(zerror<Z_OK) break;                              // Error occurred
    wrptr=(FXuchar*)gz->stream.next_out;
    if(zerror==Z_STREAM_END) break;
    }
  return wrptr-rdptr;
  }


// Try open file stream
FXbool FXGZFileStream::open(const FXString& filename,FXStreamDirection save_or_load,FXuval size){
  if(FXFileStream::open(filename,save_or_load,size)){
    if(callocElms(gz,1)){
      int zerror;
      gz->stream.next_in=nullptr;
      gz->stream.avail_in=0;
      gz->stream.next_out=nullptr;
      gz->stream.avail_out=0;
      ac=Z_NO_FLUSH;
      if(save_or_load==FXStreamLoad){
        zerror=inflateInit(&gz->stream);
        if(zerror==Z_OK) return true;
        code=FXStreamNoRead;
        }
      else{
        zerror=deflateInit(&gz->stream,Z_DEFAULT_COMPRESSION);
        if(zerror==Z_OK) return true;
        code=FXStreamNoWrite;
        }
      freeElms(gz);
      }
    FXFileStream::close();
    }
  return false;
  }


// Flush buffer
FXbool FXGZFileStream::flush(){
  FXbool result;
  int action=ac;
  if(ac!=Z_FINISH) ac=Z_SYNC_FLUSH;
  result=FXStream::flush();
  ac=action;
  return result;
  }



// Close file stream
FXbool FXGZFileStream::close(){
  if(dir){
    if(dir==FXStreamLoad){
      FXFileStream::close();
      inflateEnd(&gz->stream);
      }
    else{
      ac=Z_FINISH;
      FXFileStream::close();
      deflateEnd(&gz->stream);
      }
    freeElms(gz);
    return true;
    }
  return false;
  }


// Destructor
FXGZFileStream::~FXGZFileStream(){
  close();
  }

}

#endif
