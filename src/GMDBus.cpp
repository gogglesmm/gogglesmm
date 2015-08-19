/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2015 by Sander Jansen. All Rights Reserved      *
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
#include "GMDBus.h"
#include "FXThread.h"

#ifndef DBUS_MAJOR_VERSION
#define DBUS_MAJOR_VERSION 1
#endif

#ifndef DBUS_MINOR_VERSION
#define DBUS_MINOR_VERSION 0
#endif

#ifndef DBUS_MICRO_VERSION
#define DBUS_MICRO_VERSION 0
#endif

#define DBUSVERSION ((DBUS_MICRO_VERSION) + (DBUS_MINOR_VERSION*1000) + (DBUS_MAJOR_VERSION*100000))
#define MKDBUSVERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))


/*******************************************************************************************************/
/* GLOBAL INIT                                                                                         */
/*******************************************************************************************************/

class GMDBusTimeout;

class GMDBusGlobal {
private:
  FXMutex mutex;
  FXHash  tm;
  FXHash  connections;
public:
  GMDBusGlobal() {
    dbus_threads_init_default();
    }

  GMDBus * find(DBusConnection* dc) {
    FXScopedMutex lock(mutex);
    return static_cast<GMDBus*>(connections.at(dc));
    }

  void setuphooks() {
    for (FXint i=0;i<connections.no();i++) {
      if (!connections.empty(i)) {
        static_cast<GMDBus*>(connections.value(i))->setup_event_loop();
        }
      }
    }

  void insert(DBusConnection * dc,GMDBus * fxdc) {
    FXScopedMutex lock(mutex);
    connections.insert(dc,fxdc);
    }

  void remove(DBusConnection * dc) {
    FXScopedMutex lock(mutex);
    connections.remove(dc);
    }

  GMDBusTimeout * find(DBusTimeout*t) {
    return static_cast<GMDBusTimeout*>(tm.at(t));
    }

  void insert(DBusTimeout*t,GMDBusTimeout*f) {
    tm.insert(t,f);
    }

  void remove(DBusTimeout*t) {
    tm.remove(t);
    }
  };

static GMDBusGlobal fxdbus;

class GMDBusTimeout : public FXObject {
FXDECLARE(GMDBusTimeout);
protected:
  DBusTimeout* timeout = nullptr;
  FXuchar      flags = 0;
protected:
  GMDBusTimeout(){}
private:
  GMDBusTimeout(const GMDBusTimeout*);
  GMDBusTimeout& operator=(const GMDBusTimeout&);
protected:
  enum {
    FLAG_CALLBACK  = 0x1,
    FLAG_REMOVED   = 0x2
    };
public:
  enum {
    ID_TIMEOUT = 1
    };
public:
  long onTimeout(FXObject*,FXSelector,void*);
public:
  GMDBusTimeout(DBusTimeout *t);
  void add();
  void remove();
  ~GMDBusTimeout();
  };

FXDEFMAP(GMDBusTimeout) GMDBusTimeoutMap[]={
  FXMAPFUNC(SEL_TIMEOUT,GMDBusTimeout::ID_TIMEOUT,GMDBusTimeout::onTimeout),
  };
FXIMPLEMENT(GMDBusTimeout,FXObject,GMDBusTimeoutMap,ARRAYNUMBER(GMDBusTimeoutMap));



GMDBusTimeout::GMDBusTimeout(DBusTimeout *t) : timeout(t){
  fxdbus.insert(timeout,this);
  }

GMDBusTimeout::~GMDBusTimeout() {
  fxdbus.remove(timeout);
  }

void GMDBusTimeout::add() {
  flags&=~FLAG_REMOVED;
  FXApp::instance()->addTimeout(this,GMDBusTimeout::ID_TIMEOUT,TIME_MSEC(dbus_timeout_get_interval(timeout)));
  }

void GMDBusTimeout::remove() {
  if (flags&FLAG_CALLBACK) {
    flags|=FLAG_REMOVED;
    return;
    }
  FXApp::instance()->removeTimeout(this,GMDBusTimeout::ID_TIMEOUT);
  delete this;
  }


