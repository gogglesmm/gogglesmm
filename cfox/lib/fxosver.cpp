/********************************************************************************
*                                                                               *
*                O p e r a t i n g   S y s t e m   V e r s i o n                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2022 by Jeroen van der Zijp.   All Rights Reserved.             *
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


/*
  Notes:
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);

#if defined(WIN32)

// RtlGetVersion grabs the version information use it because
// GetVersionEx is now deprecated.
typedef LONG (WINAPI *PFN_RTLGETVERSION)(OSVERSIONINFOEXW*);

// Declare the stub function
static LONG WINAPI RtlGetVersionStub(OSVERSIONINFOEXW* osv);

// Pointer to RtlGetVersion, initially pointing to the stub function
static PFN_RTLGETVERSION fxRtlGetVersion=RtlGetVersionStub;

// Low-level routine to grab OS version
static LONG WINAPI RtlGetVersionStub(OSVERSIONINFOEXW* osv){
  if(fxRtlGetVersion==RtlGetVersionStub){
    HMODULE ntdllDll=GetModuleHandleA("ntdll.dll");
    FXASSERT(ntdllDll);
    fxRtlGetVersion=(PFN_RTLGETVERSION)GetProcAddress(ntdllDll,"RtlGetVersion");
    FXASSERT(fxRtlGetVersion);
    }
  return fxRtlGetVersion(osv);
  }


// Get operating system version string
FXival fxosversion(FXchar version[],FXival len){
  if(0<len){
    OSVERSIONINFOEXW osv;
    osv.dwOSVersionInfoSize=sizeof(osv);
    if(fxRtlGetVersion(&osv)==0){
      FXTRACE((100,"fxosversion: dwMajorVersion=%d\n",osv.dwMajorVersion));
      FXTRACE((100,"fxosversion: dwMinorVersion=%d\n",osv.dwMinorVersion));
      FXTRACE((100,"fxosversion: dwBuildNumber=%d\n",osv.dwBuildNumber));
      FXTRACE((100,"fxosversion: dwPlatformId=%d\n",osv.dwPlatformId));
      FXTRACE((100,"fxosversion: wServicePackMajor=%d\n",osv.wServicePackMajor));
      FXTRACE((100,"fxosversion: wServicePackMinor=%d\n",osv.wServicePackMinor));
      FXTRACE((100,"fxosversion: wSuiteMask=%d\n",osv.wSuiteMask));
      FXTRACE((100,"fxosversion: wProductType=%d\n",osv.wProductType));
      return __snprintf(version,len,"%d.%d.%d.%d",osv.dwMajorVersion,osv.dwMinorVersion,osv.dwBuildNumber,osv.dwPlatformId);
      }
    version[0]='\0';
    }
  return 0;
  }


#if 0

FXival fxosversion(FXchar version[],FXival len){
  if(0<len){
    TCHAR filename[MAXPATHLEN];
    DWORD n=GetSystemDirectoryW(filename,MAXPATHLEN);
    if(0<n){
      DWORD handle;
      wcscpy(filename+n,L"\\kernel32.dll");
      if(GetFileVersionInfoSizeW(filename,&handle)){
        VS_FIXEDFILEINFO ffi;
        if(GetFileVersionInfoW(filename,handle,size,data)){



VERSIONHELPERAPI IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor){
  OSVERSIONINFOEXW osvi;
  DWORDLONG dwlConditionMask = VerSetConditionMask(VerSetConditionMask(VerSetConditionMask(0,VER_MAJORVERSION,VER_GREATER_EQUAL),VER_MINORVERSION,VER_GREATER_EQUAL),VER_SERVICEPACKMAJOR,VER_GREATER_EQUAL);
  osvi.dwOSVersionInfoSize=sizeof(osvi);
  osvi.dwMajorVersion=wMajorVersion;
  osvi.dwMinorVersion=wMinorVersion;
  osvi.wServicePackMajor=wServicePackMajor;
  return VerifyVersionInfoW(&osvi,VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR,dwlConditionMask)!=FALSE;
  }





bool GetOSVersionString(WCHAR* version,size_t maxlen){
  WCHAR path[_MAX_PATH]={};

  if(!GetSystemDirectoryW(path,_MAX_PATH)) return false;

  wcscat_s( path, L"\\kernel32.dll" );

  //
  // Based on example code from this article
  // http://support.microsoft.com/kb/167597
  //

  DWORD handle;
#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
  DWORD len=GetFileVersionInfoSizeExW(FILE_VER_GET_NEUTRAL,path,&handle);
#else
  DWORD len=GetFileVersionInfoSizeW(path,&handle);
#endif
  if(!len) return false;

  std::unique_ptr<uint8_t> buff( new (std::nothrow) uint8_t[ len ] );

  if(!buff) return false;

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
  if(!GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL,path,0,len,buff.get() ) ) return false;
#else
  if(!GetFileVersionInfoW( path, 0, len, buff.get() ) ) return false;
#endif

  VS_FIXEDFILEINFO *vInfo=nullptr;
  UINT infoSize;

  if(!VerQueryValueW(buff.get(),L"\\",reinterpret_cast<LPVOID*>(&vInfo),&infoSize)) return false;

  if(!infoSize) return false;

  swprintf_s( version, maxlen, L"%u.%u.%u.%u",HIWORD( vInfo->dwFileVersionMS ),LOWORD(vInfo->dwFileVersionMS),HIWORD(vInfo->dwFileVersionLS),LOWORD(vInfo->dwFileVersionLS) );

  return true;
  }

#endif

#elif defined(HAVE_UNAME)

// Get operating system version string
FXival fxosversion(FXchar version[],FXival len){   // FIXME will change
  if(0<len){
    struct utsname buffer;
    if(uname(&buffer)==0){
      FXTRACE((100,"fxosversion: sysname=%s\n",buffer.sysname));
      FXTRACE((100,"fxosversion: nodename=%s\n",buffer.nodename));
      FXTRACE((100,"fxosversion: release=%s\n",buffer.release));
      FXTRACE((100,"fxosversion: version=%s\n",buffer.version));
      return fxstrlcpy(version,buffer.release,len);
      }
    //return __snprintf(version,len,"%d.%d.%d",LINUX_VERSION_MAJOR,LINUX_VERSION_PATCHLEVEL,LINUX_VERSION_SUBLEVEL);
    version[0]='\0';
    }
  return 0;
  }

#else

// Get operating system version string
FXival fxosversion(FXchar version[],FXival len){
  if(0<len){
    version[0]='\0';
    }
  return 0;
  }

#endif

}

