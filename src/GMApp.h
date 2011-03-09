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
#ifndef GMAPP_H
#define GMAPP_H

class GMClipboard;

enum {
  SEL_EMBED_NOTIFY = SEL_LAST,
  SEL_EMBED_MODAL_ON,
  SEL_EMBED_MODAL_OFF,
  SEL_TASK_COMPLETED,
  SEL_TASK_CANCELLED,
  SEL_TASK_STATUS,
  SEL_TASK_RUNNING,
  SEL_TASK_IDLE,
  };




class GMApp : public FXApp {
FXDECLARE(GMApp)
friend class GMPlug;
protected:
  FXID          xembed;
  FXID          xsystemtray;
  FXID          xmanager;
  GMClipboard * clipboard;
  FXFontPtr     thickfont;
protected:
#if FOXVERSION < FXVERSION(1,7,0)
  virtual bool dispatchEvent(FXRawEvent & event);
#else
  virtual FXbool dispatchEvent(FXRawEvent & event);
#endif
public:
#ifdef HAVE_NLS
  locale_t      clocale;
#endif
public:
  static GMApp * instance();
public:
  GMApp();

#if FOXVERSION < FXVERSION(1,7,0)
  virtual void init(int& argc,char** argv,bool connect=true);
#else
  virtual void init(int& argc,char** argv,FXbool connect=true);
#endif
  virtual void create();

  void setFont(const FXFontDesc &);

  void updateFont();

  FXFont* getThickFont() const { return thickfont; }

  static FXString getConfigDirectory(FXbool create=false);
  static FXString getCacheDirectory(FXbool create=false);

  static FXString getDataDirectory(FXbool create=false);

  virtual void exit(FXint code=0);

  virtual ~GMApp();
  };


class GMPlug : public FXTopWindow {
FXDECLARE(GMPlug)
protected:
  FXID         socket;
  FXuchar      xembedflags;
protected:
#if FOXVERSION < FXVERSION(1,7,0)
  virtual bool doesOverrideRedirect() const;
#else
  virtual FXbool doesOverrideRedirect() const;
#endif
private:
  GMPlug(const GMPlug*);
  GMPlug& operator=(const GMPlug&);
protected:
  GMPlug();
public:
  long onEmbedded(FXObject*,FXSelector,void*);
public:
  GMPlug(FXApp * app);

  virtual void setFocus();

  virtual void create();


  virtual ~GMPlug();
  };




#endif





