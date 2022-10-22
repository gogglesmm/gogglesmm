/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXProcess.h"

/*
  Notes:
  - On Windows, if custom environment is passed, we must merge some stock environment
    variables in with the custom ones.

    A tentative list (please let me know if this is a complete list):

        Variable:               Typical value:
        =========               =============================================
        PATH                    C:\Windows\System32\;C:\Windows\;C:\Windows\System32
        PATHEXT                 .COM; .EXE; .BAT; .CMD; .VBS; .VBE; .JS ; .WSF; .WSH
        COMSPEC                 C:\Windows\System32\cmd.exe
        SYSTEMDRIVE             C:
        SYSTEMROOT              C:\Windows
        OS                      Windows_NT
        TMP                     C:\DOCUME~1\{username}\LOCALS~1\TEMP
        TEMP                    C:\DOCUME~1\{username}\LOCALS~1\TEMP
        HOMEDRIVE               C:
        HOMEPATH                \Documents and Settings\{username}
        PROGRAMFILES            C:\Program Files
        COMPUTERNAME            {computername}
        USERNAME                {username}
        USERDOMAIN              {servername}
        USERPROFILE             C:\Documents and Settings\{username}
        ALLUSERSPROFILE         C:\Documents and Settings\All Users
        COMMONPROGRAMFILES      C:\Program Files\Common Files
        NUMBER_OF_PROCESSORS    1
        PROCESSOR_ARCHITECTURE  x86
        PROCESSOR_IDENTIFIER    {name}
        PROCESSOR_LEVEL         {stepping}
        PROCESSOR_REVISION      {flags}
        WINDIR                  C:\Windows
        LOGONSERVER             \\{servername}

    These should be copied from the parent process environment settings.

  - On Windows, no file descriptors are inherited from the parent process.  We do pass
    stdin, stdout, stderr.

  - The following reference was used to achieve proper enquoting of commandline parameters
    to pass along to CreateProcess():

        http://www.daviddeley.com/autohotkey/parameters/parameters.htm

  - On *NIX, we close all file descriptors except the stdin, stdout, stderr.

  - The Microsoft C/C++ Parameter Parsing Rules Rephrased:

     o Parameters are always separated by a space or tab (multiple spaces/tabs OK).

     o If the parameter does not contain any spaces, tabs, or double quotes, then all the characters
       in the parameter are accepted as is (there is no need to enclose the parameter in double quotes).

     o Enclose spaces and tabs in a double quoted part.

     o A double quoted part can be anywhere within a parameter.

     o 2n backslashes followed by a " produce n backslashes + start/end double quoted part.

     o 2n+1 backslashes followed by a " produce n backslashes + a literal quotation mark
       n backslashes not followed by a quotation mark produce n backslashes.

     o undocumented rules regarding double quotes:

       Prior to 2008:
         A " outside a double quoted block starts a double quoted block.
         A " inside a double quoted block ends the double quoted block.
         If a closing " is followed immediately by another ", the 2nd " is accepted literally and
         added to the parameter.

       Post 2008:
         Outside a double quoted block a " starts a double quoted block.
         Inside a double quoted block a " followed by a different character (not another ") ends the
         double quoted block.
         Inside a double quoted block a " followed immediately by another " (i.e. "") causes a
         single " to be added to the output, and the double quoted block continues.

     o Thus:

        Use "    to start/end a double quoted part
        Use \"   to insert a literal "
        Use \\"  to insert a \ then start or end a double quoted part
        Use \\\" to insert a literal \"
        Use \    to insert a literal \

  - On Linux, consider using posix_spawn() instead of the old fork()/exec() pair.
    This means setting up some posix_spawn_file_actions and posix_spawnattr to
    deal with file descriptors, signal masks, process groups, etc.
    Need POSIX >= 2008 for this.

  - Maybe pidfd_open() is useful to obtain waitable handle to process.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {


// Initialize process
FXProcess::FXProcess():pid(0),input(nullptr),output(nullptr),errors(nullptr){
  FXTRACE((100,"FXProcess::FXProcess\n"));
  }


// Return handle of this process object
FXProcessID FXProcess::id() const {
  return pid;
  }

#if defined(WIN32)

// Extra variables
static const FXchar* extravars[]={
  "PATH",
  "PATHEXT",
  "COMSPEC",
  "SYSTEMDRIVE",
  "SYSTEMROOT",
  "OS",
  "TMP",
  "TEMP",
  "HOMEDRIVE",
  "HOMEPATH",
  "PROGRAMFILES",
  "COMPUTERNAME",
  "USERNAME",
  "USERDOMAIN",
  "USERPROFILE",
  "ALLUSERSPROFILE",
  "COMMONPROGRAMFILES",
  "NUMBER_OF_PROCESSORS",
  "PROCESSOR_ARCHITECTURE",
  "PROCESSOR_IDENTIFIER",
  "PROCESSOR_LEVEL",
  "PROCESSOR_REVISION",
  "WINDIR",
  "LOGONSERVER"
  };


// Sort environment variables
static int CDECL comparison(const void *a1, const void *a2){
  return FXString::compare(*((const FXchar**)a1),*((const FXchar**)a2));
  }


// See if quotes are needed
static FXbool needquotes(const FXchar* ptr){
  FXchar c;
  while((c=*ptr++)!='\0'){
    if(c==' ' || c=='"' || c=='\t' || c=='\v' || c=='\n') return true;
    }
  return false;
  }

#ifdef UNICODE


// Build command line for windows
static FXnchar* commandline(const FXchar *const *args){
  FXnchar *result=nullptr;
  if(args){
    FXint size,s,n,w;
    const FXchar  *ptr;
    FXnchar       *dst;
    FXbool         q;
    for(size=s=0; (ptr=args[s])!=nullptr; ++s){
      q=needquotes(ptr);
      if(q) size+=2;
      n=0;
      while(1){
        w=wc(ptr);
        if(w=='\0'){
          if(q){ size+=n; }
          break;
          }
        else if(w=='\\'){
          size++;
          n++;
          }
        else if(w=='"'){
          size+=n+2;
          n=0;
          }
        else{
          size+=wc2nc(w);
          n=0;
          }
        ptr=wcinc(ptr);
        }
      size++;
      }
    if(allocElms(result,size+1)){
      for(dst=result,s=0; (ptr=args[s])!=nullptr; ++s){
        q=needquotes(ptr);
        if(q) *dst++='"';
        n=0;
        while(1){
          w=wc(ptr);
          if(w=='\0'){
            if(q){ while(--n>=0){ *dst++='\\'; } }
            break;
            }
          else if(w=='\\'){
            *dst++='\\';
            n++;
            }
          else if(w=='"'){
            while(--n>=0){ *dst++='\\'; }
            *dst++='\\';
            *dst++='"';
            n=0;
            }
          else{
            dst+=wc2nc(dst,w);
            n=0;
            }
          ptr=wcinc(ptr);
          }
        if(q) *dst++='"';
        *dst++=' ';
        }
      *dst='\0';
      }
    }
  return result;
  }


// Make windows environment block
static FXnchar* enviroblock(const FXchar *const *env){
  FXnchar* result=nullptr;
  if(env){
    const FXchar **tmp;
    FXint size=0,n=0,s,d;
    while(env[n]) n++;
    if(callocElms(tmp,n+1)){
      for(s=0; s<n; ++s){
        tmp[s]=env[s];
        size+=utf2ncs(env[s])+1;
        }
      qsort((void*)tmp,(size_t)s,sizeof(const FXchar*),comparison);
      if(allocElms(result,size+2)){
        for(s=d=0; s<n; ++s){
          d+=utf2ncs(&result[d],tmp[s],size-d)+1;
          }
        result[d+0]=0;
        result[d+1]=0;
        }
      FXASSERT(d<=size+2);
      freeElms(tmp);
      }
    }
  return result;
  }

#else

// Build command line for windows
static FXchar* commandline(const FXchar *const *args){
  FXchar *result=nullptr;
  if(args){
    FXint size,s,n,w;
    const FXchar  *ptr;
    FXchar        *dst;
    FXbool         q;
    for(size=s=0; (ptr=args[s])!=nullptr; ++s){
      q=needquotes(ptr);
      if(q) size+=2;
      n=0;
      while(1){
        w=*ptr++;
        if(w=='\0'){
          if(q){ size+=n; }
          break;
          }
        else if(w=='\\'){
          size++;
          n++;
          }
        else if(w=='"'){
          size+=n+2;
          n=0;
          }
        else{
          size++;
          n=0;
          }
        }
      size++;
      }
    if(allocElms(result,size+1)){
      for(dst=result,s=0; (ptr=args[s])!=nullptr; ++s){
        q=needquotes(ptr);
        if(q) *dst++='"';
        n=0;
        while(1){
          w=*ptr++;
          if(w=='\0'){
            if(q){ while(--n>=0){ *dst++='\\'; } }
            break;
            }
          else if(w=='\\'){
            *dst++='\\';
            n++;
            }
          else if(w=='"'){
            while(--n>=0){ *dst++='\\'; }
            *dst++='\\';
            *dst++='"';
            n=0;
            }
          else{
            *dst++=w;
            n=0;
            }
          }
        if(q) *dst++='"';
        *dst++=' ';
        }
      *dst='\0';
      }
    }
  return result;
  }


// Make windows environment block
static FXchar* enviroblock(const FXchar *const *env){
  FXchar* result=nullptr;
  if(env){
    const FXchar **tmp;
    FXint size=0,n=0,s,d;
    while(env[n]) n++;
    if(callocElms(tmp,n+1)){
      for(s=0; s<n; ++s){
        tmp[s]=env[s];
        size+=strlen(env[s])+1;
        }
      qsort((void*)tmp,(size_t)s,sizeof(const FXchar*),comparison);
      if(allocElms(result,size+2)){
        for(s=d=0; s<n; ++s){
          d+=strlen(strncpy(&result[d],tmp[s],size-d))+1;
          }
        result[d+0]=0;
        result[d+1]=0;
        }
      FXASSERT(d<=size+2);
      freeElms(tmp);
      }
    }
  return result;
  }

#endif

#endif

// Start subprocess
FXbool FXProcess::start(const FXchar* exec,const FXchar *const *args,const FXchar *const *env){
  FXbool result=false;
  if(pid==0 && exec && args){
#if defined(WIN32)
#if defined(UNICODE)
    FXnchar uniexec[MAXPATHLEN];
    utf2ncs(uniexec,exec,MAXPATHLEN);
    if(::GetFileAttributesW(uniexec)!=INVALID_FILE_ATTRIBUTES){
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      memset(&pi,0,sizeof(pi));
      memset(&si,0,sizeof(si));

      // Init startup info
      si.cb=sizeof(si);
      si.dwFlags=STARTF_USESTDHANDLES;

      // Stdin was redirected
      si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
      if(input && input->isOpen()){
        si.hStdInput=input->handle();
        }

      // Stdout was redirected
      si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
      if(output && output->isOpen()){
        si.hStdOutput=output->handle();
        }

      // Stderr was redirected
      si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
      if(errors && errors->isOpen()){
        si.hStdError=errors->handle();
        }

      // Build wide-character command line
      FXnchar *command=commandline(args);

      // Build wide-character environment block
      FXnchar *envir=enviroblock(env);

      // Create process
      if(CreateProcessW(uniexec,command,nullptr,nullptr,true,CREATE_UNICODE_ENVIRONMENT,envir,nullptr,&si,&pi)){
        CloseHandle(pi.hThread);
        pid=pi.hProcess;
        result=true;
        }

      // Free command line and environment block
      freeElms(envir);
      freeElms(command);
      }
#else
    if(::GetFileAttributesA(exec)!=INVALID_FILE_ATTRIBUTES){
      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Zero out process info and startup info
      memset(&pi,0,sizeof(pi));
      memset(&si,0,sizeof(si));

      // Init startup info
      si.cb=sizeof(si);
      si.dwFlags=STARTF_USESTDHANDLES;

      // Stdin was redirected
      si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
      if(input && input->isOpen()){
        si.hStdInput=input->handle();
        }

      // Stdout was redirected
      si.hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
      if(output && output->isOpen()){
        si.hStdOutput=output->handle();
        }

      // Stderr was redirected
      si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
      if(errors && errors->isOpen()){
        si.hStdError=output->handle();
        }

      // Build command line
      FXchar *command=commandline(args);

      // Build environment block
      FXchar *envir=enviroblock(env);

      // Create process
      if(CreateProcessA(exec,command,nullptr,nullptr,true,0,envir,nullptr,&si,&pi)){
        CloseHandle(pi.hThread);
        pid=pi.hProcess;
        result=true;
        }

      // Free command line and environment block
      freeElms(envir);
      freeElms(command);
      }
#endif
#else
    struct stat status;
    if(::stat(exec,&status)==0){
      FXProcessID hnd=::fork();
      if(0<=hnd){
        if(hnd==0){

          // Stdin was redirected
          if(input && input->isOpen()){
            dup2(input->handle(),STDIN_FILENO);
            }

          // Stdout was redirected
          if(output && output->isOpen()){
            dup2(output->handle(),STDOUT_FILENO);
            }

          // Stderr was redirected
          if(errors && errors->isOpen()){
            dup2(errors->handle(),STDERR_FILENO);
            }

          // Close all other handles
          FXint fd=::sysconf(_SC_OPEN_MAX);
          while(--fd>STDERR_FILENO){
            close(fd);
            }

          //setsid();

          // Kick off with arguments and environment
          if(env){
            ::execve(exec,const_cast<char* const*>(args),const_cast<char* const*>(env));
            }

          // Kick off with just arguments
          else{
            ::execv(exec,const_cast<char* const*>(args));
            }

          // Failed to kick off child
          fxwarning("failed to exec: %s\n",exec);

          // Child exits
          FXProcess::exit(-1);
          }
        pid=hnd;
        result=true;
        }
      }
#endif
    }
  return result;
  }


// Get process id
FXint FXProcess::current(){
#if defined(WIN32)
  return (FXint)::GetCurrentProcessId();
#else
  return (FXint)::getpid();
#endif
  }


// Exit calling process
void FXProcess::exit(FXint code){
#if defined(WIN32)
  ::ExitProcess(code);
#else
  ::exit(code);
#endif
  }


// Suspend process
FXbool FXProcess::suspend(){
  if(pid){
#if defined(WIN32)
/*
    HANDLE hThreadSnap=nullptr;
    HANDLE hThreadSnap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap!=INVALID_HANDLE_VALUE){
      THREADENTRY32  te32={0};
      te32.dwSize=sizeof(THREADENTRY32);
      if(Thread32First(hThreadSnap,&te32)){
        do{
          if(te32.th32OwnerProcessID==pid){
            HANDLE hThread=OpenThread(THREAD_SUSPEND_RESUME,false,te32.th32ThreadID);
            if(hThread){
              ResumeThread(hThread);
              CloseHandle(hThread);
              }
            }
          }
        while(Thread32Next(hThreadSnap,&te32));
        }
      CloseHandle(hThreadSnap);
      }
*/
#else
    return ::kill(pid,SIGSTOP)==0;
