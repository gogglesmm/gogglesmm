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

class GMAPI XmlParser {
private:
  FXint* nodes;
  FXint  nnodes;
  FXint  level;
public:
  void element_start(const FXchar*,const FXchar**);
  void element_end(const FXchar * element);
  void element_data(const FXchar * d,FXint len);
protected:
  virtual FXint begin(const FXchar*,const FXchar**) { return Elem_Skip;}
  virtual void data(const FXchar *,FXint) {}
  virtual void end(const FXchar *){}
  FXint node() const { return nodes[level]; }
public:
  enum {
    Elem_Skip = 0, // Skipped Element
    Elem_None,     // Document Root
    Elem_Last,
    };
public:
  XmlParser();
  virtual FXbool parseBuffer(const FXchar * buffer,FXint length);
  FXbool parse(const FXString & buffer);
  virtual ~XmlParser();
  };


class GMAPI HtmlParser : public XmlParser {
public:
  HtmlParser();
  virtual FXbool parseBuffer(const FXchar * buffer,FXint length);
  virtual ~HtmlParser();
  };


}
#endif

