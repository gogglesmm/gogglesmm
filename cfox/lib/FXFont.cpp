/********************************************************************************
*                                                                               *
*                               F o n t   O b j e c t                           *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxchar.h"
#include "fxmath.h"
#include "fxascii.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXException.h"
#include "FXRegion.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXDCWindow.h"
#include "FXApp.h"

/*
  Notes:

  - Interpretation of the hints:

      0                    No preference for pitch
      FXFont::Fixed        If specified, match for fixed pitch fonts are strongly preferred
      FXFont::Variable     If specified, match for variable pitch font strongly preferred

      0                    No hints given
      FXFont::Decorative   Ye Olde Fonte
      FXFont::Modern       Monospace fonts such as courier and so on
      FXFont::Roman        Serif font such as times
      FXFont::Script       Cursive font/script
      FXFont::Swiss        Sans serif font such as swiss, helvetica, arial
      FXFont::System       Raster based fonts, typically monospaced

      FXFont::X11          Force X11 raw font specification

      FXFont::Scalable     Strong emphasis on scalable fonts; under Windows, this means
                           TrueType fonts are desired

      FXFont::Polymorphic  Strong emphasis on polymorphic fonts; under Windows, this means
                           TrueType fonts desired also

  - FONTENCODING_DEFAULT means we prefer the fonts for the current locale;
    currently, this is hardwired to iso8859-1 until we have some means of
    determining the preferred encoding from the locale.

  - FXFont::Italic is a cursive typeface for some fonts; FXFont::Oblique is the same
    basic font written at an angle; for many fonts, FXFont::Italic and FXFont::Oblique
    means pretty much the same thing.  When matching, FXFont::Italic and FXFont::Oblique
    are considered closer to each other than FXFont::Straight.

  - A weight value of 0 indicates preference for non-bold font.

  - XFontStruct.ascent+XFontStruct.descent is the height of the font, as far as line
    spacing goes.  XFontStruct.max_bounds.ascent+XFontStruct.max_bounds.descent is
    larger, as some characters can apparently extend beyond ascent or descent!!

  - Registry section FONTSUBSTITUTIONS can be used to map typeface names to platform
    specific typeface names:

        [FONTSUBSTITUTIONS]
        arial = helvetica
        swiss = helvetica

    This allows you to change fonts in programs with hard-wired fonts.

  - Text txfm matrix [a b c d] premultiplies.

  - Should we perhaps build our own tables of font metrics? This might make
    things simpler for the advanced stuff, and be conceivably a lot faster
    under MS-Windows [no need to SelectObject() all the time just to get some
    info; also, this could be useful in case the drawing surface is not a
    window].

  - FOR THE MOMENT we're creating a dummy DC to keep the font locked into the GDI
    for MUCH quicker access to text metrics.  Soon however we want to just build
    our own font metrics tables and determine the metrics entirely with client-side
    code.  This will [predictably] be the fastest possible method as it will not
    involve context switches...

  - Matching algorithm slightly favors bitmapped fonts over scalable ones [as the
    latter may not be optimized easthetically; also, the matching algorithm should
    not weight resolution as much.

  - UNICODE means registry and encoding are set to iso10646-1

  - More human-readable font strings (e.g. registry):

       family [foundry],size,weight,slant,setwidth,encoding,hints

    For example:

       times [urw],120,bold,i,normal,iso8859-1,0

    Note that the size is in decipoints!

  - Get encoding from locale (see X11).

  - ix|xi
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_CREATION  1001

using namespace FX;

/*******************************************************************************/

namespace FX {


static const FXint LEAD_OFFSET=0xD800-(0x10000>>10);
static const FXint TAIL_OFFSET=0xDC00;


extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);

#if defined(WIN32) /////////////////////////// WIN32 ////////////////////////////


// Character set encoding
static BYTE FXFontEncoding2CharSet(FXuint encoding){
  switch(encoding){
    case FONTENCODING_DEFAULT: return DEFAULT_CHARSET;
    case FONTENCODING_TURKISH: return TURKISH_CHARSET;
    case FONTENCODING_BALTIC: return BALTIC_CHARSET;
    case FONTENCODING_CYRILLIC: return RUSSIAN_CHARSET;
    case FONTENCODING_ARABIC: return ARABIC_CHARSET;
    case FONTENCODING_GREEK: return GREEK_CHARSET;
    case FONTENCODING_HEBREW: return HEBREW_CHARSET;
    case FONTENCODING_THAI: return THAI_CHARSET;
    case FONTENCODING_EASTEUROPE: return EASTEUROPE_CHARSET;
    case FONTENCODING_USASCII: return ANSI_CHARSET;
//    case FONTENCODING_UNICODE: return ANSI_CHARSET;
    }
  return DEFAULT_CHARSET;
  }


