/********************************************************************************
*                                                                               *
*                        F i l e   A s s o c i a t i o n s                      *
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
#include "FXMutex.h"
#include "FXStream.h"
#include "FXFile.h"
#include "FXFileStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXDictionary.h"
#include "FXDictionaryOf.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXIconCache.h"
#include "FXFileAssociations.h"


/*
  Notes:

  - FXFileAssociations needs additional fields, e.g. a print command.

  - The associate member function should be virtual so we can overload it.

  - FXFileAssociations is solely responsible for determining mime-type, and
    duplicate code in FXDirList and FXFileList is eliminated.

  - We will use two different techniques:

      - For directories, we will match "/usr/people/jeroen", then
        try "/people/jeroen", and finally, try "/jeroen" to determine
        directory bindings (Note we pass a "/" in front so we won't
        match a file binding when looking for directories.

        This means we can in many cases keep the same icon bindings
        even if some directory tree of a project is moved around in
        the file system.

      - For files, we will try to match the whole name, then try
        the extensions.

      - We will try to match "defaultdirbinding" to determine directory type,
        and "defaultfilebinding" for a file, and "defaultexecbinding", for an
        executable to allow bindings to be set for broad categories.

  - We should look into using the mime-database and content-based
    file type detection [I'm not a big fan of this, but it may sometimes
    be necessary].

  - Refer to RFC 2045, 2046, 2047, 2048, and 2077.
    The Internet media type registry is at:
    ftp://ftp.iana.org/in-notes/iana/assignments/media-types/

  - We should at least organize things so that enough info is passed in
    so we can read a fragment of the content and do it (so that means
    the full pathname, and perhaps some flags).

  - The registry format has been extended; it now is:

    command string ';' extension string ';' bigicon [ ':' bigiconopen ] ';' miniicon [ ':' miniiconopen ] ';' mimetype [ ';' flags ... ]

  - An empty binding like:

      ext=""

    Can be used to override a global binding with an empty one, i.e. it is
    as if no binding exists; you will get the default or fallback in this case.

  - Obtaining icons for files, folders, executable, etc. under Windows:

    1) Get icon sizes:
        w = GetSystemMetrics(SM_CXSMICON);
        h = GetSystemMetrics(SM_CYSMICON);

    2) Default icons for folder:

        // Get key
        RegOpenKeyEx(HKEY_CLASSES_ROOT,"folder\\DefaultIcon",0,KEY_READ,&key);

        // To get data
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // Obtain HICON extract it
        result=ExtractIconEx(filename,iconindex,arrayofbigicons,arrayofsmallicons,nicons);

    3) Default icon(s) for file:

        // Extract from shell32.dll
        ExtractIconEx("shell32.dll",0,NULL,&smallicon,1);

    4) Default exe icon(s):

        // Extract from shell32.dll
        ExtractIconExA("shell32.dll",2,NULL,&smallicon,1);

    5) Other executables:

        // Extract from executable:
        ExtractIconEx("absolutepathofexe",indexoflast,NULL,&smallicon,1);

    6) Documents:

        // Key on ".bmp" (extension with . in front)
        RegOpenKeyEx(HKEY_CLASSES_ROOT,".bmp",0,KEY_READ,&key);

        // Obtain value using:
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // Key on file type and look for default icon:
        RegOpenKeyEx(HKEY_CLASSES_ROOT,"PaintPicture\\DefaultIcon",0,KEY_READ,&keyoficon);

        // Obtain value using:
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // String contains program pathname, and icon index.  Use:
        ExtractIconEx("absolutepathofprogram",index,NULL,&smallicon,1);
*/

#define TOPIC_CONSTRUCT  1000

#define COMMANDLEN   256
#define EXTENSIONLEN 128
#define MIMETYPELEN  64
#define ICONNAMELEN  256


using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXFileAssociations,FXObject,nullptr,0)


// These registry keys are used for default bindings.
const FXchar FXFileAssociations::defaultExecBinding[]="defaultexecbinding";
const FXchar FXFileAssociations::defaultDirBinding[]="defaultdirbinding";
const FXchar FXFileAssociations::defaultFileBinding[]="defaultfilebinding";


// Construct for serialization
FXFileAssociations::FXFileAssociations():cache(nullptr),settings(nullptr){
  }



// Construct an file-extension association table
FXFileAssociations::FXFileAssociations(FXApp* app):cache(app),settings(&app->reg()){
  FXTRACE((TOPIC_CONSTRUCT,"FXFileAssociations::FXFileAssociations\n"));
  cache.setIconPath(settings->readStringEntry("SETTINGS","iconpath",FXIconCache::defaultIconPath));
  }


// Construct an file-extension association table, and alternative settings database
FXFileAssociations::FXFileAssociations(FXApp* app,FXSettings* sdb):cache(app),settings(sdb){
  FXTRACE((TOPIC_CONSTRUCT,"FXFileAssociations::FXFileAssociations\n"));
  cache.setIconPath(settings->readStringEntry("SETTINGS","iconpath",FXIconCache::defaultIconPath));
  }


