/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#ifndef GMSOURCEVIEW_H
#define GMSOURCEVIEW_H

class GMTreeList;
class GMSource;
class GMScrollFrame;

class GMSourceView : public GMScrollFrame {
FXDECLARE(GMSourceView)
protected:
  GMTreeList        * sourcelist = nullptr;
  GMHeaderButton    * sourcelistheader = nullptr;
protected:
  GMSource          * source = nullptr;
  GMSource          * sourcedrop = nullptr;
protected:
  GMSourceView() = default;
  FXbool listsources();
public:
  GMSourceView(const GMSourceView&) = delete;
  GMSourceView& operator=(const GMSourceView&) = delete;
public:
  enum {
    ID_SOURCE_LIST_HEADER = FXVerticalFrame::ID_LAST,
    ID_SOURCE_LIST,
    ID_NEW_STATION,
    ID_EXPORT,
    ID_LAST,
    };
public:
  long onCmdSourceSelected(FXObject*,FXSelector,void*);
  long onCmdSortSourceList(FXObject*,FXSelector,void*);
  long onSourceContextMenu(FXObject*,FXSelector,void*);
  long onSourceTipText(FXObject*,FXSelector,void*);
  long onDndSourceMotion(FXObject*,FXSelector,void*);
  long onDndSourceDrop(FXObject*,FXSelector,void*);
  long onCmdNewStation(FXObject*,FXSelector,void*);
  long onCmdExport(FXObject*,FXSelector,void*);
  long onUpdExport(FXObject*,FXSelector,void*);
public:
  GMSourceView(FXComposite* p);

  void updateColors();

  void updateSource(GMSource * src);

  void setSource(GMSource * src,FXbool makecurrent=true);

  [[nodiscard]] GMSource * getSource() const { return source; }

  FXbool listSources();

  void sortSources() const;

  void resort();

  void init();

  void refresh();

  void refresh(GMSource * src);

  void clear();

  void loadSettings(const FXString & key);

  void saveSettings(const FXString & key) const;

  void saveView() const;

  [[nodiscard]] GMTreeList * getSourceList() const { return sourcelist; }

  FXbool focusNext();

  ~GMSourceView() override = default;
  };

#endif
