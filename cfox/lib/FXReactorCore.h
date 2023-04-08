/********************************************************************************
*                                                                               *
*                            R e a c t o r   C o r e                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2019,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#ifndef FXREACTORCORE_H
#define FXREACTORCORE_H

//#undef HAVE_EPOLL_CREATE1

namespace FX {


// Platform dependent reactor internals
struct FXReactor::Internals {
#if defined(WIN32)
  FXint              signotified[64];                   // Signal notify flag
  FXInputHandle      handles[MAXIMUM_WAIT_OBJECTS];     // Handles
  FXuint             modes[MAXIMUM_WAIT_OBJECTS];       // IO Modes each handle
#elif defined(HAVE_EPOLL_CREATE1)
  FXint              signotified[64];                   // Signal notify flag
  struct epoll_event events[128];                       // Events
  FXInputHandle      handle;                            // Poll handle
#else
  FXint              signotified[64];                   // Signal notify flag
  fd_set             watched[3];                        // Watched handles
  fd_set             handles[3];                        // Known handles
#endif
  };

}

#endif
