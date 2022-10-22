/********************************************************************************
*                                                                               *
*                        F i l e    L i s t   O b j e c t                       *
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
#include "fxkeys.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXObjectList.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXSystem.h"
#include "FXPath.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "FXURL.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXGIFIcon.h"
#include "FXScrollBar.h"
#include "FXIconSource.h"
#include "FXShell.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXMenuCaption.h"
#include "FXMenuCommand.h"
#include "FXMenuCascade.h"
#include "FXMenuRadio.h"
#include "FXMenuCheck.h"
#include "FXMenuSeparator.h"
#include "FXDictionary.h"
#include "FXDictionaryOf.h"
#include "FXIconCache.h"
#include "FXFileAssociations.h"
#include "FXHeader.h"
#include "FXIconList.h"
#include "FXFileList.h"
#include "FXFileProgressDialog.h"
#include "FXMessageBox.h"
#include "icons.h"

/*
  Notes:
  - Share icons with other widgets; upgrade icons to some nicer ones.
  - Should some of these icons move to FXFileAssociations?
  - Clipboard of filenames.
  - Clipboard, DND, etc. support.
  - When being dragged over, if hovering over a directory item for some
    time we need to open it.
  - We should generate SEL_INSERTED, SEL_DELETED, SEL_CHANGED
    messages as the FXFileList updates itself from the file system.
  - The solution currently used to determine whether or not to blend the
    icon isn't so great; this class shouldn't have to know about FXPNGIcon.
  - If you land in a large directory with images, things are a tad slow;
    need to speed this up some how.
*/


#define OPENDIRDELAY        700000000   // Delay before opening directory
#define REFRESHINTERVAL     1000000000  // Interval between refreshes
#define REFRESHCOUNT        30          // Refresh every REFRESHCOUNT-th time

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXFileItem,FXIconItem,nullptr,0)


// Map
FXDEFMAP(FXFileList) FXFileListMap[]={
  FXMAPFUNC(SEL_DND_ENTER,0,FXFileList::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXFileList::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXFileList::onDNDDrop),
  FXMAPFUNC(SEL_DND_MOTION,0,FXFileList::onDNDMotion),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXFileList::onDNDRequest),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXFileList::onBeginDrag),
  FXMAPFUNC(SEL_DRAGGED,0,FXFileList::onDragged),
  FXMAPFUNC(SEL_ENDDRAG,0,FXFileList::onEndDrag),
  FXMAPFUNC(SEL_CLIPBOARD_LOST,0,FXFileList::onClipboardLost),
  FXMAPFUNC(SEL_CLIPBOARD_REQUEST,0,FXFileList::onClipboardRequest),
  FXMAPFUNC(SEL_CHORE,FXFileList::ID_PREVIEWCHORE,FXFileList::onPreviewChore),
  FXMAPFUNC(SEL_TIMEOUT,FXFileList::ID_OPENTIMER,FXFileList::onOpenTimer),
  FXMAPFUNC(SEL_TIMEOUT,FXFileList::ID_REFRESHTIMER,FXFileList::onRefreshTimer),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_DIRECTORY_UP,FXFileList::onUpdDirectoryUp),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_NAME,FXFileList::onUpdSortByName),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_TYPE,FXFileList::onUpdSortByType),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_SIZE,FXFileList::onUpdSortBySize),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_TIME,FXFileList::onUpdSortByTime),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_USER,FXFileList::onUpdSortByUser),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_GROUP,FXFileList::onUpdSortByGroup),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_REVERSE,FXFileList::onUpdSortReverse),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_CASE,FXFileList::onUpdSortCase),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SET_PATTERN,FXFileList::onUpdSetPattern),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SET_DIRECTORY,FXFileList::onUpdSetDirectory),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SHOW_HIDDEN,FXFileList::onUpdShowHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_HIDE_HIDDEN,FXFileList::onUpdHideHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_TOGGLE_HIDDEN,FXFileList::onUpdToggleHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_TOGGLE_IMAGES,FXFileList::onUpdToggleImages),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_HEADER,FXFileList::onUpdHeader),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_CUT_SEL,FXFileList::onUpdHaveSel),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_COPY_SEL,FXFileList::onUpdHaveSel),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_DELETE_SEL,FXFileList::onUpdHaveSel),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_HEADER,FXFileList::onCmdHeader),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DIRECTORY_UP,FXFileList::onCmdDirectoryUp),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_NAME,FXFileList::onCmdSortByName),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_TYPE,FXFileList::onCmdSortByType),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_SIZE,FXFileList::onCmdSortBySize),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_TIME,FXFileList::onCmdSortByTime),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_USER,FXFileList::onCmdSortByUser),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_GROUP,FXFileList::onCmdSortByGroup),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_REVERSE,FXFileList::onCmdSortReverse),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_CASE,FXFileList::onCmdSortCase),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SET_PATTERN,FXFileList::onCmdSetPattern),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SET_DIRECTORY,FXFileList::onCmdSetDirectory),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SETVALUE,FXFileList::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SETSTRINGVALUE,FXFileList::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_GETSTRINGVALUE,FXFileList::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SHOW_HIDDEN,FXFileList::onCmdShowHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_HIDE_HIDDEN,FXFileList::onCmdHideHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_TOGGLE_HIDDEN,FXFileList::onCmdToggleHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_TOGGLE_IMAGES,FXFileList::onCmdToggleImages),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_REFRESH,FXFileList::onCmdRefresh),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_CUT_SEL,FXFileList::onCmdCutSel),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_COPY_SEL,FXFileList::onCmdCopySel),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DELETE_SEL,FXFileList::onCmdDeleteSel),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_PASTE_SEL,FXFileList::onCmdPasteSel),
  FXMAPFUNC(SEL_CHORE,FXFileList::ID_DROPASK,FXFileList::onCmdDropAsk),
  FXMAPFUNC(SEL_CHORE,FXFileList::ID_DROPCOPY,FXFileList::onCmdDropCopy),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DROPCOPY,FXFileList::onCmdDropCopy),
  FXMAPFUNC(SEL_CHORE,FXFileList::ID_DROPMOVE,FXFileList::onCmdDropMove),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DROPMOVE,FXFileList::onCmdDropMove),
  FXMAPFUNC(SEL_CHORE,FXFileList::ID_DROPLINK,FXFileList::onCmdDropLink),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DROPLINK,FXFileList::onCmdDropLink),
  };


// Object implementation
FXIMPLEMENT(FXFileList,FXIconList,FXFileListMap,ARRAYNUMBER(FXFileListMap))


// For serialization
FXFileList::FXFileList(){
  dropEnable();
  associations=nullptr;
  iconloader=nullptr;
  list=nullptr;
  big_folder=nullptr;
  mini_folder=nullptr;
  big_doc=nullptr;
  mini_doc=nullptr;
  big_app=nullptr;
  mini_app=nullptr;
#ifdef WIN32
  matchmode=FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold;
  setSortFunc(ascendingCase);
#else
  matchmode=FXPath::PathName|FXPath::NoEscape;
  setSortFunc(ascending);
#endif
  dropaction=DRAG_COPY;
  matchmode=0;
  imagesize=32;
  timestamp=0;
  counter=0;
  clipcut=false;
  draggable=true;
  }


