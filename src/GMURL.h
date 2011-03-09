/********************************************************************************
*                                                                               *
*                       U R L   M a n i p u l a t i o n                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2010 by Jeroen van der Zijp.   All Rights Reserved.        *
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
*********************************************************************************
* $Id: FXURL.h,v 1.23 2008/06/02 14:19:51 fox Exp $                             *
********************************************************************************/
#ifndef GMURL_H
#define GMURL_H

#if FOXVERSION < FXVERSION(1,7,0)
namespace GMURL {

/// Encode control characters and characters from set using %-encoding
extern FXAPI FXString encode(const FXString& string,const FXchar* set=NULL);

/// Decode string containing %-encoded characters
extern FXAPI FXString decode(const FXString& string);

/// Parse scheme from string containing url
extern FXAPI FXString scheme(const FXString& string);

/// Parse username from string containing url
extern FXAPI FXString username(const FXString& string);

/// Parse password from string containing url
extern FXAPI FXString password(const FXString& string);

/// Parse hostname from string containing url
extern FXAPI FXString host(const FXString& string);

/// Parse port number from string containing url
extern FXAPI FXint port(const FXString& string);

/// Parse path from string containing url
extern FXAPI FXString path(const FXString& string);

/// Parse query from string containing url
extern FXAPI FXString query(const FXString& string);

/// Parse fragment from string containing url
extern FXAPI FXString fragment(const FXString& string);


/// Return URL of filename
extern FXAPI FXString fileToURL(const FXString& string);

/// Return filename from URL, empty if url is not a local file
extern FXAPI FXString fileFromURL(const FXString& string);
}
#else
#define GMURL FXURL
#endif
#else
#error GMURL already included
#endif
