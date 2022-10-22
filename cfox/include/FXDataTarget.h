/********************************************************************************
*                                                                               *
*                              D a t a   T a r g e t                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXDATATARGET_H
#define FXDATATARGET_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


/**
* A Data Target allows a widget to be directly connected with a associated variable,
* without any additional "glue code".  This connection is bi-directional: widgets can
* not only only change the associated variable, but also query the associated variable,
* and reflect its value in the widget.
*
* The value of the associated variable is changed by the data target when it receives
* a SEL_COMMAND or SEL_CHANGED message from the widget.  Conversely, the widget's state
* may be updated from the data target's associated variable when the it receives a
* SEL_UPDATE query message from the widget.
*
* Valuator widgets should send an ID_VALUE message to the data target.  When a data
* target receives the ID_VALUE message, it will obtain the value of the sending valuator
* widget by querying it with a ID_GETINTVALUE, ID_GETLONGVALUE, ID_GETREALVALUE, or
* ID_GETSTRINGVALUE message, depending on the type of the associated variable.
*
* Radio Buttons, Menu Commands, and so on can also be connected to a data target.
* In this case, the widget must send an ID_OPTION+i message; the value of the associated
* variable will be obtained from the message itself, by simply subtracting ID_OPTION
* from the message ID, that is to say, the value will be i (-10000 <= i <= 10000).
*
* Updating of widgets from the data target is performed when the widget sends the data
* target a SEL_UPDATE message.  For ID_VALUE update queries, the data target responds
* with ID_SETINTVALUE, ID_SETLONGVALUE, ID_SETREALVALUE, or ID_SETSTRINGVALUE depending
* on the type of the associated variable.
* For ID_OPTION+i update queries, the data target responds with a ID_CHECK or ID_UNCHECK
* depending on whether the connected variable's value is equal to i or not.
*
* A data target may be subclassed to handle additional, user-defined data types; to
* this end, the message handlers return 1 if the type is one of DT_VOID...DT_STRING
* and 0 otherwise.
*/
class FXAPI FXDataTarget : public FXObject {
  FXDECLARE(FXDataTarget)
protected:
  void         *data;           // Associated data
  FXObject     *target;         // Target object
  FXSelector    message;        // Message ID
  FXuint        type;           // Type of data
private:
  FXDataTarget(const FXDataTarget&);
  FXDataTarget& operator=(const FXDataTarget&);
public:
  long onCmdValue(FXObject*,FXSelector,void*);
  long onUpdValue(FXObject*,FXSelector,void*);
  long onCmdOption(FXObject*,FXSelector,void*);
  long onUpdOption(FXObject*,FXSelector,void*);
public:
  enum {
    DT_VOID=0,
    DT_BOOL,
    DT_CHAR,
    DT_UCHAR,
    DT_SHORT,
    DT_USHORT,
    DT_INT,
    DT_UINT,
    DT_LONG,
    DT_ULONG,
    DT_FLOAT,
    DT_DOUBLE,
    DT_STRING,
    DT_LAST
    };
public:
  enum {
    ID_VALUE=1,                   /// Will cause the FXDataTarget to ask sender for value
    ID_OPTION=ID_VALUE+10001,     /// ID_OPTION+i will set the value to i where -10000<=i<=10000
    ID_LAST=ID_OPTION+10000
    };
public:

  /// Associate with nothing
  FXDataTarget():data(nullptr),target(nullptr),message(0),type(DT_VOID){}

  /// Associate with nothing
  FXDataTarget(FXObject* tgt,FXSelector sel):data(nullptr),target(tgt),message(sel),type(DT_VOID){}

  /// Associate with character variable
  FXDataTarget(FXbool& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_BOOL){}

  /// Associate with character variable
  FXDataTarget(FXchar& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_CHAR){}

  /// Associate with unsigned character variable
  FXDataTarget(FXuchar& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_UCHAR){}

  /// Associate with signed short variable
  FXDataTarget(FXshort& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_SHORT){}

  /// Associate with unsigned short variable
  FXDataTarget(FXushort& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_USHORT){}