static dbus_bool_t fxdbus_addtimeout(DBusTimeout *timeout,void *) {
  //fxmessage("fxdbus_addtimeout %p\n",timeout);
  GMDBusTimeout * tm = fxdbus.find(timeout);
  if (tm==nullptr)
    tm = new GMDBusTimeout(timeout);

  tm->add();
  return true;
  }


static void fxdbus_removetimeout(DBusTimeout *timeout,void *) {
  //fxmessage("fxdbus_removetimeout %p\n",timeout);
  GMDBusTimeout * tm = fxdbus.find(timeout);
  if (tm) tm->remove();
  }

static void fxdbus_toggletimeout(DBusTimeout *timeout,void*data) {
  //fxmessage("fxdbus_toggletimeout %p\n",timeout);
  if (dbus_timeout_get_enabled(timeout) && dbus_timeout_get_interval(timeout)>0)
    fxdbus_addtimeout(timeout,data);
  else
    fxdbus_removetimeout(timeout,data);
  }

long GMDBusTimeout::onTimeout(FXObject*,FXSelector,void*){
  //fxmessage("onTimeout() %p {\n",timeout);
  flags|=FLAG_CALLBACK;
  dbus_timeout_handle(timeout);
  if (flags&FLAG_REMOVED) {
    delete this;
    //fxmessage("}\n");
    return 1;
    }
  else {
    if (dbus_timeout_get_enabled(timeout) && dbus_timeout_get_interval(timeout)>0 && !FXApp::instance()->hasTimeout(this,ID_TIMEOUT)){
      FXApp::instance()->addTimeout(this,GMDBusTimeout::ID_TIMEOUT,TIME_MSEC(dbus_timeout_get_interval(timeout)));
      }
    }
  flags&=~FLAG_CALLBACK;
  //fxmessage("}\n");
  return 1;
  }











/*******************************************************************************************************/
/* Call Backs                                                                                          */
/*******************************************************************************************************/

static dbus_bool_t fxdbus_addwatch(DBusWatch *watch,void * data) {
  FXTRACE((35,"fxdbus_addwatch()\n"));
  GMDBus * dc = static_cast<GMDBus*>(data);
  FXuint mode  = INPUT_EXCEPT;
  FXuint flags = dbus_watch_get_flags(watch);

  /// If it's not enabled, we're not going to add it
  if (!dbus_watch_get_enabled(watch)) return true;

  if (flags&DBUS_WATCH_READABLE)
      mode|=INPUT_READ;
  if (flags&DBUS_WATCH_WRITABLE)
      mode|=INPUT_WRITE;

#if DBUSVERSION == MKDBUSVERSION(1,1,20) || DBUSVERSION >= MKDBUSVERSION(1,2,0)
  return FXApp::instance()->addInput(dc,GMDBus::ID_HANDLE,(FXInputHandle)dbus_watch_get_unix_fd(watch),mode,watch);
#else
  return FXApp::instance()->addInput(dc,GMDBus::ID_HANDLE,(FXInputHandle)dbus_watch_get_fd(watch),mode,watch);
#endif

  }

static void fxdbus_removewatch(DBusWatch *watch,void *) {
  FXTRACE((35,"fxdbus_removewatch()\n"));
/*
  FXuint mode=INPUT_EXCEPT;
  unsigned int flags = dbus_watch_get_flags(watch);
  if (flags&DBUS_WATCH_READABLE)
      mode|=INPUT_READ;
  if (flags&DBUS_WATCH_WRITABLE) {
      mode|=INPUT_WRITE;
      return;
      }
*/
  FXuint mode=INPUT_READ|INPUT_WRITE|INPUT_EXCEPT;
#if DBUSVERSION == MKDBUSVERSION(1,1,20) || DBUSVERSION >= MKDBUSVERSION(1,2,0)
  FXApp::instance()->removeInput(dbus_watch_get_unix_fd(watch),mode);
#else
  FXApp::instance()->removeInput(dbus_watch_get_fd(watch),mode);
#endif

  }

static void fxdbus_togglewatch(DBusWatch *watch,void* data) {
  FXTRACE((35,"fxdbus_togglewatch()\n"));
  if (dbus_watch_get_enabled(watch))
    fxdbus_addwatch(watch,data);
  else
    fxdbus_removewatch(watch,data);
  }

