/********************************************************************************
*                                                                               *
*                  V a r a r g s   P r i n t f   R o u t i n e s                *
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
#include "fxendian.h"



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
    a value of 6 will be used for floating point conversions, and a value of 1 will be used
    for integer conversions.  For integer conversions, a precision of 0 will print only an
    empty string if the number is 0; otherwise the number will be padded with zeros.
    When thousands grouping is used for integer conversions, don't pad with '0' and ','
    but use the equivalent number of spaces ' ' instead.

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

  - Subtle difference between glibc: does NOT output '\0' at the end, unless
    buffer is large enough.
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
  FLG_DOTSEEN  = 256    // Dot was seen
  };


// Conversion buffer size
enum{CONVERTSIZE=512};

// Maximum precision
enum{MAXPRECISION=100};

// Maximum decimal float digits
enum{MAXDECDIGS=19};

// Maximum hexadecimal float digits
enum{MAXHEXDIGS=14};

// Hexadecimal digits
const FXchar lower_digits[]="0123456789abcdef";
const FXchar upper_digits[]="0123456789ABCDEF";


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


// Declarations
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);
extern FXAPI FXint __vsnprintf(FXchar* string,FXint length,const FXchar* format,va_list args);

/*******************************************************************************/

// 10^x where x=0...19
static FXulong tenToThe[20]={
  FXULONG(1),
  FXULONG(10),
  FXULONG(100),
  FXULONG(1000),
  FXULONG(10000),
  FXULONG(100000),
  FXULONG(1000000),
  FXULONG(10000000),
  FXULONG(100000000),
  FXULONG(1000000000),
  FXULONG(10000000000),
  FXULONG(100000000000),
  FXULONG(1000000000000),
  FXULONG(10000000000000),
  FXULONG(100000000000000),
  FXULONG(1000000000000000),
  FXULONG(10000000000000000),
  FXULONG(100000000000000000),
  FXULONG(1000000000000000000),
  FXULONG(10000000000000000000)
  };


// Convert number to MAXFLTDIGS decimal digits in buffer.
// Return pointer to the converted string and the value of the leading digit.
// For example, decimal=5 means the value of a leading digit d is d * 10^5.
// Two extra bytes are reserved for the NUL and possible carry when rounding.
static FXchar* cvtdec(FXchar digits[],FXdouble value,FXint& decimal){
  volatile union{ FXdouble f; FXulong u; } z={value};
  FXchar *ptr=digits+MAXDECDIGS+2;
  FXlong binex,decex,negex,decor,shift;
  FXulong num,n;

  // Terminate
  *--ptr='\0';

  // Compute decimal point
  if(z.u){

    // Initialize decimal correction
    decor=0;

    // Denormalized numbers need to be normalized first.
    // Calculate the decimal point correction needed to normalize it.
    // We can't use floating point math because FTZ/DAZ flags would
    // cause truncation to zero.
    if(z.u<FXULONG(0x0010000000000000)){
      shift=clz64(z.u)-11;                      // Shift s is leading zeros minus 11
      decor=((shift*77+255)>>8);                // Decimal correction x such that 10^x > 2^s
      z.u*=tenToThe[decor];                     // Multiply by correction factor 10^x
      binex=1;                                  // Binary exponent (no bias applied)
      while(FXULONG(0x001fffffffffffff)<z.u){
        z.u>>=1;                                // Shift down until we have 1.XXXXXXXXXXXXX
        ++binex;                                // Increment exponent to correct for shift
        }
      z.u&=FXULONG(0x000fffffffffffff);         // m = XXXXXXXXXXXXX
      z.u|=(binex<<52);                         // e = binex
      }

    // Get binary exponent
    binex=(z.u>>52)-1023;

    // Compute base 10 exponent from base 2 exponent as follows:
    //
    //   decex = floor(log10(2^binex))
    //         = floor(binex*log10(2))
    //         = floor(binex*0.301029995663981)
    //
    // This may be approximated as:
    //
    //   decex = floor(binex*0.30078125)
    //         = (binex*77)>>8
    //
    // or even better, at no additional cost:
    //
    //   decex = floor(binex*0.301025390625)
    //         = (binex*1233)>>12
    //
    decex=(binex*1233)>>12;

    FXASSERT(-308<=decex && decex<=308);

//fprintf(stderr,"binex: % 4lld  decimal: % 4lld  decor: % 4lld number: % .20lG \n",binex,decex,decor,value);

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
    //
    // The variable 'z.f' is declared as 'volatile', to force compiler
    // to write back to memory in between the two statements:
    //
    //    z.f*=xxx;
    //    z.f*=yyy;
    //
    // Without this volatile declaration, these two statements may be
    // compiled as in optimized mode:
    //
    //    z.f*=(xxx*yyy);
    //
    // Mathematically this would be the same, but computer arithmetic
    // is NOT generally associative.  The optimized (xxx*yyy) may overflow,
    // and if this happens then the result of the conversion will NOT be
    // accurate.
    if(decex>=0){
      if(decex<32){
        z.f*=scaleneg1[decex];
        }
      else{
        z.f*=scaleneg1[decex&31];
        z.f*=scaleneg2[decex>>5];
        }
      }
    else{
      negex=-decex;
      if(negex<32){
        z.f*=scalepos1[negex];
        }
      else{
        z.f*=scalepos1[negex&31];
        z.f*=scalepos2[negex>>5];
        }
      }

    // Adjust decimal point, keep 18 digits only
    if(1.0E19<=z.f){
      z.f*=0.1;
      ++decex;
      }

    // Convert to string at end of buffer
    num=(FXulong)z.f;

    // Unsigned long x < 18446744073709551616
    FXASSERT(num<FXULONG(10000000000000000000));
    FXASSERT(FXULONG(1000000000000000000)<=num);

    // Place the decimal point
    decimal=decex-decor;

    // Generate digits at end of buffer
    while(digits+1<ptr){
      n=num/10;
      *--ptr='0'+(num-n*10);
      num=n;
      }
    }
  else{

    // Power of ten of leading digit
    decimal=0;

    // Generate digits at end of buffer
    while(digits+1<ptr){
      *--ptr='0';
      }
    }

  FXASSERT(digits<=ptr);

  //fprintf(stderr,"digits: %s  decimal: % 4d  number: % .20lG \n",ptr,decimal,value);

  // Return pointer to 1st digit
  return ptr;
  }


