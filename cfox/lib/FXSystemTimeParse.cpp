/********************************************************************************
*                                                                               *
*                  C o n v e r t   S t r i n g   T o   T i m e                  *
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
  - Convert a string to FXSystem::Time similar to UNIX strptime().

  - Format string may contain the following special conversion specifiers:

      %%        The % character.

      %n        Arbitrary whitespace.
      %t        Arbitrary whitespace.

      %E        Use alternative format.
      %O        Use alternative format.

      %C        The century number (0-99).

      %Y        The year, including century (for example, 1991).
      %y        The year within century (0-99). When a century is not otherwise specified, values in
                the range 69-99 refer to years in the twentieth century (1969-1999); values in the
                range 00-68 refer to years in the twenty-first century (2000-2068).
      %G        The year corresponding to the ISO week number. (For example, 1991.)
      %g        The year corresponding to the ISO week number, but without the century (0-99).

      %b        The month name, abbreviated or full.
      %B        The month name, abbreviated or full.
      %h        The month name, abbreviated or full.
      %m        The month number (1-12).

      %a        The name of the day of the week, abbreviated or full.
      %A        The name of the day of the week, abbreviated or full.
      %w        The day of the week as a decimal number (0-6), Sunday being 0.
      %u        The day of the week as a decimal number (1-7), Monday being 1.

      %d        The day of month (1-31).
      %e        The day of month (1-31).

      %j        The day number in the year (1-366).

      %p        The locale's equivalent of AM or PM. (Note: there may be none.)

      %H        The hour (0-23).
      %k        The hour (0-23).

      %I        The hour on a 12-hour clock (1-12).
      %l        The hour on a 12-hour clock (1-12).

      %M        The minute (0-59).

      %S        The second (0-60; 60 may occur for leap seconds; earlier also 61 was allowed).

      %s        The number of seconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC).

      %c        The date and time representation for the current locale.

      %D        Equivalent to %m/%d/%y. (This is the American style date, very confusing
                to non-Americans, especially since %d/%m/%y  is widely used in Europe.
                The ISO 8601 standard format is %Y-%m-%d.)

      %r        The 12-hour clock time (using the locale's AM or PM). In the POSIX locale
                equivalent to %I:%M:%S %p. If t_fmt_ampm  is empty in the LC_TIME  part of
                the current locale, then the behavior is undefined.

      %R        Equivalent to %H:%M.

      %T        Equivalent to %H:%M:%S.

      %U        The week number with Sunday the first day of the week (0-53). The first Sunday of
                January is the first day of week 1.

      %W        The week number with Monday the first day of the week (0-53). The first Monday of
                January is the first day of week 1.


      %x        The date, using the locale's date format.

      %X        The time, using the locale's time format.

      %F        Equivalent to %Y-%m-%d, the ISO 8601 date format.

      %V        The ISO 8601:1988 week number as a decimal number (1-53). If the week (starting on
                Monday) containing 1 January has four or more days in the new year, then it is
                considered week 1. Otherwise, it is the last week of the previous year, and the
                next week is week 1.

      %z        An RFC-822/ISO 8601 standard timezone specification.

      %Z        The timezone name.

  - FOX extensions:

      %fm       Fractional seconds: milli-seconds (000-999).

      %fu       Fractional seconds: micro-seconds (000000-999999).

      %fn       Fractional seconds: nano-seconds (000000000-999999999).

  - We can parse ISO week number (1..53) and ISO day of week (1..7) and get
    year day, day of month, month, and week day back from that.
    Algorithm is according to https://en.wikipedia.org/wiki/ISO_week_date.
    There are a few tweaks, due to 0=sunday:

      yday=week*7 + (wday+6)%7 - (day_of_week_jan4(year)+6)%7 - 3;

    Short explanation of the algorithm. For the year, the ISO calendar
    starts week 1 as the week containing january 4th, or (alternatively),
    the first week containing thursday, or first week with >= 4 days in
    the new year.

    We use the first definition, and calculate the day of the week (with
    sunday=0) on January 4th (dowjan4). The amount to back up is based
    on a monday-starting week, so we back up by (dowjan4+6)%7, plus
    an extra 3 days for the beginning of the year.

    Then we simply add the week number and weekday, (similarly converted
    to monday-starting by performing (wday+6)%7).

    The resulting yday may have to be adjusted; if < 1, the date falls
    into the previous year, and we correct yday and year accordingly.
    If > days_in_year(year), then the date falls into the next year,
    and we correct yday and year as needed.  Otherwise, we use yday
    and year as computed.

*/