#endif
    }
  return false;
  }


// Resume process
FXbool FXProcess::resume(){
  if(pid){
#if defined(WIN32)
/*
    HANDLE hThreadSnap=nullptr;
    HANDLE hThreadSnap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap!=INVALID_HANDLE_VALUE){
      THREADENTRY32  te32={0};
      te32.dwSize=sizeof(THREADENTRY32);
      if(Thread32First(hThreadSnap,&te32)){
        do{
          if(te32.th32OwnerProcessID==pid){
            HANDLE hThread=OpenThread(THREAD_SUSPEND_RESUME,false,te32.th32ThreadID);
            if(hThread){
              SuspendThread(hThread);
              CloseHandle(hThread);
              }
            }
          }
        while(Thread32Next(hThreadSnap,&te32));
        }
      CloseHandle(hThreadSnap);
      }
*/
#else
    return ::kill(pid,SIGCONT)==0;
#endif
    }
  return false;
  }


// Kill process
FXbool FXProcess::kill(){
  if(pid){
#if defined(WIN32)
    return ::TerminateProcess((HANDLE)pid,-1)!=0;
#else
    return ::kill(pid,SIGKILL)==0;
#endif
    }
  return false;
  }


// Wait for child process
FXbool FXProcess::wait(){
  FXbool result=false;
  if(pid){
#if defined(WIN32)
    if(::WaitForSingleObject((HANDLE)pid,INFINITE)==WAIT_OBJECT_0){
      ::CloseHandle((HANDLE)pid);
      result=true;
      pid=0;
      }
#else
    FXint code;
    if(0<waitpid(pid,&code,0)){
      result=true;
      pid=0;
      }
#endif
    }
  return result;
  }


// Wait for child process, returning exit code
FXbool FXProcess::wait(FXint& code){
  FXbool result=false;
  if(pid){
#if defined(WIN32)
    if(::WaitForSingleObject((HANDLE)pid,INFINITE)==WAIT_OBJECT_0){
      ::GetExitCodeProcess((HANDLE)pid,(ULONG*)&code);
      ::CloseHandle((HANDLE)pid);
      result=true;
      pid=0;
      }
#else
    if(0<waitpid(pid,&code,0)){
      result=true;
      pid=0;
      }
#endif
    }
  return result;
  }



// Delete
FXProcess::~FXProcess(){
  FXTRACE((100,"FXProcess::~FXProcess\n"));
  if(pid){
#if defined(WIN32)
    ::CloseHandle((HANDLE)pid);
#else
    //// Zombie ////
#endif
    }
  }


}
