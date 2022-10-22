/********************************************************************************
*                                                                               *
*                          C a l l B a c k    C l a s s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2016,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXCALLBACK_H
#define FXCALLBACK_H

namespace FX {

/********************************************************************************/

// Bland template declaration
template<typename TYPE>
class FXCallback;

/********************************************************************************/

// Specialization of callback mechanism for callback with signature:
//
//      RT FUNC()
//
// Both free functions and member functions may be called.
template<typename RT>
class FXAPI FXCallback<RT ()> {
public:
  typedef RT (*METHOD)(const void*);
private:
  METHOD      method;
  const void* object;
private:

  // Stub call to method
  template <typename T,RT (T::*mfn)()>
  static RT method_call(const void* obj){ return (static_cast<T*>(const_cast<void*>(obj))->*mfn)(); }

  // Stub call to const method
  template <typename T,RT (T::*mfn)() const>
  static RT const_method_call(const void* obj){ return (static_cast<const T*>(obj)->*mfn)(); }

  // Stub call to function
  template<RT (*fn)()>
  static RT function_call(const void*){ return (fn)(); }

  // Fallback function
  static RT default_call(const void*){ return RT(); }

public:

  // Initialize to default
  FXCallback():method(&default_call),object(0){ }

  // Copy constructor
  FXCallback(const FXCallback& org):method(org.method),object(org.object){ }

  // Assignment operator
  FXCallback& operator=(const FXCallback& org){ method=org.method; object=org.object; return *this; }

  // Equality operators
  FXbool operator==(const FXCallback& other) const { return method==other.method && object==other.object; }
  FXbool operator!=(const FXCallback& other) const { return method!=other.method || object!=other.object; }

  // Invoke the function
  RT operator()() const { return (*method)(object); }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)()>
  inline void connect(TYPE* obj){ method=&method_call<TYPE,mfn>; object=obj; }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)() const>
  inline void connect(const TYPE* obj){ method=&const_method_call<TYPE,mfn>; object=obj; }

  // Connect to plain function
  template<RT (*fn)()>
  inline void connect(){ method=&function_call<fn>; object=nullptr; }

  // Return trye if connected
  bool connected() const { return (method!=&default_call); }

  // Disconnect resets to default
  void disconnect(){ method=&default_call; object=nullptr; }

  // Disconnect conditionally
  void disconnect(const void* obj){ if(obj==object){ disconnect(); } }

  // Create default callback
  static inline FXCallback create(){ return FXCallback(); }

  // Create callback to member function of object
  template<typename TYPE,RT (TYPE::* mfn)()>
  static inline FXCallback create(TYPE* obj){ return FXCallback(&method_call<TYPE,mfn>,obj); }

  // Create callback to member function of constant object
  template<typename TYPE,RT (TYPE::* mfn)() const>
  static inline FXCallback create(const TYPE* obj){ return FXCallback(&const_method_call<TYPE,mfn>,obj); }

  // Create callback to function
  template<RT (*fn)()>
  static inline FXCallback create(){ return FXCallback(&function_call<fn>,nullptr); }
  };

/********************************************************************************/

// Specialization of callback mechanism for callback with signature:
//
//      RT FUNC(PT1)
//
// Both free functions and member functions may be called.
template<typename RT,typename PT1>
class FXAPI FXCallback<RT (PT1)> {
public:
  typedef RT (*METHOD)(const void*,PT1);
private:
  METHOD      method;
  const void* object;
private:

  // Stub call to method
  template <typename T,RT (T::*mfn)(PT1)>
  static RT method_call(const void* obj,PT1 p1){ return (static_cast<T*>(const_cast<void*>(obj))->*mfn)(p1); }

  // Stub call to const method
  template <typename T,RT (T::*mfn)(PT1) const>
  static RT const_method_call(const void* obj,PT1 p1){ return (static_cast<const T*>(obj)->*mfn)(p1); }

  // Stub call to function
  template<RT (*fn)(PT1)>
  static RT function_call(const void*,PT1 p1){ return (fn)(p1); }