// Round numeric string to given digit, adjusting the decimal
// point if a carry-over happened.
// If the entire number is truncated, return empty string.
static FXchar* rndig(FXchar* str,FXint& decimal,FXint dig){
  if(dig<MAXDECDIGS){
    FXchar* dst=str;
    if(0<=dig+1){
      dst=str+dig;
      if('5'<=*dst){
        FXchar* ptr=dst;
        while(str<ptr){
          if(++*--ptr<='9') goto x;
          *ptr='0';
          }
        decimal++;
        *--str='1';
        }
      }
x:  *dst='\0';
    }
  return str;
  }

/*******************************************************************************/

// Format fractional number +ddd.ddddd
static FXchar* fmtfrc(FXchar* buffer,FXint& len,FXdouble number,FXint precision,FXint flags){
  FXchar *ptr=buffer;
  FXchar sign=0;

  // Deal with sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    sign='-';
    }
  else if(flags&FLG_SIGN){
    sign='+';
    }
  else if(flags&FLG_BLANK){
    sign=' ';
    }

  // Handle normal numbers first
  if(Math::fpFinite(number)){
    FXchar digits[MAXDECDIGS+2];
    FXint  decimal;

    // Convert number to digits
    FXchar* p=cvtdec(digits,number,decimal);

    // Round the number (decimal may be negative)
    p=rndig(p,decimal,precision+decimal+1);

    //fprintf(stderr,"number: % 30.20lE  decimal: %4d  precision: %2d  str: %s\n",number,decimal,precision,p);

    // Write sign
    if(sign){ *ptr++=sign; }

    // +ddd.dddddd
    //  ^-- decimal=2, precision=6
    if(0<=decimal){

      // Whip out digits
      while(0<=decimal && *p){
        *ptr++=*p++;
        if(flags&FLG_THOUSAND){
          if(decimal%3==0 && decimal!=0) *ptr++=',';
          }
        --decimal;
        }

      // Zeros
      while(0<=decimal){
        *ptr++='0';
        if(flags&FLG_THOUSAND){
          if(decimal%3==0 && decimal!=0) *ptr++=',';
          }
        --decimal;
        }

      // Decimal point needed
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Whip out fraction digits
      while(0<precision && *p){
        --precision;
        *ptr++=*p++;
        }

      // More zeros
      while(0<precision){
        --precision;
        *ptr++='0';
        }
      }

    // +0.000ddd
    //       ^-- decimal=-4, precision=6
    else{

      // Always digit before decimal point
      *ptr++='0';

      // Decimal point is negative or zero
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Zeros after decimal
      while(decimal<-1 && 0<precision){
        --precision;
        *ptr++='0';
        ++decimal;
        }

      // Whip out fraction digits
      while(0<precision && *p){
        --precision;
        *ptr++=*p++;
        }

      // More zeros
      while(0<precision){
        --precision;
        *ptr++='0';
        }
      }
    }

  // Infinity
  else if(Math::fpInfinite(number)){
    if(sign){ *ptr++=sign; }
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
    if(sign){ *ptr++=sign; }
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

  // Terminate
  *ptr='\0';

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }

