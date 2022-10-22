/********************************************************************************
*                                                                               *
*           W a t c h   D i r e c t o r i e s   f o r   C h a n g e s           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxchar.h"
#include "fxmath.h"
#include "FXException.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXEvent.h"
#include "FXStringDictionary.h"
#include "FXDictionary.h"
#include "FXReverseDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXPath.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXStat.h"
#include "FXDirWatch.h"

/*
  Notes:
  - Make this more elegant.
  - See also inotify(7) and fanotify(7).
  - Perhaps only create & add handle to FXApp when first watch is created,
    and remove it when last watch is removed.
  - For inotify_event, the following may be of some importance:

      "The name field is only present when an event is returned for a file
      inside a watched directory; it identifies the file pathname relative
      to the watched directory. This pathname is null-terminated, and may
      include further null bytes to align subsequent reads to a suitable
      address boundary."

  - In other words, if watching a file then there is no name string in the
    inotify_event!!
*/


// Maximum message size
#define MAXMESSAGE 8192

// Bad handle value
#ifdef WIN32
#define BadHandle  INVALID_HANDLE_VALUE
#else
#define BadHandle  -1
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {


// Notify struct containing data from directory watch
#if defined(HAVE_INOTIFY_INIT1)
struct INotify {
  union {
    inotify_event e;
    FXlong        d[(MAXPATHLEN+sizeof(inotify_event))/sizeof(FXlong)];
    } info;
  };
#endif


// Map
FXDEFMAP(FXDirWatch) FXDirWatchMap[]={
  FXMAPFUNC(SEL_IO_READ,FXDirWatch::ID_IO_READ,FXDirWatch::onMessage)
  };


// Object implementation
FXIMPLEMENT(FXDirWatch,FXObject,FXDirWatchMap,ARRAYNUMBER(FXDirWatchMap));


// Add handler to application
FXDirWatch::FXDirWatch(FXApp* a,FXObject* tgt,FXSelector sel):app(a),hnd(BadHandle),target(tgt),message(sel){
  FXTRACE((1,"FXDirWatch::FXDirWatch(%p,%p,%d)\n",a,tgt,sel));
#if defined(HAVE_INOTIFY_INIT1)
  hnd=::inotify_init1(IN_CLOEXEC);
  if(hnd==BadHandle){ throw FXResourceException("unable to create directory watcher."); }
  FXTRACE((1,"hnd=%d\n",hnd));
  getApp()->addInput(this,ID_IO_READ,hnd,INPUT_READ);
#endif
  }


// Add path to watch; return true if added
FXbool FXDirWatch::addWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::addWatch(%s)\n",path.text()));
  if(!pathToHandle.has(path)){
#if defined(WIN32)
    FXuint attrs;
    HANDLE h;
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,path.text(),MAXPATHLEN);
    attrs=::GetFileAttributesW(unifile);
    if((attrs!=INVALID_FILE_ATTRIBUTES) && (attrs&FILE_ATTRIBUTE_DIRECTORY)){
      h=::FindFirstChangeNotificationW(unifile,false,FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE);
      if(h!=BadHandle){
        app->addInput(this,ID_IO_READ,h,INPUT_READ,(FXptr)h);
        pathToHandle[path]=(FXptr)h;
        handleToPath[(FXptr)h]=path;
        FXTRACE((1,"%s -> %d\n",path.text(),h));
        return true;
        }
      }
#else
    attrs=::GetFileAttributesA(path.text());
    if((attrs!=INVALID_FILE_ATTRIBUTES) && (attrs&FILE_ATTRIBUTE_DIRECTORY)){
      h=::FindFirstChangeNotificationA(path.text(),false,FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE);
      if(h!=BadHandle){
        app->addInput(this,ID_IO_READ,h,INPUT_READ,(FXptr)h);
        pathToHandle[path]=(FXptr)h;
        handleToPath[(FXptr)h]=path;
        FXTRACE((1,"%s -> %d\n",path.text(),h));
        return true;
        }
      }
#endif
#elif defined(HAVE_INOTIFY_INIT1)
    FXStat stat;
    if(FXStat::statFile(path,stat)){
      FXuint flags=stat.isDirectory()?(IN_ATTRIB|IN_MOVE|IN_CREATE|IN_DELETE|IN_DELETE_SELF):(IN_ATTRIB|IN_MODIFY|IN_MOVE|IN_MOVE_SELF|IN_DELETE_SELF);
      FXInputHandle h=::inotify_add_watch(hnd,path.text(),flags);
      if(h!=BadHandle){
        pathToHandle[path]=(FXptr)(FXival)h;
        handleToPath[(FXptr)(FXival)h]=path;
        FXTRACE((1,"%s -> %d\n",path.text(),h));
        return true;
        }
      }
#endif
    }
  return false;
  }


// Remove path to watch; return true if removed
FXbool FXDirWatch::remWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::remWatch(%s)\n",path.text()));
  if(pathToHandle.has(path)){
#if defined(WIN32)
    HANDLE h=(HANDLE)pathToHandle[path];
    pathToHandle.remove(path);
    handleToPath.remove((FXptr)h);
    app->removeInput(h,INPUT_READ);
    if(FindCloseChangeNotification(h)!=0){
      FXTRACE((1,"%s -> %d\n",path.text(),h));
      return true;
      }
#elif defined(HAVE_INOTIFY_INIT1)
    FXInputHandle h=(FXival)pathToHandle[path];
    pathToHandle.remove(path);
    handleToPath.remove((FXptr)(FXival)h);
    if(::inotify_rm_watch(hnd,h)!=BadHandle){
      FXTRACE((1,"%s -> %d\n",path.text(),h));
      return true;
      }
#endif
    }
  return false;
  }