  // Fallback function
  static RT default_call(const void*,PT1){ return RT(); }

public:

  // Initialize to default
  FXCallback():method(&default_call),object(0){ }

  // Copy constructor
  FXCallback(const FXCallback& org):method(org.method),object(org.object){ }

  // Assignment operator
  FXCallback& operator=(const FXCallback& org){ method=org.method; object=org.object; return *this; }

  // Equality operators
  FXbool operator==(const FXCallback& other) const { return method==other.method && object==other.object; }
  FXbool operator!=(const FXCallback& other) const { return method!=other.method || object!=other.object; }

  // Invoke the function
  RT operator()(PT1 p1) const { return (*method)(object,p1); }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1)>
  inline void connect(TYPE* obj){ method=&method_call<TYPE,mfn>; object=obj; }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1) const>
  inline void connect(const TYPE* obj){ method=&const_method_call<TYPE,mfn>; object=obj; }

  // Connect to plain function
  template<RT (*fn)(PT1)>
  inline void connect(){ method=&function_call<fn>; object=nullptr; }

  // Return trye if connected
  bool connected() const { return (method!=&default_call); }

  // Disconnect resets to default
  void disconnect(){ method=&default_call; object=nullptr; }

  // Disconnect conditionally
  void disconnect(const void* obj){ if(obj==object){ disconnect(); } }

  // Create default callback
  static inline FXCallback create(){ return FXCallback(); }

  // Create callback to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1)>
  static inline FXCallback create(TYPE* obj){ return FXCallback(&method_call<TYPE,mfn>,obj); }

  // Create callback to member function of constant object
  template<typename TYPE,RT (TYPE::* mfn)(PT1) const>
  static inline FXCallback create(const TYPE* obj){ return FXCallback(&const_method_call<TYPE,mfn>,obj); }

  // Create callback to function
  template<RT (*fn)(PT1)>
  static inline FXCallback create(){ return FXCallback(&function_call<fn>,nullptr); }
  };

/********************************************************************************/

// Specialization of callback mechanism for callback with signature:
//
//      RT FUNC(PT1,PT2)
//
// Both free functions and member functions may be called.
template<typename RT,typename PT1,typename PT2>
class FXAPI FXCallback<RT (PT1,PT2)> {
public:
  typedef RT (*METHOD)(const void*,PT1,PT2);
private:
  METHOD      method;
  const void* object;
private:

  // Stub call to method
  template <typename T,RT (T::*mfn)(PT1,PT2)>
  static RT method_call(const void* obj,PT1 p1,PT2 p2){ return (static_cast<T*>(const_cast<void*>(obj))->*mfn)(p1,p2); }

  // Stub call to const method
  template <typename T,RT (T::*mfn)(PT1,PT2) const>
  static RT const_method_call(const void* obj,PT1 p1,PT2 p2){ return (static_cast<const T*>(obj)->*mfn)(p1,p2); }

  // Stub call to function
  template<RT (*fn)(PT1,PT2)>
  static RT function_call(const void*,PT1 p1,PT2 p2){ return (fn)(p1,p2); }

  // Fallback function
  static RT default_call(const void*,PT1,PT2){ return RT(); }

public:

  // Initialize to default
  FXCallback():method(&default_call),object(0){ }

  // Copy constructor
  FXCallback(const FXCallback& org):method(org.method),object(org.object){ }

  // Assignment operator
  FXCallback& operator=(const FXCallback& org){ method=org.method; object=org.object; return *this; }

  // Equality operators
  FXbool operator==(const FXCallback& other) const { return method==other.method && object==other.object; }
  FXbool operator!=(const FXCallback& other) const { return method!=other.method || object!=other.object; }

