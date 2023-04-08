/********************************************************************************
*                                                                               *
*         M i s c e l l a n e o u s   S y s t e m   F u n c t i o n s           *
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
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXFile.h"
#include "FXProcess.h"
#include "FXSystem.h"
#include "FXStat.h"



/*
  Notes:

  - A bric-a-brack of various functions we could not place anywhere else.

  - Some ISO 8601 Formats:

     Date:                        2019-09-28
     Date and time in UTC:        2019-09-28T15:33:50+00:00
                                  2019-09-28T15:33:50Z
                                  20190928T153350Z
     Week:                        2019-W39
     Date with week number:       2019-W39-6
     Date without year:           --09-28[1]
     Ordinal date:                2019-271
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Get effective user id
FXuint FXSystem::user(){
#if defined(WIN32)
  return 0;
#else
  return geteuid();
#endif
  }


// Get effective group id
FXuint FXSystem::group(){
#if defined(WIN32)
  return 0;
#else
  return getegid();
#endif
  }


// Return owner name from uid
FXString FXSystem::userName(FXuint uid){
  FXchar result[64];
#if !defined(WIN32)
#ifdef HAVE_GETPWUID_R
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(uid,&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
    return FXString(pwd->pw_name);
    }
#else
  struct passwd *pwd=getpwuid(uid);
  if(pwd){
    return FXString(pwd->pw_name);
    }
#endif
#endif
  __snprintf(result,sizeof(result),"%u",uid);
  return FXString(result);
  }


// Return group name from gid
FXString FXSystem::groupName(FXuint gid){
  FXchar result[64];
#if !defined(WIN32)
#ifdef HAVE_GETGRGID_R
  ::group grpresult;
  ::group *grp;
  char buffer[1024];
  if(getgrgid_r(gid,&grpresult,buffer,sizeof(buffer),&grp)==0 && grp){
    return FXString(grp->gr_name);
    }
#else
  ::group *grp=getgrgid(gid);
  if(grp){
    return FXString(grp->gr_name);
    }
#endif
#endif
  __snprintf(result,sizeof(result),"%u",gid);
  return FXString(result);
  }


// Get current user name
FXString FXSystem::currentUserName(){
#if defined(WIN32)
  TCHAR buffer[MAXPATHLEN];
  DWORD size=MAXPATHLEN;
  if(GetUserName(buffer,&size)){
    return FXString(buffer);
    }
#elif defined(HAVE_GETPWUID_R)
  struct passwd pwdresult,*pwd;
  char buffer[1024];
  if(getpwuid_r(geteuid(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
    return FXString(pwd->pw_name);
    }
#else
  struct passwd *pwd=getpwuid(geteuid());
  if(pwd){
    return FXString(pwd->pw_name);
    }
#endif
  return FXString::null;
  }


// Get current effective group name
FXString FXSystem::currentGroupName(){
#if !defined(WIN32)
#ifdef HAVE_GETGRGID_R
  ::group grpresult;
  ::group *grp;
  char buffer[1024];
  if(getgrgid_r(getegid(),&grpresult,buffer,sizeof(buffer),&grp)==0 && grp){
    return FXString(grp->gr_name);
    }
#else
  ::group *grp=getgrgid(getegid());
  if(grp){
    return FXString(grp->gr_name);
    }
#endif
#endif
  return FXString::null;
  }


// Return permissions string
FXString FXSystem::modeString(FXuint mode){
  FXchar result[11];
  result[0]=(mode&FXIO::SymLink) ? 'l' : (mode&FXIO::File) ? '-' : (mode&FXIO::Directory) ? 'd' : (mode&FXIO::Character) ? 'c' : (mode&FXIO::Block) ? 'b' : (mode&FXIO::Fifo) ? 'p' : (mode&FXIO::Socket) ? 's' : '?';
  result[1]=(mode&FXIO::OwnerRead) ? 'r' : '-';
  result[2]=(mode&FXIO::OwnerWrite) ? 'w' : '-';
  result[3]=(mode&FXIO::SetUser) ? 's' : (mode&FXIO::OwnerExec) ? 'x' : '-';
  result[4]=(mode&FXIO::GroupRead) ? 'r' : '-';
  result[5]=(mode&FXIO::GroupWrite) ? 'w' : '-';
  result[6]=(mode&FXIO::SetGroup) ? 's' : (mode&FXIO::GroupExec) ? 'x' : '-';
  result[7]=(mode&FXIO::OtherRead) ? 'r' : '-';
  result[8]=(mode&FXIO::OtherWrite) ? 'w' : '-';
  result[9]=(mode&FXIO::Sticky) ? 't' : (mode&FXIO::OtherExec) ? 'x' : '-';
  result[10]=0;
  return FXString(result);
  }


// Return value of environment variable name
FXString FXSystem::getEnvironment(const FXString& name){
  if(!name.empty()){
#if defined(WIN32)
#ifdef UNICODE
    // Windows limits total process environment space to 32767 bytes
    FXnchar variable[256],buffer[32767]; DWORD len;
    utf2ncs(variable,name.text(),ARRAYNUMBER(variable));
    if((len=GetEnvironmentVariableW(variable,buffer,ARRAYNUMBER(buffer)))>0){
      return FXString(buffer,len);
      }
#else
    // Windows limits total process environment space to 32767 bytes
    FXchar buffer[32767]; DWORD len;
    if((len=GetEnvironmentVariableA(name.text(),buffer,ARRAYNUMBER(buffer)))>0){
      return FXString(buffer,len);
      }
#endif
#else
    const FXchar* value=getenv(name.text());
    if(value){
      return FXString(value);
      }
#endif
    }
  return FXString::null;
  }


// Change value of environment variable name
FXbool FXSystem::setEnvironment(const FXString& name,const FXString& value){
  if(!name.empty()){
#if defined(WIN32)
#if defined(UNICODE)
    // Windows limits total process environment space to 32767 bytes
    FXnchar variable[256],buffer[32767];
    utf2ncs(variable,name.text(),ARRAYNUMBER(variable));
    if(!value.empty()){
      utf2ncs(buffer,value.text(),ARRAYNUMBER(buffer));
      return SetEnvironmentVariableW(variable,buffer)!=0;
      }
    return SetEnvironmentVariableW(variable,nullptr)!=0;
#else
    if(!value.empty()){
      return SetEnvironmentVariableA(name.text(),value.text())!=0;
      }
    return SetEnvironmentVariableA(name.text(),nullptr)!=0;
#endif
#elif defined(__GNU_LIBRARY__)
    if(!value.empty()){
      return setenv(name.text(),value.text(),true)==0;
      }
    unsetenv(name.text());
    return true;
#endif
    }
  return false;
  }


// Get current working directory
FXString FXSystem::getCurrentDirectory(){
#if defined(WIN32)
  TCHAR buffer[MAXPATHLEN];
  if(GetCurrentDirectory(MAXPATHLEN,buffer)){
    return FXString(buffer);
    }
#else
  FXchar buffer[MAXPATHLEN];
  if(getcwd(buffer,MAXPATHLEN)){
    return FXString(buffer);
    }
#endif
  return FXString::null;
  }


// Change current directory
FXbool FXSystem::setCurrentDirectory(const FXString& path){
  if(!path.empty()){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar unipath[MAXPATHLEN];
    utf2ncs(unipath,path.text(),MAXPATHLEN);
    return SetCurrentDirectory(unipath)!=0;
#else
    return SetCurrentDirectory(path.text())!=0;
#endif
#else
    return chdir(path.text())==0;
#endif
    }
  return false;
  }


// Get current drive prefix "a:", if any
// This is the same method as used in VC++ CRT.
FXString FXSystem::getCurrentDrive(){
#if defined(WIN32)
  FXchar buffer[MAXPATHLEN];
  if(GetCurrentDirectoryA(MAXPATHLEN,buffer) && Ascii::isLetter((FXuchar)buffer[0]) && buffer[1]==':'){
    return FXString(buffer,2);
    }
#endif
  return FXString::null;
  }


#if defined(WIN32)

// Change current drive prefix "a:"
// This is the same method as used in VC++ CRT.
FXbool FXSystem::setCurrentDrive(const FXString& prefix){
  FXchar buffer[3];
  if(!prefix.empty() && Ascii::isLetter(prefix[0]) && prefix[1]==':'){
    buffer[0]=prefix[0];
    buffer[1]=':';
    buffer[2]='\0';
    return SetCurrentDirectoryA(buffer)!=0;
    }
  return false;
  }

#else

// Change current drive prefix "a:"
FXbool FXSystem::setCurrentDrive(const FXString&){
  return true;
  }

#endif


// Get executable path
FXString FXSystem::getExecPath(){
  return FXSystem::getEnvironment("PATH");
  }


// Return known executable file extensions (Windows)
// Default list is:
//
//   .COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC
//
FXString FXSystem::getExecExtensions(){
#if defined(WIN32)
  return FXSystem::getEnvironment("PATHEXT");
#else
  return FXString::null;
#endif
  }


// Get name of calling executable
FXString FXSystem::getExecFilename(){
#if defined(WIN32)
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetModuleFileName(nullptr,buffer,MAXPATHLEN);
  return FXString(buffer,len);
#elif defined(__linux__)
  FXint pid=FXProcess::current();
  FXString filename=FXString::value("/proc/%d/exe",pid);
  return FXFile::symlink(filename);
#else
  return FXString::null;
#endif
  }


// Return the home directory for the current user.
FXString FXSystem::getHomeDirectory(){
  return FXSystem::getUserDirectory(FXString::null);
  }


// Return temporary directory.
FXString FXSystem::getTempDirectory(){
#if defined(WIN32)
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetTempPath(MAXPATHLEN,buffer);
  if(1<len && ISPATHSEP(buffer[len-1]) && !ISPATHSEP(buffer[len-2])) len--;
  return FXString(buffer,len);
#else
  const FXchar* dir;
  if((dir=getenv("TMPDIR"))!=nullptr){
    return FXString(dir);
    }
  return FXString("/tmp");
#endif
  }


// Return system directory
FXString FXSystem::getSystemDirectory(){
#if defined(WIN32)
  TCHAR buffer[MAXPATHLEN];
  DWORD len=GetSystemDirectory(buffer,MAXPATHLEN);
  return FXString(buffer,len);
#else
  return FXString("/bin");
#endif
  }


/*
BOOL WINAPI LookupAccountName(
  _In_opt_   LPCTSTR lpSystemName,
  _In_       LPCTSTR lpAccountName,
  _Out_opt_  PSID Sid,
  _Inout_    LPDWORD cbSid,
  _Out_opt_  LPTSTR ReferencedDomainName,
  _Inout_    LPDWORD cchReferencedDomainName,
  _Out_      PSID_NAME_USE peUse
);

PSID GetUserSID(const FXString& name){
  FXString user=name.empty()?currentUserName():name;
  if(!user.empty()){
    PSID   sid=nullptr;
    DWORD  sid_size=0;
    TCHAR *domain=nullptr;
    DWORD  domain_size=0;
    SID_NAME_USE use=SidTypeUnknown;

    // First call to LookupAccountName to get the buffer sizes
    if(LookupAccountNameA(nullptr,user.text(),sid,&sid_size,domain,&domain_size,&use)){

      // Allocate
      sid=(PSID)LocalAlloc(LMEM_FIXED,sid_size);
      domain=new TCHAR [domain_size];

      // Second call to LookupAccountName to get the actual account info
      if(LookupAccountName(nullptr,user.text(),sid,&sid_size,domain,&domain_size,&use)){
        delete [] domain;
        return sid;
        }

      // Free
      LocalFree(sid);
      delete [] domain;
      }
    }
  return nullptr;
  }
*/