//BOOL ReadDirectoryChangesW(
//  HANDLE                          hDirectory,
//  LPVOID                          lpBuffer,
//  DWORD                           nBufferLength,
//  BOOL                            bWatchSubtree,
//  DWORD                           dwNotifyFilter,
//  LPDWORD                         lpBytesReturned,
//  LPOVERLAPPED                    lpOverlapped,
//  LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
//);


// Clear all watches
FXbool FXDirWatch::clearAll(){
  FXTRACE((1,"FXDirWatch::clearAll\n"));
#if defined(WIN32)
  HANDLE h;
  for(FXint i=0; i<pathToHandle.no(); ++i){
    if(!pathToHandle.empty(i)){
      h=(HANDLE)pathToHandle.data(i);
      app->removeInput(h,INPUT_READ);
      FindCloseChangeNotification(h);
      }
    }
#elif defined(HAVE_INOTIFY_INIT1)
  for(FXint i=0; i<pathToHandle.no(); ++i){
    if(!pathToHandle.empty(i)){
      ::inotify_rm_watch(hnd,(FXival)pathToHandle.data(i));
      }
    }
#endif
  pathToHandle.clear();
  handleToPath.clear();
  return false;
  }


// Fire signal message to target
long FXDirWatch::onMessage(FXObject*,FXSelector,void* ptr){
  FXTRACE((1,"FXDirWatch::onMessage()\n"));
#if defined(WIN32)
  FXString pathname;
  HANDLE h=(HANDLE)ptr;
  FindNextChangeNotification(h);
  pathname=handleToPath[(FXptr)h];
  FXTRACE((2,"pathname=\"%s\"\n",pathname.text()));
#elif defined(HAVE_INOTIFY_INIT1)
  FXchar *beg,*end,*pne;
  inotify_event* ne;
  FXival expect=0,actual=0;
  FXString path,pathname;
  ::ioctl(hnd,FIONREAD,&expect);
  FXTRACE((1,"expect=%ld\n",expect));
  if(0<expect){
    if(allocElms(beg,expect)){
      actual=::read(hnd,beg,expect);
      FXTRACE((1,"actual=%ld\n",actual));
      if(0<actual){
        end=beg+actual;
        pne=beg;
        while(pne<end){
          ne=(inotify_event*)pne;
          path=handleToPath[(FXptr)(FXival)ne->wd];
          pathname=FXPath::absolute(path,ne->name);
          FXTRACE((2,"wd=%d mask=%x cookie=%u len=%u name=\"%s\" path=\"%s\" pathname=\"%s\"\n",ne->wd,ne->mask,ne->cookie,ne->len,ne->name,path.text(),pathname.text()));
          if(ne->mask&IN_ACCESS)        FXTRACE((2,"IN_ACCESS "));
          if(ne->mask&IN_ATTRIB)        FXTRACE((2,"IN_ATTRIB "));
          if(ne->mask&IN_CLOSE_NOWRITE) FXTRACE((2,"IN_CLOSE_NOWRITE "));
          if(ne->mask&IN_CLOSE_WRITE)   FXTRACE((2,"IN_CLOSE_WRITE "));
          if(ne->mask&IN_CREATE)        FXTRACE((2,"IN_CREATE "));
          if(ne->mask&IN_DELETE)        FXTRACE((2,"IN_DELETE "));
          if(ne->mask&IN_DELETE_SELF)   FXTRACE((2,"IN_DELETE_SELF "));
          if(ne->mask&IN_IGNORED)       FXTRACE((2,"IN_IGNORED "));
          if(ne->mask&IN_ISDIR)         FXTRACE((2,"IN_ISDIR "));
          if(ne->mask&IN_MODIFY)        FXTRACE((2,"IN_MODIFY "));
          if(ne->mask&IN_MOVE_SELF)     FXTRACE((2,"IN_MOVE_SELF "));
          if(ne->mask&IN_MOVED_FROM)    FXTRACE((2,"IN_MOVED_FROM "));
          if(ne->mask&IN_MOVED_TO)      FXTRACE((2,"IN_MOVED_TO "));
          if(ne->mask&IN_OPEN)          FXTRACE((2,"IN_OPEN "));
          if(ne->mask&IN_Q_OVERFLOW)    FXTRACE((2,"IN_Q_OVERFLOW "));
          if(ne->mask&IN_UNMOUNT)       FXTRACE((2,"IN_UNMOUNT "));
          FXTRACE((2,"\n"));
          if(ne->mask&(IN_MOVED_TO|IN_CREATE)){
            FXTRACE((1,"SEL_INSERTED \"%s\"\n",pathname.text()));
            }
          else if(ne->mask&(IN_DELETE|IN_MOVED_FROM)){
            FXTRACE((1,"SEL_DELETED \"%s\"\n",pathname.text()));
            }
          else if(ne->mask&(IN_ATTRIB)){
            FXTRACE((1,"SEL_CHANGED \"%s\"\n",pathname.text()));
            }
          pne+=sizeof(inotify_event)+ne->len;
          }
        }
      freeElms(beg);
      }
    }
#endif
  return 1;
  }


// Remove handler from application
FXDirWatch::~FXDirWatch(){
  FXTRACE((1,"FXDirWatch::~FXDirWatch\n"));
  clearAll();
#if defined(HAVE_INOTIFY_INIT1)
  if(hnd!=BadHandle){
    app->removeInput(hnd,INPUT_READ);
    ::close(hnd);
    }
#endif
  }

}
