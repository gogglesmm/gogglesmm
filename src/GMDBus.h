/********************************************************************************
*                                                                               *
*                        F O X  D B U S  S U P P O R T                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved.                *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
********************************************************************************/
#ifndef GMDBUS_H
#define GMDBUS_H

#ifndef DBUS_BUS_H
#include <dbus/dbus.h>
#endif

/**
* GMDBus is a thin wrapper of DBusConnection with the purpose of integrating it
* into the FOX event loop (1.6), so that activity on DBus will be properly handled.
* DBusConnections may only be managed by one GMDBus. The APIs are strictly enforcing this
* by keeping a global (threadsafe) map of GMDBus/DBusConnection references.
**
* In the future I hope to support the new FXReactor framework in FOX.
*
*/

class GMDBusManager;

class GMDBus : public FXObject {
FXDECLARE(GMDBus)
private:
  DBusConnection * dc;
private:
  GMDBus(const GMDBus &);
  GMDBus &operator=(const GMDBus&);
public:
  enum {
    ID_HANDLE = 1,
    ID_DISPATCH,
    ID_LAST,
    };
public:
  long onHandleRead(FXObject*,FXSelector,void*);
  long onHandleWrite(FXObject*,FXSelector,void*);
  long onHandleExcept(FXObject*,FXSelector,void*);
  long onDispatch(FXObject*,FXSelector,void*);
public:
  /**
  *  Construct non active Dbus Connection Hook
  */
  GMDBus();

  /**
  *  Open Standard Bus
  *  return false if already managed by other FXDBusConnection
  */
  FXbool open(DBusBusType bustype=DBUS_BUS_SESSION);

  /**
  * Return FXDBusConnection for given DBusConnection. Return NULL if not found.
  */
  static GMDBus * find(DBusConnection * dc);

  /**
  * Init the event loop for all connections
  */
  static void initEventLoop();

 /**
  * Return DBUS version
  */
  static FXString dbusversion();

  /**
  * Setup Callback Hooks
  */
  virtual void setup_event_loop();

  /**
  * Return DBusConnection
  */
  DBusConnection * connection() const { return dc; }

  /**
  * Returns whether we're connected or not
  */
  FXbool connected() const;

  /**
  * Returns whether we're  authenticated
  */
  FXbool authenticated() const;


  /**
  * Flush
  */
  void flush();

  /**
  * Send with Reply
  */
  FXbool sendWithReply(DBusMessage * msg,FXint timeout,FXObject*,FXSelector);

  /**
  * Send
  */
  void send(DBusMessage * msg,FXuint & serial);
  void send(DBusMessage * msg);

  /**
  * Destructor. Existing DBusConnection will be unreffed.
  */
  virtual ~GMDBus();
  };


/* A Remote Object */
class GMDBusProxy : public FXObject {
FXDECLARE(GMDBusProxy)
protected:
  GMDBus           * bus;         /// Bus
  FXString           name;        /// Name
  FXString           path;        /// Path
  FXString           interface;   /// Interface
protected:
  FXbool             associated;  /// Are we associated
  FXObject         * target;      /// Target object
  FXSelector         message;     /// Message ID
protected:
  FXHash             serial;
protected:
  GMDBusProxy();
private:
  GMDBusProxy(const GMDBusProxy&);
  GMDBusProxy& operator=(const GMDBusProxy&);
public:
  long onCreate(FXObject*,FXSelector,void*);
  long onDestroy(FXObject*,FXSelector,void*);
  long onReplaced(FXObject*,FXSelector,void*);
  long onMethod(FXObject*,FXSelector,void*);
  long onSignal(FXObject*,FXSelector,void*);
public:
  GMDBusProxy(GMDBus*,const FXchar * name,const FXchar * path,const FXchar * interface);

  FXString getPath() const { return path; }

  FXString getName() const { return name; }

  FXString getInterface() const { return interface; }

  FXbool matchSerial(DBusMessage * msg);

  DBusMessage * method(const FXchar * method);
  
  DBusMessage * signal(const FXchar * name);

  void send(DBusMessage*,FXObject*,FXSelector);

  virtual ~GMDBusProxy();
  };



