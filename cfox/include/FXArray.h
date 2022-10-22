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
#ifndef FXARRAY_H
#define FXARRAY_H

#ifndef FXELEMENT_H
#include "FXElement.h"
#endif

namespace FX {


/// ArrayBase manages a memory buffer
class FXAPI FXArrayBase {
protected:
  FXptr ptr;
protected:
  FXArrayBase();
  FXbool resize(FXival num,FXival sz);
 ~FXArrayBase();
  };


/// Array of some generic type
template<typename EType>
class FXArray : public FXArrayBase {
public:

  /// Allocate initially empty array
  FXArray(){ }

  /// Allocate array of n elements
  FXArray(FXival n){ no(n); }

  /// Allocate array copied from another
  FXArray(const FXArray<EType>& src){
    if(no(src.no())){ copyElms(data(),src.data(),src.no()); }
    }

  /// Allocate initialized with n copies of object
  FXArray(const EType& src,FXival n){
    if(no(n)){ fillElms(data(),src,n); }
    }

  /// Allocate initialized with array of n objects
  FXArray(const EType* src,FXival n){
    if(no(n)){ copyElms(data(),src,n); }
    }

  /// Return number of items
  FXival no() const { return *(((FXival*)ptr)-1); }

  /// Change number of elements in array to n
  FXbool no(FXival n){
    FXival m=no();
    if((n-m)>0){
      if(!resize(n,sizeof(EType))) return false;
      constructElms(&at(m),n-m);
      }
    else if((m-n)>0){
      destructElms(&at(n),m-n);
      if(!resize(n,sizeof(EType))) return false;
      }
    return true;
    }

  /// Assign from another array
  FXArray<EType>& operator=(const FXArray<EType>& src){
    if(data()!=src.data() && no(src.no())){ copyElms(data(),src.data(),src.no()); }
    return *this;
    }

  /// Return pointer to array
  EType* data(){ return reinterpret_cast<EType*>(ptr); }
  const EType* data() const { return reinterpret_cast<const EType*>(ptr); }

  /// Index into array
  EType& operator[](FXival i){ return data()[i]; }
  const EType& operator[](FXival i) const { return data()[i]; }

  /// Index into array
  EType& at(FXival i){ return data()[i]; }
  const EType& at(FXival i) const { return data()[i]; }

  /// First element in array
  EType& head(){ return data()[0]; }
  const EType& head() const { return data()[0]; }

  /// Last element in array
  EType& tail(){ return data()[no()-1]; }
  const EType& tail() const { return data()[no()-1]; }

  /// Adopt array from another one; the other array becomes empty
  FXArray<EType>& adopt(FXArray<EType>& src){
    if(ptr!=src.ptr && no(0)){ swap(ptr,src.ptr); }
    return *this;
    }

  /// Assign object p to array
  FXbool assign(const EType& src){
    if(no(1)){ head()=src; return true; }
    return false;
    }

  /// Assign n copies of object to array
  FXbool assign(const EType& src,FXival n){
    if(no(n)){ fillElms(data(),src,n); return true; }
    return false;
    }

  /// Assign n objects to list
  FXbool assign(const EType* src,FXival n){
    if(no(n)){ copyElms(data(),src,n); return true; }
    return false;
    }

  /// Assign n objects to list
  FXbool assign(const FXArray<EType>& src){
    return assign(src.data(),src.no());
    }

  /// Insert an object
  FXbool insert(FXival pos,const EType& src){
    if(no(no()+1)){ moveElms(data()+pos+1,data()+pos,no()-pos-1); at(pos)=src; return true; }
    return false;
    }

  /// Insert n copies of object at specified position
  FXbool insert(FXival pos,const EType& src,FXival n){
    if(no(no()+n)){ moveElms(data()+pos+n,data()+pos,no()-pos-n); fillElms(data()+pos,src,n); return true; }
    return false;
    }

  /// Insert n objects at specified position
  FXbool insert(FXival pos,const EType* src,FXival n){
    if(no(no()+n)){ moveElms(data()+pos+n,data()+pos,no()-pos-n); copyElms(data()+pos,src,n); return true; }
    return false;
    }

  /// Insert n objects at specified position
  FXbool insert(FXival pos,const FXArray<EType>& src){
    return insert(pos,src.data(),src.no());
    }

  /// Prepend object
  FXbool prepend(const EType& src){
    if(no(no()+1)){ moveElms(data()+1,data(),no()-1); head()=src; return true; }
    return false;
    }

  /// Prepend n copies of object
  FXbool prepend(const EType& src,FXival n){
    if(no(no()+n)){ moveElms(data()+n,data(),no()-n); fillElms(data(),src,n); return true; }
    return false;
    }

  /// Prepend n objects
  FXbool prepend(const EType* src,FXival n){
    if(no(no()+n)){ moveElms(data()+n,data(),no()-n); copyElms(data(),src,n); return true; }
    return false;
    }

  /// Prepend n objects
  FXbool prepend(const FXArray<EType>& src){
    return prepend(src.data(),src.no());
    }

  /// Append object
  FXbool append(const EType& src){
    if(no(no()+1)){ tail()=src; return true; }
    return false;
    }

  /// Append n copies of object
  FXbool append(const EType& src,FXival n){
    if(no(no()+n)){ fillElms(data()+no()-n,src,n); return true; }
    return false;
    }

  /// Append n objects
  FXbool append(const EType* src,FXival n){
    if(no(no()+n)){ copyElms(data()+no()-n,src,n); return true; }
    return false;
    }

  /// Append n objects
  FXbool append(const FXArray<EType>& src){
    return append(src.data(),src.no());
    }

  /// Replace an object by other object
  FXbool replace(FXival pos,const EType& src){
    at(pos)=src;
    return true;
    }

  /// Replace the m objects at pos with n copies of other object
  FXbool replace(FXival pos,FXival m,const EType& src,FXival n){
    if(m<n){
      if(!no(no()-m+n)) return false;
      moveElms(data()+pos+n,data()+pos+m,no()-pos-n);
      }
    else if(m>n){
      moveElms(data()+pos+n,data()+pos+m,no()-pos-m);
      if(!no(no()-m+n)) return false;
      }
    fillElms(data()+pos,src,n);
    return true;
    }

  /// Replace m objects at pos by n other objects
  FXbool replace(FXival pos,FXival m,const EType* src,FXival n){
    if(m<n){
      if(!no(no()-m+n)) return false;
      moveElms(data()+pos+n,data()+pos+m,no()-pos-n);
      }
    else if(m>n){
      moveElms(data()+pos+n,data()+pos+m,no()-pos-m);
      if(!no(no()-m+n)) return false;
      }
    copyElms(data()+pos,src,n);
    return true;
    }

  /// Replace m objects at pos by other objects
  FXbool replace(FXival pos,FXival m,const FXArray<EType>& src){
    return replace(pos,m,src.data(),src.no());
    }

  /// Remove object at pos
  FXbool erase(FXival pos){
    moveElms(data()+pos,data()+pos+1,no()-pos-1);
    return no(no()-1);
    }

  /// Remove n objects starting at pos
  FXbool erase(FXival pos,FXival n){
    moveElms(data()+pos,data()+pos+n,no()-pos-n);
    return no(no()-n);
    }

  /// Push object to end
  FXbool push(const EType& src){
    if(no(no()+1)){ tail()=src; return true; }
    return false;
    }

  /// Pop object from end
  FXbool pop(){
    return no(no()-1);
    }

  /// Remove all objects
  FXbool clear(){
    return no(0);
    }

  /// Delete data
 ~FXArray(){
    clear();
    }
  };

}

#endif
