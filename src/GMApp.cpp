/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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

#ifdef HAVE_OPENGL
#include <GL/glew.h>
#endif

#include <fxkeys.h>




#include <xincs.h>
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
public:
  GMTranslator(){
    setlocale(LC_ALL,"");
    bindtextdomain(PACKAGE,LOCALEDIR);
    bind_textdomain_codeset(PACKAGE,"UTF-8");
    textdomain(PACKAGE);
    GM_DEBUG_PRINT("localedir: %s\n",LOCALEDIR);
    };
  virtual const FXchar* tr(const FXchar* context,const FXchar* message,const FXchar* hint=NULL,FXint count=-1) const;
  ~GMTranslator() {}

  };

FXIMPLEMENT(GMTranslator,FXTranslator,NULL,0)


const FXchar* GMTranslator::tr(const FXchar*,const FXchar* message,const FXchar*,FXint) const {
  return gettext(message);
  }

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
#ifdef HAVE_OPENGL
  glvisual=NULL;
  glcontext=NULL;
#endif
  }

GMApp::~GMApp(){
  delete clipboard;
  }

GMApp* GMApp::instance() {
  return dynamic_cast<GMApp*>(FXApp::instance());
  }


void GMApp::create() {

  FXString systemtray = FXString::value("_NET_SYSTEM_TRAY_S%d",DefaultScreen((Display*)getDisplay()));

  xembed      = (FXID)XInternAtom((Display*)getDisplay(),"_XEMBED",False);
  xmanager    = (FXID)XInternAtom((Display*)getDisplay(),"MANAGER",True);
  xsystemtray = XInternAtom((Display*)getDisplay(),systemtray.text(),True);

  FXApp::create();

  XSelectInput((Display*)getDisplay(),getRootWindow()->id(),KeyPressMask|KeyReleaseMask|StructureNotifyMask);

  FXFontDesc fontdescription = getNormalFont()->getFontDesc();
  fontdescription.weight = FXFont::Bold;

  thickfont = new FXFont(this,fontdescription);
  thickfont->create();


  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 10;
  fontdescription.weight   = FXFont::Bold;
  fontdescription.setwidth = FXFont::SemiCondensed;
  coverheadfont            = new FXFont(this,fontdescription);


  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 10;
  fontdescription.setwidth = FXFont::SemiCondensed;
  coverbasefont            = new FXFont(this,fontdescription);


  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 30;
  fontdescription.setwidth = FXFont::SemiCondensed;
  fontdescription.slant    = FXFont::Italic;
  fontdescription.weight   = FXFont::Light;
  listtailfont             = new FXFont(this,fontdescription);
  }


void GMApp::setFont(const FXFontDesc & fnt){
  getNormalFont()->destroy();
  getNormalFont()->setFontDesc(fnt);
  getNormalFont()->create();
  reg().writeStringEntry("SETTINGS","normalfont",getNormalFont()->getFont().text());

  FXFontDesc fontdescription = getNormalFont()->getFontDesc();
  fontdescription.weight = FXFont::Bold;
  thickfont->destroy();
  thickfont->setFontDesc(fontdescription);
  thickfont->create();

  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 10;
  fontdescription.weight   = FXFont::Bold;
  fontdescription.setwidth = FXFont::SemiCondensed;
  coverheadfont->destroy();
  coverheadfont->setFontDesc(fontdescription);
  coverheadfont->create();

  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 10;
  fontdescription.setwidth = FXFont::SemiCondensed;
  coverbasefont->destroy();
  coverbasefont->setFontDesc(fontdescription);
  coverbasefont->create();


  fontdescription = getNormalFont()->getFontDesc();
  fontdescription.size    -= 30;
  fontdescription.setwidth = FXFont::SemiCondensed;
  fontdescription.slant    = FXFont::Italic;
  fontdescription.weight   = FXFont::Light;
  listtailfont->destroy();
  listtailfont->setFontDesc(fontdescription);
  listtailfont->create();
  }

void GMApp::updateFont() {
  setFont(getNormalFont()->getFontDesc());
  }


FXString GMApp::getPodcastDirectory(FXbool create) {
  FXString xdg_data_home = FXSystem::getEnvironment("XDG_DATA_HOME");
  if (xdg_data_home.empty())
    xdg_data_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".local" PATHSEPSTRING "share" ;

  xdg_data_home += PATHSEPSTRING "gogglesmm" PATHSEPSTRING "podcasts";

  if (create)
    FXDir::createDirectories(xdg_data_home);

  return xdg_data_home;
  }

FXString GMApp::getDataDirectory(FXbool create) {
  FXString xdg_data_home = FXSystem::getEnvironment("XDG_DATA_HOME");

  if (xdg_data_home.empty())
    xdg_data_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".local" PATHSEPSTRING "share" ;

  xdg_data_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    FXDir::createDirectories(xdg_data_home);

  return xdg_data_home;
  }


