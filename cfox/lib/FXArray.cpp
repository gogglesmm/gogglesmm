/********************************************************************************
*                                                                               *
*                          G e n e r i c   A r r a y                            *
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
#include "FXElement.h"
#include "FXArray.h"

/*
  Notes:
  - FXArrayBase manages the gory details of the buffer representation: an item count
    followed by the items themselves.
  - The chosen representation allows an empty array to take up very minimal space
    only; basically, just a pointer to the memory buffer.
  - The buffer pointer is never NULL; thus its always safe to reference the buffer
    pointer.
  - Alignment is assumed to be 8 for 64-bit systems and 4 for 32-bit systems, same
    as malloc() returns.
  - Note sizeof(FXival) == sizeof(FXptr).
*/


// Special empty array value
#define EMPTY  (const_cast<void*>((const void*)(__array__empty__+1)))

using namespace FX;

/*******************************************************************************/

namespace FX {


// Empty array value
extern const FXival __array__empty__[];
const FXival __array__empty__[2]={0,0};


// Copying empty array uses same empty-array pointer regardless of
// subclass; if array is non-empty, subclass does the actual copying.
FXArrayBase::FXArrayBase():ptr(EMPTY){
  }


// Resize the array to num elements of size sz; if size becomes zero,
// substitute special empty-array pointer again which contains zero
// elements of any type.
FXbool FXArrayBase::resize(FXival num,FXival sz){
  FXival old=*(((FXival*)ptr)-1);
  if(__likely(old!=num)){
    FXptr p;
    if(0<num){
      if(ptr!=EMPTY){
        if(__unlikely((p=::realloc(((FXival*)ptr)-1,sizeof(FXival)+num*sz))==nullptr)) return false;
        }
      else{
        if(__unlikely((p=::malloc(sizeof(FXival)+num*sz))==nullptr)) return false;
        }
      ptr=((FXival*)p)+1;
      *(((FXival*)ptr)-1)=num;
      }
    else{
      if(ptr!=EMPTY){
        ::free(((FXival*)ptr)-1);
        ptr=EMPTY;
        }
      }
    }
  return true;
  }


// Destructor
FXArrayBase::~FXArrayBase(){
  resize(0,0);
  ptr=(void*)-1L;
  }

}
