/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2012-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#include "gmdefs.h"
#include "GMSession.h"

namespace ap {
extern FXbool ap_set_closeonexec(FXInputHandle fd);
}

#include <xincs.h>
#include <X11/ICE/ICElib.h>
#include <X11/SM/SMlib.h>

class IceHook : public FXObject {
FXDECLARE(IceHook)
protected:
  FXApp * application = nullptr;
protected:

  static void IceError(IceConn,Bool,int,unsigned long,int,int,IcePointer) {
    }

  static void IceIOError(IceConn) {
    }

  static void IceWatch(IceConn c,IcePointer data,Bool opening,IcePointer*){
    IceHook * hook = static_cast<IceHook*>(data);
    if (opening) {
      ap::ap_set_closeonexec(IceConnectionNumber(c));
      //fcntl(IceConnectionNumber(c),F_SETFD,FD_CLOEXEC);
      hook->application->addInput(hook,ID_CONNECTION,IceConnectionNumber(c),INPUT_READ,c);
      }
    else
      hook->application->removeInput(IceConnectionNumber(c),INPUT_READ);
    }

protected:
  IceHook(){}
  IceHook(const IceHook&);
  IceHook& operator=(const IceHook&);
public:
  enum {
    ID_CONNECTION = 1
    };
public:
  long onConnection(FXObject*,FXSelector,void*ptr) {
    IceConn connection = (IceConn)ptr;
    IceProcessMessagesStatus status = IceProcessMessages(connection,nullptr,nullptr);
    if (status==IceProcessMessagesIOError) {
      application->removeInput(IceConnectionNumber(connection),INPUT_READ);
      IceCloseConnection(connection);
      }
    return 0;
    }

public:

  IceHook(FXApp * app) : application(app) {
    IceSetIOErrorHandler(IceIOError);
    IceSetErrorHandler(IceError);
    IceAddConnectionWatch(IceWatch,this);
    }

  ~IceHook(){
    IceRemoveConnectionWatch(IceWatch,this);
    }
  };

FXDEFMAP(IceHook) IceHookMap[]={
  FXMAPFUNC(SEL_IO_READ,IceHook::ID_CONNECTION,IceHook::onConnection),
  };

FXIMPLEMENT(IceHook,FXObject,IceHookMap,ARRAYNUMBER(IceHookMap))




static FXchar * gm_session_from_command(FXint argc,const FXchar * const argv[]){
  for (int i=1;i<argc;i++){
    if (FXString::compare(argv[i],"--session=",10)==0) {
      return (FXchar*)(argv[i]+10);
      }
    }
  return nullptr;
  }


class SMClient : public FXObject {
FXDECLARE(SMClient)
protected:
  SmcConn   connection = nullptr;
  GMSession* session   = nullptr;
  IceHook*   icehook   = nullptr;
protected:
  SMClient(){}
public:
  static void sm_save(SmcConn conn,SmPointer,int /*saveType*/,Bool /*shutdown*/,int /*interactStyle*/,Bool /*fast*/){
//    if (shutdown) {
//      }
    SmcSaveYourselfDone(conn,True);
    }

  static void sm_die(SmcConn conn,SmPointer ptr){
    SMClient * client = static_cast<SMClient*>(ptr);
    SmcCloseConnection(conn,0,nullptr);
    client->connection = nullptr;
    client->session->quit();
    }

  static void sm_shutdown_cancelled(SmcConn,SmPointer) {}
  static void sm_save_complete(SmcConn,SmPointer) {}
public:
  SMClient(FXApp*app,GMSession * s) : session(s) {
    icehook = new IceHook(app);
    }

  FXbool init(int argc,const FXchar * const argv[]) {
    FXString session_manager = FXSystem::getEnvironment("SESSION_MANAGER");
    if (!session_manager.empty()) {

      SmcCallbacks cb;

      cb.save_yourself.callback         = sm_save;
      cb.save_yourself.client_data      = this;
      cb.die.callback                   = sm_die;
      cb.die.client_data                = this;
      cb.save_complete.callback         = sm_save_complete;
      cb.save_complete.client_data      = this;
      cb.shutdown_cancelled.callback    = sm_shutdown_cancelled;
      cb.shutdown_cancelled.client_data = this;

      char * previd   = gm_session_from_command(argc,argv);
      char * clientid = nullptr;

      connection = SmcOpenConnection(nullptr,
                                     nullptr,
                                     SmProtoMajor,
                                     SmProtoMinor,
                                     SmcSaveYourselfProcMask|SmcDieProcMask|SmcSaveCompleteProcMask|SmcShutdownCancelledProcMask,
                                     &cb,
                                     previd,
                                     &clientid,
                                     0,
                                     nullptr);
      if (connection) {
        //setProperty(SmCurrentDirectory,FXSystem::getCurrentDirectory().text());
        setProperty(SmProcessID,FXString::value(FXProcess::current()).text());
        setProperty(SmUserID,FXSystem::currentUserName().text());
        setProperty(SmProgram,argv[0]);

        /// Seems to work for XFCE Session Manager
        //setProperty("_GSM_DesktopFile","/usr/share/applications/gogglesmm.desktop");

        FXStringList command;

        command.append(argv[0]);
        setProperty(SmCloneCommand,command);

        if (clientid) {

          command.append(FXString::value("--session=%s",clientid));
          setProperty(SmRestartCommand,command);

          free(clientid);
          }
        return true;
        }
      }
    return false;
    }

  void setProperty(const FXchar * name,const FXStringList & values) {


    SmPropValue * vals;


    allocElms(vals,values.no());

    for (int i=0;i<values.no();i++){
      vals[i].value  = (void*)values[i].text();
      vals[i].length = values[i].length()+1;
      }

    SmProp* list;
    SmProp prop;

    prop.name     = (char*)name;
    prop.type     = (char*)SmLISTofARRAY8;
    prop.num_vals = values.no();
    prop.vals     = vals;

    list=&prop;

    SmcSetProperties(connection,1,&list);
    freeElms(vals);
    }


  void setProperty(const FXchar * name,const FXString & value) {
    FXASSERT(connection);
    SmProp *list[1];
    SmProp prop;
    SmPropValue val;

    val.length    = value.length()+1;
    val.value     = (void*)value.text();

    prop.name     = (char*)name;
    prop.type     = (char*)SmARRAY8;
    prop.num_vals = 1;
    prop.vals     = &val;

    list[0]=&prop;

    SmcSetProperties(connection,1,list);
    }
  };

FXIMPLEMENT(SMClient,FXObject,nullptr,0)


FXIMPLEMENT(GMSession,FXObject,nullptr,0);


GMSession::GMSession(FXApp*app,FXObject*tgt,FXSelector sel) : target(tgt),message(sel) {
  smclient = new SMClient(app,this);
  }

GMSession::~GMSession() {
  delete smclient;
  }


FXbool GMSession::init(FXint argc,const FXchar * const argv[]){
  if (!smclient->init(argc,argv)){
    delete smclient;
    smclient=nullptr;
    return false;
    }
  return true;
  }


void GMSession::quit() {
  if (target)
    target->handle(this,FXSEL(SEL_SESSION_CLOSED,message),nullptr);
  }
