/********************************************************************************
*                                                                               *
*                  V a r a r g s   P r i n t f   R o u t i n e s                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2017 by Jeroen van der Zijp.   All Rights Reserved.        *
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



/*
  Notes:
  - Handles conversions of the form:

        % [digits$] [#0-+' ] [width] [.precision] [l|ll|h|hh|L|q|t|z] [d|i|o|u|x|X|e|E|f|F|g|G|c|s|n|p]

  - Positional argument:
     'digits$'  A sequence of decimal digits indication the position in the parameter list,
                followed by '$'.  The first parameter starts at 1.

  - Interpretation of the flags:
     '#'        Alternate form (prefix '0' for octal, '0x' for hex, decimal point if float.
     '0'        Zero-padding.
     '-'        Left-adjust.
     '+'        Show sign always.
     '''        Insert commas for thousands, like 1,000,000. Fractional part is printed with
                no thousands-separators. [For exponential notation, make exponent a multiple of
                3, so for example, print 3.567E-5 as 35.67E-6.  The C library ignores this option
                unless printing fractions].
     ' '        Leave blank for positive numbers.

  - Width:
    digits      Explicit format width.
    *           Format width passed as a parameter.
    *digits$    Format width in positional parameter.  The first parameter starts at 1.

    If the format width is negative, it is interpreted as left-justified, same if the '-'
    flag was used.

  - Precision:
    digits      Explicit precision.
    *           Precision passed as a parameter.
    *digits$    Precision in positional parameter.  The first parameter starts at 1.

    The maximum precision supported is 100, and the minimum value is 0.  If not specified,
    a value of 6 will be used for floating point conversions.

  - Interpretation of size parameters:
     'hh'       convert from FXchar.
     'h'        convert from FXshort.
     ''         convert from FXint (or FXfloat if real).
     'l'        convert from long (or FXdouble if real).
     'll'       convert from FXlong (64-bit number).
     'L'        ditto.
     'q'        ditto.
     't'        convert from FXival (size depends on pointer size).
     'z'        convert from FXuval (sizedepends on pointer size).

  - Conversion specifiers:

     'd'        Decimal integer conversion.
     'b'        Binary integer conversion.
     'i'        Integer conversion from octal, hex, or decimal number.
     'o'        Octal integer conversion.
     'u'        Unsigned decimal integer conversion.
     'x' or 'X' Hexadecimal conversion.
     's'        String conversion of printing characters [no spaces].
     'c'        String conversion.
     'n'        Assign number of characters printed so far.
     'p'        Hexadecimal pointer conversion.
     'e', 'E',  Exponential notation floating point conversion.
     'f', 'F'   Simple point conversion.
     'g', 'G'   Shortest representation point conversion.

  - If the range of positional parameters in a format string is not contiguous,
    i.e. if a positional parameter is skipped (e.g. "%3$d%1$d"), then the missing
    one is assumed to be of type "int".
    Its therefore best if no parameters are skipped; referencing a single parameter
    multiple times however, is no problem!!
  - FIXME Subtle difference between glibc: does NOT output '\0' at the end, unless
    buffer is large enough.  This implementation is better for our purposes, however.
*/

#define CONVERTSIZE     512     // Conversion buffer
#define NDIG            512     // Maximum space used for numbers

#ifdef WIN32
#ifndef va_copy
#define va_copy(arg,list) ((arg)=(list))
#endif
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Declarations
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);
extern FXAPI FXint __vsnprintf(FXchar* string,FXint length,const FXchar* format,va_list args);


// Type modifiers
enum {
  ARG_DEFAULT  = 0,     // (No specifier)
  ARG_HALFHALF = 1,     // 'hh'
  ARG_HALF     = 2,     // 'h'
  ARG_LONG     = 3,     // 'l'
  ARG_LONGLONG = 4,     // 'll' / 'L' / 'q'
  ARG_VARIABLE = 5      // Depending on size of pointer
  };

