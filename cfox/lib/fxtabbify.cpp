/********************************************************************************
*                                                                               *
*                  T a b - S t o p s   M a n i p u l a t i o n s                *
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
#include "fxmath.h"
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"


/*
  Notes:
  - Expand tabs to spaces, or compress runs of spaces to tabs, given tab stops
    and UTF8 encoding.
  - Assume UTF8 characters account for 1 column.
  - Complex shifting and tabbification of text.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Expand tabs with the equivalent amount of spaces
FXString FXString::detab(const FXchar* str,FXint num,FXint tabcols){
  FXString result;
  FXint is,d,s;
  FXuchar c;
  is=d=s=0;
  while(s<num){
    c=str[s++];
    if(c=='\n'){
      d++; is=0;
      continue;
      }
    if(c=='\t'){
      do{ d++; }while(++is%tabcols);
      continue;
      }
    d++; is++;
    if(c<0xC0) continue;
    d++; s++;
    if(c<0xE0) continue;
    d++; s++;
    if(c<0xF0) continue;
    d++; s++;
    }
  result.length(d);
  is=d=s=0;
  while(s<num){
    c=str[s++];
    if(c=='\n'){
      result[d++]=c; is=0;
      continue;
      }
    if(c=='\t'){
      do{ result[d++]=' '; }while(++is%tabcols);
      continue;
      }
    result[d++]=c; is++;
    if(c<0xC0) continue;
    result[d++]=str[s++];
    if(c<0xE0) continue;
    result[d++]=str[s++];
    if(c<0xF0) continue;
    result[d++]=str[s++];
    }
  FXASSERT(d<=result.length());
  return result;
  }


// Expand tabs with the equivalent amount of spaces
FXString FXString::detab(const FXchar* str,FXint tabcols){
  return FXString::detab(str,strlen(str),tabcols);
  }


// Expand tabs with the equivalent amount of spaces.
FXString FXString::detab(const FXString& str,FXint tabcols){
  return FXString::detab(str.text(),str.length(),tabcols);
  }


// Compress runs of more than 2 spaces with tabs.
FXString FXString::entab(const FXchar* str,FXint num,FXint tabcols){
  FXString result;
  FXint is,ie,d,s,ts;
  FXuchar c;
  is=ie=d=s=0;
  while(s<num){
    c=str[s++]; d++; ie++;
    if(c==' '){                                 // Accumulate spaces
      if((ie-is)<3) continue;                   // Run of less than 3
      ts=is+tabcols-is%tabcols;
      if(ie<ts) continue;                       // Not crossing tabstop
      d+=1+is-ts;                               // Adjust
      is=ts;                                    // Advance to tabstop
      continue;
      }
    if(c=='\t'){                                // Keep the tab
      d+=is-ie;                                 // Adjust
      d++;
      ie--;
      ie+=tabcols-ie%tabcols;                   // New tab-column
      is+=tabcols-is%tabcols;
      if(is==ie) continue;                      // Reached tabstop
      is+=tabcols-is%tabcols;
      d++;
      continue;
      }
    if(c=='\n'){                                // Reset columns
      is=0;
      ie=0;
      continue;
      }
    is=ie;                                      // One UTF8 character
    if(c<0xC0) continue;
    d++; s++;
    if(c<0xE0) continue;
    d++; s++;
    if(c<0xF0) continue;
    d++; s++;
    }
  result.length(d);
  is=ie=d=s=0;
  while(s<num){
    c=result[d++]=str[s++]; ie++;
    if(c==' '){                                 // Accumulate spaces
      if((ie-is)<3) continue;                   // Run of less than 3
      ts=is+tabcols-is%tabcols;
      if(ie<ts) continue;                       // Not crossing tabstop
      result[d+is-ie]='\t';                     // Write a tab at start of run
      d+=1+is-ts;                               // Adjust
      is=ts;                                    // Advance to tabstop
      continue;
      }
    if(c=='\t'){                                // Keep the tab
      d+=is-ie;                                 // Adjust
      result[d++]='\t';
      ie--;
      ie+=tabcols-ie%tabcols;
      is+=tabcols-is%tabcols;
      if(is==ie) continue;                      // Reached tabstop
      is+=tabcols-is%tabcols;
      result[d++]='\t';
      continue;
      }
    if(c=='\n'){                                // Reset columns
      is=0;
      ie=0;
      continue;
      }
    is=ie;                                      // One UTF8 character
    if(c<0xC0) continue;
    result[d++]=str[s++];
    if(c<0xE0) continue;
    result[d++]=str[s++];
    if(c<0xF0) continue;
    result[d++]=str[s++];
    }
  FXASSERT(d<=result.length());
  return result;
  }


// Compress runs of more than 2 spaces with tabs.
FXString FXString::entab(const FXchar* str,FXint tabcols){
  return FXString::entab(str,strlen(str),tabcols);
  }

// Compress runs of more than 2 spaces with tabs.
FXString FXString::entab(const FXString& str,FXint tabcols){
  return FXString::entab(str.text(),str.length(),tabcols);
  }


// Retabbify line
// Assume original starting column of the string is indent, and the output
// starting column is outdent; this affects accounting of the tab-stops in the
// input string, and of the output string relative to the first character.
// Along the way, extra columns may be inserted or removed as per shift.
// If shift=0, indent=0, and outdent=0, this routine has the effect of harmonizing
// the output of white space according to the current tab setting ("clean indent").
// For now, we assume all unicode characters to be one column.
FXString FXString::tabbify(const FXchar* str,FXint num,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs){
  FXString result;
  FXint oec=outdent+shift;
  FXint osc=outdent;
  FXint isc=indent;
  FXint iec=indent;
  FXint s=0;
  FXint d=0;
  FXint ntabs;
  FXuchar c;
  while(s<num){
    c=str[s++];
    if(c==' '){ iec++; continue; }                              // Space is one column
    if(c=='\t'){ iec+=tabcols-iec%tabcols; continue; }          // Tabs is multiple columns
    oec+=(iec-isc);
    if(osc<oec){                                                // Owe some spaces
      if(tabs && 2<(oec-osc)){
        ntabs=oec/tabcols-osc/tabcols;                          // How many tabs to emit
        if(ntabs){ d+=ntabs; osc=(oec/tabcols)*tabcols; }
        }
      d+=oec-osc;
      osc=oec;
      }
    if(c=='\n'){                                                // Emit a newline and reset columns
      d++;
      isc=indent;
      iec=indent;
      osc=outdent;
      oec=outdent+shift;
      continue;
      }
    isc=++iec;                                                  // Advance input columns
    osc=++oec;                                                  // Advance output columns
    d++;                                                        // Copy character
    if(c<0xC0) continue;
    d++;
    s++;
    if(c<0xE0) continue;
    d++;
    s++;
    if(c<0xF0) continue;
    d++;
    s++;
    }
  result.length(d);
  oec=outdent+shift;
  osc=outdent;
  isc=indent;
  iec=indent;
  s=0;
  d=0;
  while(s<num){
    c=str[s++];
    if(c==' '){ iec++; continue; }                              // Space is one column
    if(c=='\t'){ iec+=tabcols-iec%tabcols; continue; }          // Tabs is multiple columns
    oec+=(iec-isc);
    if(osc<oec){                                                // Owe some spaces
      if(tabs && 2<(oec-osc)){
        ntabs=oec/tabcols-osc/tabcols;                          // How many tabs to emit
        if(ntabs){
          do{ result[d++]='\t'; }while(--ntabs);
          osc=(oec/tabcols)*tabcols;                            // Advance starting column to the last tabstop
          }
        }
      while(osc<oec){ result[d++]=' '; osc++; }                 // Emit spaces to reach current column
      }
    if(c=='\n'){                                                // Emit a newline and reset columns
      result[d++]='\n';
      isc=indent;
      iec=indent;
      osc=outdent;
      oec=outdent+shift;
      continue;
      }
    isc=++iec;                                                  // Advance input columns
    osc=++oec;                                                  // Advance output columns
    result[d++]=c;                                              // Copy character
    if(c<0xC0) continue;
    result[d++]=str[s++];
    if(c<0xE0) continue;
    result[d++]=str[s++];
    if(c<0xF0) continue;
    result[d++]=str[s++];
    }
  FXASSERT(d<=result.length());
  result.trunc(d);
  return result;
  }


// Retabbify lines
FXString FXString::tabbify(const FXchar* str,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs){
  return FXString::tabbify(str,strlen(str),tabcols,indent,outdent,shift,tabs);
  }


// Retabbify lines
FXString FXString::tabbify(const FXString& str,FXint tabcols,FXint indent,FXint outdent,FXint shift,FXbool tabs){
  return FXString::tabbify(str.text(),str.length(),tabcols,indent,outdent,shift,tabs);
  }

}
