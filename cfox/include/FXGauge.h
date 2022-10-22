/********************************************************************************
*                                                                               *
*                             G a u g e   W i d g e t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2010,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXGAUGE_H
#define FXGAUGE_H

#ifndef FXFRAME_H
#include "FXFrame.h"
#endif


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


namespace FX {


/// Gauge styles
enum {
  GAUGE_NORMAL       = 0,               /// Normal gauge style
  GAUGE_PIVOT_CENTER = 0x00008000,      /// Gauge pivot in center of widget
  GAUGE_PIVOT_INSIDE = 0x00010000,      /// Keep pivot inside widget
  GAUGE_ELLIPTICAL   = 0x00020000,      /// Allow gauge to be elliptical
  GAUGE_CYCLIC       = 0x00040000       /// Dials go around full circle
  };


/**
* Gauge widget.
*/
class FXAPI FXGauge : public FXFrame {
  FXDECLARE(FXGauge)
protected:
  struct Indicator {
    FXdouble radius;            // Fraction of dial radius
    FXdouble ratio;             // Ratio of movement
    FXColor  color;             // Color
    FXbool   shown;             // Whether its shown or not
    };
protected:
  FXString      caption;        // Caption text
  FXFont       *numberFont;     // Font for number labels
  FXFont       *captionFont;    // Font for centered caption
  Indicator     indicator[3];   // Up to three indicators
  FXint         startAngle;     // Start angle of arc (ccw from x-axis)
  FXint         sweepAngle;     // Sweep angle of arc (ccw from startAngle)
  FXdouble      range[2];       // Reported data range
  FXdouble      value;          // Reported data value
FXdouble      innerRadius;    // Inside radius
  FXdouble      majorTickDelta; // Major tick delta
  FXdouble      minorTickDelta; // Minor tick delta
  FXColor       majorTickColor; // Major tickmark color
  FXColor       minorTickColor; // Major tickmark color
  FXshort       majorTickSize;  // Major tick size
  FXshort       minorTickSize;  // Minor tick size
  FXColor       faceColor;      // Dial face color
  FXColor       arcColor;       // Color of arc
  FXshort       arcWeight;      // Line weight
  FXString      help;           // Help string
  FXString      tip;            // Tip string
protected:
  FXGauge();
  void drawPointer(FXDCWindow& dc,FXdouble ang,FXint xx,FXint yy,FXint ww,FXint hh,FXint cx,FXint cy,FXint rx,FXint ry,FXint p) const;
  void drawGauge(FXDCWindow& dc,FXint xx,FXint yy,FXint ww,FXint hh,FXint cx,FXint cy,FXint rx,FXint ry) const;
private:
  FXGauge(const FXGauge&);
  FXGauge &operator=(const FXGauge&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
  long onCmdSetValue(FXObject*,FXSelector,void*);
  long onCmdSetIntValue(FXObject*,FXSelector,void*);
  long onCmdGetIntValue(FXObject*,FXSelector,void*);
  long onCmdSetRealValue(FXObject*,FXSelector,void*);
  long onCmdGetRealValue(FXObject*,FXSelector,void*);
  long onCmdSetLongValue(FXObject*,FXSelector,void*);
  long onCmdGetLongValue(FXObject*,FXSelector,void*);
  long onCmdSetIntRange(FXObject*,FXSelector,void*);
  long onCmdGetIntRange(FXObject*,FXSelector,void*);
  long onCmdSetRealRange(FXObject*,FXSelector,void*);
  long onCmdGetRealRange(FXObject*,FXSelector,void*);
  long onCmdSetHelp(FXObject*,FXSelector,void*);
  long onCmdGetHelp(FXObject*,FXSelector,void*);
  long onCmdSetTip(FXObject*,FXSelector,void*);
  long onCmdGetTip(FXObject*,FXSelector,void*);
public:

  /// Construct Gauge widget
  FXGauge(FXComposite* p,FXuint opts=FRAME_NORMAL,FXint startang=180,FXint sweepang=-180,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Set the text for caption
  void setCaption(const FXString& text);

  /// Get the text for caption
  FXString getCaption() const { return caption; }

  /// Set start angle in degrees ccw
  void setStartAngle(FXint degrees);

  /// Return start angle in degrees ccw
  FXint getStartAngle() const { return startAngle; }

  /// Set sweep angle in degrees ccw
  void setSweepAngle(FXint degrees);

  /// Return sweep angle in degrees ccw
  FXint getSweepAngle() const { return sweepAngle; }

  /// Change current value
  void setValue(FXdouble v,FXbool notify=false);

  /// Get current value
  FXdouble getValue() const { return value; }

  /// Change the gauge range
  void setRange(FXdouble lo,FXdouble hi,FXbool notify=false);

  /// Get the gauge range
  void getRange(FXdouble& lo,FXdouble& hi) const { lo=range[0]; hi=range[1]; }

  /// Change major tick space
  void setMajorTickDelta(FXdouble delta);

  /// Get major tick space
  FXdouble getMajorTickDelta() const { return majorTickDelta; }

  /// Change minor tick space
  void setMinorTickDelta(FXdouble delta);

  /// Get minor tick space
  FXdouble getMinorTickDelta() const { return minorTickDelta; }

  /// Set the current gauge style
  void setGaugeStyle(FXuint style);

  /// Get the gauge style
  FXuint getGaugeStyle() const;

  /// Set the number font
  void setNumberFont(FXFont* fnt);

  /// Get the number font
  FXFont* getNumberFont() const { return numberFont; }

  /// Set the caption font
  void setCaptionFont(FXFont* fnt);

  /// Get the caption font
  FXFont* getCaptionFont() const { return captionFont; }

  /// Set the help text to be displayed on the status line
  void setHelpText(const FXString& text){ help=text; }

  /// Get the current help text
  const FXString& getHelpText() const { return help; }

  /// Set the tip text to be displayed in the tooltip
  void setTipText(const FXString& text){ tip=text; }

  /// Get the current tooltip text value
  const FXString& getTipText() const { return tip; }

  /// Save gauge to a stream
  virtual void save(FXStream& store) const;

  /// Load gauge from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXGauge();
  };


}

#endif