// Conversion flags
enum {
  FLG_DEFAULT  = 0,     // Default option
  FLG_BLANK    = 1,     // Print blank if positive
  FLG_SIGN     = 2,     // Always show sign
  FLG_ZERO     = 4,     // Pad with zeroes if numeric
  FLG_LEFT     = 8,     // Left aligned
  FLG_ALTER    = 16,    // Alternate form
  FLG_UPPER    = 32,    // Use upper case
  FLG_UNSIGNED = 64,    // Unsigned
  FLG_THOUSAND = 128,   // Print comma's for thousands
  FLG_EXPONENT = 256,   // Exponential notation
  FLG_FRACTION = 512,   // Fractional notation
  FLG_DOTSEEN  = 1024   // Dot was seen
  };


static const FXchar lower_digits[]="0123456789abcdef";
static const FXchar upper_digits[]="0123456789ABCDEF";

/*******************************************************************************/

// The buffer must have at least NDIG bytes
static FXchar* _cvt(FXchar* buffer,FXdouble arg,FXint& decimal,FXint prec,FXuint flags){
  register FXchar *ptr=buffer;
  register FXchar *p;
  register FXint d=0;
  FXdouble fi;
  FXdouble fj;

  // Clamp to maximum digits
  if(prec>=NDIG-1) prec=NDIG-2;

  // Split into whole and fraction
  arg=modf(arg,&fi);

  // Generate integer part
  p=buffer+NDIG;
  if(fi!=0.0){
    while(buffer<p && fi!=0.0){
      fj=modf(fi/10.0,&fi);
      *--p=(FXint)((fj+0.03)*10.0)+'0';
      d++;
      }
    while(p<buffer+NDIG){
      *ptr++=*p++;
      }
    }

  // Else shift fraction until >=1
  else if(0.0<arg){
    while((fj=arg*10.0)<1.0){
      arg=fj;
      d--;
      }
    }

  // End of digits to generate
  p=buffer+prec;

  // In normal mode, add the ones before the decimal point
  if(flags&FLG_FRACTION) p+=d;

  // Not enough room for expected digits
  if(p<buffer){
    decimal=-prec;           // With prec=5 0.000001 becomes 0.00000
    buffer[0]='\0';
    return buffer;
    }

  // Decimal point location from start; may be negative
  decimal=d;

  // Generate fraction of up to prec digits, plus one
  while(ptr<=p && ptr<buffer+NDIG){
    arg*=10.0;
    arg=modf(arg,&fj);
    *ptr++=(FXint)fj+'0';
    }

  // No more space
  if(p>=buffer+NDIG){
    buffer[NDIG-1]='\0';
    return buffer;
    }

  // Round the result
  ptr=p;
  *p+=5;
  while('9' < *p){
    *p='0';
    if(buffer<p){
      ++*--p;
      }
    else{
      *p='1';
      decimal++;
      if(flags&FLG_FRACTION){           // Extra zero at the end for normal mode
        if(buffer<ptr) *ptr='0';
        ptr++;
        }
      }
    }

  // Terminate
  *ptr='\0';

  // Done
  return buffer;
  }


/*******************************************************************************/

