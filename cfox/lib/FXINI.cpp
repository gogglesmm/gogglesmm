/********************************************************************************
*                                                                               *
*                      I N I   R e a d e r  &  W r i t e r                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2022 by Jeroen van der Zijp.   All Rights Reserved.             *
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
#include "FXElement.h"
#include "FXArray.h"
#include "FXString.h"
#include "FXIO.h"
#include "FXIODevice.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXParseBuffer.h"
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXINI.h"

/*
  Notes:

  - Load and save FXVariant object from/to INI format files.

  - FXVariant is a more general, hierarchical key-value store, and able to
    store anything that can be stored in FXSettings.  However, ability to
    read/write to .INI files is required to allow existing software config
    to remain readable/writable.

  - Formal:

         sections    : section
                     | section sections
                     ;

         section     : sectiongroup keyvalues
                     ;

         sectiongroup: '[' key ']' newline
                     ;

         keyvalues   : keyvalue
                     | keyvalue keyvalues
                     ;

         keyvalue    : key '=' value newline
                     | key '=' newline
                     ;

         key         : characters
                     ;

         value       : characters
                     | string
                     ;

         string      : '"' '"'
                     | '"' dq-chars '"'
                     ;

         comments    : comment
                     | comment comments
                     | newline
                     :

         comment     : ';' characters newline
                     | '#' characters newline
                     ;

         dq-chars    : dq-char
                     | dq-char dq-chars
                     ;

         dq-char     : any-character-except-double-quotes
                     | '\\"' | '\\\\' | '\\/' | '\\b' | '\\f' | '\\n' | '\\r' | '\\t'
                     | '\\x' hxdigit hxdigit
                     | '\\u' hxdigit hxdigit hxdigit hxdigit
                     | '\\' octaldigits
                     | '\\' newline
                     ;

         hexdigits   : hexdigit
                     | hexdigit hexdigits
                     ;

         hexdigit    : '0'...'9' | 'a'...'f' | 'A'...'F'
                     ;

         newline     : '\n'
                     | '\r\n'
                     | '\r'
                     ;
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_DETAIL    1001
#define TOPIC_DEBUG     1002

using namespace FX;

/*******************************************************************************/

