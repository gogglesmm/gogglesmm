/********************************************************************************
*                                                                               *
*                     P a r a l l e l   C o m p u t a t i o n                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 2012,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXPARALLEL_H
#define FXPARALLEL_H

namespace FX {


enum{
  FXParallelMax=128     // Maximum number of parallel jobs
  };


/**
* FXParallelCallFunctor is a helper for FXParallelInvoke.  It executes a functor on
* a thread provided by the FXThreadPool.
*/
template <typename Functor>
class FXParallelCallFunctor : public FXRunnable {
  const Functor& functor;
private:
  FXParallelCallFunctor(const FXParallelCallFunctor&);
  FXParallelCallFunctor &operator=(const FXParallelCallFunctor&);
public:
  FXParallelCallFunctor(const Functor& fun):functor(fun){ }
  virtual FXint run(){ functor(); return 0; }
  };


/**
* Perform a parallel call to functors fun1 and fun2 using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  group.execute(&task1);
  group.executeAndWait(&task2);
  }


/**
* Perform a parallel call to functors fun1 and fun2 using the FXThreadPool
* associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2>
void FXParallelInvoke(const Functor1& fun1,const Functor2& fun2){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2);
  }


/**
* Perform a parallel call to functors fun1, fun2, and fun3, using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  group.execute(&task1);
  group.execute(&task2);
  group.executeAndWait(&task3);
  }


/**
* Perform a parallel call to functors fun1, fun2, and fun3, using the FXThreadPool
* associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3>
void FXParallelInvoke(const Functor1& fun1,const Functor2& fun2,const Functor3& fun3){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, and fun4, using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  FXParallelCallFunctor<Functor4> task4(fun4);
  group.execute(&task1);
  group.execute(&task2);
  group.execute(&task3);
  group.executeAndWait(&task4);
  }

/**
* Perform a parallel call to functors fun1, fun2, fun3, and fun4, using the FXThreadPool
* associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4>
void FXParallelInvoke(const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3,fun4);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, and fun5, using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor4& fun5){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  FXParallelCallFunctor<Functor4> task4(fun4);
  FXParallelCallFunctor<Functor5> task5(fun5);
  group.execute(&task1);
  group.execute(&task2);
  group.execute(&task3);
  group.execute(&task4);
  group.executeAndWait(&task5);
  }

/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, and fun5, using the
* FXThreadPool associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5>
void FXParallelInvoke(const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor4& fun5){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3,fun4,fun5);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, and fun6, using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  FXParallelCallFunctor<Functor4> task4(fun4);
  FXParallelCallFunctor<Functor5> task5(fun5);
  FXParallelCallFunctor<Functor6> task6(fun6);
  group.execute(&task1);
  group.execute(&task2);
  group.execute(&task3);
  group.execute(&task4);
  group.execute(&task5);
  group.executeAndWait(&task6);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, and fun6, using the
* FXThreadPool associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6>
void FXParallelInvoke(Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3,fun4,fun5,fun6);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, fun6, and fun7, using the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6,typename Functor7>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6,const Functor7& fun7){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  FXParallelCallFunctor<Functor4> task4(fun4);
  FXParallelCallFunctor<Functor5> task5(fun5);
  FXParallelCallFunctor<Functor6> task6(fun6);
  FXParallelCallFunctor<Functor7> task7(fun7);
  group.execute(&task1);
  group.execute(&task2);
  group.execute(&task3);
  group.execute(&task4);
  group.execute(&task5);
  group.execute(&task6);
  group.executeAndWait(&task7);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, fun6, and fun7, using the
* FXThreadPool associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6,typename Functor7>
void FXParallelInvoke(Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6,const Functor7& fun7){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3,fun4,fun5,fun6,fun7);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, fun6, fun7, and fun8, using
* the given FXThreadPool.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6,typename Functor7,typename Functor8>
void FXParallelInvoke(FXThreadPool* pool,const Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6,const Functor7& fun7,const Functor8& fun8){
  FXTaskGroup group(pool);
  FXParallelCallFunctor<Functor1> task1(fun1);
  FXParallelCallFunctor<Functor2> task2(fun2);
  FXParallelCallFunctor<Functor3> task3(fun3);
  FXParallelCallFunctor<Functor4> task4(fun4);
  FXParallelCallFunctor<Functor5> task5(fun5);
  FXParallelCallFunctor<Functor6> task6(fun6);
  FXParallelCallFunctor<Functor7> task7(fun7);
  FXParallelCallFunctor<Functor8> task8(fun8);
  group.execute(&task1);
  group.execute(&task2);
  group.execute(&task3);
  group.execute(&task4);
  group.execute(&task5);
  group.execute(&task6);
  group.execute(&task7);
  group.executeAndWait(&task8);
  }


/**
* Perform a parallel call to functors fun1, fun2, fun3, fun4, fun5, fun6, fun7, and fun8, using the
* FXThreadPool associated with the calling thread.
* Return when all functors have completed.
*/
template <typename Functor1,typename Functor2,typename Functor3,typename Functor4,typename Functor5,typename Functor6,typename Functor7,typename Functor8>
void FXParallelInvoke(Functor1& fun1,const Functor2& fun2,const Functor3& fun3,const Functor4& fun4,const Functor5& fun5,const Functor6& fun6,const Functor7& fun7,const Functor8& fun8){
  FXParallelInvoke(FXThreadPool::instance(),fun1,fun2,fun3,fun4,fun5,fun6,fun7,fun8);
  }