// Convert minimal length mode; precision is significant digits
static FXchar *convertGeneral(FXchar *buffer,FXint& len,FXdouble number,FXint prec,FXuint flags){
  register FXchar *ptr=buffer;
  register FXchar *p;
  register FXint i;
  FXchar digits[NDIG];
  FXint  decimal;
  FXbool mode;

  // Handle sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    *ptr++='-';
    }
  else if(flags&FLG_SIGN){
    *ptr++='+';
    }
  else if(flags&FLG_BLANK){
    *ptr++=' ';
    }

  // Infinity
  if(Math::fpInfinite(number)){
    *ptr++='i';
    *ptr++='n';
    *ptr++='f';
    }

  // NaN
  else if(Math::fpNan(number)){
    *ptr++='n';
    *ptr++='a';
    *ptr++='n';
    }

  // Finite
  else{

    // Convert the number as if by exponential mode
    p=_cvt(digits,number,decimal,prec,flags);

    // Check if we need to use exponential mode
    mode=(prec<decimal) || (decimal<-3);

    // Eliminate trailing zeroes; not done for alternate mode
    if(!(flags&FLG_ALTER)){
      while(0<prec && p[prec-1]=='0') prec--;
      }

    // Exponential notation
    if(mode){
      *ptr++=*p++;                                // One before the decimal point
      if((1<prec) || (flags&FLG_ALTER)){          // Decimal point needed
        *ptr++='.';
        }
      for(i=1; i<prec; i++){                      // Remaining fraction, if any
        *ptr++=*p++;
        }
      *ptr++=(flags&FLG_UPPER)?'E':'e';           // Exponent
      decimal--;
      if(decimal<0){                              // Sign of exponent
        decimal=-decimal;
        *ptr++='-';
        }
      else{
        *ptr++='+';
        }
      if(99<decimal){                             // Decimal should be [-308...308]
        *ptr++=(decimal/100)+'0';
        *ptr++=((decimal/10))%10+'0';
        *ptr++=(decimal%10)+'0';
        }
      else{                                       // Typical case
        *ptr++=(decimal/10)+'0';
        *ptr++=(decimal%10)+'0';
        }
      }

    // Fractional notation
    else{

      // Fraction-only notation +0.0000dddddd
      if(decimal<=0){
        *ptr++='0';
        if(decimal<0 || 0<prec) *ptr++='.';         // Decimal point only if followed by at least one digit
        while(decimal++<0){                         // Generate leading zeroes after decimal point
          *ptr++='0';
          }
        while(prec-->0){                            // Generate prec digits
          *ptr++=*p++;
          }
        }

      // Integral notation
      else if(prec<=decimal){
        while(prec-->0){                          // Generate prec digits
          *ptr++=*p++;
          decimal--;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }
        while(decimal-->0){
          *ptr++='0';
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }
        if(flags&FLG_ALTER) *ptr++='.';             // End with decimal point if alternate mode
        }

      // Normal notation +dddd.dd
      else{
        while(decimal-->0){
          *ptr++=*p++;
          prec--;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }
        *ptr++='.';                                 // Output decimal point
        while(prec-->0){                            // Append more digits until we get prec
          *ptr++=*p++;
          }
        }
      }
    }

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }


/*******************************************************************************/


// Convert double number; precision is digits after decimal point
static FXchar* convertDouble(FXchar* buffer,FXint& len,FXdouble number,FXint prec,FXint flags){
  register FXchar *ptr=buffer;
  register FXchar *p;
  FXchar digits[NDIG];
  FXint  decimal;

  // Handle sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    *ptr++='-';
    }
  else if(flags&FLG_SIGN){
    *ptr++='+';
    }
  else if(flags&FLG_BLANK){
    *ptr++=' ';
    }

  // Infinity
  if(Math::fpInfinite(number)){
    *ptr++='i';
    *ptr++='n';
    *ptr++='f';
    }

  // NaN
  else if(Math::fpNan(number)){
    *ptr++='n';
    *ptr++='a';
    *ptr++='n';
    }

  // Finite
  else{

    // Exponential notation
    if(flags&FLG_EXPONENT){
      p=_cvt(digits,number,decimal,prec+1,flags);
      }

    // Normal notation
    else{
      p=_cvt(digits,number,decimal,prec,flags);
      }

    // Exponential notation
    if(flags&FLG_EXPONENT){
      *ptr++=*p++;                                // One digit before decimal point
      if((0<prec) || (flags&FLG_ALTER)){          // Decimal point needed
        *ptr++='.';
        }
      while(*p){                                  // Copy fraction
        *ptr++=*p++;
        }
      *ptr++=(flags&FLG_UPPER)?'E':'e';           // Exponent
      if(number!=0.0){
        decimal--;
        if(decimal<0){
          decimal=-decimal;
          *ptr++='-';
          }
        else{
          *ptr++='+';
          }
        if(99<decimal){                           // Decimal should be [-308...308]
          *ptr++=(decimal/100)+'0';
          *ptr++=((decimal/10))%10+'0';
          *ptr++=(decimal%10)+'0';
          }
        else{                                     // Typical case
          *ptr++=(decimal/10)+'0';
          *ptr++=(decimal%10)+'0';
          }
        }
      else{
        *ptr++='+';
        *ptr++='0';
        *ptr++='0';
        }
      }

    // Fractional notation
    else{

      // Fractional notation +0.0000ddddd
      if(decimal<=0){                             // Decimal point is negative or zero
        *ptr++='0';
        if((0<prec) || (flags&FLG_ALTER)){
          *ptr++='.';
          }
        while(decimal++<0){                       // Output a bunch of zeroes preceeded by '0.'
          *ptr++='0';
          }
        while(*p){                                // Copy fraction
          *ptr++=*p++;
          }
        }

      // Normal notation +ddd.dd
      else{                                       // Decimal point is positive
        while(decimal-->0){
          *ptr++=*p++;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }
        if((0<prec) || (flags&FLG_ALTER)){        // Decimal point needed
          *ptr++='.';
          }
        while(*p){                                // Copy fraction
          *ptr++=*p++;
          }
        }
      }
    }

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }

/*******************************************************************************/


// Convert long value
static FXchar* convertLong(FXchar* buffer,FXint& len,FXlong value,FXuint base,FXint prec,FXuint flags){
  const FXchar *digits=(flags&FLG_UPPER)?upper_digits:lower_digits;
  FXchar *end=buffer+CONVERTSIZE-1;
  FXchar *ptr=end;
  FXulong number=value;
  FXchar sign=0;
  FXint digs=0;

  // Deal with sign
  if(!(flags&FLG_UNSIGNED)){
    if(value<0){
      sign='-';
      number=-value;
      }
    else if(flags&FLG_SIGN){
      sign='+';
      }
    else if(flags&FLG_BLANK){
      sign=' ';
      }
    }

  // Terminate string
  *ptr='\0';

  // Convert to string using base
  do{
    *--ptr=digits[number%base];
    number/=base;
    if(flags&FLG_THOUSAND){
      if(++digs%3==0 && number) *--ptr=',';
      }
    prec--;
    }
  while(number);

  // Pad to precision
  while(0<prec){
    *--ptr='0';
    --prec;
    }

  // Alternate form
  if(flags&FLG_ALTER){
    if(base==8 && *ptr!='0'){           // Prepend '0'
      *--ptr='0';
      }
    else if(base==16 && value){         // Prepend '0x'
      *--ptr=(flags&FLG_UPPER)?'X':'x';
      *--ptr='0';
      }
    else if(base==2 && value){          // Prepend '0b'
      *--ptr=(flags&FLG_UPPER)?'B':'b';
      *--ptr='0';
      }
    }

  // Prepend sign
  if(sign){
    *--ptr=sign;
    }

  // Return length
  len=end-ptr;
  return ptr;
  }


/*******************************************************************************/


// Advance ag from args to position before pos
void vadvance(va_list& ag,va_list args,const FXchar* format,FXint pos){
  register FXint ch,modifier,val,v;
  register const FXchar* fmt;
  register FXint cur=1;
  va_copy(ag,args);
  while(cur<pos){
    fmt=format;
    while((ch=*fmt++)!='\0'){
      if(ch=='%'){
        ch=*fmt++;
        if(ch=='%') continue;
        modifier=ARG_DEFAULT;
        val=0;
flg:    switch(ch){
          case ' ':
          case '-':
          case '+':
          case '#':
          case '\'':
          case '.':                                     // Precision follows
            ch=*fmt++;
            goto flg;
          case '*':                                     // Width or precision parameter
            ch=*fmt++;
            if(Ascii::isDigit(ch)){
              v=ch-'0';
              ch=*fmt++;
              while(Ascii::isDigit(ch)){
                v=v*10+ch-'0';
                ch=*fmt++;
                }
              if(ch!='$') return;                       // Bail on format-error
              ch=*fmt++;
              if(v==cur){
                (void)va_arg(ag,FXint);
                goto nxt;
                }
              }
            goto flg;
          case '0':                                     // Print leading zeroes
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            v=ch-'0';
            ch=*fmt++;
            while(Ascii::isDigit(ch)){
              v=v*10+ch-'0';
              ch=*fmt++;
              }
            if(ch=='$'){                                // Positional parameter
              ch=*fmt++;
              val=v;
              goto flg;
              }
            goto flg;
          case 'l':                                     // Long
            modifier=ARG_LONG;
            ch=*fmt++;
            if(ch=='l'){                                // Long Long
              modifier=ARG_LONGLONG;
              ch=*fmt++;
              }
            goto flg;
          case 'h':                                     // Short
            modifier=ARG_HALF;
            ch=*fmt++;
            if(ch=='h'){                                // Char
              modifier=ARG_HALFHALF;
              ch=*fmt++;
              }
            goto flg;
          case 'L':
          case 'q':                                     // Long Long
            modifier=ARG_LONGLONG;
            ch=*fmt++;
            goto flg;
          case 't':
          case 'z':                                     // Size depends on pointer
            modifier=ARG_VARIABLE;
            ch=*fmt++;
            goto flg;
          case 'u':
          case 'd':
          case 'i':
          case 'b':
          case 'o':
          case 'X':
          case 'x':
            if(val==cur){
              if(modifier==ARG_DEFAULT){                // 32-bit always
                (void)va_arg(ag,FXuint);
                }
              else if(modifier==ARG_LONG){              // Whatever size a long is
                (void)va_arg(ag,unsigned long);
                }
              else if(modifier==ARG_LONGLONG){          // 64-bit always
                (void)va_arg(ag,FXulong);
                }
              else if(modifier==ARG_HALF){              // 16-bit always
                (void)va_arg(ag,FXuint);
                }
              else if(modifier==ARG_HALFHALF){          // 8-bit always
                (void)va_arg(ag,FXuint);
                }
              else{                                     // Whatever size a pointer is
                (void)va_arg(ag,FXuval);
                }
              goto nxt;
              }
            break;
          case 'F':
          case 'f':
          case 'E':
          case 'e':
          case 'G':
          case 'g':
            if(val==cur){
              (void)va_arg(ag,FXdouble);
              goto nxt;
              }
            break;
          case 'c':
            if(val==cur){
              (void)va_arg(ag,FXint);
              goto nxt;
              }
            break;
          case 's':
            if(val==cur){
              (void)va_arg(ag,FXchar*);
              goto nxt;
              }
            break;
          case 'n':
            if(val==cur){
              (void)va_arg(ag,FXint*);
              goto nxt;
              }
            break;
          case 'p':
            if(val==cur){
              (void)va_arg(ag,FXuval);
              goto nxt;
              }
            break;
          default:                                      // Bail on format-error
            return;
          }
        }
      }

    // Position cur$ not found; assume it was an int
    (void)va_arg(ag,FXint);

    // Advance to next parameter
nxt:cur++;
    }
  }


