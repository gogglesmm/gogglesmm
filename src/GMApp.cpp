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
#include <xincs.h>

#include "gmdefs.h"
#include "gmutils.h"
#include "GMApp.h"
#include "GMTrack.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMSource.h"
#include "GMPreferences.h"
#include "GMPlayerManager.h"
#include "GMClipboard.h"
#include "GMTrayIcon.h"


#ifdef HAVE_NLS

#define PACKAGE "gogglesmm"

#ifndef LOCALEDIR
#error LOCALEDIR needs to be defined!!
#endif

#include <libintl.h>
#include <FXTranslator.h>




class GMTranslator : public FXTranslator {
FXDECLARE(GMTranslator)
private:
private:
  GMTranslator(const GMTranslator&);
  GMTranslator &operator=(const GMTranslator&);
#if FOXVERSION < FXVERSION(1,7,16)
protected:
  GMTranslator(){}
public:
  /// Construct translator
  GMTranslator(FXApp* a): FXTranslator(a) {
    setlocale(LC_ALL,"");
    bindtextdomain(PACKAGE,LOCALEDIR);
    bind_textdomain_codeset(PACKAGE,"UTF-8");
    textdomain(PACKAGE);
    GM_DEBUG_PRINT("localedir: %s\n",LOCALEDIR);
    };
#else
public:
  GMTranslator(){
    setlocale(LC_ALL,"");
    bindtextdomain(PACKAGE,LOCALEDIR);
    bind_textdomain_codeset(PACKAGE,"UTF-8");
    textdomain(PACKAGE);
    GM_DEBUG_PRINT("localedir: %s\n",LOCALEDIR);
    };
#endif

#if FOXVERSION < FXVERSION(1,7,16)
  virtual const FXchar* tr(const FXchar* context,const FXchar* message,const FXchar* hint=NULL) const;
#else
  virtual const FXchar* tr(const FXchar* context,const FXchar* message,const FXchar* hint=NULL,FXint count=-1) const;
#endif

  ~GMTranslator() {
    }

  };

FXIMPLEMENT(GMTranslator,FXTranslator,NULL,0)


#if FOXVERSION < FXVERSION(1,7,16)
const FXchar* GMTranslator::tr(const FXchar*,const FXchar* message,const FXchar*) const {
  return gettext(message);
  }
#else
const FXchar* GMTranslator::tr(const FXchar*,const FXchar* message,const FXchar*,FXint) const {
  return gettext(message);
  }
#endif

#endif


extern const FXchar * fxtr(const FXchar *x){
#ifdef HAVE_NLS
  return FXApp::instance()->getTranslator()->tr(NULL,x);
#else
  return x;
#endif
  }


FXIMPLEMENT(GMApp,FXApp,NULL,0)

GMApp::GMApp() : FXApp("gogglesmm","gogglesmm"){
  clipboard = new GMClipboard(this);
  xembed=0;
#ifdef HAVE_NLS
  clocale = newlocale(LC_ALL_MASK,"C",NULL);
#endif
  }

GMApp::~GMApp(){
#ifdef HAVE_NLS
  freelocale(clocale);
#endif
  delete clipboard;
  }

GMApp* GMApp::instance() {
  return dynamic_cast<GMApp*>(FXApp::instance());
  }


void GMApp::create() {

  FXString systemtray = GMStringFormat("_NET_SYSTEM_TRAY_S%d",DefaultScreen((Display*)getDisplay()));

  xembed      = (FXID)XInternAtom((Display*)getDisplay(),"_XEMBED",False);
  xmanager    = (FXID)XInternAtom((Display*)getDisplay(),"MANAGER",True);
  xsystemtray = XInternAtom((Display*)getDisplay(),systemtray.text(),True);

  FXApp::create();

  XSelectInput((Display*)getDisplay(),getRootWindow()->id(),KeyPressMask|KeyReleaseMask|StructureNotifyMask);


#if FOXVERSION < FXVERSION(1,7,17)
  FXFontDesc fontdescription;
  getNormalFont()->getFontDesc(fontdescription);
#else
  FXFontDesc fontdescription = getNormalFont()->getFontDesc();
#endif
  fontdescription.weight = FXFont::Bold;

  thickfont = new FXFont(this,fontdescription);
  thickfont->create();
  }


