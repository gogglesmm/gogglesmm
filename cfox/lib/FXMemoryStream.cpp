/********************************************************************************
*                                                                               *
*                   M e m o r y   S t r e a m   C l a s s e s                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXStream.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXObject.h"
#include "FXStream.h"
#include "FXMemoryStream.h"


/*
  Notes:
  - Also need memory mapped file stream.
*/


using namespace FX;


/*******************************************************************************/

namespace FX {


// Create memory stream
FXMemoryStream::FXMemoryStream(const FXObject* cont):FXStream(cont){
  }


// Create and open memory stream
FXMemoryStream::FXMemoryStream(FXStreamDirection save_or_load,FXuchar* data,FXuval size,FXbool owned){
  open(save_or_load,data,size,owned);
  }


// Write at least count bytes from the buffer
FXuval FXMemoryStream::writeBuffer(FXuval count){
  if(owns){ setSpace(getSpace()+count); }
  return endptr-wrptr;
  }


// Read at least count bytes into the buffer
FXuval FXMemoryStream::readBuffer(FXuval){
  return wrptr-rdptr;
  }


// Open a stream, possibly with initial data array of certain size
FXbool FXMemoryStream::open(FXStreamDirection save_or_load,FXuchar* data,FXuval size,FXbool owned){
  if(save_or_load!=FXStreamSave && save_or_load!=FXStreamLoad){fxerror("FXMemoryStream::open: illegal stream direction.\n");}
  if(FXStream::open(save_or_load,data,size,owned)){
    if(save_or_load==FXStreamSave){
      wrptr=begptr;
      rdptr=begptr;
      }
    else{
      wrptr=endptr;
      rdptr=begptr;
      }
    return true;
    }
  return false;
  }


// Take buffer away from stream
void FXMemoryStream::takeBuffer(FXuchar*& data,FXuval& size){
  data=begptr;
  size=endptr-begptr;
  begptr=nullptr;
  wrptr=nullptr;
  rdptr=nullptr;
  endptr=nullptr;
  owns=false;
  }


// Give buffer to stream
void FXMemoryStream::giveBuffer(FXuchar *data,FXuval size){
  if(data==nullptr){ fxerror("FXMemoryStream::giveBuffer: NULL buffer argument.\n"); }
  if(owns){freeElms(begptr);}
  begptr=data;
  endptr=data+size;
  if(dir==FXStreamSave){
    wrptr=begptr;
    rdptr=begptr;
    }
  else{
    wrptr=endptr;
    rdptr=begptr;
    }
  owns=true;
  }


// Move to position; if saving and we own the buffer, try to resize
// and 0-fill the space; if loading and not out of range, move the pointer;
// otherwise, return error code.
FXbool FXMemoryStream::position(FXlong offset,FXWhence whence){
  if(dir==FXStreamDead){ fxerror("FXMemoryStream::position: stream is not open.\n"); }
  if(code==FXStreamOK){
    if(whence==FXFromCurrent) offset=offset+pos;
    else if(whence==FXFromEnd) offset=offset+(endptr-begptr);
    if(dir==FXStreamSave){
      if(begptr+offset>endptr){
        if(!owns){ setError(FXStreamFull); return false; }
        setSpace(offset);
        if(begptr+offset>endptr) return false;
        }
      wrptr=begptr+offset;
      }
    else{
      if(begptr+offset>endptr){ setError(FXStreamEnd); return false; }
      rdptr=begptr+offset;
      }
    pos=offset;
    return true;
    }
  return false;
  }


// Destructor
FXMemoryStream::~FXMemoryStream(){
  close();
  }

}
