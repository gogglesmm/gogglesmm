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
#include "FXIO.h"
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

  - We need a fallback version that uses timers.
*/


// Maximum message size
#define MAXMESSAGE 8192

// Bad handle value
#ifdef WIN32
#define BadHandle  INVALID_HANDLE_VALUE
#else
#define BadHandle  -1
#endif


// Test
#undef HAVE_INOTIFY_INIT1


using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXDirWatch) FXDirWatchMap[]={
  FXMAPFUNC(SEL_IO_READ,FXDirWatch::ID_CHANGE,FXDirWatch::onMessage),
  FXMAPFUNC(SEL_TIMEOUT,FXDirWatch::ID_CHANGE,FXDirWatch::onMessage)
  };


// Object implementation
FXIMPLEMENT(FXDirWatch,FXObject,FXDirWatchMap,ARRAYNUMBER(FXDirWatchMap));


// Add handler to application
FXDirWatch::FXDirWatch(FXApp* a,FXObject* tgt,FXSelector sel):app(a),hnd(BadHandle),timestamp(0),target(tgt),message(sel){
  FXTRACE((1,"FXDirWatch::FXDirWatch(%p,%p,%d)\n",a,tgt,sel));
  }


// Remove handler from application
FXDirWatch::~FXDirWatch(){
  FXTRACE((1,"FXDirWatch::~FXDirWatch\n"));
  clearAll();
  app=(FXApp*)-1L;
  target=(FXObject*)-1L;
  }


#if defined(WIN32)  /////////////////////////////////////////////////////////////


// Event filter flags
const FXuint FILTER=FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE;


// FIXME important note: watches contents of directory, not a file.
// FIXME also, does not watch directory itself.
// FIXME thus, to watch directory /path/dir, you may need to watch /path AND /path/dir,
// FIXME or watch /path with recursive option....

// Add path to watch; return true if added
FXbool FXDirWatch::addWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::addWatch(%s)\n",path.text()));
  if(!pathToHandle.has(path)){
#if defined(UNICODE)
    FXnchar unifile[MAXPATHLEN];
    utf2ncs(unifile,path.text(),MAXPATHLEN);
    FXuint attrs=::GetFileAttributesW(unifile);
    if((attrs!=INVALID_FILE_ATTRIBUTES) && (attrs&FILE_ATTRIBUTE_DIRECTORY)){
      HANDLE h=::FindFirstChangeNotificationW(unifile,false,FILTER);
#else
    FXuint attrs=::GetFileAttributesA(path.text());
    if((attrs!=INVALID_FILE_ATTRIBUTES) && (attrs&FILE_ATTRIBUTE_DIRECTORY)){
      HANDLE h=::FindFirstChangeNotificationA(path.text(),false,FILTER);
#endif
      if(h!=BadHandle){
        app->addInput(this,ID_CHANGE,h,INPUT_READ,(FXptr)h);
        pathToHandle[path]=(FXptr)h;
        handleToPath[(FXptr)h]=path;
        FXTRACE((1,"%s -> %d\n",path.text(),h));
        return true;
        }
      }
    }
  return false;
  }


// Remove path to watch; return true if removed
FXbool FXDirWatch::remWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::remWatch(%s)\n",path.text()));
  if(pathToHandle.has(path)){
    HANDLE h=(HANDLE)pathToHandle[path];
    pathToHandle.remove(path);
    handleToPath.remove((FXptr)h);
    app->removeInput(h,INPUT_READ);
    if(FindCloseChangeNotification(h)!=0){
      FXTRACE((1,"%s -> %d\n",path.text(),h));
      return true;
      }
    }
  return false;
  }


// Clear all watches
FXbool FXDirWatch::clearAll(){
  FXTRACE((1,"FXDirWatch::clearAll\n"));
  if(pathToHandle.used()!=0){
    for(FXint i=0; i<pathToHandle.no(); ++i){
      if(!pathToHandle.empty(i)){
        HANDLE h=(HANDLE)pathToHandle.data(i);
        app->removeInput(h,INPUT_READ);
        FindCloseChangeNotification(h);
        }
      }
    pathToHandle.clear();
    handleToPath.clear();
    return true;
    }
  return false;
  }


