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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegion.h"
#include "FXFont.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"
#include "FXGauge.h"


/*
  Notes:
  - Unless GAUGE_ELLIPTICAL is passed, dial will be segment of a circle.
  - GAUGE_CYCLIC means value is unconstrained, and dial indicators go around cyclically.
  - GAUGE_PIVOT_CENTER means fix dial pivot at center of control, instead of setting dial
    pivot at optimum position inside control.  If GAUGE_PIVOT_CENTER is not passed, the
    pivor may be off center, and possibly outside of the control unless GAUGE_PIVOT_INSIDE
    is also passed.
  - Layout for non-elliptical gauge without pivot centered is a bit complicated.

      o First, we compare aspect ratios of gauge and aspect ratio of the window.  If the
        gauge is flatter than the window, we adjust vertical gauge size; otherwise, we adjust
        horizontal gauge size.
      o Next, we try to shift the position of the gauge so as to make as much of the pivot
        location visible as possible.

      o If any space left over and the entire gauge is visible, equal amounts of space are
        apportioned in either side of gauge.

  - The rx and ry ellipse radii are based on the full gauge arc, even if the full arc is
    not drawn.
  - FIXME:
     - draw to off-screen dial.
     - only indicators drawn in onPaint().
     - startAngle and sweepAngle are angles of tickmarked axis.
     - extraAngle is extra space (angle) before first/after last tick.
     - extraAngle is such that extremes don't touch if going on to full-circle.
     - Placement of caption?
*/

#define GAUGE_MASK  (GAUGE_PIVOT_CENTER|GAUGE_PIVOT_INSIDE|GAUGE_ELLIPTICAL|GAUGE_CYCLIC)

#define GAUGERADIUS 100         // Default gauge radius

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXGauge) FXGaugeMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXGauge::onPaint),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXGauge::onQueryTip),
  FXMAPFUNC(SEL_QUERY_HELP,0,FXGauge::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETVALUE,FXGauge::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETINTVALUE,FXGauge::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETINTVALUE,FXGauge::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETREALVALUE,FXGauge::onCmdSetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETREALVALUE,FXGauge::onCmdGetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETLONGVALUE,FXGauge::onCmdSetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETLONGVALUE,FXGauge::onCmdGetLongValue),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETINTRANGE,FXGauge::onCmdSetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETINTRANGE,FXGauge::onCmdGetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETREALRANGE,FXGauge::onCmdSetRealRange),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETREALRANGE,FXGauge::onCmdGetRealRange),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETHELPSTRING,FXGauge::onCmdSetHelp),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETHELPSTRING,FXGauge::onCmdGetHelp),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_SETTIPSTRING,FXGauge::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXGauge::ID_GETTIPSTRING,FXGauge::onCmdGetTip),
  };


// Object implementation
FXIMPLEMENT(FXGauge,FXFrame,FXGaugeMap,ARRAYNUMBER(FXGaugeMap))


// Deserialization
FXGauge::FXGauge(){
  numberFont=(FXFont*)-1L;
  captionFont=(FXFont*)-1L;
  indicator[0].radius=0.0;
  indicator[0].ratio=0.0;
  indicator[0].color=0;
  indicator[0].shown=false;
  indicator[1].radius=0.0;
  indicator[1].ratio=0.0;
  indicator[1].color=0;
  indicator[1].shown=false;
  indicator[2].radius=0.0;
  indicator[2].ratio=0.0;
  indicator[2].color=0;
  indicator[2].shown=false;
  startAngle=180;
  sweepAngle=-360;
  range[0]=0.0;
  range[1]=0.0;
  value=0.0;
innerRadius=0.0;
  majorTickDelta=0.0;
  minorTickDelta=0.0;
  majorTickColor=0;
  minorTickColor=0;
  majorTickSize=0;
  minorTickSize=0;
  faceColor=0;
  arcColor=0;
  arcWeight=0;
  }


