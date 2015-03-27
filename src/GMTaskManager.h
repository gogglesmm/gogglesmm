/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTHREAD_H
#define GMTHREAD_H

class GMWorker;

class GMWorkerThread : public FXThread {
protected:
  GMWorker * worker;
public:
  FXint run();
protected:
  GMWorkerThread(const GMWorkerThread&);
  GMWorkerThread &operator=(const GMWorkerThread&);
public:
  GMWorkerThread(GMWorker *w);
  ~GMWorkerThread();
  };

class GMWorker : public FXObject {
FXDECLARE(GMWorker)
protected:
  GMWorkerThread   * thread;
  FXMessageChannel * channel;
  volatile FXbool    processing;
public:
  enum {
    ID_THREAD_ENTER = 1,
    ID_THREAD_LEAVE,
    ID_LAST
  };
protected:
  GMWorker();
  GMWorker(const GMWorker&);
  GMWorker &operator=(const GMWorker&);
public:
  GMWorker(FXApp * app);

  virtual FXint run() { return 0; }

  void start();

  void stop();

  FXbool send(FXSelector msg,const void* data=nullptr,FXint size=0);

  virtual ~GMWorker();
  };




class GMTaskManager;

class GMTask {
friend class GMTaskManager;
private:
  GMTask(const GMTask&);
  GMTask &operator=(const GMTask&);
protected:
  GMTaskManager    * taskmanager;
  FXMessageChannel * mc;
  volatile FXbool    processing;
protected:
  FXObject * target;
  FXSelector message;
public:
  GMTask(FXObject*tgt=nullptr,FXSelector sel=0);

  void setTarget(FXObject * tgt) { target=tgt; }

  void setSelector(FXSelector sel) { message=sel; }

  virtual FXint run() = 0;

  virtual ~GMTask();
  };

typedef FXArray<GMTask*> GMTaskList;

class GMTaskManager : public FXThread {
protected:
  FXMutex           mutex;
  FXCondition       condition_task;
  volatile FXbool   processing;
  FXbool            started;
protected:
  FXbool wait();
  FXbool next();
protected:
  GMTask          * active;
  FXObject*         target;
  FXSelector        message;
  FXMessageChannel  mc;
  GMTaskList        tasks;
protected:
  FXint run();
public:
  GMTaskManager(FXObject*tgt=nullptr,FXSelector sel=0);

  void run(GMTask*);
  void shutdown();
  void cancelTask();

  void setStatus(const FXString &);

  virtual ~GMTaskManager();
  };


#endif



