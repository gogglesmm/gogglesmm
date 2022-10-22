/********************************************************************************
*                                                                               *
*                            D a t e   C l a s s                                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXDate.h"


/*
  Notes:
  - Henry F. Fliegel and Thomas C. Van Flandern, "A Machine Algorithm for
    Processing Calendar Dates". CACM, Vol. 11, No. 10, October 1968, pp 657.
  - Major clean up and simplification was done!
  - Added week number calculations!
  - Start of the JD count is 0 at 12 NOON 1 JAN -4712 (4713 BC).
  - Reminder, MJD = JD - 2400000.5.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


// Many nanoseconds in a second
static const FXTime seconds=1000000000L;

// Julian day number of GPS week zero (Jan 6, 1980)
static const FXuint GPS_EPOCH_JDAY=2444245;

// Julian day number of UNIX epoch (Jan 1, 1970)
static const FXuint UNIX_EPOCH_JDAY=2440588;

// Julian day number of J2000 (Jan 1, 2000)
static const FXuint J2000_EPOCH_JDAY=2451545;

// UNIX time to GPS time offset in nanoseconds
static const FXTime UNIX_TO_GPS=315964800L*seconds;

// Short month names
const FXchar FXDate::shortMonthName[12][4]={
 "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
  };


// Long month names
const FXchar FXDate::longMonthName[12][10]={
 "January","February","March","April","May","June","July","August","September","October","November","December"
  };


// Short week day name
const FXchar FXDate::shortWeekDay[7][4]={
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
  };


// Long week day name
const FXchar FXDate::longWeekDay[7][10]={
  "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
  };


// Number of days in nomimal month
static const FXuchar monthDays[13]={
  0,31,28,31,30,31,30,31,31,30,31,30,31
  };


/*******************************************************************************/

// Initialize with year and day of year
FXDate::FXDate(FXint yr,FXint dy){
  setDate(yr,dy);
  }


// Initialize with year, month, and day of month
FXDate::FXDate(FXint yr,FXint mo,FXint dy){
  setDate(yr,mo,dy);
  }


// Set date to year and day of year
void FXDate::setDate(FXint yr,FXint dy){
  if(dy<1 || dy>366){ fxerror("FXDate::setDate: bad argument.\n"); }
  julian=(1461*(yr+4799))/4-(3*((yr+4899)/100))/4+dy-31739;
  }


// Get year and day of year from date
void FXDate::getDate(FXint& yr,FXint& dy) const {
  FXint l,n,i,j;
  l=julian+68569;
  n=(4*l)/146097;
  l=l-(146097*n+3)/4;
  i=(4000*(l+1))/1461001;
  l=l-(1461*i)/4+31;
  j=(80*l)/2447;
  l=j/11;
  yr=100*(n-49)+i+l;
  dy=julian-(1461*(yr+4799))/4+(3*((yr+4899)/100))/4+31739;
  }


// Set date to year, month, and day of month
void FXDate::setDate(FXint yr,FXint mo,FXint dy){
  if(mo<1 || mo>12 || dy<1 || dy>31){ fxerror("FXDate::setDate: bad argument.\n"); }
  julian=(1461*(yr+4800+(mo-14)/12))/4+(367*(mo-2-12*((mo-14)/12)))/12-(3*((yr+4900+(mo-14)/12)/100))/4+dy-32075;
  }


// Get year, month, and day of month from date
void FXDate::getDate(FXint& yr,FXint& mo,FXint& dy) const {
  FXint l,n,i,j;
  l=julian+68569;
  n=(4*l)/146097;
  l=l-(146097*n+3)/4;
  i=(4000*(l+1))/1461001;
  l=l-(1461*i)/4+31;
  j=(80*l)/2447;
  dy=l-(2447*j)/80;
  l=j/11;
  mo=j+2-(12*l);
  yr=100*(n-49)+i+l;
  }


// Set date from nanoseconds since 1/1/1970
// Technically, Julian Day starts at noon; however we truncate
// incoming time to 00:00:00.
void FXDate::setTime(FXTime ns){
  const FXTime days=86400L*seconds;
  julian=UNIX_EPOCH_JDAY+(((0<=ns)?ns:ns-days+1)/days);
  }


// Get nanoseconds since 1/1/1970 from date
// Return time in nanoseconds at start of the day
FXTime FXDate::getTime() const {
  const FXTime days=86400L*seconds;
  return ((FXTime)julian-(FXTime)UNIX_EPOCH_JDAY)*days;
  }


