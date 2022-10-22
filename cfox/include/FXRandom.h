/********************************************************************************
*                                                                               *
*                  R a n d o m   N u m b e r   G e n e r a t o r                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2007,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXRANDOM_H
#define FXRANDOM_H

namespace FX {


/**
* FXRandom provides a thread-safe random number generator with a period
* of around 1.84E19.
*/
class FXAPI FXRandom {
private:
  FXulong state;
public:

  /**
  * Construct random generator with default seed value.
  */
  FXRandom();

  /**
  * Construct random generator with given seed s.
  * A good source of seed values may be the FXThread::time() function.
  */
  FXRandom(FXulong s);

  /**
  * Initialize this random generator with the state of another.
  */
  FXRandom(const FXRandom& s):state(s.state){}

  /**
  * Assignment state from another random generator.
  */
  FXRandom& operator=(const FXRandom& s){ state=s.state; return *this; }

  /**
  * Generate next state of the random generator.
  */
  FXulong next();

  /**
  * Change state of random generator with new seed s.
  */
  void seed(FXulong s);

  /**
  * Get state of random generator.
  */
  FXulong getState() const { return state; }

  /**
  * Set state of random generator. Ensure state is not zero!.
  */
  void setState(FXulong s){ state=s; }

  /**
  * Draw random unsigned long, uniformly distributed.
  */
  FXulong randLong();

  /**
  * Draw random float, uniformly distributed between 0 and 1.
  */
  FXfloat randFloat();

  /**
  * Draw random double, uniformly distributed between 0 and 1.
  */
  FXdouble randDouble();
  };

}


#endif