// Get home directory for a given user
FXString FXSystem::getUserDirectory(const FXString& user){
#if defined(WIN32)
#ifdef UNICODE
  if(user.empty()){
    FXnchar path[4096];
    FXnchar drive[256];
    DWORD pathlen;
    DWORD drivelen;
    HKEY hKey;
    LONG result;
    if((pathlen=GetEnvironmentVariableW(L"USERPROFILE",path,ARRAYNUMBER(path)))>0){
      return FXString(path,pathlen);
      }
    if((pathlen=GetEnvironmentVariableW(L"HOME",path,ARRAYNUMBER(path)))>0){
      return FXString(path,pathlen);
      }
    if((pathlen=GetEnvironmentVariableW(L"HOMEPATH",path,ARRAYNUMBER(path)))>0){        // This should be good for WinNT, Win2K according to MSDN
      FXString result("C:");
      if((drivelen=GetEnvironmentVariableW(L"HOMEDRIVE",drive,ARRAYNUMBER(drive)))>0){
        result.assign(drive,drivelen);
        }
      result.append(path,pathlen);
      return result;
      }
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&hKey)==ERROR_SUCCESS){
      result=RegQueryValueExW(hKey,L"Personal",nullptr,nullptr,(LPBYTE)path,&pathlen);        // Change "Personal" to "Desktop" if you want...
      RegCloseKey(hKey);
      if(result==ERROR_SUCCESS){
        return FXString(path,pathlen);
        }
      }
    }
  return FXString::null;