static void fxdbus_wakeup(void *){
  /// To Do
  }



/*******************************************************************************************************/
/* PUBLIC API                                                                                          */
/*******************************************************************************************************/

FXDEFMAP(GMDBus) GMDBusMap[]={
  FXMAPFUNC(SEL_IO_READ,GMDBus::ID_HANDLE,GMDBus::onHandleRead),
  FXMAPFUNC(SEL_IO_WRITE,GMDBus::ID_HANDLE,GMDBus::onHandleWrite),
  FXMAPFUNC(SEL_IO_EXCEPT,GMDBus::ID_HANDLE,GMDBus::onHandleExcept),
  FXMAPFUNC(SEL_CHORE,GMDBus::ID_DISPATCH,GMDBus::onDispatch)
  };

FXIMPLEMENT(GMDBus,FXObject,GMDBusMap,ARRAYNUMBER(GMDBusMap));

GMDBus::GMDBus() {
  }

GMDBus::~GMDBus(){
  if (dc) {
    fxdbus.remove(dc);
    dbus_connection_unref(dc);
    dc=nullptr;
    FXApp::instance()->removeChore(this,ID_DISPATCH);
    }
  }


GMDBus * GMDBus::find(DBusConnection * dc) {
  return fxdbus.find(dc);
  }

void GMDBus::initEventLoop() {
  fxdbus.setuphooks();
  }


FXbool GMDBus::open(DBusBusType bustype/*=DBUS_BUS_SESSION*/){
  FXASSERT(dc==nullptr);
  dc = dbus_bus_get(bustype,nullptr);
  if (dc==nullptr) return false;
  if (fxdbus.find(dc)) {
    dbus_connection_unref(dc);
    dc=nullptr;
    return false;
    }
  fxdbus.insert(dc,this);
  dbus_connection_set_exit_on_disconnect(dc,false);
  return true;
  }


FXbool GMDBus::connected() const {
  if (dc)
    return dbus_connection_get_is_connected(dc);
  else
    return false;
  }

FXbool GMDBus::authenticated() const {
  if (dc)
    return dbus_connection_get_is_authenticated(dc);
  else
    return false;
  }

void GMDBus::flush() {
  if (dc) dbus_connection_flush(dc);
  }


FXString GMDBus::dbusversion() {
#if (DBUSVERSION == MKDBUSVERSION(1,1,20)) || DBUSVERSION >= MKDBUSVERSION(1,2,0)
  int major,minor,micro;
  dbus_get_version(&major,&minor,&micro);
  return FXString::value("%d.%d.%d",major,minor,micro);
#else
  return FXString("1.0.x");
#endif
  }

struct CallTarget{
  FXObject * target;
  FXSelector message;
  };

static void fxdbus_pendingcallfree(void *memory){
  //fxmessage("fxdbuspendingcallfree\n");
  if (memory){
    CallTarget * call = static_cast<CallTarget*>(memory);
    delete call;
    }
  }

static void fxdbus_pendingcallnotify(DBusPendingCall*pending,void*data){
  DBusMessage * msg = dbus_pending_call_steal_reply(pending);
  if (msg) {
    CallTarget * call = static_cast<CallTarget*>(data);
    if (call && call->target && call->message) {
      call->target->handle(nullptr,FXSEL(SEL_COMMAND,call->message),msg);
      }
    dbus_message_unref(msg);
    }
  }


FXbool GMDBus::sendWithReply(DBusMessage * msg,FXint timeout,FXObject*tgt,FXSelector sel){
  FXASSERT(msg);
  DBusPendingCall * pending = nullptr;
  if (dbus_connection_send_with_reply(dc,msg,&pending,timeout)) {
    if (pending) {
      CallTarget * call = new CallTarget;
      call->target=tgt;
      call->message=sel;
      dbus_pending_call_set_notify(pending,fxdbus_pendingcallnotify,(void*)call,fxdbus_pendingcallfree);
      dbus_pending_call_unref(pending);
      }
    dbus_message_unref(msg);
    return true;
    }
  return false;
  }


void GMDBus::send(DBusMessage * msg){
  FXuint serial;
  dbus_connection_send(dc,msg,&serial);
  dbus_message_unref(msg);
  }

