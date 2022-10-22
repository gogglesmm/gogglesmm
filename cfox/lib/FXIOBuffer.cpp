/********************************************************************************
*                                                                               *
*                        I / O   B u f f e r   C l a s s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIOBuffer.h"


/*
  Notes:
  - A fixed size memory buffer that can be accessed through file API.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct
FXIOBuffer::FXIOBuffer():buffer(nullptr),space(0L){
  }


// Construct and open
FXIOBuffer::FXIOBuffer(FXuchar* ptr,FXuval sz,FXuint m):buffer(nullptr),space(0L){
  open(ptr,sz,m);
  }


// Open buffer
FXbool FXIOBuffer::open(FXuchar* ptr,FXuval sz,FXuint m){
  if(ptr && sz && (m&ReadWrite)){
    buffer=ptr;
    space=sz;
    access=m;
    pointer=0L;
    return true;
    }
  return false;
  }


// Return true if open
FXbool FXIOBuffer::isOpen() const {
  return buffer!=nullptr;
  }


// Return true if serial access only
FXbool FXIOBuffer::isSerial() const {
  return false;
  }


// Get position
FXlong FXIOBuffer::position() const {
  return pointer;
  }


// Move to position
FXlong FXIOBuffer::position(FXlong offset,FXuint from){
  if(__likely(access&ReadWrite)){
    if(from==Current) offset=pointer+offset;
    else if(from==End) offset=space+offset;
    if(0<=offset && offset<=(FXlong)space){
      pointer=offset;
      return pointer;
      }
    }
  return -1;
  }


// Read block
FXival FXIOBuffer::readBlock(void* ptr,FXival count){
  if(__likely(access&ReadOnly)){
    FXival remaining=space-pointer;
    if(count>remaining) count=remaining;
    memcpy(ptr,&buffer[pointer],count);
    pointer+=count;
    return count;
    }
  return 0;
  }


// Write block
FXival FXIOBuffer::writeBlock(const void* ptr,FXival count){
  if(__likely(access&WriteOnly)){
    FXival remaining=space-pointer;
    if(count>remaining) count=remaining;
    memcpy(&buffer[pointer],ptr,count);
    pointer+=count;
    return count;
    }
  return 0;
  }


// Truncate file
FXlong FXIOBuffer::truncate(FXlong sz){
  if(buffer && 0<=sz && sz<=(FXlong)space){
    if(pointer>sz) pointer=sz;
    space=sz;
    return sz;
    }
  return -1;
  }


// Synchronize disk with cached data
FXbool FXIOBuffer::flush(){
  return true;
  }


// Test if we're at the end; -1 if error
FXint FXIOBuffer::eof(){
  return pointer>=(FXlong)space;
  }


// Return file size
FXlong FXIOBuffer::size(){
  return space;
  }


// Close file
FXbool FXIOBuffer::close(){
  buffer=nullptr;
  space=0L;
  pointer=0L;
  access=NoAccess;
  return true;
  }


// Destroy
FXIOBuffer::~FXIOBuffer(){
  close();
  }


}