// Parse binding and populate association from it
FXFileAssoc* FXFileAssociations::parse(const FXString& assoc){
  FXTRACE((200,"FXFileAssociations::parse(\"%s\")\n",assoc.text()));
  if(!assoc.empty()){
    FXFileAssoc* result=new FXFileAssoc;
    if(result){
      FXString mininameopen;
      FXString bignameopen;
      FXString mininame;
      FXString bigname;
      FXString string;
      FXuint   flags=0;

      // Parse command
      result->command=assoc.section(';',0);

      // Parse description
      result->extension=assoc.section(';',1);

      // Big icon closed and open
      string=assoc.section(';',2);
      bigname=string.section(':',0);
      bignameopen=string.section(':',1);

      // Small icon closed and open
      string=assoc.section(';',3);
      mininame=string.section(':',0);
      mininameopen=string.section(':',1);

      // Initialize icons
      result->bigicon=nullptr;
      result->miniicon=nullptr;
      result->bigiconopen=nullptr;
      result->miniiconopen=nullptr;

      // Insert icons into icon dictionary
      if(!bigname.empty()){ result->bigicon=result->bigiconopen=cache.insert(bigname); }
      if(!mininame.empty()){ result->miniicon=result->miniiconopen=cache.insert(mininame); }

      // Add open icons also; we will fall back on the regular icons in needed
      if(!bignameopen.empty()){ result->bigiconopen=cache.insert(bignameopen); }
      if(!mininameopen.empty()){ result->miniiconopen=cache.insert(mininameopen); }

      // Parse mime type
      result->mimetype=assoc.section(';',4);

      // Drag type will be set later
      result->dragtype=0;

      // Parse flags
      string=assoc.section(';',5);
      if(string.contains("c")) flags|=1;
      if(string.contains("t")) flags|=2;
      result->flags=flags;

      FXTRACE((300,"FXFileAssociations::parse: command=\"%s\" extension=\"%s\" mimetype=\"%s\" big=\"%s\" bigopen=\"%s\" mini=\"%s\" miniopen=\"%s\" flags=%d\n",result->command.text(),result->extension.text(),result->mimetype.text(),bigname.text(),bignameopen.text(),mininame.text(),mininameopen.text(),flags));

      // Return it
      return result;
      }
    }
  return nullptr;
  }


// Fetch file association
FXFileAssoc* FXFileAssociations::fetch(const FXString& ext){
  FXTRACE((200,"FXFileAssociations::fetch(\"%s\")\n",ext.text()));
  FXFileAssoc* result=bindings[ext];
  if(!result){
    result=parse(getSettings()->readStringEntry("FILETYPES",ext,nullptr));
    if(result){
      bindings[ext]=result;
      }
    }
  return result;
  }


// Find file association from registry
FXFileAssoc* FXFileAssociations::findFileBinding(const FXString& pathname){
  FXTRACE((200,"FXFileAssociations::findFileBinding(\"%s\")\n",pathname.text()));
  const FXchar* filename=pathname.text();
  FXFileAssoc* record;
  for(const FXchar* p=filename; *p; ++p){
    if(ISPATHSEP(*p)) filename=p+1;
    }
  while(*filename!='\0'){
    if((record=fetch(filename))!=nullptr) return record;
    filename=strchr(filename,'.');
    if(!filename) break;
    filename++;
    }
  return fetch(defaultFileBinding);
  }


// Find directory association from registry
FXFileAssoc* FXFileAssociations::findDirBinding(const FXString& pathname){
  FXTRACE((200,"FXFileAssociations::findDirBinding(\"%s\")\n",pathname.text()));
  const FXchar* path=pathname.text();
  FXFileAssoc* record;
  while(*path){
    if((record=fetch(path))!=nullptr) return record;
    while(ISPATHSEP(*path)) path++;             // Skip path seperators
    while(*path && !ISPATHSEP(*path)) path++;   // Skip directory name
    }
  return fetch(defaultDirBinding);
  }


// Find executable association from registry
FXFileAssoc* FXFileAssociations::findExecBinding(const FXString& pathname){
  FXTRACE((200,"FXFileAssociations::findExecBinding(\"%s\")\n",pathname.text()));
  return fetch(defaultExecBinding);
  }


// Delete all icons
void FXFileAssociations::clear(){
  for(FXival i=0; i<bindings.no(); ++i){
    delete bindings.data(i);
    }
  bindings.clear();
  cache.clear();
  }



// Save object to stream
void FXFileAssociations::save(FXStream& store) const {
  cache.save(store);
  }


// Load object from stream
void FXFileAssociations::load(FXStream& store){
  cache.load(store);
  }


// Destructor
FXFileAssociations::~FXFileAssociations(){
  FXTRACE((TOPIC_CONSTRUCT,"FXFileAssociations::~FXFileAssociations\n"));
  clear();
  settings=(FXSettings*)-1L;
  }

}