// Character set encoding
static FXuint CharSet2FXFontEncoding(BYTE lfCharSet){
  switch(lfCharSet){
    case ANSI_CHARSET: return FONTENCODING_USASCII;
    case ARABIC_CHARSET: return FONTENCODING_ARABIC;
    case BALTIC_CHARSET: return FONTENCODING_BALTIC;
    case CHINESEBIG5_CHARSET: return FONTENCODING_DEFAULT;
    case DEFAULT_CHARSET: return FONTENCODING_DEFAULT;
    case EASTEUROPE_CHARSET: return FONTENCODING_EASTEUROPE;
    case GB2312_CHARSET: return FONTENCODING_DEFAULT;
    case GREEK_CHARSET: return FONTENCODING_GREEK;
#if !defined (__WATCOMC__) || (__WATCOMC__ >= 1200)
    case HANGUL_CHARSET: return FONTENCODING_DEFAULT;
#endif
    case HEBREW_CHARSET: return FONTENCODING_HEBREW;
    case MAC_CHARSET: return FONTENCODING_DEFAULT;
    case OEM_CHARSET: return FONTENCODING_DEFAULT;
    case SYMBOL_CHARSET: return FONTENCODING_DEFAULT;
    case RUSSIAN_CHARSET: return FONTENCODING_CYRILLIC;
    case SHIFTJIS_CHARSET: return FONTENCODING_DEFAULT;
    case THAI_CHARSET: return FONTENCODING_THAI;
    case TURKISH_CHARSET: return FONTENCODING_TURKISH;
    }
  return FONTENCODING_DEFAULT;
  }


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  FXTRACE((150,"wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res));
  TEXTMETRIC *font;
  LOGFONT lf;
  FXchar buffer[256];

  // Hang on to this for text metrics functions
  dc=CreateCompatibleDC(nullptr);

  // Now fill in the fields
  lf.lfHeight=-MulDiv(wantsize,GetDeviceCaps((HDC)dc,LOGPIXELSY),720);
  lf.lfWidth=0;
  if(wanthints&FXFont::Rotatable){
    lf.lfEscapement=(angle*10)/64;
    lf.lfOrientation=(angle*10)/64;
    }
  else{
    lf.lfEscapement=0;
    lf.lfOrientation=0;
    }
  lf.lfWeight=wantweight*10;
  if((wantslant==FXFont::Italic) || (wantslant==FXFont::Oblique))
    lf.lfItalic=true;
  else
    lf.lfItalic=false;
  lf.lfUnderline=false;
  lf.lfStrikeOut=false;

  // Character set encoding
  lf.lfCharSet=FXFontEncoding2CharSet(wantencoding);

  // Other hints
  lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
  if(wanthints&FXFont::System) lf.lfOutPrecision=OUT_RASTER_PRECIS;
  if(wanthints&FXFont::Scalable) lf.lfOutPrecision=OUT_TT_PRECIS;
  if(wanthints&FXFont::Polymorphic) lf.lfOutPrecision=OUT_TT_PRECIS;

  // Clip precision
  lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;

  // Quality
  lf.lfQuality=DEFAULT_QUALITY;

  // Pitch and Family
  lf.lfPitchAndFamily=0;

  // Pitch
  if(wanthints&FXFont::Fixed) lf.lfPitchAndFamily|=FIXED_PITCH;
  else if(wanthints&FXFont::Variable) lf.lfPitchAndFamily|=VARIABLE_PITCH;
  else lf.lfPitchAndFamily|=DEFAULT_PITCH;

  // Family
  if(wanthints&FXFont::Decorative) lf.lfPitchAndFamily|=FF_DECORATIVE;
  else if(wanthints&FXFont::Modern) lf.lfPitchAndFamily|=FF_MODERN;
  else if(wanthints&FXFont::Roman) lf.lfPitchAndFamily|=FF_ROMAN;
  else if(wanthints&FXFont::Script) lf.lfPitchAndFamily|=FF_SCRIPT;
  else if(wanthints&FXFont::Swiss) lf.lfPitchAndFamily|=FF_SWISS;
  else lf.lfPitchAndFamily|=FF_DONTCARE;

  // Font substitution
#ifdef UNICODE
  utf2ncs(lf.lfFaceName,wantfamily.text(),LF_FACESIZE);
#else
  fxstrlcpy(lf.lfFaceName,wantfamily.text(),LF_FACESIZE);
#endif

  // Here we go!
  xid=CreateFontIndirect(&lf);

  // Uh-oh, we failed
  if(!xid) return nullptr;

  // Obtain text metrics
  if(!callocElms(font,1)) return nullptr;

  SelectObject((HDC)dc,xid);
  GetTextMetrics((HDC)dc,(TEXTMETRIC*)font);

  // Get actual face name
  GetTextFaceA((HDC)dc,sizeof(buffer),buffer);
  actualName=buffer;
  actualSize=MulDiv(((TEXTMETRIC*)font)->tmHeight,720,GetDeviceCaps((HDC)dc,LOGPIXELSY));
  actualWeight=((TEXTMETRIC*)font)->tmWeight/10;
  actualSlant=((TEXTMETRIC*)font)->tmItalic?FXFont::Italic:FXFont::Straight;
  actualSetwidth=0;
  actualEncoding=CharSet2FXFontEncoding(((TEXTMETRIC*)font)->tmCharSet);

  // Return it
  return font;
  }


#elif defined(HAVE_XFT_H) /////////////////////// XFT ///////////////////////////


// Access to display
#define DISPLAY(app)      ((Display*)((app)->display))

// For Fontconfig 1.0.2
#ifndef FC_WEIGHT_THIN
#define FC_WEIGHT_THIN              0
#undef FC_WEIGHT_LIGHT
#define FC_WEIGHT_LIGHT             50
#endif
#ifndef FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_EXTRALIGHT        40
#endif
#ifndef FC_WEIGHT_NORMAL
#define FC_WEIGHT_NORMAL            80
#endif
#ifndef FC_WEIGHT_EXTRABOLD
#define FC_WEIGHT_EXTRABOLD         205
#endif


// From FOX weight to fontconfig weight
static FXint weight2FcWeight(FXint weight){
  switch(weight){
    case FXFont::Thin:      return FC_WEIGHT_THIN;
    case FXFont::ExtraLight:return FC_WEIGHT_EXTRALIGHT;
    case FXFont::Light:     return FC_WEIGHT_LIGHT;
    case FXFont::Normal:    return FC_WEIGHT_NORMAL;
    case FXFont::Medium:    return FC_WEIGHT_MEDIUM;
    case FXFont::DemiBold:  return FC_WEIGHT_DEMIBOLD;
    case FXFont::Bold:      return FC_WEIGHT_BOLD;
    case FXFont::ExtraBold: return FC_WEIGHT_EXTRABOLD;
    case FXFont::Black:     return FC_WEIGHT_BLACK;
    }
  return FC_WEIGHT_NORMAL;
  }


// From fontconfig weight to FOX weight
static FXint fcWeight2Weight(FXint fcWeight){
  switch(fcWeight){
    case FC_WEIGHT_THIN:      return FXFont::Thin;
    case FC_WEIGHT_EXTRALIGHT:return FXFont::ExtraLight;
    case FC_WEIGHT_LIGHT:     return FXFont::Light;
    case FC_WEIGHT_NORMAL:    return FXFont::Normal;
    case FC_WEIGHT_MEDIUM:    return FXFont::Medium;
    case FC_WEIGHT_DEMIBOLD:  return FXFont::DemiBold;
    case FC_WEIGHT_BOLD:      return FXFont::Bold;
    case FC_WEIGHT_EXTRABOLD: return FXFont::ExtraBold;
    case FC_WEIGHT_BLACK:     return FXFont::Black;
    }
  return FXFont::Normal;
  }


// From FOX setwidth to fontconfig setwidth
static FXint setWidth2FcSetWidth(FXint setwidth){
#ifdef FC_WIDTH
  switch(setwidth){
    case FXFont::UltraCondensed:return FC_WIDTH_ULTRACONDENSED;
    case FXFont::ExtraCondensed:return FC_WIDTH_EXTRACONDENSED;
    case FXFont::Condensed:     return FC_WIDTH_CONDENSED;
    case FXFont::SemiCondensed: return FC_WIDTH_SEMICONDENSED;
    case FXFont::NonExpanded:        return FC_WIDTH_NORMAL;
    case FXFont::SemiExpanded:  return FC_WIDTH_SEMIEXPANDED;
    case FXFont::Expanded:      return FC_WIDTH_EXPANDED;
    case FXFont::ExtraExpanded: return FC_WIDTH_EXTRAEXPANDED;
    case FXFont::UltraExpanded: return FC_WIDTH_ULTRAEXPANDED;
    }
  return FC_WIDTH_NORMAL;
#else
  return 0;
#endif
  }


// From fontconfig setwidth to FOX setwidth
static FXint fcSetWidth2SetWidth(FXint fcSetWidth){
#ifdef FC_WIDTH
  switch(fcSetWidth){
    case FC_WIDTH_ULTRACONDENSED:return FXFont::UltraCondensed;
    case FC_WIDTH_EXTRACONDENSED:return FXFont::ExtraCondensed;
    case FC_WIDTH_CONDENSED:     return FXFont::Condensed;
    case FC_WIDTH_SEMICONDENSED: return FXFont::SemiCondensed;
    case FC_WIDTH_NORMAL:        return FXFont::NonExpanded;
    case FC_WIDTH_SEMIEXPANDED:  return FXFont::SemiExpanded;
    case FC_WIDTH_EXPANDED:      return FXFont::Expanded;
    case FC_WIDTH_EXTRAEXPANDED: return FXFont::ExtraExpanded;
    case FC_WIDTH_ULTRAEXPANDED: return FXFont::UltraExpanded;
    }
#endif
  return FXFont::NonExpanded;
  }


// From FOX slant to fontconfig slant
static FXint slant2FcSlant(FXint slant){
  switch(slant){
    case FXFont::Straight: return FC_SLANT_ROMAN;
    case FXFont::Italic: return FC_SLANT_ITALIC;
    case FXFont::Oblique: return FC_SLANT_OBLIQUE;
    case FXFont::ReverseItalic: return FC_SLANT_ITALIC;         // No equivalent FC value
    case FXFont::ReverseOblique: return FC_SLANT_OBLIQUE;       // No equivalent FC value
    }
  return FC_SLANT_ROMAN;
  }


// From fontconfig slant to FOX slant
static FXint fcSlant2Slant(FXint fcSlant){
  switch(fcSlant){
    case FC_SLANT_ROMAN:  return FXFont::Straight;
    case FC_SLANT_ITALIC: return FXFont::Italic;
    case FC_SLANT_OBLIQUE:return FXFont::Oblique;
    }
  return FXFont::Straight;
  }


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  const FXchar *rgba=getApp()->reg().readStringEntry("Xft","rgba","unknown");
  const FXchar *hs=getApp()->reg().readStringEntry("Xft","hintstyle","full");
  FXbool     fc_hinting=getApp()->reg().readBoolEntry("Xft","hinting",true);
  FXbool     fc_autohint=getApp()->reg().readBoolEntry("Xft","autohint",false);
  FXbool     fc_antialias=getApp()->reg().readBoolEntry("Xft","antialias",true);
  FXint      fc_rgba=FC_RGBA_UNKNOWN;
  FXint      fc_hintstyle=FC_HINT_FULL;
  int        pp,sw,wt,sl;
  double     a,s,c,sz;
  FcPattern *pattern,*p;
  FcChar8   *fam,*fdy;
  FcCharSet *charset;
  XftFont   *fnt;
  FcResult   result;
  FcBool     sc;
  FcMatrix   matrix;

  FXTRACE((150,"wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res));

  // Create pattern object
  pattern=FcPatternCreate();

  // RGBA color order hint
  if(rgba[0]=='u') fc_rgba=FC_RGBA_UNKNOWN;
  else if(rgba[0]=='r') fc_rgba=FC_RGBA_RGB;
  else if(rgba[0]=='b') fc_rgba=FC_RGBA_BGR;
  else if(rgba[0]=='v' && rgba[1]=='r') fc_rgba=FC_RGBA_VRGB;
  else if(rgba[0]=='v' && rgba[1]=='b') fc_rgba=FC_RGBA_VBGR;
  else if(rgba[0]=='n') fc_rgba=FC_RGBA_NONE;

  if(hs[0]=='s') fc_hintstyle=FC_HINT_SLIGHT;
  else if(hs[0]=='m') fc_hintstyle=FC_HINT_MEDIUM;
  else if(hs[0]=='f') fc_hintstyle=FC_HINT_FULL;
  else if(hs[0]=='n') fc_hintstyle=FC_HINT_NONE;

  // Set additional hints
  FcPatternAddBool(pattern,FC_HINTING,fc_hinting);
  FcPatternAddBool(pattern,FC_ANTIALIAS,fc_antialias);
  FcPatternAddBool(pattern,FC_AUTOHINT,fc_autohint);
  FcPatternAddInteger(pattern,FC_RGBA,fc_rgba);
  FcPatternAddInteger(pattern,FC_HINT_STYLE,fc_hintstyle);

  // Set family
  if(!wantfamily.empty()){
    FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)wantfamily.text());
    }

  // Set foundry
  if(!wantforge.empty()){
    FcPatternAddString(pattern,FC_FOUNDRY,(const FcChar8*)wantforge.text());
    }

  // Set pixel size, based on given screen res and desired point size
  if(wantsize!=0){
    FcPatternAddDouble(pattern,FC_PIXEL_SIZE,(res*wantsize)/720.0);
    }

  // Set font weight
  if(wantweight!=0){
    FcPatternAddInteger(pattern,FC_WEIGHT,weight2FcWeight(wantweight));
    }

  // Set slant
  if(wantslant!=0){
    FcPatternAddInteger(pattern,FC_SLANT,slant2FcSlant(wantslant));
    }

#ifdef FC_WIDTH
  // Set setwidth
  if(wantsetwidth!=0){
    FcPatternAddInteger(pattern,FC_WIDTH,setWidth2FcSetWidth(wantsetwidth));
    }
#endif

  // Set encoding
  if(wantencoding!=FONTENCODING_DEFAULT){                                       // FIXME
//    FcCharSet* charSet=FcCharSetCreate();
//    encoding2FcCharSet((void*)charSet, (FXFontEncoding)encoding);
//    FcPatternAddCharSet(pattern, FC_CHARSET, charSet);
//    FcCharSetDestroy(charSet);
    }

  // Set pitch
  if(wanthints&FXFont::Fixed){
    FcPatternAddInteger(pattern,FC_SPACING,FC_MONO);
    }
  else if(wanthints&FXFont::Variable){
    FcPatternAddInteger(pattern,FC_SPACING,FC_PROPORTIONAL);
    }

  // Scalable font hint; also set if we want rotation
  if(wanthints&(FXFont::Scalable|FXFont::Rotatable)){
    FcPatternAddBool(pattern,FC_SCALABLE,true);
    }

  // Always set matrix if rotatable
  if(wanthints&FXFont::Rotatable){
    a=0.00027270769562411399179*angle;
    c=Math::cos(a);
    s=Math::sin(a);
    matrix.xx=c; matrix.xy=-s;
    matrix.yx=s; matrix.yy=c;
    FcPatternAddMatrix(pattern,FC_MATRIX,&matrix);
    }

  // Pattern substitutions
  FcConfigSubstitute(0,pattern,FcMatchPattern);
  FcDefaultSubstitute(pattern);

  // Find pattern matching a font
  p=FcFontMatch(0,pattern,&result);
  if(!p) return nullptr;

  // Get name and foundry
  if(FcPatternGetString(p,FC_FAMILY,0,&fam)==FcResultMatch){
    actualName=(const FXchar*)fam;
    if(FcPatternGetString(p,FC_FOUNDRY,0,&fdy)==FcResultMatch){
      actualName.append(" [");
      actualName.append((const FXchar*)fdy);
      actualName.append("]");
      }
    }

#ifdef FC_WIDTH
  // Get setwidth
  if(FcPatternGetInteger(p,FC_WIDTH,0,&sw)==FcResultMatch){
    actualSetwidth=fcSetWidth2SetWidth(sw);
    }
#endif

  // Get weight
  if(FcPatternGetInteger(p,FC_WEIGHT,0,&wt)==FcResultMatch){
    actualWeight=fcWeight2Weight(wt);
    }

  // Get slant
  if(FcPatternGetInteger(p,FC_SLANT,0,&sl)==FcResultMatch){
    actualSlant=fcSlant2Slant(sl);
    }

  // Get pitch
  if(FcPatternGetInteger(p,FC_SPACING,0,&pp)==FcResultMatch){
    flags&=~(FXFont::Fixed|FXFont::Variable);
//    if(pp==FC_MONO || pp==FC_DUAL || pp==FC_CHARCELL) flags|=FXFont::Fixed;
    if(pp==FC_MONO || pp==FC_CHARCELL) flags|=FXFont::Fixed;
    else if(pp==FC_PROPORTIONAL) flags|=FXFont::Variable;
    }

  // Get scalable flag
  if(FcPatternGetBool(p,FC_SCALABLE,0,&sc)==FcResultMatch){
    flags=sc?(flags|FXFont::Scalable):(flags&~FXFont::Scalable);
    }

  // Get pixel size and work it back to deci-points using given screen res
  if(FcPatternGetDouble(p,FC_PIXEL_SIZE,0,&sz)==FcResultMatch){
    actualSize=(int)((720.0*sz)/res);
    }

  // Get charset
  if(FcPatternGetCharSet(p,FC_CHARSET,0,&charset)==FcResultMatch){      // FIXME
    }

  // Get the encoding
  actualEncoding=FONTENCODING_UNICODE;

  // Open font
  fnt=XftFontOpenPattern(DISPLAY(getApp()),p);
  xid=(FXID)fnt;

  // Destroy pattern
  FcPatternDestroy(pattern);

  return fnt;
  }


#else ///////////////////////////////// XLFD ////////////////////////////////////


#define SGN(x)        ((x)<-0.0005?"~":"")
#define DISPLAY(app)  ((Display*)((app)->display))


// Convert text to font weight
static FXuint xlfdWeight(const FXchar* text){
  FXchar c1=Ascii::toLower(text[0]);
  FXchar c2=Ascii::toLower(text[1]);
  if(c1=='l' && c2=='i') return FXFont::Light;
  if(c1=='o' && c2=='u') return FXFont::Light;
  if(c1=='s' && c2=='h') return FXFont::Light;
  if(c1=='n' && c2=='o') return FXFont::Normal;
  if(c1=='r' && c2=='e') return FXFont::Normal;
  if(c1=='m' && c2=='e') return FXFont::Medium;
  if(c1=='d' && c2=='e') return FXFont::DemiBold;
  if(c1=='s' && c2=='e') return FXFont::DemiBold;
  if(c1=='b' && c2=='o') return FXFont::Bold;
  if(c1=='b' && c2=='l') return FXFont::Black;
  return 0;
  }


// Convert text to slant
static FXuint xlfdSlant(const FXchar* text){
  FXchar c1=Ascii::toLower(text[0]);
  FXchar c2=Ascii::toLower(text[1]);
  if(c1=='i') return FXFont::Italic;
  if(c1=='o') return FXFont::Oblique;
  if(c1=='r' && c2=='i') return FXFont::ReverseItalic;
  if(c1=='r' && c2=='o') return FXFont::ReverseOblique;
  if(c1=='r') return FXFont::Straight;
  return 0;
  }


