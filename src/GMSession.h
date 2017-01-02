/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2012-2017 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMSESSION_H
#define GMSESSION_H

class SMClient;

// Desktop Session Manager Interface
class GMSession : public FXObject {
FXDECLARE(GMSession)
private:
  friend class SMClient;
private:
  SMClient * smclient = nullptr;
protected:
  FXObject * target = nullptr;
  FXSelector message = 0;
protected:
  void quit();
protected:
  GMSession(){}
public:
  GMSession(FXApp*,FXObject*,FXSelector sel);

  FXbool init(int argc,const FXchar * const argv[]);

  ~GMSession();
  };
#endif
