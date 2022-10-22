/********************************************************************************
*                                                                               *
*                      A u t o m a t i c   P o i n t e r                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXAUTOPTR_H
#define FXAUTOPTR_H


namespace FX {


/// Implicitly used FXAutoPtrRef to hand FXAutoPtr through implicitly called
/// constructors and conversion operators.  Probably not used directly.
template<typename EType>
struct FXAutoPtrRef {
  EType* ptr;
  explicit FXAutoPtrRef(EType* src):ptr(src){ }
  };



/// Automatic pointer
template<typename EType>
class FXAutoPtr {
private:
  EType* ptr;
public:

  /// Construct from optional pointer
  explicit FXAutoPtr(EType* src=nullptr):ptr(src){ }

  /// Construct from another automatic pointer
  FXAutoPtr(FXAutoPtr<EType>& src):ptr(src.release()){ }

  /// Construct from FXAutoPtrRef
  FXAutoPtr(FXAutoPtrRef<EType> src):ptr(src.ptr){ }

  /// Construct from another automatic pointer of compatible type
  template <typename T> explicit FXAutoPtr(FXAutoPtr<T>& src):ptr(src.release()){ }

  /// Assign from pointer
  FXAutoPtr& operator=(EType *src){ return reset(src); }

  /// Assign from an another automatic pointer
  FXAutoPtr& operator=(FXAutoPtr<EType>& src){ return reset(src.release()); }

  /// Assign from FXAutoPtrRef
  FXAutoPtr& operator=(FXAutoPtrRef<EType> src){ if(src.ptr!=ptr){ delete ptr; ptr=src.ptr; } return *this; }

  /// Assign from an automatic pointer with compatible type
  template<typename T> FXAutoPtr& operator=(FXAutoPtr<T>& src){ return reset(src.release()); }

  /// Conversion operators
  operator EType*() const { return ptr; }

  /// Conversion to FXAutoPtr of another type T
  template<typename T> operator FXAutoPtr<T>() throw() { return FXAutoPtr<T>(this->release()); }

  /// Conversion to FXAutoPtrRef of another type T
  template<typename T> operator FXAutoPtrRef<T>() throw() { return FXAutoPtrRef<T>(this->release()); }

  /// Dereference operator
  EType& operator*() const { return *ptr; }

  /// Follow pointer operator
  EType* operator->() const { return ptr; }

  /// Array indexing
  EType& operator[](FXint i) const { return ptr[i]; }

  /// Release hold on the pointer
  EType* release(){ EType* tmp=ptr; ptr=nullptr; return tmp; }

  /// Delete old object, replace by new, if any
  FXAutoPtr& reset(EType* p=nullptr){ if(p!=ptr){ delete ptr; ptr=p; } return *this; }

  /// Destruction deletes pointer
  ~FXAutoPtr(){ delete ptr; }
  };


/// Serialize of automatic pointer
template <typename EType> FXStream& operator<<(FXStream& store,const FXAutoPtr<EType>& obj){
  EType *temp=obj; store << temp; return store;
  }


/// Deserialize of automatic pointer
template <typename EType> FXStream& operator>>(FXStream& store,FXAutoPtr<EType>& obj){
  EType *temp; store >> temp; obj=temp; return store;
  }

}

#endif