/*******************************************************************************/

/**
* FXParallelLoopFunctor is a helper for FXParallelFor.  It executes a subrange of the
* global indexing range on a thread provided by the FXThreadPool.
*/
template <typename Functor,typename Index>
class FXParallelForFunctor : public FXRunnable {
  const Functor& functor;
  const Index    fm;
  const Index    to;
  const Index    by;
private:
  FXParallelForFunctor(const FXParallelForFunctor&);
  FXParallelForFunctor &operator=(const FXParallelForFunctor&);
public:
  FXParallelForFunctor(const Functor& fun,Index f,Index t,Index b):functor(fun),fm(f),to(t),by(b){ }
  virtual FXint run(){ for(Index ix=fm;ix<to;ix+=by){ functor(ix); } return 0; }
  };


/**
* Perform parallel for-loop executing functor fun(x) for indexes x=fm+by*i, where x<to.
* The index range is split into at most nc pieces.  Each piece is executed in parallel
* using the given FXThreadPool.
*/
template <typename Functor,typename Index>
void FXParallelFor(FXThreadPool* pool,Index fm,Index to,Index by,Index nc,const Functor& fun){
  const FXuval size=(sizeof(FXParallelForFunctor<Functor,Index>)+sizeof(FXulong)-1)/sizeof(FXulong);
  if(fm<to){
    if(by<(to-fm)){
      FXTaskGroup group(pool);
      FXulong space[FXParallelMax*((sizeof(FXParallelForFunctor<Functor,Index>)+sizeof(FXulong)-1)/sizeof(FXulong))];
      Index nits=1+(to-fm-1)/by,ni,c;
      if(nc>FXParallelMax) nc=FXParallelMax;
      if(nc>nits) nc=nits;
      for(c=0; c<nc; fm+=ni*by,++c){
        ni=(nits+nc-1-c)/nc;
        group.execute(new (&space[c*size]) FXParallelForFunctor<Functor,Index>(fun,fm,fm+ni*by,by));
        }
      group.wait();
      }
    else{
      fun(fm);
      }
    }
  }


/**
* Perform parallel for-loop executing functor fun(i) for indexes x=fm+by*i, where x<to.
* The index range is split into at most nc pieces.  Each piece is executed in parallel
* on the FXThreadPool associated with the calling thread.
*/
template <typename Functor,typename Index>
void FXParallelFor(Index fm,Index to,Index by,Index nc,const Functor& fun){
  FXParallelFor(FXThreadPool::instance(),fm,to,by,nc,fun);
  }


/**
* Perform parallel for loop executing functor fun(i) for indexes x=fm+by*i, where x<to.
* The index range is split into at most N pieces, where N is the number of processors on the
* system.  Each piece is executed in parallel using the given FXThreadPool.
*/
template <typename Functor,typename Index>
void FXParallelFor(FXThreadPool* pool,Index fm,Index to,Index by,const Functor& fun){
  FXParallelFor(pool,fm,to,by,(Index)pool->getMaximumThreads(),fun);
  }


/**
* Perform parallel for loop executing functor fun(i) for indexes x=fm+by*i, where x<to.
* The index range is split into at most N pieces, where N is the number of processors on the
* system.  Each piece is executed in parallel on the FXThreadPool associated with the calling
* thread.
*/
template <typename Functor,typename Index>
void FXParallelFor(Index fm,Index to,Index by,const Functor& fun){
  FXParallelFor(FXThreadPool::instance(),fm,to,by,fun);
  }

}

#endif
