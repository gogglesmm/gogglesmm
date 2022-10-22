/********************************************************************************
*                                                                               *
*                           V a r i a n t - A r r a y                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXString.h"
#include "FXElement.h"
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"


/*
  Notes:
  - FXVariantArray is an array of FXVariant's, which are data structures that
    can hold anything.
  - Adds equality and inequality to the operators inherited from FXArray.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Construct an array
FXVariantArray::FXVariantArray(){
  }

// Allocate array of n elements
FXVariantArray::FXVariantArray(FXival n):FXArray<FXVariant>(n){
  }


// Construct from another array
FXVariantArray::FXVariantArray(const FXVariantArray& other):FXArray<FXVariant>(other){
  }


// Allocate initialized with n copies of object
FXVariantArray::FXVariantArray(const FXVariant& src,FXival n):FXArray<FXVariant>(src,n){
  }


// Allocate initialized with array of n objects
FXVariantArray::FXVariantArray(const FXVariant* src,FXival n):FXArray<FXVariant>(src,n){
  }

// Destructor
FXVariantArray::~FXVariantArray(){
  }

}