// Make gauge
FXGauge::FXGauge(FXComposite* p,FXuint opts,FXint startang,FXint sweepang,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):FXFrame(p,opts,x,y,w,h,pl,pr,pt,pb){
  numberFont=getApp()->getNormalFont();
  captionFont=getApp()->getNormalFont();
  indicator[0].radius=1.0;
  indicator[0].ratio=1.0;
  indicator[0].color=FXRGB(0,0,0);
  indicator[0].shown=true;
  indicator[1].radius=0.9;
  indicator[1].ratio=0.1;
  indicator[1].color=FXRGB(0,0,0);
  indicator[1].shown=false;
  indicator[2].radius=0.6;
  indicator[2].ratio=0.01;
  indicator[2].color=FXRGB(0,0,0);
  indicator[2].shown=false;
  if(sweepang==0) sweepang=1;
  startAngle=FXCLAMP(-360,startang,360);
  sweepAngle=FXCLAMP(-360,sweepang,360);
  range[0]=0.0;
  range[1]=1.0;
  value=0.0;
innerRadius=0.5;
  majorTickDelta=0.1;
  minorTickDelta=0.01;
  majorTickColor=FXRGB(0,0,0);
  minorTickColor=FXRGB(0,0,0);
  majorTickSize=5;
  minorTickSize=3;
  faceColor=hiliteColor;
  arcColor=shadowColor;
  arcWeight=3;
  }


// Get minimum width
FXint FXGauge::getDefaultWidth(){
  return GAUGERADIUS+padleft+padright+(border<<1);
  }


// Get minimum height
FXint FXGauge::getDefaultHeight(){
  return GAUGERADIUS+padtop+padbottom+(border<<1);
  }


// Update value from a message
long FXGauge::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXGauge::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXGauge::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=(FXint)getValue();
  return 1;
  }


// Update value from a message
long FXGauge::onCmdSetLongValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)*((FXlong*)ptr));
  return 1;
  }


// Obtain value with a message
long FXGauge::onCmdGetLongValue(FXObject*,FXSelector,void* ptr){
  *((FXlong*)ptr)=(FXlong)getValue();
  return 1;
  }


// Update value from a message
long FXGauge::onCmdSetRealValue(FXObject*,FXSelector,void* ptr){
  setValue(*((FXdouble*)ptr));
  return 1;
  }


// Obtain value with a message
long FXGauge::onCmdGetRealValue(FXObject*,FXSelector,void* ptr){
  *((FXdouble*)ptr)=getValue();
  return 1;
  }


