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
#include "GMAbout.h"
#include "icons.h"


#ifdef HAVE_DBUS
#include "GMDBus.h"
#endif
#include <FXPNGIcon.h>
#include <sqlite3.h>
#include <tag.h>

#define UTF8_COPYRIGHT_SIGN "\xc2\xa9"

#define APPLICATION_TITLE "Goggles Music Manager"

// Object implementation
FXIMPLEMENT(GMAboutDialog,FXDialogBox,NULL,0)

GMAboutDialog::GMAboutDialog(FXApp * app) : FXDialogBox(app,FXString::null,DECOR_ALL,0,0,0,0,0,0,0,0,0,0) {
  setup();
  }

GMAboutDialog::GMAboutDialog(FXWindow* owner) : FXDialogBox(owner,FXString::null,DECOR_TITLE|DECOR_BORDER|DECOR_CLOSE,0,0,0,0,0,0,0,0,0,0) {
  setup();
  }

GMAboutDialog::~GMAboutDialog(){
  }


void GMAboutDialog::setup(){

  setTitle("About " APPLICATION_TITLE);

  logo = new FXPNGIcon(getApp(),about_png);
  logo->blend(FXRGB(255,255,255));

  FXFontDesc fontdescription = getApp()->getNormalFont()->getFontDesc();
  fontdescription.size  += 10;
  fontdescription.weight = FXFont::Bold;
  titlefont = new FXFont(getApp(),fontdescription);
  titlefont->create();

  fontdescription = getApp()->getNormalFont()->getFontDesc();
  fontdescription.size -= 10;
  licensefont  = new FXFont(getApp(),fontdescription);
  licensefont->create();


  FXLabel * label = new FXLabel(this,APPLICATION_TITLE,logo,ICON_ABOVE_TEXT|LAYOUT_CENTER_X|JUSTIFY_CENTER_X|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  label->setFont(titlefont);
  label->setBackColor(FXRGB(255,255,255));
  label->setTextColor(FXRGB(0,0,0));


  const FXchar gpl[] = "This program is free software: you can\n"
                 "redistribute it and/or modify it under the\n"
                 "terms of the GNU General Public License\n"
                 "as published by the Free Software Foundation,\n"
                 "either version 3 of the License, or (at your\n"
                 "option) any later version.";

  label = new FXLabel(this,"v"APPLICATION_VERSION_STRING,NULL,LAYOUT_CENTER_X|LAYOUT_FILL_X,0,0,0,0,5,5,0,5);
  label->setBackColor(FXRGB(255,255,255));
  label->setTextColor(FXRGB(0,0,0));

  label = new FXLabel(this,"Copyright " UTF8_COPYRIGHT_SIGN " 2004-2010 Sander Jansen",NULL,LAYOUT_CENTER_X|LAYOUT_FILL_X,0,0,0,0,5,5,0,0);
  label->setBackColor(FXRGB(255,255,255));
  label->setTextColor(FXRGB(0,0,0));

  label = new FXLabel(this,gpl,NULL,LAYOUT_CENTER_X|LAYOUT_FILL_X,0,0,0,0,5,5,5,0);
  label->setBackColor(FXRGB(255,255,255));
  label->setTextColor(FXRGB(0,0,0));
  label->setFont(licensefont);


  FXString libraries;


//  XML_Expat_Version expatversion = XML_ExpatVersionInfo();
 // FXuchar ap_major,ap_minor;
//  ap_get_version(ap_major,ap_minor);

#ifdef HAVE_DBUS
  libraries.format("Build with FOX %d.%d.%d\nSQLite %s, DBus %s \nand Taglib.",fxversion[0],fxversion[1],fxversion[2],sqlite3_libversion(),GMDBus::dbusversion().text());
#else
  libraries.format("Build with FOX %d.%d.%d\nSQLite %s and Taglib.",fxversion[0],fxversion[1],fxversion[2],sqlite3_libversion());
#endif

  label = new FXLabel(this,libraries,NULL,LAYOUT_CENTER_X|LAYOUT_FILL_X,0,0,0,0,5,5,5,5);
  label->setBackColor(FXRGB(255,255,255));
  label->setTextColor(FXRGB(0,0,0));
  label->setFont(licensefont);

  new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X);
  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,tr("&Homepage"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_CENTER_X|FRAME_RAISED|FRAME_THICK,0,0,0,0,5,5);
  new GMButton(closebox,tr("&Report Issue"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_CENTER_X|FRAME_RAISED|FRAME_THICK,0,0,0,0,5,5);
  new GMButton(closebox,tr("&Close"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_CENTER_X|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  }