// is value a leap year?
FXbool FXDate::leapYear(FXint yr){
  return ((yr%4==0) && (yr%100!=0)) || (yr%400==0);
  }


// Return number of days in a given year
FXint FXDate::daysInYear(FXint yr){
  return leapYear(yr) ? 366 : 365;
  }


// Return number of days in the month in given year, month
FXint FXDate::daysInMonth(FXint yr,FXint mo){
  return (mo==2 && leapYear(yr)) ? 29 : monthDays[mo];
  }


// Return day of the month
FXint FXDate::day() const {
  FXint yr,mo,dy;
  getDate(yr,mo,dy);
  return dy;
  }


// Return month
FXint FXDate::month() const {
  FXint yr,mo,dy;
  getDate(yr,mo,dy);
  return mo;
  }


// Return year
FXint FXDate::year() const {
  FXint yr,mo,dy;
  getDate(yr,mo,dy);
  return yr;
  }


// Return day of the week
FXint FXDate::dayOfWeek() const {
  return (julian+1)%7;                  // Sunday is day 0 of week
  }


// Return true if leap year
FXbool FXDate::leapYear() const {
  return leapYear(year());
  }


// Return number of days in this year
FXint FXDate::daysInYear() const {
  return daysInYear(year());
  }


// Return days in this month
FXint FXDate::daysInMonth() const {
  FXint yr,mo,dy;
  getDate(yr,mo,dy);
  return daysInMonth(yr,mo);
  }


// Return day of year
FXint FXDate::dayOfYear() const {
  FXDate s(year(),1);
  return julian-s.julian+1;
  }


// Return ISO8601 week number of this date
FXint FXDate::weekOfYear() const {
  FXint d4=(((julian+31741-julian%7)%146097)%36524)%1461;
  FXint L=d4/1460;
  FXint d1=(d4-L)%365+L;
  return 1+d1/7;
  }


// Add d days to this date
FXDate& FXDate::addDays(FXint d){
  julian+=d;
  return *this;
  }


// Add m months to this date; day of month is adjusted for leap-years
FXDate& FXDate::addMonths(FXint m){
  FXint yr,mo,dy,mx;
  getDate(yr,mo,dy);
  if(0<=m){
    yr=yr+(mo-1+m)/12;
    mo=1+(mo-1+m)%12;
    }
  else{
    yr=yr+(mo-12+m)/12;
    mo=1+(mo+2147483627+m)%12;
    }
  mx=daysInMonth(yr,mo);
  if(dy>mx) dy=mx;
  setDate(yr,mo,dy);
  return *this;
  }


// Add y years to this date; day of month is adjusted for leap-years
FXDate& FXDate::addYears(FXint y){
  FXint yr,mo,dy;
  getDate(yr,mo,dy);
  yr+=y;
  if(dy>28 && mo==2 && !leapYear(yr)) dy=28;
  setDate(yr,mo,dy);
  return *this;
  }


// Return current local date
FXDate FXDate::localDate(){
#if defined(WIN32)
  SYSTEMTIME t;
  GetLocalTime(&t);
  return FXDate(t.wYear,t.wMonth,t.wDay);
#elif defined(HAVE_LOCALTIME_R)
  struct tm result,*t;
  time_t ltime;
  time(&ltime);
  t=localtime_r(&ltime,&result);
  return FXDate(t->tm_year+1900,t->tm_mon+1,t->tm_mday);
#else
  struct tm *t;
  time_t ltime;
  time(&ltime);
  t=localtime(&ltime);
  return FXDate(t->tm_year+1900,t->tm_mon+1,t->tm_mday);
#endif
  }


// Return current universal (UTC) date
FXDate FXDate::universalDate(){
#if defined(WIN32)
  SYSTEMTIME t;
  GetSystemTime(&t);
  return FXDate(t.wYear,t.wMonth,t.wDay);
#elif defined(HAVE_GMTIME_R)
  struct tm result,*t;
  time_t ltime;
  time(&ltime);
  t=gmtime_r(&ltime,&result);
  return FXDate(t->tm_year+1900,t->tm_mon+1,t->tm_mday);
#else
  struct tm *t;
  time_t ltime;
  time(&ltime);
  t=gmtime(&ltime);
  return FXDate(t->tm_year+1900,t->tm_mon+1,t->tm_mday);
#endif
  }


// save to store
FXStream& operator<<(FXStream& store,const FXDate& d){
  store << d.julian;
  return store;
  }


// load from store
FXStream& operator>>(FXStream& store,FXDate& d){
  store >> d.julian;
  return store;
  }

}