  // Invoke the function
  RT operator()(PT1 p1,PT2 p2) const { return (*method)(object,p1,p2); }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2)>
  inline void connect(TYPE* obj){ method=&method_call<TYPE,mfn>; object=obj; }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2) const>
  inline void connect(const TYPE* obj){ method=&const_method_call<TYPE,mfn>; object=obj; }

  // Connect to plain function
  template<RT (*fn)(PT1,PT2)>
  inline void connect(){ method=&function_call<fn>; object=nullptr; }

  // Return trye if connected
  bool connected() const { return (method!=&default_call); }

  // Disconnect resets to default
  void disconnect(){ method=&default_call; object=nullptr; }

  // Disconnect conditionally
  void disconnect(const void* obj){ if(obj==object){ disconnect(); } }

  // Create default callback
  static inline FXCallback create(){ return FXCallback(); }

  // Create callback to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2)>
  static inline FXCallback create(TYPE* obj){ return FXCallback(&method_call<TYPE,mfn>,obj); }

  // Create callback to member function of constant object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2) const>
  static inline FXCallback create(const TYPE* obj){ return FXCallback(&const_method_call<TYPE,mfn>,obj); }

  // Create callback to function
  template<RT (*fn)(PT1,PT2)>
  static inline FXCallback create(){ return FXCallback(&function_call<fn>,nullptr); }
  };

/********************************************************************************/

// Specialization of callback mechanism for callback with signature:
//
//      RT FUNC(PT1,PT2,PT3)
//
// Both free functions and member functions may be called.
template<typename RT,typename PT1,typename PT2,typename PT3>
class FXAPI FXCallback<RT (PT1,PT2,PT3)> {
public:
  typedef RT (*METHOD)(const void*,PT1,PT2,PT3);
private:
  METHOD      method;
  const void* object;
private:

  // Stub call to method
  template <typename T,RT (T::*mfn)(PT1,PT2,PT3)>
  static RT method_call(const void* obj,PT1 p1,PT2 p2,PT3 p3){ return (static_cast<T*>(const_cast<void*>(obj))->*mfn)(p1,p2,p3); }

  // Stub call to const method
  template <typename T,RT (T::*mfn)(PT1,PT2,PT3) const>
  static RT const_method_call(const void* obj,PT1 p1,PT2 p2,PT3 p3){ return (static_cast<const T*>(obj)->*mfn)(p1,p2,p3); }

  // Stub call to function
  template<RT (*fn)(PT1,PT2,PT3)>
  static RT function_call(const void*,PT1 p1,PT2 p2,PT3 p3){ return (fn)(p1,p2,p3); }

  // Fallback function
  static RT default_call(const void*,PT1,PT2,PT3){ return RT(); }

public:

  // Initialize to default
  FXCallback():method(&default_call),object(0){ }

  // Copy constructor
  FXCallback(const FXCallback& org):method(org.method),object(org.object){ }

  // Assignment operator
  FXCallback& operator=(const FXCallback& org){ method=org.method; object=org.object; return *this; }

  // Equality operators
  FXbool operator==(const FXCallback& other) const { return method==other.method && object==other.object; }
  FXbool operator!=(const FXCallback& other) const { return method!=other.method || object!=other.object; }

  // Invoke the function
  RT operator()(PT1 p1,PT2 p2,PT3 p3) const { return (*method)(object,p1,p2,p3); }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3)>
  inline void connect(TYPE* obj){ method=&method_call<TYPE,mfn>; object=obj; }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3) const>
  inline void connect(const TYPE* obj){ method=&const_method_call<TYPE,mfn>; object=obj; }

  // Connect to plain function
  template<RT (*fn)(PT1,PT2,PT3)>
  inline void connect(){ method=&function_call<fn>; object=nullptr; }

  // Return trye if connected
  bool connected() const { return (method!=&default_call); }

  // Disconnect resets to default
  void disconnect(){ method=&default_call; object=nullptr; }

  // Disconnect conditionally
  void disconnect(const void* obj){ if(obj==object){ disconnect(); } }

  // Create default callback
  static inline FXCallback create(){ return FXCallback(); }

  // Create callback to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3)>
  static inline FXCallback create(TYPE* obj){ return FXCallback(&method_call<TYPE,mfn>,obj); }

  // Create callback to member function of constant object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3) const>
  static inline FXCallback create(const TYPE* obj){ return FXCallback(&const_method_call<TYPE,mfn>,obj); }

  // Create callback to function
  template<RT (*fn)(PT1,PT2,PT3)>
  static inline FXCallback create(){ return FXCallback(&function_call<fn>,nullptr); }
  };

