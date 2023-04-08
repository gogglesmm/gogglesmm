/********************************************************************************
*                                                                               *
*                   V a r a r g s   S c a n f   R o u t i n e s                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXString.h"


/*
  Notes:
  - It may be not perfect, but at least its the same on all platforms.
  - Handles conversions of the form:

        % [*'] [digits$] [width] [l|ll|h|hh|L|q|t|z] [n|p|d|u|i|x|X|o|D|c|s|[SET]|e|E|f|G|g|b]

  - Assignment suppression and number-grouping:
     '*'        When '*' is passed assignment of the matching quantity is suppressed.
     '''        Commas for thousands, like 1,000,000.

  - Positional argument:
     'digits$'  A sequence of decimal digits indication the position in the parameter
                list, followed by '$'.  The first parameter starts at 1.

  - Width:
     digits     Specifies field width to read, not counting spaces [except for %[] and
                %c and %n directives where spaces do count].

  - Interpretation of size parameters:

     'hh'       convert to FXchar
     'h'        convert to FXshort
     ''         convert to FXint (or FXfloat if real)
     'l'        convert to long (or FXdouble if real)
     'll'       convert to FXlong (64-bit number)
     'L'        ditto
     'q'        ditto
     't'        convert to FXival (size depends on pointer size)
     'z'        convert to FXuval (size depends on pointer size)

  - Conversion specifiers:

     'd'        Decimal integer conversion.
     'b'        Binary integer conversion.
     'i'        Integer conversion from octal, hex, or decimal number.
     'o'        Octal integer conversion.
     'u'        Unsigned decimal integer conversion.
     'x' or 'X' Hexadecimal conversion.
     'c'        String conversion.
     's'        String conversion of printing characters [no spaces].
     'n'        Assign number of characters scanned so far.
     'p'        Hexadecimal pointer conversion.
     [SET]      String conversion of characters in set only.
     'e', 'E',  Floating point conversion.
     'f', 'F'   Floating point conversion.
     'g', 'G'   Floating point conversion.
     'a', 'A'   Floating point conversion.

  - In the case of string (%s or %[...]), do not report a conversion unless at
    least one character is returned.

  - Algorithm accumulate up to 16 digits from the string, starting from most
    significant digit; then convert to real and adjust exponent as necessary.

  - Background: integer numbers larger than 2**53 (52 bits plus one hidden bit)
    can not be represented exactly as double-precision real number, so any
    additional digits past 10**16 are only useful for rounding purposes.

  - Some special numeric values:

    2**53   =     9007199254740992  =  Largest representable mantissa, plus 1.
    10**16  =    10000000000000000  =  Power of 10 slightly larger than 2**53.
    10**18  =  1000000000000000000  =  Power of 10 less than 2**63.
    2**63   =  9223372036854775808  =  Largest 64-bit signed integer, plus 1
    10**19  = 10000000000000000000  =  Power of 10 slightly less than 2**64.
    2**64   = 18446744073709551616  =  Largest 64-bit unsigned integer, plus 1.
*/

#if defined(WIN32)
#ifndef va_copy
#define va_copy(arg,list) ((arg)=(list))
#endif
#endif


using namespace FX;

/*******************************************************************************/

