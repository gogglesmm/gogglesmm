/********************************************************************************
*                                                                               *
*                  V a r a r g s   P r i n t f   R o u t i n e s                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2020 by Jeroen van der Zijp.   All Rights Reserved.        *
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
     '#'        Alternate form (prefix '0' for octal, '0x' for hex, '0b' for binary, and
                always emit decimal point for floats.
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
     'a', 'A'   Hexadecimal floating point output. If no precision specified,
                print the shortest number of digits (at most 13 after the hexadecimal
                point), otherwise, round the last printed digit.  Use upper- or lower-case,
                depending on whether A or a was used.

  - Printing special floating point values:

     nan,       If floating point conversion specifier is lower case ('f', 'g',
     inf        or 'e'), if needed, preceded by a sign.

     NAN,       If floating point conversion specifier is lower case ('F', 'G',
     INF        or 'E'), if needed, preceded by a sign.

    We may have to change inf -> Infinity or INFINITY instead [have to find spec
    for this change].

  - If the range of positional parameters in a format string is not contiguous,
    i.e. if a positional parameter is skipped (e.g. "%3$d%1$d"), then the missing
    one is assumed to be of type "int".
    Its therefore best if no parameters are skipped; referencing a single parameter
    multiple times however, is no problem!!

  - A sticky situation developed with compilers optimizing statements like:

        number*=xxx;
        number*=yyy;

    to:

        number*=(xxx*yyy);

    This causes overflow of the (xxx*yyy) expression, and thus incorrect outputs.
    The problem is prevented by declaring 'number' as 'volatile' which causes the
    value to be written to memory after assignment, and thus execute these two
    statements as written.

  - Subtle difference between glibc: does NOT output '\0' at the end, unless
    buffer is large enough.
*/

#if defined(_WIN32)
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
  FLG_HEXADEC  = 1024,  // Hexadecimal notation
  FLG_DOTSEEN  = 2048   // Dot was seen
  };


// Conversion buffer
const FXint CONVERTSIZE=512;

// Hexadecimal digits
const FXchar lower_digits[]="0123456789abcdef";
const FXchar upper_digits[]="0123456789ABCDEF";

/*******************************************************************************/

static FXdouble scalepos1[32] = {
  1.0E+18, 1.0E+19, 1.0E+20, 1.0E+21, 1.0E+22, 1.0E+23, 1.0E+24, 1.0E+25,
  1.0E+26, 1.0E+27, 1.0E+28, 1.0E+29, 1.0E+30, 1.0E+31, 1.0E+32, 1.0E+33,
  1.0E+34, 1.0E+35, 1.0E+36, 1.0E+37, 1.0E+38, 1.0E+39, 1.0E+40, 1.0E+41,
  1.0E+42, 1.0E+43, 1.0E+44, 1.0E+45, 1.0E+46, 1.0E+47, 1.0E+48, 1.0E+49
  };

static FXdouble scaleneg1[32] = {
  1.0E+18, 1.0E+17, 1.0E+16, 1.0E+15, 1.0E+14, 1.0E+13, 1.0E+12, 1.0E+11,
  1.0E+10, 1.0E+09, 1.0E+08, 1.0E+07, 1.0E+06, 1.0E+05, 1.0E+04, 1.0E+03,
  1.0E+02, 1.0E+01, 1.0E+00, 1.0E-01, 1.0E-02, 1.0E-03, 1.0E-04, 1.0E-05,
  1.0E-06, 1.0E-07, 1.0E-08, 1.0E-09, 1.0E-10, 1.0E-11, 1.0E-12, 1.0E-13
  };

static FXdouble scalepos2[10] = {
  1.0E+00,  1.0E+32,  1.0E+64,  1.0E+96,  1.0E+128,
  1.0E+160, 1.0E+192, 1.0E+224, 1.0E+256, 1.0E+288
  };

static FXdouble scaleneg2[10] = {
  1.0E-00,  1.0E-32,  1.0E-64,  1.0E-96,  1.0E-128,
  1.0E-160, 1.0E-192, 1.0E-224, 1.0E-256, 1.0E-288
  };


