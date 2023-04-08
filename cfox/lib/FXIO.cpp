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

using namespace FX;

/*******************************************************************************/

namespace FX {


// Construct
FXIO::FXIO(){
  }


// Return true if open
FXbool FXIO::isOpen() const {
  return true;
  }


// Return true if serial access only
FXbool FXIO::isSerial() const {
  return true;
  }


// Return access mode
FXuint FXIO::mode() const {
  return FXIO::ReadWrite;
  }


// Change access mode
FXbool FXIO::mode(FXuint){
  return false;
  }


// Return permissions
FXuint FXIO::perms() const {
  return FXIO::AllFull;
  }


// Set permissions
FXbool FXIO::perms(FXuint){
  return false;
  }


// Get position
FXlong FXIO::position() const {
  return FXIO::Error;
  }


// Move to position
FXlong FXIO::position(FXlong,FXuint){
  return FXIO::Error;
  }


// Read block
FXival FXIO::readBlock(void*,FXival){
  return 0;
  }


// Write block
FXival FXIO::writeBlock(const void*,FXival count){
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
FXlong FXIO::truncate(FXlong){
  return FXIO::Error;
  }


// Synchronize disk with cached data
FXbool FXIO::flush(){
  return true;
  }


// Test if we're at the end; -1 if error
FXint FXIO::eof(){
  return 1;
  }


// Return file size
FXlong FXIO::size(){
  return 0;
  }


// Close file
FXbool FXIO::close(){
  return true;
  }


// Destroy
FXIO::~FXIO(){
  }

}
