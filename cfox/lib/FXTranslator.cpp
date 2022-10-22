/********************************************************************************
*                                                                               *
*                       M e s s a g e   T r a n s l a t o r                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXSystem.h"
#include "FXTranslator.h"


/*
  Notes:
  - Since the original input string is potentially used as output string if no
    translation is available, the original input string needs to be UTF8 also.
  - The tr() function has an extra parameter count to allow translations which
    require special plural forms dependent on some number.  If set to -1 (default),
    the first available translation is used.
  -
*/

#define TOPIC_CONSTRUCT 1000
#define TOPIC_TRANSLATE 1010

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXTranslator,FXObject,nullptr,0)


// Construct translator
FXTranslator::FXTranslator(){
  FXTRACE((TOPIC_CONSTRUCT,"FXTranslator::FXTranslator()\n"));
  }

/*
#ifdef WIN32
  LCID mylcid=GetUserDefaultLCID();
  TCHAR buffer[256];

  // ISO Standard 639 values language name
  GetLocaleInfo(mylcid,LOCALE_SISO639LANGNAME,buffer,sizeof(buffer)/sizeof(TCHAR));

  // ISO Standard 3166 country names
  GetLocaleInfo(mylcid,LOCALE_SISO3166CTRYNAME,buffer,sizeof(buffer)/sizeof(TCHAR));

  // ISO Standard 4217 currency
  GetLocaleInfo(mylcid,LOCALE_SINTLSYMBOL,buffer,sizeof(buffer)/sizeof(TCHAR)));

#else
  char *locale=setlocale(LC_ALL, nullptr);
  char *lang=strstr(locale,"LANG=");
  if(!lang) lang=strstr(locale,"LC_MESSAGES=");
  if(!lang) lang=strstr(locale,"LC_CTYPE=");
  if(!lang){	// Try the LANG environment variable
    lang=getenv("LANG");
    if(!lang) break;
    }
  else{
    lang=strchr(lang, '=')+1;
    if('"'==*lang) lang++;
    }
  while(*lang!='_'){
    iso639.append(*lang);
    lang++;
    }
  lang++;
  while(isalpha(*lang) || '@'==*lang){
    iso3166.append(*lang);
    lang++;
    }
#endif
*/


// Translate a string
const FXchar* FXTranslator::tr(const FXchar* context,const FXchar* message,const FXchar* hint,FXint count) const {
  FXTRACE((TOPIC_TRANSLATE,"tr context: '%s' message: '%s' hint: '%s' count: %d.\n",context,message,hint?hint:"",count));
  return message;
  }


// Save data
void FXTranslator::save(FXStream& store) const {
  FXObject::save(store);
  }


// Load data
void FXTranslator::load(FXStream& store){
  FXObject::load(store);
  }


// Destroy translator
FXTranslator::~FXTranslator(){
  FXTRACE((TOPIC_CONSTRUCT,"FXTranslator::~FXTranslator()\n"));
  }

}
