/********************************************************************************
*                                                                               *
*                      J S O N   R e a d e r  &  W r i t e r                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2013,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXException.h"
#include "FXVariant.h"
#include "FXVariantArray.h"
#include "FXVariantMap.h"
#include "FXJSON.h"

/*
  Notes:

  - Load and save FXVariant object from/to JSON format files.

  - We read over C and C++-style comment; yes, this is not in the spec;
    but it was, once.  Plus, its convenient.

  - Closing the file does not reset comment and line number (in case of error, this
    point to the problem).

  - JSON syntax is very simple:

         value     : object | array | string | number | 'true' | 'false' | 'null' ;

         object    : '{'   '}' | '{' members '}' ;

         members   : pair | pair ',' members ;

         pair      : string ':' value ;

         array     : '[' ']' | '[' elements ']' ;

         elements  : value | value ',' elements ;

         string    : '"' '"' | '"' chars '"' ;

         chars     : char | char chars ;

         char      : unicode_except_esc_or_quotes
                   | '\"' | '\\' | '\/' | '\b' | '\f' | '\n' | '\r' | '\t'
                   | '\' octdigit octdigit octdigit
                   | '\x' hxdigit hxdigit
                   | '\u' hxdigit hxdigit hxdigit hxdigit
                   ;

         number    : int
                   | int frac
                   | int exp
                   | int frac exp
                   ;

         int       : digit | nzdigit digits | "-" digit | "-" nzdigit digits ;

         digits    : digit | digit digits ;

         frac      : "." digits ;

         exp       : e digits ;

         e         : "e" | "e+" | "e-" | "E" | "E-" | "E+" ;

  - Flow controls the looks of the output.  The current values supported are:

      Stream  No formatting whatsoever.  This is the most compact format but is essentially
              not human-readable.

      Compact Try to cram as much as possible on a line, but break lines beyond
              a certain size.

      Pretty  Nicely indented and pretty printed, but fluffy, output.

  - Need to differentiate between opening a non-owned populated buffer
    for reading or an owned empty buffer; in case one wants to read JSON
    from a string, lets say.

  - Need to split off parse buffer into its own class; we now have several parsers,
    all using similar logic.  New parse buffer should have better way of managing
    exact state at which fill() or flush() would be called.  (Possibly, end-of-file
    signaled in-band?).
*/


#define MINBUFFER 1024          // Minimum buffer size
#define MAXTOKEN  256           // Maximum token size

using namespace FX;

