/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2017 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef AP_SIGNAL_H
#define AP_SIGNAL_H

namespace ap {

//#define GAP_NO_EVENTFD 1

/// Using eventfd requires Linux 2.6.30. 
#ifndef GAP_NO_EVENTFD
#if defined(__linux__) && defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8))
#define HAVE_EVENTFD
#endif
#endif


enum class WaitEvent {
  Input,
  Signal,
  Timeout,
  Error
  };

enum class WaitMode {
  Read,
  Write,
#ifdef _WIN32
  Connect
#else
  Connect = Write
#endif
  };


/// A signal that can either be set or unset and can be waited on using select/poll/WaitFor
class Signal {
private:
#if !defined(_WIN32) && !defined(HAVE_EVENTFD) 
  FXInputHandle wrptr;
#endif
protected:
  FXInputHandle device;
private:
  Signal(const Signal&);
  Signal& operator=(const Signal&);
public:
  Signal();

  FXbool create();

  void clear();

  void set();

  void close();

  void wait();

  // Wait for signal, input or timeout
  WaitEvent wait(FXInputHandle input,WaitMode mode=WaitMode::Read,FXTime timeout=0) const;

  FXInputHandle handle() const { return device; }
  };


/// A semaphore that can be waited on using select/poll/WaitFor*
class Semaphore {
private:
// For them BSDs we use a pipe
#if !defined(_WIN32) && !defined(HAVE_EVENTFD) 
  FXInputHandle wrptr;
#endif
protected:
  FXInputHandle device;
private:
  Semaphore(const Semaphore&);
  Semaphore& operator=(const Semaphore&);
public:
  Semaphore();

  // Create Semaphore with initial count
  FXbool create(FXint count);

  // Release semaphore
  void release();

  // Block until semaphore is acquired or input is signalled
  FXbool wait(const Signal & input);

  // Close
  void close();
  };

}
#endif