// Convert text to setwidth
static FXuint xlfdSetwidth(const FXchar* text){
  if(text[0]=='m') return FXFont::NonExpanded;
  if(text[0]=='w') return FXFont::ExtraExpanded;
  if(text[0]=='r') return FXFont::NonExpanded;
  if(text[0]=='c') return FXFont::Condensed;
  if(text[0]=='n'){
    if(text[1]=='a') return FXFont::Condensed;
    if(text[1]=='o') return FXFont::NonExpanded;
    return 0;
    }
  if(text[0]=='e' && text[1]=='x' && text[2]=='p') return FXFont::Expanded;
  if(text[0]=='e' && text[1]=='x' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FXFont::ExtraCondensed;
    if(text[5]=='e') return FXFont::ExtraExpanded;
    return 0;
    }
  if(text[0]=='u' && text[1]=='l' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FXFont::UltraCondensed;
    if(text[5]=='e') return FXFont::UltraExpanded;
    return 0;
    }
  if((text[0]=='s' || text[0]=='d') && text[1]=='e' && text[2]=='m' && text[3]=='i'){
    if(text[5]=='c') return FXFont::SemiCondensed;
    if(text[5]=='e') return FXFont::SemiExpanded;
    return 0;
    }
  return 0;
  }


// Convert pitch to flags
static FXuint xlfdPitch(const FXchar* text){
  FXchar c=Ascii::toLower(text[0]);
  if(c=='p') return FXFont::Variable;
  if(c=='m' || c=='c') return FXFont::Fixed;
  return 0;
  }


// Convert fields to scalable flag
static FXuint xlfdScalable(const FXchar *pixelsize,const FXchar *pointsize,const FXchar* average){
  if(pixelsize[0]=='[' && pointsize[0]=='[' && average[0]=='0') return FXFont::Scalable;
  if(pixelsize[0]=='0' && pointsize[0]=='0' && average[0]=='0') return FXFont::Scalable;
  return 0;
  }


// Convert fields to rotatable flag
static FXuint xlfdRotatable(const FXchar *pixelsize,const FXchar *pointsize){
  if(pixelsize[0]=='[' && pointsize[0]=='[') return FXFont::Rotatable;
  return 0;
  }


// Convert fields to rotatable flag
static FXuint xlfdPolymorph(const FXchar *weight,const FXchar* slant,const FXchar *setwidth,const FXchar* addstyle){
  if(weight[0]=='0' || slant[0]=='0' || setwidth[0]=='0' || addstyle[0]=='0') return FXFont::Polymorphic;
  return 0;
  }


// Determine encoding; these codes should tie into the codec list in some way
static FXuint xlfdEncoding(const FXchar* text){
  if((text[0]=='i' || text[0]=='I') && (text[1]=='s' || text[1]=='S') && (text[2]=='o' || text[2]=='O')){
    if(text[3]=='8' && text[4]=='8' && text[5]=='5' && text[6]=='9' && text[7]=='-'){
      return FONTENCODING_ISO_8859_1+atoi(text+8)-1;    // iso8859-XX
      }
    if(text[3]=='1' && text[4]=='0' &&text[5]=='6' && text[6]=='4' && text[7]=='6' && text[8]=='-' && text[9]=='1'){
      return FONTENCODING_UNICODE;                      // iso10646-1
      }
    return FONTENCODING_DEFAULT;
    }
  if((text[0]=='k' || text[0]=='K') && (text[1]=='o' || text[1]=='O') && (text[2]=='i' || text[2]=='I') && text[3]=='8'){
    if(text[4]=='r' || text[4]=='R'){
      return FONTENCODING_KOI8_R;                       // koi8r
      }
    if(text[4]=='u' || text[4]=='U'){                   // koi8u
      return FONTENCODING_KOI8_U;
      }
    return FONTENCODING_KOI8;
    }
  if((text[0]=='m' || text[0]=='M') && (text[1]=='i' || text[1]=='I') && (text[2]=='c' || text[2]=='C') && (text[3]=='r' || text[3]=='R') && (text[4]=='o' || text[4]=='O') && (text[5]=='s' || text[5]=='S') && (text[6]=='o' || text[6]=='O') && (text[7]=='f' || text[7]=='F') && (text[8]=='t' || text[8]=='T') && text[9]=='-'){
    if((text[10]=='c' || text[10]=='C') && (text[11]=='p' || text[11]=='P')){
      return atoi(text+12);                             // microsoft-cpXXXX
      }
    return FONTENCODING_DEFAULT;
    }
  if((text[0]=='j' || text[0]=='J') && (text[1]=='i' || text[1]=='I') && (text[2]=='s' || text[2]=='S') && (text[3]=='x' || text[3]=='X')){
    return FONTENCODING_DEFAULT;                        // jisx.XXXX.YYYY-Z FIXME
    }
  if((text[0]=='b' || text[0]=='B') && (text[1]=='i' || text[1]=='I') && (text[2]=='g' || text[2]=='G') && text[3]=='5'){
    return FONTENCODING_DEFAULT;                        // big5XXX FIXME
    }
  if((text[0]=='k' || text[0]=='K') && (text[1]=='s' || text[1]=='S') && (text[2]=='c' || text[2]=='C')){
    return FONTENCODING_DEFAULT;                        // kscXXX FIXME
    }
  if((text[0]=='g' || text[0]=='G') && (text[1]=='b' || text[1]=='B')){
    return FONTENCODING_DEFAULT;                        // gbXXXX FIXME
    }
  return FONTENCODING_DEFAULT;
  }


// Split XLFD into pieces
static void xlfdSplit(const char *fld[],char* font){
  fld[0]=fld[1]=fld[2]=fld[3]=fld[4]=fld[5]=fld[6]=fld[7]=fld[8]=fld[9]=fld[10]=fld[11]=fld[12]="";
  if(*font=='-'){
    fld[0]=++font;
    for(int f=1; f<=12; f++){
      while(*font && *font!='-') font++;
      if(*font=='-') *font++='\0';
      fld[f]=font;
      }
    }
  else{
    fld[1]=font;
    }
  }


// Return font name from a font possibly containing wildcards
static FXString xlfdFont(Display *dpy,const FXString& font){
  char **fontnames; int nfontnames;
  FXString fontname(font);
  if((fontnames=XListFonts(dpy,font.text(),1,&nfontnames))!=nullptr){
    fontname=fontnames[0];
    XFreeFontNames(fontnames);
    }
  return fontname;
  }


// Try find matching font
void* FXFont::match(const FXString& wantfamily,const FXString& wantforge,FXuint wantsize,FXuint wantweight,FXuint wantslant,FXuint wantsetwidth,FXuint wantencoding,FXuint wanthints,FXint res){
  FXuint   encoding,weight,slant,setwidth,pitch,scalable,rotatable,polymorph,xres,yres,points,size;
  FXint    bencoding,bweight,bslant,bsetwidth,bpitch,bscalable,brotatable,bpolymorph,bsize,bxres,byres;
  FXint    dencoding,dweight,dslant,dsetwidth,dpitch,dscalable,drotatable,dpolymorph,dsize;
  FXchar   candidate[256],xlfd[256];
  FXchar **fontnames;
  FXint    nfontnames,b,f;
  const FXchar *field[13];
  FXdouble c,s,a;
  XFontStruct *fs;

  FXTRACE((150,"wantfamily=%s wantforge=%s wantsize=%d wantweight=%d wantslant=%d wantsetwidth=%d wantencoding=%d wanthints=%d res=%d\n",wantfamily.text(),wantforge.text(),wantsize,wantweight,wantslant,wantsetwidth,wantencoding,wanthints,res));

  // Get fonts matching the pattern
  __snprintf(candidate,sizeof(candidate),"-%s-%s-*-*-*-*-*-%s-*-*-*-*-*-*",wantforge.empty()?"*":wantforge.text(),wantfamily.empty()?"*":wantfamily.text(),(hints&FXFont::Rotatable)?"[1 0 0 1]":"*");
  fontnames=XListFonts(DISPLAY(getApp()),candidate,65535,&nfontnames);
  if(fontnames && 0<nfontnames){

    b=-1;
    bencoding=100000;
    bweight=100000;
    bslant=100000;
    bsetwidth=100000;
    bpitch=100000;
    bscalable=100000;
    brotatable=100000;
    bpolymorph=100000;
    bsize=100000;
    bxres=res;
    byres=res;

    // Match them
    for(f=0; f<nfontnames; f++){

      //FXTRACE((1,"font=%s\n",fontnames[f]));

      // Break apart into fields
      fxstrlcpy(candidate,fontnames[f],sizeof(candidate));
      xlfdSplit(field,candidate);

      // Get info
      weight=xlfdWeight(field[2]);
      slant=xlfdSlant(field[3]);
      setwidth=xlfdSetwidth(field[4]);
      scalable=xlfdScalable(field[6],field[7],field[11]);
      polymorph=xlfdPolymorph(field[2],field[3],field[4],field[5]);
      rotatable=xlfdRotatable(field[6],field[7]);
      points=atoi(field[7]);    // Deci-points
      xres=atoi(field[8]);
      yres=atoi(field[9]);
      pitch=xlfdPitch(field[10]);
      encoding=xlfdEncoding(field[12]);

      // The font can be rendered at any resolution, so render at actual device resolution
      if(xres==0 && yres==0){
        xres=res;
        yres=res;
        }

      // Encoding
      if(wantencoding==FONTENCODING_DEFAULT){
        dencoding=(encoding!=FONTENCODING_UNICODE);     // Want unicode if nothing else given
        }
      else{
        dencoding=(encoding!=wantencoding);
        }

      // Weight
      if(wantweight){
        dweight=Math::iabs(FXint(weight-wantweight));
        }
      else{
        dweight=Math::iabs(FXint(weight-FXFont::Normal));
        }

      // Slant
      if(wantslant){
        dslant=Math::iabs(FXint(slant-wantslant));
        }
      else{
        dslant=Math::iabs(FXint(slant-FXFont::Straight));
        }

      // Set width
      if(wantsetwidth){
        dsetwidth=Math::iabs(FXint(setwidth-wantsetwidth));
        }
      else{
        dsetwidth=Math::iabs(FXint(setwidth-FXFont::NonExpanded));
        }

      // Pitch
      if(wanthints&FXFont::Fixed){
        dpitch=(FXFont::Fixed!=pitch);
        }
      else if(wanthints&FXFont::Variable){
        dpitch=(FXFont::Variable!=pitch);
        }
      else{
        dpitch=0;
        }

      // Scalable
      if(wanthints&FXFont::Scalable){
        dscalable=(FXFont::Scalable!=scalable);
        }
      else{
        dscalable=0;
        }

      // Rotatable
      if(wanthints&FXFont::Rotatable){
        drotatable=(FXFont::Rotatable!=rotatable);
        }
      else{
        drotatable=0;
        }

      // Polymorphic
      if(wanthints&FXFont::Polymorphic){
        dpolymorph=(FXFont::Polymorphic!=polymorph);
        }
      else{
        dpolymorph=0;
        }

      // If scalable, we can of course get the exact size we want
      // We do not set dsize to 0, as we prefer a bitmapped font that gets within
      // 10% over a scalable one that's exact, as the bitmapped fonts look much better
      // at small sizes than scalable ones...
      if(scalable){
        dsize=wantsize/10;
        size=wantsize;
        }

      // We correct for the actual screen resolution; if the font is rendered at a
      // 100 dpi, and we have a screen with 90dpi, the actual point size of the font
      // should be multiplied by (100/90).
      else{
        size=(yres*points)/res;
        dsize=Math::iabs(FXint(size-wantsize));
        }

      FXTRACE((160,"%4d: dweight=%-3d dsize=%3d dslant=%d dsetwidth=%d dscalable=%d dpolymorph=%d xres=%-3d yres=%-3d xlfd=\"%s\"\n",f,dweight,dsize,dslant,dsetwidth,dscalable,dpolymorph,xres,yres,fontnames[f]));

      // But I'm NOT drinking any fucking Merlot!
      if((dencoding<bencoding) || ((dencoding==bencoding) && ((drotatable<brotatable) || ((drotatable==brotatable) && ((dpitch<bpitch) || ((dpitch==bpitch) && ((dsize<bsize) || ((dsize==bsize) && ((dweight<bweight) || ((dweight==bweight) && ((dslant<bslant) || ((dslant==bslant) && ((dsetwidth<bsetwidth) || ((dsetwidth==bsetwidth) && ((dscalable<bscalable) || ((dscalable==bscalable) && (dpolymorph<bpolymorph))))))))))))))))){

        // Best match
        actualName=field[1];
        actualName.append(" [");
        actualName.append(field[0]);
        actualName.append("]");
        actualSize=size;
        actualWeight=weight;
        actualSlant=slant;
        actualSetwidth=setwidth;
        actualEncoding=encoding;
        flags=pitch|scalable|polymorph|rotatable;

        // Closeness
        bencoding=dencoding;
        brotatable=drotatable;
        bpitch=dpitch;
        bsize=dsize;
        bweight=dweight;
        bslant=dslant;
        bsetwidth=dsetwidth;
        bscalable=dscalable;
        bpolymorph=dpolymorph;
        bxres=xres;
        byres=yres;
        b=f;
        }
      }

    // Got a font?
    if(0<=b){

      FXTRACE((150,"bweight=%-3d bsize=%3d bslant=%d bsetwidth=%d bscalable=%d bpolymorph=%d bxres=%-3d byres=%-3d xlfd=\"%s\"\n",bweight,bsize,bslant,bsetwidth,bscalable,bpolymorph,bxres,byres,fontnames[b]));

      // Keep desired font name
      fxstrlcpy(xlfd,fontnames[b],sizeof(xlfd));

      // Free the list
      XFreeFontNames(fontnames);

/*
      // If font is scaled or rotated, build custom xlfd
      if(flags&(FXFont::Scalable|FXFont::Rotatable)){

        // Bust up XLFD into parts
        fxstrlcpy(candidate,xlfd,sizeof(candidate));
        xlfdSplit(field,candidate);

        // Create rotated font
        if(flags&FXFont::Rotatable){
          a=0.00027270769562411399179*angle;
          c=(cos(a)*res*actualSize)/(10.0*byres);
          s=(sin(a)*res*actualSize)/(10.0*byres);
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-[%s%.3f %s%.3f %s%.3f %s%.3f]-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],SGN(c),fabs(c),SGN(s),fabs(s),SGN(-s),fabs(s),SGN(c),fabs(c),bxres,byres,field[10],field[12]);
          }

        // Create scaled font
        else{
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],(res*actualSize)/byres,bxres,byres,field[10],field[12]);
          }
        }

      // Try load it
      font=XLoadQueryFont(DISPLAY(getApp()),xlfd);
*/

      // If font is scaled or rotated, build custom xlfd
      if(flags&(FXFont::Scalable|FXFont::Rotatable)){

        // Bust up XLFD into parts
        fxstrlcpy(candidate,xlfd,sizeof(candidate));
        xlfdSplit(field,candidate);

        // Create scaled font
        __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],(res*actualSize)/byres,bxres,byres,field[10],field[12]);

        FXTRACE((140,"XLoadQueryFont(\"%s\")\n",xlfd));

        // Load normal scaled font
        fs=XLoadQueryFont(DISPLAY(getApp()),xlfd);

        // This is an ugly workaround:- according to docs, the X11 server is supposed to
        // fill XCharStruct's in the XFontStruct such that the attribute data represents
        // the escapement amount; however, this appears to be total garbage for all but a
        // few of the installed fonts; moreover, the width member of the XCharStruct is
        // also total garbage for these rotated fonts.
        // The fix here is very tricky, but it works:- first, we grab the normal horizontal
        // font's XFontStruct using XLoadQueryFont().  Then, we XUnloadFont() the font while
        // keeping the XFontStruct.  Next, we load the rotated font using XLoadFont() and
        // plug the font id into the XFontStruct.  This works, since while the XCharStruct's
        // may sometimes be located in read-only shared memory, the XFontStruct is always
        // allocated in the client by Xlib.
        if(fs && (flags&FXFont::Rotatable)){
          a=0.00027270769562411399179*angle;
          c=(Math::cos(a)*res*actualSize)/(10.0*byres);
          s=(Math::sin(a)*res*actualSize)/(10.0*byres);
          __snprintf(xlfd,sizeof(xlfd),"-%s-%s-%s-%s-%s-%s-*-[%s%.3f %s%.3f %s%.3f %s%.3f]-%d-%d-%s-*-%s",field[0],field[1],field[2],field[3],field[4],field[5],SGN(c),Math::fabs(c),SGN(s),Math::fabs(s),SGN(-s),Math::fabs(s),SGN(c),Math::fabs(c),bxres,byres,field[10],field[12]);
          XUnloadFont(DISPLAY(getApp()),((XFontStruct*)fs)->fid);
          ((XFontStruct*)fs)->fid=XLoadFont(DISPLAY(getApp()),xlfd);
          }
        }

      // Simple case for horizontally drawn fonts
      else{

        FXTRACE((140,"XLoadQueryFont(\"%s\")\n",xlfd));

        // Load normal scaled font
        fs=XLoadQueryFont(DISPLAY(getApp()),xlfd);
        }

/*
      if(font){
        for(b=0; b<((XFontStruct*)font)->n_properties; b++){
          if(((XFontStruct*)font)->properties[b].name==XA_FONT){
            char *fn=XGetAtomName(DISPLAY(getApp()),((XFontStruct*)font)->properties[b].card32);
            fxstrlcpy(candidate,fn,sizeof(candidate));
            FXTRACE((100,"FONT = %s\n",candidate));
            xlfdSplit(field,candidate);
            XFree(fn);
            FXTRACE((100,"mat  = %s\n",field[6]));
            }
          else{
            FXTRACE((100,"%d (%s) = %d\n",((XFontStruct*)font)->properties[b].name,XGetAtomName(DISPLAY(getApp()),((XFontStruct*)font)->properties[b].name),((XFontStruct*)font)->properties[b].card32));
            }
          }
        }
*/
      return fs;
      }
    }
  return nullptr;
  }


