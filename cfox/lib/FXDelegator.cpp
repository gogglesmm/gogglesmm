/********************************************************************************
*                                                                               *
*                       D e l e g a t o r   T a r g e t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXDelegator.h"

/*

  Notes:
  - A delegator forwards messages to another target.
  - A delegator does not itself understand any message; it is mainly used
    to provide a single point at which the actual object receiving the messages
    from the controls can be switched around to another.
*/

using namespace FX;

/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXDelegator,FXObject,nullptr,0)


// Delegate message to another target
long FXDelegator::onDefault(FXObject* sender,FXSelector sel,void* ptr){
  return delegate && delegate->handle(sender,sel,ptr);
  }

}