void GMApp::setFont(const FXFontDesc & fnt){
  getNormalFont()->destroy();
  getNormalFont()->setFontDesc(fnt);
  getNormalFont()->create();
  reg().writeStringEntry("SETTINGS","normalfont",getNormalFont()->getFont().text());

#if FOXVERSION < FXVERSION(1,7,17)
  FXFontDesc fontdescription;
  getNormalFont()->getFontDesc(fontdescription);
#else
  FXFontDesc fontdescription = getNormalFont()->getFontDesc();
#endif
  fontdescription.weight = FXFont::Bold;
  thickfont->destroy();
  thickfont->setFontDesc(fontdescription);
  thickfont->create();
  }

void GMApp::updateFont() {
#if FOXVERSION < FXVERSION(1,7,17)
  FXFontDesc fontdescription;
  getNormalFont()->getFontDesc(fontdescription);
#else
  FXFontDesc fontdescription = getNormalFont()->getFontDesc();
#endif
  setFont(fontdescription);
  }


FXString GMApp::getDataDirectory(FXbool create) {
  FXString xdg_data_home = FXSystem::getEnvironment("XDG_DATA_HOME");

  if (xdg_data_home.empty())
    xdg_data_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".local" PATHSEPSTRING "share" ;

  xdg_data_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    gm_make_path(xdg_data_home);

  return xdg_data_home;
  }


FXString GMApp::getConfigDirectory(FXbool create) {
  FXString xdg_config_home = FXSystem::getEnvironment("XDG_CONFIG_HOME");

  if (xdg_config_home.empty())
    xdg_config_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".config" ;

  xdg_config_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    gm_make_path(xdg_config_home);

  return xdg_config_home;
  }

FXString GMApp::getCacheDirectory(FXbool create) {
  FXString xdg_cache_home = FXSystem::getEnvironment("XDG_CACHE_HOME");
  if (xdg_cache_home.empty())
    xdg_cache_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".cache" ;

  xdg_cache_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    gm_make_path(xdg_cache_home);

  return xdg_cache_home;
  }