// Convert number to string in buffer
static FXchar* cvt(FXchar* buffer,FXuval size,FXdouble value,FXint& decimal,FXint precision,FXuint flags){
  volatile FXdouble number=value;
  FXchar *end=buffer+size;
  FXchar *dst=buffer;
  FXchar *src=end;
  FXchar *p;
  FXint digits,binex,decex,negex,dex,x;
  FXulong num,n;

  // Compute decimal point
  if(__likely(number)){

    // Get binary exponent
    binex=Math::fpExponent(number);

    // Compute base 10 exponent from base 2 exponent as follows:
    //
    //   decex = log10(2^binex) = binex*log10(2) = binex*0.301029995663981
    //
    // This may be approximated as:
    //
    //   decex = binex*0.30078125 = (binex*77)>>8
    //
    decex=(binex*77)>>8;

    // Ought to be in this range
    FXASSERT(-308<=decex && decex<=308);

    // Bring mantissa in range for conversion to a 64-bit long.
    // For normalized floating point numbers the mantissa is always
    // [0.5...1), thus we scale by:
    //
    //   1.0E18 * 10^-decex       decex>=0
    //
    // or:
    //
    //   1.0E18 * 10^+decex       decex<0
    //
    // This leaves a number 0.5E18...1.0E18, which fits in 64-bit long.
    // If the floating point number was denormalized, apply an additional
    // scale after the one above.  We scale the number an arbitrary number
    // of times by 10, until its in the expected range 0.5E18...1.0E18, and
    // adjust the exponent along the way.
    //
    // The variable 'number' is declared as 'volatile', to force compiler
    // to write back to memory in between the two statements:
    //
    //    number*=xxx;
    //    number*=yyy;
    //
    // Without this volatile declaration, these two statements may be
    // compiled as in optimized mode:
    //
    //    number*=(xxx*yyy);
    //
    // Mathematically this would be the same, but computer arithmetic
    // is NOT generally associative.  The optimized (xxx*yyy) may overflow,
    // and if this happens then the result of the conversion will NOT be
    // accurate.
    if(decex>=0){
      if(decex<32){
        number*=scaleneg1[decex];
        }
      else{
        number*=scaleneg1[decex&31];
        number*=scaleneg2[decex>>5];
        }
      }
    else{
      negex=-decex;
      if(negex<32){
        number*=scalepos1[negex];
        }
      else{
        number*=scalepos1[negex&31];
        number*=scalepos2[negex>>5];
        }
      }

    // Denormalized number hack
    if(binex==-1023){
      dex=(Math::fpExponent(number)*77)>>8;
      number*=Math::pow10i(18-dex);
      decex+=dex-18;
      }

    // Adjust decimal point, keep 18 digits only
    if(1.0E19<=number){
      number*=0.1;
      decex++;
      }

    // Convert to string at end of buffer
    num=(FXulong)number;

    FXASSERT(num<FXULONG(10000000000000000000));

    // Generate digits at end of buffer, starting with last
    do{
      n=num/10;
      *--src='0'+(num-n*10);
      num=n;
      }
    while(num);

    // Decimal point location after 1st digit
    decimal=decex+1;
    }
  else{

    // Just one lone '0'
    *--src='0';

    // Assume decimal point location 0.
    decimal=1;
    }

  // Digits to be returned
  digits=precision;

  // In exponent mode, add one before decimal point; add up to
  // two more digits if engineering mode also in effect.
  if(flags&FLG_EXPONENT){
    if(flags&FLG_THOUSAND){
      x=(decimal+600-1)%3;
      digits+=x;
      }
    digits++;
    }

  // In fraction mode, add digits before the decimal point. If the
  // decimal point is negative, there will be fewer digits generated,
  // and the decimal point will be adjusted accordingly.
  if(flags&FLG_FRACTION){
    if(digits<-decimal) decimal=-digits;
    digits+=decimal;
    }

  // Don't exceed buffer space
  digits=Math::imin(digits,size-1);

  // Move string to begin of buffer
  while(0<digits && src<end){
    *dst++=*src++;
    digits--;
    }

  // Rounding result if using fewer digits than generated
  if(src<end && '5'<=*src){
    p=dst;
    while(buffer<p){
      if(++*--p<='9') goto n;
      *p='0';
      }
    if(buffer==p){
      *buffer='1';
      if(flags&FLG_FRACTION){
        if(buffer<dst) *dst='0';        // Extra zero at the end for normal mode
        dst++;                          // Added a digit
        }
      decimal++;
      digits--;
      }
    }

  // Need more digits
n:while(digits>0){
    *dst++='0';
    digits--;
    }

  FXASSERT(dst<end);

  // Terminate
  *dst='\0';

  return buffer;
  }

/*******************************************************************************/

// Convert number to hex string in buffer
static FXchar* cvthex(FXchar* buffer,FXuval size,FXdouble value,FXint& decimal,FXint precision,FXuint flags){
  const FXchar* hexdigits=(flags&FLG_UPPER)?upper_digits:lower_digits;
  const FXlong HEXRND=FXLONG(0x0080000000000000);
  const FXlong HEXMSK=FXLONG(0xFF00000000000000);
  FXulong mantissa=Math::fpMantissa(value);
  FXint exponent=Math::fpExponent(value);
  FXint digits=Math::imin(precision,size-2);
  FXchar *dst=buffer;

  // Zero mantissa means exponent should be zero also
  if(mantissa==0) exponent=0;

  // Decimal point location after 1st digit
  decimal=exponent+1;

  // Round to precision nibbles, and zero the rest
  if(precision<14){
    mantissa+=HEXRND>>(precision<<2);
    mantissa&=HEXMSK>>(precision<<2);
    }

  // Whip out digits
  while(digits>0){
    *dst++=hexdigits[(mantissa>>52)&15];
    mantissa<<=4;
    digits--;
    }

  // Terminate
  *dst='\0';

  return buffer;
  }

