/********************************************************************************
*                                                                               *
*                                T i m e   S t u f f                            *
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
#include "FXAtomic.h"
#include "FXElement.h"
#include "FXString.h"
#include "FXSystem.h"


/*
  Notes:
  - Handy functions for manipulating calendar time.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Time units in terms of nanoseconds
static const FXTime seconds=1000000000;
static const FXTime minutes=60*seconds;
static const FXTime hours=60*minutes;
static const FXTime days=24*hours;


// Default formatting string used for time formatting
const FXchar FXSystem::defaultTimeFormat[]="%m/%d/%Y %H:%M:%S";


// ISO 8601 time format (yyyy-mm-ddThh:mm:ss+hhmm) formatting string
const FXchar FXSystem::isoTimeFormat[]="%FT%T%z";


// Cumulative days of the year, for non-leap-years and leap-years
static const FXint days_of_year[2][13]={
  {0,31,59,90,120,151,181,212,243,273,304,334,365},
  {0,31,60,91,121,152,182,213,244,274,305,335,366}
  };


// Is year a leap-year
static FXint is_leap(FXint year){
  return !(year%4) && ((year%100) || !(year%400));
  }


// Returns day of week in civil calendar [0, 6] -> [Sun, Sat],
// from z is number of days since 1970-01-01.
static FXuint weekday_from_days(FXlong z){
  return (FXuint)(z>=-4 ? (z+4)%7 : (z+5)%7+6);
  }


// From year, month (1..12), day (1..31) return year-day (1..366).
static FXint yearday_from_date(FXint y,FXint m,FXint d){
  return days_of_year[is_leap(y)][m-1]+d;
  }

/*******************************************************************************/

// Returns number of days since civil 1970-01-01.  Negative values indicate
// days prior to 1970-01-01.
// y is year, m is month of year (1..12), d is day of month (1..31).
FXlong FXSystem::daysFromCivil(FXint y,FXint m,FXint d){
  y-=(m<=2);                                            // March
  FXlong era=(y>=0?y:y-399)/400;                        // Era
  FXuint yoe=(FXuint)(y-era*400);                       // [0, 399]
  FXuint doy=(153*(m+(m>2?-3:9))+2)/5+d-1;              // [0, 365]
  FXuint doe=yoe*365+yoe/4-yoe/100+doy;                 // [0, 146096]
  return era*146097+doe-719468;                         // Relative to epoch
  }


// Returns year/month/day in civil calendar.
// z is number of days since 1970-01-01. Negative values indicate
// days prior to 1970-01-01.
// y is year, m is month of year (1..12), d is day of month (1..31).
void FXSystem::civilFromDays(FXint& y,FXint& m,FXint& d,FXlong z){
  z+=719468;                                            // Relative to era
  FXlong era=(z>=0?z:z-146096)/146097;                  // Era
  FXuint doe=(FXuint)(z-era*146097);                    // [0, 146096]
  FXuint yoe=(doe-doe/1460+doe/36524-doe/146096)/365;   // [0, 399]
  FXuint doy=doe-(365*yoe+yoe/4-yoe/100);               // [0, 365]
  FXuint mp=(5*doy+2)/153;                              // [0, 11]
  y=(FXint)(era*400+yoe);
  d=doy-(153*mp+2)/5+1;                                 // [1, 31]
  m=mp+(mp<10?3:-9);                                    // [1, 12]
  y+=(m<=2);                                            // March
  }

/*******************************************************************************/

// Compute nanoseconds since Unix Epoch from struct tm
FXTime FXSystem::timeFromSystemTime(const Time& st){
  FXint year=st.year;
  FXint month=st.month;
  FXint day=st.mday;
  FXint hour=st.hour;
  FXint min=st.min;
  FXint sec=st.sec;
  FXint nano=st.nano;
  FXint leap;

  // Validate nanoseconds
  if(nano>seconds){
    sec+=nano/seconds;
    nano%=seconds;
    }

  // Validate seconds
  if(sec>=60){
    min+=sec/60;
    sec%=60;
    }

  // Validate minutes
  if(min>=60){
    hour+=min/60;
    min%=60;
    }

  // Validate days
  if(hour>=24){
    day+=hour/24;
    hour%=24;
    }

  // Validate month
  if(month>=13){
    year+=(month-1)/12;
    month=(month-1)%12+1;
    }

  // Is leap year
  leap=is_leap(year);

  // Validate day of month
  while(day>days_of_year[leap][month]){
    day-=days_of_year[leap][month];
    month+=1;
    if(13<=month){
      month=1;
      year+=1;
      leap=is_leap(year);
      }
    }

  // Return nanoseconds since Epoch
  return (((daysFromCivil(year,month,day)*24+hour)*60+min)*60+sec)*seconds+nano;
  }