// Update range from a message
long FXGauge::onCmdSetIntRange(FXObject*,FXSelector,void* ptr){
  setRange((FXdouble)((FXint*)ptr)[0],(FXdouble)((FXint*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXGauge::onCmdGetIntRange(FXObject*,FXSelector,void* ptr){
  ((FXint*)ptr)[0]=(FXint)range[0];
  ((FXint*)ptr)[1]=(FXint)range[1];
  return 1;
  }


// Update range from a message
long FXGauge::onCmdSetRealRange(FXObject*,FXSelector,void* ptr){
  setRange(((FXdouble*)ptr)[0],((FXdouble*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXGauge::onCmdGetRealRange(FXObject*,FXSelector,void* ptr){
  ((FXdouble*)ptr)[0]=range[0];
  ((FXdouble*)ptr)[1]=range[1];
  return 1;
  }


// Set help using a message
long FXGauge::onCmdSetHelp(FXObject*,FXSelector,void* ptr){
  setHelpText(*((FXString*)ptr));
  return 1;
  }


// Get help using a message
long FXGauge::onCmdGetHelp(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getHelpText();
  return 1;
  }


// Set tip using a message
long FXGauge::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXGauge::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// We were asked about tip text
long FXGauge::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// We were asked about status text
long FXGauge::onQueryHelp(FXObject* sender,FXSelector sel,void* ptr){
  if(FXFrame::onQueryHelp(sender,sel,ptr)) return 1;
  if((flags&FLAG_HELP) && !help.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }

/*
dc.setFont(font);
font->getFontHeight();
font->getTextWidth(&text[beg],end-beg);
font->getFontAscent();
dc.drawText(xx,yy,&text[beg],end-beg);
*/

 // FIXME use image, ONLY draw arrows in onPaint, BLIT the rest.

// Draw only one pointer
void FXGauge::drawPointer(FXDCWindow& dc,FXdouble ang,FXint xx,FXint yy,FXint ww,FXint hh,FXint cx,FXint cy,FXint rx,FXint ry,FXint p) const {
  FXdouble angle=startAngle*DTOR+ang;
  FXdouble r=indicator[p].radius;
  dc.setForeground(indicator[p].color);
  dc.drawLine(cx,cy,(FXint)(0.5+cx+Math::cos(angle)*r*rx),(FXint)(0.5+cy-Math::sin(angle)*r*ry));
  }


// Draw dial face
void FXGauge::drawGauge(FXDCWindow& dc,FXint xx,FXint yy,FXint ww,FXint hh,FXint cx,FXint cy,FXint rx,FXint ry) const {
  FXint rrx=(FXint)(rx*innerRadius);
  FXint rry=(FXint)(ry*innerRadius);
  FXdouble sa=startAngle*DTOR;
  FXdouble ea=(startAngle+sweepAngle)*DTOR;
  FXdouble ma=(sa+ea)*0.5;
  FXdouble csa=Math::cos(sa);
  FXdouble ssa=Math::sin(sa);
  FXdouble cea=Math::cos(ea);
  FXdouble sea=Math::sin(ea);

  // Draw the arc
  if(faceColor!=backColor){
    dc.setForeground(faceColor);
    dc.fillArc(cx-rx,cy-ry,rx<<1,ry<<1,startAngle*64,sweepAngle*64);
    dc.setForeground(backColor);
    dc.fillArc(cx-rrx,cy-rry,rrx<<1,rry<<1,startAngle*64,sweepAngle*64);
    }

  // Draw arc
  dc.setForeground(arcColor);
  dc.setLineWidth(arcWeight);
  dc.drawArc(cx-rx,cy-ry,rx<<1,ry<<1,startAngle*64,sweepAngle*64);
  dc.drawArc(cx-rrx,cy-rry,rrx<<1,rry<<1,startAngle*64,sweepAngle*64);
  dc.drawLine((FXint)(0.5+cx+csa*rrx),(FXint)(0.5+cy-ssa*rry),(FXint)(0.5+cx+csa*rx),(FXint)(0.5+cy-ssa*ry));
  dc.drawLine((FXint)(0.5+cx+cea*rrx),(FXint)(0.5+cy-sea*rry),(FXint)(0.5+cx+cea*rx),(FXint)(0.5+cy-sea*ry));
  dc.setLineWidth(0);

//  dc.setForeground(FXRGB(0,0,0));

//  dc.drawLine(cx,cy,(FXint)(0.5+cx+FX::cos(ma)*rx),(FXint)(0.5+cy-FX::sin(ma)*ry));
//  dc.fillArc(cx-3,cy-3,7,7,0,23040);
//  dc.fillArc((FXint)(0.5+cx+csa*rx)-3,(FXint)(0.5+cy-ssa*ry)-3,7,7,0,23040);
//  dc.fillArc((FXint)(0.5+cx+cea*rx)-3,(FXint)(0.5+cy-sea*ry)-3,7,7,0,23040);

//  dc.fillArc((FXint)(0.5+cx+FX::cos(ma)*rx)-3,(FXint)(0.5+cy-FX::sin(ma)*ry)-3,7,7,0,23040);

//  dc.setForeground(FXRGB(255,0,0));
//  dc.drawLine(cx,yy,cx,yy+hh);
//  dc.drawLine(xx,cy,xx+ww,cy);

#if 0
  // Draw indicators
  if(indicator[0].shown){
    drawPointer(dc,0.0,xx,yy,ww,hh,cx,cy,rx,ry,0);
    if(indicator[1].shown){
      drawPointer(dc,0.0,xx,yy,ww,hh,cx,cy,rx,ry,1);
      if(indicator[2].shown){
        drawPointer(dc,0.0,xx,yy,ww,hh,cx,cy,rx,ry,2);
        }
      }
    }
#endif
  }


// Draw the gauge
long FXGauge::onPaint(FXObject*,FXSelector,void *ptr){
  FXDCWindow dc(this,(FXEvent*)ptr);
  FXint xx,yy,ww,hh,cenx,ceny,cx,cy,rx,ry,sa,ea,sw;
  FXdouble csa,ssa,cea,sea,abox,W,H,E;
  FXdouble xmin,xmax,ymin,ymax;

  // Depend on it
  FXASSERT(-360<=startAngle && startAngle<=360);
  FXASSERT(-360<=sweepAngle && sweepAngle<=360);

  // Content placement
  xx=border+padleft;
  yy=border+padtop;
  ww=width-(border<<1)-padleft-padright;
  hh=height-(border<<1)-padtop-padbottom;

  // Repaint background
  dc.setForeground(backColor);
  dc.fillRectangle(0,0,width,height);

  // Repaint border
  drawFrame(dc,0,0,width,height);

  // Don't bother if too small
  if(1<ww && 1<hh){

    // Tentative arc radii
    if(options&GAUGE_ELLIPTICAL){
      rx=ww/2;
      ry=hh/2;
      }
    else{
      rx=ry=FXMIN(ww,hh)/2;
      }

    // Tentative pivot center
    cx=cenx=border+padleft+(ww>>1);
    cy=ceny=border+padtop+(hh>>1);

    // Things are complicated if pivot not at center
    if((FXABS(sweepAngle)<330) || !(options&GAUGE_PIVOT_CENTER)){

      // Adjust sa in [-360..-1] and sw in [0..360]
      if(sweepAngle<0){
        sa=startAngle+sweepAngle;
        sw=-sweepAngle;
        }
      else{
        sa=startAngle;
        sw=sweepAngle;
        }
      sa=(720+sa)%360-360;
      ea=sa+sw;

      //FXTRACE((100,"sa=%d ea=%d sw=%d\n",sa,ea,sw));

      // Include endpoints of arc
      csa=Math::cos(DTOR*sa);
      ssa=Math::sin(DTOR*sa);
      cea=Math::cos(DTOR*ea);
      sea=Math::sin(DTOR*ea);
      FXMINMAX(xmin,xmax,csa,cea);
      FXMINMAX(ymin,ymax,ssa,sea);

      // Include major axes arc sweeps past
      if(360<=sa+sw) xmax=1.0;
      if(sa+360<=90) sa+=360;
      if(sa+sw>=90) ymax=1.0;
      if(sa+360<=180) sa+=360;
      if(sa+sw>=180) xmin=-1.0;
      if(sa+360<=270) sa+=360;
      if(sa+sw>=270) ymin=-1.0;

      // Include pivot if desired
      if(options&GAUGE_PIVOT_INSIDE){
        if(0.0<xmin) xmin=0.0;
        if(0.0>xmax) xmax=0.0;
        if(0.0<ymin) ymin=0.0;
        if(0.0>ymax) ymax=0.0;
        }

      // We should have non-zero box
      FXASSERT(-1.0<=xmin && xmin<xmax && xmax<=1.0);
      FXASSERT(-1.0<=ymin && ymin<ymax && ymax<=1.0);

      FXTRACE((100,"xmin=%.3lf xmax=%.3lf ymin=%.3lf ymax=%.3lf\n",xmin,xmax,ymin,ymax));

      W=xmax-xmin;
      H=ymax-ymin;

      FXTRACE((100,"was W=%lf H=%lf\n",W,H));

      // Keep circular
      if(!(options&GAUGE_ELLIPTICAL)){
        abox=H/W;
        if(abox*ww<hh){                                   // Box aspect wider than window
          FXTRACE((100,"Box aspect wider than window\n"));
          E=-H;
          H=hh*(W/ww);                                      // New box height
          E+=H;

          FXTRACE((1,"E=%.3lf\n",E));

          // Pivot above box
          if(0.0>ymax){
            if(-ymax<E){ E+=ymax; ymax=0.0; } else { ymax+=E; E=0.0; }    // Expand box on the top
            }

          // Pivot belox box
          else if(0.0<ymin){
            if(ymin<E){ E-=ymin; ymin=0.0; } else { ymin-=E; E=0.0; }     // Expand box on the bottom
            }

          // Expand both sides evenly
          ymin-=0.5*E;
          ymax+=0.5*E;
          }
        else{                                             // Box aspect taller than window
          FXTRACE((100,"Box aspect taller than window\n"));
          E=-W;
          W=ww*H/hh;                                      // New box width
          E+=W;

          FXTRACE((100,"E=%.3lf\n",E));

          // Pivot right of box
          if(0.0>xmax){
            if(-xmax<E){ E+=xmax; xmax=0.0; } else { xmax+=E; E=0.0; }    // Expand box on the right
            }

          // Pivot left of box
          else if(0.0<xmin){
            if(xmin<E){ E-=xmin; xmin=0.0; } else { xmin-=E; E=0.0; }     // Expand box on the left
            }

          // Expand both sides evenly
          xmin-=0.5*E;
          xmax+=0.5*E;
          }
        FXTRACE((100,"xmin=%.3lf xmax=%.3lf ymin=%.3lf ymax=%.3lf\n",xmin,xmax,ymin,ymax));
        }

      FXTRACE((100,"now W=%lf H=%lf\n",W,H));

      FXASSERT(0.0<W && 0.0<H);

      // Arc radii
      rx=ww/W;
      ry=hh/H;

      // Pivot center (may be outside of window)
      cx=cenx-(FXint)(0.5+0.5*(xmax+xmin)*rx);
      cy=ceny+(FXint)(0.5+0.5*(ymax+ymin)*ry);
      }

    FXTRACE((100,"width=%d height=%d\n",width,height));
    FXTRACE((100,"xx=%d yy=%d ww=%d hh=%d\n",xx,yy,ww,hh));
    FXTRACE((100,"cx=%d cy=%d\n",cx,cy));
    FXTRACE((100,"rx=%d ry=%d\n",rx,ry));
    FXTRACE((100,"\n"));

    // Don't draw over borders
    dc.setClipRectangle(border,border,width-(border<<1),height-(border<<1));

    // Draw dial
    drawGauge(dc,xx,yy,ww,hh,cx,cy,rx,ry);
    }
  return 1;
  }


// Set the text for caption
void FXGauge::setCaption(const FXString& text){
  if(caption!=text){
    caption=text;
    recalc();
    update();
    }
  }


// Set start angle in degrees ccw
void FXGauge::setStartAngle(FXint degrees){
  if(degrees<-360 || 360<degrees){ fxerror("%s::setStartAngle: argument out of range.\n",getClassName()); }
  if(startAngle!=degrees){
    startAngle=degrees;
    update();
    }
  }


// Set sweep angle in degrees ccw
void FXGauge::setSweepAngle(FXint degrees){
  if(degrees<-360 || 360<degrees || degrees==0){ fxerror("%s::setSweepAngle: argument out of range.\n",getClassName()); }
  if(sweepAngle!=degrees){
    sweepAngle=degrees;
    update();
    }
  }


// Change current value
void FXGauge::setValue(FXdouble v,FXbool notify){
  if(v<range[0]) v=range[0];
  if(v>range[1]) v=range[1];
  if(value!=v){
    value=v;
    update(border,border,width-(border<<1),height-(border<<1));
    if(notify && target){target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&value);}
    }
  }


// Change the gauge range
void FXGauge::setRange(FXdouble lo,FXdouble hi,FXbool notify){
  if(lo>hi){ fxerror("%s::setRange: trying to set negative range.\n",getClassName()); }
  if(range[0]!=lo || range[1]!=hi){
    range[0]=lo;
    range[1]=hi;
    setValue(value,notify);
    }
  }


// Change major tick space
void FXGauge::setMajorTickDelta(FXdouble delta){
  if(delta<=0.0){ fxerror("FXAxis::setMajorTickDelta: tick delta should be > 0.\n"); }
  if(majorTickDelta!=delta){
    majorTickDelta=delta;
    update(border,border,width-(border<<1),height-(border<<1));
    }
  }


// Change minor tick space
void FXGauge::setMinorTickDelta(FXdouble delta){
  if(delta<=0.0){ fxerror("FXAxis::setMinorTickDelta: tick delta should be > 0.\n"); }
  if(minorTickDelta!=delta){
    minorTickDelta=delta;
    update(border,border,width-(border<<1),height-(border<<1));
    }
  }


// Set the current gauge style
void FXGauge::setGaugeStyle(FXuint style){
  FXuint opts=(options&~GAUGE_MASK) | (style&GAUGE_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Set the number font
void FXGauge::setNumberFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setNumberFont: NULL font specified.\n",getClassName()); }
  if(numberFont!=fnt){
    numberFont=fnt;
    recalc();
    update();
    }
  }


// Set the caption font
void FXGauge::setCaptionFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setCaptionFont: NULL font specified.\n",getClassName()); }
  if(captionFont!=fnt){
    captionFont=fnt;
    recalc();
    update();
    }
  }


// Get the gauge style
FXuint FXGauge::getGaugeStyle() const {
  return (options&GAUGE_MASK);
  }


// Save object to stream
void FXGauge::save(FXStream& store) const {
  FXFrame::save(store);
  store << caption;
  store << numberFont;
  store << captionFont;
  store << startAngle;
  store << sweepAngle;
  store << range[0] << range[1];
  store << value;
  store << majorTickDelta;
  store << minorTickDelta;
  store << majorTickColor;
  store << minorTickColor;
  store << majorTickSize;
  store << minorTickSize;
  store << faceColor;
  store << arcColor;
  store << arcWeight;
  store << help;
  store << tip;
  }


// Load object from stream
void FXGauge::load(FXStream& store){
  FXFrame::load(store);
  store >> caption;
  store >> numberFont;
  store >> captionFont;
  store >> startAngle;
  store >> sweepAngle;
  store >> range[0] >> range[1];
  store >> value;
  store >> majorTickDelta;
  store >> minorTickDelta;
  store >> majorTickColor;
  store >> minorTickColor;
  store >> majorTickSize;
  store >> minorTickSize;
  store >> faceColor;
  store >> arcColor;
  store >> arcWeight;
  store >> help;
  store >> tip;
  }


// Destroy
FXGauge::~FXGauge(){
  }


}