/*******************************************************************************/

// Format exponential number +d.dddddE+dd
static FXchar* fmtexp(FXchar* buffer,FXint& len,FXdouble number,FXint precision,FXint flags){
  FXchar *ptr=buffer;
  FXchar sign=0;

  // Deal with sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    sign='-';
    }
  else if(flags&FLG_SIGN){
    sign='+';
    }
  else if(flags&FLG_BLANK){
    sign=' ';
    }

  // Handle normal numbers first
  if(Math::fpFinite(number)){
    FXchar digits[MAXDECDIGS+2];
    FXint  decimal,extra;

    // Convert number to digits
    FXchar* p=cvtdec(digits,number,decimal);

    // In exponent mode, add one before decimal point; add up to
    // two more digits if engineering mode also in effect.
    extra=0;
    if(flags&FLG_THOUSAND){
      extra=(decimal+600)%3;
      }

    // Round the number (extra before decimal point)
    p=rndig(p,decimal,precision+extra+1);

    //fprintf(stderr,"number: % 30.20lE  decimal: %4d  precision: %2d  extra: %2d str: %s\n",number,decimal,precision,extra,p);

    // Up to 3 digits before decimal
    if(flags&FLG_THOUSAND){

      // Extra digits before decimal
      extra=(decimal+600)%3;

      // Write sign
      if(sign){
        if(flags&FLG_BLANK){
          if(extra<2){ *ptr++=' '; }
          if(extra<1){ *ptr++=' '; }
          }
        *ptr++=sign;
        }

      // One digit before decimal point
      *ptr++=*p++;

      // Extra digits
      while(extra && *p){
        --decimal;
        --extra;
        *ptr++=*p++;
        }

      // Extra zeroes
      while(extra){
        --decimal;
        --extra;
        *ptr++='0';
        }
      }

    // One digit before decimal
    else{

      // Write sign
      if(sign){ *ptr++=sign; }

      // One digit before decimal point
      *ptr++=*p++;
      }

    // Decimal point needed
    if((0<precision) || (flags&FLG_ALTER)){
      *ptr++='.';
      }

    // Whip out fraction digits
    while(0<precision && *p){
      --precision;
      *ptr++=*p++;
      }

    // More zeros
    while(0<precision){
      --precision;
      *ptr++='0';
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
        *ptr++=(decimal/10)%10+'0';
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

  // Infinity
  else if(Math::fpInfinite(number)){
    if(sign){ *ptr++=sign; }
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
    if(sign){ *ptr++=sign; }
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

  // Terminate
  *ptr='\0';

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }

/*******************************************************************************/

// Format general number +d.dddE+dd or ddd.dd
static FXchar* fmtgen(FXchar* buffer,FXint& len,FXdouble number,FXint precision,FXint flags){
  FXchar *ptr=buffer;
  FXchar sign=0;

  // Deal with sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    sign='-';
    }
  else if(flags&FLG_SIGN){
    sign='+';
    }
  else if(flags&FLG_BLANK){
    sign=' ';
    }

  // Handle normal numbers first
  if(Math::fpFinite(number)){
    FXchar digits[MAXDECDIGS+2];
    FXint  decimal,expo;

    // Convert number to digits
    FXchar* p=cvtdec(digits,number,decimal);

    //fprintf(stderr,"digits: %s  decimal: % 4d  number: % .20lG \n",p,decimal,number);

    // Round the number (no additional precision)
    p=rndig(p,decimal,precision);

    //fprintf(stderr,"number: % 30.20lE  decimal: %4d  prec: %2d  precision: %2d str: %s\n",number,decimal,precision,precision,p);

    // Switch exponential mode
    expo=(precision<=decimal) || (decimal<-4);

    // Eliminate trailing zeroes; not done for alternate mode
    if(!(flags&FLG_ALTER)){
      if(precision>MAXDECDIGS) precision=MAXDECDIGS;
      while(0<precision && p[precision-1]=='0') --precision;
      }

    // Write sign
    if(sign){ *ptr++=sign; }

    // Exponential mode
    if(expo){

      // One digit before decimal point; don't adjust decimal
      --precision;
      *ptr++=*p++;

      // One or two more before decimal point
      if(flags&FLG_THOUSAND){
        while((decimal+600)%3 && *p){
          --precision;
          --decimal;
          *ptr++=*p++;
          }
        while((decimal+600)%3){
          --precision;
          --decimal;
          *ptr++='0';
          }
        }

      // Decimal point needed
      if((0<precision) || (flags&FLG_ALTER)){
        *ptr++='.';
        }

      // Remaining fraction, if any
      while(0<precision && *p){
        --precision;
        *ptr++=*p++;
        }

      // Zeros
      while(0<precision){
        --precision;
        *ptr++='0';
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
          *ptr++=(decimal/10)%10+'0';
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

      // Normal notation +dddd.dd
      //                  ^-- decimal=3, precision=6
      if(0<=decimal){

        // Decimal point is positive
        while(0<=decimal && *p){
          --precision;
          *ptr++=*p++;
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          --decimal;
          }

        // Zeros
        while(0<=decimal){
          *ptr++='0';
          if(flags&FLG_THOUSAND){
            if(decimal%3==0 && decimal!=0) *ptr++=',';
            }
          --decimal;
          }

        // Decimal point needed
        if((0<precision) || (flags&FLG_ALTER)){
          *ptr++='.';
          }

        // Append more digits until we get precision
        while(0<precision && *p){
          --precision;
          *ptr++=*p++;
          }

        // More zeros
        while(0<precision){
          --precision;
          *ptr++='0';
          }
        }

      // Fractional notation +0.000dddddd
      //                           ^-- decimal=-4, precision=6
      else{

        // Always digit before decimal point
        *ptr++='0';

        // Decimal point only if followed by at least one digit
        if(decimal<0 || 0<precision){
          *ptr++='.';
          }

        // Output a bunch of zeroes preceeded by '0.'
        while(decimal<-1){
          ++decimal;
          *ptr++='0';
          }

        // Generate precision digits
        while(0<precision && *p){
          --precision;
          *ptr++=*p++;
          }

        // More zeros
        while(0<precision){
          --precision;
          *ptr++='0';
          }
        }
      }
    }

  // Infinity
  else if(Math::fpInfinite(number)){
    if(sign){ *ptr++=sign; }
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
    if(sign){ *ptr++=sign; }
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

  // Terminate
  *ptr='\0';

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }

/*******************************************************************************/

// Convert number to hex string in buffer
static FXchar* cvthex(FXchar* digits,FXdouble value,FXint& decimal,FXint precision,FXint flags){
  const FXchar* hexdigits=(flags&FLG_UPPER)?upper_digits:lower_digits;
  const FXlong HEXRND=FXLONG(0x0008000000000000);
  const FXlong HEXMSK=FXLONG(0xFFF0000000000000);
  FXulong mantissa=Math::fpMantissa(value);
  FXchar *ptr=digits+MAXHEXDIGS+1;

  // binary point location after 1st digit
  decimal=Math::fpExponent(value);

  // Round to precision nibbles, and zero the rest.
  // The leading digit is at most 1 (its the hidden bit), so no
  // additional digits can be generated due to carry propagation...
  if(precision<MAXHEXDIGS){
    mantissa+=HEXRND>>(precision<<2);
    mantissa&=HEXMSK>>(precision<<2);
    }

  // Terminate
  *--ptr='\0';

  // Generate digits at end of buffer
  while(digits<ptr){
    *--ptr=hexdigits[mantissa&15];
    mantissa>>=4;
    }

  FXASSERT(digits<=ptr);

  // Return pointer to 1st digit
  return ptr;
  }


// Format hexadecimal number 0x1.hhhhhhhhhhhhhp+dddd
static FXchar* fmthex(FXchar* buffer,FXint& len,FXdouble number,FXint precision,FXint flags){
  FXchar *ptr=buffer;
  FXchar sign=0;

  // Deal with sign
  if(Math::fpSign(number)){
    number=Math::fabs(number);
    sign='-';
    }
  else if(flags&FLG_SIGN){
    sign='+';
    }
  else if(flags&FLG_BLANK){
    sign=' ';
    }

  // Handle normal numbers first
  if(Math::fpFinite(number)){
    FXchar digits[MAXHEXDIGS+1];
    FXint  decimal;

    // Convert with 1 extra hexdigit before decimal point
    FXchar* p=cvthex(digits,number,decimal,precision,flags);

    //fprintf(stderr,"number: % 30.20lE  decimal: %4d  precision: %2d  str: %s\n",number,decimal,precision,p);

    // Eliminate trailing zeroes; not done for alternate mode
    if(precision<0){
      precision=MAXHEXDIGS-1;
      while(0<precision && p[precision]=='0') --precision;
      }

    // Write sign
    if(sign){ *ptr++=sign; }

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

    // Decimal point needed
    if((0<precision) || (flags&FLG_ALTER)){
      *ptr++='.';
      }

    // Copy fraction
    while(0<precision && *p){
      --precision;
      *ptr++=*p++;
      }

    // Zeros
    while(0<precision){
      --precision;
      *ptr++='0';
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
            *ptr++=(decimal/1000)+'0';
            }
          *ptr++=(decimal/100)%10+'0';
          }
        *ptr++=(decimal/10)%10+'0';
        }
      *ptr++=decimal%10+'0';
      }
    else{
      *ptr++='+';
      *ptr++='0';
      }
    }

  // Infinity
  else if(Math::fpInfinite(number)){
    if(sign){ *ptr++=sign; }
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
    if(sign){ *ptr++=sign; }
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

  // Terminate
  *ptr='\0';

  // Set length
  len=ptr-buffer;

  // Done
  return buffer;
  }

/*******************************************************************************/

// Convert long value
static FXchar* fmtlng(FXchar* buffer,FXint& len,FXlong value,FXint base,FXint precision,FXint flags){
  FXchar *end=buffer+CONVERTSIZE-1;
  FXchar *ptr=end;

  // Terminate
  *ptr='\0';

  // Print only when at least 1 digit or when value non-zero
  if(0<precision || value){
    const FXchar *digits=(flags&FLG_UPPER)?upper_digits:lower_digits;
    FXulong number=value;
    FXchar sign=0;
    FXint digs=0;
    FXulong n;

    // Deal with sign
    if(!(flags&FLG_UNSIGNED)){
      if(value<0){
        number=-value;
        sign='-';
        }
      else if(flags&FLG_SIGN){
        sign='+';
        }
      else if(flags&FLG_BLANK){
        sign=' ';
        }
      }

    // Output decimal with thousands separator
    if(flags&FLG_THOUSAND){
      do{
        ++digs;
        --precision;
        n=number/10;
        *--ptr=digits[number-n*10];
        number=n;
        if(digs%3==0 && number) *--ptr=',';
        }
      while(number);
      while(0<precision){       // Pad with spaces
        if(digs%3==0 && precision) *--ptr=' ';
        ++digs;
        --precision;
        *--ptr=' ';
        }
      }

    // Output with arbitrary base
    else{
      do{
        --precision;
        n=number/base;
        *--ptr=digits[number-n*base];
        number=n;
        }
      while(number);
      while(0<precision){       // Pad with zeros if needed
        --precision;
        *--ptr='0';
        }
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
    }

  // Return length
  len=end-ptr;
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
    if(ch=='%'){                                        // Format characters

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
          if(precision<0) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,10,precision,flags);
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
          if(precision<0) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,10,precision,flags);
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
          if(precision<0) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,2,precision,flags);
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
          if(precision<0) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,8,precision,flags);
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
          if(precision<0) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,16,precision,flags);
          break;
        case 'F':
          flags|=FLG_UPPER;
        case 'f':                                       // Fractional notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtfrc(buffer,len,number,precision,flags);
          break;
        case 'E':
          flags|=FLG_UPPER;
        case 'e':                                       // Exponential notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtexp(buffer,len,number,precision,flags);
          break;
        case 'G':
          flags|=FLG_UPPER;
        case 'g':                                       // General notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision<0) precision=6;
          if(precision<1) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtgen(buffer,len,number,precision,flags);
          break;
        case 'A':
          flags|=FLG_UPPER;
        case 'a':                                       // Hexadecimal notation
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          number=va_arg(ag,FXdouble);
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmthex(buffer,len,number,precision,flags);
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
            if(precision<len && 0<=precision) len=precision;
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
          flags|=FLG_ALTER;
          if(0<pos) vadvance(ag,args,format,pos);       // Advance ag to position
          value=(FXulong)va_arg(ag,FXuval);
          if(precision<1) precision=1;
          if(precision>MAXPRECISION) precision=MAXPRECISION;
          str=fmtlng(buffer,len,value,16,precision,flags);
          break;
        default:                                        // Format error
          goto x;
        }

      // Justify to the right
      if(!(flags&FLG_LEFT)){
        if(flags&FLG_ZERO){                             // Pad on left with zeroes
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
        else{                                           // Pad on left with spaces
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
      if(flags&FLG_LEFT){                               // Pad on right always with spaces
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
