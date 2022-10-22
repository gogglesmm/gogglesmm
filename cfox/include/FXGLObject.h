/********************************************************************************
*                                                                               *
*                           O p e n G L   O b j e c t                           *
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
#ifndef FXGLOBJECT_H
#define FXGLOBJECT_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


class FXGLViewer;
class FXGLObject;



/// Basic OpenGL object
class FXAPI FXGLObject : public FXObject {
  FXDECLARE(FXGLObject)
public:
  enum {
    ID_LAST=10000       // Leaving ample room for FXGLViewer subclasses
    };
public:

  /// Constructors
  FXGLObject(){}

  /// Copy constructor
  FXGLObject(const FXGLObject& orig):FXObject(orig){}

  /// Called by the viewer to get bounds for this object
  virtual void bounds(FXRangef& box);

  /// Draw this object in a viewer
  virtual void draw(FXGLViewer* viewer);

  /// Draw this object for hit-testing purposes
  virtual void hit(FXGLViewer* viewer);

  /// Copy this object
  virtual FXGLObject* copy();

  /// Identify sub-object given path
  virtual FXGLObject* identify(FXuint* path);

  /// Return true if this object can be dragged around
  virtual FXbool canDrag() const;

  /// Return true if this object can be deleted from the scene
  virtual FXbool canDelete() const;

  /// Drag this object from one position to another
  virtual FXbool drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty);

  /// Destructor
  virtual ~FXGLObject(){}
  };


/// List of GL objects
typedef FXObjectListOf<FXGLObject> FXGLObjectList;


/// Group object
class FXAPI FXGLGroup : public FXGLObject {
  FXDECLARE(FXGLGroup)
protected:
  FXGLObjectList list;    // List of all objects
public:

  /// Constructor
  FXGLGroup(){ }

  /// Copy constructor
  FXGLGroup(const FXGLGroup& orig):FXGLObject(orig),list(orig.list){ }

  /// Return list of childern
  FXGLObjectList& getList(){ return list; }

  /// Return bounding box
  virtual void bounds(FXRangef& box);

  /// Draw into viewer
  virtual void draw(FXGLViewer* viewer);

  /// Hit in viewer
  virtual void hit(FXGLViewer* viewer);

  /// Copy this object
  virtual FXGLObject* copy();

  /// Identify object by means of path
  virtual FXGLObject* identify(FXuint* path);

  /// Return true if group can be dragged
  virtual FXbool canDrag() const;

  /// Drag group object
  virtual FXbool drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty);

  /// Return number of children
  FXival no() const { return list.no(); }

  /// Child at position
  FXGLObject* child(FXival pos) const { return list[pos]; }

  /// Insert child object at given position
  void insert(FXival pos,FXGLObject* obj){ list.insert(pos,obj); }

  /// Insert list of child objects at given position
  void insert(FXival pos,const FXGLObjectList& objs){ list.insert(pos,objs); }

  /// Prepend child object
  void prepend(FXGLObject* obj){ list.prepend(obj); }

  /// Prepend list of child objects at given position
  void prepend(const FXGLObjectList& objs){ list.prepend(objs); }

  /// Append child object
  void append(FXGLObject* obj){ list.append(obj); }

  /// Append list of child objects at given position
  void append(const FXGLObjectList& objs){ list.append(objs); }

  /// Replace child object
  void replace(FXival pos,FXGLObject* obj){ list.replace(pos,obj); }

  /// Remove child object
  void remove(FXGLObject* obj){ list.remove(obj); }

  /// Remove child object at given position
  void erase(FXival pos){ list.erase(pos); }

  /// Remove all children
  void clear(){ list.clear(); }

  /// Stream save and load
  virtual void save(FXStream& store) const;
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXGLGroup();
  };


}

#endif

