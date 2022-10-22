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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXString.h"
#include "FXSystem.h"
#include "FXProcess.h"
#include "FXPath.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "FXStringDictionary.h"
#include "FXRegistry.h"

/*
  Notes:

  - The FOX settings tree stores configuration data for FOX-based applications; it
    is organized as follows:

    SettingsRoot/foxrc                  The "foxrc" file stores common configuration
                                        settings for all FOX-based applications,
                                        such as colors and file bindings.

    SettingsRoot/Vendor/Vendorrc        The Vendorrc file is common to all applications
                                        from a particular organization, so that common
                                        configurable attributes may be given consistent
                                        values easily.

    SettingsRoot/Vendor/Applicationrc   The Applicationrc file stores configuration data
                                        unique to a specific application only.

    SettingsRoot/Applicationrc          If no Vendor name is specified, the Applicationrc
                                        file is located at the toplevel of the SettingsRoot.

  - A System-Wide SettingsRoot may be located in a read-only, system adminstrator designated
    place, to be shared by all users on a particular installation.  For example, this may store
    license-keys, references to online documentation, and other installation-time parameters.

  - A Per-User SettingsRoot is typically located in a user's home directory.
    Configurations stored in these files will be merged with the System-Wide configurations.
    Parameters given different values in the Per-User files will override those in the
    System-Wide ones.

  - Rationale:

      1)    When installing an application, simply copy "seed" registry files to
            the System-Wide SettingsRoot; having a subdirectory Vendor prevents
            clobbering other people's registry files, even if their application
            has the same name.

      2)    System-Wide registry files are, as a matter of principle, read-only.

      3)    System-Wide registry files are loaded first, and Per-User registry
            files are loaded on top of that.

      4)    Registry files loaded later will take precedence over those loaded
            earlier; i.e. key/value pairs in a later file will override a key/value
            pair with the same key loaded earlier.

      5)    The exception to the rule is that a key/value pair will not be overridden
            if the value of the key was changed since it had been loaded.

      6)    An application reads files in the order:

                System-Wide:
                        fox.rc,
                        Vendor/Vendor.rc,
                        Vendor/Application.rc

                Per-User:
                        fox.rc,
                        Vendor/Vendor.rc,
                        Vendor/Application.rc

      7)    When System-Wide settings files are loaded, entries are not marked as
            changed, and thus not written into the Per-User settings file.  Only
            values changed by the Application will be written into the Per-User file.

      8)    Settings loaded from the Per-User settings file *will* however be written
            back to the Per-User settings file, regarless whether they are changed or
            not (exception is when no single entry was changed at all!).

      9)    ONLY the Per-User, Application-specific settings files ARE WRITTEN!

     10)    Special applications, such as ControlPanel, may change other settings files,
            however.

  - Locations of registry settings files:

      o System-Wide registry files are loaded from the directory defined by user-
        specified environment variable $FOXDIR, if this variable was set.

      o System-Wide registry files are searched in SystemDirectories path-list.
        The path-list may be changed by setSystemDirectories().

      o On UNIX systems, the XDG standard is followed; this means the location
        of System-Wide settings should controlled by $XDG_CONFIG_DIRS.

      o If this environment variable isn't set, then SystemDirectories with only have
        one single directory, the default value: "/etc/xdg".

      o Directories in SystemDirectories will be searched for a subdirectory "foxrc"
        which is the root of the System-Wide settings tree for all FOX applications;
        for example: "/etc/xdg/foxrc/Desktop".

      o Per-User registry files are located from a single toplevel directory in variable
        UserDirectory.  The value of this variable may be changed by setUserDirectory().

      o On UNIX systems, the XDG standard is followed; this means the location of the
        Per-User settings should controlled by $XDG_CONFIG_HOME.   If this environment
        variable set, the root for the Per-User settings tree for all FOX applications
        is $XDG_CONFIG_HOME/fox.rc.

      o Otherwise, it will have the default value: "~/.config".


  - The Freedesktop.org XDG standard is found at:

        http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html

    Important for FOX is:

     $XDG_DATA_HOME     Per-user data files, defaults to "$HOME/.local/share".

     $XDG_CONFIG_HOME   Per-user configuration files, defaults to "$HOME/.config".

     $XDG_DATA_DIRS     Colon separated search path for per-user data files; default
                        paths are "/usr/local/share/:/usr/share/"

     $XDG_CONFIG_DIRS   Colon separated search path for per-user configuration files;
                        defaults to "/etc/xdg".
*/