/********************************************************************************/

// Specialization of callback mechanism for callback with signature:
//
//      RT FUNC(PT1,PT2,PT3,PT4)
//
// Both free functions and member functions may be called.
template<typename RT,typename PT1,typename PT2,typename PT3,typename PT4>
class FXAPI FXCallback<RT (PT1,PT2,PT3,PT4)> {
public:
  typedef RT (*METHOD)(const void*,PT1,PT2,PT3,PT4);
private:
  METHOD      method;
  const void* object;
private:

  // Stub call to method
  template <typename T,RT (T::*mfn)(PT1,PT2,PT3,PT4)>
  static RT method_call(const void* obj,PT1 p1,PT2 p2,PT3 p3,PT4 p4){ return (static_cast<T*>(const_cast<void*>(obj))->*mfn)(p1,p2,p3,p4); }

  // Stub call to const method
  template <typename T,RT (T::*mfn)(PT1,PT2,PT3,PT4) const>
  static RT const_method_call(const void* obj,PT1 p1,PT2 p2,PT3 p3,PT4 p4){ return (static_cast<const T*>(obj)->*mfn)(p1,p2,p3,p4); }

  // Stub call to function
  template<RT (*fn)(PT1,PT2,PT3,PT4)>
  static RT function_call(const void*,PT1 p1,PT2 p2,PT3 p3,PT4 p4){ return (fn)(p1,p2,p3,p4); }

  // Fallback function
  static RT default_call(const void*,PT1,PT2,PT3,PT4){ return RT(); }

public:

  // Initialize to default
  FXCallback():method(&default_call),object(0){ }

  // Copy constructor
  FXCallback(const FXCallback& org):method(org.method),object(org.object){ }

  // Assignment operator
  FXCallback& operator=(const FXCallback& org){ method=org.method; object=org.object; return *this; }

  // Equality operators
  FXbool operator==(const FXCallback& other) const { return method==other.method && object==other.object; }
  FXbool operator!=(const FXCallback& other) const { return method!=other.method || object!=other.object; }

  // Invoke the function
  RT operator()(PT1 p1,PT2 p2,PT3 p3,PT4 p4) const { return (*method)(object,p1,p2,p3,p4); }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3,PT4)>
  inline void connect(TYPE* obj){ method=&method_call<TYPE,mfn>; object=obj; }

  // Connect to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3,PT4) const>
  inline void connect(const TYPE* obj){ method=&const_method_call<TYPE,mfn>; object=obj; }

  // Connect to plain function
  template<RT (*fn)(PT1,PT2,PT3,PT4)>
  inline void connect(){ method=&function_call<fn>; object=nullptr; }

  // Return trye if connected
  bool connected() const { return (method!=&default_call); }

  // Disconnect resets to default
  void disconnect(){ method=&default_call; object=nullptr; }

  // Disconnect conditionally
  void disconnect(const void* obj){ if(obj==object){ disconnect(); } }

  // Create default callback
  static inline FXCallback create(){ return FXCallback(); }

  // Create callback to member function of object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3,PT4)>
  static inline FXCallback create(TYPE* obj){ return FXCallback(&method_call<TYPE,mfn>,obj); }

  // Create callback to member function of constant object
  template<typename TYPE,RT (TYPE::* mfn)(PT1,PT2,PT3,PT4) const>
  static inline FXCallback create(const TYPE* obj){ return FXCallback(&const_method_call<TYPE,mfn>,obj); }

  // Create callback to function
  template<RT (*fn)(PT1,PT2,PT3,PT4)>
  static inline FXCallback create(){ return FXCallback(&function_call<fn>,nullptr); }
  };

}

#endif
