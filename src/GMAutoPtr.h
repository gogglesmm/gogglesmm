/********************************************************************************
*                                                                               *
*                    A u t o m a t i c   P o i n t e r                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007-2010 by Jeroen van der Zijp.   All Rights Reserved.        *
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
*********************************************************************************
* $Id: FXAutoPtr.h,v 1.11 2007/07/09 16:02:41 fox Exp $                         *
********************************************************************************/
#ifndef GMAUTOPTR_H
#define GMAUTOPTR_H

namespace FX {

/// Automatic pointer
template<class TYPE> class FXAutoArrayPtr {
private:
  TYPE* ptr;
public:

  /// Construct with optional pointer
  FXAutoArrayPtr(TYPE* p=NULL):ptr(p){ }

  /// Copy constructor from an automatic pointer with compatible type
  template<class T> FXAutoArrayPtr(FXAutoArrayPtr<T>& orig):ptr(orig.release()){ }

  /// Assign from pointer
  FXAutoArrayPtr& operator=(TYPE *p){ ptr=p; return *this; }

  /// Assign from an automatic pointer with compatible type
  template<class T> FXAutoArrayPtr& operator=(FXAutoArrayPtr<T>& orig){ reset(orig.release()); return *this; }

  /// Conversion operators
  operator TYPE*() const { return ptr; }

  /// Dereference operator
  TYPE& operator*() const { return *ptr; }

  /// Follow pointer operator
  TYPE* operator->() const { return ptr; }

  /// Release hold on the pointer
  TYPE* release(){ TYPE* tmp=ptr; ptr=NULL; return tmp; }

  /// Delete old object, replace by new, if any
  void reset(TYPE* p=NULL){ if(p!=ptr){ delete [] ptr; ptr=p; } }

  /// Destruction deletes pointer
  ~FXAutoArrayPtr(){ delete [] ptr; }
  };
}

#endif