#endif //////////////////////////////////////////////////////////////////////////


/*******************************************************************************/

// Object implementation
FXIMPLEMENT(FXFont,FXId,nullptr,0)


// Deserialization
FXFont::FXFont(){
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  flags=0;
  angle=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Construct font from given font description
FXFont::FXFont(FXApp* a,const FXString& string):FXId(a){
  FXTRACE((TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this));
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  flags=0;
  angle=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  setFont(string);
  }


// Construct a font with given family name, size in points, weight, slant, character set encoding, setwidth, and hints
FXFont::FXFont(FXApp* a,const FXString& face,FXuint size,FXuint weight,FXuint slant,FXuint encoding,FXuint setwidth,FXuint h):FXId(a),wantedName(face){
  FXTRACE((TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this));
  wantedSize=10*size;
  wantedWeight=weight;
  wantedSlant=slant;
  wantedSetwidth=setwidth;
  wantedEncoding=encoding;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=(h&~FXFont::X11);          // System-independent method
  flags=0;
  angle=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Construct font from font description
FXFont::FXFont(FXApp* a,const FXFontDesc& fontdesc):FXId(a),wantedName(fontdesc.face){
  FXTRACE((TOPIC_CONSTRUCT,"FXFont::FXFont %p\n",this));
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=fontdesc.flags;
  flags=0;
  angle=0;
  font=nullptr;
#ifdef WIN32
  dc=nullptr;
#endif
  }


// Return family part of name
FXString FXFont::getFamily() const {
  return wantedName.before('[').trimEnd();
  }


// Return foundry part of name
FXString FXFont::getFoundry() const {
  return wantedName.section("[]",1);
  }


/*******************************************************************************/


// Create font
void FXFont::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::create %p\n",getClassName(),this));

#if defined(WIN32)              ///// WIN32 /////

      FXString family=getFamily();

      FXTRACE((150,"%s::create: win32 font\n",getClassName()));

      // Try to match with specified family and foundry
      if(!family.empty()){
        family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
        font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,100);
        }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }

#elif defined(HAVE_XFT_H)       ///// XFT /////

      FXString family=getFamily();
      FXString foundry=getFoundry();
      FXint    res;

      // Override screen resolution via registry
      res=getApp()->reg().readUIntEntry("SETTINGS","screenres",100);

      FXTRACE((150,"%s::create: xft font\n",getClassName()));

      // Try to match with specified family and foundry
      if(!family.empty()){
        family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
        if(!foundry.empty()){
          foundry=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",foundry.text(),foundry.text());
          font=match(family,foundry,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
          }
        if(!font){
          font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
          }
        }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }

#else                           ///// XLFD /////

      FXString family=getFamily();
      FXString foundry=getFoundry();
      FXint    res;

      // Override screen resolution via registry
      res=getApp()->reg().readUIntEntry("SETTINGS","screenres",100);

      FXTRACE((150,"%s::create: xlfd font\n",getClassName()));

      // X11 font specification
      if(hints&FXFont::X11){

        // Resolve font name
        actualName=xlfdFont(DISPLAY(getApp()),wantedName);

        // Try load the font
        font=XLoadQueryFont(DISPLAY(getApp()),actualName.text());
        }

      // Platform independent specification
      if(!font){

        // First we try to match with specified family and foundry
        if(!family.empty()){
          family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family.text(),family.text());
          if(!foundry.empty()){
            foundry=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",foundry.text(),foundry.text());
            font=match(family,foundry,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          if(!font){
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          }

        // Try based on hints
        if(!font){

          // Try swiss if we want swiss or indicated no preference
          if((hints&(FXFont::Swiss|FXFont::System)) || !(hints&(FXFont::Decorative|FXFont::Modern|FXFont::Roman|FXFont::Script|FXFont::Swiss|FXFont::System))){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","helvetica","helvetica");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","lucida","lucida");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try roman
          else if(hints&FXFont::Roman){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","times","times");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","charter","charter");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try modern
          else if(hints&FXFont::Modern){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","courier","courier");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            if(!font){
              family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","lucidatypewriter","lucidatypewriter");
              font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
              }
            }

          // Try decorative
          else if(hints&FXFont::Decorative){
            family=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","gothic","gothic");
            font=match(family,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }

          // Try anything
          if(!font){
            font=match(FXString::null,FXString::null,wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints,res);
            }
          }
        }

      // If we still don't have a font yet, use fixed font
      if(!font){

        // Resolve font name
        actualName="fixed";

        // Try load the font
        font=XLoadQueryFont(DISPLAY(getApp()),actualName.text());
        }

      // Remember font id
      if(font){ xid=((XFontStruct*)font)->fid; }

      // Uh-oh, we failed
      if(!xid){ throw FXFontException("unable to create font"); }

#endif

      // What was really matched
      FXTRACE((100,"wantedName=%s wantedSize=%d wantedWeight=%d wantedSlant=%d wantedSetwidth=%d wantedEncoding=%d\n",wantedName.text(),wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding));
      FXTRACE((100,"actualName=%s actualSize=%d actualWeight=%d actualSlant=%d actualSetwidth=%d actualEncoding=%d\n",actualName.text(),actualSize,actualWeight,actualSlant,actualSetwidth,actualEncoding));
      }
    }
  }


// Detach font
void FXFont::detach(){
  if(xid){
    FXTRACE((TOPIC_CREATION,"%s::detach %p\n",getClassName(),this));

#if defined(WIN32)              ///// WIN32 /////

    // Free font metrics
    freeElms(font);

#elif defined(HAVE_XFT_H)       ///// XFT /////

    XftFontClose(DISPLAY(getApp()),(XftFont*)font);

#else                           ///// XLFD /////

    XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);

#endif

    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=nullptr;
    xid=0;
    }
  }


// Destroy font
void FXFont::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((TOPIC_CREATION,"%s::destroy %p\n",getClassName(),this));

#if defined(WIN32)              ///// WIN32 /////

      // Necessary to prevent resource leak
      SelectObject((HDC)dc,GetStockObject(SYSTEM_FONT));

      // Delete font
      DeleteObject((HFONT)xid);

      // Delete dummy DC
      DeleteDC((HDC)dc);

      // Free font metrics
      freeElms(font);

#elif defined(HAVE_XFT_H)       ///// XFT /////

      // Free font
      XftFontClose(DISPLAY(getApp()),(XftFont*)font);

#else                           ///// XLFD /////

      // Free font
      XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);

#endif
      }

    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=nullptr;
    xid=0;
    }
  }


/*******************************************************************************/


// Set to new angle, in degrees*64 relative to positive x axis
void FXFont::setAngle(FXint ang){
  if(xid){ fxerror("%s::setAngle: font has already been created.\n",getClassName()); }
  angle=(ang+34560)%23040-11520;
  if(angle!=ang){
    angle=ang;
    }
  }