// File List
FXFileList::FXFileList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXIconList(p,tgt,sel,opts,x,y,w,h),directory(PATHSEPSTRING),pattern("*"){
  dropEnable();
  appendHeader(tr("Name\tName"),nullptr,200);
  appendHeader(tr("Type\tFile type"),nullptr,100);
  appendHeader(tr("Size\tFile size"),nullptr,60);
  appendHeader(tr("Modified Date\tDate when last modified"),nullptr,150);
  appendHeader(tr("User\tUser name"),nullptr,50);
  appendHeader(tr("Group\tGroup name"),nullptr,50);
  appendHeader(tr("Attributes\tFile attributes"),nullptr,100);
#ifndef WIN32
  appendHeader(tr("Link\tSymbolic link to"),nullptr,200);
#endif
  associations=nullptr;
  if(!(options&FILELIST_NO_OWN_ASSOC)) associations=new FXFileAssociations(getApp());
  iconloader=&FXIconSource::defaultIconSource;
  list=nullptr;
  big_folder=new FXGIFIcon(getApp(),bigfolder);
  mini_folder=new FXGIFIcon(getApp(),minifolder);
  big_doc=new FXGIFIcon(getApp(),bigdoc);
  mini_doc=new FXGIFIcon(getApp(),minidoc);
  big_app=new FXGIFIcon(getApp(),bigapp);
  mini_app=new FXGIFIcon(getApp(),miniapp);
  timeformat=tr(FXSystem::defaultTimeFormat);
  dropaction=DRAG_COPY;
#ifdef WIN32
  matchmode=FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold;
  setSortFunc(ascendingCase);
#else
  matchmode=FXPath::PathName|FXPath::NoEscape;
  setSortFunc(ascending);
#endif
  imagesize=32;
  timestamp=0;
  counter=0;
  clipcut=false;
  draggable=true;
  }


// Starts the timer
void FXFileList::create(){
  FXIconList::create();
  getApp()->addTimeout(this,ID_REFRESHTIMER,REFRESHINTERVAL);
  big_folder->create();
  mini_folder->create();
  big_doc->create();
  mini_doc->create();
  big_app->create();
  mini_app->create();
  }


// Detach disconnects the icons
void FXFileList::detach(){
  FXIconList::detach();
  getApp()->removeTimeout(this,ID_REFRESHTIMER);
  getApp()->removeTimeout(this,ID_OPENTIMER);
  big_folder->detach();
  mini_folder->detach();
  big_doc->detach();
  mini_doc->detach();
  big_app->detach();
  mini_app->detach();
  deleteType=0;
  urilistType=0;
  actionType=0;
  }


// Destroy zaps the icons
void FXFileList::destroy(){
  FXIconList::destroy();
  getApp()->removeTimeout(this,ID_REFRESHTIMER);
  getApp()->removeTimeout(this,ID_OPENTIMER);
  big_folder->destroy();
  mini_folder->destroy();
  big_doc->destroy();
  mini_doc->destroy();
  big_app->destroy();
  mini_app->destroy();
  }


// Create custom item
FXIconItem *FXFileList::createItem(const FXString& text,FXIcon *big,FXIcon* mini,void* ptr){
  return new FXFileItem(text,big,mini,ptr);
  }

/*******************************************************************************/

// Compare string and string with natural interpretation of decimal numbers
FXint FXFileList::compareSectionNatural(const FXchar* s1,const FXchar* s2,FXint s,FXbool ci){
  const FXchar *ns1,*ne1,*ns2,*ne2;
  FXint diff=0,c1=0,c2=0,d;
  for(d=s; d && *s1; d-=(*s1++=='\t')){}
  for(d=s; d && *s2; d-=(*s2++=='\t')){}
  while((c1=(FXuchar)*s1)>=' ' && (c2=(FXuchar)*s2)>=' '){

    // Both are numbers: special treatment
    if(c1<='9' && c2<='9' && '0'<=c1 && '0'<=c2){

      // Parse over leading zeroes
      for(ns1=s1; *ns1=='0'; ++ns1){ }
      for(ns2=s2; *ns2=='0'; ++ns2){ }

      // Use number of leading zeroes as tie-breaker
      if(diff==0){ diff=(ns1-s1)-(ns2-s2); }

      // Parse over numbers
      for(ne1=ns1; '0'<=*ne1 && *ne1<='9'; ++ne1){ }
      for(ne2=ns2; '0'<=*ne2 && *ne2<='9'; ++ne2){ }

      // Check length difference of the numbers
      if((d=(ne1-ns1)-(ne2-ns2))!=0){ return d; }

      // Compare the numbers
      while(ns1<ne1){
        if((d=*ns1++ - *ns2++)!=0){ return d; }
        }

      // Continue with the rest
      s1=ne1;
      s2=ne2;
      continue;
      }

    // Get lower case
    if(ci){
      c1=Ascii::toLower(c1);
      c2=Ascii::toLower(c2);
      }

    // Characters differ
    if(c1!=c2){ return c1-c2; }

    // Advance
    s1++;
    s2++;
    }

  if(c1<' ') c1=0;
  if(c2<' ') c2=0;

  // Characters differ
  if(c1!=c2){ return c1-c2; }

  // Use tie-breaker
  return diff;
  }



// Compare file names
FXint FXFileList::ascending(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSectionNatural(a->label.text(),b->label.text(),0,false);
    }
  return diff;
  }


// Reversed compare file name
FXint FXFileList::descending(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSectionNatural(b->label.text(),a->label.text(),0,false);
    }
  return diff;
  }


// Compare file names, case insensitive
FXint FXFileList::ascendingCase(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSectionNatural(a->label.text(),b->label.text(),0,true);
    }
  return diff;
  }


// Reversed compare file name, case insensitive
FXint FXFileList::descendingCase(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSectionNatural(b->label.text(),a->label.text(),0,true);
    }
  return diff;
  }


// Compare file types
FXint FXFileList::ascendingType(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(a->label.text(),b->label.text(),1);
    if(diff==0){
      diff=compareSection(a->label.text(),b->label.text(),0);
      }
    }
  return diff;
  }


// Reversed compare file type
FXint FXFileList::descendingType(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(b->label.text(),a->label.text(),1);
    if(diff==0){
      diff=compareSection(b->label.text(),a->label.text(),0);
      }
    }
  return diff;
  }


// Compare file size
FXint FXFileList::ascendingSize(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=FXSGNZ(static_cast<const FXFileItem*>(a)->size - static_cast<const FXFileItem*>(b)->size);
    if(diff==0){
      diff=compareSection(a->label.text(),b->label.text(),0);
      }
    }
  return diff;
  }


// Reversed compare file size
FXint FXFileList::descendingSize(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=FXSGNZ(static_cast<const FXFileItem*>(b)->size - static_cast<const FXFileItem*>(a)->size);
    if(diff==0){
      diff=compareSection(b->label.text(),a->label.text(),0);
      }
    }
  return diff;
  }


// Compare file time
FXint FXFileList::ascendingTime(const FXIconItem* a,const FXIconItem* b){
  FXint diff=(FXint)((const FXFileItem*)b)->isDirectory() - (FXint)((const FXFileItem*)a)->isDirectory();
  if(diff==0){
    diff=FXSGNZ(static_cast<const FXFileItem*>(a)->date - static_cast<const FXFileItem*>(b)->date);
    if(diff==0){
      diff=compareSection(a->label.text(),b->label.text(),0);
      }
    }
  return diff;
  }