#define MAXNAME   200
#define MAXVALUE  2000

// Default locations and names
#if defined(WIN32)
#define FOXRC           "fox.ini"
#define SYSTEMDIRS      "C:\\Program Files;C:\\Windows"
#define USERDIR         "%USERPROFILE%\\fox"
#define FILEEXT         ".ini"
#else
#define FOXRC           "fox.rc"
#define SYSTEMDIRS      "/etc/xdg"
#define USERDIR         "~/.config"
#define FILEEXT         ".rc"
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// File extension for settings files
const FXchar FXRegistry::ext[]=FILEEXT;


// File name of common settings file
const FXchar FXRegistry::foxrc[]=FOXRC;


// Make registry object
FXRegistry::FXRegistry(const FXString& akey,const FXString& vkey):applicationkey(akey),vendorkey(vkey),systemdirs(SYSTEMDIRS),userdir(USERDIR){
#if defined(WIN32)
  ascii=false;
#else
  ascii=true;
#endif
  }


#if defined(WIN32)

// Read from Windows Registry
FXbool FXRegistry::readFromRegistry(FXptr hroot,FXbool mrk){
  HKEY hsoftware;
  FXbool ok=false;

  FXTRACE((100,"FXRegistry::readFromRegistry(%p,%d)\n",hroot,mrk));

  // Open Software registry section
  if(RegOpenKeyExA((HKEY)hroot,"Software",0,KEY_READ,&hsoftware)==ERROR_SUCCESS){

    // Read Software\FOX
    if(readFromRegistryGroup("FOX",hsoftware,false)) ok=true;

    // Have vendor key
    if(!vendorkey.empty()){
      HKEY hvendor;

      // Open vendor registry sub-section
      if(RegOpenKeyExA(hsoftware,vendorkey.text(),0,KEY_READ,&hvendor)==ERROR_SUCCESS){

        // Read under "Software\Vendor\Vendor"
        if(readFromRegistryGroup(vendorkey,hvendor,false)) ok=true;

        // Have application key
        if(!applicationkey.empty()){

          // Read under "Software\Vendor\Application"
          if(readFromRegistryGroup(applicationkey,hvendor,mrk)) ok=true;
          }
        RegCloseKey(hvendor);
        }
      }

    // No vendor key
    else{

      // Have application key
      if(!applicationkey.empty()){

        // Read under "Software\Application"
        if(readFromRegistryGroup(applicationkey,hsoftware,mrk)) ok=true;
        }
      }
    RegCloseKey(hsoftware);
    }
  return ok;
  }


