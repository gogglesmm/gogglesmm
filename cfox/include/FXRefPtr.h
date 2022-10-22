/********************************************************************************
*                                                                               *
*         R e f e r e n c e   C o u n t e d   O b j e c t  P o i n t e r        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXREFPTR_H
#define FXREFPTR_H

namespace FX {


/// Pointer to shared object
template<typename EType>
class FXRefPtr {
private:
  EType* ptr;
public:

  /// Default constructor
  FXRefPtr():ptr(nullptr){
    }

  /// Construct and init
  FXRefPtr(EType* p):ptr(p){
    if(ptr) ptr->ref();
    }

  /// Copy constructor
  FXRefPtr(const FXRefPtr<EType>& org):ptr(org.ptr){
    if(ptr) ptr->ref();
    }

  /// Copy constructor from shared pointer of compatible type
  template<typename T> FXRefPtr(FXRefPtr<T>& org):ptr(org.ptr){
    if(ptr) ptr->ref();
    }

  /// Swap this and other, pain-free
  FXRefPtr<EType>& swap(FXRefPtr<EType>& other){
    other.ptr=atomicSet(&ptr,other.ptr);
    return *this;
    }

  /// Assignment
  FXRefPtr<EType>& operator=(EType* src){
    FXRefPtr<EType> tmp(src);
    swap(tmp);
    return *this;
    }

  /// Assignment
  FXRefPtr<EType>& operator=(const FXRefPtr<EType>& src){
    FXRefPtr<EType> tmp(src);
    swap(tmp);
    return *this;
    }

  /// Assignment from shared pointer of compatible type
  template<typename T> FXRefPtr<EType>& operator=(FXRefPtr<T>& src){
    FXRefPtr<EType> tmp(src);
    swap(tmp);
    return *this;
    }

  /// Conversion operators
  operator EType*() const { return ptr; }

  /// Dereference operator
  EType& operator*() const { return *ptr; }

  /// Follow pointer operator
  EType* operator->() const { return ptr; }

  /// Test for non-null
  operator FXbool() const { return !!ptr; }

  /// Test for NULL
  FXbool operator!() const { return !ptr; }

  /// Comparison operator.
  FXbool operator==(EType *p) const { return ptr==p; }

  /// Comparison operator.
  FXbool operator!=(EType *p) const { return ptr!=p; }

  /// Return pointer value
  EType* get() const { return ptr; }

  /// Clear pointer
  void clear(){
    FXRefPtr<EType> tmp;
    swap(tmp);
    }

  /// Destructor
  ~FXRefPtr(){
    if(ptr) ptr->unref();
    }
  };


/// Serialize of reference counted pointer
template<typename EType> FXStream& operator<<(FXStream& store,const FXRefPtr<EType>& obj){
  EType *temp=obj; store << temp; return store;
  }


/// Deserialize of reference counted pointer
template<typename EType> FXStream& operator>>(FXStream& store,FXRefPtr<EType>& obj){
  EType *temp; store >> temp; obj=temp; return store;
  }

}

#endif
