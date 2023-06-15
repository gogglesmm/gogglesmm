/********************************************************************************
*                                                                               *
*                    D i r e c t o r y   E n u m e r a t o r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXPath.h"
#include "FXDir.h"

/*
  Notes:
  - This class implements a way to list the files in a directory.
  - We just want to wrap directory iteration, nothing fancy.
  - Maybe add positioning for seek and tell type functions.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


#ifdef WIN32
struct SPACE {
  HANDLE          handle;
  FXuint          first;
  WIN32_FIND_DATA result;
  };
#else
struct SPACE {
  DIR*            handle;
  struct dirent*  dp;
  };
#endif


// Construct directory enumerator
FXDir::FXDir(){
  // If this fails on your machine, determine what sizeof(SPACE) is
  // on your machine and mail it to: jeroen@fox-toolkit.net!
  //FXTRACE((150,"sizeof(SPACE)=%ld\n",sizeof(SPACE)));
  FXASSERT(sizeof(SPACE)<=sizeof(space));
#ifdef WIN32
  alias_cast<SPACE>(space)->handle=INVALID_HANDLE_VALUE;
#else
  alias_cast<SPACE>(space)->handle=nullptr;
#endif
  }


// Construct directory enumerator
FXDir::FXDir(const FXString& path){
  // If this fails on your machine, determine what sizeof(SPACE) is
  // on your machine and mail it to: jeroen@fox-toolkit.net!
  //FXTRACE((150,"sizeof(SPACE)=%ld\n",sizeof(SPACE)));
  FXASSERT(sizeof(SPACE)<=sizeof(space));
#ifdef WIN32
  alias_cast<SPACE>(space)->handle=INVALID_HANDLE_VALUE;
#else
  alias_cast<SPACE>(space)->handle=nullptr;
#endif
  open(path);
  }


// Open directory to path, return true if ok.
FXbool FXDir::open(const FXString& path){
  if(!path.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar buffer[MAXPATHLEN+2];
    utf2ncs(buffer,path.text(),MAXPATHLEN);
    wcsncat(buffer,TEXT("\\*"),MAXPATHLEN+2);
#else
    FXchar buffer[MAXPATHLEN+2];
    fxstrlcpy(buffer,path.text(),MAXPATHLEN);
    fxstrlcat(buffer,"\\*",MAXPATHLEN+2);
#endif
    alias_cast<SPACE>(space)->handle=FindFirstFile(buffer,&alias_cast<SPACE>(space)->result);
    if(alias_cast<SPACE>(space)->handle!=INVALID_HANDLE_VALUE){
      alias_cast<SPACE>(space)->first=true;
      return true;
      }
#else
    alias_cast<SPACE>(space)->handle=opendir(path.text());
    if(alias_cast<SPACE>(space)->handle!=nullptr){
      return true;
      }
#endif
    }
  return false;
  }


// Returns true if the directory is open
FXbool FXDir::isOpen() const {
#ifdef WIN32
  return (alias_cast<const SPACE>(space)->handle!=INVALID_HANDLE_VALUE);
#else
  return alias_cast<const SPACE>(space)->handle!=nullptr;
#endif
  }


// Go to next directory entry and return its name
FXbool FXDir::next(FXString& name){
  if(isOpen()){
#if defined(WIN32)
    if(alias_cast<SPACE>(space)->first || FindNextFile(alias_cast<SPACE>(space)->handle,&alias_cast<SPACE>(space)->result)){
      alias_cast<SPACE>(space)->first=false;
      name.assign(alias_cast<SPACE>(space)->result.cFileName);
      return true;
      }
#else
    if((alias_cast<SPACE>(space)->dp=readdir(alias_cast<SPACE>(space)->handle))!=nullptr){
      name.assign(alias_cast<SPACE>(space)->dp->d_name);
      return true;
      }
#endif
    }
  name.clear();
  return false;
  }


// Close directory
void FXDir::close(){
  if(isOpen()){
#ifdef WIN32
    FindClose(alias_cast<SPACE>(space)->handle);
    alias_cast<SPACE>(space)->handle=INVALID_HANDLE_VALUE;
#else
    closedir(alias_cast<SPACE>(space)->handle);
    alias_cast<SPACE>(space)->handle=nullptr;
#endif
    }
  }


// Create new directory
FXbool FXDir::create(const FXString& path,FXuint perm){
  if(!path.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar buffer[MAXPATHLEN];
    utf2ncs(buffer,path.text(),MAXPATHLEN);
    return CreateDirectoryW(buffer,nullptr)!=0;
#else
    return CreateDirectoryA(path.text(),nullptr)!=0;
#endif
#else
    return ::mkdir(path.text(),perm)==0;
#endif
    }
  return false;
  }


// Remove directory
FXbool FXDir::remove(const FXString& path){
  if(!path.empty()){
#ifdef WIN32
#ifdef UNICODE
    FXnchar buffer[MAXPATHLEN];
    utf2ncs(buffer,path.text(),MAXPATHLEN);
    return RemoveDirectoryW(buffer)!=0;
#else
    return RemoveDirectoryA(path.text())!=0;
#endif
#else
    return ::rmdir(path.text())==0;
#endif
    }
  return false;
  }


// List all the files in directory
FXint FXDir::listFiles(FXString*& filelist,const FXString& path,const FXString& pattern,FXuint flags){
  FXDir dir(path);

  // Initialize to empty
  filelist=nullptr;

  // Get directory stream pointer
  if(dir.isOpen()){
    FXuint    mode=(flags&CaseFold)?(FXPath::PathName|FXPath::NoEscape|FXPath::CaseFold):(FXPath::PathName|FXPath::NoEscape);
    FXString *newlist;
    FXint     size=0;
    FXint     count=0;
    FXString  pathname;
    FXString  name;
    FXStat    data;

    // Loop over directory entries
    while(dir.next(name)){

      // Build full pathname
      pathname=path;
      if(!ISPATHSEP(pathname.tail())) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Get info on file
      if(!FXStat::statFile(pathname,data)) continue;

#ifdef WIN32

      // Filter out files; a bit tricky...
      if(!data.isDirectory() && ((flags&NoFiles) || (data.isHidden() && !(flags&HiddenFiles)) || (!(flags&AllFiles) && !FXPath::match(name,pattern,mode)))) continue;

      // Filter out directories; even more tricky!
      if(data.isDirectory() && ((flags&NoDirs) || (data.isHidden() && !(flags&HiddenDirs)) || ((name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0))) && (flags&NoParent)) || (!(flags&AllDirs) && !FXPath::match(name,pattern,mode)))) continue;

#else

      // Filter out files; a bit tricky...
      if(!data.isDirectory() && ((flags&NoFiles) || (name[0]=='.' && !(flags&HiddenFiles)) || (!(flags&AllFiles) && !FXPath::match(name,pattern,mode)))) continue;

      // Filter out directories; even more tricky!
      if(data.isDirectory() && ((flags&NoDirs) || (name[0]=='.' && !(flags&HiddenDirs)) || ((name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0))) && (flags&NoParent)) || (!(flags&AllDirs) && !FXPath::match(name,pattern,mode)))) continue;

#endif

      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString [size];
        for(FXint i=0; i<count; i++){
          newlist[i].adopt(filelist[i]);
          }
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++].adopt(name);
      }
    return count;
    }
  return 0;
  }


// List drives, i.e. roots of directory trees.
FXint FXDir::listDrives(FXString*& drivelist){
  FXint count=0;
#ifdef WIN32
  FXchar drives[256];
  clearElms(drives,ARRAYNUMBER(drives));
  GetLogicalDriveStringsA(ARRAYNUMBER(drives),drives);
  drivelist=new FXString [33];
  for(const FXchar* drive=drives; *drive && count<32; drive++){
    drivelist[count].assign(drive);
    while(*drive) drive++;
    count++;
    }
#else
  drivelist=new FXString [2];
  drivelist[count++].assign(PATHSEP);
#endif
  return count;
  }


#if 0

FXint FXDir::listShares(FXString*& sharelist){
  FXint count=0;
#ifdef WIN32
#else
  sharelist=new FXString [2];
  sharelist[count++].assign(PATHSEP);
#endif
  return count;
  }
#endif


// Create a directories recursively
FXbool FXDir::createDirectories(const FXString& path,FXuint perm){
  if(FXPath::isAbsolute(path)){
    if(FXStat::isDirectory(path)) return true;
    if(createDirectories(FXPath::upLevel(path),perm)){
      if(FXDir::create(path,perm)) return true;
      }
    }
  return false;
  }


// Cleanup
FXDir::~FXDir(){
  close();
  }


}




#if 0

// List all the files in directory
FXint FXDir::listFiles(FXString*& filelist,const FXString& path,const FXString& pattern,FXuint flags){
  FXuint matchmode=FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE;
  FXString pathname;
  FXString name;
  FXString *newlist;
  FXint count=0;
  FXint size=0;
  WIN32_FIND_DATA ffData;
  DWORD nCount,nSize,i,j;
  HANDLE hFindFile,hEnum;
  FXchar server[200];

  // Initialize to empty
  filelist=nullptr;

/*
  // Each drive is a root on windows
  if(path.empty()){
    FXchar letter[4];
    letter[0]='a';
    letter[1]=':';
    letter[2]=PATHSEP;
    letter[3]='\0';
    filelist=new FXString[28];
    for(DWORD mask=GetLogicalDrives(); mask; mask>>=1,letter[0]++){
      if(mask&1) list[count++]=letter;
      }
    filelist[count++]=PATHSEPSTRING PATHSEPSTRING;    // UNC for file shares
    return count;
    }
*/
/*
  // A UNC name was given of the form "\\" or "\\server"
  if(ISPATHSEP(path[0]) && ISPATHSEP(path[1]) && path.find(PATHSEP,2)<0){
    NETRESOURCE host;

    // Fill in
    host.dwScope=RESOURCE_GLOBALNET;
    host.dwType=RESOURCETYPE_DISK;
    host.dwDisplayType=RESOURCEDISPLAYTYPE_GENERIC;
    host.dwUsage=RESOURCEUSAGE_CONTAINER;
    host.lpLocalName=nullptr;
    host.lpRemoteName=(char*)path.text();
    host.lpComment=nullptr;
    host.lpProvider=nullptr;

    // Open network enumeration
    if(WNetOpenEnum((path[2]?RESOURCE_GLOBALNET:RESOURCE_CONTEXT),RESOURCETYPE_DISK,0,(path[2]?&host:nullptr),&hEnum)==NO_ERROR){
      NETRESOURCE resource[16384/sizeof(NETRESOURCE)];
      FXTRACE((1,"Enumerating=%s\n",path.text()));
      while(1){
        nCount=-1;    // Read as many as will fit
        nSize=sizeof(resource);
        if(WNetEnumResource(hEnum,&nCount,resource,&nSize)!=NO_ERROR) break;
        for(i=0; i<nCount; i++){

          // Dump what we found
          FXTRACE((1,"dwScope=%s\n",resource[i].dwScope==RESOURCE_CONNECTED?"RESOURCE_CONNECTED":resource[i].dwScope==RESOURCE_GLOBALNET?"RESOURCE_GLOBALNET":resource[i].dwScope==RESOURCE_REMEMBERED?"RESOURCE_REMEMBERED":"?"));
          FXTRACE((1,"dwType=%s\n",resource[i].dwType==RESOURCETYPE_ANY?"RESOURCETYPE_ANY":resource[i].dwType==RESOURCETYPE_DISK?"RESOURCETYPE_DISK":resource[i].dwType==RESOURCETYPE_PRINT?"RESOURCETYPE_PRINT":"?"));
          FXTRACE((1,"dwDisplayType=%s\n",resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_DOMAIN?"RESOURCEDISPLAYTYPE_DOMAIN":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_SERVER?"RESOURCEDISPLAYTYPE_SERVER":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_SHARE?"RESOURCEDISPLAYTYPE_SHARE":resource[i].dwDisplayType==RESOURCEDISPLAYTYPE_GENERIC?"RESOURCEDISPLAYTYPE_GENERIC":resource[i].dwDisplayType==6?"RESOURCEDISPLAYTYPE_NETWORK":resource[i].dwDisplayType==7?"RESOURCEDISPLAYTYPE_ROOT":resource[i].dwDisplayType==8?"RESOURCEDISPLAYTYPE_SHAREADMIN":resource[i].dwDisplayType==9?"RESOURCEDISPLAYTYPE_DIRECTORY":resource[i].dwDisplayType==10?"RESOURCEDISPLAYTYPE_TREE":resource[i].dwDisplayType==11?"RESOURCEDISPLAYTYPE_NDSCONTAINER":"?"));
          FXTRACE((1,"dwUsage=%s\n",resource[i].dwUsage==RESOURCEUSAGE_CONNECTABLE?"RESOURCEUSAGE_CONNECTABLE":resource[i].dwUsage==RESOURCEUSAGE_CONTAINER?"RESOURCEUSAGE_CONTAINER":"?"));
          FXTRACE((1,"lpLocalName=%s\n",resource[i].lpLocalName));
          FXTRACE((1,"lpRemoteName=%s\n",resource[i].lpRemoteName));
          FXTRACE((1,"lpComment=%s\n",resource[i].lpComment));
          FXTRACE((1,"lpProvider=%s\n\n",resource[i].lpProvider));

          // Grow list
          if(count+1>=size){
            size=size?(size<<1):256;
            newlist=new FXString[size];
            for(j=0; j<count; j++) newlist[j]=list[j];
            delete [] filelist;
            filelist=newlist;
            }

          // Add remote name to list
          filelist[count]=resource[i].lpRemoteName;
          count++;
          }
        }
      WNetCloseEnum(hEnum);
      }
    return count;
    }
*/
  // Folding case
  if(flags&LIST_CASEFOLD) matchmode|=FILEMATCH_CASEFOLD;

  // Copy directory name
  pathname=path;
  if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
  pathname+="*";

  // Open directory
  hFindFile=FindFirstFile(pathname.text(),&ffData);
  if(hFindFile!=INVALID_HANDLE_VALUE){

    // Loop over directory entries
    do{

      // Get name
      name=ffData.cFileName;

      // Filter out files; a bit tricky...
      if(!(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_FILES) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_FILES)) || (!(flags&LIST_ALL_FILES) && !match(pattern,name,matchmode)))) continue;

      // Filter out directories; even more tricky!
      if((ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_DIRS) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_DIRS)) || (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0 && (flags&LIST_NO_PARENT)))) || (!(flags&LIST_ALL_DIRS) && !match(pattern,name,matchmode)))) continue;

      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString[size];
        for(int f=0; f<count; f++){
          newlist[f].adopt(filelist[f]);
          }
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++]=name;
      }
    while(FindNextFile(hFindFile,&ffData));
    FindClose(hFindFile);
    }
  return count;
  }