// Return system time from utc in nanoseconds since Unix Epoch
void FXSystem::systemTimeFromTime(Time& st,FXTime utc){

  // Compute days from nanoseconds, rounding down
  FXlong zz=(0<=utc ? utc : utc-(days-1))/days;

  // Compute date from seconds
  civilFromDays(st.year,st.month,st.mday,zz);

  // Compute day of year
  st.yday=yearday_from_date(st.year,st.month,st.mday);

  // Compute day of week
  st.wday=weekday_from_days(zz);

  // Hours
  utc=utc-zz*days;
  st.hour=(FXint)(utc/hours);

  // Minutes
  utc=utc-st.hour*hours;
  st.min=(FXint)(utc/minutes);

  // Seconds
  utc=utc-st.min*minutes;
  st.sec=(FXint)(utc/seconds);

  // Nanoseconds
  utc=utc-st.sec*seconds;
  st.nano=(FXint)utc;

  // Offset utc
  st.offset=0;
  }

/*******************************************************************************/


// Time zone variables, set only once
static volatile FXuint local_zone_set=0;

#if defined(_WIN32)
//
// The Windows TIME_ZONE_INFORMATION struct contains:
//
//   struct TIME_ZONE_INFORMATION {
//     LONG       Bias;                 // UTC = localtime + bias (minutes)
//     WCHAR      StandardName[32];     // Standard time name
//     SYSTEMTIME StandardDate;         // Time when daylight saving to standard time occurs
//     LONG       StandardBias;         // Value added to Bias during standard time (most zones = 0)
//     WCHAR      DaylightName[32];     // Daylight savings time name
//     SYSTEMTIME DaylightDate;         // Time when standard time to daylight savings time occurs
//     LONG       DaylightBias;         // Value added to Bias during daylight savings time (most zones = -60)
//     };
//
// While the Windows SYSTEMTIME struct contains:
//
//   struct SYSTEMTIME {
//     WORD wYear;                      // Year, 1601..
//     WORD wMonth;                     // Month, january..december (1..12)
//     WORD wDayOfWeek;                 // Day of week, sunday..monday (0..7)
//     WORD wDay;                       // Day of month, (1..31) [SEE NOTES BELOW]
//     WORD wHour;                      // Hour (0..23)
//     WORD wMinute;                    // Minutes (0..59)
//     WORD wSecond;                    // Seconds (0..59)
//     WORD wMilliseconds;              // Milliseconds (0..999)
//     };
//
static TIME_ZONE_INFORMATION tzi;
#endif


// Call tzset() only once
static void setuplocaltimezone(){
  if(atomicSet(&local_zone_set,1)==0){
#if defined(_WIN32)
    GetTimeZoneInformation(&tzi);
#else
    tzset();
#endif
    }
  }


// Return offset between standard local time zone to UTC, in nanoseconds
FXTime FXSystem::localTimeZoneOffset(){
  setuplocaltimezone();
#if defined(_WIN32)
  return minutes*tzi.Bias;              // +minutes*tzi.StandardBias;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  return 0;     // FIXME
#else
  return seconds*timezone;
#endif
  }


// Return offset daylight savings time to standard time, in nanoseconds
FXTime FXSystem::daylightSavingsOffset(){
  setuplocaltimezone();
#if defined(_WIN32)
  return minutes*tzi.DaylightBias;      // Or difference between standard and daylight bias.
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  return 0;     // FIXME
#else
  return -hours*daylight;
#endif
  }