#define DEBUG_DBUS_MESSAGE(msg) { \
    FXTRACE((80,"-----%s-------\n",__func__)); \
    FXTRACE((80,"type: %s\n",dbus_message_type_to_string(dbus_message_get_type(msg))));\
    FXTRACE((80,"path: %s\n",dbus_message_get_path(msg))); \
    FXTRACE((80,"member: \"%s\"\n",dbus_message_get_member(msg))); \
    FXTRACE((80,"interface: %s\n",dbus_message_get_interface(msg))); \
    FXTRACE((80,"sender: %s\n",dbus_message_get_sender(msg))); \
    FXTRACE((80,"signature: %s\n",dbus_message_get_signature(msg))); }

/* Some Helper Functions */
extern void gm_dbus_variant_append_basic(DBusMessageIter * iter,const FXchar * element_string,FXint element,const void * value);
extern void gm_dbus_variant_append_string(DBusMessageIter * iter,const FXchar * value);
extern void gm_dbus_variant_append_path(DBusMessageIter * iter,const FXchar * value);
extern void gm_dbus_variant_append_int32(DBusMessageIter * iter,const FXint value);
extern void gm_dbus_variant_append_uint32(DBusMessageIter * iter,const FXuint value);
extern void gm_dbus_variant_append_bool(DBusMessageIter * iter,const dbus_bool_t value);
extern void gm_dbus_variant_append_double(DBusMessageIter * iter,const FXdouble value);
extern void gm_dbus_variant_append_long(DBusMessageIter * iter,const FXlong value);
extern void gm_dbus_variant_append_string_list(DBusMessageIter * iter,const FXchar * data[]);
extern void gm_dbus_variant_append_string_list(DBusMessageIter * iter,const FXStringList&);

extern void gm_dbus_append_string(DBusMessageIter *iter,const FXString & value);
extern void gm_dbus_append_string_pair(DBusMessageIter *iter,const FXchar * key,const FXchar * value);

extern void gm_dbus_dict_append_int32(DBusMessageIter * dict,const FXchar * key,const FXint value);
extern void gm_dbus_dict_append_uint32(DBusMessageIter * dict,const FXchar * key,const FXuint value);
extern void gm_dbus_dict_append_string(DBusMessageIter * dict,const FXchar * key,const FXchar * value);
extern void gm_dbus_dict_append_string(DBusMessageIter * dict,const FXchar * key,const FXString & value);
extern void gm_dbus_dict_append_double(DBusMessageIter * dict,const FXchar * key,const FXdouble & value);
extern void gm_dbus_dict_append_long(DBusMessageIter * dict,const FXchar * key,const FXlong & value);
extern void gm_dbus_dict_append_path(DBusMessageIter * dict,const FXchar * key,const FXchar * value);
extern void gm_dbus_dict_append_bool(DBusMessageIter * dict,const FXchar * key,const dbus_bool_t value);
extern void gm_dbus_dict_append_string_list(DBusMessageIter * dict,const FXchar * key,const FXchar * data[]);
extern void gm_dbus_dict_append_string_list(DBusMessageIter * dict,const FXchar * key,const FXStringList &);

extern DBusHandlerResult gm_dbus_reply_string(DBusConnection * connection,DBusMessage * msg,const FXchar * xml);
extern DBusHandlerResult gm_dbus_reply_uint_string(DBusConnection * connection,DBusMessage * msg,const FXuint val,const FXchar * xml);
extern DBusHandlerResult gm_dbus_reply_int(DBusConnection * connection,DBusMessage * msg,const FXint value);
extern DBusHandlerResult gm_dbus_reply_double(DBusConnection * connection,DBusMessage * msg,const FXdouble value);
extern DBusHandlerResult gm_dbus_reply_unsigned_int(DBusConnection * connection,DBusMessage * msg,const FXuint value);
extern DBusHandlerResult gm_dbus_reply_long(DBusConnection * connection,DBusMessage * msg,const FXlong value);
extern DBusHandlerResult gm_dbus_reply_if_needed(DBusConnection * connection,DBusMessage * msg);
extern DBusHandlerResult gm_dbus_reply_bool(DBusConnection*connection,DBusMessage*msg,const dbus_bool_t);
extern DBusHandlerResult gm_dbus_reply_string_list(DBusConnection * connection,DBusMessage * msg,const FXchar * data[]);

extern void gm_dbus_match_signal(DBusConnection*,const FXchar * path,const FXchar * interface,const FXchar * member);

#endif