void fxenumWNetContainerResource(NETRESOURCE* netResource,FXObjectListOf<FXStringObject>& netResourceList,DWORD openEnumScope){
//  Comments are mine, unless indicated otherwise. - Daniël Hörchner <dbjh@gmx.net>
//
//  Passing the value RESOURCE_GLOBALNET for openEnumScope will make this
//  function search recursively through the network shares for disk resources.
//  Passing the value RESOURCE_CONTEXT will make this function list the servers
//  in the network neighbourhood.
  DWORD retVal;
  HANDLE handle;

  //  WNetEnumResource() reports containers as being disk resources if
  //  WNetOpenEnum() is called with RESOURCETYPE_DISK. This does not happen if
  //  it's called with RESOURCETYPE_ANY.
  //  BTW RESOURCETYPE_DISK does not guarantee that only disk resources are
  //  reported (I also get a printer container in the list).
  if((retVal=WNetOpenEnum(openEnumScope,RESOURCETYPE_DISK,0,netResource,&handle))!=NO_ERROR){
    // we get here also if access was denied to enumerate the container
    FXTRACE((1,"ERROR: WNetOpenEnum() (%d)\n", retVal));
    return;
    }

  NETRESOURCE *netResources;
  DWORD netResourcesSize=16*1024;               // 16 kB is a good size, according to MSDN
  if ((netResources=(NETRESOURCE *)malloc(netResourcesSize))==nullptr){
    FXTRACE((1,"ERROR: Not enough memory for NETRESOURCE structures\n"));
    WNetCloseEnum(handle);
    return;
    }

  do{

    DWORD nEntries=(DWORD)-1;
    retVal=WNetEnumResource(handle,&nEntries,netResources,&netResourcesSize);
    // netResourcesSize is not modified if the buffer is large enough

    if(retVal==ERROR_MORE_DATA){
      // MSDN info is not correct; ERROR_MORE_DATA means the buffer was too
      //  small for a _single_ entry
      // netResourcesSize (now) contains required size
      if((netResources=(NETRESOURCE *)realloc(netResources,netResourcesSize))==nullptr){
        FXTRACE((1,"ERROR: Reallocation for NETRESOURCE structures failed\n"));
        WNetCloseEnum(handle);
        return;
        }
      nEntries=(DWORD)-1;
      retVal=WNetEnumResource(handle,&nEntries,netResources,&netResourcesSize);
      }

    if(retVal!=NO_ERROR && retVal!=ERROR_NO_MORE_ITEMS){
      char *str;
      switch (retVal){
        case ERROR_MORE_DATA: str="more data"; break; // shouldn't happen
        case ERROR_INVALID_HANDLE: str="invalid handle"; break;
        case ERROR_NO_NETWORK: str="no network"; break;
        case ERROR_EXTENDED_ERROR: str="extended error"; break;
        default: str="unknown";
        }
      FXTRACE((1,"ERROR: Network enum error: %s (%d)\n",str,retVal));
      free(netResources);
      WNetCloseEnum(handle);
      return;
      }

    for(DWORD n=0; n < nEntries; n++){
      FXbool isContainer=false;

      // if RESOURCE_CONTEXT was passed to WNetOpenEnum(), dwScope will be
      //  RESOURCE_GLOBALNET
      if(netResources[n].dwScope==RESOURCE_GLOBALNET && (netResources[n].dwUsage&RESOURCEUSAGE_CONTAINER)){
        isContainer=true;

        //  If RESOURCE_CONTEXT was passed to WNetOpenEnum(), the first entry is
        //  a "self reference". For example, starting from the network root one
        //  can find a container entry with lpComment "Entire Network"
        //  (lpRemoteName and lpLocalName are nullptr for me). This container
        //  contains an entry with the same properties. In order to avoid getting
        //  into an infinite loop, we must handle this case.
        //  However, trying to enumerate normal containers while RESOURCE_CONTEXT
        //  was used causes WNetOpenEnum() to return ERROR_INVALID_PARAMETER.
        if(netResources[n].lpRemoteName){
          netResourceList.append(new FXStringObject(((FXString)netResources[n].lpRemoteName)+PATHSEP));
          if(openEnumScope!=RESOURCE_CONTEXT){
            fxenumWNetContainerResource(&netResources[n],netResourceList,openEnumScope);
            }
          }
        }

      // Using the variable isContainer is necessary if WNetOpenEnum() is
      //  called with RESOURCETYPE_DISK. See above.
      if(netResources[n].dwType==RESOURCETYPE_DISK && !isContainer){
        netResourceList.append(new FXStringObject(((FXString)netResources[n].lpRemoteName)+PATHSEP));
        }
      }
    }
  while(retVal!=ERROR_NO_MORE_ITEMS);

  free(netResources);
  WNetCloseEnum(handle);                        // it makes no sense to check for NO_ERROR
  }






  // Folding case
  if(flags&LIST_CASEFOLD) matchmode|=FILEMATCH_CASEFOLD;

  if(FXFile::isShareServer(path)){
    pathname=path;
    // pathname must not have an ending back slash or else
    //  fxenumWNetContainerResource() (WNetOpenEnum()) will fail
    if(ISPATHSEP(pathname[pathname.length()-1])) pathname.trunc(pathname.length()-1);
    NETRESOURCE netResource;
    memset(&netResource,0,sizeof(NETRESOURCE));
    netResource.lpRemoteName=(char *)pathname.text();

    FXObjectListOf<FXStringObject> netResourceList;
    // a share server can only provide shares, which are similar to directories
    if(!(flags&LIST_NO_DIRS))
      fxenumWNetContainerResource(&netResource,netResourceList,RESOURCE_GLOBALNET);

    for(int n=0; n < netResourceList.no(); n++){
      // Get name
      name=*netResourceList[n];
      if(ISPATHSEP(name[name.length()-1])) name.trunc(name.length()-1);
      name=FXFile::name(name);

      // Filter out directories
      if(!(flags&LIST_ALL_DIRS) && !match(pattern,name,matchmode)) continue;

      // Grow list
      if(count+1>=size){
        size=size?(size<<1):256;
        newlist=new FXString[size];
        for(int f=0; f<count; f++) newlist[f]=filelist[f];
        delete [] filelist;
        filelist=newlist;
        }

      // Add to list
      filelist[count++]=name;
      }
    for(int n=0; n < netResourceList.no(); n++) delete netResourceList[n];
    netResourceList.clear();
    }
  else{
    // Copy directory name
    pathname=path;
    if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
    pathname+="*";

    // Open directory
    hFindFile=FindFirstFile(pathname.text(),&ffData);
    if(hFindFile!=INVALID_HANDLE_VALUE){

      // Loop over directory entries
      do{

        // Get name
        name=ffData.cFileName;

        // Filter out files; a bit tricky...
        if(!(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_FILES) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_FILES)) || (!(flags&LIST_ALL_FILES) && !match(pattern,name,matchmode)))) continue;

        // Filter out directories; even more tricky!
        if((ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((flags&LIST_NO_DIRS) || ((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(flags&LIST_HIDDEN_DIRS)) || (name[0]=='.' && (name[1]==0 || (name[1]=='.' && name[2]==0 && (flags&LIST_NO_PARENT)))) || (!(flags&LIST_ALL_DIRS) && !match(pattern,name,matchmode)))) continue;

        // Grow list
        if(count+1>=size){
          size=size?(size<<1):256;
          newlist=new FXString[size];
          for(int f=0; f<count; f++) newlist[f]=filelist[f];
          delete [] filelist;
          filelist=newlist;
          }

        // Add to list
        filelist[count++]=name;
        }
      while(FindNextFile(hFindFile,&ffData));
      FindClose(hFindFile);
      }
    }
  return count;
  }

#endif
