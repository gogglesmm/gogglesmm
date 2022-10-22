/********************************************************************************
*                                                                               *
*                           R e g i s t r y   C l a s s                         *
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
#ifndef FXREGISTRY_H
#define FXREGISTRY_H

#ifndef FXSETTINGS_H
#include "FXSettings.h"
#endif

namespace FX {


/**
* The Registry maintains a persistent settings database for an application.
* The settings database is organized in two groups of three layers each.
* Configuration data shared by all users on a system is stored in the system-
* wide settings database. Configuration data for a single user is stored in
* the per-user database (typically somewhere in the user's home directory).
* Each group is further subdivided into three layers.
* The Common layer contains configuration data shared by all FOX programs.
* The Vendor layer in turn contains configuration data shared by all programs
* from a particular vendor (like a suite of interoperating applications).
* The Application layer contains configuration data for a particular application
* only.
* When an FXApp object is constructed, the application name and optional vendor
* name parameters determine the names of the Application layer and Vendor layer
* settings files (the name of Common layer settings file is pre-determined).
* Upon startup, the application's configuration data are assembled by merging
* layers from the system-wide group first, and then the layers from the per-
* user group.
* During this merge, per-user settings override system-wide settings, and
* application settings take precedence over vendor settings, which in turn
* override common settings.
* Only per-user, application-layer settings are ever written:- the other settings
* are rarely modified.  Typically, system-wide settings are set when an
* application is installed, while common per-user settings tend to be changed
* by specialized applications (e.g. ControlPanel) or through installation scripts.
* The Rregistry is automatically read when FXApp::init() is called, and written
* back to the system when FXApp::exit() is called.
*/
class FXAPI FXRegistry : public FXSettings {
protected:
  FXString applicationkey;  // Application key
  FXString vendorkey;       // Vendor key
  FXString systemdirs;      // System-wide settings directories
  FXString userdir;         // User settings directory
  FXbool   ascii;           // ASCII file-based registry
protected:
#if defined(WIN32)
  FXbool readFromRegistry(FXptr hroot,FXbool mark=false);
  FXbool writeToRegistry(FXptr hroot);
  FXbool readFromRegistryGroup(const FXString& group,FXptr hbase,FXbool mark=false);
  FXbool writeToRegistryGroup(const FXString& group,FXptr hbase);
#endif
private:
  FXRegistry(const FXRegistry&);
  FXRegistry &operator=(const FXRegistry&);
public:

  /**
  * Construct registry object; akey and vkey must be string constants.
  * Regular applications SHOULD set a vendor key!
  */
  FXRegistry(const FXString& akey=FXString::null,const FXString& vkey=FXString::null);

  /**
  * Set ASCII mode; under MS-Windows, this will switch the system to a
  * file-based registry system, instead of using the System Registry API.
  */
  void setAsciiMode(FXbool asciiMode){ ascii=asciiMode; }

  /// Get ASCII mode
  FXbool getAsciiMode() const { return ascii; }

  /// Change application key name
  void setAppKey(const FXString& name){ applicationkey=name; }

  /// Return application key name
  const FXString& getAppKey() const { return applicationkey; }

  /// Change vendor key name
  void setVendorKey(const FXString& name){ vendorkey=name; }

  /// Return vendor key name
  const FXString& getVendorKey() const { return vendorkey; }

  /// Change search path for system-wide settings
  void setSystemDirectories(const FXString& dirs){ systemdirs=dirs; }

  /// Return search path for system-wide settings
  const FXString& getSystemDirectories() const { return systemdirs; }

  /// Change directory root for per-user settings tree
  void setUserDirectory(const FXString& dir){ userdir=dir; }

  /// Return directory root of per-user settings tree
  const FXString& getUserDirectory() const { return userdir; }

  /// Read registry
  virtual FXbool read();

  /// Write registry
  virtual FXbool write();

  /// File extension for settings files
  static const FXchar ext[];

  /// File name of common settings file
  static const FXchar foxrc[];

  /// Destructor
  virtual ~FXRegistry();
  };

}

#endif
