/********************************************************************************
*                                                                               *
*                          E x c e p t i o n  T y p e s                         *
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
#include "fxver.h"
#include "fxdefs.h"
#include "FXException.h"

using namespace FX;

namespace FX {

// Generic unspecified exception
const FXchar FXException::exceptionName[]="unknown";


// Fatal error occured
const FXchar FXFatalException::exceptionName[]="fatal";


// Error occured
const FXchar FXErrorException::exceptionName[]="error";


// Index out of range
const FXchar FXRangeException::exceptionName[]="out of range";


// Invalid pointer argument
const FXchar FXPointerException::exceptionName[]="invalid pointer";


// Some resource exhausted
const FXchar FXResourceException::exceptionName[]="resource exhausted";


// Out of memory
const FXchar FXMemoryException::exceptionName[]="out of memory";


// Window exception
const FXchar FXWindowException::exceptionName[]="window exception";


// Image, cursor, bitmap exception
const FXchar FXImageException::exceptionName[]="image exception";


// Font exception
const FXchar FXFontException::exceptionName[]="font exception";


// Thread exception
const FXchar FXThreadException::exceptionName[]="thread exception";


}