FXString GMApp::getConfigDirectory(FXbool create) {
  FXString xdg_config_home = FXSystem::getEnvironment("XDG_CONFIG_HOME");

  if (xdg_config_home.empty())
    xdg_config_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".config" ;

  xdg_config_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    FXDir::createDirectories(xdg_config_home);

  return xdg_config_home;
  }

FXString GMApp::getCacheDirectory(FXbool create) {
  FXString xdg_cache_home = FXSystem::getEnvironment("XDG_CACHE_HOME");
  if (xdg_cache_home.empty())
    xdg_cache_home=FXSystem::getHomeDirectory()+PATHSEPSTRING ".cache" ;

  xdg_cache_home+=PATHSEPSTRING "gogglesmm";

  if (create)
    FXDir::createDirectories(xdg_cache_home);

  return xdg_cache_home;
  }


void GMApp::init(int& argc,char** argv,FXbool connect) {
  reg().setVendorKey("gogglesmm");
  reg().setAppKey("settings");
  FXApp::init(argc,argv,connect);
#ifdef HAVE_NLS
  FXTranslator * old = getTranslator();
  setTranslator(new GMTranslator());
  if (old) delete old;
#endif
  }

void GMApp::exit(FXint code) {
#ifdef HAVE_OPENGL
  releaseOpenGL();
#endif

  /// Write the new xdg settings file.
  reg().unparseFile(GMApp::getConfigDirectory()+PATHSEPSTRING "settings.rc");
  reg().clear();
  reg().setModified(false);

  FXApp::exit(code);
  }


#ifdef HAVE_OPENGL

FXbool GMApp::hasOpenGL() {
  return (GMPlayerManager::instance()->getPreferences().gui_use_opengl && FXGLVisual::hasOpenGL(this));
  }


void GMApp::initOpenGL() {
  if (glcontext == NULL) {
    glvisual  = new FXGLVisual(this,VISUAL_DOUBLE_BUFFER);
    glcontext = new FXGLContext(this,glvisual);

    FXImage * glimage = new FXImage(this);
    glimage->setVisual(glvisual);
    glimage->create();

    if (glcontext->begin(glimage)) {
      if (glewInit()!=GLEW_OK) {   
        fxwarning("failed to initialize opengl extensions");
        }
      glcontext->end();
      }

    delete glimage;
    }
  }

void GMApp::releaseOpenGL() {
  if (glcontext) {
    delete glcontext;
    glcontext=NULL;
    }
  if (glvisual) {
    delete glvisual;
    glvisual=NULL;
    }
  }

#endif









enum {
  XEMBED_EMBEDDED_NOTIFY = 0,
  XEMBED_MODALITY_ON     = 10,
  XEMBED_MODALITY_OFF    = 11,
  XEMBED_REQUEST_FOCUS   = 3
  };


// Get keysym; interprets the modifiers!
static FXuint keysym(FXRawEvent& event){
  KeySym sym=KEY_VoidSymbol;
  char buffer[40];
  XLookupString(&event.xkey,buffer,sizeof(buffer),&sym,NULL);
  return sym;
  }

FXbool GMApp::dispatchEvent(FXRawEvent & ev) {

  /// Handle Global Hotkeys
  if (ev.xany.window==getRootWindow()->id()){

    if (ev.xany.type==KeyPress) {
      //fxmessage("keypress %d %x\n",ev.xkey.keycode,keysym(ev));
      if (GMPlayerManager::instance()->handle_global_hotkeys(keysym(ev)))
        return true;
      }
    else if (ev.xany.type==ClientMessage) {
      if (ev.xclient.message_type==xmanager && ((FXID)ev.xclient.data.l[1])==xsystemtray) {
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

GMPlug::GMPlug(FXApp * app) : FXTopWindow(app,"gogglesmm",NULL,NULL,DECOR_NONE,0,0,1,1,0,0,0,0,0,0) , socket(0) {
  }

GMPlug::~GMPlug(){
  }

FXbool GMPlug::doesOverrideRedirect() const{
  return true;
  }

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
  fix_wm_properties(this);
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



void fix_wm_properties(const FXWindow * window) {
#ifndef WIN32
  XTextProperty textprop;

  FXString host=FXSystem::getHostName();
  /// set the name of the machine on which this application is running
  textprop.value = (unsigned char *)host.text();
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = host.length();
  XSetWMClientMachine((Display*)window->getApp()->getDisplay(),window->id(), &textprop);

  /// Override class hints
  XClassHint hint;
  hint.res_name=(char*)"gogglesmm";
  hint.res_class=(char*)"gogglesmm";
  XSetClassHint((Display*)window->getApp()->getDisplay(),window->id(),&hint);
#endif
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
    data[i+2]=icon->getData()[i];
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




