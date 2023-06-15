/********************************************************************************
*                                                                               *
*                      T e x t   R e p l a c e   D i a l o g                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXREPLACEDIALOG_H
#define FXREPLACEDIALOG_H

#ifndef FXDIALOGBOX_H
#include "FXDialogBox.h"
#endif

namespace FX {


class FXButton;
class FXLabel;
class FXTextField;
class FXHorizontalFrame;


/**
* Search and Replace Dialog.
* This dialog allows the user to enter a string or pattern to
* search for, and a replace string to replace the matched contents with.
* Some options can control the search process: case-sensitivity, regular
* expression searches, and search direction (forward or backward).
* For convenience, the most recent search and replace strings are remembered,
* and can be quickly recalled by scrolling back in the respective entry-boxes.
* The flags controlling case-sensitivity, pattern search, and search direction
* are also remembered and will automatically be reinstated when the corresponding
* search string is selected.
* Note, the dialog does not itself perform a search, it just provided a convenient
* entry box for the search and replace parameters.
* A history is kept of past strings and flags that we searched; this can be quickly
* recalled by using the arrow-buttons.
*/
class FXAPI FXReplaceDialog : public FXDialogBox {
  FXDECLARE(FXReplaceDialog)
protected:
  FXLabel           *searchlabel;
  FXTextField       *searchtext;
  FXHorizontalFrame *searchbox;
  FXLabel           *replacelabel;
  FXTextField       *replacetext;
  FXHorizontalFrame *replacebox;
  FXButton          *search;
  FXButton          *replace;
  FXButton          *replacesel;
  FXButton          *replaceall;
  FXButton          *cancel;
  FXString           searchHistory[20]; // Search string history
  FXString           replacHistory[20]; // Replace string history
  FXuint             optionHistory[20]; // Options history
  FXint              activeHistory;     // Current active history index
  FXuint             searchmode;        // Search mode
protected:
  static const FXchar sectionName[];
protected:
  FXReplaceDialog(){}
  void appendHistory(const FXString& pat,const FXString& sub,FXuint opt);
  void loadHistory();
  void saveHistory();
private:
  FXReplaceDialog(const FXReplaceDialog&);
  FXReplaceDialog &operator=(const FXReplaceDialog&);
public:
  long onUpdDir(FXObject*,FXSelector,void*);
  long onCmdDir(FXObject*,FXSelector,void*);
  long onUpdWrap(FXObject*,FXSelector,void*);
  long onCmdWrap(FXObject*,FXSelector,void*);
  long onUpdCase(FXObject*,FXSelector,void*);
  long onCmdCase(FXObject*,FXSelector,void*);
  long onUpdWords(FXObject*,FXSelector,void*);
  long onCmdWords(FXObject*,FXSelector,void*);
  long onUpdRegex(FXObject*,FXSelector,void*);
  long onCmdRegex(FXObject*,FXSelector,void*);
  long onSearchKey(FXObject*,FXSelector,void*);
  long onReplaceKey(FXObject*,FXSelector,void*);
  long onCmdSearchHistUp(FXObject*,FXSelector,void*);
  long onCmdSearchHistDn(FXObject*,FXSelector,void*);
  long onCmdReplaceHistUp(FXObject*,FXSelector,void*);
  long onCmdReplaceHistDn(FXObject*,FXSelector,void*);
  long onWheelSearch(FXObject*,FXSelector,void*);
  long onWheelReplace(FXObject*,FXSelector,void*);
  long onCmdSearch(FXObject*,FXSelector,void*);
  long onUpdSearch(FXObject*,FXSelector,void*);
  long onCmdReplace(FXObject*,FXSelector,void*);
  long onCmdReplaceSel(FXObject*,FXSelector,void*);
  long onCmdReplaceAll(FXObject*,FXSelector,void*);
public:
  enum{
    ID_SEARCH_UP=FXDialogBox::ID_LAST,
    ID_SEARCH_DN,
    ID_REPLACE_UP,
    ID_REPLACE_DN,
    ID_SEARCH,
    ID_SEARCH_NEXT,
    ID_SEARCH_PREV,
    ID_REPLACE,
    ID_REPLACE_SEL,
    ID_REPLACE_ALL,
    ID_DIR,
    ID_CASE,
    ID_WORDS,
    ID_REGEX,
    ID_WRAP,
    ID_SEARCH_TEXT,
    ID_REPLACE_TEXT,
    ID_LAST
    };
public:
  enum {
    DONE          = 0,    /// Cancel search
    SEARCH        = 1,    /// Search for pattern
    REPLACE       = 2,    /// Replace first occurrence
    REPLACE_SEL   = 3,    /// Replace occurrences in selection
    REPLACE_ALL   = 4     /// Replace all occurrences
    };
public:

  /// Construct search and replace dialog box
  FXReplaceDialog(FXWindow* owner,const FXString& caption,FXIcon* ic=nullptr,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Set text or pattern to search for
  void setSearchText(const FXString& text);

  /// Return text or pattern the user has entered
  FXString getSearchText() const;

  /// Set replace text
  void setReplaceText(const FXString& text);

  /// Return replace text the user has entered
  FXString getReplaceText() const;

  /// Set search match mode
  void setSearchMode(FXuint mode){ searchmode=mode; }

  /// Return search mode the user has selected
  FXuint getSearchMode() const { return searchmode; }

  /// Change search text color
  void setSearchTextColor(FXColor clr);

  /// Return search text color
  FXColor getSearchTextColor() const;

  /// Change replace text color
  void setReplaceTextColor(FXColor clr);

  /// Return replace text color
  FXColor getReplaceTextColor() const;

  /// Run modal invocation of the dialog
  virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXReplaceDialog();
  };

}

#endif
