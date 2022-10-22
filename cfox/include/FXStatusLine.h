/********************************************************************************
*                                                                               *
*                       S t a t u s L i n e   W i d g e t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXSTATUSLINE_H
#define FXSTATUSLINE_H

#ifndef FXFRAME_H
#include "FXFrame.h"
#endif

namespace FX {


/**
* The status line normally shows its permanent message.
* A semi-permanent message can override this permanent message, for example to
* indicate the application is busy or in a particular operating mode.
* The status line obtains the semi-permanent message by sending its target (if any)
* SEL_UPDATE message.
* A ID_SETSTRINGVALUE can be used to change the status message.
* When the user moves the cursor over a widget which has status-line help, the
* status line can flash a very temporarily message with help about the widget.
* For example, the status line may flash the "Quit Application" message when
* the user moves the cursor over the Quit button.
* The status line obtains the help message from the control by sending it a
* ID_QUERY_HELP message with type SEL_UPDATE.
* Unless the value is overridden, the status line will display the normal text,
* i.e. the string set via setNormalText().
* If the message contains a newline (\n), then the part before the newline
* will be displayed in the highlight color, while the part after the newline
* is shown using the normal text color.
*/
class FXAPI FXStatusLine : public FXFrame {
  FXDECLARE(FXStatusLine)
protected:
  FXString  status;             // Current status message
  FXString  normal;             // Normally displayed message
  FXFont   *font;               // Font
  FXColor   textColor;          // Status text color
  FXColor   textHighlightColor; // Status text highlight color
protected:
  FXStatusLine();
private:
  FXStatusLine(const FXStatusLine&);
  FXStatusLine& operator=(const FXStatusLine&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onUpdate(FXObject*,FXSelector,void*);
  long onCmdGetStringValue(FXObject*,FXSelector,void*);
  long onCmdSetStringValue(FXObject*,FXSelector,void*);
public:

  /// Constructor
  FXStatusLine(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Change the temporary status message
  void setText(const FXString& text);

  /// Return the temporary status message
  FXString getText() const { return status; }

  /// Change the permanent status message
  void setNormalText(const FXString& text);

  /// Return the permanent status message
  FXString getNormalText() const { return normal; }

  /// Change the font
  void setFont(FXFont* fnt);

  /// Return the current font
  FXFont* getFont() const { return font; }

  /// Return the text color
  FXColor getTextColor() const { return textColor; }

  /// Change the text color
  void setTextColor(FXColor clr);

  /// Return the highlight text color
  FXColor getTextHighlightColor() const { return textHighlightColor; }

  /// Change the highlight text color
  void setTextHighlightColor(FXColor clr);

  /// Save status line to stream
  virtual void save(FXStream& store) const;

  /// Load status line from stream
  virtual void load(FXStream& store);

  /// Destroy
  virtual ~FXStatusLine();
  };

}

#endif
