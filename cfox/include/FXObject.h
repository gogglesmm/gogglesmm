/********************************************************************************
*                                                                               *
*                         T o p l e v e l   O b j e c t                         *
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
#ifndef FXOBJECT_H
#define FXOBJECT_H

#ifndef FXMETACLASS_H
#include "FXMetaClass.h"
#endif

namespace FX {


class FXStream;

/// Macro to set up class declaration
#define FXDECLARE(classname) \
  public: \
   struct FXMapEntry { FX::FXSelector keylo; FX::FXSelector keyhi; long (classname::* func)(FX::FXObject*,FX::FXSelector,void*); }; \
   static const FX::FXMetaClass metaClass; \
   static FX::FXObject* manufacture(); \
   virtual long handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr); \
   virtual const FX::FXMetaClass* getMetaClass() const { return &metaClass; } \
  private:


/// Macro to set up class implementation
#define FXIMPLEMENT(classname,baseclassname,mapping,nmappings) \
  FX::FXObject* classname::manufacture(){return new classname;} \
  const FX::FXMetaClass classname::metaClass(#classname,classname::manufacture,&baseclassname::metaClass,mapping,nmappings,sizeof(classname::FXMapEntry)); \
  long classname::handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr){ \
    const FXMapEntry* me=(const FXMapEntry*)metaClass.search(sel); \
    return me ? (this->* me->func)(sender,sel,ptr) : baseclassname::handle(sender,sel,ptr); \
    }


/// Macro to set up abstract class declaration
#define FXDECLARE_ABSTRACT(classname) \
  public: \
   struct FXMapEntry { FX::FXSelector keylo; FX::FXSelector keyhi; long (classname::* func)(FX::FXObject*,FX::FXSelector,void*); }; \
   static const FX::FXMetaClass metaClass; \
   virtual long handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr); \
   virtual const FX::FXMetaClass* getMetaClass() const { return &metaClass; } \
  private:


/// Macro to set up abstract class implementation
#define FXIMPLEMENT_ABSTRACT(classname,baseclassname,mapping,nmappings) \
  const FX::FXMetaClass classname::metaClass(#classname,FX::FXMetaClass::nullObject,&baseclassname::metaClass,mapping,nmappings,sizeof(classname::FXMapEntry)); \
  long classname::handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr){ \
    const FXMapEntry* me=(const FXMapEntry*)metaClass.search(sel); \
    return me ? (this->* me->func)(sender,sel,ptr) : baseclassname::handle(sender,sel,ptr); \
    }


/// MetaClass of a class
#define FXMETACLASS(classname) (&classname::metaClass)


/// Set up map type
#define FXDEFMAP(classname) static const classname::FXMapEntry

/// Define range of function types
#define FXMAPTYPES(typelo,typehi,func) {FXSEL(typelo,FX::MINKEY),FXSEL(typehi,FX::MAXKEY),&func}

/// Define range of function types
#define FXMAPTYPE(type,func) {FXSEL(type,FX::MINKEY),FXSEL(type,FX::MAXKEY),&func}

/// Define range of functions
#define FXMAPFUNCS(type,keylo,keyhi,func) {FXSEL(type,keylo),FXSEL(type,keyhi),&func}

/// Define one function
#define FXMAPFUNC(type,key,func) {FXSEL(type,key),FXSEL(type,key),&func}


/**
* Object is the base class for all objects in FOX; in order to receive
* messages from the user interface, your class must derive from Object.
* The Object class also provides serialization facilities, with which
* you can save and restore the object's state.  If you've subclassed
* from Object, you can save your subclasses' state by overloading the
* save() and load() functions and use the stream API to serialize its
* member data.
*/
class FXAPI FXObject {
  FXDECLARE(FXObject)
public:

  /// Get class name of some object
  const FXchar* getClassName() const;

  /// Check if object is member of metaclass
  FXbool isMemberOf(const FXMetaClass* metaclass) const;

  /// Try handle message safely, catching certain exceptions
  virtual long tryHandle(FXObject* sender,FXSelector sel,void* ptr);

  /// Called for unhandled messages
  virtual long onDefault(FXObject*,FXSelector,void*);

  /// Save object to stream
  virtual void save(FXStream& store) const;

  /// Load object from stream
  virtual void load(FXStream& store);

  /// Virtual destructor
  virtual ~FXObject();
  };

}

#endif