// Fire signal message to target
long FXDirWatch::onMessage(FXObject*,FXSelector,void* ptr){
  FXTRACE((1,"FXDirWatch::onMessage()\n"));
  FXString pathname;
  HANDLE h=(HANDLE)ptr;
  FindNextChangeNotification(h);
  pathname=handleToPath[(FXptr)h];
  FXTRACE((2,"pathname=\"%s\"\n",pathname.text()));
#if 0
BOOL ReadDirectoryChangesW(
  HANDLE                          hDirectory,
  LPVOID                          lpBuffer,
  DWORD                           nBufferLength,
  BOOL                            bWatchSubtree,
  DWORD                           dwNotifyFilter,
  LPDWORD                         lpBytesReturned,
  LPOVERLAPPED                    lpOverlapped,
  LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
  );

struct FILE_NOTIFY_INFORMATION {
  DWORD NextEntryOffset;
  DWORD Action;
  DWORD FileNameLength;
  WCHAR FileName[1];
  };

  USES_CONVERSION;
  HANDLE hDir = CreateFile("c:\\Folder", // pointer to the file name
    FILE_LIST_DIRECTORY,                // access (read-write) mode
    FILE_SHARE_READ|FILE_SHARE_DELETE,  // share mode
    NULL, // security descriptor
    OPEN_EXISTING, // how to create
    FILE_FLAG_BACKUP_SEMANTICS, // file attributes
    NULL // file with attributes to copy
  );

  FILE_NOTIFY_INFORMATION Buffer[1024];
  DWORD BytesReturned;
  while( ReadDirectoryChangesW(hDir, // handle to directory
     &Buffer, // read results buffer
     sizeof(Buffer), // length of buffer
     TRUE, // monitoring option
     FILE_NOTIFY_CHANGE_SECURITY|FILE_NOTIFY_CHANGE_CREATION|FILE_NOTIFY_CHANGE_LAST_ACCESS|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_FILE_NAME, // filter conditions
     &BytesReturned, // bytes returned
     NULL, // overlapped buffer
     NULL)){ // completion routine
       ...
   }
  ::CloseHandle(hDir);
#endif

  return 1;
  }


#elif defined(HAVE_INOTIFY_INIT1)  //////////////////////////////////////////////


// Event filter flags
const FXuint FILTER_DIRS=IN_ATTRIB|IN_DELETE_SELF|IN_MOVE|IN_CREATE|IN_DELETE;
const FXuint FILTER_FILE=IN_ATTRIB|IN_DELETE_SELF|IN_MOVE|IN_MODIFY|IN_MOVE_SELF;


// Add path to watch; return true if added
FXbool FXDirWatch::addWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::addWatch(%s)\n",path.text()));
  if(!pathToHandle.has(path)){
    FXStat stat;
    if(FXStat::statFile(path,stat)){
      if(pathToHandle.used()==0){
        hnd=::inotify_init1(IN_CLOEXEC);  //hnd=::inotify_init1(IN_CLOEXEC|O_NONBLOCK);
        if(hnd!=BadHandle){
          getApp()->addInput(this,ID_CHANGE,hnd,INPUT_READ);
          }
        }
      if(hnd!=BadHandle){
        FXuint mask=stat.isDirectory()?FILTER_DIRS:FILTER_FILE;
        FXInputHandle h=::inotify_add_watch(hnd,path.text(),mask);
        if(h!=BadHandle){
          pathToHandle[path]=(FXptr)(FXival)h;
          handleToPath[(FXptr)(FXival)h]=path;
          FXTRACE((1,"FXDirWatch::addWatch(%s) -> %d\n",path.text(),h));
          return true;
          }
        }
      }
    }
  return false;
  }


// Remove path to watch; return true if removed
FXbool FXDirWatch::remWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::remWatch(%s)\n",path.text()));
  if(pathToHandle.has(path)){
    if(hnd!=BadHandle){
      FXInputHandle h=(FXival)pathToHandle[path];
      pathToHandle.remove(path);
      handleToPath.remove((FXptr)(FXival)h);
      if(::inotify_rm_watch(hnd,h)!=BadHandle){
        FXTRACE((1,"FXDirWatch::remWatch(%s) -> %d\n",path.text(),h));
        if(pathToHandle.used()==0){
          app->removeInput(hnd,INPUT_READ);
          ::close(hnd);
          hnd=BadHandle;
          }
        return true;
        }
      }
    }
  return false;
  }


