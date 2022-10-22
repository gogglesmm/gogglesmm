/********************************************************************************
*                                                                               *
*             D y n a m i c   L i n k   L i b r a r y   S u p p o r t           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXDLL.h"

/*
  Notes:

  - Make sure it works on other unices.

  - Get main executable handle like:

      GetOwnModuleHandle();

    or

      dlopen(nullptr,RTLD_NOW|RTLD_GLOBAL);

  - Nice thing for tracing:

      Dl_info dli;
      dladdr(__builtin_return_address(0), &dli);
      fprintf(stderr, "debug trace [%d]: %s called by %p [ %s(%p) %s(%p) ].\n",getpid(), __func__,__builtin_return_address(0),strrchr(dli.dli_fname, '/') ?strrchr(dli.dli_fname, '/')+1 : dli.dli_fname,dli.dli_fbase, dli.dli_sname, dli.dli_saddr);
      dladdr(__builtin_return_address(1), &dli);
      fprintf(stderr, "debug trace [%d]: %*s called by %p [ %s(%p) %s(%p) ].\n",getpid(), strlen(__func__), "...",__builtin_return_address(1),strrchr(dli.dli_fname, '/') ?strrchr(dli.dli_fname, '/')+1 : dli.dli_fname,dli.dli_fbase, dli.dli_sname, dli.dli_saddr);

  - Some machines have dlinfo(); you can get directory from where DLL comes:

      char directory[1024];
      if(dlinfo(hnd,RTLD_DI_ORIGIN,directory)!=-1){
        return directory;
        }

*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Return the name of the library module
FXString FXDLL::name() const {
  if(hnd){
#if defined(WIN32)              // WIN32
    char buffer[1024];
    if(GetModuleFileNameA((HINSTANCE)hnd,buffer,sizeof(buffer))){
      return FXString(buffer);
      }
#elif defined(HAVE_SHL_LOAD)    // HP-UX
    struct shl_descriptor desc;
    if(shl_gethandle_r((shl_t)hnd,&desc)!=-1){
      return FXString(desc.filename);
      }
#elif defined(__minix)          // MINIX
    //// NOT SUPPORTED ////
#else                           // POSIX
    Dl_info info;
    void *ptr=dlsym(hnd,"_init");       // FIXME any better way?
    if(ptr && dladdr(ptr,&info)){
      return FXString(info.dli_fname);
      }
#endif
    }
  return FXString::null;
  }


// Load the library module from the given name
FXbool FXDLL::load(const FXString& nm){
  if(!hnd && !nm.empty()){
#if defined(WIN32)              // WIN32
    // Order of loading with LoadLibrary (or LoadLibraryEx with no
    // LOAD_WITH_ALTERED_SEARCH_PATH flag):
    //
    // 1. Directory from which the application was loaded.
    // 2. Current directory.
    // 3. System directory, as determined by GetSystemDirectory().
    // 4. 16-bit system directory.
    // 5. Windows directory, as determined by GetWindowsDirectory().
    // 6. Directories in the $PATH.
    //
    // With flag LOAD_WITH_ALTERED_SEARCH_PATH:
    //
    // 1. Directory specified by the filename path.
    // 2. Current directory.
    // 3. System directory.
    // 4. 16-bit system directory.
    // 5. Windows directory.
    // 6. Directories in the $PATH.
    //
    // We switched to the latter so sub-modules needed by a DLL are
    // plucked from the same place as name (thanks to Rafael de
    // Pelegrini Soares" <Rafael@enq.ufrgs.br>).
    hnd=LoadLibraryExA(nm.text(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
#elif defined(HAVE_SHL_LOAD)    // HP-UX
    hnd=shl_load(nm.text(),BIND_IMMEDIATE|BIND_NONFATAL|DYNAMIC_PATH,0L);
#elif defined(__minix)          // MINIX
    //// NOT SUPPORTED ////
#else			        // POSIX
    hnd=dlopen(nm.text(),RTLD_NOW|RTLD_GLOBAL);
#endif
    }
  return hnd!=nullptr;
  }


// Unload the library module
void FXDLL::unload(){
  if(hnd){
#if defined(WIN32)              // WIN32
    FreeLibrary((HMODULE)hnd);
#elif defined(HAVE_SHL_LOAD)    // HP-UX
    shl_unload((shl_t)hnd);
#elif defined(__minix)          // MINIX
    //// NOT SUPPORTED ////
#else			        // POSIX
    dlclose(hnd);
#endif
    hnd=nullptr;
    }
  }


// Return the address of the symbol in this library module
void* FXDLL::address(const FXchar* sym) const {
  if(hnd && sym && sym[0]){
#if defined(WIN32)              // WIN32
    return (void*)GetProcAddress((HMODULE)hnd,sym);
#elif defined(HAVE_SHL_LOAD)    // HP-UX
    void* ptr=nullptr;
    if(shl_findsym((shl_t*)&hnd,sym,TYPE_UNDEFINED,&ptr)==0) return ptr;
#elif defined(__minix)          // MINIX
    //// NOT SUPPORTED ////
#else			        // POSIX
    return dlsym(hnd,sym);
#endif
    }
  return nullptr;
  }


// Return the address of the symbol in this library module
void* FXDLL::address(const FXString& sym) const {
  return address(sym.text());
  }


// Return the symbol name of the given address
FXString FXDLL::symbol(void *addr){
#if defined(WIN32)              // WIN32
  // FIXME //
#elif defined(HAVE_SHL_LOAD)    // HP-UX
  // FIXME //
#elif defined(__minix)          // MINIX
  //// NOT SUPPORTED ////
#else                           // POSIX
  Dl_info info;
  if(dladdr(addr,&info)){
    return FXString(info.dli_sname);
    }
#endif
  return FXString::null;
  }


// Return the name of the library module containing the address
FXString FXDLL::name(void *addr){
#if defined(WIN32)              // WIN32
  MEMORY_BASIC_INFORMATION mbi;
  if(VirtualQuery((const void*)addr,&mbi,sizeof(mbi))){
    char buffer[1024];
    if(GetModuleFileNameA((HINSTANCE)mbi.AllocationBase,buffer,sizeof(buffer))){
      return FXString(buffer);
      }
//  FXnchar buffer[1024];
//  if(GetModuleFileNameW((HINSTANCE)mbi.AllocationBase,buffer,sizeof(buffer))){
//    return FXString(buffer);
//    }
    }
#elif defined(HAVE_SHL_LOAD)    // HP-UX
  // FIXME //
#elif defined(__minix)          // MINIX
  //// NOT SUPPORTED ////
#else                           // POSIX
  Dl_info info;
  if(dladdr(addr,&info)){
    return FXString(info.dli_fname);
    }
#endif
  return FXString::null;
  }


// Find DLL containing symbol
FXDLL FXDLL::dll(void* addr){
#if defined(WIN32)              // WIN32
  MEMORY_BASIC_INFORMATION mbi;
  if(VirtualQuery((const void*)addr,&mbi,sizeof(mbi))){
    //FXTRACE((1,"BaseAddress       = %p\n",mbi.BaseAddress));
    //FXTRACE((1,"AllocationBase    = %p\n",mbi.AllocationBase));
    //FXTRACE((1,"AllocationProtect = 0x%x\n",mbi.AllocationProtect));
    //FXTRACE((1,"RegionSize        = %d\n",mbi.RegionSize));
    //FXTRACE((1,"State             = 0x%x\n",mbi.State));
    //FXTRACE((1,"Protect           = 0x%x\n",mbi.Protect));
    //FXTRACE((1,"Type              = 0x%x\n",mbi.Type));
    return FXDLL(mbi.AllocationBase);
    }
#elif defined(HAVE_SHL_LOAD)    // HP-UX
  // FIXME //
#elif defined(__minix)          // MINIX
  //// NOT SUPPORTED ////
#else                           // POSIX
  Dl_info info;
  if(dladdr(addr,&info)){
    //FXTRACE((1,"dli_fname = %s\n",info.dli_fname));
    //FXTRACE((1,"dli_fbase = %p\n",info.dli_fbase));
    //FXTRACE((1,"dli_sname = %s\n",info.dli_sname));
    //FXTRACE((1,"dli_saddr = %p\n",info.dli_saddr));
    return FXDLL(dlopen(info.dli_fname,RTLD_NOLOAD|RTLD_NOW|RTLD_GLOBAL));
    }
#endif
  return FXDLL(nullptr);
  }


// Find DLL of ourselves
FXDLL FXDLL::dll(){
  return dll((void*)FXDLL::error);
  }


// Return error message if error occurred loading the library module
FXString FXDLL::error(){
#if defined(WIN32)              // WIN32
  DWORD dw=GetLastError();
  FXchar buffer[512];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,nullptr,dw,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),(LPTSTR)buffer,sizeof(buffer),nullptr);
  return FXString(buffer);
#elif defined(HAVE_SHL_LOAD)    // HP-UX
  return FXString::null;
#elif defined(__minix)          // MINIX
  //// NOT SUPPORTED ////
#else			        // POSIX
  return FXString(dlerror());
#endif
  }


/*******************************************************************************/


// Initialize by loading given library name
FXAUTODLL::FXAUTODLL(const FXString& nm){
  load(nm);
  }


// Unload library if we have one
FXAUTODLL::~FXAUTODLL(){
  unload();
  }

}