// Reversed compare file time
FXint FXFileList::descendingTime(const FXIconItem* a,const FXIconItem* b){
  FXint diff=(FXint)((const FXFileItem*)b)->isDirectory() - (FXint)((const FXFileItem*)a)->isDirectory();
  if(diff==0){
    diff=FXSGNZ(static_cast<const FXFileItem*>(b)->date - static_cast<const FXFileItem*>(a)->date);
    if(diff==0){
      diff=compareSection(b->label.text(),a->label.text(),0);
      }
    }
  return diff;
  }


// Compare file user
FXint FXFileList::ascendingUser(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(a->label.text(),b->label.text(),4);
    if(diff==0){
      diff=compareSection(a->label.text(),b->label.text(),0);
      }
    }
  return diff;
  }


// Reversed compare file user
FXint FXFileList::descendingUser(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(b->label.text(),a->label.text(),4);
    if(diff==0){
      diff=compareSection(b->label.text(),a->label.text(),0);
      }
    }
  return diff;
  }


// Compare file group
FXint FXFileList::ascendingGroup(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(a->label.text(),b->label.text(),5);
    if(diff==0){
      diff=compareSection(a->label.text(),b->label.text(),0);
      }
    }
  return diff;
  }


// Reversed compare file group
FXint FXFileList::descendingGroup(const FXIconItem* a,const FXIconItem* b){
  FXint diff=static_cast<const FXFileItem*>(b)->isDirectory() - static_cast<const FXFileItem*>(a)->isDirectory();
  if(diff==0){
    diff=compareSection(b->label.text(),a->label.text(),5);
    if(diff==0){
      diff=compareSection(b->label.text(),a->label.text(),0);
      }
    }
  return diff;
  }

/*******************************************************************************/

// Select files matching wildcard pattern
FXbool FXFileList::selectMatching(const FXString& ptrn,FXuint mode,FXbool notify){
  FXbool changes=false;
  for(FXint i=0; i<getNumItems(); i++){
    if(!isItemNavigational(i) && FXPath::match(getItemFilename(i),ptrn,mode)){
      changes|=selectItem(i,notify);
      }
    }
  return changes;
  }


// Return uri-list of selected files
FXString FXFileList::getSelectedFiles() const {
  FXString result;
  for(FXint i=0; i<getNumItems(); i++){
    if(!isItemNavigational(i) && isItemSelected(i)){
      result.append(FXURL::fileToURL(getItemPathname(i)));
      result.append("\r\n");
      }
    }
  return result;
  }


// Update if we have selection
long FXFileList::onUpdHaveSel(FXObject* sender,FXSelector,void*){
  for(FXint i=0; i<getNumItems(); i++){
    if(!isItemNavigational(i) && isItemSelected(i)){
      sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
      return 1;
      }
    }
  sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }

/*******************************************************************************/

// Delete selection
long FXFileList::onCmdDeleteSel(FXObject*,FXSelector,void*){
  FXString delfiles=getSelectedFiles();
  delete_files(delfiles);       // FIXME confirmation would be nice
  return 1;
  }


// Paste clipboard
long FXFileList::onCmdPasteSel(FXObject*,FXSelector,void*){
  FXString files,action;
  if(getDNDData(FROM_CLIPBOARD,urilistType,files)){
    if(getDNDData(FROM_CLIPBOARD,actionType,action)){
      FXTRACE((100,"%s::onCmdPasteSel(): Action: %s Files: %s\n",getClassName(),action.text(),files.text()));
      if(action[0]=='1'){
        move_files(directory,files);
        }
      else{
//        FXint count=files.contains("\r\n");
//        FXTRACE((1,"number of files=%d\n",count));
//FXFileProgressDialog fileprogress(this,"Copying Files","Copying 10 files (100MB)",big_folder,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,600,0);
//fileprogress.execute();
        copy_files(directory,files);
        }
      return 1;
      }
    }
  getApp()->beep();
  return 1;
  }


// Cut
long FXFileList::onCmdCutSel(FXObject*,FXSelector,void*){
  FXDragType types[2]={urilistType,actionType};
  if(acquireClipboard(types,ARRAYNUMBER(types))){
    clipfiles=getSelectedFiles();
    clipcut=true;
    }
  return 1;
  }


// Copy
long FXFileList::onCmdCopySel(FXObject*,FXSelector,void*){
  FXDragType types[2]={urilistType,actionType};
  if(acquireClipboard(types,ARRAYNUMBER(types))){
    clipfiles=getSelectedFiles();
    clipcut=false;
    }
  return 1;
  }


// We lost the selection somehow
long FXFileList::onClipboardLost(FXObject* sender,FXSelector sel,void* ptr){
  FXIconList::onClipboardLost(sender,sel,ptr);
  FXTRACE((100,"deleting clipfiles\n"));
  clipfiles=FXString::null;
  clipcut=false;
  return 1;
  }


// Somebody wants our selection
long FXFileList::onClipboardRequest(FXObject* sender,FXSelector sel,void* ptr){

  // Try base class first
  if(FXIconList::onClipboardRequest(sender,sel,ptr)) return 1;

  // Return list of filenames as a uri-list
  if(((FXEvent*)ptr)->target==urilistType){
    FXTRACE((100,"Returning urilistType\n"));
    setDNDData(FROM_CLIPBOARD,urilistType,clipfiles);
    return 1;
    }

  // Return type of clipboard action
  if(((FXEvent*)ptr)->target==actionType){
    FXTRACE((100,"Returning actionType\n"));
    setDNDData(FROM_CLIPBOARD,actionType,clipcut?"1":"0");
    return 1;
    }

  return 0;
  }

/*******************************************************************************/


// Delete files from the systems
void FXFileList::delete_files(const FXString& files){
  FXString filename;
  FXint beg,end;
  for(beg=0; beg<files.length(); beg=end+2){
    if((end=files.find_first_of("\r\n",beg))<0) end=files.length();
    filename=FXURL::fileFromURL(files.mid(beg,end-beg));
    if(!FXFile::removeFiles(filename,true)){
      if(FXMessageBox::question(this,MBOX_OK_CANCEL,tr("Failed Deleting Files"),tr("Failed to delete file: %s; continue?"),filename.text())==MBOX_CLICKED_CANCEL) break;
      }
    }
  }


// Copy files to directory
void FXFileList::copy_files(const FXString& dir,const FXString& files){
  FXString filedst,filesrc;
  FXuint answer=0;
  FXint beg,end;
  FXbool ok;
  for(beg=0; beg<files.length(); beg=end+2){
    if((end=files.find_first_of("\r\n",beg))<0) end=files.length();
    filesrc=FXURL::fileFromURL(files.mid(beg,end-beg));
    filedst=FXPath::absolute(dir,FXPath::name(filesrc));
    ok=FXFile::copyFiles(filesrc,filedst,(answer==MBOX_CLICKED_YESALL));
    if(!ok){
      if(answer==MBOX_CLICKED_NOALL) continue;
      if(answer!=MBOX_CLICKED_YESALL){
        answer=FXMessageBox::question(this,MBOX_YES_YESALL_NO_NOALL_CANCEL,tr("Overwrite File"),tr("Overwrite existing file or directory: %s?"),filedst.text());
        if(answer==MBOX_CLICKED_CANCEL) break;
        if(answer==MBOX_CLICKED_NO) continue;
        if(answer==MBOX_CLICKED_NOALL) continue;
        ok=FXFile::copyFiles(filesrc,filedst,true);
        }
      if(!ok){
        if(FXMessageBox::question(this,MBOX_OK_CANCEL,tr("Failed Copying File"),tr("Failed to overwrite file: %s; continue?"),filedst.text())==MBOX_CLICKED_OK) continue;
        break;
        }
      }
    }
  }