void GMDBus::send(DBusMessage * msg,FXuint & serial){
  dbus_connection_send(dc,msg,&serial);
  dbus_message_unref(msg);
  }



/*******************************************************************************************************/
/* MESSAGE HANDLERS                                                                                    */
/*******************************************************************************************************/

long GMDBus::onHandleRead(FXObject*,FXSelector,void*ptr){
  DBusWatch * watch = static_cast<DBusWatch*>(ptr);
  dbus_watch_handle(watch,DBUS_WATCH_READABLE);
  FXApp::instance()->addChore(this,ID_DISPATCH);
  return 0;
  }

long GMDBus::onHandleWrite(FXObject*,FXSelector,void*ptr){
  DBusWatch * watch = static_cast<DBusWatch*>(ptr);
  dbus_watch_handle(watch,DBUS_WATCH_WRITABLE);
  return 0;
  }

long GMDBus::onHandleExcept(FXObject*,FXSelector,void*ptr){
  DBusWatch * watch = static_cast<DBusWatch*>(ptr);
  dbus_watch_handle(watch,DBUS_WATCH_ERROR|DBUS_WATCH_HANGUP);
  return 0;
  }

long GMDBus::onDispatch(FXObject*,FXSelector,void*){
  if (dbus_connection_dispatch((DBusConnection*)dc)==DBUS_DISPATCH_DATA_REMAINS) {
    FXApp::instance()->addChore(this,ID_DISPATCH);
    }
  return 0;
  }


/*******************************************************************************************************/
/* PROTECTED API                                                                                       */
/*******************************************************************************************************/

void GMDBus::setup_event_loop() {
  FXASSERT(dc);

  dbus_connection_set_watch_functions(dc,
                                      fxdbus_addwatch,
                                      fxdbus_removewatch,
                                      fxdbus_togglewatch,
                                      this,nullptr);

  dbus_connection_set_timeout_functions(dc,
                                        fxdbus_addtimeout,
                                        fxdbus_removetimeout,
                                        fxdbus_toggletimeout,
                                        this,nullptr);

  dbus_connection_set_wakeup_main_function(dc,fxdbus_wakeup,nullptr,nullptr);
  }





