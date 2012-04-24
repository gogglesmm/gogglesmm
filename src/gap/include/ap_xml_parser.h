/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_XML_PARSER_H
#define AP_XML_PARSER_H

namespace ap {

class GMAPI XMLStream {
protected:
  void *       parser;
  FXint        depth;
  FXint        skip;
private:
  static void xml_element_start(void*,const FXchar*,const FXchar**);
  static void xml_element_end(void*,const FXchar*);
  static void xml_element_data(void*,const FXchar*,FXint);
protected:
  virtual FXint begin(const FXchar *,const FXchar**) { return 1;}
  virtual void data(const FXchar *,FXint) {}
  virtual void end(const FXchar *){}
  void xml_print_error();
public:
  XMLStream();
  FXbool parse(const FXchar * buffer,FXint length);
  FXbool parse(const FXString & buffer);
  virtual ~XMLStream();
  };

}
#endif