// Move files to directory
void FXFileList::move_files(const FXString& dir,const FXString& files){
  FXString filedst,filesrc;
  FXuint answer=0;
  FXint beg,end;
  FXbool ok;
  for(beg=0; beg<files.length(); beg=end+2){
    if((end=files.find_first_of("\r\n",beg))<0) end=files.length();
    filesrc=FXURL::fileFromURL(files.mid(beg,end-beg));
    filedst=FXPath::absolute(dir,FXPath::name(filesrc));
    ok=FXFile::moveFiles(filesrc,filedst,(answer==MBOX_CLICKED_YESALL));
    if(!ok){
      if(answer==MBOX_CLICKED_NOALL) continue;
      if(answer!=MBOX_CLICKED_YESALL){
        answer=FXMessageBox::question(this,MBOX_YES_YESALL_NO_NOALL_CANCEL,tr("Overwrite File"),tr("Overwrite existing file or directory: %s?"),filedst.text());
        if(answer==MBOX_CLICKED_CANCEL) break;
        if(answer==MBOX_CLICKED_NO) continue;
        if(answer==MBOX_CLICKED_NOALL) continue;
        ok=FXFile::moveFiles(filesrc,filedst,true);
        }
      if(!ok){
        if(FXMessageBox::question(this,MBOX_OK_CANCEL,tr("Failed Moving File"),tr("Failed to overwrite file: %s; continue?"),filedst.text())==MBOX_CLICKED_OK) continue;
        break;
        }
      }
    }
  }

#if 0
        FXProgressDialog progress(this,tr("Copying Files"),FXString::null,PROGRESSDIALOG_NORMAL|PROGRESSDIALOG_CANCEL,0,0,520,0);
        FXuint ans=MBOX_CLICKED_NO;
        FXString filedst,filesrc;
        FXint beg,end,ok;
        progress.create();
        progress.show(PLACEMENT_CURSOR);
        progress.setTotal(dropfiles.contains("\r\n"));
        progress.setProgress(0);
        getApp()->flush();
        getApp()->runModalWhileEvents(&progress,500000000);
        getApp()->flush();
        for(beg=0; beg<files.length(); beg=end+2){
          if(progress.isCancelled()) break;
          if((end=files.find_first_of("\r\n",beg))<0) end=files.length();
          filesrc=FXURL::fileFromURL(files.mid(beg,end-beg));
          filedst=FXPath::absolute(directory,FXPath::name(filesrc));
          progress.setMessage(tr("Copying file:\n\n")+filesrc);
          progress.increment(1);
          getApp()->flush();
          getApp()->runModalWhileEvents(&progress,500000000);
          FXThread::sleep(100000000);
//          FXFile::copyFiles(filesrc,filedst,false);
/*
          ok=FXFile::copyFiles(filesrc,filedst,(ans==MBOX_CLICKED_YESALL));
          if(!ok && (ans!=MBOX_CLICKED_NOALL)){
            if(ans!=MBOX_CLICKED_YESALL && ans!=MBOX_CLICKED_NOALL){
              ans=FXMessageBox::question(this,MBOX_YES_YESALL_NO_NOALL_CANCEL,tr("Overwrite File"),tr("Overwrite existing file or directory: %s?"),filedst.text());
              if(ans==MBOX_CLICKED_CANCEL) break;
              if((ans==MBOX_CLICKED_YESALL) || (ans==MBOX_CLICKED_YES)){
                ok=FXFile::copyFiles(filesrc,filedst,true);
                }
              }
            }
*/
          }
#endif

/*******************************************************************************/

// Copy files to drop directory
long FXFileList::onCmdDropCopy(FXObject*,FXSelector,void*){
  copy_files(dropdirectory,dropfiles);
  dropdirectory=FXString::null;
  dropfiles=FXString::null;
  dropaction=DRAG_REJECT;
  return 1;
  }


// Move files to drop directory
long FXFileList::onCmdDropMove(FXObject*,FXSelector,void*){
  move_files(dropdirectory,dropfiles);
  dropdirectory=FXString::null;
  dropfiles=FXString::null;
  dropaction=DRAG_REJECT;
  return 1;
  }


// Link to files from dropdirectory
long FXFileList::onCmdDropLink(FXObject*,FXSelector,void*){
  FXString filedst,filesrc; FXint beg,end;
  for(beg=0; beg<dropfiles.length(); beg=end+2){
    if((end=dropfiles.find_first_of("\r\n",beg))<0) end=dropfiles.length();
    filesrc=FXURL::fileFromURL(dropfiles.mid(beg,end-beg));
    filedst=FXPath::absolute(dropdirectory,FXPath::name(filesrc));
    if(!FXFile::symlink(filesrc,filedst)){
      if(FXMessageBox::question(this,MBOX_OK_CANCEL,tr("Failed Linking File"),tr("Failed to make symbolic link from: %s; continue?"),filedst.text())==MBOX_CLICKED_CANCEL) break;
      }
    }
  dropdirectory=FXString::null;
  dropfiles=FXString::null;
  dropaction=DRAG_REJECT;
  return 1;
  }


// Deal with the drop that has just occurred
long FXFileList::onCmdDropAsk(FXObject*,FXSelector,void* ptr){
  FXMenuPane dropmenu(this);
  FXGIFIcon filemoveicon(getApp(),filemove);
  FXGIFIcon filecopyicon(getApp(),filecopy);
  FXGIFIcon filelinkicon(getApp(),filelink);
  FXGIFIcon filecancelicon(getApp(),filecancel);
  new FXMenuCommand(&dropmenu,tr("Move Here"),&filemoveicon,this,ID_DROPMOVE);
  new FXMenuCommand(&dropmenu,tr("Copy Here"),&filecopyicon,this,ID_DROPCOPY);
  new FXMenuCommand(&dropmenu,tr("Link Here"),&filelinkicon,this,ID_DROPLINK);
  new FXMenuSeparator(&dropmenu);
  new FXMenuCommand(&dropmenu,tr("Cancel"),&filecancelicon);
  dropmenu.create();
  dropmenu.popup(nullptr,((FXEvent*)ptr)->root_x,((FXEvent*)ptr)->root_y);
  getApp()->runModalWhileShown(&dropmenu);
  dropdirectory=FXString::null;
  dropfiles=FXString::null;
  dropaction=DRAG_REJECT;
  return 1;
  }

/*******************************************************************************/

// Change directory when hovering over a folder
// Remember current directory prior to change
long FXFileList::onOpenTimer(FXObject*,FXSelector,void*){
  FXint xx,yy,index; FXuint buttons;
  getCursorPosition(xx,yy,buttons);
  index=getItemAt(xx,yy);
  if(0<=index && isItemDirectory(index)){
    if(startdirectory.empty()){ startdirectory=getDirectory(); }
    dropdirectory=getItemPathname(index);
    setDirectory(dropdirectory,true);
    }
  return 1;
  }


// Handle drag-and-drop enter, remember current directory
long FXFileList::onDNDEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXIconList::onDNDEnter(sender,sel,ptr);
  return 1;
  }