namespace FX {


// Type modifiers
enum {
  ARG_HALFHALF,         // 'hh'
  ARG_HALF,             // 'h'
  ARG_DEFAULT,          // (No specifier)
  ARG_LONG,             // 'l'
  ARG_LONGLONG,         // 'll' / 'L' / 'q'
  ARG_VARIABLE          // Depending on size of pointer
  };


// Some floating point magick
static const union{ FXulong u; FXdouble f; } dblinf={FXULONG(0x7ff0000000000000)};
static const union{ FXulong u; FXdouble f; } dblnan={FXULONG(0x7fffffffffffffff)};

// Normal mode (non-ascii character)
const FXint NORMAL=0x100;

// Thousands separator
const FXint COMMA=',';

// Declarations
extern FXAPI FXint __sscanf(const FXchar* string,const FXchar* format,...);
extern FXAPI FXint __vsscanf(const FXchar* string,const FXchar* format,va_list arg_ptr);

/*******************************************************************************/

// Make double from sign, exponent, and mantissa
static inline FXdouble fpMake(FXlong m,FXlong exp){
  union{ FXulong u; FXdouble f; } z={(m&FXULONG(0x000fffffffffffff)) | (((exp+1023)&0x7ff)<<52)};
  return z.f;
  }

#if 0
// Fast hex digit '0'..'9', 'A'..'F', 'a'..'f' to int
// Otherwise bad values
static inline FXint htoi(FXint x){
  return (x>>6)*9+(x&15);
  }
#endif

// Scan with va_list arguments
FXint __vsscanf(const FXchar* string,const FXchar* format,va_list args){
  FXint modifier,width,convert,comma,base,digits,count,exponent,expo,done,neg,nex,pos,ww,v;
  const FXchar *start=string;
  const FXchar *ss;
  FXdouble dvalue;
  FXulong  ivalue;
  FXulong  imult;
  FXchar   set[256];
  FXchar  *ptr;
  FXuchar  ch,nn;
  va_list  ag;

  count=0;

  // Process format string
  va_copy(ag,args);
  while((ch=*format++)!='\0'){

    // Check for format-characters
    if(ch=='%'){

      // Get next format character
      ch=*format++;

      // Check for '%%'
      if(ch=='%') goto nml;

      // Default settings
      modifier=ARG_DEFAULT;
      width=0;
      convert=1;
      done=0;
      comma=NORMAL;
      base=0;

      // Parse format specifier
flg:  switch(ch){
        case '*':                                               // Suppress conversion
          convert=0;
          ch=*format++;
          goto flg;
        case '\'':                                              // Print thousandths
          comma=COMMA;                                          // Thousands grouping character
          ch=*format++;
          goto flg;
        case '0':                                               // Width or positional parameter
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          width=ch-'0';
          ch=*format++;
          while(Ascii::isDigit(ch)){
            width=width*10+ch-'0';
            ch=*format++;
            }
          if(ch=='$'){                                          // Positional parameter
            ch=*format++;
            if(width<=0) goto x;                                // Not a legal parameter position
            va_copy(ag,args);
            for(pos=1; pos<width; ++pos){                       // Advance to position prior to arg
              (void)va_arg(ag,void*);
              }
            width=0;                                            // Reset width
            }
          goto flg;
        case 'l':                                               // Long
          modifier=ARG_LONG;
          ch=*format++;
          if(ch=='l'){                                          // Long Long
            modifier=ARG_LONGLONG;
            ch=*format++;
            }
          goto flg;
        case 'h':                                               // Short
          modifier=ARG_HALF;
          ch=*format++;
          if(ch=='h'){                                          // Char
            modifier=ARG_HALFHALF;
            ch=*format++;
            }
          goto flg;
        case 'L':                                               // Long Long
        case 'q':
          modifier=ARG_LONGLONG;
          ch=*format++;
          goto flg;
        case 't':                                               // Size depends on pointer
        case 'z':
          modifier=ARG_VARIABLE;
          ch=*format++;
          goto flg;
        case 'n':                                               // Consumed characters till here
          ivalue=string-start;
          if(convert){
            if(modifier==ARG_DEFAULT){                          // 32-bit always
              *va_arg(ag,FXint*)=(FXint)ivalue;
              }
            else if(modifier==ARG_LONG){                        // Whatever size a long is
              *va_arg(ag,long*)=(long)ivalue;
              }
            else if(modifier==ARG_LONGLONG){                    // 64-bit always
              *va_arg(ag,FXlong*)=ivalue;
              }
            else if(modifier==ARG_HALF){                        // 16-bit always
              *va_arg(ag,FXshort*)=(FXshort)ivalue;
              }
            else if(modifier==ARG_HALFHALF){                    // 8-bit always
              *va_arg(ag,FXchar*)=(FXchar)ivalue;
              }
            else{                                               // Whatever size a pointer is
              *va_arg(ag,FXival*)=(FXival)ivalue;
              }
            }
          break;
        case 'p':                                               // Hex pointer
          modifier=ARG_VARIABLE;
          base=16;
          comma=NORMAL;
          goto integer;
        case 'x':                                               // Hex
        case 'X':
          base=16;
          comma=NORMAL;
          goto integer;
        case 'o':                                               // Octal
          base=8;
          comma=NORMAL;
          goto integer;
        case 'b':                                               // Binary
          base=2;
          comma=NORMAL;
          goto integer;
        case 'd':                                               // Decimal
        case 'u':
          base=10;
        case 'i':                                               // Integer
integer:  ivalue=0;
          digits=0;
          if(width<1) width=2147483647;                         // Width at least 1
          while(Ascii::isSpace(string[0])) string++;            // Skip white space; not included in field width
          if((neg=(string[0]=='-')) || (string[0]=='+')){
            string++;
            width--;
            }
          if(0<width && string[0]=='0'){                        // Got "0"
            if(1<width && (string[1]|0x20)=='x'){               // Got a "0x"
              if(base==0 || base==16){                          // Its hex
                comma=NORMAL;
                base=16;
                string+=2;
                width-=2;
                }
              }
            else if(1<width && (string[1]|0x20)=='b'){          // Got a "0b"
              if(base==0 || base==2){                           // Its binary
                comma=NORMAL;
                base=2;
                string+=2;
                width-=2;
                }
              }
            else{                                               // Got a "0"
              if(base==0 || base==8){                           // Its octal
                comma=NORMAL;
                base=8;
                }
              }
            }
          else{                                                 // Got something else
            if(base==0){                                        // If not set, its decimal now
              base=10;
              }
            }
          while(0<width && 0<=(v=Ascii::digitValue(string[0])) && v<base){
            ivalue=ivalue*base+v;
            string++;
            width--;
            digits++;
            }
          if(3<width && string[0]==comma && 0<digits && digits<4){ // Thousands group is ',ddd' or nothing
            do{
              if((ch=string[1])<'0' || ch>'9') break;
              if((ch=string[2])<'0' || ch>'9') break;
              if((ch=string[3])<'0' || ch>'9') break;
              ivalue=((ivalue*10+string[1])*10+string[2])*10+string[3]-5328;
              string+=4;
              width-=4;
              digits+=3;
              }
            while(3<width && string[0]==comma);
            }
          if(!digits) goto x;                                   // No digits seen!
          if(neg){                                              // Apply sign
            ivalue=0-ivalue;
            }
          if(convert){
            if(modifier==ARG_DEFAULT){                          // 32-bit always
              *va_arg(ag,FXint*)=(FXint)ivalue;
              }
            else if(modifier==ARG_LONG){                        // Whatever size a long is
              *va_arg(ag,long*)=(long)ivalue;
              }
            else if(modifier==ARG_LONGLONG){                    // 64-bit always
              *va_arg(ag,FXlong*)=ivalue;
              }
            else if(modifier==ARG_HALF){                        // 16-bit always
              *va_arg(ag,FXshort*)=(FXshort)ivalue;
              }
            else if(modifier==ARG_HALFHALF){                    // 8-bit always
              *va_arg(ag,FXchar*)=(FXchar)ivalue;
              }
            else{                                               // Whatever size a pointer is
              *va_arg(ag,FXival*)=(FXival)ivalue;
              }
            count++;
            }
          break;
        case 'a':                                               // Floating point
        case 'A':
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
          dvalue=0.0;
          ivalue=0;
          digits=0;
          if(width<1) width=2147483647;                         // Width at least 1
          while(Ascii::isSpace(string[0])) string++;            // Skip white space; not included in field width
          if((neg=(string[0]=='-')) || (string[0]=='+')){
            string++;
            width--;
            }
          if(2<width && (string[0]|0x20)=='n'){                 // NaN
            if((string[1]|0x20)!='a') goto x;
            if((string[2]|0x20)!='n') goto x;
            string+=3;
            dvalue=dblnan.f;
            }
          else if(2<width && (string[0]|0x20)=='i'){            // Inf{inity}
            if((string[1]|0x20)!='n') goto x;
            if((string[2]|0x20)!='f') goto x;
            if(7<width && (string[3]|0x20)=='i' && (string[4]|0x20)=='n' && (string[5]|0x20)=='i' && (string[6]|0x20)=='t' && (string[7]|0x20)=='y'){
              string+=5;
              }
            string+=3;
            dvalue=dblinf.f;
            }
          else if(1<width && string[0]=='0' && (string[1]|0x20)=='x'){  // Hexadecimal float
            string+=2;
            width-=2;
            exponent=-4;
            imult=FXLONG(0x0010000000000000);
            while(0<width && string[0]=='0'){
              string++;
              width--;
              digits++;
              }
            while(0<width && 0<=(v=Ascii::digitValue(string[0])) && v<16){
              ivalue+=imult*v;
              imult>>=4;
              exponent+=4;
              string++;
              width--;
              digits++;
              }
            if(0<width && string[0]=='.'){                      // Hexadecimals following '.'
              string++;
              width--;
              if(ivalue==0){
                while(0<width && string[0]=='0'){
                  exponent-=4;
                  string++;
                  width--;
                  digits++;
                  }
                }
              while(0<width && 0<=(v=Ascii::digitValue(string[0])) && v<16){
                ivalue+=imult*v;
                imult>>=4;
                string++;
                width--;
                digits++;
                }
              }
            if(!digits) goto x;                                 // No digits seen!
            if(1<width && (string[0]|0x20)=='p'){               // Handle exponent, tentatively
              ss=string;                                        // Rewind point if no match
              ww=width;
              ss++;
              ww--;
              expo=0;
              if((nex=(ss[0]=='-')) || (ss[0]=='+')){           // Handle exponent sign
                ss++;
                ww--;
                }
              if(0<ww && '0'<=ss[0] && ss[0]<='9'){             // Have exponent?
                while(0<ww && '0'<=(ch=ss[0]) && ch<='9'){
                  expo=expo*10+(ch-'0');
                  ss++;
                  ww--;
                  }
                if(nex){
                  exponent-=expo;
                  }
                else{
                  exponent+=expo;
                  }
                string=ss;                                      // Eat exponent characters
                }
              }
            if(ivalue!=0){
              while(FXLONG(0x001fffffffffffff)<ivalue){         // Adjust to get hidden 1-bit in place
                ivalue>>=1;                                     // Shift up to 3 times
                exponent++;
                }
              if(1024<=exponent){                               // Check for overflow
                if((1024<exponent) || (FXLONG(0x0010000000000000)<=ivalue)){
                  ivalue=FXLONG(0x0010000000000000);
                  exponent=1024;
                  }
                }
              if(exponent<-1022){                               // Check for denormal or underflow
                if(exponent<-1074){
                  ivalue=0;
                  exponent=0;
                  }
                v=-exponent-1022;                               // Shift mantissa to fix exponent
                ivalue>>=v;
                exponent=-1023;
                }
              dvalue=fpMake(ivalue,exponent);                   // Assemble double
              }
            }
          else{                                                 // Decimal float
            ivalue=0;
            imult=FXLONG(10000000000000000);
            exponent=-1;
            while(0<width && string[0]=='0'){
              string++;
              width--;
              digits++;
              }
            while(0<width && '0'<=(ch=string[0]) && ch<='9'){
              ivalue+=(ch-'0')*imult;
              imult/=10;
              exponent++;
              string++;
              width--;
              digits++;
              }
            if(3<width && string[0]==comma && 0<digits && digits<4){ // Thousands group is ',ddd' or nothing
              do{
                if((ch=string[1])<'0' || ch>'9') break;
                if((ch=string[2])<'0' || ch>'9') break;
                if((ch=string[3])<'0' || ch>'9') break;
                ivalue+=(string[1]-'0')*imult;
                if(ivalue){imult/=10;exponent++;}
                ivalue+=(string[2]-'0')*imult;
                if(ivalue){imult/=10;exponent++;}
                ivalue+=(string[3]-'0')*imult;
                if(ivalue){imult/=10;exponent++;}
                string+=4;
                width-=4;
                digits+=3;
                }
              while(3<width && string[0]==comma);
              }
            if(0<width && string[0]=='.'){                      // Decimals following '.'
              string++;
              width--;
              if(ivalue==0){
                while(0<width && string[0]=='0'){
                  exponent--;
                  string++;
                  width--;
                  digits++;
                  }
                }
              while(0<width && '0'<=(ch=string[0]) && ch<='9'){
                ivalue+=(ch-'0')*imult;
                imult/=10;
                string++;
                width--;
                digits++;
                }
              }
            if(!digits) goto x;                                 // No digits seen!
            if(1<width && (string[0]|0x20)=='e'){               // Handle exponent, tentatively
              ss=string;                                        // Rewind point if no match
              ww=width;
              ss++;
              ww--;
              expo=0;
              if((nex=(ss[0]=='-')) || (ss[0]=='+')){           // Handle exponent sign
                ss++;
                ww--;
                }
              if(0<ww && '0'<=ss[0] && ss[0]<='9'){             // Have exponent?
                while(0<ww && '0'<=(ch=ss[0]) && ch<='9'){
                  expo=expo*10+(ch-'0');
                  ss++;
                  ww--;
                  }
                if(nex){
                  exponent-=expo;
                  }
                else{
                  exponent+=expo;
                  }
                string=ss;                                      // Eat exponent characters
                }
              }
            dvalue=1.0E-16*(FXlong)ivalue;                      // Convert to 64-bit integer to 64-bit real
            if(dvalue!=0.0){
              if(308<=exponent){                                // Check for overflow
                if((308<exponent) || (1.79769313486231570815<=dvalue)){
                  dvalue=1.79769313486231570815E+308;
                  exponent=0;
                  }
                }
              if(exponent<-308){                                // Check for denormal or underflow
                if((exponent<-324) || ((exponent==-324) && (dvalue<=4.94065645841246544177))){
                  dvalue=0.0;
                  exponent=0;
                  }
                dvalue*=1.0E-16;
                exponent+=16;
                }
              dvalue*=Math::pow10i(exponent);                   // Exponent in range
              }
            }
          if(neg){                                              // Apply sign
            dvalue=-dvalue;
            }
          if(convert){
            if(modifier==ARG_DEFAULT){
              *va_arg(ag,FXfloat*)=(FXfloat)dvalue;             // 32-bit float
              }
            else{
              *va_arg(ag,FXdouble*)=dvalue;                     // 64-bit double
              }
            count++;
            }
          break;
        case 'c':                                               // Character(s)
          if(width<1) width=1;                                  // Width at least 1
          if(convert){
            ptr=va_arg(ag,FXchar*);
            while(0<width && (ch=string[0])!='\0'){
              *ptr++=ch;
              string++;
              width--;
              done=1;
              }
            count+=done;
            }
          else{
            while(0<width && string[0]!='\0'){
              string++;
              width--;
              }
            }
          break;
        case 's':                                               // String
          if(width<1) width=2147483647;                         // Width at least 1
          while(Ascii::isSpace(string[0])) string++;            // Skip white space
          if(convert){
            ptr=va_arg(ag,FXchar*);
            while(0<width && (ch=string[0])!='\0' && !Ascii::isSpace(ch)){
              *ptr++=ch;
              string++;
              width--;
              done=1;
              }
            *ptr='\0';
            count+=done;
            }
          else{
            while(0<width && (ch=string[0])!='\0' && !Ascii::isSpace(ch)){
              string++;
              width--;
              }
            }
          break;
        case '[':                                               // Character set
          if(width<1) width=2147483647;                         // Width at least 1
          ch=(FXuchar)*format++;
          v=1;                                                  // Add characters to set
          if(ch=='^'){                                          // Negated character set
            ch=(FXuchar)*format++;
            v=0;                                                // Remove characters from set
            }
          memset(set,1-v,sizeof(set));                          // Initialize set
          if(ch=='\0') goto x;                                  // Format error
          for(;;){                                              // Parse set
            set[ch]=v;
            nn=(FXuchar)*format++;
            if(nn=='\0') goto x;                                // Format error
            if(nn==']') break;
            if(nn=='-'){
              nn=(FXuchar)*format;
              if(nn!=']' && ch<=nn){                            // Range if not at end
                while(ch<nn){ set[++ch]=v; }
                format++;
                }
              else{                                             // Otherwise just '-'
                nn='-';
                }
              }
            ch=nn;
            }
          if(convert){
            ptr=va_arg(ag,FXchar*);
            while(0<width && (ch=*string)!='\0' && set[(FXuchar)ch]){
              *ptr++=ch;
              string++;
              width--;
              done=1;
              }
            *ptr='\0';
            count+=done;
            }
          else{
            while(0<width && (ch=*string)!='\0' && set[(FXuchar)ch]){
              string++;
              width--;
              }
            }
          break;
        default:                                                // Format error
          goto x;
        }
      continue;
      }

    // Check for spaces
nml:if(Ascii::isSpace(ch)){
      while(Ascii::isSpace(*format)) format++;
      while(Ascii::isSpace(*string)) string++;
      continue;
      }

    // Regular characters must match
    if(*string!=ch) break;

    // Next regular character
    string++;
    }
x:va_end(ag);
  return count;
  }


// Scan with variable number of arguments
FXint __sscanf(const FXchar* string,const FXchar* format,...){
  va_list args;
  va_start(args,format);
  FXint result=__vsscanf(string,format,args);
  va_end(args);
  return result;
  }

}
