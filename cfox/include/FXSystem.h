/********************************************************************************
*                                                                               *
*         M i s c e l l a n e o u s   S y s t e m   F u n c t i o n s           *
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
#ifndef FXSYSTEM_H
#define FXSYSTEM_H

namespace FX {

namespace FXSystem {

  /// System Time in parts
  struct Time {
    FXint year;         /// Year (e.g. 1970)
    FXint month;        /// Month 1..12
    FXint mday;         /// Day of the month 1..31
    FXint yday;         /// Day in the year 1..366
    FXint wday;         /// Day of the week 0..6
    FXint hour;         /// Hours 0..23
    FXint min;          /// Minutes 0..59
    FXint sec;          /// Seconds 0..60
    FXint nano;         /// Nanoseconds 0..999999999
    FXint offset;       /// Seconds east of utc
    };

  /// Compute nanoseconds since Unix Epoch from system time
  extern FXAPI FXTime timeFromSystemTime(const Time& st);

  /// Return system time from utc in nanoseconds since Unix Epoch
  extern FXAPI void systemTimeFromTime(Time& st,FXTime utc);


  /// Default formatting (mm/dd/yyyy hh:mm:ss) string used for time formatting
  extern FXAPI const FXchar defaultTimeFormat[];

  /// ISO 8601 time format (yyyy-mm-ddThh:mm:ss+hh:mm) formatting string
  extern FXAPI const FXchar isoTimeFormat[];


  /// Format system time to string
  extern FXAPI FXString systemTimeFormat(const Time& st,const FXchar* format=defaultTimeFormat);

  /// Parse system time from string, returning number of characters parsed
  extern FXAPI FXint systemTimeParse(Time& st,const FXchar* string,const FXchar* format=defaultTimeFormat);

  /// Parse system time from string, returning number of characters parsed
  extern FXAPI FXint systemTimeParse(Time& st,const FXString& string,const FXchar* format=defaultTimeFormat);


  /**
  * Returns number of days since civil 1970-01-01.  Negative values indicate
  * days prior to 1970-01-01.
  * y is year, m is month of year (1..12), d is day of month (1..31).
  */
  extern FXAPI FXlong daysFromCivil(FXint y,FXint m,FXint d);

  /**
  * Returns year/month/day in civil calendar.
  * z is number of days since 1970-01-01. Negative values indicate
  * days prior to 1970-01-01.
  * y is year, m is month of year (1..12), d is day of month (1..31).
  */
  extern FXAPI void civilFromDays(FXint& y,FXint& m,FXint& d,FXlong z);


  /// Return leap seconds from utc in nanocseconds since Unix Epoch
  extern FXAPI FXival leapSeconds(FXTime utc);

  /// Return leap seconds from tai in nanocseconds since Unix Epoch
  extern FXAPI FXival leapSecondsTAI(FXTime tai);

  /// Return offset between standard local time zone to UTC, in nanoseconds
  extern FXAPI FXTime localTimeZoneOffset();

  /// Return offset daylight savings time to standard time, in nanoseconds
  extern FXAPI FXTime daylightSavingsOffset();

  /// Return 1 if daylight savings time is active at utc in nanoseconds since Unix Epoch
  extern FXAPI FXTime daylightSavingsActive(FXTime utc);

  /// Return time zone name (or daylight savings time time zone name)
  extern FXAPI FXString localTimeZoneName(FXbool dst=false);


  /// Convert NTP format (ssss:ffff) to nanoseconds since Unix Epoch
  extern FXAPI FXTime timeFromNTPTime(FXulong ntptime);

  /// Convert utc in nanoseconds since Unix Epoch to NTP (ssss:ffff)
  extern FXAPI FXulong ntpTimeFromTime(FXTime utc);


  /// Format utc in nanoseconds since Unix Epoch to date-time string using given format
  extern FXAPI FXString universalTime(FXTime utc,const FXchar *format=defaultTimeFormat);

  /// Parse date-time string to UTC nanoseconds since Unix Epoch using given format
  extern FXAPI FXTime universalTime(const FXchar* string,const FXchar* format=defaultTimeFormat);
  extern FXAPI FXTime universalTime(const FXString& string,const FXchar* format=defaultTimeFormat);


  /// Format utc in nanoseconds since Unix Epoch to local date-time string using given format
  extern FXAPI FXString localTime(FXTime utc,const FXchar *format=defaultTimeFormat);

  /// Parse local date-time string to UTC nanoseconds since Unix Epoch using given format
  extern FXAPI FXTime localTime(const FXchar* string,const FXchar* format=defaultTimeFormat);
  extern FXAPI FXTime localTime(const FXString& string,const FXchar* format=defaultTimeFormat);


  /// Get effective user id
  extern FXAPI FXuint user();

  /// Get effective group id
  extern FXAPI FXuint group();

  /// Return owner name from uid if available
  extern FXAPI FXString userName(FXuint uid);

  /// Return group name from gid if available
  extern FXAPI FXString groupName(FXuint gid);

  /// Get current effective user name
  extern FXAPI FXString currentUserName();

  /// Get current effective group name
  extern FXAPI FXString currentGroupName();


  /// Get permissions string
  extern FXAPI FXString modeString(FXuint mode);


  /// Return value of environment variable name
  extern FXAPI FXString getEnvironment(const FXString& name);

  /// Change value of environment variable name, return true if success
  extern FXAPI FXbool setEnvironment(const FXString& name,const FXString& value);


  /// Get the current working directory
  extern FXAPI FXString getCurrentDirectory();

  /// Set the current working directory
  extern FXAPI FXbool setCurrentDirectory(const FXString& path);

  /// Return the current drive (for Win32 systems)
  extern FXAPI FXString getCurrentDrive();

  /// Set the current drive (for Win32 systems)
  extern FXAPI FXbool setCurrentDrive(const FXString& prefix);


  /// Get executable path
  extern FXAPI FXString getExecPath();

  /// Return known executable file extensions (Windows)
  extern FXAPI FXString getExecExtensions();

  /// Get name of calling executable
  extern FXAPI FXString getExecFilename();

  /// Return the home directory for the current user
  extern FXAPI FXString getHomeDirectory();

  /// Return temporary directory
  extern FXAPI FXString getTempDirectory();

  /// Return system directory
  extern FXAPI FXString getSystemDirectory();

  /// Return the home directory for a given user
  extern FXAPI FXString getUserDirectory(const FXString& user);


  /// Return host name
  extern FXAPI FXString getHostName();


  /// Determine if UTF8 locale in effect
  extern FXAPI FXbool localeIsUTF8();

  /**
  * Get DLL name for given base name; for example "png"
  * becomes "libpng.so" on Linux, and "png.dll" on Windows.
  */
  extern FXAPI FXString dllName(const FXString& name);
  }

}

#endif