// Handle drag-and-drop leave
// Restore current directory and scroll position prior drag
long FXFileList::onDNDLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXIconList::onDNDLeave(sender,sel,ptr);
  getApp()->removeTimeout(this,ID_OPENTIMER);
  stopAutoScroll();
  if(!startdirectory.empty()){
    setDirectory(startdirectory,true);
    startdirectory=FXString::null;
    }
  dropdirectory=FXString::null;
  dropfiles=FXString::null;
  dropaction=DRAG_REJECT;
  return 1;
  }


// Handle drag-and-drop motion
long FXFileList::onDNDMotion(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint index=-1;

  // Cancel open up timer
  getApp()->removeTimeout(this,ID_OPENTIMER);

  // Start autoscrolling
  if(startAutoScroll(event,false)) return 1;

  // Give base class a shot
  if(FXIconList::onDNDMotion(sender,sel,ptr)) return 1;

  // Dropping list of filenames
  if(offeredDNDType(FROM_DRAGNDROP,urilistType)){

    // Drop in the background
    dropdirectory=getDirectory();

    // Drop action to be performed
    dropaction=inquireDNDAction();

    // We will open up a folder if we can hover over it for a while
    index=getItemAt(event->win_x,event->win_y);
    if(0<=index && isItemDirectory(index)){

      // Start open up timer when hovering over item
      getApp()->addTimeout(this,ID_OPENTIMER,OPENDIRDELAY);

      // Directory where to drop, or directory to open up
      dropdirectory=getItemPathname(index);
      }

    // See if dropdirectory is writable
    if(FXStat::isAccessible(dropdirectory,FXIO::ReadOnly|FXIO::WriteOnly)){
      acceptDrop(DRAG_ACCEPT);
      }
    return 1;
    }
  return 0;
  }


// Handle drag-and-drop drop
long FXFileList::onDNDDrop(FXObject* sender,FXSelector sel,void* ptr){

  // Cancel open up timer
  getApp()->removeTimeout(this,ID_OPENTIMER);

  // Stop scrolling
  stopAutoScroll();

  // Restore start directory and scroll position
  if(!startdirectory.empty()){
    setDirectory(startdirectory,true);
    startdirectory=FXString::null;
    }

  // Perhaps target wants to deal with it
  if(FXIconList::onDNDDrop(sender,sel,ptr)) return 1;

  // Get list of files as uri-list
  if(getDNDData(FROM_DRAGNDROP,urilistType,dropfiles)){
    if(!dropfiles.empty()){
      switch(dropaction){
        case DRAG_COPY:
          getApp()->addChore(this,ID_DROPCOPY,ptr);
          break;
        case DRAG_MOVE:
          getApp()->addChore(this,ID_DROPMOVE,ptr);
          break;
        case DRAG_LINK:
          getApp()->addChore(this,ID_DROPLINK,ptr);
          break;
        default:
          getApp()->addChore(this,ID_DROPASK,ptr);
          break;
        }
      return 1;
      }
    }
  return 0;
  }


// Somebody wants our dragged data
long FXFileList::onDNDRequest(FXObject* sender,FXSelector sel,void* ptr){

  // Perhaps the target wants to supply its own data
  if(FXIconList::onDNDRequest(sender,sel,ptr)) return 1;

  // Return list of filenames as a uri-list
  if(((FXEvent*)ptr)->target==urilistType){
    setDNDData(FROM_DRAGNDROP,urilistType,dragfiles);
    return 1;
    }

  // Delete selected files
  if(((FXEvent*)ptr)->target==deleteType){
    FXTRACE((100,"Delete files not yet implemented\n"));
    return 1;
    }

  return 0;
  }


// Start a drag operation
long FXFileList::onBeginDrag(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXIconList::onBeginDrag(sender,sel,ptr)){
    beginDrag(&urilistType,1);
    dragfiles=getSelectedFiles();
    }
  return 1;
  }


// Dragged stuff around
long FXFileList::onDragged(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(!FXIconList::onDragged(sender,sel,ptr)){
    FXDragAction action=DRAG_ASK;
    if(event->state&CONTROLMASK) action=DRAG_COPY;
    if(event->state&SHIFTMASK) action=DRAG_MOVE;
    if(event->state&ALTMASK) action=DRAG_LINK;
    handleDrag(event->root_x,event->root_y,action);
    action=didAccept();
    switch(action){
      case DRAG_COPY:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDCOPY_CURSOR));
        break;
      case DRAG_MOVE:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDMOVE_CURSOR));
        break;
      case DRAG_LINK:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDLINK_CURSOR));
        break;
      case DRAG_ASK:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDASK_CURSOR));
        break;
      default:
        setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
        break;
      }
    }
  return 1;
  }


// End drag operation
long FXFileList::onEndDrag(FXObject* sender,FXSelector sel,void* ptr){
  if(!FXIconList::onEndDrag(sender,sel,ptr)){
    endDrag((didAccept()!=DRAG_REJECT));
    setDragCursor(getDefaultCursor());
    dragfiles=FXString::null;
    }
  return 1;
  }

/*******************************************************************************/

// Cycle through items that represent images
long FXFileList::onPreviewChore(FXObject*,FXSelector,void* ptr){
  FXint index=(FXint)(FXival)ptr;
  if(showImages() && iconloader && index<getNumItems()){
    FXIcon *icon=iconloader->loadScaledIconFile(getApp(),getItemPathname(index),imagesize);
    if(icon){
      icon->create();
      setItemBigIcon(index,icon,true);
      setItemMiniIcon(index,icon,false);
      }
   if(++index<getNumItems()){
     getApp()->addChore(this,ID_PREVIEWCHORE,(void*)(FXival)index);
     }
   }
  return 1;
  }

/*******************************************************************************/

// Set value from a message
long FXFileList::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentFile((const FXchar*)ptr,true);
  return 1;
  }


// Set current directory from dir part of filename
long FXFileList::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setCurrentFile(*((FXString*)ptr),true);
  return 1;
  }


// Get current file name (NULL if no current file)
long FXFileList::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getCurrentFile();
  return 1;
  }


// Toggle hidden files display
long FXFileList::onCmdToggleHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(!showHiddenFiles(),true);
  return 1;
  }


// Update toggle hidden files widget
long FXFileList::onUpdToggleHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Show hidden files
long FXFileList::onCmdShowHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(true,true);
  return 1;
  }


// Update show hidden files widget
long FXFileList::onUpdShowHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Hide hidden files
long FXFileList::onCmdHideHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(false,true);
  return 1;
  }


// Update hide hidden files widget
long FXFileList::onUpdHideHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),nullptr);
  return 1;
  }


// Toggle image preview
long FXFileList::onCmdToggleImages(FXObject*,FXSelector,void*){
  showImages(!showImages(),true);
  return 1;
  }


// Update image preview
long FXFileList::onUpdToggleImages(FXObject* sender,FXSelector,void*){
  sender->handle(this,showImages()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Move up one level
long FXFileList::onCmdDirectoryUp(FXObject*,FXSelector,void*){
  setDirectory(FXPath::upLevel(directory),true);
  return 1;
  }


// Determine if we can still go up more
long FXFileList::onUpdDirectoryUp(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXPath::isTopDirectory(directory)?FXSEL(SEL_COMMAND,ID_DISABLE):FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  return 1;
  }


// Change pattern
long FXFileList::onCmdSetPattern(FXObject*,FXSelector,void* ptr){
  setPattern((const char*)ptr,true);
  return 1;
  }


// Update pattern
long FXFileList::onUpdSetPattern(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),(void*)pattern.text());
  return 1;
  }