/*
WINGDIAPI DWORD WINAPI GetGlyphIndices(
  HDC hdc,       // handle to DC
  LPCTSTR lpstr, // string to convert
  int c,         // number of characters in string
  LPWORD pgi,    // array of glyph indices
  DWORD fl       // glyph options
);
Parameters
hdc
[in] Handle to the device context.
lpstr
[in] Pointer to the string to be converted.
c
[in] Number of characters in pgi.
pgi
[out] Array of glyph indices corresponding to the characters in the string.
fl
[in] Specifies how glyphs should be handled if they are not supported. This parameter can be the following value. Value Meaning
GGI_MARK_NONEXISTING_GLYPHS Marks unsupported glyphs with the hexadecimal value 0xffff.
*/

// Does font have given character glyph?
FXbool FXFont::hasChar(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    // FIXME may want to use GetGlyphIndices()
    return ((TEXTMETRIC*)font)->tmFirstChar<=ch && ch<=((TEXTMETRIC*)font)->tmLastChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return XftCharExists(DISPLAY(getApp()),(XftFont*)font,ch);
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    const XCharStruct *cm;
    FXuchar row=ch>>8;
    FXuchar col=ch&255;
    if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
      if(!fs->per_char) return true;
      cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
      if(cm->width || cm->ascent || cm->descent || cm->rbearing || cm->lbearing) return true;
      }
#endif
    }
  return false;
  }


// Get first character glyph in font
FXwchar FXFont::getMinChar() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmFirstChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    return (((XFontStruct*)font)->min_byte1<<8)|((XFontStruct*)font)->min_char_or_byte2;
#endif
    }
  return 0;
  }


// Get last character glyph in font
FXwchar FXFont::getMaxChar() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmLastChar;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0x10ffff;                                    // FIXME
#else                           ///// XLFD /////
    return (((XFontStruct*)font)->max_byte1<<8)|((XFontStruct*)font)->max_char_or_byte2;
#endif
    }
  return 0;
  }


// Get font leading [that is lead-ing as in Pb!]
FXint FXFont::getFontLeading() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmExternalLeading;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent-((XFontStruct*)font)->max_bounds.ascent-((XFontStruct*)font)->max_bounds.descent;
#endif
    }
  return 0;
  }


// Get font line spacing [height+leading]
FXint FXFont::getFontSpacing() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmHeight;               // Includes font point size plus internal leading
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Left bearing
FXint FXFont::leftBearing(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return 0;                                           // FIXME
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    if(fs->per_char){
      FXuchar row=ch>>8;
      FXuchar col=ch&255;
      if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
        const XCharStruct *cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
        if(cm->width || cm->ascent || cm->descent) return cm->lbearing;
        }
      return fs->per_char[((fs->default_char>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+((fs->default_char&255)-fs->min_char_or_byte2)].lbearing;
      }
    return fs->min_bounds.lbearing;
#endif
    }
  return 0;
  }


// Right bearing
FXint FXFont::rightBearing(FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return 0;                                           // FIXME
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return 0;                                           // FIXME
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    if(fs->per_char){
      FXuchar row=ch>>8;
      FXuchar col=ch&255;
      if(fs->min_char_or_byte2<=col && col<=fs->max_char_or_byte2 && fs->min_byte1<=row && row<=fs->max_byte1){
        const XCharStruct *cm=fs->per_char+((row-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1))+(col-fs->min_char_or_byte2);
        if(cm->width || cm->ascent || cm->descent) return cm->rbearing;
        }
      return fs->per_char[((fs->default_char>>8)-fs->min_byte1)*(fs->max_char_or_byte2-fs->min_char_or_byte2+1)+((fs->default_char&255)-fs->min_char_or_byte2)].rbearing;
      }
    return fs->min_bounds.rbearing;
#endif
    }
  return 0;
  }


// Is it a mono space font
FXbool FXFont::isFontMono() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return !(((TEXTMETRIC*)font)->tmPitchAndFamily&TMPF_FIXED_PITCH);
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo i_extents,m_extents;
    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)"i",1,&i_extents); // FIXME better than before but no cigar yet
    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)"M",1,&m_extents);
    return i_extents.xOff==m_extents.xOff;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->min_bounds.width == ((XFontStruct*)font)->max_bounds.width;
#endif
    }
  return true;
  }


// Get font width
FXint FXFont::getFontWidth() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmMaxCharWidth;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)font)->max_advance_width;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->max_bounds.width;
#endif
    }
  return 1;
  }


// Get font height
FXint FXFont::getFontHeight() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Get font ascent
FXint FXFont::getFontAscent() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmAscent;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)font)->ascent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->ascent;
#endif
    }
  return 1;
  }


// Get font descent
FXint FXFont::getFontDescent() const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)font)->tmDescent;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct*)font)->descent;
#endif
    }
  return 0;
  }


// Calculate width of single wide character in this font
FXint FXFont::getCharWidth(const FXwchar ch) const {
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    FXnchar sbuffer[2];
    SIZE size;
    sbuffer[0]=ch;
    if(0xFFFF<ch){                      // Deal with surrogate pair
      sbuffer[0]=(ch>>10)+LEAD_OFFSET;
      sbuffer[1]=(ch&0x3FF)+TAIL_OFFSET;
      GetTextExtentPoint32W((HDC)dc,sbuffer,2,&size);
      return size.cx;
      }
    GetTextExtentPoint32W((HDC)dc,sbuffer,1,&size);
    return size.cx;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    XftTextExtents32(DISPLAY(getApp()),(XftFont*)font,(const FcChar32*)&ch,1,&extents);
    return extents.xOff;
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    FXint width,size;
    FXuchar r,c;
    if(fs->per_char){
      r=ch>>8;
      c=ch&255;
      size=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        width=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        if(width) return width;
        }
      r=fs->default_char>>8;
      c=fs->default_char&255;
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        return fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        }
      }
    return fs->min_bounds.width;
#endif
    }
  return 1;
  }


// Text width
FXint FXFont::getTextWidth(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextWidth: NULL string argument\n",getClassName()); }
  if(font){
#if defined(WIN32)              ///// WIN32 /////
    FXnchar sbuffer[4096];
    FXint count=utf2ncs(sbuffer,string,ARRAYNUMBER(sbuffer),length);
    SIZE size;
    GetTextExtentPoint32W((HDC)dc,sbuffer,count,&size);
    return size.cx;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    // This returns rotated metrics; FOX likes to work with unrotated metrics, so if angle
    // is not 0, we calculate the unrotated baseline; note however that the calculation is
    // not 100% pixel exact when the angle is not a multiple of 90 degrees.
    XftTextExtentsUtf8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)string,length,&extents);
    if(angle){ return (FXint)(0.5+Math::sqrt((FXdouble)(extents.xOff*extents.xOff+extents.yOff*extents.yOff))); }
    return extents.xOff;
#else                           ///// XLFD /////
    const XFontStruct *fs=(XFontStruct*)font;
    FXint defwidth=fs->min_bounds.width;
    FXint width=0,ww;
    FXuint p=0;
    FXuint s;
    FXuchar r;
    FXuchar c;
    FXwchar w;
    if(fs->per_char){
      r=fs->default_char>>8;
      c=fs->default_char&255;
      s=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        defwidth=fs->per_char[(r-fs->min_byte1)*s+(c-fs->min_char_or_byte2)].width;
        }
      while(p<length){
        w=wc(string+p);
        p+=wclen(string+p);
        r=w>>8;
        c=w&255;
        if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
          if((ww=fs->per_char[(r-fs->min_byte1)*s+(c-fs->min_char_or_byte2)].width)!=0){
            width+=ww;
            continue;
            }
          }
        width+=defwidth;
        }
      }
    else{
      while(p<length){
        p+=wclen(string+p);
        width+=defwidth;
        }
      }
    return width;
#endif
    }
  return length;
  }


// Text width
FXint FXFont::getTextWidth(const FXString& string) const {
  return getTextWidth(string.text(),string.length());
  }


