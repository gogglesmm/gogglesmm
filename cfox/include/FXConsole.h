/********************************************************************************
*                                                                               *
*                         C o n s o l e   W i d g e t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXCONSOLE_H
#define FXCONSOLE_H

#ifndef FXSCROLLAREA_H
#include "FXScrollArea.h"
#endif

#ifndef FXARRAY_H
#include "FXArray.h"
#endif


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////

namespace FX {


/// Scrollbar options
enum {
  CONSOLE_WRAPLINES = 0x00100000        /// Wrap lines
  };


// Strings buffer
typedef FXArray<FXString>  FXStringBuffer;


/**
* The console widget is a scrolling text widget used primarily
* for logging purposes and "terminals"  It is high-performance
* and features bounded but arbitrary scroll-back capability.
*/
class FXAPI FXConsole : public FXScrollArea {
  FXDECLARE(FXConsole)
protected:
  FXStringBuffer contents;      // Text data
  FXStringBuffer style;         // Text style
  FXFont        *font;          // Text font
  FXint          margintop;     // Margins top
  FXint          marginbottom;  // Margin bottom
  FXint          marginleft;    // Margin left
  FXint          marginright;   // Margin right
  FXint          historylines;  // History lines
  FXint          visiblelines;  // Visible lines
  FXint          topline;       // Where first line is in contents
  FXint          vrows;         // Default visible rows
  FXint          vcols;         // Default visible columns
  FXColor        textColor;     // Normal text color
  FXColor        selbackColor;  // Select background color
  FXColor        seltextColor;  // Select text color
  FXColor        cursorColor;   // Cursor color
  FXString       help;          // Status line help
  FXString       tip;           // Tooltip
protected:
  FXConsole();
  FXint charWidth(FXwchar ch,FXint col) const;
  FXuint styleOf(FXint line,FXint index,FXint p,FXint c) const;
  virtual void moveContents(FXint x,FXint y);
  void drawTextFragment(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h,const FXchar *text,FXint n,FXuint sty) const;
  void drawTextLine(FXDCWindow& dc,FXint line,FXint left,FXint right) const;
  void drawContents(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h) const;
protected:
  enum {
    STYLE_MASK      = 0x00FF,   // Mask color table
    STYLE_TEXT      = 0x0100,   // Draw some content
    STYLE_SELECTED  = 0x0200,   // Selected
    STYLE_CONTROL   = 0x0400,   // Control character
    STYLE_HILITE    = 0x0800,   // Highlighted
    STYLE_ACTIVE    = 0x1000    // Active
    };
private:
  FXConsole(const FXConsole&);
  FXConsole &operator=(const FXConsole&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onXXX(FXObject*,FXSelector,void*);
public:
  enum {
    ID_XXX=FXScrollArea::ID_LAST,
    ID_LAST
    };
public:

  /// Construct console window
  FXConsole(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=3,FXint pr=3,FXint pt=2,FXint pb=2);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Perform layout
  virtual void layout();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Returns true because a text widget can receive focus
  virtual FXbool canFocus() const;

  /// Return content width
  virtual FXint getContentWidth();

  /// Return content height
  virtual FXint getContentHeight();

  /// Change text font
  void setFont(FXFont* fnt);

  /// Return text font
  FXFont* getFont() const { return font; }

  /// Change top margin
  void setMarginTop(FXint pt);

  /// Return top margin
  FXint getMarginTop() const { return margintop; }

  /// Change bottom margin
  void setMarginBottom(FXint pb);

  /// Return bottom margin
  FXint getMarginBottom() const { return marginbottom; }

  /// Change left margin
  void setMarginLeft(FXint pl);

  /// Return left margin
  FXint getMarginLeft() const { return marginleft; }

  /// Change right margin
  void setMarginRight(FXint pr);

  /// Return right margin
  FXint getMarginRight() const { return marginright; }

  /// Change history lines
  void setHistoryLines(FXint hl);

  /// Return history lines
  FXint getHistoryLines() const { return historylines; }

  /// Change number of visible rows
  void setVisibleRows(FXint rows);

  /// Return number of visible rows
  FXint getVisibleRows() const { return vrows; }

  /// Change number of visible columns
  void setVisibleColumns(FXint cols);

  /// Return number of visible columns
  FXint getVisibleColumns() const { return vcols; }

  /// Set help text
  void setHelpText(const FXString& text){ help=text; }

  /// Return help text
  FXString getHelpText() const { return help; }

  /// Set the tool tip message for this widget
  void setTipText(const FXString& text){ tip=text; }

  /// Get the tool tip message for this widget
  FXString getTipText() const { return tip; }

  /// Save to a stream
  virtual void save(FXStream& store) const;

  /// Load from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXConsole();
  };

}

#endif