#else
  if(user.empty()){
    FXchar path[4096];
    FXchar drive[256];
    DWORD pathlen;
    DWORD drivelen;
    HKEY hKey;
    LONG result;
    if((pathlen=GetEnvironmentVariableA("USERPROFILE",path,ARRAYNUMBER(path)))>0){
      return FXString(path,pathlen);
      }
    if((pathlen=GetEnvironmentVariableA("HOME",path,ARRAYNUMBER(path)))>0){
      return FXString(path,pathlen);
      }
    if((pathlen=GetEnvironmentVariableA("HOMEPATH",path,ARRAYNUMBER(path)))>0){         // This should be good for WinNT, Win2K according to MSDN
      FXString result("C:");
      if((drivelen=GetEnvironmentVariableA("HOMEDRIVE",drive,ARRAYNUMBER(drive)))>0){
        result.assign(drive,drivelen);
        }
      result.append(path,pathlen);
      return result;
      }
    if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&hKey)==ERROR_SUCCESS){
      result=RegQueryValueExA(hKey,"Personal",nullptr,nullptr,(LPBYTE)path,&pathlen);         // Change "Personal" to "Desktop" if you want...
      RegCloseKey(hKey);
      if(result==ERROR_SUCCESS){
        return FXString(path,pathlen);
        }
      }
    }
  return FXString::null;