static DBusHandlerResult dbus_proxy_filter(DBusConnection *,DBusMessage * msg,void * ptr){
  GMDBusProxy * proxy = static_cast<GMDBusProxy*>(ptr);
  FXASSERT(proxy);
  FXuint type = dbus_message_get_type(msg);

  if (type==DBUS_MESSAGE_TYPE_METHOD_CALL) {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  else if (type==DBUS_MESSAGE_TYPE_METHOD_RETURN || type==DBUS_MESSAGE_TYPE_ERROR) {
    if (proxy->matchSerial(msg))
      return DBUS_HANDLER_RESULT_HANDLED;
    else
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  else { /* SIGNALS */
    DEBUG_DBUS_MESSAGE(msg);

    if (dbus_message_has_path(msg,DBUS_PATH_DBUS)) {
      if (dbus_message_is_signal(msg,DBUS_INTERFACE_DBUS,"NameOwnerChanged")) {
        const FXchar * dbus_name=nullptr;
        const FXchar * new_owner=nullptr;
        const FXchar * old_owner=nullptr;
        if (dbus_message_get_args(msg,nullptr,DBUS_TYPE_STRING,&dbus_name,DBUS_TYPE_STRING,&old_owner,DBUS_TYPE_STRING,&new_owner,DBUS_TYPE_INVALID)) {
          if (compare(proxy->getName(),dbus_name)==0) {
            if (new_owner==nullptr || compare(new_owner,"")==0) {
              proxy->handle(proxy,FXSEL(SEL_DESTROY,0),nullptr);
              }
            else if (old_owner==nullptr || compare(old_owner,"")==0){
              proxy->handle(proxy,FXSEL(SEL_CREATE,0),nullptr);
              }
            else {
              proxy->handle(proxy,FXSEL(SEL_REPLACED,0),nullptr);
              }
            return DBUS_HANDLER_RESULT_HANDLED;
            }
          }
        }
      }
    /// Make sure it is for us...
    if (dbus_message_has_path(msg,proxy->getPath().text()) && dbus_message_has_interface(msg,proxy->getInterface().text())) {
      return proxy->handle(proxy,FXSEL(SEL_SIGNAL,0),msg) ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
      }
    }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }



struct GMDBusProxyReply {
  FXObject * target;
  FXSelector message;
  GMDBusProxyReply(FXObject * t,FXSelector m) : target(t), message(m) {}
  };


FXDEFMAP(GMDBusProxy) GMDBusProxyMap[]={
  FXMAPFUNC(SEL_CREATE,   0,GMDBusProxy::onCreate),
  FXMAPFUNC(SEL_DESTROY,  0,GMDBusProxy::onDestroy),
  FXMAPFUNC(SEL_REPLACED, 0,GMDBusProxy::onReplaced),
  FXMAPFUNC(SEL_SIGNAL,   0,GMDBusProxy::onSignal),
  FXMAPFUNC(SEL_COMMAND,  0,GMDBusProxy::onMethod),
  };

FXIMPLEMENT(GMDBusProxy,FXObject,GMDBusProxyMap,ARRAYNUMBER(GMDBusProxyMap));


GMDBusProxy::GMDBusProxy() {
  }

GMDBusProxy::~GMDBusProxy()  {
  FXString rule = "type='signal',sender='"+ name +"',path='" + path +"',interface='"+interface+"'";
  dbus_bus_remove_match(bus->connection(),rule.text(),nullptr);
  dbus_connection_remove_filter(bus->connection(),dbus_proxy_filter,this);

  /// remove any pending proxy replies;
  for (FXint i=0;i<serial.no();i++) {
    if (!serial.empty(i)) {
      GMDBusProxyReply * reply = static_cast<GMDBusProxyReply*>(serial.value(i));
      delete reply;
      }
    }
  serial.clear();
  }

GMDBusProxy::GMDBusProxy(GMDBus *c,const FXchar * n,const FXchar * p,const FXchar * i) : bus(c),name(n),path(p),interface(i),associated(true),target(nullptr) {
  FXString rule = "type='signal',sender='"+ name +"',path='" + path +"',interface='"+interface+"'";
  dbus_bus_add_match(bus->connection(),rule.text(),nullptr);
  dbus_connection_add_filter(bus->connection(),dbus_proxy_filter,this,nullptr);
  }


FXbool GMDBusProxy::matchSerial(DBusMessage * msg) {
  void * ptr;
  FXuint s=dbus_message_get_reply_serial(msg);
  if (s && (ptr=serial.at((void*)(FXuval)s))!=nullptr) {
    GMDBusProxyReply * reply = static_cast<GMDBusProxyReply*>(ptr);
    if (reply->target) reply->target->handle(this,FXSEL(SEL_COMMAND,reply->message),msg);
    serial.remove((void*)(FXuval)dbus_message_get_reply_serial(msg));
    delete reply;
    return true;
    }
  else {
    return false;
    }
  }

void GMDBusProxy::send(DBusMessage*msg,FXObject * obj,FXSelector m) {
  FXuint s;
  dbus_connection_send(bus->connection(),msg,&s);
  dbus_message_unref(msg);
  serial.insert((void*)(FXuval)s,new GMDBusProxyReply(obj,m));
  }



DBusMessage * GMDBusProxy::method(const FXchar * methodcall){
  return dbus_message_new_method_call(name.text(),path.text(),interface.text(),methodcall);
  }

DBusMessage * GMDBusProxy::signal(const FXchar * sname){
  return dbus_message_new_signal(path.text(),interface.text(),sname);
  }


long GMDBusProxy::onCreate(FXObject*,FXSelector,void*ptr) {
  associated=true;
  return target && target->tryHandle(this,FXSEL(SEL_DESTROY,message),ptr);
  }
long GMDBusProxy::onDestroy(FXObject*,FXSelector,void*ptr) {
  associated=false;
  return target && target->tryHandle(this,FXSEL(SEL_DESTROY,message),ptr);
  }
long GMDBusProxy::onReplaced(FXObject*,FXSelector,void*ptr) {
  return target && target->tryHandle(this,FXSEL(SEL_REPLACED,message),ptr);
  }
long GMDBusProxy::onSignal(FXObject*,FXSelector,void*ptr) {
  return target && target->tryHandle(this,FXSEL(SEL_SIGNAL,message),ptr);
  }
long GMDBusProxy::onMethod(FXObject*,FXSelector,void*ptr) {
  return target && target->tryHandle(this,FXSEL(SEL_COMMAND,message),ptr);
  }





















/*******************************************************************************************************/
/* HELPER API                                                                                 */
/*******************************************************************************************************/


void gm_dbus_variant_append_basic(DBusMessageIter * iter,const FXchar * element_type_string,FXint element_type,const void * value) {
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_VARIANT,element_type_string,&container);
  dbus_message_iter_append_basic(&container,element_type,value);
  dbus_message_iter_close_container(iter,&container);
  }


void gm_dbus_variant_append_string(DBusMessageIter * iter,const FXchar * value) {
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_STRING_AS_STRING,DBUS_TYPE_STRING,&value);
  }