namespace FX {


// Maximum token size
enum {
  MAXTOKEN = 256
  };


// Tokens
enum {
  TK_ERROR   = 0,       // Syntax error
  TK_EOF     = 1,       // End of file
  TK_SECTION = 2,       // Section
  TK_KEY     = 3,       // Key
  TK_VALUE   = 4        // String
  };


// States
enum {
  ST_DEF = 0,
  ST_CMT = 1,
  ST_CHR = 2,
  ST_EQU = 4,
  ST_SEC = 8,
  ST_VAL = 16,
  ST_STR = 32
  };

// Error messages
const FXchar *const FXINI::errors[]={
  "OK",
  "Unable to save",
  "Unable to load",
  "Illegal token",
  "Expected a ':'",
  "Expected a ','",
  "Unmatched ']'",
  "Unmatched '}'",
  "Unmatched '\"'",
  "Unmatched '\''",
  "Bad number",
  "Unexpected end of file"
  };


// Construct INIParser serializer
FXINI::FXINI():offset(0),token(TK_EOF),column(0),line(1),state(ST_DEF),prec(15),fmt(2),esc(0){
  FXTRACE((TOPIC_CONSTRUCT,"FXINI::FXINI\n"));
  }


// Construct and open for loading
FXINI::FXINI(FXchar* buffer,FXuval sz,Direction d):offset(0),token(TK_EOF),column(0),line(1),state(ST_DEF),prec(16),fmt(2),esc(0){
  FXTRACE((TOPIC_CONSTRUCT,"FXINI::FXINI(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(buffer,sz,d);
  }


// Open INIParser stream for given direction and set its buffer
FXbool FXINI::open(FXchar* buffer,FXuval sz,Direction d){
  FXTRACE((TOPIC_DETAIL,"FXINI::open(%p,%lu,%s)\n",buffer,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  if(FXParseBuffer::open(buffer,sz,d)){
    token=TK_ERROR;
    state=ST_DEF;
    offset=0;
    column=0;
    line=1;
    return true;
    }
  return false;
  }

/*******************************************************************************/

// Get next token
FXint FXINI::next(){
  FXuchar c,s;

  rptr=sptr;

  // While more data
  while(need(MAXTOKEN)){

    FXASSERT(sptr<wptr);

    // Prior state
    s=state;

    // Process characters
    switch((c=sptr[0])){
      case ' ':                                 // Space
        column++;
        //FALL//
      case '\v':                                // Vertical tab
      case '\f':                                // Form feed
        offset++;
        sptr++;
        continue;
      case '\t':                                // Tab hops to next tabstop
        column+=(8-column%8);
        offset++;
        sptr++;
        continue;
      case '\r':                                // Carriage return
        if(sptr[1]=='\n'){
          offset++;
          sptr++;
          }
        //FALL//
      case '\n':                                // Newline
        state=ST_DEF;
        if(!(s&ST_CMT)){
          if(s&ST_VAL) return TK_VALUE;
          if(s&ST_CHR) return TK_ERROR;         // '=' expected
          }
        line++;
        column=0;
        offset++;
        sptr++;
        rptr=sptr;
        continue;
      case '#':                                 // Start comment
      case ';':
        if(!(s&(ST_CMT|ST_SEC|ST_VAL|ST_STR))){
          state=ST_CMT;                         // Comment state
          rptr=sptr;
          }
        column++;
        offset++;
        sptr++;
        continue;
      case '[':                                 // Begin of group
        if(!(s&(ST_CMT|ST_VAL|ST_STR))){
          if(s&ST_SEC) return TK_ERROR;         // '[' unexpected
          state=ST_SEC;
          rptr=sptr;
          }
        column++;
        offset++;
        sptr++;
        continue;
      case ']':                                 // End of group
        if(!(s&(ST_CMT|ST_VAL|ST_STR))){
          if(!(s&ST_SEC)) return TK_ERROR;      // ']' unexpected
          state=ST_DEF;
          column++;
          offset++;
          sptr++;
          return TK_SECTION;
          }
        column++;
        offset++;
        sptr++;
        continue;
      case '=':                                 // Equals sign
        if(!(s&(ST_CMT|ST_VAL|ST_STR))){
          if(!(s&ST_CHR)) return TK_ERROR;      // Unexpected '='
          state|=ST_EQU;
          if(!(s&ST_EQU)) return TK_KEY;
          state=ST_VAL;
          rptr=sptr+1;
          }
        column++;
        offset++;
        sptr++;
        continue;
      case '"':                                 // Begin or end of string
        if(!(s&(ST_CMT|ST_SEC))){
          if(s&ST_VAL){
            if(s&ST_CHR) return TK_ERROR;       // Unexpected '"'
            state=ST_STR;
            rptr=sptr;
            }
          if(s&ST_STR){
            state=ST_DEF;
            column++;
            offset++;
            sptr++;
            return TK_VALUE;
            }
          }
        column++;
        offset++;
        sptr++;
        continue;
      case '\\':                                // Escaped characters
        state|=ST_CHR;
        column+=2;
        offset+=2;
        sptr+=2;
        continue;
      case '!':                                 // All these may be part of a key
      case '$':
      case '%':
      case '&':
      case '(':
      case ')':
      case '*':
      case '+':
      case ',':
      case '-':
      case '.':
      case '/':
      case ':':
      case '<':
      case '>':
      case '?':
      case '\'':
      case '^':
      case '`':
      case '{':
      case '|':
      case '}':
      case '~':
      case '_':
      case '@':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
        state|=ST_CHR;
        column++;
        offset++;
        sptr++;
        continue;
      case 0xC0:                                // 2-byte UTF8 sequences
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC4:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC8:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCC:
      case 0xCD:
      case 0xCE:
      case 0xCF:
      case 0xD0:
      case 0xD1:
      case 0xD2:
      case 0xD3:
      case 0xD4:
      case 0xD5:
      case 0xD6:
      case 0xD7:
      case 0xD8:
      case 0xD9:
      case 0xDA:
      case 0xDB:
      case 0xDC:
      case 0xDD:
      case 0xDE:
      case 0xDF:
        if((sptr[1]&192)!=128) return TK_ERROR; // Bad followers
        state|=ST_CHR;
        column++;
        offset+=2;
        sptr+=2;
        continue;
      case 0xE0:                                // 3-byte UTF8 sequences
      case 0xE1:
      case 0xE2:
      case 0xE3:
      case 0xE4:
      case 0xE5:
      case 0xE6:
      case 0xE7:
      case 0xE8:
      case 0xE9:
      case 0xEA:
      case 0xEB:
      case 0xEC:
      case 0xED:
      case 0xEE:
      case 0xEF:
        if((sptr[1]&192)!=128) return TK_ERROR; // Bad followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if(sptr[0]=='\xEF' && sptr[1]=='\xBB' && sptr[2]=='\xBF'){
          if(s) return TK_ERROR;                // BOM unexpected
          offset+=3;
          sptr+=3;                              // Byte order mark
          continue;
          }
        state|=ST_CHR;
        column++;
        offset+=3;
        sptr+=3;
        continue;
      case 0xF0:                                // 4-byte UTF8 sequences
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
        if((sptr[1]&192)!=128) return TK_ERROR; // Bad followers
        if((sptr[2]&192)!=128) return TK_ERROR;
        if((sptr[3]&192)!=128) return TK_ERROR;
        state|=ST_CHR;
        column++;
        offset+=4;
        sptr+=4;
        continue;
      default:                                  // Disallowed characters
        return TK_ERROR;                        // Bad token
      }
    }
  return TK_EOF;
  }


// Load variant
FXINI::Error FXINI::loadVariant(FXVariant& var){
  FXString sectionkey,entrykey,entryvalue;
  while(token!=TK_EOF){

    // Expect section first
    if(token!=TK_SECTION) return ErrToken;

    // Get section key
    sectionkey=FXString::unescape(rptr,sptr-rptr,'[',']');

    // Empty section key
    if(sectionkey.empty()) return ErrToken;

    FXTRACE((TOPIC_DEBUG,"sectionkey=[%s]\n",sectionkey.text()));

    // Next token
    token=next();

    // Load entries under section
    while(token==TK_KEY){

      // Get entry key
      entrykey.assign(rptr,sptr-rptr);

      // Empty entry key
      if(entrykey.empty()) return ErrToken;

      FXTRACE((TOPIC_DEBUG,"entrykey=\"%s\"\n",entrykey.text()));

      // Next token
      token=next();

      // Expect value
      if(token!=TK_VALUE) return ErrToken;

      // Get entry value
      entryvalue=FXString::unescape(rptr,sptr-rptr,'"','"');

      FXTRACE((TOPIC_DEBUG,"entryvalue=\"%s\"\n",entryvalue.text()));

      // Assign to variant
      var[sectionkey][entrykey]=entryvalue;

      // Next token
      token=next();
      }
    }
  return ErrOK;
  }


// Load a variant
FXINI::Error FXINI::load(FXVariant& variant){
  FXTRACE((TOPIC_DETAIL,"FXINI::load(variant)\n"));
  Error err=ErrLoad;
  if(dir==Load){
    token=next();
    err=loadVariant(variant);
    }
  return err;
  }

/*******************************************************************************/

// Save section key; escape if needed
FXINI::Error FXINI::saveSection(const FXString& str){
  FXString string=FXString::escape(str,'[',']',esc);
  if(!emit(string.text(),string.length())) return ErrSave;
  column+=string.count();
  offset+=string.length();
  return ErrOK;
  }


// Save string value, escape if needed
FXINI::Error FXINI::saveString(const FXString& str){
  if(FXString::shouldEscape(str,'"','"',esc)){
    FXString string=FXString::escape(str,'"','"',esc);
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.count();
    offset+=string.length();
    return ErrOK;
    }
  if(!emit(str.text(),str.length())) return ErrSave;
  column+=str.count();
  offset+=str.length();
  return ErrOK;
  }


// Save entry
FXINI::Error FXINI::saveEntry(const FXVariant& var){
  FXString string;
  switch(var.getType()){
  case FXVariant::NullType:
    return ErrOK;
  case FXVariant::BoolType:
    if(var.asULong()){
      if(!emit("true",4)) return ErrSave;
      column+=4;
      offset+=4;
      }
    else{
      if(!emit("false",5)) return ErrSave;
      column+=5;
      offset+=5;
      }
    return ErrOK;
  case FXVariant::CharType:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    return ErrOK;
  case FXVariant::IntType:
  case FXVariant::LongType:
    string.fromLong(var.asLong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    return ErrOK;
  case FXVariant::UIntType:
  case FXVariant::ULongType:
    string.fromULong(var.asULong());
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    return ErrOK;
  case FXVariant::FloatType:
  case FXVariant::DoubleType:
    string.fromDouble(var.asDouble(),prec,fmt);
    if(!emit(string.text(),string.length())) return ErrSave;
    column+=string.length();
    offset+=string.length();
    return ErrOK;
  case FXVariant::StringType:
    if(saveString(var.asString())!=ErrOK) return ErrSave;
    return ErrOK;
  default:
    return ErrSave;
    }
  return ErrSave;
  }


// Save top-level section
FXINI::Error FXINI::saveVariant(const FXVariant& section){
  if(section.getType()==FXVariant::MapType){

    // Loop over sections
    for(FXival sec=0; sec<section.asMap().no(); ++sec){

      // Get reference to section key
      const FXString& sectionkey=section.asMap().key(sec);

      // Skip empty slots
      if(sectionkey.empty()) continue;

      // Get reference to section data
      const FXVariant& sectiondata=section.asMap().data(sec);

      // Each section must be a map as well
      if(sectiondata.getType()==FXVariant::MapType){
        FXbool sectionsaved=false;

        // Loop over entries
        for(FXival ent=0; ent<sectiondata.asMap().no(); ++ent){

          // Get reference to entry key
          const FXString& entrykey=sectiondata.asMap().key(ent);

          // Skip empty slots
          if(entrykey.empty()) continue;

          // Get reference to entry data
          const FXVariant& entrydata=sectiondata.asMap().data(ent);

          // Write section name if we haven't already
          if(!sectionsaved){

            // Save section
            if(saveSection(sectionkey)!=ErrOK) return ErrSave;

            // New line for the entry
            if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
            offset+=strlen(ENDLINE);
            column=0;
            line++;

            // Section key saved now
            sectionsaved=true;
            }

          // Save the key
          if(!emit(entrykey.text(),entrykey.length())) return ErrSave;
          offset+=entrykey.length();
          column+=entrykey.count();

          // Save '='
          if(!emit('=',1)) return ErrSave;
          offset+=1;
          column+=1;

          // Save entry
          if(saveEntry(entrydata)!=ErrOK) return ErrSave;

          // End the line
          if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
          offset+=strlen(ENDLINE);
          column=0;
          line++;
          }

        // Extra blank line if non-empty section
        if(sectionsaved){
          if(!emit(ENDLINE,strlen(ENDLINE))) return ErrSave;
          offset+=strlen(ENDLINE);
          column=0;
          line++;
          }
        }
      }
    return ErrOK;
    }
  return ErrSave;
  }


// Save a variant
FXINI::Error FXINI::save(const FXVariant& variant){
  FXTRACE((TOPIC_DETAIL,"FXINI::save(variant)\n"));
  Error err=ErrSave;
  if(dir==Save){
    err=saveVariant(variant);
    if(flush(0)<0) err=ErrSave;
    }
  return err;
  }

/*******************************************************************************/

// Close json parse buffer
FXbool FXINI::close(){
  FXTRACE((TOPIC_DETAIL,"FXINI::close()\n"));
  if(FXParseBuffer::close()){
    return true;
    }
  return false;
  }


// Close stream and clean up
FXINI::~FXINI(){
  FXTRACE((TOPIC_CONSTRUCT,"FXINI::~FXINI\n"));
  close();
  }

}
