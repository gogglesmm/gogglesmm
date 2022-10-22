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
#ifndef FXVARIANTARRAY_H
#define FXVARIANTARRAY_H

namespace FX {


/// Array of variants
class FXAPI FXVariantArray : public FXArray<FXVariant> {
public:

  /// Construct an array
  FXVariantArray();

  /// Allocate array of n elements
  FXVariantArray(FXival n);

  /// Construct from another array
  FXVariantArray(const FXVariantArray& other);

  /// Allocate initialized with n copies of object
  FXVariantArray(const FXVariant& src,FXival n);

  /// Allocate initialized with array of n objects
  FXVariantArray(const FXVariant* src,FXival n);

  /// Destructor
 ~FXVariantArray();
  };

}

#endif