void gm_dbus_variant_append_int32(DBusMessageIter * iter,const FXint value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_INT32_AS_STRING,DBUS_TYPE_INT32,&value);
  }

void gm_dbus_variant_append_uint32(DBusMessageIter * iter,const FXuint value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_UINT32_AS_STRING,DBUS_TYPE_UINT32,&value);
  }

void gm_dbus_variant_append_path(DBusMessageIter * iter,const FXchar * value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_OBJECT_PATH_AS_STRING,DBUS_TYPE_OBJECT_PATH,&value);
  }

void gm_dbus_variant_append_bool(DBusMessageIter * iter,const dbus_bool_t value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_BOOLEAN_AS_STRING,DBUS_TYPE_BOOLEAN,&value);
  }

void gm_dbus_variant_append_double(DBusMessageIter * iter,const FXdouble value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_DOUBLE_AS_STRING,DBUS_TYPE_DOUBLE,&value);
  }

void gm_dbus_variant_append_long(DBusMessageIter * iter,const FXlong value){
  gm_dbus_variant_append_basic(iter,DBUS_TYPE_INT64_AS_STRING,DBUS_TYPE_INT64,&value);
  }

void gm_dbus_variant_append_string_list(DBusMessageIter * iter,const FXchar * value[]){
  DBusMessageIter container;
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_VARIANT,DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING,&container);
  dbus_message_iter_open_container(&container,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
  for (FXint n=0;value[n];n++) {
    dbus_message_iter_append_basic(&array,DBUS_TYPE_STRING,&value[n]);
    }
  dbus_message_iter_close_container(&container,&array);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_variant_append_string_list(DBusMessageIter * iter,const FXStringList & list){
  DBusMessageIter container;
  DBusMessageIter array;
  dbus_message_iter_open_container(iter,DBUS_TYPE_VARIANT,DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING,&container);
  dbus_message_iter_open_container(&container,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
  for (FXint n=0;n<list.no();n++) {
    gm_dbus_append_string(&array,list[n]);
    }
  dbus_message_iter_close_container(&container,&array);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_append_string(DBusMessageIter *iter,const FXString & value){
  const FXchar * v = value.text();
  dbus_message_iter_append_basic(iter,DBUS_TYPE_STRING,&v);
  }


void gm_dbus_append_string_pair(DBusMessageIter *iter,const FXchar * key,const FXchar * value){
  dbus_message_iter_append_basic(iter,DBUS_TYPE_STRING,&key);
  dbus_message_iter_append_basic(iter,DBUS_TYPE_STRING,&value);
  }


void gm_dbus_dict_append_int32(DBusMessageIter * iter,const FXchar * key,const FXint value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_int32(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_uint32(DBusMessageIter * iter,const FXchar * key,const FXuint value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_uint32(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }


void gm_dbus_dict_append_string(DBusMessageIter * iter,const FXchar * key,const FXchar * value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_string(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_string(DBusMessageIter * iter,const FXchar * key,const FXString & value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_string(&container,value.text());
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_path(DBusMessageIter * iter,const FXchar * key,const FXchar * value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_path(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_bool(DBusMessageIter * iter,const FXchar * key,const dbus_bool_t value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_bool(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }


void gm_dbus_dict_append_double(DBusMessageIter * iter,const FXchar * key,const FXdouble & value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_double(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_long(DBusMessageIter * iter,const FXchar * key,const FXlong & value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_long(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }


void gm_dbus_dict_append_string_list(DBusMessageIter * iter,const FXchar * key,const FXchar * value[]){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_string_list(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }

void gm_dbus_dict_append_string_list(DBusMessageIter * iter,const FXchar * key,const FXStringList &value){
  DBusMessageIter container;
  dbus_message_iter_open_container(iter,DBUS_TYPE_DICT_ENTRY,0,&container);
  dbus_message_iter_append_basic(&container,DBUS_TYPE_STRING,&key);
  gm_dbus_variant_append_string_list(&container,value);
  dbus_message_iter_close_container(iter,&container);
  }


DBusHandlerResult gm_dbus_property_string(DBusConnection * connection,DBusMessage * msg,const FXchar * data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply,&iter);
    gm_dbus_variant_append_string(&iter,data);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_property_string_list(DBusConnection * connection,DBusMessage * msg,const FXchar * data[]) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply,&iter);
    gm_dbus_variant_append_string_list(&iter,data);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }


DBusHandlerResult gm_dbus_property_bool(DBusConnection * connection,DBusMessage * msg,const dbus_bool_t data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply,&iter);
    gm_dbus_variant_append_bool(&iter,data);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_property_long(DBusConnection * connection,DBusMessage * msg,const FXlong data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply,&iter);
    gm_dbus_variant_append_long(&iter,data);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }


DBusHandlerResult gm_dbus_property_double(DBusConnection * connection,DBusMessage * msg,const FXdouble data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply,&iter);
    gm_dbus_variant_append_double(&iter,data);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }



DBusHandlerResult gm_dbus_reply_string(DBusConnection * connection,DBusMessage * msg,const FXchar * data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_STRING,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_uint_string(DBusConnection * connection,DBusMessage * msg,const FXuint val,const FXchar * data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_UINT32,&val,DBUS_TYPE_STRING,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_int(DBusConnection * connection,DBusMessage * msg,const FXint data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_INT32,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_double(DBusConnection * connection,DBusMessage * msg,const FXdouble data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_DOUBLE,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_long(DBusConnection * connection,DBusMessage * msg,const FXlong data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_INT64,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_unsigned_int(DBusConnection * connection,DBusMessage * msg,const FXuint data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_UINT32,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_bool(DBusConnection * connection,DBusMessage * msg,const dbus_bool_t data) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    dbus_message_append_args(reply,DBUS_TYPE_BOOLEAN,&data,DBUS_TYPE_INVALID);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

DBusHandlerResult gm_dbus_reply_string_list(DBusConnection * connection,DBusMessage * msg,const FXchar * data[]) {
  FXuint serial;
  DBusMessage * reply = dbus_message_new_method_return(msg);
  if (reply) {
    DBusMessageIter iter;
    DBusMessageIter array;
    dbus_message_iter_init_append(reply,&iter);
    dbus_message_iter_open_container(&iter,DBUS_TYPE_ARRAY,DBUS_TYPE_STRING_AS_STRING,&array);
    for (FXint n=0;data[n];n++) {
      dbus_message_iter_append_basic(&array,DBUS_TYPE_STRING,&data[n]);
      }
    dbus_message_iter_close_container(&iter,&array);
    dbus_connection_send(connection,reply,&serial);
    dbus_message_unref(reply);
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }




DBusHandlerResult gm_dbus_reply_if_needed(DBusConnection * connection,DBusMessage * msg) {
  if (!dbus_message_get_no_reply(msg)) {
    FXuint serial;
    DBusMessage * reply = dbus_message_new_method_return(msg);
    if (reply) {
      dbus_connection_send(connection,reply,&serial);
      dbus_message_unref(reply);
      }
    }
  return DBUS_HANDLER_RESULT_HANDLED;
  }

void gm_dbus_match_signal(DBusConnection*connection,const FXchar * path,const FXchar * interface,const FXchar * member){
  FXString rule = FXString::value("type='signal',path='%s',interface='%s',member='%s'",path,interface,member);
  dbus_bus_add_match(connection,rule.text(),nullptr);
  }