// Read from given group
FXbool FXRegistry::readFromRegistryGroup(const FXString& group,FXptr hbase,FXbool mrk){
  FXchar section[MAXNAME],name[MAXNAME],value[MAXVALUE];
  HKEY hgroup;

  // Open registry group
  if(RegOpenKeyExA((HKEY)hbase,group.text(),0,KEY_READ,&hgroup)==ERROR_SUCCESS){
    DWORD sectionsize=MAXNAME;
    DWORD sectionindex=0;
    FILETIME writetime;

    // Read sections
    while(RegEnumKeyExA(hgroup,sectionindex,section,&sectionsize,nullptr,nullptr,nullptr,&writetime)==ERROR_SUCCESS){

      // Open section
      HKEY hsection;
      if(RegOpenKeyExA(hgroup,section,0,KEY_READ,&hsection)==ERROR_SUCCESS){
        DWORD namesize=MAXNAME;
        DWORD valuesize=MAXVALUE;
        DWORD index=0;
        DWORD type;

        // Read key-value pairs
        while(RegEnumValueA(hsection,index,name,&namesize,nullptr,&type,(BYTE*)value,&valuesize)!=ERROR_NO_MORE_ITEMS){
          FXASSERT(type==REG_SZ);
          at(section).at(name,mrk)=value;
          namesize=MAXNAME;
          valuesize=MAXVALUE;
          index++;
          }

        // Close section
        RegCloseKey(hsection);
        }
      sectionsize=MAXNAME;
      sectionindex++;
      }

    // Close group
    RegCloseKey(hgroup);
    return true;
    }
  return false;
  }


// Update current user's settings
FXbool FXRegistry::writeToRegistry(FXptr hroot){
  FXbool ok=false;

  FXTRACE((100,"FXRegistry::writeToRegistry(%p)\n",hroot));

  // Have application key
  if(!applicationkey.empty()){
    HKEY hsoftware;

    // Open software registry section
    if(RegOpenKeyExA((HKEY)hroot,"Software",0,KEY_WRITE,&hsoftware)==ERROR_SUCCESS){

      // Have vendor key
      if(!vendorkey.empty()){
        HKEY hvendor;
        DWORD disp;

        // Open vendor registry sub-section
        if(RegCreateKeyExA(hsoftware,vendorkey.text(),0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,nullptr,&hvendor,&disp)==ERROR_SUCCESS){

          // Have application key
          if(!applicationkey.empty()){

            // Write under "Software\Vendor\Application"
            if(writeToRegistryGroup(applicationkey,hvendor)) ok=true;
            }

          // Done with vendor key
          RegCloseKey(hvendor);
          }
        }

      // No vendor key
      else{

        // Have application key
        if(!applicationkey.empty()){

          // Write under "Software\Application"
          if(writeToRegistryGroup(applicationkey,hsoftware)) ok=true;
          }
        }

      // Done with software key
      RegCloseKey(hsoftware);
      }
    }
  return ok;
  }


// Write to registry group
FXbool FXRegistry::writeToRegistryGroup(const FXString& group,FXptr hbase){
  FXchar section[MAXNAME];
  DWORD sectionsize,sectionindex,disp;
  HKEY hgroup,hsection;
  FXint s,e;
  FILETIME writetime;

  // Open registry group
  if(RegCreateKeyExA((HKEY)hbase,group.text(),0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,nullptr,&hgroup,&disp)==ERROR_SUCCESS){

    // First, purge all existing sections
    while(1){
      sectionindex=0;
      sectionsize=MAXNAME;
      if(RegEnumKeyExA(hgroup,sectionindex,section,&sectionsize,nullptr,nullptr,nullptr,&writetime)!=ERROR_SUCCESS) break;
      if(RegDeleteKeyA(hgroup,section)!=ERROR_SUCCESS) break;
      }

    // Write sections
    for(s=0; s<no(); ++s){

      // Section is non-empty
      if(!empty(s)){
        hsection=nullptr;

        // Write keys in this section
        for(e=0; e<data(s).no(); ++e){

          // Key is non-empty and marked
          if(!data(s).empty(e) && data(s).mark(e)){

            // Create section in registry upon finding first key in it
            if(hsection==nullptr){
              if(RegCreateKeyExA(hgroup,key(s).text(),0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,nullptr,&hsection,&disp)!=ERROR_SUCCESS) goto x;
              }

            // Write key-value pair
            if(RegSetValueExA(hsection,data(s).key(e).text(),0,REG_SZ,(BYTE*)data(s).data(e).text(),data(s).data(e).length()+1)!=ERROR_SUCCESS) break;
            }
          }

        // Close section
        if(hsection) RegCloseKey(hsection);
        }

      // Process next registry section
x:    continue;
      }

    // Close group
    RegCloseKey(hgroup);
    return true;
    }
  return false;
  }

