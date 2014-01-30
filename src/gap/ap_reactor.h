/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2012 by Sander Jansen. All Rights Reserved      *
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
#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

struct pollfd;

namespace ap {

class Reactor {
private:
#ifndef WIN32
  struct pollfd * pfds;
  FXint nfds;
  FXint mfds;
#endif
public:

#ifndef WIN32
  class Native {
  public:
    // Number of inputs
    virtual FXint no(){ return 0;};
    
    // Prepare inputs
    virtual void prepare(struct pollfd*){};

    // Dispatch inputs
    virtual void dispatch(struct pollfd*){};
  
    virtual ~Native(){}
    };
#endif

  class Input {
    public:
      FXInputHandle handle;
      FXuchar       mode;
    public:
      enum {
        Readable    = (1<<0),
        Writable    = (1<<1),
        Exception   = (1<<2),
        IsReadable  = (1<<3),
        IsWritable  = (1<<4),
        IsException = (1<<5),      
        Disabled    = (1<<6)   
        };
    public:
      virtual void onSignal() {}
    public:
      Input(FXInputHandle h,FXuchar m) : handle(h),mode(m) {}

      void disable() { mode|=Disabled; }

      void enable()  { mode&=~Disabled; }

      FXbool readable() const {  return (mode&IsReadable); }

      virtual ~Input(){}
    };

  class Timer {
    friend class Reactor;
    private:
      Timer * next;
    protected:
      FXTime  time;
    public:
      Timer() : time(0) {}
      virtual void onExpired() {}
      virtual ~Timer() {}
    };

  class Deferred {  
    public:
      FXuchar mode;
    public:
      enum {
        Disabled  = 0x1,   
        };
    public:
      Deferred() : mode(0) {}

      void disable() { mode|=Disabled; }

      void enable()  { mode&=~Disabled; }
  
      virtual void run() {}
      virtual ~Deferred() {}
    };


protected:
  FXPtrListOf<Deferred> deferred; // run once every iteration
  FXPtrListOf<Input>    inputs;
  FXPtrListOf<Native>   native;
  Timer *               timers;
protected:
  FXTime prepare();
  void dispatch();
  void wait(FXTime);
  FXbool dispatchDeferred();
public:
  Reactor();

  void addNative(Native*);

  void removeNative(Native*);  

  // Add a deferred call
  void addDeferred(Deferred*);

  // Remove a deferred call
  void removeDeferred(Deferred*);

  // Add a watch
  void addInput(Input*);

  // Remove a watch
  void removeInput(Input*);

  // Add a timer
  void addTimer(Timer*,FXTime);

  // Remove a timer
  void removeTimer(Timer*);

  // Run Once
  void runOnce();

  // Run any pending events
  void runPending();

  // Run Once with Timeout
  void runOnce(FXTime timeout);

#ifdef DEBUG
  // Debug Objects
  void debug();  
#endif

  ~Reactor();
  };

}
#endif