/*******************************************************************************/

// Print using format
FXint __vsnprintf(FXchar* string,FXint length,const FXchar* format,va_list args){
  FXint ch,modifier,count,flags,width,precision,pos,val,len,i;
  const FXchar *fmt=format;
  const FXchar *str;
  FXdouble number;
  FXlong value;
  FXchar buffer[CONVERTSIZE+2];
  va_list ag;

  count=0;

  // Process format string
  va_copy(ag,args);
  while((ch=*fmt++)!='\0'){

    // Check for format-characters
    if(ch=='%'){

      // Get next format character
      ch=*fmt++;

      // Check for '%%'
      if(ch=='%') goto nml;

      // Default settings
      modifier=ARG_DEFAULT;
      flags=FLG_DEFAULT;
      precision=-1;
      width=-1;
      pos=-1;
      val=0;

      // Parse format specifier
flg:  switch(ch){
        case ' ':                                       // Print blank if not negative
          flags|=FLG_BLANK;
          ch=*fmt++;
          goto flg;
        case '-':                                       // Left adjust
          flags|=FLG_LEFT;
          ch=*fmt++;
          goto flg;
        case '+':                                       // Always print sign even if positive
          flags|=FLG_SIGN;
          ch=*fmt++;
          goto flg;
        case '#':                                       // Alternate form
          flags|=FLG_ALTER;
          ch=*fmt++;
          goto flg;
        case '\'':                                      // Print thousandths
          flags|=FLG_THOUSAND;
          ch=*fmt++;
          goto flg;
        case '.':                                       // Precision follows
          flags|=FLG_DOTSEEN;
          ch=*fmt++;
          precision=0;                                  // Default is zero
          goto flg;
        case '*':                                       // Width or precision parameter
          ch=*fmt++;
          val=0;                                        // Assume non-positional parameter
          if(Ascii::isDigit(ch)){
            val=ch-'0';
            ch=*fmt++;
            while(Ascii::isDigit(ch)){
              val=val*10+ch-'0';
              ch=*fmt++;
              }
            if(ch!='$') goto x;                         // Expected positional parameter suffix '$'
            ch=*fmt++;
            if(0<val){                                  // Positional argument follows; scan to proper place in args
              vadvance(ag,args,format,val);
              }
            }
          if(flags&FLG_DOTSEEN){                        // After period: its precision
            precision=va_arg(ag,FXint);
            if(precision<0){ precision=0; }
            if(precision>100){ precision=100; }
            }
          else{                                         // Before period: its width
            width=va_arg(ag,FXint);
            if(width<0){ width=-width; flags|=FLG_LEFT; }
            }
          goto flg;
        case '0':                                       // Print leading zeroes
          if(!(flags&FLG_DOTSEEN)) flags|=FLG_ZERO;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          val=ch-'0';
          ch=*fmt++;
          while(Ascii::isDigit(ch)){
            val=val*10+ch-'0';
            ch=*fmt++;
            }
          if(ch=='$'){                                  // Positional parameter
            ch=*fmt++;
            if(val<=0) goto x;                          // Not a legal parameter position
            pos=val;                                    // Remember position
            goto flg;
            }
          if(flags&FLG_DOTSEEN){                        // After period: its precision
            precision=val;
            if(precision>100){ precision=100; }
            }
          else{                                         // Before period: its width
            width=val;
            }
          goto flg;
        case 'l':                                       // Long
          modifier=ARG_LONG;
          ch=*fmt++;
          if(ch=='l'){                                  // Long Long
            modifier=ARG_LONGLONG;
            ch=*fmt++;
            }
          goto flg;
        case 'h':                                       // Short
          modifier=ARG_HALF;
          ch=*fmt++;
          if(ch=='h'){                                  // Char
            modifier=ARG_HALFHALF;
            ch=*fmt++;
            }
          goto flg;
        case 'L':
        case 'q':                                       // Long Long
          modifier=ARG_LONGLONG;
          ch=*fmt++;
          goto flg;
        case 't':
        case 'z':                                       // Size depends on pointer
          modifier=ARG_VARIABLE;
          ch=*fmt++;
          goto flg;
        case 'u':
          flags|=FLG_UNSIGNED;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            value=(FXulong)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            value=(FXulong)va_arg(ag,unsigned long);
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            value=(FXulong)va_arg(ag,FXulong);
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            value=(FXulong)(FXushort)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            value=(FXulong)(FXuchar)va_arg(ag,FXuint);
            }
          else{                                         // Whatever size a pointer is
            value=(FXulong)va_arg(ag,FXuval);
            }
          str=convertLong(buffer,len,value,10,precision,flags);
          break;
        case 'd':
        case 'i':
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            value=(FXlong)va_arg(ag,FXint);
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            value=(FXlong)va_arg(ag,long);
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            value=(FXlong)va_arg(ag,FXlong);
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            value=(FXlong)(FXshort)va_arg(ag,FXint);
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            value=(FXlong)(FXschar)va_arg(ag,FXint);
            }
          else{                                         // Whatever size a pointer is
            value=(FXlong)va_arg(ag,FXival);
            }
          str=convertLong(buffer,len,value,10,precision,flags);
          break;
        case 'b':
          flags|=FLG_UNSIGNED;
          flags&=~FLG_THOUSAND;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            value=(FXulong)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            value=(FXulong)va_arg(ag,unsigned long);
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            value=(FXulong)va_arg(ag,FXulong);
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            value=(FXulong)(FXushort)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            value=(FXulong)(FXuchar)va_arg(ag,FXuint);
            }
          else{                                         // Whatever size a pointer is
            value=(FXulong)va_arg(ag,FXuval);
            }
          str=convertLong(buffer,len,value,2,precision,flags);
          break;
        case 'o':
          flags|=FLG_UNSIGNED;
          flags&=~FLG_THOUSAND;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            value=(FXulong)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            value=(FXulong)va_arg(ag,unsigned long);
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            value=(FXulong)va_arg(ag,FXulong);
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            value=(FXulong)(FXushort)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            value=(FXulong)(FXuchar)va_arg(ag,FXuint);
            }
          else{                                         // Whatever size a pointer is
            value=(FXulong)va_arg(ag,FXuval);
            }
          str=convertLong(buffer,len,value,8,precision,flags);
          break;
        case 'X':
          flags|=FLG_UPPER;
        case 'x':
          flags|=FLG_UNSIGNED;
          flags&=~FLG_THOUSAND;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            value=(FXulong)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            value=(FXulong)va_arg(ag,unsigned long);
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            value=(FXulong)va_arg(ag,FXulong);
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            value=(FXulong)(FXushort)va_arg(ag,FXuint);
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            value=(FXulong)(FXuchar)va_arg(ag,FXuint);
            }
          else{                                         // Whatever size a pointer is
            value=(FXulong)va_arg(ag,FXuval);
            }
          str=convertLong(buffer,len,value,16,precision,flags);
          break;
        case 'F':
          flags|=FLG_UPPER;
        case 'f':
          flags|=FLG_FRACTION;                          // Fractional notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          str=convertDouble(buffer,len,number,precision,flags);
          break;
        case 'E':
          flags|=FLG_UPPER;
        case 'e':
          flags|=FLG_EXPONENT;                          // Exponential notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          str=convertDouble(buffer,len,number,precision,flags);
          break;
        case 'G':
          flags|=FLG_UPPER;
        case 'g':
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          if(precision<1) precision=1;
          str=convertGeneral(buffer,len,number,precision,flags);
          break;
        case 'c':                                       // Single character
          flags&=~FLG_ZERO;
          if(0<pos) vadvance(ag,args,format,pos);
          buffer[0]=va_arg(ag,FXint);
          str=buffer;
          len=1;
          break;
        case 's':
          flags&=~FLG_ZERO;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          str=va_arg(ag,FXchar*);                       // String value
          if(str){
            len=strlen(str);
            if(0<=precision && precision<len) len=precision;
            }
          else{                                         // NULL string passed
            str="(null)";
            len=6;
            }
          break;
        case 'n':
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          if(modifier==ARG_DEFAULT){                    // 32-bit always
            *va_arg(ag,FXint*)=(FXint)count;
            }
          else if(modifier==ARG_LONG){                  // Whatever size a long is
            *va_arg(ag,long*)=(long)count;
            }
          else if(modifier==ARG_LONGLONG){              // 64-bit always
            *va_arg(ag,FXlong*)=count;
            }
          else if(modifier==ARG_HALF){                  // 16-bit always
            *va_arg(ag,FXshort*)=(FXshort)count;
            }
          else if(modifier==ARG_HALFHALF){              // 8-bit always
            *va_arg(ag,FXchar*)=(FXchar)count;
            }
          else{                                         // Whatever size a pointer is
            *va_arg(ag,FXival*)=(FXival)count;
            }
          continue;                                     // No printout
        case 'p':
          flags&=~FLG_ZERO;
          flags&=~FLG_THOUSAND;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          value=(FXulong)va_arg(ag,FXuval);
          str=convertLong(buffer,len,value,16,precision,flags);
          break;
        default:                                        // Format error
          goto x;
        }

      // Justify to the right
      if(!(flags&FLG_LEFT)){
        if(flags&FLG_ZERO){                     // Pad on left with zeroes
          if(*str=='+' || *str=='-' || *str==' '){
            if(count<length){ *string++=*str++; }
            count++;
            width--;
            len--;
            }
          else if(*str=='0' && (*(str+1)=='x' || *(str+1)=='X')){
            if(count<length){ *string++=*str++; }
            count++;
            if(count<length){ *string++=*str++; }
            count++;
            width-=2;
            len-=2;
            }
          while(width>len){
            if(count<length){ *string++='0'; }
            count++;
            width--;
            }
          }
        else{                                   // Pad on left with spaces
          while(width>len){
            if(count<length){ *string++=' '; }
            count++;
            width--;
            }
          }
        }

      // Output the string str
      for(i=0; i<len; i++){
        if(count<length){ *string++=*str++; }
        count++;
        }

      // Justify to the left
      if(flags&FLG_LEFT){                       // Pad on right always with spaces
        while(width>len){
          if(count<length){ *string++=' '; }
          count++;
          width--;
          }
        }

      // Get format character
      continue;
      }

    // Regular characters just added
nml:if(count<length){ *string++=ch; }
    count++;
    }

  // Last character
  if(count<length){ *string++='\0'; }

  // Done
x:va_end(ag);
  return count;
  }


// Print using format
FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...){
  va_list args;
  va_start(args,format);
  FXint result=__vsnprintf(string,length,format,args);
  va_end(args);
  return result;
  }

}