#ifndef LLONG_MAX
#define LLONG_MAX  FXLONG(9223372036854775807)
#endif

using namespace FX;

/*******************************************************************************/

namespace FX {


// Elements successfully parsed
enum {
  SET_CENT=1,
  SET_YEAR=2,
  SET_MON=4,
  SET_WEEK=8,
  SET_YDAY=16,
  SET_MDAY=32,
  SET_WDAY=64,
  SET_12HR=128,
  SET_24HR=256,
  SET_MIN=512,
  SET_SEC=1024,
  SET_OFF=2048,
  SET_TZ=4096,
  SET_ALTE=8192,
  SET_ALTO=16384,
  SET_ALTH=32768,
  SET_PM=65536,
  SET_ISO=131072
  };


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


// US standard timezones
static const FXchar *const USSTD[6]={
  "EST","CST","MST","PST","AKST","HST"
  };

// US daylight savings timezones
static const FXchar *const USDST[5]={
  "EDT","CDT","MDT","PDT","AKDT"
  };

// Names for UTC
static const FXchar *const GMT[]={
  "GMT","UTC", "UT", "Z"
  };


// Start of month
static const FXint start_of_month[2][13]={
  {0,31,59,90,120,151,181,212,243,273,304,334,365},
  {0,31,60,91,121,152,182,213,244,274,305,335,366}
  };


// Military time zone offsets (A-I,J,K-Z)
static const FXint militaryzoneoffsets[]={
  -3600, -7200, -10800, -14400, -18000, -21600, -25200, -28800, -32400, 0, -36000, -39600, -43200,
   3600,  7200,  10800,  14400,  18000,  21600,  25200,  28800,  32400,     36000,  39600,  43200, 0
  };


// Find string str in a list list[] of nlist strings, and set number num accordingly
static const FXchar *findstring(FXint& result,const FXchar *string,const FXchar *const *list,FXint nlist){
  for(FXint i=0; i<nlist; i++){
    FXuval len=strlen(list[i]);
    if(FXString::comparecase(list[i],string,len)==0){
      result=i;
      return string+len;
      }
    }
  return nullptr;
  }


// Convert nanoseconds fraction of second, up to 9 digits 0...999999999
static const FXchar *convertnanoseconds(FXint& result,const FXchar *string,FXint digs){
  FXint w=100000000;
  result=0;
  if('0'<=*string && *string<='9' && digs){     // Need at least 1 digit
    do{
      result+=(*string++-'0')*w;
      w/=10;
      }
    while('0'<=*string && *string<='9' && --digs);
    return string;
    }
  return nullptr;
  }



// Convert string of digits, but ensure number is [lo..hi]
static const FXchar *convertstring(FXint& result,const FXchar *string,FXint lo,FXint hi){
  FXint digs=hi;
  result=0;
  while(*string==' ' || *string=='\t'){         // Some numerics may have leading blanks
    string++;
    }
  if('0'<=*string && *string<='9' && digs){     // But then we absolutely need a digit
    do{
      result=result*10+(*string++-'0');
      digs/=10;                                 // Counting down digits
      }
    while('0'<=*string && *string<='9' && digs && result*10<=hi);
    if(lo<=result && result<=hi){
      return string;
      }
    }
  return nullptr;
  }


// Is leap year
static FXint is_leap(FXint year){
  return !(year%4) && ((year%100) || !(year%400));
  }


// Days for year
static FXint days_in_year(FXint year){
  return 365+is_leap(year);
  }


// Calculate the week day of the first day of a year, for Gregorian calendar.
// Simplified version of Tomohiko Sakamoto's method for Gregorian Calendar.
static FXint day_of_week_jan1(FXint year){
  year-=1;
  return (year+year/4-year/100+year/400+1)%7;
  }


// Calculate the week day of the 4th day of a year, for Gregorian calendar.
// Simplified version of Tomohiko Sakamoto's method for Gregorian Calendar.
static FXint day_of_week_jan4(FXint year){
  year-=1;
  return (year+year/4-year/100+year/400+4)%7;
  }


// Calculate the day of the week from year, month (1..12), and day of the month (1..31).
// Tomohiko Sakamoto's method for Gregorian Calendar.
static FXint day_of_week(FXint year,FXint month,FXint day){
  static const FXint table[]={0,3,2,5,0,3,5,1,4,6,2,4};
  year-=(month<3);
  return (year+year/4-year/100+year/400+table[month-1]+day)%7;
  }


// Convert string to time according to format.
static const FXchar* systemTimeParseRecursive(FXSystem::Time& st,FXuint& set,const FXchar* string,const FXchar* format){
  const FXTime seconds=1000000000;
  const FXchar *s;
  FXint startofweek=0;
  FXint week=0;
  FXint leap=0;
  FXint pm=0;
  FXint num,ext,neg;
  FXlong sse;
  FXchar ch;

  // Parse format
  while((ch=*format++)!='\0'){

    // Check format characters
    if(ch=='%'){

      // Get next format character
nxt:  ch=*format++;
      switch(ch){
      case '%':                 // Just match '%'
        if(*string!='%') return nullptr;
        string++;
        continue;
      case 'n':                 // Any kind of white-space
      case 't':
        while(Ascii::isSpace(*string)) string++;
        continue;
      case 'E':                 // "%E?" alternative conversion modifier
        set|=SET_ALTE;
        goto nxt;
      case 'O':                 // "%O?" alternative conversion modifier
        set|=SET_ALTO;
        goto nxt;
      case '#':
        set|=SET_ALTH;
        goto nxt;
      case 'P':                 // AM/PM 0..1
      case 'p':
        string=findstring(pm,string,ampm,2);
        if(!string) return nullptr;
        set|=SET_PM;
        continue;
      case 'a':
      case 'A':                 // Weekday 0..6
        if((s=findstring(st.wday,string,weekdays,7))!=nullptr){    // Longest first
          string=s;
          set|=SET_WDAY;
          continue;
          }
        if((s=findstring(st.wday,string,sweekdays,7))!=nullptr){   // Then shorter
          string=s;
          set|=SET_WDAY;
          continue;
          }
        return nullptr;
      case 'B':                 // Month 1..12
      case 'b':
      case 'h':
        if((s=findstring(num,string,months,12))!=nullptr){         // Longest first
          st.month=num+1;
          string=s;
          set|=SET_MON;
          continue;
          }
        if((s=findstring(num,string,smonths,12))!=nullptr){        // Then shorter
          st.month=num+1;
          string=s;
          set|=SET_MON;
          continue;
          }
        return nullptr;
      case 'j':                 // The day of year 1..366
        string=convertstring(st.yday,string,1,366);
        if(!string) return nullptr;
        set|=SET_YDAY;
        continue;
      case 'g':                 // Year corresponding to the ISO week, w/o century
        string=convertstring(num,string,0,99);
        if(!string) return nullptr;
        if(set&SET_CENT){       // Keep century, set the years
          st.year=(st.year/100)*100+num;
          }
        else{                   // Set year, guess the century
          st.year=(num<69)?num+1900:num+2000;
          }
        set|=SET_YEAR|SET_ISO;
        continue;
      case 'G':                 // Year corresponding to the ISO week, with century
        string=convertstring(st.year,string,0,9999);
        if(!string) return nullptr;
        set|=SET_CENT|SET_YEAR|SET_ISO;
        continue;
      case 'y':                 // Year without century
        string=convertstring(num,string,0,99);
        if(!string) return nullptr;
        if(set&SET_CENT){       // Keep century, set the years
          st.year=(st.year/100)*100+num;
          }
        else{                   // Set year, guess the century
          st.year=(num<69)?num+1900:num+2000;
          }
        set|=SET_YEAR;
        continue;
      case 'Y':                 // Year with century 0..9999
        string=convertstring(st.year,string,0,9999);
        if(!string) return nullptr;
        set|=SET_CENT|SET_YEAR;
        continue;
      case 'C':                 // Century number 0..99
        string=convertstring(num,string,0,99);
        if(!string) return nullptr;
        if(set&SET_YEAR){       // Keep the years, set the century
          st.year=st.year%100+num*100;
          }
        else{
          st.year=num*100;      // Set the century
          }
        set|=SET_CENT;
        continue;
      case 'w':                 // The day of week, sunday = 0
        string=convertstring(st.wday,string,0,6);
        if(!string) return nullptr;
        startofweek=0;
        set|=SET_WDAY;
        continue;
      case 'u':                 // The day of week, monday = 1
        string=convertstring(num,string,1,7);
        if(!string) return nullptr;
        st.wday=num%7;
        startofweek=1;
        set|=SET_WDAY;
        continue;
      case 'U':                 // Week number, starting 1st Sunday (00..53)
        string=convertstring(week,string,0,53);
        if(!string) return nullptr;
        startofweek=0;          // SUN
        set|=SET_WEEK;
        continue;
      case 'W':                  // Week number, starting 1st Monday (00..53)
        string=convertstring(week,string,0,53);
        if(!string) return nullptr;
        startofweek=1;          // MON
        set|=SET_WEEK;
        continue;
      case 'V':                 // Week number, ISO 8601:1988 (01..53)
        string=convertstring(week,string,1,53);
        if(!string) return nullptr;
        startofweek=1;          // MON
        set|=SET_WEEK|SET_ISO;
        continue;
      case 'Z':                 // Time zone
        while(*string==' ' || *string=='\t') string++;
        if('a'<=(*string|0x20) && (*string|0x20)<='z' && *(string+1)<=' '){     // Military time zone offset
          st.offset=militaryzoneoffsets[(*string|0x20)-'a'];                    // Offset in seconds
          if((*string|0x20)=='j'){                                              // Local timezone offset
            st.offset=-FXSystem::localTimeZoneOffset()/seconds;
            }
          string++;
          set|=SET_OFF;
          continue;
          }
        if((s=findstring(num,string,USSTD,ARRAYNUMBER(USSTD)))!=nullptr){          // Standard time
          string=s;
          st.offset=-3600*(num+5);
          set|=SET_OFF;
          continue;
          }
        if((s=findstring(num,string,USDST,ARRAYNUMBER(USDST)))!=nullptr){          // Daylight savings time
          string=s;
          st.offset=-3600*(num+4);
          set|=SET_OFF;
          continue;
          }
        if((s=findstring(num,string,GMT,ARRAYNUMBER(GMT)))!=nullptr){              // GMT
          string=s;
          st.offset=0;
          set|=SET_OFF;
          continue;
          }
        while(*string && *string!=' ' && *string!='\t'){                        // Parse over it, for now..
          string++;
          }
        continue;
      case 'z':                 // Time zone offset
        while(*string==' ' || *string=='\t') string++;
        if((*string|0x20)=='z' && *(string+1)<=' '){    // (Z|z) Zulu time indicator
          string++;
          st.offset=0;                                  // Assign to gmt offset
          set|=SET_OFF;
          continue;
          }
        if((neg=*string=='-') || *string=='+'){         // (+|-)HH(:?MM)? Offset hours and minutes
          string++;
          if('0'<=*string && *string<='9'){
            num=*string++-'0';
            if('0'<=*string && *string<='9'){
              num*=10;
              num+=*string++-'0';
              num*=3600;                                // Convert hours to seconds
              if(*string==':' || ('0'<=*string && *string<='9')){
                if(*string==':') string++;
                if('0'<=*string && *string<='9'){
                  ext=*string++-'0';
                  if('0'<=*string && *string<='9'){
                    ext*=10;
                    ext+=*string++-'0';
                    if(ext>=60) return nullptr;            // More than 60 minutes
                    ext*=60;                            // Convert minutes to seconds
                    num+=ext;                           // Add to the rest
                    }
                  else{
                    return nullptr;                        // 1 minute digit only!
                    }
                  }
                else{
                  return nullptr;                          // 0 minute digits at all!
                  }
                }
              if(neg) num=-num;
              st.offset=num;                            // Assign to gmt offset
              set|=SET_OFF;
              continue;
              }
            }
          }
        return nullptr;
      case 'e':                 // Day of month 1..31
      case 'd':
        string=convertstring(st.mday,string,1,31);
        if(!string) return nullptr;
        set|=SET_MDAY;
        continue;
      case 'k':                 // The hour using a 24 hour clock
      case 'H':
        string=convertstring(st.hour,string,0,23);
        if(!string) return nullptr;
        set|=SET_24HR;
        continue;
      case 'l':                 // The hour using a 12 hour clock
      case 'I':
        string=convertstring(st.hour,string,0,12);
        if(!string) return nullptr;
        st.hour%=12;
        set|=SET_12HR;
        continue;
      case 'm':                 // Month 1..12
        string=convertstring(st.month,string,1,12);
        if(!string) return nullptr;
        set|=SET_MON;
        continue;
      case 'M':                 // Minutes 0..59
        string=convertstring(st.min,string,0,59);
        if(!string) return nullptr;
        set|=SET_MIN;
        continue;
      case 'S':                 // Seconds 0..61
        string=convertstring(st.sec,string,0,61);
        if(!string) return nullptr;
        set|=SET_SEC;
        continue;
      case 's':                 // Seconds since Unix Epoch
        if(*string<'0' || '9'<*string) return nullptr;
        sse=0;
        do{
          num=*string++-'0';
          if(sse>=(LLONG_MAX/10)){
            if(sse>(LLONG_MAX/10)) return nullptr;
            if(num>(LLONG_MAX%10)) return nullptr;
            }
          sse=sse*10+num;
          }
        while('0'<=*string && *string<='9');
        systemTimeFromTime(st,sse*seconds);
        set|=SET_CENT|SET_YEAR|SET_MON|SET_YDAY|SET_MDAY|SET_WDAY|SET_24HR|SET_MIN|SET_SEC|SET_OFF;
        continue;
      case 'f':                 // Fraction
        ch=*format++;
        if(ch=='m'){            // Milliseconds
          string=convertnanoseconds(st.nano,string,3);
          if(!string) return nullptr;
          continue;
          }
        if(ch=='u'){            // Microseconds
          string=convertnanoseconds(st.nano,string,6);
          if(!string) return nullptr;
          continue;
          }
        if(ch=='n'){            // Nanoseconds
          string=convertnanoseconds(st.nano,string,9);
          if(!string) return nullptr;
          continue;
          }
        return nullptr;
      case 'D':                 // The date as "%m/%d/%y"
        string=systemTimeParseRecursive(st,set,string,"%m/%d/%y");
        if(!string) return nullptr;
        continue;
      case 'F':                 // The date as "%Y-%m-%d"
        string=systemTimeParseRecursive(st,set,string,"%Y-%m-%d");
        if(!string) return nullptr;
        continue;
      case 'c':                 // Date and time, using the locale's format
        string=systemTimeParseRecursive(st,set,string,"%b %a %d %k:%M:%S %Z %Y");
        if(!string) return nullptr;
        continue;
      case 'r':                 // The time in 12-hour clock representation
        string=systemTimeParseRecursive(st,set,string,"%I:%M:%S %p");
        if(!string) return nullptr;
        continue;
      case 'R':                 // The time as "%H:%M"
        string=systemTimeParseRecursive(st,set,string,"%H:%M");
        if(!string) return nullptr;
        continue;
      case 'x':                 // The date, using the locale's format
        string=systemTimeParseRecursive(st,set,string,"%b %a %d");
        if(!string) return nullptr;
        continue;
      case 'X':                 // The time, using the locale's format
        string=systemTimeParseRecursive(st,set,string,"%k:%M:%S");
        if(!string) return nullptr;
        continue;
      case 'T':                 // The time as "%H:%M:%S"
        string=systemTimeParseRecursive(st,set,string,"%H:%M:%S");
        if(!string) return nullptr;
        continue;
      default:                  // Error: not supported
        return nullptr;
        }
      return nullptr;
      }

    // Match whitespace
    if(ch==' ' || ch=='\t'){
      while(Ascii::isSpace(*string)) string++;
      continue;
      }

    // Match other characters
    if(ch!=*string) return nullptr;
    string++;
    }

  // Apply PM if 12-hour clock
  if((set&SET_PM)&&(set&SET_12HR)){
    st.hour+=12*pm;
    }

  // Some things may be calculated
  if(set&SET_YEAR){

    // If we have month and day-of-month, we can calculate day-of-year
    if((set&SET_MON) && (set&SET_MDAY)){
      if(!(set&SET_YDAY)){
        leap=is_leap(st.year);
        st.yday=start_of_month[leap][st.month-1]+st.mday; // 1..366
        set|=SET_YDAY;
        }
      }

    // If we have week and day-of-week, we can calculate day-of-year
    if((set&SET_WEEK) && (set&SET_WDAY)){
      if(!(set&SET_YDAY)){
        if(set&SET_ISO){        // ISO week number
          st.yday=week*7+(st.wday+6)%7-(day_of_week_jan4(st.year)+6)%7-3;
          if(st.yday<1){
            st.yday=st.yday+days_in_year(st.year-1);
            st.year-=1;
            }
          else if(st.yday>days_in_year(st.year)){
            st.yday=st.yday-days_in_year(st.year);
            st.year+=1;
            }
          }
        else{                   // Simple week number
          st.yday=(week-1)*7+(7-day_of_week_jan1(st.year)+startofweek)%7+(st.wday-startofweek+7)%7+1;
          }
        set|=SET_YDAY;
        }
      }

    // If we have day-of-year, we can calculate month, day-of-month, and day-of-week
    if(set&SET_YDAY){
      if(!(set&SET_MON)){
        leap=is_leap(st.year);
        st.month=13;
        while(st.yday<=start_of_month[leap][st.month-1]) --st.month;
        if(12<st.month){
          st.yday-=start_of_month[leap][12];
          st.year++;
          st.month=1;
          }
        set|=SET_MON;
        }
      if(!(set&SET_MDAY)){
        leap=is_leap(st.year);
        st.mday=st.yday-start_of_month[leap][st.month-1];
        set|=SET_MDAY;
        }
      if(!(set&SET_WDAY)){
        st.wday=(day_of_week_jan1(st.year)+st.yday-1)%7;
        set|=SET_WDAY;
        }
      }
    }
  return string;
  }


// Parse system time from string, returning number of characters parsed
FXint FXSystem::systemTimeParse(Time& st,const FXchar* string,const FXchar* format){
  const FXchar* ptr; FXuint set=0;
  if((ptr=systemTimeParseRecursive(st,set,string,format))!=nullptr){
    return ptr-string;
    }
  return 0;
  }


// Parse system time from string, returning number of characters parsed
FXint FXSystem::systemTimeParse(Time& st,const FXString& string,const FXchar* format){
  return FXSystem::systemTimeParse(st,string.text(),format);
  }

}
