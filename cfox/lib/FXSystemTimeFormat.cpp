/********************************************************************************
*                                                                               *
*                   C o n v e r t   T i m e   T o   S t r i n g                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXElement.h"
#include "FXString.h"
#include "FXSystem.h"


/*
  Notes:
  - Convert FXSystem::Time to a string, similar to UNIX strftime().

  - Format string may contain the following special conversion specifiers:

      %%        A literal '%' character.
      %n        A newline character.
      %t        A tab character.

      %E        Use alternative format.
      %O        Use alternative format.

      %C        The century number (year/100) as a 2-digit integer.

      %Y        The year as a decimal number including the century (range 0000-9999).
      %y        The year as a decimal number without a century (range 00 to 99).
      %G        The ISO 8601 week-based year with century (range 0000-9999).
      %g        ISO 8601 week-based year without century (range 00 to 99).

      %b        Abbreviated month name.
      %h        Abbreviated month name.
      %B        Full month name (in locale).
      %m        The month as a decimal number (range 01 to 12).

      %a        Abbreviated day of the week.
      %A        Full name of the day of the week.
      %w        The day of the week as a decimal, range 0 to 6, Sunday being 0.
      %u        The day of the week as a decimal, range 1 to 7, Monday being 1.

      %U        The week number of the current year as a decimal number, range 00 to 53, starting with
                the first Sunday as the first day of week 01.
      %W        The week number of the current year as a decimal number, range 00 to 53, starting with
                the first Monday as the first day of week 01.
      %V        The ISO 8601 week number (where week 1 is the first week that has at least 4 days in the new
                year) of the current year as a decimal number (range 01 to 53).

      %d        Day of the month as a decimal number (range 01 to 31).
      %e        Day of the month as a decimal number (range 1 to 31), with single digits preceded by a blank.

      %j        The day of the year as a decimal number (range 001 to 366).

      %p        Either "AM" or "PM", depending on time value.
      %P        Lowercase: "am" or "pm", depending on time value.

      %H        The hour using a 24-hour clock (range 00 to 23).
      %k        The hour using a 24-hour clock (range 0 to 23), with single digits preceded by a blank.

      %I        The hour using a 12-hour clock (range 01 to 12).
      %l        The hour using a 12-hour clock (range 1 to 12), with single digits preceded by a blank.

      %M        The minute as a decimal number (range 00 to 59).

      %S        The second as a decimal number (range 00 to 60). The range is up to 60 to allow for
                occasional leap seconds.

      %s        The number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).
                DISCREPANCY between FOX and standard library version: FOX does NOT add offset
                to local time when interpreting broken-down time "FXSystem::Time"; thus passing
                1/1/1970 00:00:00 in "FXSystem::Time", FOX version will always print "0".
                Its more consistent with the actual printout of the other flags.

      %c        Preferred date and time representation (in locale)

      %D        Equivalent to %m/%d/%y.

      %F        Equivalent to %Y-%m-%d  (the ISO 8601 date format).

      %r        The time in a.m. or p.m. notation (%I:%M:%S %p) (in locale)
      %R        The time in 24-hour notation (%H:%M).
      %T        The time in 24-hour notation (%H:%M:%S).

      %x        The preferred date representation without the time (%b %a %d) (in locale).

      %X        The preferred time representation without the date (%k:%M:%S) (in locale).

      %z        The +hhmm  or -hhmm  numeric timezone (that is, the hour and minute offset from UTC).

      %Z        The timezone name or abbreviation.

  - FOX extensions:

      %fm       Fractional seconds: milli-seconds (000-999).

      %fu       Fractional seconds: micro-seconds (000000-999999).

      %fn       Fractional seconds: nano-seconds (000000000-999999999).

*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Furnish our own version
extern FXAPI FXint __snprintf(FXchar* string,FXint length,const FXchar* format,...);


// Short weekdays
static const FXchar *const sweekdays[7]={
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };

// Long weekdays
static const FXchar *const weekdays[7]={
  "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
  };

// Short months
static const FXchar *const smonths[12]={
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

// Long months
static const FXchar *const months[12]={
  "January", "February", "March",     "April",   "May",      "June",
  "July",    "August",   "September", "October", "November", "December"
  };

// am/pm
static const FXchar *const ampm[2]={
  "am", "pm"
  };

// AM/PM
static const FXchar *const AMPM[2]={
  "AM", "PM"
  };


// Generate digs worth of digits from number x
static FXchar* intoa(FXchar* dest,FXuint x,FXuint digs,FXchar pad){
  FXchar *ptr=dest+digs;
  *ptr='\0';
  while(dest<ptr){
    *--ptr=(x%10)+'0';
    x/=10;
    if(x==0) break;
    }
  while(dest<ptr){
    *--ptr=pad;
    }
  return ptr;
  }


// Is year a leap-year
static FXint is_leap(FXint year){
  return !(year%4) && ((year%100) || !(year%400));
  }


// ISO 8601 week number, first week with >= 4 days
static FXint iso_week(const FXSystem::Time& st){
  FXint week,dec31,jan1;

  // Week number starting 1st monday (hint: break at thursday!)
  week=(st.yday+6-(st.wday+6)%7+3)/7;

  // Week belongs to previous year; figure out if that was a 52-week
  // year or a 53-week year: if 31 December of previous year is a Thursday,
  // or Friday of a leap year, then the previous year has 53 weeks.
  if(week==0){
    week=52;
    dec31=(7+st.wday-st.yday)%7;        // Weekday before jan1
    if(dec31==4 || (dec31==5 && is_leap(st.year%400-1))) week++;
    }

  // Week belongs potentially belongs to the next year; figure out if
  // that is the case: if 1 January is not a Thursday, and not a Wednesday
  // of a leap year, then this year had only 52 weeks, and the week belongs
  // to the next year.
  else if(week==53){
    jan1=(371+st.wday-st.yday+1)%7;     // Weekday on jan1
    if(jan1!=4 && (jan1!=3 || !is_leap(st.year))) week=1;
    }
  return week;
  }


// ISO 8601 year, i.e. year in which week number falls
static FXint iso_year(const FXSystem::Time& st){
  FXint year=st.year;
  if(st.yday<3 && iso_week(st)!=1) year--;
  else if(st.yday>360 && iso_week(st)==1) year++;
  return year;
  }


// Format system time to string, generating up to length characters of the result.
// Return number of characters that would have been needed to store the full result.
static FXint systemTimeFormatRecursive(const FXSystem::Time& st,FXchar* string,FXint length,const FXchar* format){
  const FXchar* src;
  FXchar buf[32];
  FXint result,ch,val,n;
  FXlong sec;
  FXchar pad;

  // Process format string
  result=0;
  while((ch=*format++)!='\0'){

    // Check for format-characters
    if(ch=='%'){

      // Get next format character
nxt:  ch=*format++;
      pad='0';

      // Parse format specifier
      switch(ch){
      case '%':                 // Literal '%' character
        goto nml;
      case 'n':                 // Newline character
        ch='\n';
        goto nml;
      case 't':                 // Tab character
        ch='\t';
        goto nml;
      case 'E':                 // Use alternative format
      case 'O':
      case '#':
        goto nxt;
      case 'p':                 // AM/PM
        src=AMPM[st.hour/12];
        goto app;
      case 'P':                 // am/pm
        src=ampm[st.hour/12];
        goto app;
      case 'a':                 // Short weekday name
        src=sweekdays[st.wday];
        goto app;
      case 'A':                 // Full weekday name
        src=weekdays[st.wday];
        goto app;
      case 'h':
      case 'b':                 // Short month name
        src=smonths[st.month-1];
        goto app;
      case 'B':                 // Full month name
        src=months[st.month-1];
        goto app;
      case 'j':                 // Day of year (001-366)
        src=intoa(buf,st.yday,3,'0');
        goto app;
      case 'g':                 // ISO week-based year without century
        src=intoa(buf,iso_year(st)%100,2,'0');
        goto app;
      case 'G':                 // ISO week-based year with century
        src=intoa(buf,iso_year(st),4,'0');
        goto app;
      case 'y':                 // Year not including century
        val=st.year%100;
        goto num;
      case 'Y':                 // Year including century
        src=intoa(buf,st.year,4,'0');
        goto app;
      case 'C':                 // Century
        val=st.year/100;
        goto num;
      case 'w':                 // Numerical week day (0=Sunday,...6)
        ch='0'+st.wday;
        goto nml;
      case 'u':                 // Numerical week day (1=Monday,...7)
        ch=st.wday?st.wday+'0':'7';
        goto nml;
      case 'U':                 // Week number, starting 1st Sunday (00..53)
        val=(st.yday+6-st.wday)/7;
        goto num;
      case 'W':                 // Week number, starting 1st Monday (00..53)
        val=(st.yday+6-(st.wday+6)%7)/7;
        goto num;
      case 'V':                 // ISO week number (01..53)
        val=iso_week(st);
        goto num;
      case 'Z':                 // Time zone (abbreviated) name
//        tzset();                        // FIXME
//        src=tzname[0];
//        src="Z";
        src="GMT";
        goto app;
      case 'z':                 // Offset +hhmm or -hhmm from UTC
        __snprintf(buf,sizeof(buf),"%+03d%02d",st.offset/3600,(Math::iabs(st.offset)/60)%60);
        src=buf;
        goto app;
      case 'e':                 // Day of the month (leading blank)
        pad=' ';
      case 'd':                 // Day of the month (leading 0)
        val=st.mday;
        goto num;
      case 'k':                 // Hours using a 24-hour clock (leading blank)
        pad=' ';
      case 'H':                 // Hours using a 24-hour clock (leading 0)
        val=st.hour;
        goto num;
      case 'l':                 // Hours using a 12-hour clock (leading blank)
        pad=' ';
      case 'I':                 // Hours using a 12-hour clock (leading 0)
        val=st.hour;
        if(val==0) val=12;      // 12-hour clock starts as "12:00 am", not "00:00 am"
        else if(val>12) val-=12;
        goto num;
      case 'm':                 // Numerical month
        val=st.month;
        goto num;
      case 'M':                 // Minutes
        val=st.min;
        goto num;
      case 'S':                 // Seconds
        val=st.sec;
        goto num;
      case 's':                 // Seconds since Unix Epoch
        sec=timeFromSystemTime(st)/1000000000;
        __snprintf(buf,sizeof(buf),"%lld",sec);
        src=buf;
        goto app;
      case 'f':                 // Fraction
        ch=*format++;
        if(ch=='m'){            // Milliseconds (000-999)
          src=intoa(buf,(st.nano+500000)/1000000,3,'0');
          goto app;
          }
        if(ch=='u'){            // Microseconds (000000-999999)
          src=intoa(buf,(st.nano+500)/1000,6,'0');
          goto app;
          }
        if(ch=='n'){            // Nanoseconds (000000000-999999999)
          src=intoa(buf,st.nano,9,'0');
          goto app;
          }
        goto x;
      case 'D':
        n=systemTimeFormatRecursive(st,string,length-result,"%m/%d/%y");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'F':
        n=systemTimeFormatRecursive(st,string,length-result,"%Y-%m-%d");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'c':
        n=systemTimeFormatRecursive(st,string,length-result,"%b %a %d %k:%M:%S %Z %Y");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'r':
        n=systemTimeFormatRecursive(st,string,length-result,"%I:%M:%S %p");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'R':
        n=systemTimeFormatRecursive(st,string,length-result,"%H:%M");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'x':
        n=systemTimeFormatRecursive(st,string,length-result,"%b %a %d");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'X':
        n=systemTimeFormatRecursive(st,string,length-result,"%k:%M:%S");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      case 'T':
        n=systemTimeFormatRecursive(st,string,length-result,"%H:%M:%S");
        if(result+n<length){ string+=n; }
        result+=n;
        continue;
      default:
        goto x;
      }

      // Convert number val
num:  src=intoa(buf,val,2,pad);

      // Append to string
app:  while((ch=*src++)!='\0'){
        if(result<length){ *string++=ch; }
        result++;
        }

      // Next character
      continue;
      }

    // Normal characters
nml:if(result<length){ *string++=ch; }
    result++;
    }

  // Last character
x:if(result<length){
    *string='\0';
    }

  // Done
  return result;
  }


// Format system time to string
FXString FXSystem::systemTimeFormat(const Time& st,const FXchar* format){
  FXString result;
  if(format && *format){
    FXint len=systemTimeFormatRecursive(st,result.text(),result.length(),format);       // Try to see if existing buffer fits
    if(result.length()<len){
      result.length(len);
      len=systemTimeFormatRecursive(st,result.text(),result.length(),format);           // Again with exactly the right size
      }
    result.length(len);
    }
  return result;
  }

}