namespace FX {

enum {
  TK_ERROR   = 0,       // Syntax error
  TK_EOF     = 1,       // End of file
  TK_COMMA   = 2,       // Element separator
  TK_COLON   = 3,       // Key:value pair separator
  TK_NULL    = 4,       // NULL value
  TK_FALSE   = 5,       // False value
  TK_TRUE    = 6,       // True value
  TK_INT     = 7,       // Integer value
  TK_REAL    = 8,       // Real value
  TK_QUOTES  = 9,       // Quotes
  TK_LBRACK  = 10,      // Start of array
  TK_LBRACE  = 11,      // Start of map
  TK_RBRACK  = 12,      // End of array
  TK_RBRACE  = 13       // End of map
  };


/*******************************************************************************/

// Error messages
const FXchar *const FXJSON::errors[]={
  "OK",
  "Unable to save",
  "Unable to load",
  "Illegal token",
  "Expected a ':'",
  "Expected a ','",
  "Unmatched ']'",
  "Unmatched '}'",
  "Unmatched '\"'",
  "Bad number",
  "Unexpected end of file"
  };


// Construct JSON serializer
FXJSON::FXJSON():begptr(NULL),endptr(NULL),sptr(NULL),rptr(NULL),wptr(NULL),token(TK_EOF),column(0),indent(0),line(1),wrap(80),dir(Stop),flow(Compact),prec(15),fmt(2),dent(2),owns(false){
  FXTRACE((1,"FXJSON::FXJSON\n"));
  }


// Construct and open for loading
FXJSON::FXJSON(FXchar* data,FXuval sz,Direction d):begptr(NULL),endptr(NULL),sptr(NULL),rptr(NULL),wptr(NULL),token(TK_EOF),column(0),indent(0),line(1),wrap(80),dir(Stop),flow(Compact),prec(16),fmt(2),dent(2),owns(false){
  FXTRACE((1,"FXJSON::FXJSON(%p,%lu,%s)\n",data,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  open(data,sz,d);
  }


// Open FXJSON stream for given direction and set its buffer
FXbool FXJSON::open(FXchar* data,FXuval sz,Direction d){
  FXTRACE((2,"FXJSON::open(%p,%lu,%s)\n",data,sz,(d==Save)?"Save":(d==Load)?"Load":"Stop"));
  if((dir==Stop) && (d!=Stop)){
    if(data){                   // External buffer
      begptr=data;
      endptr=begptr+sz;
      if(d==Save){              // Saving
        sptr=begptr;
        rptr=begptr;
        wptr=begptr;
        }
      else{                     // Loading
        sptr=begptr;
        rptr=begptr;
        wptr=endptr;            // Assume passed in buffer with existing json data
        }
      owns=false;
      }
    else{                       // Internal buffer
      if(sz<MINBUFFER) sz=MINBUFFER;
      if(!allocElms(begptr,sz)) return false;
      endptr=begptr+sz;
      if(d==Save){              // Saving
        sptr=begptr;
        rptr=begptr;
        wptr=begptr;
        }
      else{                     // Loading
        sptr=endptr;
        rptr=endptr;
        wptr=endptr;
        }
      owns=true;
      }
    token=TK_ERROR;
    column=0;
    indent=0;
    line=1;
    dir=d;
    return true;
    }
  return false;
  }


/*******************************************************************************/

// Fill buffer from file
FXbool FXJSON::fill(){
  return rptr<wptr;     // Bytes left to read
  }


// Flush buffer to file
FXbool FXJSON::flush(){
  return wptr<endptr;   // Space left to write
  }


// Ensure we have a requisite number of bytes in the buffer, calling fill()
// to load additional data into the buffer if needed.
// Near the end of the file, there may be fewer than n bytes in the buffer
// even after fill() is called.
FXbool FXJSON::need(FXival n){
  if(wptr<rptr+n){
    if(wptr==endptr){
      fill();
      }
    return rptr<wptr;
    }
  return true;
  }


// Get next token
FXint FXJSON::next(){
  FXint comment=0;
  FXint tok;

  // While more data
  while(need(MAXTOKEN)){

    // Start new token
    sptr=rptr;

    FXASSERT(rptr<wptr);

    // Process characters
    switch(rptr[0]){
      case '\t':                // Tab hops to next tabstop
        column+=(8-column%8);
        rptr++;
        continue;
      case '\n':                // Newline increases line number, resets column
        column=0;
        line++;
        rptr++;
        if(comment<0) comment=0;
        continue;
      case ' ':                 // Space
        column++;
      case '\v':                // Other white space not incrementing column
      case '\f':
      case '\r':
        rptr++;
        continue;
      case '\xef':              // BOM (byte order mark) should behave as spaces
        if(rptr+2<wptr && rptr[1]=='\xbb' && rptr[2]=='\xbf'){ rptr+=3; continue; }
        if(comment){ column++; rptr=wcinc(rptr); continue; }
        return TK_ERROR;
      case '/':                 // Possible start of comment
        column++;
        rptr++;
        if(rptr<wptr && rptr[0]=='*' && comment>=0){ comment+=1; column++; rptr++; continue; }
        if(rptr<wptr && rptr[0]=='/' && comment==0){ comment-=1; column++; rptr++; continue; }
        if(comment) continue;
        return TK_ERROR;
      case '*':                 // Possible end of comment
        column++;
        rptr++;
        if(rptr<wptr && rptr[0]=='/' && comment>=1){ comment-=1; column++; rptr++; continue; }
        if(comment) continue;
        return TK_ERROR;
      case '{':                 // Begin of map
        column++;
        rptr++;
        if(comment) continue;
        return TK_LBRACE;
      case '}':                 // End of map
        column++;
        rptr++;
        if(comment) continue;
        return TK_RBRACE;
      case '[':                 // Begin of array
        column++;
        rptr++;
        if(comment) continue;
        return TK_LBRACK;
      case ']':                 // End of array
        column++;
        rptr++;
        if(comment) continue;
        return TK_RBRACK;
      case ',':                 // Element separator
        column++;
        rptr++;
        if(comment) continue;
        return TK_COMMA;
      case ':':                 // Key:value separator
        column++;
        rptr++;
        if(comment) continue;
        return TK_COLON;
      case '"':                 // String delimiters
        column++;
        rptr++;
        if(comment) continue;
        return TK_QUOTES;
      case '+':                 // Number value
      case '-':
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
        if(comment){ column++; rptr++; continue; }
        tok=TK_INT;
        if(rptr[0]=='-' || rptr[0]=='+'){
          column++;
          rptr++;
          }
        while(rptr<wptr && Ascii::isDigit(rptr[0])){
          column++;
          rptr++;
          }
        if(rptr<wptr && rptr[0]=='.'){
          tok=TK_REAL;
          column++;
          rptr++;
          while(rptr<wptr && Ascii::isDigit(rptr[0])){
            column++;
            rptr++;
            }
          }
        if(rptr<wptr && (rptr[0]=='e' || rptr[0]=='E')){
          tok=TK_REAL;
          column++;
          rptr++;
          if(rptr<wptr && (rptr[0]=='-' || rptr[0]=='+')){
            column++;
            rptr++;
            }
          while(rptr<wptr && Ascii::isDigit(rptr[0])){
            column++;
            rptr++;
            }
          }
        return tok;
      case 'n':                 // Null value
        column++;
        rptr++;
        if(comment) continue;
        if(rptr+2<wptr && rptr[0]=='u' && rptr[1]=='l' && rptr[2]=='l'){ column+=3; rptr+=3; return TK_NULL; }
        return TK_ERROR;
      case 't':                 // True value
        column++;
        rptr++;
        if(comment) continue;
        if(rptr+2<wptr && rptr[0]=='r' && rptr[1]=='u' && rptr[2]=='e'){ column+=3; rptr+=3; return TK_TRUE; }
        return TK_ERROR;
      case 'f':                 // False value
        column++;
        rptr++;
        if(comment) continue;
        if(rptr+3<wptr && rptr[0]=='a' && rptr[1]=='l' && rptr[2]=='s' && rptr[3]=='e'){ column+=4; rptr+=4; return TK_FALSE; }
        return TK_ERROR;
      default:                  // Other characters
        column++;
        rptr=wcinc(rptr);       // Next wide character
        if(comment) continue;
        return TK_ERROR;
      }
    }
  return TK_EOF;
  }


// Load characters into string
FXJSON::Error FXJSON::loadString(FXString& str){
  FXString string;
  while(need(MAXTOKEN) && rptr[0]!='"'){
    if((wptr==endptr) && (wptr<=rptr+MAXTOKEN)){
      string.append(sptr,rptr-sptr);
      sptr=rptr;
      }
    if(rptr[0]=='\\' && rptr+1<wptr){
      column++;
      rptr++;
      }
    column++;
    rptr=wcinc(rptr);                   // Next wide character
    }
  if(rptr>=wptr) return ErrEnd;         // Found closing quotes, not end of file
  string.append(sptr,rptr-sptr);
  sptr=rptr;
  str=unescape(string,'"','"');
  return ErrOK;
  }


// Load map elements int o var
FXJSON::Error FXJSON::loadMap(FXVariant& var){
  FXString key;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::VMap);

  // While more keys
  while(token==TK_QUOTES){

    // Load string
    if((err=loadString(key))!=ErrOK) return err;

    // Eat the quotes
    token=next();

    // Expect closing quotes
    if(token!=TK_QUOTES) return ErrQuotes;

    // Eat the quotes
    token=next();

    // Expect colon
    if(token!=TK_COLON) return ErrColon;

    // Eat the colon
    token=next();

    // Load item directly into associated slot
    if((err=loadVariant(var[key]))!=ErrOK) return err;

    // Expect another key-value pair
    if(token!=TK_COMMA) break;

    // Eat the comma
    token=next();
    }
  return ErrOK;
  }


// Load array elements into var
FXJSON::Error FXJSON::loadArray(FXVariant& var){
  FXival index=0;
  Error err;

  // Make it into an array now
  var.setType(FXVariant::VArray);

  // While possible item start token
  while(TK_NULL<=token && token<=TK_LBRACE){

    // Load item directly into array slot
    if((err=loadVariant(var[index]))!=ErrOK) return err;

    // Expect another value
    if(token!=TK_COMMA) break;

    // Next token
    token=next();

    // Next array index
    index++;
    }
  return ErrOK;
  }


// Load variant
FXJSON::Error FXJSON::loadVariant(FXVariant& var){
  FXString value(sptr,rptr-sptr);
  FXbool ok=false;
  Error err;
  switch(token){
  case TK_EOF:                                  // Unexpected end of file
    var=FXVariant::null;
    return ErrEnd;
  case TK_NULL:                                 // Null
    var=FXVariant::null;
    token=next();
    return ErrOK;
  case TK_FALSE:                                // False
    var=false;
    token=next();
    return ErrOK;
  case TK_TRUE:                                 // True
    var=true;
    token=next();
    return ErrOK;
  case TK_INT:
    var=value.toLong(0,&ok);
    if(!ok) return ErrNumber;                   // Numeric conversion error
    token=next();
    return ErrOK;
  case TK_REAL:
    var=value.toDouble(&ok);
    if(!ok) return ErrNumber;                   // Numeric conversion error
    token=next();
    return ErrOK;
  case TK_LBRACK:                               // Array
    token=next();
    if((err=loadArray(var))!=ErrOK) return err;
    if(token!=TK_RBRACK) return ErrBracket;     // Expected closing bracket
    token=next();
    return ErrOK;
  case TK_LBRACE:                               // Map
    token=next();
    if((err=loadMap(var))!=ErrOK) return err;
    if(token!=TK_RBRACE) return ErrBrace;       // Expected closing brace
    token=next();
    return ErrOK;
  case TK_QUOTES:                               // String
    if((err=loadString(value))!=ErrOK) return err;
    token=next();
    if(token!=TK_QUOTES) return ErrQuotes;
    token=next();
    var=value;
    return ErrOK;
  default:                                      // Illegal token
    var=FXVariant::null;
    return ErrToken;
    }
  return ErrToken;
  }

/*******************************************************************************/

// Emit text to buffer
FXJSON::Error FXJSON::emit(const FXchar* str,FXint count){
  FXival num;
  while(0<count){
    if(wptr>=endptr && !flush()){ FXTRACE((2,"%s:%d: flush() failed!\n",__FILE__,__LINE__)); return ErrSave; }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    memcpy(wptr,str,num);
    wptr+=num;
    str+=num;
    count-=num;
    }
  return ErrOK;
  }


// Emit characters to buffer
FXJSON::Error FXJSON::emit(FXchar ch,FXint count){
  FXival num;
  while(0<count){
    if(wptr>=endptr && !flush()){ FXTRACE((2,"%s:%d: flush() failed!\n",__FILE__,__LINE__)); return ErrSave; }
    FXASSERT(wptr<endptr);
    num=FXMIN(count,endptr-wptr);
    memset(wptr,ch,num);
    wptr+=num;
    count-=num;
    }
  return ErrOK;
  }


// Save string after escaping it
FXJSON::Error FXJSON::saveString(const FXString& str){
  FXString string=escape(str,'"','"',2);
  if(emit(string.text(),string.length())==ErrOK){
    column+=string.length();
    return ErrOK;
    }
  return ErrSave;
  }


// Save map elements from var
FXJSON::Error FXJSON::saveMap(const FXVariant& var){
  FXival count=var.asMap().used();

  FXASSERT(var.getType()==FXVariant::VMap);

  // Object start
  if(emit("{",1)!=ErrOK) return ErrSave;
  column+=1;

  // Skip the whole thing if no items
  if(count){
    FXint oldindent=indent;

    // Figure indent
    indent=(flow==Pretty)?indent+dent:(flow==Compact)?column:0;

    // Write indent
    if(flow==Pretty){
      if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
      if(emit(' ',indent)!=ErrOK) return ErrSave;
      column=indent;
      line++;
      }

    // Loop over the items
    for(FXival i=0; i<var.asMap().no(); ++i){

      // Skip empty slots
      if(var.asMap().key(i).empty()) continue;

      // Save string
      if(saveString(var.asMap().key(i))!=ErrOK) return ErrSave;

      // Write separator
      if(flow==Stream){
        if(emit(":",1)!=ErrOK) return ErrSave;
        column+=1;
        }
      else{
        if(emit(" : ",3)!=ErrOK) return ErrSave;
        column+=3;
        }

      // Write variant
      if(saveVariant(var.asMap().data(i))!=ErrOK) return ErrSave;

      // Another item to follow?
      if(--count>0){

        // Write comma
        if(emit(",",1)!=ErrOK) return ErrSave;
        column+=1;

        // Write newline and indent
        if(flow || wrap<column){
          if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
          if(emit(' ',indent)!=ErrOK) return ErrSave;
          column=indent;
          line++;
          }
        }
      }

    indent=oldindent;

    // Write indent
    if(flow==Pretty){
      if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
      if(emit(' ',indent)!=ErrOK) return ErrSave;
      column=indent;
      line++;
      }
    }

  // Object end
  if(emit("}",1)!=ErrOK) return ErrSave;
  column+=1;

  return ErrOK;
  }


// Save array elements from var
FXJSON::Error FXJSON::saveArray(const FXVariant& var){

  FXASSERT(var.getType()==FXVariant::VArray);

  // Array start
  if(emit("[",1)!=ErrOK) return ErrSave;
  column+=1;

  // Skip the whole thing if no items
  if(var.asArray().no()){
    FXint oldindent=indent;

    // Figure indent
    indent=(flow==Pretty)?indent+dent:(flow==Compact)?column:0;

    // Write indent
    if(flow==Pretty){
      if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
      if(emit(' ',indent)!=ErrOK) return ErrSave;
      column=indent;
      line++;
      }

    // Loop over the items
    for(FXival i=0; i<var.asArray().no(); ++i){

      // Write variant
      if(saveVariant(var.asArray().at(i))!=ErrOK) return ErrSave;

      // Another item to follow?
      if(i+1<var.asArray().no()){

        // Write comma
        if(emit(",",1)!=ErrOK) return ErrSave;
        column+=1;

        // Write space or newline and indent
        if(flow==Pretty || wrap<column || (flow==Compact && FXVariant::VMap==var.asArray().at(i).getType())){
          if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
          if(emit(' ',indent)!=ErrOK) return ErrSave;
          column=indent;
          line++;
          }
        else if(flow){
          if(emit(" ",1)!=ErrOK) return ErrSave;
          column+=1;
          }
        }
      }

    indent=oldindent;

    // Write indent
    if(flow==Pretty){
      if(emit(ENDLINE,strlen(ENDLINE))!=ErrOK) return ErrSave;
      if(emit(' ',indent)!=ErrOK) return ErrSave;
      column=indent;
      line++;
      }
    }

  // Array end
  if(emit("]",1)!=ErrOK) return ErrSave;
  column+=1;
  return ErrOK;
  }


// Recursively save variant var
FXJSON::Error FXJSON::saveVariant(const FXVariant& var){
  const FXchar truth[2][6]={"false","true"};
  FXString string;
  FXbool flag;
  switch(var.getType()){
  case FXVariant::VNull:
    if(emit("null",4)!=ErrOK) return ErrSave;
    column+=4;
    break;
  case FXVariant::VBool:
    flag=(FXbool)var.asULong();
    emit(truth[flag],strlen(truth[flag]));
    column+=strlen(truth[flag]);
    break;
  case FXVariant::VChar:
    string.fromULong(var.asULong());
    if(emit(string.text(),string.length())!=ErrOK) return ErrSave;
    column+=string.length();
    break;
  case FXVariant::VInt:
  case FXVariant::VLong:
    string.fromLong(var.asLong());
    if(emit(string.text(),string.length())!=ErrOK) return ErrSave;
    column+=string.length();
    break;
  case FXVariant::VUInt:
  case FXVariant::VULong:
    string.fromULong(var.asULong());
    if(emit(string.text(),string.length())!=ErrOK) return ErrSave;
    column+=string.length();
    break;
  case FXVariant::VFloat:
  case FXVariant::VDouble:
    string.fromDouble(var.asDouble(),prec,fmt);
    if(emit(string.text(),string.length())!=ErrOK) return ErrSave;
    column+=string.length();
    break;
  case FXVariant::VPointer:
    string.format("%p",var.asPtr());
    if(emit(string.text(),string.length())!=ErrOK) return ErrSave;
    column+=string.length();
    break;
  case FXVariant::VString:
    if(saveString(var.asString())!=ErrOK) return ErrSave;
    break;
  case FXVariant::VArray:
    if(saveArray(var)!=ErrOK) return ErrSave;
    break;
  case FXVariant::VMap:
    if(saveMap(var)!=ErrOK) return ErrSave;
    break;
    }
  return ErrOK;
  }


/*******************************************************************************/

// Load a variant
FXJSON::Error FXJSON::load(FXVariant& variant){
  FXTRACE((2,"FXJSON::load(variant)\n"));
  Error err=ErrLoad;
  if(dir==Load){
    token=next();
    err=loadVariant(variant);
    }
  return err;
  }


// Save a variant
FXJSON::Error FXJSON::save(const FXVariant& variant){
  FXTRACE((2,"FXJSON::save(variant)\n"));
  Error err=ErrSave;
  if(dir==Save){
    err=saveVariant(variant);
    }
  return err;
  }


// Close stream and delete buffers
FXbool FXJSON::close(){
  FXTRACE((2,"FXJSON::close()\n"));
  if(dir!=Stop){
    if((dir==Load) || flush()){
      if(owns){ freeElms(begptr); }
      begptr=NULL;
      endptr=NULL;
      sptr=NULL;
      rptr=NULL;
      wptr=NULL;
      token=TK_ERROR;
      dir=Stop;
      return true;
      }
    }
  return false;
  }


// Close stream and clean up
FXJSON::~FXJSON(){
  FXTRACE((1,"FXJSON::~FXJSON\n"));
  close();
  }

}