/*******************************************************************************/

// Convert double number; precision is digits after decimal point
static FXchar* convertDouble(FXchar* buffer,FXint& len,FXdouble number,FXint precision,FXint flags){
  FXchar *ptr=buffer;
  FXchar *p;
  FXchar digits[512];
  FXint  decimal,pr;
  FXbool expo;

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
    if(flags&FLG_UPPER){
      *ptr++='I';
      *ptr++='N';
      *ptr++='F';
      }
    else{
      *ptr++='i';
      *ptr++='n';
      *ptr++='f';
      }
    }

  // NaN
  else if(Math::fpNan(number)){
    if(flags&FLG_UPPER){
      *ptr++='N';
      *ptr++='A';
      *ptr++='N';
      }
    else{
      *ptr++='n';
      *ptr++='a';
      *ptr++='n';
      }
    }

  // Hexadecimal float mode
  else if(flags&FLG_HEXADEC){

    pr=precision;
    if(precision<0) precision=13;
    precision++;

    // Convert with 1 extra hexdigit before decimal point
    p=cvthex(digits,ARRAYNUMBER(digits),number,decimal,precision,flags);

    // Eliminate trailing zeroes; not done for alternate mode
    if(pr<0 && !(flags&FLG_ALTER)){
      while(0<precision && p[precision-1]=='0') precision--;
      }

    // Prefix with 0x
    *ptr++='0';
    if(flags&FLG_UPPER){
      *ptr++='X';
      }
    else{
      *ptr++='x';
      }

    // One digit before decimal point
    *ptr++=*p++;
    decimal--;
    precision--;

    // Decimal point needed
    if((0<precision) || (flags&FLG_ALTER)){
      *ptr++='.';
      }

    // Copy fraction
    while(0<precision){
      *ptr++=*p++;
      precision--;
      }

    // Exponent
    *ptr++=(flags&FLG_UPPER)?'P':'p';
    if(Math::fpBits(number)){

      // Negative exponent
      if(decimal<0){
        decimal=-decimal;
        *ptr++='-';
        }
      else{
        *ptr++='+';
        }

      // Exponent is 1..4 digits
      if(10<=decimal){
        if(100<=decimal){
          if(1000<=decimal){
            *ptr++='0'+(decimal/1000);
            decimal%=1000;
            }
          *ptr++='0'+(decimal/100);
          decimal%=100;
          }
        *ptr++='0'+(decimal/10);
        decimal%=10;
        }
      *ptr++='0'+decimal;
      }
    else{
      *ptr++='+';
      *ptr++='0';
      }
    }

  // Force exponential mode
  else if(flags&FLG_EXPONENT){

    if(precision<0) precision=6;

    // Convert with 1 extra digit before decimal point
    p=cvt(digits,ARRAYNUMBER(digits),number,decimal,precision,flags);

    // One digit before decimal point
    *ptr++=*p++;
    decimal--;

    // One or two more before decimal point
    if(flags&FLG_THOUSAND){
      while((decimal+600)%3){
        *ptr++=*p++;
        decimal--;
        }
      }

    // Decimal point needed
    if((0<precision) || (flags&FLG_ALTER)){
      *ptr++='.';
      }

    // Copy fraction
    while(*p){
      *ptr++=*p++;
      precision--;
      }

    // Exponent
    *ptr++=(flags&FLG_UPPER)?'E':'e';
    if(Math::fpBits(number)){

      // Negative exponent
      if(decimal<0){
        decimal=-decimal;
        *ptr++='-';
        }
      else{
        *ptr++='+';
        }

      // Large exponent is 3 digits
      if(99<decimal){
        *ptr++=(decimal/100)+'0';
        *ptr++=((decimal/10))%10+'0';
        *ptr++=(decimal%10)+'0';
        }

      // Normal exponent is 2 digit
      else{
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

  // Forced fractional mode
  else if(flags&FLG_FRACTION){

    if(precision<0) precision=6;

    // Convert with precision decimals after decimal point
    p=cvt(digits,ARRAYNUMBER(digits),number,decimal,precision,flags);

    // Fractional notation +0.000ddd
    if(decimal<=0){

      // Always digit before decimal point
      *ptr++='0';

      // Decimal point is negative or zero
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Output a bunch of zeroes preceeded by '0.'
      while(decimal<0){
        *ptr++='0';
        decimal++;
        }

      // Copy fraction
      while(*p){
        *ptr++=*p++;
        }
      }

    // Normal notation +ddd.ddd
    else{

      // Decimal point is positive
      while(0<decimal){
        *ptr++=*p++;
        decimal--;
        if(flags&FLG_THOUSAND){
          if(decimal%3==0 && decimal!=0) *ptr++=',';
          }
        }

      // Decimal point needed
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Copy fraction
      while(*p){
        *ptr++=*p++;
        }
      }
    }

  // General mode
  else{

    if(precision<0) precision=6;
    if(precision<1) precision=1;

    // Convert with precision decimals
    p=cvt(digits,ARRAYNUMBER(digits),number,decimal,precision,flags);

    // Switch exponential mode
    expo=(precision<decimal) || (decimal<-3);

    // Eliminate trailing zeroes; not done for alternate mode
    if(!(flags&FLG_ALTER)){
      while(0<precision && p[precision-1]=='0') precision--;
      }

    // Exponential mode
    if(expo){

      // One digit before decimal point
      *ptr++=*p++;
      decimal--;
      precision--;

      // One or two more before decimal point
      if(flags&FLG_THOUSAND){
        while((decimal+600)%3){
          *ptr++=*p++;
          decimal--;
          precision--;
          }
        }

      // Decimal point needed
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Remaining fraction, if any
      while(0<precision){
        *ptr++=*p++;
        precision--;
        }

      // Exponent
      *ptr++=(flags&FLG_UPPER)?'E':'e';
      if(Math::fpBits(number)){

        // Negative exponent
        if(decimal<0){
          decimal=-decimal;
          *ptr++='-';
          }
        else{
          *ptr++='+';
          }

        // Large exponent is 3 digits
        if(99<decimal){
          *ptr++=(decimal/100)+'0';
          *ptr++=((decimal/10))%10+'0';
          *ptr++=(decimal%10)+'0';
          }

        // Normal exponent is 2 digit
        else{
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

    // Fraction mode
    else{

      // Fractional notation +0.000ddd
      if(decimal<=0){

        // Always digit before decimal point
        *ptr++='0';

        // Decimal point only if followed by at least one digit
        if(decimal<0 || 0<precision){
          *ptr++='.';
          }

        // Output a bunch of zeroes preceeded by '0.'
        while(decimal<0){
          *ptr++='0';
          decimal++;
          }

        // Generate precision digits
        while(0<precision){
          *ptr++=*p++;
          precision--;
          }
        }

      // Integral notation +ddd000.
      else if(precision<=decimal){

        // Generate precision digits
        while(0<precision){
          *ptr++=*p++;
          decimal--;
          precision--;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }
        while(0<decimal){
          *ptr++='0';
          decimal--;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }

        // End with decimal point if alternate mode
        if(flags&FLG_ALTER) *ptr++='.';
        }

      // Normal notation +ddd.ddd
      else{

        // Decimal point is positive
        while(0<decimal){
          *ptr++=*p++;
          decimal--;
          precision--;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          }

        // Output decimal point
        *ptr++='.';

        // Append more digits until we get precision
        while(0<precision){
          *ptr++=*p++;
          precision--;
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
static FXchar* convertLong(FXchar* buffer,FXint& len,FXlong value,FXuint base,FXint precision,FXuint flags){
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
    precision--;
    }
  while(number);

  // Pad to precision
  while(0<precision){
    *--ptr='0';
    --precision;
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
  len=(FXint)(end-ptr);
  return ptr;
  }

/*******************************************************************************/

// Advance ag from args to position before pos
void vadvance(va_list& ag,va_list args,const FXchar* format,FXint pos){
  FXint ch,modifier,val,v;
  const FXchar* fmt;
  FXint cur=1;
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
          case 'A':
          case 'a':
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
  FXchar buffer[CONVERTSIZE+2];
  const FXchar *fmt=format;
  const FXchar *str;
  FXdouble number;
  FXlong value;
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
          str=convertDouble(buffer,len,number,precision,flags);
          break;
        case 'E':
          flags|=FLG_UPPER;
        case 'e':
          flags|=FLG_EXPONENT;                          // Exponential notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          str=convertDouble(buffer,len,number,precision,flags);
          break;
        case 'G':
          flags|=FLG_UPPER;
        case 'g':
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          str=convertDouble(buffer,len,number,precision,flags);
          break;
        case 'A':
          flags|=FLG_UPPER;
        case 'a':
          flags|=FLG_HEXADEC;                           // Hexadecimal notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          str=convertDouble(buffer,len,number,precision,flags);
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

      // Next character
      continue;
      }

    // Regular characters just added
nml:if(count<length){ *string++=ch; }
    count++;
    }

  // Last character
x:if(count<length){ *string++='\0'; }

  // Done
  va_end(ag);
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
