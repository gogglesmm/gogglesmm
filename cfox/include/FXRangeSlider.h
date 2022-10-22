/********************************************************************************
*                                                                               *
*                           S l i d e r   W i d g e t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXRANGERANGESLIDER_H
#define FXRANGERANGESLIDER_H

#ifndef FXFRAME_H
#include "FXFrame.h"
#endif

namespace FX {


/// Range Slider Control styles
enum {
  RANGESLIDER_HORIZONTAL   = 0,                         /// Slider shown horizontally
  RANGESLIDER_VERTICAL     = 0x00008000,                /// Slider shown vertically
  RANGESLIDER_ARROW_UP     = 0x00010000,                /// Slider has arrow head pointing up
  RANGESLIDER_ARROW_DOWN   = 0x00020000,                /// Slider has arrow head pointing down
  RANGESLIDER_ARROW_LEFT   = RANGESLIDER_ARROW_UP,      /// Slider has arrow head pointing left
  RANGESLIDER_ARROW_RIGHT  = RANGESLIDER_ARROW_DOWN,    /// Slider has arrow head pointing right
  RANGESLIDER_INSIDE_BAR   = 0x00040000,                /// Slider is inside the slot rather than overhanging
  RANGESLIDER_NORMAL       = RANGESLIDER_HORIZONTAL
  };


/**
* The range slider widget is a valuator widget which determines a subrange of the
* given range.  It has two heads, one for the lower and one for the upper range.
* Two visual appearances are supported:- the sunken look, which is enabled with
* the RANGESLIDER_INSIDE_BAR option and the regular look.  The latter may have optional
* arrows on the slider thumb.
* While being moved, the slider sends a SEL_CHANGED message to its target;
* at the end of the interaction, a SEL_COMMAND message is sent.
* The message data represents the current slider value, a pointer to an array
* of two FXint's, representing the values of the lower and upper positions.
*/
class FXAPI FXRangeSlider : public FXFrame {
  FXDECLARE(FXRangeSlider)
protected:
  FXint         headPos[2];     // Head position
  FXint         headSize;       // Head size
  FXint         slotSize;       // Slot size
  FXColor       slotColor;      // Color of slot the head moves in
  FXint         dragPoint;      // Where the head is grabbed
  FXint         values[4];      // Slider values
  FXint         active;         // Which head is being manipulated
  FXint         incr;           // Increment when auto-sliding
  FXString      help;           // Help string
  FXString      tip;            // Tip string
protected:
  FXRangeSlider();
  void drawSliderHead(FXDCWindow& dc,FXint x,FXint y,FXint w,FXint h);
private:
  FXRangeSlider(const FXRangeSlider&);
  FXRangeSlider &operator=(const FXRangeSlider&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onMouseWheel(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onMiddleBtnPress(FXObject*,FXSelector,void*);
  long onMiddleBtnRelease(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onAutoSlide(FXObject*,FXSelector,void*);
  long onCmdSetIntRange(FXObject*,FXSelector,void*);
  long onCmdGetIntRange(FXObject*,FXSelector,void*);
  long onCmdSetRealRange(FXObject*,FXSelector,void*);
  long onCmdGetRealRange(FXObject*,FXSelector,void*);
  long onCmdSetHelp(FXObject*,FXSelector,void*);
  long onCmdGetHelp(FXObject*,FXSelector,void*);
  long onCmdSetTip(FXObject*,FXSelector,void*);
  long onCmdGetTip(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
public:
  enum{
    ID_AUTOSLIDE=FXFrame::ID_LAST,
    ID_LAST
    };
public:

  /// Construct a slider widget
  FXRangeSlider(FXComposite* p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=RANGESLIDER_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0);

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Returns true because a slider can receive focus
  virtual FXbool canFocus() const;

  /// Perform layout
  virtual void layout();

  /// Enable the slider
  virtual void enable();

  /// Disable the slider
  virtual void disable();

  /// Change slider values
  void setValue(FXint head,FXint value,FXbool notify=false);

  /// Return slider value
  FXint getValue(FXint head) const { return values[1+head]; }

  /// Change the slider's range
  void setRange(FXint lo,FXint hi,FXbool notify=false);

  /// Get the slider's current range
  void getRange(FXint& lo,FXint& hi) const { lo=values[0]; hi=values[3]; }

  /// Change the slider style
  FXuint getSliderStyle() const;

  /// Get the current slider style
  void setSliderStyle(FXuint style);

  /// Get the slider's head size
  FXint getHeadSize() const { return headSize; }

  /// Change the slider's head size
  void setHeadSize(FXint hs);

  /// Get the slider's current slot size
  FXint getSlotSize() const { return slotSize; }

  /// Change the slider's slot size
  void setSlotSize(FXint bs);

  /// Get the slider's auto-increment/decrement value
  FXint getIncrement() const { return incr; }

  /// Change the slider's auto-increment/decrement value
  void setIncrement(FXint inc);

  /// Change the color of the slot the slider head moves in
  void setSlotColor(FXColor clr);

  /// Get the current slot color
  FXColor getSlotColor() const { return slotColor; }

  /// Set the help text to be displayed on the status line
  void setHelpText(const FXString& text){ help=text; }

  /// Get the current help text
  const FXString& getHelpText() const { return help; }

  /// Set the tip text to be displayed in the tooltip
  void setTipText(const FXString& text){ tip=text; }

  /// Get the current tooltip text value
  const FXString& getTipText() const { return tip; }

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destroy the slider
  virtual ~FXRangeSlider();
  };

}

#endif