// Text height
FXint FXFont::getTextHeight(const FXchar *string,FXuint length) const {
  if(!string && length){ fxerror("%s::getTextHeight: NULL string argument\n",getClassName()); }
  if(font){
#if defined(WIN32)              ///// WIN32 /////
//    SIZE size;
//    FXASSERT(dc!=nullptr);
//    GetTextExtentPoint32((HDC)dc,string,length,&size);
//    return size.cy;
    return ((TEXTMETRIC*)font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
//    XGlyphInfo extents;
//    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)text,n,&extents);
//    return extents.height; // TODO: Is this correct?
    // Patch from ivan.markov@wizcom.bg
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else                           ///// XLFD /////
//    XCharStruct chst; int dir,asc,desc;
//    XTextExtents((XFontStruct*)font,string,length,&dir,&asc,&desc,&chst);
//    return asc+desc;
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
    }
  return 1;
  }


// Text height
FXint FXFont::getTextHeight(const FXString& string) const {
  return getTextHeight(string.text(),string.length());
  }


/*
static FX88591Codec codec_8859_1;
static FX88592Codec codec_8859_2;
static FX88593Codec codec_8859_3;
static FX88594Codec codec_8859_4;
static FX88595Codec codec_8859_5;
static FX88596Codec codec_8859_6;
static FX88597Codec codec_8859_7;
static FX88598Codec codec_8859_8;
static FX88599Codec codec_8859_9;
static FX885910Codec codec_8859_10;
static FX885911Codec codec_8859_11;
static FX885913Codec codec_8859_13;
static FX885914Codec codec_8859_14;
static FX885915Codec codec_8859_15;
static FX885916Codec codec_8859_16;

  FXchar sbuffer[4096];
  switch(actualEncoding){
    case FONTENCODING_ISO_8859_1: count=codec_8859_1.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_2: count=codec_8859_2.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_3: count=codec_8859_3.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_4: count=codec_8859_4.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_5: count=codec_8859_5.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_6: count=codec_8859_6.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_7: count=codec_8859_7.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_8: count=codec_8859_8.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_9: count=codec_8859_9.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_10: count=codec_8859_10.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_11: count=codec_8859_11.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_13: count=codec_8859_13.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_14: count=codec_8859_14.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_15: count=codec_8859_15.utf2mb(sbuffer,4096,string,length); break;
    case FONTENCODING_ISO_8859_16: count=codec_8859_16.utf2mb(sbuffer,4096,string,length); break;
    default: count=0; break;
    }
  XDrawString(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,x,y,sbuffer,count);
*/


/*
  /// Draw text starting at x,y
  virtual void drawText(FXDC* dc,FXint x,FXint y,const FXString& string) const;

  /// Draw text starting at x,y
  virtual void drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const;

  /// Draw text starting at x,y over filled background
  virtual void drawImageText(FXDC* dc,FXint x,FXint y,const FXString& string) const;

  /// Draw text starting at x,y over filled background
  virtual void drawImageText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const;


#if defined(WIN32)              ///// WIN32 /////

// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  FXnchar sbuffer[4096];
  FXint iBkMode=SetBkMode((HDC)dc->ctx,TRANSPARENT);
  FXint count=utf2ncs(sbuffer,string,ARRAYNUMBER(sbuffer),FXMIN(length,4096));
  FXASSERT(count<=length);
  TextOutW((HDC)dc->ctx,x,y,sbuffer,count);
  SetBkMode((HDC)dc->ctx,iBkMode);
  }

#elif defined(HAVE_XFT_H)       ///// XFT /////

// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  XftColor color;
  color.pixel=((FXDCWindow*)dc)->devfg;
  color.color.red=FXREDVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.green=FXGREENVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.blue=FXBLUEVAL(((FXDCWindow*)dc)->fg)*257;
  color.color.alpha=FXALPHAVAL(((FXDCWindow*)dc)->fg)*257;
  XftDrawStringUtf8((XftDraw*)((FXDCWindow*)dc)->xftDraw,&color,(XftFont*)((FXDCWindow*)dc)->font->font,x,y,(const FcChar8*)string,length);
  }

#else                           ///// XLFD /////

static FXint utf2db(XChar2b *dst,const FXchar *src,FXint n){
  FXint len,p;
  FXwchar w;
  for(p=len=0; p<n; p+=wclen(src+p),len++){
    w=wc(src+p);
    dst[len].byte1=(w>>8);
    dst[len].byte2=(w&255);
    }
  return len;
  }


// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  const XFontStruct *fs=(XFontStruct*)font;
  FXint count,escapement,defwidth,ww,size,i;
  FXdouble ang,ux,uy;
  FXuchar r,c;
  XChar2b sbuffer[4096];
  count=utf2db(sbuffer,string,FXMIN(length,4096));
  FXASSERT(count<=length);
  if(angle){
    ang=angle*0.00027270769562411399179;
    defwidth=fs->min_bounds.width;
    ux=cos(ang);
    uy=sin(ang);
    if(fs->per_char){
      r=fs->default_char>>8;
      c=fs->default_char&255;
      size=(fs->max_char_or_byte2-fs->min_char_or_byte2+1);
      if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
        defwidth=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width;
        }
      for(i=escapement=0; i<count; i++){
        XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,(FXint)(x+escapement*ux),(FXint)(y-escapement*uy),&sbuffer[i],1);
        r=sbuffer[i].byte1;
        c=sbuffer[i].byte2;
        escapement+=defwidth;
        if(fs->min_char_or_byte2<=c && c<=fs->max_char_or_byte2 && fs->min_byte1<=r && r<=fs->max_byte1){
          if((ww=fs->per_char[(r-fs->min_byte1)*size+(c-fs->min_char_or_byte2)].width)!=0) escapement+=ww-defwidth;
          }
        }
      }
    else{
      for(i=escapement=0; i<count; i++){
        XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,(FXint)(x+escapement*ux),(FXint)(y-escapement*uy),&sbuffer[i],1);
        escapement+=defwidth;
        }
      }
    }
  else{
    XDrawString16(DISPLAY(getApp()),((FXDCWindow*)dc)->surface->id(),(GC)((FXDCWindow*)dc)->ctx,x,y,sbuffer,count);
    }
  }


#endif


// Draw text starting at x,y
void FXFont::drawText(FXDC* dc,FXint x,FXint y,const FXString& string) const {
  drawText(dc,x,y,string.text(),string.length());
  }



// Draw text starting at x,y over filled background
void FXFont::drawImageText(FXDC* dc,FXint x,FXint y,const FXchar* string,FXuint length) const {
  // ...
  }


// Draw text starting at x,y over filled background
void FXFont::drawImageText(FXDC* dc,FXint x,FXint y,const FXString& string) const {
  drawImageText(dc,x,y,string.text(),string.length());
  }
*/


/*******************************************************************************/


// Function to sort by name, weight, slant, and size
static int CDECL comparefont(const void *a,const void *b){
  const FXFontDesc *fa=(const FXFontDesc*)a;
  const FXFontDesc *fb=(const FXFontDesc*)b;
  FXint cmp=strcmp(fa->face,fb->face);
  return cmp ? cmp : (fa->weight!=fb->weight) ? fa->weight-fb->weight : (fa->slant!=fb->slant) ? fa->slant-fb->slant : fa->size-fb->size;
  }



#if defined(WIN32)              ///////  MS-Windows ///////

// Need to get some data into the callback function.
struct FXFontStore {
  HDC         hdc;
  FXFontDesc *fonts;
  FXuint      numfonts;
  FXuint      size;
  FXFontDesc  desc;
  };


// Callback function for EnumFontFamiliesEx()
static int CALLBACK EnumFontFamExProc(const LOGFONTA *lf,const TEXTMETRICA *lptm,DWORD FontType,LPARAM lParam){
  FXFontStore *pFontStore=(FXFontStore*)lParam;
  FXASSERT(lf);
  FXASSERT(lptm);
  FXASSERT(pFontStore);

  // Get pitch
  FXuint flags=0;
  if(lf->lfPitchAndFamily&FIXED_PITCH) flags|=FXFont::Fixed;
  if(lf->lfPitchAndFamily&VARIABLE_PITCH) flags|=FXFont::Variable;

  // Get hints
  if(lf->lfPitchAndFamily&FF_DONTCARE) flags|=0;
  if(lf->lfPitchAndFamily&FF_MODERN) flags|=FXFont::Modern;
  if(lf->lfPitchAndFamily&FF_ROMAN) flags|=FXFont::Roman;
  if(lf->lfPitchAndFamily&FF_SCRIPT) flags|=FXFont::Script;
  if(lf->lfPitchAndFamily&FF_DECORATIVE) flags|=FXFont::Decorative;
  if(lf->lfPitchAndFamily&FF_SWISS) flags|=FXFont::Swiss;

  // Skip if no match
  FXuint h=pFontStore->desc.flags;
  if((h&FXFont::Fixed) && !(flags&FXFont::Fixed)) return 1;
  if((h&FXFont::Variable) && !(flags&FXFont::Variable)) return 1;

  // Get weight (also guess from the name)
  FXuint weight=lf->lfWeight/10;
  if(strstr(lf->lfFaceName,"Bold")!=nullptr) weight=FXFont::Bold;
  if(strstr(lf->lfFaceName,"Black")!=nullptr) weight=FXFont::Black;
  if(strstr(lf->lfFaceName,"Demi")!=nullptr) weight=FXFont::DemiBold;
  if(strstr(lf->lfFaceName,"Light")!=nullptr) weight=FXFont::Light;
  if(strstr(lf->lfFaceName,"Medium")!=nullptr) weight=FXFont::Medium;

  // Skip if weight doesn't match
  FXuint wt=pFontStore->desc.weight;
  if((wt!=0) && (wt!=weight)) return 1;

  // Get slant
  FXuint slant=FXFont::Straight;
  if(lf->lfItalic) slant=FXFont::Italic;
  if(strstr(lf->lfFaceName,"Italic")!=nullptr) slant=FXFont::Italic;
  if(strstr(lf->lfFaceName,"Roman")!=nullptr) slant=FXFont::Straight;

  // Skip if no match
  FXuint sl=pFontStore->desc.slant;
  if((sl!=0) && (sl!=slant)) return 1;

  // Get set width (also guess from the name)
  FXuint setwidth=0;
  if(strstr(lf->lfFaceName,"Cond")!=nullptr) setwidth=FXFont::Condensed;
  if(strstr(lf->lfFaceName,"Narrow")!=nullptr) setwidth=FXFont::Condensed;
  if(strstr(lf->lfFaceName,"Ext Cond")!=nullptr) setwidth=FXFont::ExtraCondensed;

  // Skip if no match
  FXuint sw=pFontStore->desc.setwidth;
  if((sw!=0) && (sw!=setwidth)) return 1;

  // Get encoding
  FXuint encoding=CharSet2FXFontEncoding(lf->lfCharSet);

  // Skip if no match
  FXuint en=pFontStore->desc.encoding;
  if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) return 1;

  // Is it scalable?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FXFont::Scalable;
    }

  // Is it polymorphic?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FXFont::Polymorphic;
    }

  // Initial allocation of storage?
  if(pFontStore->numfonts==0){
    allocElms(pFontStore->fonts,50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size=50;
    }

  // Grow the array if needed
  if(pFontStore->numfonts>=pFontStore->size){
    resizeElms(pFontStore->fonts,pFontStore->size+50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size+=50;
    }

  FXFontDesc *fonts=pFontStore->fonts;
  FXuint numfonts=pFontStore->numfonts;

  fxstrlcpy(fonts[numfonts].face,lf->lfFaceName,116);
  if(lf->lfHeight<0){
    fonts[numfonts].size=-MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  else{
    fonts[numfonts].size=MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  fonts[numfonts].weight=weight;
  fonts[numfonts].slant=slant;
  fonts[numfonts].encoding=encoding;
  fonts[numfonts].setwidth=setwidth;
  fonts[numfonts].flags=flags;

  pFontStore->fonts=fonts;
  pFontStore->numfonts++;

  // Must return 1 to continue enumerating fonts
  return 1;
  }


// List all fonts matching hints
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  FXuint i,j;

  // Initialize return values
  fonts=nullptr;
  numfonts=0;

  // This data gets passed into the callback function
  FXFontStore fontStore;
  HDC hdc=GetDC(GetDesktopWindow());
  SaveDC(hdc);
  fontStore.hdc=hdc;
  fontStore.fonts=fonts;
  fontStore.numfonts=numfonts;
  fontStore.desc.weight=wt;
  fontStore.desc.slant=sl;
  fontStore.desc.setwidth=sw;
  fontStore.desc.encoding=en;
  fontStore.desc.flags=h;

  // Fill in the appropriate fields of the LOGFONT structure. Note that
  // EnumFontFamiliesEx() only examines the lfCharSet, lfFaceName and
  // lpPitchAndFamily fields of this struct.
  LOGFONTA lf;
  lf.lfHeight=0;
  lf.lfWidth=0;
  lf.lfEscapement=0;
  lf.lfOrientation=0;
  lf.lfWeight=0;
  lf.lfItalic=0;
  lf.lfUnderline=0;
  lf.lfStrikeOut=0;
  lf.lfCharSet=FXFontEncoding2CharSet(en);
  lf.lfOutPrecision=0;
  lf.lfClipPrecision=0;
  lf.lfQuality=0;
  lf.lfPitchAndFamily=0;                          // Should be MONO_FONT for Hebrew and Arabic?
  FXASSERT(face.length()<LF_FACESIZE);
  fxstrlcpy(lf.lfFaceName,face.text(),LF_FACESIZE);

  // Start enumerating!
  EnumFontFamiliesExA(hdc,&lf,EnumFontFamExProc,(LPARAM)&fontStore,0);
  RestoreDC(hdc,-1);
  ReleaseDC(GetDesktopWindow(),hdc);

  // Copy stuff back from the store
  fonts=fontStore.fonts;
  numfonts=fontStore.numfonts;

  // Any fonts found?
  if(numfonts==0){
    freeElms(fonts);
    return false;
    }

  // Sort them by name, weight, slant, and size respectively
  ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);

  // Weed out duplicates if we were just listing the face names
  if(lf.lfCharSet==DEFAULT_CHARSET && lf.lfFaceName[0]==0){
    i=j=1;
    while(j<numfonts){
      if(strcmp(fonts[i-1].face,fonts[j].face)!=0){
        fonts[i]=fonts[j];
        i++;
        }
      j++;
      }
    numfonts=i;
    }

  // Realloc to shrink the block
  resizeElms(fonts,numfonts);

//   FXTRACE((150,"%d fonts:\n",numfonts));
//   for(FXuint f=0; f<numfonts; f++){
//     FXTRACE((150,"Font=%s weight=%d slant=%d size=%3d setwidth=%d encoding=%d\n",fonts[f].face,fonts[f].weight,fonts[f].slant,fonts[f].size,fonts[f].setwidth,fonts[f].encoding));
//     }
//   FXTRACE((150,"\n\n"));

  return true;
  }


#elif defined(HAVE_XFT_H)       ///////  X Freetype ///////


// List all fonts that match the passed requirements
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  int          encoding,setwidth,weight,slant,size,pitch,scalable,res,i,j;
  FXchar       fullname[256];
  FcPattern   *pattern,*p;
  FcObjectSet *objset;
  FcFontSet   *fontset;
  FcChar8     *fam,*fdy;
  FcBool       scale;
  FcCharSet   *charset;
  FXString     family;
  FXString     foundry;
  double       points;

  fonts=nullptr;
  numfonts=0;

  // Need to have application
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }

  // Need to have display open
  if(!DISPLAY(FXApp::instance())){ fxerror("FXFont::listFonts: trying to list fonts before opening display.\n"); }

  // Get family part of name
  family=face.before('[').trimEnd();

  // Get foundry part of name
  foundry=face.section("[]",1);

  FXTRACE((150,"FXFont::listFonts: family=\"%s\" foundry=\"%s\" weight=%d slant=%d setwidth=%d encoding=%d hints=%x\n",family.text(),foundry.text(),wt,sl,sw,en,h));

  // Screen resolution may be overidden by registry
  res=FXApp::instance()->reg().readUIntEntry("SETTINGS","screenres",100);

  // Build object set
#ifdef FC_WIDTH
  objset=FcObjectSetBuild(FC_FAMILY,FC_FOUNDRY,FC_SPACING,FC_SCALABLE,FC_WIDTH,FC_WEIGHT,FC_SLANT,FC_PIXEL_SIZE,nullptr);
