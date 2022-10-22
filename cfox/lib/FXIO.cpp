/********************************************************************************
*                                                                               *
*                       A b s t r a c t   I / O   C l a s s                     *
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



/*
  Notes:
  - The class FXIO provides the interface to stream- or block-devices.
  - The base class implementation provides essentially a file-pointer.
  - When reading, an infinite stream of zeroes is returned to the caller; when writing
    all data are dropped but in either case the file-pointer maintains a tally of the
    number of bytes moved up to that point.
  - FXIO can thus be used to determine the length of a file without actually performing
    any i/o to the actual device; this is often convenient.
*/

// Bad handle value
#ifdef WIN32
#define BadHandle INVALID_HANDLE_VALUE
#else
#define BadHandle -1
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {



// Construct
FXIO::FXIO():pointer(0L),access(NoAccess){
  }


// Construct with given mode
FXIO::FXIO(FXuint m):pointer(0L),access(m){
  }


// Is readable
FXbool FXIO::isReadable() const {
  return ((access&ReadOnly)!=0);
  }


// Is writable
FXbool FXIO::isWritable() const {
  return ((access&WriteOnly)!=0);
  }


// Change access mode
FXbool FXIO::setMode(FXuint){
  return false;
  }


// Return true if open
FXbool FXIO::isOpen() const {
  return true;
  }


// Return true if serial access only
FXbool FXIO::isSerial() const {
  return false;
  }


// Get position
FXlong FXIO::position() const {
  return pointer;
  }


// Move to position
FXlong FXIO::position(FXlong offset,FXuint from){
  if(from==Current) offset=pointer+offset;
  if(0<=offset){
    pointer=offset;
    return pointer;
    }
  return FXIO::Error;
  }


// Read block
FXival FXIO::readBlock(void* ptr,FXival count){
  memset(ptr,0,count);
  pointer+=count;
  return count;
  }


// Write block
FXival FXIO::writeBlock(const void*,FXival count){
  pointer+=count;
  return count;
  }


// Read character
FXbool FXIO::readChar(FXchar& ch){
  return readBlock(&ch,1)==1;
  }


// Write character
FXbool FXIO::writeChar(FXchar ch){
  return writeBlock(&ch,1)==1;
  }


// Truncate file
FXlong FXIO::truncate(FXlong sz){
  if(0<=sz){
    if(pointer>=sz) pointer=sz;
    return sz;
    }
  return FXIO::Error;
  }


// Synchronize disk with cached data
FXbool FXIO::flush(){
  return true;
  }


// Test if we're at the end; -1 if error
FXint FXIO::eof(){
  return 0;
  }


// Return file size
FXlong FXIO::size(){
  return pointer;
  }


// Close file
FXbool FXIO::close(){
  access=NoAccess;
  pointer=0L;
  return true;
  }


// Destroy
FXIO::~FXIO(){
  }


}