// Change directory
long FXFileList::onCmdSetDirectory(FXObject*,FXSelector,void* ptr){
  setDirectory((const char*)ptr,true);
  return 1;
  }


// Update directory
long FXFileList::onUpdSetDirectory(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),(void*)directory.text());
  return 1;
  }


// Sort by name
long FXFileList::onCmdSortByName(FXObject*,FXSelector,void*){
#ifdef WIN32
  if(getSortFunc()==ascending) setSortFunc(descending);
  else if(getSortFunc()==ascendingCase) setSortFunc(descendingCase);
  else if(getSortFunc()==descending) setSortFunc(ascending);
  else setSortFunc(ascendingCase);
#else
  if(getSortFunc()==ascending) setSortFunc(descending);
  else if(getSortFunc()==ascendingCase) setSortFunc(descendingCase);
  else if(getSortFunc()==descending) setSortFunc(ascending);
  else setSortFunc(ascending);
#endif
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByName(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascending || getSortFunc()==descending || getSortFunc()==ascendingCase || getSortFunc()==descendingCase) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Sort by type
long FXFileList::onCmdSortByType(FXObject*,FXSelector,void*){
  setSortFunc((getSortFunc()==ascendingType) ? descendingType : ascendingType);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByType(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingType || getSortFunc()==descendingType) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Sort by size
long FXFileList::onCmdSortBySize(FXObject*,FXSelector,void*){
  setSortFunc((getSortFunc()==ascendingSize) ? descendingSize : ascendingSize);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortBySize(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingSize || getSortFunc()==descendingSize) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Sort by time
long FXFileList::onCmdSortByTime(FXObject*,FXSelector,void*){
  setSortFunc((getSortFunc()==ascendingTime) ? descendingTime : ascendingTime);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByTime(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingTime || getSortFunc()==descendingTime) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Sort by user
long FXFileList::onCmdSortByUser(FXObject*,FXSelector,void*){
  setSortFunc((getSortFunc()==ascendingUser) ? descendingUser : ascendingUser);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByUser(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingUser || getSortFunc()==descendingUser) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Sort by group
long FXFileList::onCmdSortByGroup(FXObject*,FXSelector,void*){
  setSortFunc((getSortFunc()==ascendingGroup) ? descendingGroup : ascendingGroup);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByGroup(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingGroup || getSortFunc()==descendingGroup) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Reverse sort order
long FXFileList::onCmdSortReverse(FXObject*,FXSelector,void*){
  if(getSortFunc()==ascending) setSortFunc(descending);
  else if(getSortFunc()==descending) setSortFunc(ascending);
  else if(getSortFunc()==ascendingCase) setSortFunc(descendingCase);
  else if(getSortFunc()==descendingCase) setSortFunc(ascendingCase);
  else if(getSortFunc()==ascendingType) setSortFunc(descendingType);
  else if(getSortFunc()==descendingType) setSortFunc(ascendingType);
  else if(getSortFunc()==ascendingSize) setSortFunc(descendingSize);
  else if(getSortFunc()==descendingSize) setSortFunc(ascendingSize);
  else if(getSortFunc()==ascendingTime) setSortFunc(descendingTime);
  else if(getSortFunc()==descendingTime) setSortFunc(ascendingTime);
  else if(getSortFunc()==ascendingUser) setSortFunc(descendingUser);
  else if(getSortFunc()==descendingUser) setSortFunc(ascendingUser);
  else if(getSortFunc()==ascendingGroup) setSortFunc(descendingGroup);
  else if(getSortFunc()==descendingGroup) setSortFunc(ascendingGroup);
  sortItems();
  return 1;
  }


// Update sender
long FXFileList::onUpdSortReverse(FXObject* sender,FXSelector,void*){
  FXSelector selector=FXSEL(SEL_COMMAND,ID_UNCHECK);
  if(getSortFunc()==descending) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingCase) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingType) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingSize) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingTime) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingUser) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(getSortFunc()==descendingGroup) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  sender->handle(this,selector,nullptr);
  return 1;
  }


// Toggle case sensitivity
long FXFileList::onCmdSortCase(FXObject*,FXSelector,void*){
  if(getSortFunc()==ascending) setSortFunc(ascendingCase);
  else if(getSortFunc()==ascendingCase) setSortFunc(ascending);
  else if(getSortFunc()==descending) setSortFunc(descendingCase);
  else if(getSortFunc()==descendingCase) setSortFunc(descending);
  sortItems();
  return 1;
  }


// Check if case sensitive
long FXFileList::onUpdSortCase(FXObject* sender,FXSelector,void*){
  sender->handle(this,(getSortFunc()==ascendingCase || getSortFunc()==descendingCase) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  sender->handle(this,(getSortFunc()==ascendingCase || getSortFunc()==ascending || getSortFunc()==descendingCase || getSortFunc()==descending) ? FXSEL(SEL_COMMAND,ID_ENABLE) : FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Clicked header button
long FXFileList::onCmdHeader(FXObject*,FXSelector,void* ptr){
  if(((FXuint)(FXuval)ptr)<6) handle(this,FXSEL(SEL_COMMAND,(ID_SORT_BY_NAME+(FXuint)(FXuval)ptr)),nullptr);
  return 1;
  }


// Clicked header button
long FXFileList::onUpdHeader(FXObject*,FXSelector,void*){
  header->setArrowDir(0,(getSortFunc()==ascending || getSortFunc()==ascendingCase)  ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descending || getSortFunc()==descendingCase) ? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // Name
  header->setArrowDir(1,(getSortFunc()==ascendingType)  ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descendingType) ? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // Type
  header->setArrowDir(2,(getSortFunc()==ascendingSize)  ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descendingSize) ? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // Size
  header->setArrowDir(3,(getSortFunc()==ascendingTime)  ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descendingTime) ? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // Date
  header->setArrowDir(4,(getSortFunc()==ascendingUser)  ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descendingUser) ? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // User
  header->setArrowDir(5,(getSortFunc()==ascendingGroup) ? FXHeaderItem::ARROW_DOWN : (getSortFunc()==descendingGroup)? FXHeaderItem::ARROW_UP : FXHeaderItem::ARROW_NONE);   // Group
  return 1;
  }

/*******************************************************************************/

// Periodically check to see if directory was changed, and update the list if it was.
long FXFileList::onRefreshTimer(FXObject*,FXSelector,void*){
  if(flags&FLAG_UPDATE){
    counter+=1;
    if(!listItems((counter==REFRESHCOUNT),true)){
      setDirectory(FXPath::validPath(directory),true);
      }
    }
  getApp()->addTimeout(this,ID_REFRESHTIMER,REFRESHINTERVAL);
  return 0;
  }


// Force an immediate update of the list
long FXFileList::onCmdRefresh(FXObject*,FXSelector,void*){
  listItems(true,true);
  return 1;
  }


// Compare till '\t' or '\0'
static FXbool fileequal(const FXchar* p1,const FXchar* p2){
  FXint c1,c2;
  do{
    c1=*p1++;
    c2=*p2++;
    }
  while((c1==c2) && ('\t'<c1));
  return (c1<='\t') && (c2<='\t');
  }


// List the items in the directory.
// Regenerate the list if an update is forced or the directory timestamp was changed.
// Add, remove, or update items as needed, generating the proper callbacks.
// In addition, re-sort the items using current sort-function.
// Return false if the directory can not be accessed, true otherwise.
FXbool FXFileList::listItems(FXbool force,FXbool notify){
  FXStat info;

  FXTRACE((100,"%s::listItems(%d,%d)\n",getClassName(),force,notify));

  // See if directory still there
  if(FXStat::statFile(directory,info)){

    // Last modified time of current directory
    FXTime time=info.modified();

    // Regenerate list if update forced or modified time changed
    if(force || time!=timestamp){
      FXFileItem  *oldlist=list;    // Old insert-order list
      FXFileItem  *newlist=nullptr; // New insert-order list
      FXFileItem **po=&oldlist;     // Head of old list
      FXFileItem **pn=&newlist;     // Head of new list
      FXFileItem  *olditem;
      FXFileItem  *newitem;
      FXFileItem  *link;
      FXString     pathname;
      FXString     extension;
      FXString     label;
      FXString     name;
      FXString     grpid;
      FXString     usrid;
      FXString     attrs;
      FXString     modtm;
      FXString     lnknm;
      FXuint       mode;
      FXbool       istop;
      FXDir        dir;

      // Get directory stream pointer
      if(dir.open(directory)){

        // Are we at the top directory?
        istop=FXPath::isTopDirectory(directory);

        // Loop over directory entries
        while(dir.next(name)){

          // Suppress '.' if not showing navigational items or not at top directory
          if(name[0]=='.'){
            if(name[1]=='\0'){
              if(!istop || (options&FILELIST_NO_PARENT)) continue;
              }
            else if(name[1]=='.' && name[2]=='\0'){
              if(istop || (options&FILELIST_NO_PARENT)) continue;
              }
            else{
              if(!(options&FILELIST_SHOWHIDDEN)) continue;
              }
            }

          // Build full pathname
          pathname=directory;
          if(!ISPATHSEP(pathname.tail())) pathname+=PATHSEPSTRING;
          pathname+=name;

#ifdef WIN32

          // Get file/link info
          if(!FXStat::statFile(pathname,info)) continue;

          mode=info.mode();

          // Suppress hidden files or directories
          if((mode&FXIO::Hidden) && !(options&FILELIST_SHOWHIDDEN)) continue;
#else

          // Get file/link info
          if(!FXStat::statLink(pathname,info)) continue;

          mode=info.mode();

          // If its a link, get file mode from target
          if(info.isLink()){
            mode=FXStat::mode(pathname) | FXIO::SymLink;
            }

#endif

          // Skip item if it is a directory and we want only files, if it is a file and we want only directories,
          // or if it is a file and it fails to match the wildcard pattern.
          if(mode&FXIO::Directory){
            if(options&FILELIST_SHOWFILES) continue;
            }
          else{
            if(options&FILELIST_SHOWDIRS) continue;
            if(!FXPath::match(name,pattern,matchmode)) continue;
            }

          // Search for item in old list, unlink from old if found
          for(FXFileItem** pp=po; (olditem=*pp)!=nullptr; pp=&olditem->link){
            if(fileequal(olditem->label.text(),name.text())){
              *pp=olditem->link; olditem->link=nullptr;
              break;
              }
            }

          // Use a new item if forced, if there was no old item, or if the item information was changed
          if(force || !olditem || olditem->getDate()!=info.modified() || olditem->getSize()!=info.size() || olditem->getMode()!=mode){

            // Make new item
            newitem=(FXFileItem*)createItem(FXString::null,nullptr,nullptr,nullptr);

            // Obtain user name
            usrid=FXSystem::userName(info.user());

            // Obtain group name
            grpid=FXSystem::groupName(info.group());

            // Permissions
            attrs=FXSystem::modeString(mode);

            // Mod time
            modtm=FXSystem::localTime(info.modified(),timeformat.text());

            // Link name, if any
            lnknm=FXString::null;
            if(info.isLink()) lnknm=FXFile::symlink(pathname);

            // Update item information
            newitem->setDraggable(draggable);
            newitem->setSize(info.size());
            newitem->setDate(info.modified());
            newitem->setMode(mode);
            newitem->setAssoc(nullptr);

            // Determine icons and type
            if(newitem->isDirectory()){
              extension=tr("Folder");
              newitem->setBigIcon(big_folder);
              newitem->setMiniIcon(mini_folder);
              if(associations) newitem->setAssoc(associations->findDirBinding(pathname));
              }
            else if(newitem->isExecutable()){
              extension=tr("Application");
              newitem->setBigIcon(big_app);
              newitem->setMiniIcon(mini_app);
              if(associations) newitem->setAssoc(associations->findExecBinding(pathname));
              }
            else{
              extension=tr("Document");
              newitem->setBigIcon(big_doc);
              newitem->setMiniIcon(mini_doc);
              if(associations) newitem->setAssoc(associations->findFileBinding(pathname));
              }

            // If association is found, use it
            if(newitem->getAssoc()){
              extension=newitem->getAssoc()->extension;
              if(newitem->getAssoc()->bigicon) newitem->setBigIcon(newitem->getAssoc()->bigicon);
              if(newitem->getAssoc()->miniicon) newitem->setMiniIcon(newitem->getAssoc()->miniicon);
              }

            // Update item information
            label.format("%s\t%s\t%'lld\t%s\t%s\t%s\t%s\t%s",name.text(),extension.text(),newitem->size,modtm.text(),usrid.text(),grpid.text(),attrs.text(),lnknm.text());

            // New label
            newitem->setText(label);

            // Create item
            if(id()) newitem->create();

            // Replace or add item
            if(olditem){
              setItem(items.find(olditem),newitem,notify);
              }
            else{
              appendItem(newitem,notify);
              }
            *pn=newitem; pn=&newitem->link;
            }

          // Keep old item if nothing changed
          else{
            *pn=olditem; pn=&olditem->link;
            }
          }

        // Show thumbnails
        if(showImages()){
          getApp()->addChore(this,ID_PREVIEWCHORE,(void*)(FXival)0);
          }

        // Close directory
        dir.close();
        }

      // Wipe items remaining in list:- they have disappeared!!
      for(olditem=oldlist; olditem; olditem=link){
        link=olditem->link;
        removeItem(items.find(olditem),notify);
        }

      // Remember new list
      list=newlist;

      // Update sort order
      sortItems();

      // Update timestamp
      timestamp=time;

      // Reset counter
      counter=0;
      }
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Set current file; return true if success
FXbool FXFileList::setCurrentFile(const FXString& file,FXbool notify){
  FXTRACE((100,"%s::setCurrentFile(%s)\n",getClassName(),file.text()));
  if(setDirectory(FXPath::directory(file),notify)){
    FXint index=findItem(FXPath::name(file));
    if(0<=index){
      makeItemVisible(index);
      setAnchorItem(index);
      setCurrentItem(index,notify);
      selectItem(index,notify);
      return true;
      }
    }
  return false;
  }


// Get pathname to current file, if any
FXString FXFileList::getCurrentFile() const {
  if(0<=getCurrentItem()){
    return getItemPathname(getCurrentItem());
    }
  return FXString::null;
  }


// Set current directory; return true if success
FXbool FXFileList::setDirectory(const FXString& pathname,FXbool notify){
  FXTRACE((100,"%s::setDirectory(%s)\n",getClassName(),pathname.text()));
  FXString path(FXPath::absolute(directory,pathname));
  if(FXStat::isDirectory(path)){
    if(directory==path) return true;
    clearItems(notify);
    directory=path;
    list=nullptr;
    if(listItems(true,notify)){
      if(getNumItems()){
        makeItemVisible(0);
        setAnchorItem(0);
        setCurrentItem(0,notify);
        }
      return true;
      }
    }
  return false;
  }

/*******************************************************************************/

// Get file name from item
FXString FXFileList::getItemFilename(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemFilename: index out of range.\n",getClassName()); }
  return items[index]->label.section('\t',0);
  }


// Get full pathname to item
FXString FXFileList::getItemPathname(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemPathname: index out of range.\n",getClassName()); }
  return FXPath::absolute(directory,items[index]->label.section('\t',0));
  }


// Is file
FXbool FXFileList::isItemFile(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::isItemFile: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isFile();
  }


// Is directory
FXbool FXFileList::isItemDirectory(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::isItemDirectory: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isDirectory();
  }


// Is executable
FXbool FXFileList::isItemExecutable(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::isItemExecutable: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isExecutable();
  }


// Return true if this is a symbolic link item
FXbool FXFileList::isItemSymlink(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::isItemSymlink: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isSymlink();
  }


// Return true if item is navigational item like '.' or '..'
FXbool FXFileList::isItemNavigational(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::isItemNavigational: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isNavigational();
  }


// Get associations (if any) from the file
FXFileAssoc* FXFileList::getItemAssoc(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemAssoc: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->getAssoc();
  }


// Return the file size for this item
FXlong FXFileList::getItemSize(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemSize: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->getSize();
  }


// Return the date for this item, in nanoseconds
FXTime FXFileList::getItemDate(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemDate: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->getDate();
  }


// Return the mode bits for this item
FXuint FXFileList::getItemMode(FXint index) const {
  if(index<0 || getNumItems()<=index){ fxerror("%s::getItemMode: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->getMode();
  }

/*******************************************************************************/

// Set the pattern to filter
void FXFileList::setPattern(const FXString& ptrn,FXbool notify){
  if(!ptrn.empty() && pattern!=ptrn){
    pattern=ptrn;
    listItems(true,notify);
    }
  }


// Change file match mode
void FXFileList::setMatchMode(FXuint mode,FXbool notify){
  if(matchmode!=mode){
    matchmode=mode;
    listItems(true,notify);
    }
  }


// Change show hidden files mode
void FXFileList::showHiddenFiles(FXbool flag,FXbool notify){
  FXuint opts=((-(FXint)flag^options)&FILELIST_SHOWHIDDEN)^options;
  if(opts!=options){
    options=opts;
    listItems(true,notify);
    }
  }


// Return true if showing hidden files
FXbool FXFileList::showHiddenFiles() const {
  return (options&FILELIST_SHOWHIDDEN)!=0;
  }


// Change show directories only mode
void FXFileList::showOnlyDirectories(FXbool flag,FXbool notify){
  FXuint opts=(((0-flag)^options)&FILELIST_SHOWDIRS)^options;
  if(opts!=options){
    options=opts;
    listItems(true,notify);
    }
  }


// Return true if showing directories only
FXbool FXFileList::showOnlyDirectories() const {
  return (options&FILELIST_SHOWDIRS)!=0;
  }


// Show files only
void FXFileList::showOnlyFiles(FXbool flag,FXbool notify){
  FXuint opts=(((0-flag)^options)&FILELIST_SHOWFILES)^options;
  if(opts!=options){
    options=opts;
    listItems(true,notify);
    }
  }


// Return true if showing files only
FXbool FXFileList::showOnlyFiles() const {
  return (options&FILELIST_SHOWFILES)!=0;
  }


// Show parent directories
void FXFileList::showParents(FXbool flag,FXbool notify){
  FXuint opts=(((flag-1)^options)&FILELIST_NO_PARENT)^options;
  if(opts!=options){
    options=opts;
    listItems(true,notify);
    }
  }


// Return true if showing parent directories
FXbool FXFileList::showParents() const {
  return (options&FILELIST_NO_PARENT)==0;
  }


// Change show image display mode
void FXFileList::showImages(FXbool flag,FXbool notify){
  FXuint opts=(((0-flag)^options)&FILELIST_SHOWIMAGES)^options;
  if(opts!=options){
    options=opts;
    listItems(true,notify);
    }
  }


// Return true if displaying image
FXbool FXFileList::showImages() const {
  return (options&FILELIST_SHOWIMAGES)!=0;
  }


// Change images preview size
void FXFileList::setImageSize(FXint size,FXbool notify){
  if(size!=imagesize){
    imagesize=size;
    listItems(true,notify);
    }
  }


// Change file associations; delete the old one unless it was shared
void FXFileList::setAssociations(FXFileAssociations* assocs,FXbool owned,FXbool notify){
  FXuint opts=options;
  options^=((owned-1)^options)&FILELIST_NO_OWN_ASSOC;
  if(associations!=assocs){
    if(!(opts&FILELIST_NO_OWN_ASSOC)) delete associations;
    associations=assocs;
    listItems(true,notify);
    }
  }


// Set draggable files
void FXFileList::setDraggableFiles(FXbool flg,FXbool notify){
  if(draggable!=flg){
    draggable=flg;
    listItems(true,notify);
    }
  }


// Set file time format
void FXFileList::setTimeFormat(const FXString& fmt,FXbool notify){
  if(timeformat!=fmt){
    timeformat=fmt;
    listItems(true,notify);
    }
  }

/*******************************************************************************/

// Save data
void FXFileList::save(FXStream& store) const {
  FXIconList::save(store);
  store << associations;
  store << iconloader;
  store << big_folder;
  store << mini_folder;
  store << big_doc;
  store << mini_doc;
  store << big_app;
  store << mini_app;
  store << directory;
  store << pattern;
  store << timeformat;
  store << matchmode;
  store << imagesize;
  store << draggable;
  }


// Load data
void FXFileList::load(FXStream& store){
  FXIconList::load(store);
  store >> associations;
  store >> iconloader;
  store >> big_folder;
  store >> mini_folder;
  store >> big_doc;
  store >> mini_doc;
  store >> big_app;
  store >> mini_app;
  store >> directory;
  store >> pattern;
  store >> timeformat;
  store >> matchmode;
  store >> imagesize;
  store >> draggable;
  }


// Cleanup
FXFileList::~FXFileList(){
  getApp()->removeChore(this);
  getApp()->removeTimeout(this,ID_OPENTIMER);
  getApp()->removeTimeout(this,ID_REFRESHTIMER);
  if(!(options&FILELIST_NO_OWN_ASSOC)) delete associations;
  delete big_folder;
  delete mini_folder;
  delete big_doc;
  delete mini_doc;
  delete big_app;
  delete mini_app;
  associations=(FXFileAssociations*)-1L;
  iconloader=(FXIconSource*)-1L;
  list=(FXFileItem*)-1L;
  big_folder=(FXIcon*)-1L;
  mini_folder=(FXIcon*)-1L;
  big_doc=(FXIcon*)-1L;
  mini_doc=(FXIcon*)-1L;
  big_app=(FXIcon*)-1L;
  mini_app=(FXIcon*)-1L;
  }

}