#endif
#elif defined(HAVE_GETPWNAM_R)
  struct passwd pwdresult,*pwd;
  const FXchar* str;
  char buffer[1024];
  if(user.empty()){
    if((str=getenv("HOME"))!=nullptr){
      return FXString(str);
      }
    if((str=getenv("USER"))!=nullptr || (str=getenv("LOGNAME"))!=nullptr){
      if(getpwnam_r(str,&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
        return FXString(pwd->pw_dir);
        }
      }
    if(getpwuid_r(getuid(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
      return FXString(pwd->pw_dir);
      }
    return FXString::null;
    }
  if(getpwnam_r(user.text(),&pwdresult,buffer,sizeof(buffer),&pwd)==0 && pwd){
    return FXString(pwd->pw_dir);
    }
  return FXString::null;
#else
  struct passwd *pwd;
  const FXchar* str;
  if(user.empty()){
    if((str=getenv("HOME"))!=nullptr){
      return FXString(str);
      }
    if((str=getenv("USER"))!=nullptr || (str=getenv("LOGNAME"))!=nullptr){
      if((pwd=getpwnam(str))!=nullptr){
        return FXString(pwd->pw_dir);
        }
      }
    if((pwd=getpwuid(getuid()))!=nullptr){
      return FXString(pwd->pw_dir);
      }
    return FXString::null;
    }
  if((pwd=getpwnam(user.text()))!=nullptr){
    return FXString(pwd->pw_dir);
    }
  return FXString::null;
#endif
  }


// Return host name
FXString FXSystem::getHostName(){
  FXchar name[MAXHOSTNAMELEN];
  if(gethostname(name,sizeof(name))==0){
    name[MAXHOSTNAMELEN-1]='\0';
    return FXString(name);
    }
  return "localhost";
  }


// Determine if UTF8 locale in effect
FXbool FXSystem::localeIsUTF8(){
#if defined(WIN32)
  return GetACP()==CP_UTF8;
#else
  const FXchar* str;
  if((str=getenv("LC_CTYPE"))!=nullptr || (str=getenv("LC_ALL"))!=nullptr || (str=getenv("LANG"))!=nullptr){
    return (strstr(str,"utf")!=nullptr || strstr(str,"UTF")!=nullptr);
    }
  return false;
#endif
  }


// Get DLL name for given base name
FXString FXSystem::dllName(const FXString& name){
#if defined(WIN32)
  return name+".dll";
#elif defined(_HPUX_) || defined(_HPUX_SOURCE)
  return "lib"+name+".sl";
#elif defined(__APPLE__)
  return "lib"+name+".dylib";
#else
  return "lib"+name+".so";
#endif
  }

}

