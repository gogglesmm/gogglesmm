/********************************************************************************
*                                                                               *
*                              I c o n   C a c h e                              *
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
#ifndef FXICONCACHE_H
#define FXICONCACHE_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


// Forward declarations
class FXIcon;
class FXIconSource;


// Dictionary of icons
typedef FXDictionaryOf<FXIcon> FXIconDictionary;


/**
* The Icon Cache manages a collection of icons.  The icons are referenced
* by their file name.  When first encountering a new file name, the icon is
* located by searching the icon search path for the icon file.  If found, the
* services of the Icon Source object are used to load the icon from the file.
* A custom Icon Source may be installed to furnish support for additonal
* image file formats.
* Once the icon is loaded, an association between the icon name and the icon
* is entered into the Icon Dictionary.  Subsequent searches for an icon with
* this name will be satisfied from the cached value.
* The lifetype of the icons is managed by the Icon Cache, and thus all
* icons will be deleted when the Icon Cache itself is deleted.
*/
class FXAPI FXIconCache : public FXObject {
  FXDECLARE(FXIconCache)
private:
  FXApp            *app;        // Application object
  FXIconSource     *loader;     // Icon source loads the icons
  FXIconDictionary  dict;       // Map filename to icon resource
  FXString          path;       // Paths to search for icons
protected:
  FXIconCache();
private:
  FXIconCache(const FXIconCache&);
  FXIconCache &operator=(const FXIconCache&);
public:

  /// Default icon search path
  static const FXchar defaultIconPath[];

public:

  /**
  * Construct Icon Cache, and set initial search path; also
  * sets a pointer to the default icon source object.
  */
  FXIconCache(FXApp* ap,const FXString& sp=defaultIconPath);

  /// Get application
  FXApp* getApp() const { return app; }

  /// Change icon source
  void setIconSource(FXIconSource* src){ loader=src; }

  /// Return icon source
  FXIconSource* getIconSource() const { return loader; }

  /// Set the icon search paths
  void setIconPath(const FXString& p){ path=p; }

  /// Return the current icon search paths
  const FXString& getIconPath() const { return path; }

  /// Insert icon into cache, load it from file if not already cached.
  FXIcon* insert(const FXchar* name);

  /// Insert icon into cache, load it from file if not already cached.
  FXIcon* insert(const FXString& name){ return insert(name.text()); }

  /// Find icon by name, return NULL if no icon found in cache
  FXIcon* find(const FXchar* name) const { return dict.at(name); }

  /// Find icon by name, return NULL if no icon found in cache
  FXIcon* find(const FXString& name) const { return dict.at(name); }

  /// Remove icon from cache and delete it
  void remove(const FXchar* name);

  /// Remove icon from cache
  void remove(const FXString& name){ remove(name.text()); }

  /// Delete all icons
  void clear();

  /// Save object to stream
  virtual void save(FXStream& store) const;

  /// Load object from stream
  virtual void load(FXStream& store);

  /// Delete everything
  virtual ~FXIconCache();
  };


}

#endif