// Return time zone name (or daylight savings time time zone name)
FXString FXSystem::localTimeZoneName(FXbool dst){
  setuplocaltimezone();
#if defined(_WIN32)
  return FXString(dst?tzi.DaylightName:tzi.StandardName);
#else
  return FXString(tzname[dst]);
#endif
  }


#if defined(WIN32)

// Convert utc (ns) since 01/01/1970 to 100ns since 01/01/1601
static inline FXTime fxwintime(FXTime utc){
  return utc/FXLONG(100)+FXLONG(116444736000000000);
  }


// Compare incoming date cur against changeover date chg; there are
// two ways to compare the numbers.
// When chg.wYear!=0, the change over occurs only once on the indicated
// absolute time.
// If chg.wYear==0, the switch between standard and daylight savings time
// occurs yearly, on the nth occurrence of wDayOfWeek, where n is given
// in the wDay variable of the TIME_ZONE_INFORMATION StandardDate or
// DaylightDate struct.
static FXint compare_zone_switch_over(const SYSTEMTIME& cur,const SYSTEMTIME& chg){
  static const FXint month_lengths[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  FXint cursecs,chgsecs,chgday,first,last;

  // Absolute date
  if(chg.wYear){
    if(cur.wYear==chg.wYear){
      if(cur.wMonth==chg.wMonth){
        if(cur.wDay==chg.wDay){
          cursecs=(cur.wHour*60+cur.wMinute)*60+cur.wSecond;
          chgsecs=(chg.wHour*60+chg.wMinute)*60+chg.wSecond;
          return cursecs-chgsecs;
          }
        return cur.wDay-chg.wDay;
        }
      return cur.wMonth-chg.wMonth;
      }
    return cur.wYear-chg.wYear;
    }

  // Relative date
  if(cur.wMonth==chg.wMonth){

    // Calculate the day of the first wDayOfWeek in the month
    first=(6+chg.wDayOfWeek-cur.wDayOfWeek+cur.wDay)%7+1;

    // Check needed for the 5th weekday of the month
    last=month_lengths[cur.wMonth-1]+(cur.wMonth==2 && is_leap(cur.wYear));

    // Switch at the nth occurrence (value in chg.wDay) of certain day of week
    chgday=first+7*(chg.wDay-1);
    if(chgday>last) chgday-=7;

    chgsecs=((chgday*24+chg.wHour)*60+chg.wMinute)*60+chg.wSecond;
    cursecs=((cur.wDay*24+cur.wHour)*60+cur.wMinute)*60+cur.wSecond;
    return cursecs-chgsecs;
    }
  return (FXint)cur.wMonth-(FXint)chg.wMonth;
  }


// Determine if daylight savings in effect
// The daylight savings time switch is tied to local time, and local
// time's year is tied to time zone.  However, time zone is a function
// of the local year [time zone info can change from year to year].
// We assume that the time zone is relatively stable, but we may
// call GetTimeZoneInformationForYear() in a future revision to
// obtain only DST/STD time switches, which are changed more rapidly.
static FXint daylightSavingsState(FXTime utc,FXbool local){

  // Expanded date/time
  SYSTEMTIME loc;

  // Assume local time
  FXTime ftloc=utc;
  FXTime ftdst=utc;
  FXTime ftstd=utc;

  // If UTC, convert to local using
  if(!local){
    ftloc=ftloc-tzi.Bias*minutes;
    ftdst=ftloc-tzi.DaylightBias*minutes;
    ftstd=ftloc-tzi.StandardBias*minutes;
    }

  // Convert to windows time
  ftloc=fxwintime(ftloc);

  // Get expanded date/time
  FileTimeToSystemTime((const FILETIME*)&ftloc,&loc);

  // If wMonth is zero then no daylight savings in effect
  if(tzi.DaylightDate.wMonth && tzi.StandardDate.wMonth){

    // Convert UNIX Epoch to windows time
    ftdst=fxwintime(ftdst);
    ftstd=fxwintime(ftstd);

    // Expanded date/time
    SYSTEMTIME dst;
    SYSTEMTIME std;

    // Get expanded date/time
    FileTimeToSystemTime((const FILETIME*)&ftdst,&dst);
    FileTimeToSystemTime((const FILETIME*)&ftstd,&std);

    // Daylight savings time prior to switch to standard time
    FXbool before_std_date=(compare_zone_switch_over(dst,tzi.StandardDate)<0);

    // Standard time after switch to daylight savings time
    FXbool after_dst_date=(compare_zone_switch_over(std,tzi.DaylightDate)>=0);

    // Northern hemisphere
    if(tzi.DaylightDate.wMonth<tzi.StandardDate.wMonth){
      if(before_std_date && after_dst_date) return 1;
      }

    // Southern hemisphere
    else{
      if(before_std_date || after_dst_date) return 1;
      }
    }
  return 0;
  }

#endif


// Return 1 if daylight savings time is active at utc in nanoseconds since Unix Epoch
FXTime FXSystem::daylightSavingsActive(FXTime utc){
  setuplocaltimezone();
#if defined(_WIN32)
  return daylightSavingsState(utc,false);
#elif defined(HAVE_LOCALTIME_R)
  struct tm tmresult;
  time_t tmp=(time_t)(utc/seconds);
  struct tm* ptm=localtime_r(&tmp,&tmresult);
  FXTRACE((300,"FXSystem::daylightSavingsActive(%lld) = %d\n",utc,ptm && ptm->tm_isdst!=0));
  return ptm && ptm->tm_isdst!=0;
#else
  time_t tmp=(time_t)(utc/seconds);
  struct tm* ptm=localtime(&tmp);
  FXTRACE((300,"FXSystem::daylightSavingsActive(%lld) = %d\n",utc,ptm && ptm->tm_isdst!=0));
  return ptm && ptm->tm_isdst!=0;
#endif
  }

/*******************************************************************************/

// Format utc in nanoseconds since Unix Epoch to date-time string using given format
FXString FXSystem::universalTime(FXTime utc,const FXchar *format){
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
  FXSystem::systemTimeFromTime(st,utc);
  return FXSystem::systemTimeFormat(st,format);
  }


// Parse utc date-time string to UTC nanoseconds since Unix Epoch using given format
// Assume string is time in UTC, unless a time zone offset was parsed; in that
// case, adjust the time according to the zone offset.
FXTime FXSystem::universalTime(const FXchar* string,const FXchar* format){
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
  if(FXSystem::systemTimeParse(st,string,format)){
    return FXSystem::timeFromSystemTime(st)+st.offset*seconds;
    }
  return forever;
  }


// Parse date-time string to UTC nanoseconds since Unix Epoch using given format
FXTime FXSystem::universalTime(const FXString& string,const FXchar* format){
  return FXSystem::universalTime(string.text(),format);
  }

/*******************************************************************************/

// Format utc in nanoseconds since Unix Epoch to local date-time string using given format
FXString FXSystem::localTime(FXTime utc,const FXchar *format){
  FXTime zoneoffset=FXSystem::localTimeZoneOffset()+FXSystem::daylightSavingsOffset()*FXSystem::daylightSavingsActive(utc);
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,(FXint)(zoneoffset/seconds)};
  FXSystem::systemTimeFromTime(st,utc-zoneoffset);
  return FXSystem::systemTimeFormat(st,format);
  }


// Parse local date-time string to UTC nanoseconds since Unix Epoch using given format
// Assume string is time in local time, unless a time zone offset was parsed; in that
// case, adjust the time according to the zone offset.
FXTime FXSystem::localTime(const FXchar* string,const FXchar* format){
  FXSystem::Time st={0,0,0,0,0,0,0,0,0,0};
  if(FXSystem::systemTimeParse(st,string,format)){
    //FXTime zoneoffset=FXSystem::localTimeZoneOffset()+FXSystem::daylightSavingsOffset()*FXSystem::daylightSavingsActive(FXThread::time());
    return FXSystem::timeFromSystemTime(st)+st.offset*seconds;
    }
  return forever;
  }


// Parse local date-time string to UTC nanoseconds since Unix Epoch using given format
FXTime FXSystem::localTime(const FXString& string,const FXchar* format){
  return FXSystem::localTime(string.text(),format);
  }

}