  /// Associate with int variable
  FXDataTarget(FXint& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_INT){}

  /// Associate with unsigned int variable
  FXDataTarget(FXuint& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_UINT){}

  /// Associate with long variable
  FXDataTarget(FXlong& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_LONG){}

  /// Associate with unsigned long variable
  FXDataTarget(FXulong& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_ULONG){}

  /// Associate with float variable
  FXDataTarget(FXfloat& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_FLOAT){}

  /// Associate with double variable
  FXDataTarget(FXdouble& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_DOUBLE){}

  /// Associate with string variable
  FXDataTarget(FXString& value,FXObject* tgt=nullptr,FXSelector sel=0):data(&value),target(tgt),message(sel),type(DT_STRING){}


  /// Set the message target object for this data target
  void setTarget(FXObject *t){ target=t; }

  /// Get the message target object for this data target, if any
  FXObject* getTarget() const { return target; }


  /// Set the message identifier for this data target
  void setSelector(FXSelector sel){ message=sel; }

  /// Get the message identifier for this data target
  FXSelector getSelector() const { return message; }


  /// Return type of data its connected to
  FXuint getType() const { return type; }

  /// Return pointer to data its connected to
  void* getData() const { return data; }


  /// Associate with nothing
  void connect(){ data=nullptr; type=DT_VOID; }

  /// Associate with FXbool variable
  void connect(FXbool& value){ data=&value; type=DT_BOOL; }

  /// Associate with character variable
  void connect(FXchar& value){ data=&value; type=DT_CHAR; }

  /// Associate with unsigned character variable
  void connect(FXuchar& value){ data=&value; type=DT_UCHAR; }

  /// Associate with signed short variable
  void connect(FXshort& value){ data=&value; type=DT_SHORT; }

  /// Associate with unsigned short variable
  void connect(FXushort& value){ data=&value; type=DT_USHORT; }

  /// Associate with int variable
  void connect(FXint& value){ data=&value; type=DT_INT; }

  /// Associate with unsigned int variable
  void connect(FXuint& value){ data=&value; type=DT_UINT; }

  /// Associate with long variable
  void connect(FXlong& value){ data=&value; type=DT_LONG; }

  /// Associate with unsigned long variable
  void connect(FXulong& value){ data=&value; type=DT_ULONG; }

  /// Associate with float variable
  void connect(FXfloat& value){ data=&value; type=DT_FLOAT; }

  /// Associate with double variable
  void connect(FXdouble& value){ data=&value; type=DT_DOUBLE; }

  /// Associate with string variable
  void connect(FXString& value){ data=&value; type=DT_STRING; }


  /// Associate with nothing; also set target and message
  void connect(FXObject* tgt,FXSelector sel){ data=nullptr; target=tgt; message=sel; type=DT_VOID; }

  /// Associate with character variable; also set target and message
  void connect(FXbool& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_BOOL; }

  /// Associate with character variable; also set target and message
  void connect(FXchar& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_CHAR; }

  /// Associate with unsigned character variable; also set target and message
  void connect(FXuchar& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_UCHAR; }

  /// Associate with signed short variable; also set target and message
  void connect(FXshort& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_SHORT; }

  /// Associate with unsigned short variable; also set target and message
  void connect(FXushort& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_USHORT; }

  /// Associate with int variable; also set target and message
  void connect(FXint& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_INT; }

  /// Associate with unsigned int variable; also set target and message
  void connect(FXuint& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_UINT; }

  /// Associate with long variable; also set target and message
  void connect(FXlong& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_LONG; }

  /// Associate with unsigned long variable; also set target and message
  void connect(FXulong& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_ULONG; }

  /// Associate with float variable; also set target and message
  void connect(FXfloat& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_FLOAT; }

  /// Associate with double variable; also set target and message
  void connect(FXdouble& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_DOUBLE; }

  /// Associate with string variable; also set target and message
  void connect(FXString& value,FXObject* tgt,FXSelector sel){ data=&value; target=tgt; message=sel; type=DT_STRING; }


  /// Destroy
  virtual ~FXDataTarget();
  };

}

#endif
