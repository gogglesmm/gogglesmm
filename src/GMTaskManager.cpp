/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include "gmdefs.h"
#include "gmutils.h"
#include "GMApp.h"
#include "GMTaskManager.h"


GMTask::GMTask(FXObject*tgt,FXSelector sel) : taskmanager(NULL),mc(NULL),processing(true),target(tgt),message(sel) {
  }

GMTask::~GMTask() {
  }

GMTaskManager::GMTaskManager(FXObject*tgt,FXSelector sel) : processing(false),started(false),active(NULL),target(tgt),message(sel),mc(FXApp::instance())  {
  }

GMTaskManager::~GMTaskManager() {
  }

void GMTaskManager::setStatus(const FXString & msg) {
  if (target) mc.message(target,FXSEL(SEL_TASK_STATUS,message),msg.text(),msg.length()+1);
  }

void GMTaskManager::run(GMTask* task) {
  mutex.lock();
  tasks.append(task);
  task->mc = &mc;
  task->taskmanager = this;
  processing=true;
  if (!started) {
    started=true;
    mutex.unlock();
    start();
    }
  else {
    condition_task.signal();
    mutex.unlock();
    }
  }

FXbool GMTaskManager::next() {
  mutex.lock();
  if (tasks.no()) {
    active = tasks[0];
    tasks.erase(0);
    }
  else {
    active=NULL;
    }
  mutex.unlock();
  return ((active!=NULL) && processing);
  }

FXbool GMTaskManager::wait() {
  if (processing) {
    if (target) mc.message(target,FXSEL(SEL_TASK_IDLE,message),NULL,0);
    mutex.lock();
    condition_task.wait(mutex);
    mutex.unlock();
    }
  return processing;
  }

FXint GMTaskManager::run() {
  ap_set_thread_name("taskmanager");
  do {
    while(next()) {
      if (target) mc.message(target,FXSEL(SEL_TASK_RUNNING,message),NULL,0);
      FXint code = active->run();
      mutex.lock();
      if (target) {

        if (code)
          mc.message(active->target,FXSEL(SEL_TASK_CANCELLED,active->message),&active,sizeof(GMTask*));
        else
          mc.message(active->target,FXSEL(SEL_TASK_COMPLETED,active->message),&active,sizeof(GMTask*));

        active=NULL;
        }
      else {
        delete active;
        active=NULL;
        }
      mutex.unlock();
      }
    }
  while(wait());
  return 0;
  }

void GMTaskManager::cancelTask() {
  FXMutexLock lock(mutex);
  if (active) active->processing=false;
  }

void GMTaskManager::shutdown() {
  mutex.lock();
  if (active) active->processing=false;
  processing=false;
  condition_task.signal();
  mutex.unlock();
  join();
  started=false;
  }
