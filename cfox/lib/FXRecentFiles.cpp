/********************************************************************************
*                                                                               *
*                     R e c e n t   F i l e s   L i s t                         *
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
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXRecentFiles.h"



/*
  Notes:
  - Use the auto-hide or auto-gray feature to hide menus which are connected
    to the FXRecentFiles class.
  - Default constructor is deprecated in applications; used only for serialization.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Up to 32 files kepts
const FXchar FXRecentFiles::key[32][7]={
  {"FILE1"}, {"FILE2"}, {"FILE3"}, {"FILE4"}, {"FILE5"}, {"FILE6"}, {"FILE7"}, {"FILE8"},
  {"FILE9"}, {"FILE10"},{"FILE11"},{"FILE12"},{"FILE13"},{"FILE14"},{"FILE15"},{"FILE16"},
  {"FILE17"},{"FILE18"},{"FILE19"},{"FILE20"},{"FILE21"},{"FILE22"},{"FILE23"},{"FILE24"},
  {"FILE25"},{"FILE26"},{"FILE27"},{"FILE28"},{"FILE29"},{"FILE30"},{"FILE31"},{"FILE32"}
  };



// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Message map
FXDEFMAP(FXRecentFiles) FXRecentFilesMap[] = {
  FXMAPFUNC(SEL_UPDATE,FXRecentFiles::ID_ANYFILES,FXRecentFiles::onUpdAnyFiles),
  FXMAPFUNC(SEL_UPDATE,FXRecentFiles::ID_CLEAR,FXRecentFiles::onUpdAnyFiles),
  FXMAPFUNC(SEL_COMMAND,FXRecentFiles::ID_CLEAR,FXRecentFiles::onCmdClear),
  FXMAPFUNCS(SEL_COMMAND,FXRecentFiles::ID_FILE_1,FXRecentFiles::ID_FILE_32,FXRecentFiles::onCmdFile),
  FXMAPFUNCS(SEL_UPDATE,FXRecentFiles::ID_FILE_1,FXRecentFiles::ID_FILE_32,FXRecentFiles::onUpdFile),
  };


// Class implementation
FXIMPLEMENT(FXRecentFiles,FXObject,FXRecentFilesMap,ARRAYNUMBER(FXRecentFilesMap))


// Serialization
FXRecentFiles::FXRecentFiles():settings(nullptr),target(nullptr),message(0),maxfiles(10){
  }


// Make new Recent Files group
FXRecentFiles::FXRecentFiles(FXApp* a,const FXString& gp,FXObject *tgt,FXSelector sel):settings(&a->reg()),target(tgt),message(sel),group(gp),maxfiles(10){
  }


// Make new Recent Files group
FXRecentFiles::FXRecentFiles(FXSettings* st,const FXString& gp,FXObject *tgt,FXSelector sel):settings(st),target(tgt),message(sel),group(gp),maxfiles(10){
  }



// Change number of files we're tracking
void FXRecentFiles::setMaxFiles(FXuint mx){
  maxfiles=FXCLAMP(1,mx,ARRAYNUMBER(key));
  }


// Obtain the filename at index
FXString FXRecentFiles::getFile(FXuint index) const {
  return settings->readStringEntry(group,key[index],FXString::null);
  }


// Change the filename at index
void FXRecentFiles::setFile(FXuint index,const FXString& filename){
  if(!filename.empty()){
    settings->writeStringEntry(group,key[index],filename.text());
    }
  }


// Append a file; its added to the top of the list, and everything else
// is moved down the list one notch; the last one is dropped from the list.
void FXRecentFiles::appendFile(const FXString& filename){
  if(!filename.empty()){
    FXString newname=filename;
    FXString oldname;
    FXuint i=0;
    FXuint j=0;
    do{
      do{ oldname=settings->readStringEntry(group,key[j++],nullptr); }while(oldname==filename);
      settings->writeStringEntry(group,key[i],newname.text());
      if(oldname.empty()) break;
      newname=oldname;
      }
    while(++i<maxfiles);
    }
  }


// Remove a file
void FXRecentFiles::removeFile(const FXString& filename){
  if(!filename.empty()){
    FXString name;
    FXuint i=0;
    FXuint j=0;
    do{
      name=settings->readStringEntry(group,key[i],nullptr);
      if(name.empty()) break;
      if(name!=filename){
        settings->writeStringEntry(group,key[j++],name.text());
        }
      }
    while(++i<maxfiles);
    settings->deleteEntry(group,key[j++]);
    }
  }


// Remove all files from the list
void FXRecentFiles::clear(){
  settings->deleteSection(group);
  }


// Clear the files list
long FXRecentFiles::onCmdClear(FXObject*,FXSelector,void*){
  clear();
  return 1;
  }


// User clicks on one of the file names
long FXRecentFiles::onCmdFile(FXObject*,FXSelector sel,void*){
  const FXchar *filename=settings->readStringEntry(group,key[FXSELID(sel)-ID_FILE_1],nullptr);
  if(filename){
    if(target){ target->handle(this,FXSEL(SEL_COMMAND,message),(void*)filename); }
    }
  return 1;
  }


// Update handler for same
long FXRecentFiles::onUpdFile(FXObject *sender,FXSelector sel,void*){
  const FXchar *filename=settings->readStringEntry(group,key[FXSELID(sel)-ID_FILE_1],nullptr);
  if(filename){
    FXint which=FXSELID(sel)-ID_FILE_1+1;
    FXString string;
    string.format("%d %s",which,filename);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETSTRINGVALUE),(void*)&string);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SHOW),nullptr);
    }
  else{
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),nullptr);
    }
  return 1;
  }


// Show or hide depending on whether there are any files
long FXRecentFiles::onUpdAnyFiles(FXObject *sender,FXSelector,void*){
  if(settings->readStringEntry(group,key[0],nullptr))
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SHOW),nullptr);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),nullptr);
  return 1;
  }


// Save data
void FXRecentFiles::save(FXStream& store) const {
  FXObject::save(store);
//  store << settings;
  store << target;
  store << message;
  store << group;
  store << maxfiles;
  }


// Load data
void FXRecentFiles::load(FXStream& store){
  FXObject::load(store);
//  store >> settings;
  store >> target;
  store >> message;
  store >> group;
  store >> maxfiles;
  }


// Destructor
FXRecentFiles::~FXRecentFiles(){
  target=(FXObject*)-1L;
  }

}