#else
  objset=FcObjectSetBuild(FC_FAMILY,FC_FOUNDRY,FC_SPACING,FC_SCALABLE,FC_WEIGHT,FC_SLANT,FC_PIXEL_SIZE,nullptr);
#endif
  if(objset){

    // Create pattern object
    pattern=FcPatternCreate();
    if(pattern){

      // Set family
      if(!family.empty()){
        FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)family.text());
        }

      // Set foundry
      if(!foundry.empty()){
        FcPatternAddString(pattern,FC_FOUNDRY,(const FcChar8*)foundry.text());
        }

      // If we set this we get no fonts
//      if(h&FXFont::Rotatable){
//        const FcMatrix matrix={1.0,0.0,0.0,1.0};
//        FcPatternAddMatrix(pattern,FC_MATRIX,&matrix);
//        }

      // List fonts matching pattern
      fontset=FcFontList(0,pattern,objset);
      if(fontset && 0<fontset->nfont){

        // Allocate return array
        if(allocElms(fonts,fontset->nfont)){

          // Collect the info now...
          for(i=0; i<fontset->nfont; i++){
            p=fontset->fonts[i];

            // Get full face name
            fullname[0]=0;
            if(FcPatternGetString(p,FC_FAMILY,0,&fam)==FcResultMatch){
              fxstrlcpy(fullname,(const char*)fam,sizeof(fullname));
              if(FcPatternGetString(p,FC_FOUNDRY,0,&fdy)==FcResultMatch){
                fxstrlcat(fullname," [",sizeof(fullname));
                fxstrlcat(fullname,(const char*)fdy,sizeof(fullname));
                fxstrlcat(fullname,"]",sizeof(fullname));
                }
              }

#ifdef FC_WIDTH
            // Get setwidth
            setwidth=0;
            if(FcPatternGetInteger(p,FC_WIDTH,0,&setwidth)==FcResultMatch){
              setwidth=fcSetWidth2SetWidth(setwidth);
              }
#endif

            // Get weight
            weight=0;
            if(FcPatternGetInteger(p,FC_WEIGHT,0,&weight)==FcResultMatch){
              weight=fcWeight2Weight(weight);
              }

            // Get slant
            slant=0;
            if(FcPatternGetInteger(p,FC_SLANT,0,&slant)==FcResultMatch){
              slant=fcSlant2Slant(slant);
              }

            // Get pitch
            pitch=FXFont::Variable;
            if(FcPatternGetInteger(p,FC_SPACING,0,&pitch)==FcResultMatch){
//              if(pitch==FC_MONO || pitch==FC_DUAL || pitch==FC_CHARCELL) pitch=FXFont::Fixed;
              if(pitch==FC_MONO || pitch==FC_CHARCELL) pitch=FXFont::Fixed;
              }

            // Pixel size works for both bitmap and scalable fonts
            size=0;
            if(FcPatternGetDouble(p,FC_PIXEL_SIZE,0,&points)==FcResultMatch){
              size=(int)((720.0*points)/res);
              }

            // Get scalable flag
            scalable=0;
            if(FcPatternGetBool(p,FC_SCALABLE,0,&scale)==FcResultMatch){
              if(scale) scalable=FXFont::Scalable;
              }

            // Get charset
            if(FcPatternGetCharSet(p,FC_CHARSET,0,&charset)==FcResultMatch){    // FIXME
              }

            // Get the encoding
            encoding=FONTENCODING_UNICODE;

            FXTRACE((160,"wt=%2d sl=%d sw=%3d en=%5d sz=%3d sc=%4x pi=%d name=%s\n",weight,slant,setwidth,encoding,size,scalable,pitch,fullname));

            // Skip if pitch does not match
            if((h&FXFont::Fixed) && (pitch!=FXFont::Fixed)) continue;
            if((h&FXFont::Variable) && (pitch!=FXFont::Variable)) continue;

            // Skip if weight does not match
            if((wt!=0) && (wt!=weight)) continue;

            // Skip if slant does not match
            if((sl!=0) && (sl!=slant)) continue;

            // Skip if setwidth does not match
            if((sw!=0) && (sw!=setwidth)) continue;

            // Want scalable
            if((h&FXFont::Scalable) && (scalable!=FXFont::Scalable)) continue;

            // If NULL face name, just list one of each face
            if(family.empty()){
              for(j=numfonts-1; j>=0; j--){
                if(strcmp(fullname,fonts[j].face)==0) goto next;
                }
              }

            // Add this font
            fxstrlcpy(fonts[numfonts].face,fullname,116);
            fonts[numfonts].size=size;
            fonts[numfonts].weight=weight;
            fonts[numfonts].slant=slant;
            fonts[numfonts].encoding=encoding;
            fonts[numfonts].setwidth=setwidth;
            fonts[numfonts].flags=pitch|scalable;
            numfonts++;

            // Next font
next:       continue;
            }

          // Realloc to shrink the block
          resizeElms(fonts,numfonts);

          // Sort them by name, weight, slant, and size respectively
          ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);
          }
        FcFontSetDestroy(fontset);
        }
      FcPatternDestroy(pattern);
      }
    FcObjectSetDestroy(objset);
    }
  return (0<numfonts);
  }


#else                           ///////  X XLFD ///////


// Try find matching font
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  FXuint   encoding,weight,slant,setwidth,pitch,scalable,rotatable,polymorph,xres,yres,points,size,res;
  FXchar   candidate[256],fullname[256];
  const FXchar  *field[13];
  FXchar **fontnames;
  FXint    nfontnames,f,j;
  FXString family;
  FXString foundry;

  fonts=nullptr;
  numfonts=0;

  // Need to have application
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }

  // Need to have display open
  if(!DISPLAY(FXApp::instance())){ fxerror("FXFont::listFonts: trying to list fonts before opening display.\n"); }

  // Screen resolution may be overidden by registry
  res=FXApp::instance()->reg().readUIntEntry("SETTINGS","screenres",100);

  // Get family part of name
  family=face.before('[').trimEnd();

  // Get foundry part of name
  foundry=face.section("[]",1);

  FXTRACE((150,"FXFont::listFonts: family=\"%s\" foundry=\"%s\" weight=%d slant=%d setwidth=%d encoding=%d hints=%x\n",family.text(),foundry.text(),wt,sl,sw,en,h));

  // Match RAW X11
  if(h&FXFont::X11){
    __snprintf(candidate,sizeof(candidate),"%s",face.empty()?"*":face.text());
    }

  // Match XLFD
  else{
    __snprintf(candidate,sizeof(candidate),"-%s-%s-*-*-*-*-*-%s-*-*-*-*-*-*",foundry.empty() ? "*" : foundry.text(),family.empty() ? "*" : family.text(),(h&FXFont::Rotatable) ? "[1 0 0 1]" : "*");
    }

  // Get fonts matching the pattern
  fontnames=XListFonts(DISPLAY(FXApp::instance()),candidate,65535,&nfontnames);
  if(fontnames && 0<nfontnames){

    // Allocate return array
    if(!allocElms(fonts,nfontnames)){ XFreeFontNames(fontnames); return false; }

    // List them
    for(f=0; f<nfontnames; f++){

      // Break apart into fields
      fxstrlcpy(candidate,fontnames[f],sizeof(candidate));
      xlfdSplit(field,candidate);

      // Get info
      weight=xlfdWeight(field[2]);
      slant=xlfdSlant(field[3]);
      setwidth=xlfdSetwidth(field[4]);
      scalable=xlfdScalable(field[6],field[7],field[11]);
      polymorph=xlfdPolymorph(field[2],field[3],field[4],field[5]);
      rotatable=xlfdRotatable(field[6],field[7]);
      points=atoi(field[7]);
      xres=atoi(field[8]);
      yres=atoi(field[9]);
      pitch=xlfdPitch(field[10]);
      encoding=xlfdEncoding(field[12]);

      FXTRACE((160,"wt=%2d sl=%d sw=%3d en=%5d pt=%3d sc=%4x po=%4x ro=%4x xlfd=%s\n",weight,slant,setwidth,encoding,points,scalable,polymorph,rotatable,fontnames[f]));

      // The font can be rendered at any resolution
      if(xres==0 && yres==0){ xres=res; yres=res; }

      // Skip if encoding does not match
      if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) continue;

      // Skip if pitch does not match
      if((h&FXFont::Fixed) && (pitch!=FXFont::Fixed)) continue;
      if((h&FXFont::Variable) && (pitch!=FXFont::Variable)) continue;

      // Skip if weight does not match
      if((wt!=0) && (wt!=weight)) continue;

      // Skip if slant does not match
      if((sl!=0) && (sl!=slant)) continue;

      // Skip if setwidth does not match
      if((sw!=0) && (sw!=setwidth)) continue;

      // Want rotatable
      if((h&FXFont::Rotatable) && (rotatable!=FXFont::Rotatable)) continue;

      // Want scalable
      if((h&FXFont::Scalable) && (scalable!=FXFont::Scalable)) continue;

      // If scalable, we can of course get the exact size we want
      if(scalable){
        size=0;
        }

      // Correct for the actual screen resolution
      else{
        size=(yres*points)/res;
        }

      // Get full face name
      fxstrlcpy(fullname,field[1],sizeof(fullname));
      if(field[0][0]){
        fxstrlcat(fullname," [",sizeof(fullname));
        fxstrlcat(fullname,field[0],sizeof(fullname));
        fxstrlcat(fullname,"]",sizeof(fullname));
        }

      // If NULL face name, just list one of each face
      if(family.empty()){
        for(j=numfonts-1; j>=0; j--){
          if(strcmp(fullname,fonts[j].face)==0) goto next;
          }
        }

      // Add this font
      fxstrlcpy(fonts[numfonts].face,fullname,116);
      fonts[numfonts].size=size;
      fonts[numfonts].weight=weight;
      fonts[numfonts].slant=slant;
      fonts[numfonts].encoding=encoding;
      fonts[numfonts].setwidth=setwidth;
      fonts[numfonts].flags=pitch|scalable|polymorph|rotatable;
      numfonts++;

      // Next font
next: continue;
      }

    // Free the list
    XFreeFontNames(fontnames);

    // Realloc to shrink the block
    resizeElms(fonts,numfonts);

    // Sort them by name, weight, slant, and size respectively
    ::qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);
    }
  return (0<numfonts);
  }


#endif


/*******************************************************************************/


// For tables
struct ENTRY { const FXchar *name; FXuint value; };


// Character set encodings
static const ENTRY encodingtable[]={
  {"",FONTENCODING_DEFAULT},
  {"iso10646-1",FONTENCODING_UNICODE},
  {"iso8859-1",FONTENCODING_ISO_8859_1},
  {"iso8859-2",FONTENCODING_ISO_8859_2},
  {"iso8859-3",FONTENCODING_ISO_8859_3},
  {"iso8859-4",FONTENCODING_ISO_8859_4},
  {"iso8859-5",FONTENCODING_ISO_8859_5},
  {"iso8859-6",FONTENCODING_ISO_8859_6},
  {"iso8859-7",FONTENCODING_ISO_8859_7},
  {"iso8859-8",FONTENCODING_ISO_8859_8},
  {"iso8859-9",FONTENCODING_ISO_8859_9},
  {"iso8859-10",FONTENCODING_ISO_8859_10},
  {"iso8859-11",FONTENCODING_ISO_8859_11},
  {"iso8859-13",FONTENCODING_ISO_8859_13},
  {"iso8859-14",FONTENCODING_ISO_8859_14},
  {"iso8859-15",FONTENCODING_ISO_8859_15},
  {"iso8859-16",FONTENCODING_ISO_8859_16},
  {"koi8",FONTENCODING_KOI8},
  {"koi8-r",FONTENCODING_KOI8_R},
  {"koi8-u",FONTENCODING_KOI8_U},
  {"koi8-unified",FONTENCODING_KOI8_UNIFIED},
  {"cp437",FONTENCODING_CP437},
  {"cp850",FONTENCODING_CP850},
  {"cp851",FONTENCODING_CP851},
  {"cp852",FONTENCODING_CP852},
  {"cp855",FONTENCODING_CP855},
  {"cp856",FONTENCODING_CP856},
  {"cp857",FONTENCODING_CP857},
  {"cp860",FONTENCODING_CP860},
  {"cp861",FONTENCODING_CP861},
  {"cp862",FONTENCODING_CP862},
  {"cp863",FONTENCODING_CP863},
  {"cp864",FONTENCODING_CP864},
  {"cp865",FONTENCODING_CP865},
  {"cp866",FONTENCODING_CP866},
  {"cp869",FONTENCODING_CP869},
  {"cp870",FONTENCODING_CP870},
  {"cp1250",FONTENCODING_CP1250},
  {"cp1251",FONTENCODING_CP1251},
  {"cp1252",FONTENCODING_CP1252},
  {"cp1253",FONTENCODING_CP1253},
  {"cp1254",FONTENCODING_CP1254},
  {"cp1255",FONTENCODING_CP1255},
  {"cp1256",FONTENCODING_CP1256},
  {"cp1257",FONTENCODING_CP1257},
  {"cp1258",FONTENCODING_CP1258},
  {"cp874",FONTENCODING_CP874},
  {"ascii",FONTENCODING_ISO_8859_1}
  };