#endif


/*******************************************************************************/

// Read registry
FXbool FXRegistry::read(){
  FXbool ok=false;
  if(ascii){
    FXString path;

    // Read system-wide settings from systemdirs
    if(!systemdirs.empty()){

      // Find common settings
      path=FXPath::search(systemdirs,FOXRC);
      if(!path.empty()){
        if(parseFile(path,false)) ok=true;
        }

      // Have vendor subdirectory
      if(!vendorkey.empty()){

        // Find vendor subdirectory
        path=FXPath::search(systemdirs,vendorkey);
        if(!path.empty()){

          // Try read vendor settings
          if(parseFile(path+PATHSEPSTRING+vendorkey+ext,false)) ok=true;

          // Try read application settings
          if(!applicationkey.empty()){
            if(parseFile(path+PATHSEPSTRING+applicationkey+ext,false)) ok=true;
            }
          }
        }

      // Have application settings only
      else if(!applicationkey.empty()){

        // Find applications settings
        path=FXPath::search(systemdirs,applicationkey+ext);

        // Try read application settings
        if(!path.empty()){
          if(parseFile(path,false)) ok=true;
          }
        }
      }

    // Read per-user settings from userdir
    if(!userdir.empty()){

      // Path to settings data
      path=FXPath::absolute(FXPath::expand(userdir));

      // Try read common settings
      if(parseFile(path+PATHSEPSTRING FOXRC,false)) ok=true;

      // Try read vendor settings
      if(!vendorkey.empty()){
        path.append(PATHSEPSTRING+vendorkey);
        if(parseFile(path+PATHSEPSTRING+vendorkey+ext,false)) ok=true;
        }

      // Try read application settings
      if(!applicationkey.empty()){
        if(parseFile(path+PATHSEPSTRING+applicationkey+ext,true)) ok=true;
        }
      }
    }
#if defined(WIN32)
  else{
    // Try read system-wide registry settings from HKEY_LOCAL_MACHINE
    if(readFromRegistry(HKEY_LOCAL_MACHINE,false)) ok=true;

    // Try read per-user registry settings from HKEY_CURRENT_USER
    if(readFromRegistry(HKEY_CURRENT_USER,true)) ok=true;
    }
#endif
  return ok;
  }

/*******************************************************************************/

// Write registry
FXbool FXRegistry::write(){
  FXbool ok=false;
  FXString path;
  if(isModified()){
    if(ascii){

      // Write per-user settings to userdir
      if(!userdir.empty()){

        // Have application key
        if(!applicationkey.empty()){

          // Path to settings data
          path=FXPath::absolute(FXPath::expand(userdir));

          // Have vendor key
          if(!vendorkey.empty()){
            path.append(PATHSEPSTRING+vendorkey);
            }

          // Ensure parent directories exist
          if(FXDir::createDirectories(path)){
            FXString realfile;
            FXString tempfile;

            // Final registry filename
            realfile=path+PATHSEPSTRING+applicationkey+ext;

            // Temporary registry filename
            tempfile=path+PATHSEPSTRING+applicationkey+"_"+FXString::value(FXProcess::current())+ext;

            // Unparse settings into temp file first
            if(unparseFile(tempfile)){

              // Rename ATOMICALLY to proper name
              if(FXFile::move(tempfile,realfile,true)){
                setModified(false);
                ok=true;
                }
              }
            }
          }
        }
      }
#if defined(WIN32)
    else{
      // Write per-user registry settings to HKEY_CURRENT_USER
      if(writeToRegistry(HKEY_CURRENT_USER)) ok=true;
      }
#endif
    }
  return ok;
  }


// Destructor
FXRegistry::~FXRegistry(){
  }

}