// Clear all watches
FXbool FXDirWatch::clearAll(){
  FXTRACE((1,"FXDirWatch::clearAll\n"));
  if(pathToHandle.used()!=0){
    for(FXint i=0; i<pathToHandle.no(); ++i){
      if(!pathToHandle.empty(i)){
        ::inotify_rm_watch(hnd,(FXival)pathToHandle.data(i));
        }
      }
    pathToHandle.clear();
    handleToPath.clear();
    app->removeInput(hnd,INPUT_READ);
    ::close(hnd);
    hnd=BadHandle;
    return true;
    }
  return false;
  }


// Fire signal message to target
long FXDirWatch::onMessage(FXObject*,FXSelector,void*){
  FXTRACE((1,"FXDirWatch::onMessage()\n"));
  FXival expect=0;
  if(0<=::ioctl(hnd,FIONREAD,&expect) && 0<expect){
    FXTRACE((1,"expect=%ld\n",expect));
    FXchar *ptr;
    if(allocElms(ptr,expect)){
      FXival actual=::read(hnd,ptr,expect);
      FXTRACE((1,"actual=%ld\n",actual));
      if(0<actual){
        FXchar *end=ptr+actual;
        FXchar *pne=ptr;
        while(pne<end){
          inotify_event* ne=(inotify_event*)pne;
          FXString pathname=FXPath::absolute(handleToPath[(FXptr)(FXival)ne->wd],ne->name);
          FXTRACE((2,"wd=%d mask=%x cookie=%u len=%u name=\"%s\" pathname=\"%s\"\n",ne->wd,ne->mask,ne->cookie,ne->len,ne->name,pathname.text()));
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
      freeElms(ptr);
      }
    }
  return 1;
  }


#else  //////////////////////////////////////////////////////////////////////////


// Interval between refreshes
const FXTime REFRESHINTERVAL=1000000000;


// Add path to watch; return true if added
FXbool FXDirWatch::addWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::addWatch(%s)\n",path.text()));
  if(!pathToHandle.has(path)){
    FXStat stat;
    if(FXStat::statFile(path,stat)){
      if(pathToHandle.used()==0){
        getApp()->addTimeout(this,ID_CHANGE,REFRESHINTERVAL);
        }
      pathToHandle[path]=(FXptr)(stat.isFile()?1L:2L);
      return true;
      }
    }
  return false;
  }


// Remove path to watch; return true if removed
FXbool FXDirWatch::remWatch(const FXString& path){
  FXTRACE((1,"FXDirWatch::remWatch(%s)\n",path.text()));
  if(pathToHandle.has(path)){
    pathToHandle.remove(path);
    if(pathToHandle.used()==0){
      getApp()->removeTimeout(this,ID_CHANGE);
      }
    return true;
    }
  return false;
  }


// Clear all watches
FXbool FXDirWatch::clearAll(){
  FXTRACE((1,"FXDirWatch::clearAll()\n"));
  if(pathToHandle.used()!=0){
    pathToHandle.clear();
    getApp()->removeTimeout(this,ID_CHANGE);
    return true;
    }
  return false;
  }


// Fire signal message to target
long FXDirWatch::onMessage(FXObject*,FXSelector,void*){
  FXTRACE((1,"FXDirWatch::onMessage()\n"));
  if(pathToHandle.used()!=0){
    FXTime newstamp=0;
    for(FXint i=0; i<pathToHandle.no(); ++i){
      if(!pathToHandle.empty(i)){
        FXStat stat;
        if(FXStat::statFile(pathToHandle.key(i),stat)){
          FXTime time=stat.modified();
          if(newstamp<time) newstamp=time;
          if(timestamp<time){
            FXTRACE((1,"SEL_CHANGED \"%s\"\n",pathToHandle.key(i).text()));
            }
          }
        else{
          FXTRACE((1,"SEL_DELETED \"%s\"\n",pathToHandle.key(i).text()));
          }
        }
      }
    timestamp=newstamp;
    getApp()->addTimeout(this,ID_CHANGE,REFRESHINTERVAL);
    }
  return 1;
  }


#endif  /////////////////////////////////////////////////////////////////////////

}