// Font style table
static const ENTRY styletable[]={
  {"",0},
  {"decorative",FXFont::Decorative},
  {"modern",FXFont::Modern},
  {"roman",FXFont::Roman},
  {"script",FXFont::Script},
  {"swiss",FXFont::Swiss},
  {"system",FXFont::System}
  };


// Font pitch table
static const ENTRY pitchtable[]={
  {"",0},
  {"mono",FXFont::Fixed},
  {"fixed",FXFont::Fixed},
  {"constant",FXFont::Fixed},
  {"variable",FXFont::Variable},
  {"proportional",FXFont::Variable},
  {"c",FXFont::Fixed},
  {"m",FXFont::Fixed},
  {"p",FXFont::Variable}
  };


// Font text angles
static const ENTRY slanttable[]={
  {"",0},
  {"regular",FXFont::Straight},
  {"italic",FXFont::Italic},
  {"oblique",FXFont::Oblique},
  {"normal",FXFont::Straight},
  {"reverse italic",FXFont::ReverseItalic},
  {"reverse oblique",FXFont::ReverseOblique},
  {"r",FXFont::Straight},
  {"n",FXFont::Straight},
  {"i",FXFont::Italic},
  {"o",FXFont::Oblique},
  {"ri",FXFont::ReverseItalic},
  {"ro",FXFont::ReverseOblique}
  };


// Set width table
static const ENTRY setwidthtable[]={
  {"",0},
  {"ultracondensed",FXFont::UltraCondensed},
  {"extracondensed",FXFont::ExtraCondensed},
  {"condensed",FXFont::Condensed},
  {"narrow",FXFont::Condensed},
  {"compressed",FXFont::Condensed},
  {"semicondensed",FXFont::SemiCondensed},
  {"medium",FXFont::NonExpanded},
  {"normal",FXFont::NonExpanded},
  {"regular",FXFont::NonExpanded},
  {"semiexpanded",FXFont::SemiExpanded},
  {"expanded",FXFont::Expanded},
  {"wide",FXFont::ExtraExpanded},
  {"extraexpanded",FXFont::ExtraExpanded},
  {"ultraexpanded",FXFont::UltraExpanded},
  {"n",FXFont::Condensed},
  {"r",FXFont::NonExpanded},
  {"c",FXFont::Condensed},
  {"w",FXFont::ExtraExpanded},
  {"m",FXFont::NonExpanded},
  {"x",FXFont::Expanded}
  };


// Weight table
static const ENTRY weighttable[]={
  {"",0},
  {"thin",FXFont::Thin},
  {"extralight",FXFont::ExtraLight},
  {"light",FXFont::Light},
  {"normal",FXFont::Normal},
  {"regular",FXFont::Normal},
  {"medium",FXFont::Medium},
  {"demibold",FXFont::DemiBold},
  {"bold",FXFont::Bold},
  {"extrabold",FXFont::ExtraBold},
  {"heavy",FXFont::Black},
  {"black",FXFont::Black},
  {"b",FXFont::Bold},
  {"l",FXFont::Light},
  {"n",FXFont::Normal},
  {"r",FXFont::Normal},
  {"m",FXFont::Medium},
  };


// Search for value and return name
static FXString findbyvalue(const ENTRY* table,FXint n,FXuint value){
  for(int i=0; i<n; i++){ if(table[i].value==value) return table[i].name; }
  return FXString::value(value);
  }


// Search for name and return value
static FXuint findbyname(const ENTRY* table,FXint n,const FXString& name){
  for(int i=0; i<n; i++){ if(FXString::comparecase(table[i].name,name)==0) return table[i].value; }
  return name.toUInt();
  }


// Get style from string
FXuint FXFont::styleFromString(const FXString& str){
  return str.toUInt();
  }


// Get string from style
FXString FXFont::stringFromStyle(FXuint style){
   return FXString::value(style);
  }


// Get slant from string
FXuint FXFont::slantFromString(const FXString& str){
  return findbyname(slanttable,ARRAYNUMBER(slanttable),str);
  }


// Get string from slant
FXString FXFont::stringFromSlant(FXuint slant){
  return findbyvalue(slanttable,ARRAYNUMBER(slanttable),slant);
  }


// Get weight from string
FXuint FXFont::weightFromString(const FXString& str){
  return findbyname(weighttable,ARRAYNUMBER(weighttable),str);
  }


// Get string from weight
FXString FXFont::stringFromWeight(FXuint weight){
  return findbyvalue(weighttable,ARRAYNUMBER(weighttable),weight);
  }


// Get setwidth from string
FXuint FXFont::setWidthFromString(const FXString& str){
  return findbyname(setwidthtable,ARRAYNUMBER(setwidthtable),str);
  }


// Get string from setwidth
FXString FXFont::stringFromSetWidth(FXuint setwidth){
  return findbyvalue(setwidthtable,ARRAYNUMBER(setwidthtable),setwidth);
  }


// Get encoding from string
FXuint FXFont::encodingFromString(const FXString& str){
  return findbyname(encodingtable,ARRAYNUMBER(encodingtable),str);
  }


// Get string from encoding
FXString FXFont::stringFromEncoding(FXuint encoding){
  return findbyvalue(encodingtable,ARRAYNUMBER(encodingtable),encoding);
  }


// Change font description
void FXFont::setFontDesc(const FXFontDesc& fontdesc){
  wantedName=fontdesc.face;
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  hints=fontdesc.flags;
  }


// Get font description
FXFontDesc FXFont::getFontDesc() const {
  FXFontDesc result;
  fxstrlcpy(result.face,wantedName.text(),sizeof(result.face));
  result.size=wantedSize;
  result.weight=wantedWeight;
  result.slant=wantedSlant;
  result.setwidth=wantedSetwidth;
  result.encoding=wantedEncoding;
  result.flags=hints;
  return result;
  }


// Get actual font description
FXFontDesc FXFont::getActualFontDesc() const {
  FXFontDesc result;
  fxstrlcpy(result.face,actualName.text(),sizeof(result.face));
  result.size=actualSize;
  result.weight=actualWeight;
  result.slant=actualSlant;
  result.setwidth=actualSetwidth;
  result.encoding=actualEncoding;
  result.flags=flags;
  return result;
  }

/*******************************************************************************/

#if 0
  /// Construct font description
  FXFontDesc(const FXString& fc,FXuint sz,FXuint wt,FXuint sl,FXuint enc,FXuint sw,FXuint h);

// Construct font description
FXFontDesc::FXFontDesc(const FXString& fc,FXuint sz,FXuint wt,FXuint sl,FXuint enc,FXuint sw,FXuint h){
  memcpy(face,fc.text(),sizeof(face)-1);    // FIXME yes, not liking this...
  size=10*sz;
  weight=wt;
  slant=sl;
  setwidth=sw;
  encoding=enc;
  flags=(h&~FXFont::X11);          // System-independent method
  }
#endif


// Set font description from a string
void FXFontDesc::setFont(const FXString& string){
  FXint len=string.find(',');
  clearElms(face,sizeof(face));
  if(0<=len){
    memcpy(face,string.text(),Math::imin(len,sizeof(face)-1));
    size=string.section(',',1).toUInt();
    weight=FXFont::weightFromString(string.section(',',2));
    slant=FXFont::slantFromString(string.section(',',3));
    setwidth=FXFont::setWidthFromString(string.section(',',4));
    encoding=FXFont::encodingFromString(string.section(',',5));
    flags=FXFont::styleFromString(string.section(',',6));
    }
  else{
    fxstrlcpy(face,string.text(),sizeof(face));
    size=0;
    weight=0;
    slant=0;
    setwidth=0;
    encoding=0;
    flags=FXFont::X11;
    }
  }


// Get string of font description
FXString FXFontDesc::getFont() const {
  FXString string=face;
  if(!(flags&FXFont::X11)){
    string.append(',');
    string.append(FXString::value(size));
    if(weight || slant || setwidth || encoding || flags){
      string.append(',');
      string.append(FXFont::stringFromWeight(weight));
      if(slant || setwidth || encoding || flags){
        string.append(',');
        string.append(FXFont::stringFromSlant(slant));
        if(setwidth || encoding || flags){
          string.append(',');
          string.append(FXFont::stringFromSetWidth(setwidth));
          if(encoding || flags){
            string.append(',');
            string.append(FXFont::stringFromEncoding(encoding));
            if(flags){
              string.append(',');
              string.append(FXFont::stringFromStyle(flags));
              }
            }
          }
        }
      }
    }
  return string;
  }


/*******************************************************************************/


// Change font description from a string
void FXFont::setFont(const FXString& string){
  FXint len;

  // Raw X11 font is only the name
  wantedName=string;
  wantedSize=0;
  wantedWeight=0;
  wantedSlant=0;
  wantedSetwidth=0;
  wantedEncoding=0;
  hints=FXFont::X11;

  // Normal font description
  len=string.find(',');
  if(0<=len){

    // Name and foundry
    wantedName.trunc(len);

    // Point size
    wantedSize=string.section(',',1).toUInt();

    // Weight
    wantedWeight=weightFromString(string.section(',',2));

    // Slant
    wantedSlant=slantFromString(string.section(',',3));

    // Set width
    wantedSetwidth=setWidthFromString(string.section(',',4));

    // Encoding
    wantedEncoding=encodingFromString(string.section(',',5));

    // Flags
    hints=styleFromString(string.section(',',6));
    }
  }



// Return the font description as a string; keep it as simple
// as possible by dropping defaulted fields at the end.
FXString FXFont::getFont() const {
  FXString string=wantedName;

  // Raw X11 font is only the name
  if(!(hints&FXFont::X11)){

    // Append size
    string.append(',');
    string.append(FXString::value(wantedSize));

    // Weight and other stuff
    if(wantedWeight || wantedSlant || wantedSetwidth || wantedEncoding || hints){

      // Append weight
      string.append(',');
      string.append(stringFromWeight(wantedWeight));

      // Slant and other stuff
      if(wantedSlant || wantedSetwidth || wantedEncoding || hints){

        // Append slant
        string.append(',');
        string.append(stringFromSlant(wantedSlant));

        // Setwidth and other stuff
        if(wantedSetwidth || wantedEncoding || hints){

          // Append set width
          string.append(',');
          string.append(stringFromSetWidth(wantedSetwidth));

          // Encoding and other stuff
          if(wantedEncoding || hints){

            // Append encoding
            string.append(',');
            string.append(stringFromEncoding(wantedEncoding));

            // Hints
            if(hints){

              // Append hint flags
              string.append(',');
              string.append(stringFromStyle(hints));
              }
            }
          }
        }
      }
    }
  return string;
  }


/*******************************************************************************/


// Save font to stream
void FXFont::save(FXStream& store) const {
  FXId::save(store);
  store << wantedName;
  store << wantedSize;
  store << wantedWeight;
  store << wantedSlant;
  store << wantedSetwidth;
  store << wantedEncoding;
  store << angle;
  store << hints;
  }


// Load font from stream; create() should be called later
void FXFont::load(FXStream& store){
  FXId::load(store);
  store >> wantedName;
  store >> wantedSize;
  store >> wantedWeight;
  store >> wantedSlant;
  store >> wantedSetwidth;
  store >> wantedEncoding;
  store >> angle;
  store >> hints;
  }


// Clean up
FXFont::~FXFont(){
  FXTRACE((TOPIC_CONSTRUCT,"FXFont::~FXFont %p\n",this));
  destroy();
  }

}