#if FOXVERSION < FXVERSION(1,7,0)
void GMApp::init(int& argc,char** argv,bool connect) {
#else
void GMApp::init(int& argc,char** argv,FXbool connect) {
#endif

#if FOXVERSION >= FXVERSION(1,7,22)
  reg().setVendorKey("gogglesmm");
  reg().setAppKey("settings");
  FXApp::init(argc,argv,connect);
#else
  ///FIXME This is slightly broken, I think.
  FXApp::init(argc,argv,connect);

  /// Read the new xdg settings file.
  reg().parseFile(GMApp::getConfigDirectory()+PATHSEPSTRING "settings.rc",true);
#endif


#ifdef HAVE_NLS
#if FOXVERSION < FXVERSION(1,7,16)
  setTranslator(new GMTranslator(this));
#else
  setTranslator(new GMTranslator());
#endif
#endif

  }

void GMApp::exit(FXint code) {

  /// Write the new xdg settings file.
  reg().unparseFile(GMApp::getConfigDirectory()+PATHSEPSTRING "settings.rc");
  reg().clear();
  reg().setModified(false);

  FXApp::exit(code);
  }

enum {
  XEMBED_EMBEDDED_NOTIFY = 0,
  XEMBED_MODALITY_ON     = 10,
  XEMBED_MODALITY_OFF    = 11,
  XEMBED_REQUEST_FOCUS   = 3
  };

#include <fxkeys.h>

// Get keysym; interprets the modifiers!
static FXuint keysym(FXRawEvent& event){
  KeySym sym=KEY_VoidSymbol;
  char buffer[40];
  XLookupString(&event.xkey,buffer,sizeof(buffer),&sym,NULL);
  return sym;
  }

#if FOXVERSION < FXVERSION(1,7,0)
bool GMApp::dispatchEvent(FXRawEvent & ev) {
#else
FXbool GMApp::dispatchEvent(FXRawEvent & ev) {
#endif

  /// Handle Global Hotkeys
  if (ev.xany.window==getRootWindow()->id()){

    if (ev.xany.type==KeyPress) {
      //fxmessage("keypress %d %x\n",ev.xkey.keycode,keysym(ev));
      if (GMPlayerManager::instance()->handle_global_hotkeys(keysym(ev)))
        return true;
      }
    else if (ev.xany.type==ClientMessage) {
      if (ev.xclient.message_type==xmanager && ev.xclient.data.l[1]==xsystemtray) {
        if (GMPlayerManager::instance()->getTrayIcon())
          GMPlayerManager::instance()->getTrayIcon()->dock();
        return true;
        }
      }
    }

  FXWindow * window = findWindowWithId(ev.xany.window);
  if (window && ev.xany.type==ClientMessage && ev.xclient.message_type==xembed) {
    switch(ev.xclient.data.l[1]) {
      case XEMBED_EMBEDDED_NOTIFY: window->tryHandle(this,FXSEL(SEL_EMBED_NOTIFY,0),(void*)(FXival)ev.xclient.data.l[3]); break;
      case XEMBED_MODALITY_ON    : window->tryHandle(this,FXSEL(SEL_EMBED_MODAL_ON,0),NULL); break;
      case XEMBED_MODALITY_OFF   : window->tryHandle(this,FXSEL(SEL_EMBED_MODAL_OFF,0),NULL); break;
      default                    : /*fxmessage("Missed a message %d\n",ev.xclient.data.l[1]);*/ break;
      }
    return true;
    }

  return FXApp::dispatchEvent(ev);
  }


FXDEFMAP(GMPlug) GMPlugMap[]={
  FXMAPFUNC(SEL_EMBED_NOTIFY,0,GMPlug::onEmbedded)
  };

FXIMPLEMENT(GMPlug,FXTopWindow,GMPlugMap,ARRAYNUMBER(GMPlugMap));

GMPlug::GMPlug(){
  }

GMPlug::GMPlug(FXApp * app) : FXTopWindow(app,"test",NULL,NULL,DECOR_NONE,0,0,1,1,0,0,0,0,0,0) , socket(0) {
  }

GMPlug::~GMPlug(){
  }

#if FOXVERSION < FXVERSION(1,7,0)
bool GMPlug::doesOverrideRedirect() const {
  return true;
  }
#else
FXbool GMPlug::doesOverrideRedirect() const{
  return true;
  }
#endif

void GMPlug::setFocus(){
  FXShell::setFocus();
  if (xid && socket) {
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = socket;
    ev.xclient.message_type = ((GMApp*)getApp())->xembed;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = XEMBED_REQUEST_FOCUS;
    XSendEvent((Display*)getApp()->getDisplay(),socket, False, NoEventMask, &ev);
    }
  }

void GMPlug::create() {
  FXTopWindow::create();
  if (xid) {
    Atom xembedinfo = XInternAtom((Display*)getApp()->getDisplay(),"_XEMBED_INFO",0);
    if (xembedinfo!=None) {
      unsigned long info[2]={0,(1<<0)};
      XChangeProperty((Display*)getApp()->getDisplay(),xid,xembedinfo,xembedinfo,32,PropModeReplace,(unsigned char*)info,2);
      }
    }
  }

long GMPlug::onEmbedded(FXObject*,FXSelector,void*ptr){
  flags|=FLAG_SHOWN;
  socket=(FXID)(FXival)ptr;
  return 1;
  }


void ewmh_set_window_icon(const FXWindow * window,FXImage * icon) {
#ifndef WIN32
  Atom net_wm_icon = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_ICON",False);

  unsigned long * data=NULL;
  int nelems=2+(icon->getWidth()*icon->getHeight());

  allocElms(data,nelems);

  data[0]=icon->getWidth();
  data[1]=icon->getHeight();
  for (FXint i=0;i<(icon->getWidth()*icon->getHeight());i++){
    const FXColor val = icon->getData()[i];
    data[i+2]=FXRGBA(FXBLUEVAL(val),FXGREENVAL(val),FXREDVAL(val),FXALPHAVAL(val));
    }

  /// Set Property
  XChangeProperty((Display*)window->getApp()->getDisplay(),window->id(),net_wm_icon,XA_CARDINAL,32,PropModeReplace,(unsigned char*)data,nelems);

  freeElms(data);
#endif
  }


void ewmh_activate_window(const FXWindow * window) {
#ifndef WIN32

  FXASSERT(window->getApp());
  FXASSERT(window->getApp()->getDisplay());
  FXASSERT(window->id());

  static Atom net_active_window = None;

  if (net_active_window==None) {
    net_active_window = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_ACTIVE_WINDOW",False);
    }

  Display * display = (Display*)window->getApp()->getDisplay();

  XClientMessageEvent ev;
  ev.type             = ClientMessage;
  ev.send_event       = False;
  ev.display          = display;
  ev.window           = window->id();
  ev.message_type     = net_active_window;
  ev.format           = 32;
  ev.data.l[0]        = 2; // 1 = applications, 2 = pagers
  ev.data.l[1]        = 0;
  ev.data.l[2]        = 0;
  XSendEvent(display,window->getApp()->getRootWindow()->id(),False,(SubstructureRedirectMask|SubstructureNotifyMask),(XEvent*)&ev);
#endif
  }


void ewmh_change_window_type(const FXWindow * window,FXuint kind) {
#ifndef WIN32
  static Atom net_wm_window_type               = None;
  static Atom net_wm_window_type_menu          = None;
  static Atom net_wm_window_type_dropdown_menu = None;
  static Atom net_wm_window_type_popup_menu    = None;
  static Atom net_wm_window_type_combo         = None;
  static Atom net_wm_window_type_tooltip       = None;
  static Atom net_wm_window_type_dialog        = None;
  static Atom net_wm_window_type_normal        = None;

  FXASSERT(window->getApp());
  FXASSERT(window->getApp()->getDisplay());
  FXASSERT(window->id());

  if (net_wm_window_type==None){
    net_wm_window_type               = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE",False);
    net_wm_window_type_menu          = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_MENU",False);
    net_wm_window_type_dropdown_menu = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",False);
    net_wm_window_type_popup_menu    = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_POPUP_MENU",False);
    net_wm_window_type_combo         = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_COMBO",False);
    net_wm_window_type_tooltip       = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_TOOLTIP",False);
    net_wm_window_type_dialog        = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_DIALOG",False);
    net_wm_window_type_normal        = XInternAtom((Display*)window->getApp()->getDisplay(),"_NET_WM_WINDOW_TYPE_NORMAL",False);
    }


  unsigned int ntypes=0;
  Atom types[3]={0};

  switch(kind) {
    case WINDOWTYPE_DIALOG          : types[0]=net_wm_window_type_dialog;
                                      ntypes=1;
                                      break;
    case WINDOWTYPE_COMBO           : types[0]=net_wm_window_type_combo;
                                      types[1]=net_wm_window_type_dropdown_menu;
                                      types[2]=net_wm_window_type_menu;
                                      ntypes=3;
                                      break;
    case WINDOWTYPE_POPUP_MENU      : types[0]=net_wm_window_type_popup_menu;
                                      types[1]=net_wm_window_type_menu;
                                      ntypes=2;
                                      break;
    case WINDOWTYPE_DROPDOWN_MENU   : types[0]=net_wm_window_type_dropdown_menu;
                                      types[1]=net_wm_window_type_menu;
                                      ntypes=2;
                                      break;
    case WINDOWTYPE_TOOLTIP         : types[0]=net_wm_window_type_tooltip;
                                      ntypes=1;
                                      break;
    default                         : types[0]=net_wm_window_type_normal;
                                      ntypes=1;
                                      break;
    }

  /// Set Property
  XChangeProperty((Display*)window->getApp()->getDisplay(),window->id(),net_wm_window_type,XA_ATOM,32,PropModeReplace,(unsigned char*)&types,ntypes);
#endif
  }




