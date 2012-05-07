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
#include "gmutils.h"
#include "GMIconTheme.h"
#include "GMApp.h"
#include "icons.h"

#include <FXPNGImage.h>
#include <FXPNGIcon.h>
#include <FXJPGImage.h>
#include <FXJPGIcon.h>

#define SMALL_SIZE 16
#define MEDIUM_SIZE 22
#define LARGE_SIZE 48

void GMIconSet::save(FXStream & store) {
  store << name;
  store << dir;
  store << small;
  store << medium;
  store << large;
  }


void GMIconSet::load(FXStream & store) {
  store >> name;
  store >> dir;
  store >> small;
  store >> medium;
  store >> large;
  }



static void init_basedirs(FXStringList & basedirs) {
  FXString userdir  = FXSystem::getHomeDirectory() + PATHSEPSTRING ".icons";
  FXString datadirs = FXSystem::getEnvironment("XDG_DATA_DIRS");
  if (datadirs.empty()) datadirs = "/usr/local/share:/usr/share";

  FXDict pathdict;

  if (FXStat::exists(userdir)) {
    pathdict.insert(userdir.text(),(void*)(FXival)1);
    basedirs.append(userdir);
    }

  FXint no = datadirs.contains(":")+1;
  for (FXint i=0;i<no;i++){
    FXString dir = FXPath::expand(datadirs.section(':',i));
    if (dir.empty()) continue;
    if (dir[dir.length()-1]==PATHSEP)
      dir += "icons";
    else
      dir += PATHSEPSTRING "icons";

    if (pathdict.find(dir.text()) || !FXStat::exists(dir) ) continue;
    basedirs.append(dir);
    pathdict.insert(dir.text(),(void*)(FXival)1);
    }

  if (pathdict.find("/usr/share/pixmaps")==NULL && FXStat::exists("/usr/share/pixmaps"))
    basedirs.append("/usr/share/pixmaps");

#ifdef DEBUG
  fxmessage("basedirs:\n");
  for (FXint i=0;i<basedirs.no();i++){
    fxmessage("\t%s\n",basedirs[i].text());
    }
#endif
  }

static void init_themedict(FXStringList & basedirs,FXStringDict & themedict){
  FXString * dirs=NULL;
  for (FXint i=0;i<basedirs.no();i++) {
    FXint no = FXDir::listFiles(dirs,basedirs[i],"*",FXDir::AllDirs|FXDir::NoParent|FXDir::NoFiles);
    if (no) {
      for (FXint j=0;j<no;j++) {
        if (themedict.find(dirs[j].text())==NULL && !FXStat::isLink(basedirs[i]+PATHSEPSTRING+dirs[j])) {
          FXString index = basedirs[i]+PATHSEPSTRING+dirs[j]+PATHSEPSTRING+"index.theme";
          if (FXStat::exists(index)) {
            themedict.insert(dirs[j].text(),index.text());
            }
          }
        }
      delete [] dirs;
      dirs=NULL;
      }
    }
#ifdef DEBUG
  fxmessage("themes:\n");
  for (FXint i=themedict.first();i<themedict.size();i=themedict.next(i)){
    fxmessage("\t%s\n",themedict.key(i));
    }
#endif
  }

#if 0
class XDGIconDir {
public:
  enum {
    Scalable,
    Fixed,
    Threshold
    };
public:
  FXString path;
  FXint    size;
  FXint    threshold;
  FXint    min;
  FXint    max;
  FXuchar  type;
public:
  XDGIconDir();
  void init(const FXString&,FXSettings*);

  void debug();
  };


XDGIconDir::XDGIconDir() : size(0),threshold(2),min(0),max(0),type(Threshold) {
  }

void XDGIconDir::init(const FXString & subdir,FXSettings * index){
  path                     = subdir;
  size                     = index->readIntEntry(subdir,"Size",0);
  threshold                = index->readIntEntry(subdir,"Threshold",2);
  min                      = index->readIntEntry(subdir,"MinSize",size);
  max                      = index->readIntEntry(subdir,"MaxSize",size);
  const FXchar * typestr   = index->readStringEntry(subdir,"Type","Threshold");

  if (comparecase(typestr,"scalable")==0)
    type=Scalable;
  else if (comparecase(typestr,"fixed")==0)
    type=Fixed;
  else
    type=Threshold;
  }


void XDGIconDir::debug() {
  fxmessage("\t\t\t\t%s - %d - %d - (%d , %d) - ",path.text(),size,threshold,min,max);
  switch(type){
    case Scalable: fxmessage("Scalable\n"); break;
    case Fixed: fxmessage("Fixed\n"); break;
    case Threshold: fxmessage("Threshold\n"); break;
    }
  }


class XDGIconTheme;

typedef FXArray<XDGIconTheme> XDGIconThemeList;

class XDGIconTheme {
public:
  FXString name;
  FXString dir;
  FXbool   hidden;
public:
  XDGIconTheme *      parent;
  FXStringList        basedirs;
  FXArray<XDGIconDir> subdirs;
protected:
  void parseDirectories(FXSettings*);
  void init(const FXchar * theme,FXSettings *,const FXStringList & pathlist);
public:
  XDGIconTheme();
  ~XDGIconTheme();

  void debug();

  static void setup(XDGIconThemeList &);
  };



XDGIconTheme::XDGIconTheme() : parent(NULL) {
  }

XDGIconTheme::~XDGIconTheme(){
  }

void XDGIconTheme::debug() {
  FXint i;
  fxmessage("---\n");
  fxmessage("\t    Name: %s\n",name.text());
  fxmessage("\t     Dir: %s\n",dir.text());
  fxmessage("\t  Hidden: %d\n",hidden ? 1 : 0);
  fxmessage("\t  Parent: %s\n",parent ? parent->name.text() : "");
  fxmessage("\tBasedirs:\n");
  for (i=0;i<basedirs.no();i++) {
    fxmessage("\t\t\t\t%s\n",basedirs[i].text());
    }
  fxmessage("\t Subdirs:\n");
  for (i=0;i<subdirs.no();i++) {
    subdirs[i].debug();
    }
  }


void XDGIconTheme::parseDirectories(FXSettings * index,const FXString & pathlist) {
  FXString item;
  FXint beg,end;
  for(beg=0; pathlist[beg]; beg=end){
    while(pathlist[beg]==',') beg++;
    for(end=beg; pathlist[end] && pathlist[end]!=','; end++){}
    if(beg==end) break;
    item=pathlist.mid(beg,end-beg);
    if (!item.empty()) {
      subdirs.no(subdirs.no()+1);
      subdirs[subdirs.no()-1].init(item,index);
      }
    }
  }

void XDGIconTheme::init(const FXchar * theme,FXSettings * index,const FXStringList & pathlist) {
  name   = index->readStringEntry("Icon Theme","Name",theme);
  hidden = index->readBoolEntry("Icon Theme","Hidden",false);
  dir    = theme;
  parent = NULL;
  for (FXint i=0;i<pathlist.no();i++) {
    if (FXStat::exists(FXPath::absolute(FXPath::expand(pathlist[i]),theme)))
      basedirs.append(pathlist[i]);
    }
  parseDirectories(index);
  }


void XDGIconTheme::setup(XDGIconThemeList & themelist) {
  FXStringList basedirs;
  FXStringDict themedict;
  FXDict       indexmap;

  init_basedirs(basedirs);
  init_themedict(basedirs,themedict);

  if (themedict.no()==0)
    return;

  GMSettings * index    = new GMSettings[themedict.no()];
  FXDict     * inherits = new FXDict[themedict.no()];

  /// Parse Index Files
  for (FXint i=themedict.first(),j=0;i!=themedict.last();i=themedict.next(i)){
    const FXchar * themedir  = themedict.key(i);
    const FXchar * themefile = themedict.data(i);
    index[j++].parseFile(themefile,true);
    indexmap.insert(themedir,(void*)(FXival)(j));
    }

  themelist.no(themedict.no());
  for (FXint i=themedict.first(),j=0;i!=themedict.last();i=themedict.next(i),j++){
    const FXchar * themedir  = themedict.key(i);
    themelist[j].init(themedir,&index[j],basedirs)
    }
  }


#endif








































void gm_set_application_icon(FXWindow * window) {
  FXPNGImage * image = new FXPNGImage(FXApp::instance(),gogglesmm_32_png,0,0);
  ewmh_set_window_icon(window,image);
  delete image;
  }



static const FXuint CACHE_FILE_VERSION = 20101108;
static const FXchar CACHE_FILE_NAME[]  = PATHSEPSTRING "icontheme.cache";
static const FXchar CACHE_SVG_NAME[] = PATHSEPSTRING "svg";


void GMIconTheme::save_cache() {
  const FXuint cache_file_version = CACHE_FILE_VERSION;

  FXString dirs;
  for (FXint i=0;i<basedirs.no();i++) {
    dirs+=basedirs[i] + ":";
    }

  FXFileStream store;
  if (store.open(GMApp::getCacheDirectory(true)+CACHE_FILE_NAME,FXStreamSave)) {
    store << cache_file_version;
    store << dirs;
    store << smallsize;
    store << mediumsize;
    store << largesize;
    store << (FXint)iconsets.no();
    for (FXint i=0;i<iconsets.no();i++) {
      iconsets[i].save(store);
      }
    }
 }


FXbool GMIconTheme::load_cache() {
  FXString cache_file = GMApp::getCacheDirectory()+CACHE_FILE_NAME;
  FXString cache_dirs,dirs;
  FXuint   cache_file_version;
  FXint    cache_size;
  FXint    n;
  FXTime theme_dir_date=0;
  FXTime cache_dir_date=FXStat::modified(cache_file);

  /// Find last modified date for themedirs
  for (FXint i=0;i<basedirs.no();i++) {
    FXTime tm = FXStat::modified(basedirs[i]);
    if (tm>theme_dir_date) theme_dir_date=tm;
    }

  /// Any dirs newer, then reload.
  if (theme_dir_date==0 || cache_dir_date==0 || theme_dir_date>cache_dir_date)
    return false;

  FXFileStream store;
  if (store.open(GMApp::getCacheDirectory()+PATHSEPSTRING+"icontheme.cache",FXStreamLoad)) {

    store >> cache_file_version;
    if (cache_file_version!=CACHE_FILE_VERSION)
      return false;

    for (FXint i=0;i<basedirs.no();i++) {
      dirs+=basedirs[i] + ":";
      }

    store >> cache_dirs;
    if (dirs!=cache_dirs)
      return false;

    store >> cache_size;
    if (cache_size!=smallsize)
      return false;

    store >> cache_size;
    if (cache_size!=mediumsize)
      return false;

    store >> cache_size;
    if (cache_size!=largesize)
      return false;

    store >> n;
    iconsets.no(n);
    for (FXint i=0;i<iconsets.no();i++){
      iconsets[i].load(store);
      }
#ifdef DEBUG
    fxmessage("loading icon cache succesful\n");
#endif
    return true;
    }
  return false;
  }




#if 0
static debug_iconsets(const GMIconSetList & iconsets){
  for (FXint i=0;i<iconsets.no();i++){
    fxmessage("----------------------------------------------\n");
    fxmessage("Set %d\n",i);
    fxmessage("\tname: %s\n",iconsets[i].name.text());
    fxmessage("\tdir: %s\n",iconsets[i].dir.text());
    fxmessage("\tSmall: %d\n",iconsets[i].smallsize);
    for (FXint j=0;j<iconsets[i].small.no();j++){
      fxmessage("\t\t%s\n",iconsets[i].small[j].text());
      }
    fxmessage("\tMedium: %d\n",iconsets[i].mediumsize);
    for (FXint j=0;j<iconsets[i].medium.no();j++){
      fxmessage("\t\t%s\n",iconsets[i].medium[j].text());
      }
    fxmessage("\tLarge: %d\n",iconsets[i].largesize);
    for (FXint j=0;j<iconsets[i].large.no();j++){
      fxmessage("\t\t%s\n",iconsets[i].large[j].text());
      }
    }
  }
#endif


GMIconTheme * GMIconTheme::me=NULL;

GMIconTheme::GMIconTheme(FXApp * application) : app(application), set(-1),rsvg(false) {
  FXASSERT(me==NULL);
  me=this;

  smallsize  = app->reg().readIntEntry("user-interface","icon-theme-small-size",SMALL_SIZE);
  mediumsize = app->reg().readIntEntry("user-interface","icon-theme-medium-size",MEDIUM_SIZE);
  largesize  = app->reg().readIntEntry("user-interface","icon-theme-large-size",LARGE_SIZE);

  if (!FXPath::search(FXSystem::getEnvironment("PATH"),"rsvg-convert").empty()){
    rsvg=true;
    }

#if 0
  XDGIconThemeList xdg;
  XDGIconTheme::setup(xdg);

  for (FXint i=0;i<xdg.no();i++)
    xdg[i].debug();
#endif

  init_basedirs(basedirs);
  if (!load_cache()) {
    clear_svg_cache();
    build();
    save_cache();
    }

  const FXchar * theme = app->reg().readStringEntry("user-interface","icon-theme","Tango");
  if (theme) {
    for (FXint i=0;i<iconsets.no();i++){
      if (compare(iconsets[i].dir,theme)==0) {
        set=i;
        break;
        }
      }
    }
  if (set==-1 && iconsets.no())
    setCurrentTheme(0);

  cursor_hand=new FXGIFCursor(app,cursor_hand_gif,5,0);
  cursor_hand->create();
  }


GMIconTheme::~GMIconTheme() {
  }

GMIconTheme * GMIconTheme::instance(){
  return me;
  }






static void add_path(const FXStringList & list,const FXString & theme,const FXString & path,FXString & result) {
  for (FXint i=0;i<list.no();i++) {
    FXString p = list[i] + PATHSEPSTRING + theme + PATHSEPSTRING + path;
    if (FXStat::exists(p)) result += ":" + p;
    }

  }





void GMIconTheme::build() {
  FXString parents;
  FXString themedirs;
  FXString base;
  FXString dir;

  FXStringDict themedict;
  FXDict       indexmap;
  FXint        s,i,j,xx;

  init_themedict(basedirs,themedict);

  if (themedict.no()) {
    FXSettings * index    = new FXSettings[themedict.no()];
    FXDict     * inherits = new FXDict[themedict.no()];

    /// Parse Index Files
    for (i=themedict.first(),j=0;i<themedict.size();i=themedict.next(i)){
      const FXchar * themedir  = themedict.key(i);
      const FXchar * themefile = themedict.data(i);
      index[j++].parseFile(themefile,true);
      indexmap.insert(themedir,(void*)(FXival)(j));
      }

    for (i=themedict.first();i<themedict.size();i=themedict.next(i)){
      const FXchar * themedir  = themedict.key(i);
      const FXint            x = (FXint)(FXival)indexmap.find(themedir) - 1;

      if (index[x].readBoolEntry("Icon Theme","Hidden",false))
        continue;

      themedirs = index[x].readStringEntry("Icon Theme","Directories",NULL);
      if (themedirs.empty())
        continue;

      FXString smallpath;
      FXString mediumpath;
      FXString largepath;


      base = themedir;
      for(xx=x;xx>=0;) {
        themedirs = index[xx].readStringEntry("Icon Theme","Directories",NULL);
        parents   = index[xx].readStringEntry("Icon Theme","Inherits","hicolor");

        inherits[x].insert(base.text(),(void*)(FXival)1);

        for (s=0;;s++) {

          dir = themedirs.section(',',s);
          if (dir.empty()) break;

          const FXchar * type   = index[xx].readStringEntry(dir.text(),"Type","Threshold");
          const FXint size      = index[xx].readIntEntry(dir.text(),"Size",0);
          const FXint threshold = index[xx].readIntEntry(dir.text(),"Threshold",2);
          const FXint minsize   = index[xx].readIntEntry(dir.text(),"MinSize",size);
          const FXint maxsize   = index[xx].readIntEntry(dir.text(),"MaxSize",size);

          if (comparecase(type,"scalable")==0) {
            if (smallsize>=minsize && smallsize<=maxsize)
              add_path(basedirs,base,dir,smallpath);
            if (mediumsize>=minsize && mediumsize<=maxsize)
              add_path(basedirs,base,dir,mediumpath);
            if (largesize>=minsize && largesize<=maxsize)
              add_path(basedirs,base,dir,largepath);
            }
          else if (comparecase(type,"fixed")==0) {
            if (size==smallsize)
              add_path(basedirs,base,dir,smallpath);
            else if (size==mediumsize)
              add_path(basedirs,base,dir,mediumpath);
            else if (size==largesize)
              add_path(basedirs,base,dir,largepath);
            }
          else {
            if (FXABS(smallsize-size)<=threshold)
              add_path(basedirs,base,dir,smallpath);
            else if (FXABS(mediumsize-size)<=threshold)
              add_path(basedirs,base,dir,mediumpath);
            else if (FXABS(largesize-size)<=threshold)
              add_path(basedirs,base,dir,largepath);
            }
          }

        /// Find next inherited
        for (s=0,xx=-1;xx==-1;s++) {
          base = parents.section(',',s);
          if (base.empty() || inherits[x].find(base.text()))
            break;
          xx = (FXint)(FXival)indexmap.find(base.text()) - 1;
          }
        }

      if (smallpath.empty() && mediumpath.empty() && largepath.empty())
        continue;

      /// Finally add the theme
      const FXint current=iconsets.no();
      iconsets.no(current+1);
      iconsets[current].name        = index[x].readStringEntry("Icon Theme","Name",themedir);
      iconsets[current].dir         = themedir;
      iconsets[current].small.adopt(smallpath);
      iconsets[current].medium.adopt(mediumpath);
      iconsets[current].large.adopt(largepath);
      }

    delete [] index;
    delete [] inherits;
    }
  }



void GMIconTheme::clear_svg_cache() {
#ifdef DEBUG
  fxmessage("clear svg cache\n");
#endif
  FXFile::removeFiles(get_svg_cache(),true);
  }

FXString GMIconTheme::get_svg_cache() {
  return GMApp::getCacheDirectory(false) + CACHE_SVG_NAME;
  }



FXImage * GMIconTheme::loadImage(const FXString & filename) {
  FXImage * img=NULL;
  const FXString ext = FXPath::extension(filename);
  FXFileStream store;
  if(store.open(filename,FXStreamLoad,65536)){
    if(comparecase(FXPNGImage::fileExt,ext)==0){
      img=new FXPNGImage(app);
      }
    else if(comparecase(FXJPGImage::fileExt,ext)==0 || comparecase("jpeg",ext)==0){
      img=new FXJPGImage(app);
      }
    else if(comparecase(FXBMPIcon::fileExt,ext)==0){
      img=new FXBMPImage(app);
      }
    else if(comparecase(FXGIFIcon::fileExt,ext)==0){
      img=new FXGIFImage(app);
      }
    else {
      img=NULL;
      }
    if(img){
      if(img->loadPixels(store)) return img;
      delete img;
      img=NULL;
      }
    }
  return NULL;
  }


FXIcon * GMIconTheme::loadIcon(const FXString & filename) {
  FXIcon * icon=NULL;
  const FXString ext = FXPath::extension(filename);
  FXFileStream store;
  if(store.open(filename,FXStreamLoad,65536)){
    if(comparecase(FXPNGImage::fileExt,ext)==0){
      icon=new FXPNGIcon(app);
      }
    else if(comparecase(FXJPGImage::fileExt,ext)==0 || comparecase("jpeg",ext)==0){
      icon=new FXJPGIcon(app);
      }
    else if(comparecase(FXBMPIcon::fileExt,ext)==0){
      icon=new FXBMPIcon(app);
      }
    else if(comparecase(FXGIFIcon::fileExt,ext)==0){
      icon=new FXGIFIcon(app);
      }
    else if(comparecase(FXICOIcon::fileExt,ext)==0 || comparecase("cur",ext)==0){
      icon=new FXICOIcon(app);
      }
    else {
      icon=NULL;
      }
    if(icon){
      icon->setOptions(IMAGE_SHMI|IMAGE_SHMP);
      if(icon->loadPixels(store)) return icon;
      delete icon;
      icon=NULL;
      }
    }
  return NULL;
  }



void GMIconTheme::loadIcon(FXIconPtr & icon,const FXString & pathlist,FXint size,const FXchar * value,const FXColor blendcolor) {
  FXIcon * ic=NULL;
  FXString name,path,item;
  FXint beg,end;

  FXString file = value;
  FXString png  = file + ".png";
  FXString svg  = file + ".svg";

  ///FIXME: Linux/Unix only
  for(beg=0; pathlist[beg]; beg=end){
    while(pathlist[beg]==PATHLISTSEP) beg++;
    for(end=beg; pathlist[end] && pathlist[end]!=PATHLISTSEP; end++){}
    if(beg==end) break;
    item=FXPath::expand(pathlist.mid(beg,end-beg));
    path=FXPath::absolute(item,png);
    if(FXStat::exists(path)){
      name.adopt(path);
      break;
      }
    if (rsvg) {
      path=FXPath::absolute(item,svg);
      if(FXStat::exists(path)){
        FXString dest   = get_svg_cache() + PATHSEPSTRING + FXString::value(size);
        FXString target = dest + PATHSEPSTRING + svg + ".png";
        if (!FXStat::exists(target)) {
          FXDir::createDirectories(dest);
#ifdef DEBUG
          fxmessage("make %s\n",target.text());
#endif
          if (system(FXString::value("rsvg-convert --format=png --width=%d --height=%d -o %s %s\n",size,size,target.text(),path.text()).text())==0){
            name.adopt(target);
            break;
            }
          }
        else {
          name.adopt(target);
          break;
          }
        }
      }
    }

  if (!name.empty())
    ic=loadIcon(name);

  if (ic==NULL)
    ic = new FXIcon(app,NULL,0,IMAGE_OWNED,size,size);

  if (icon) {
    icon->destroy();
    icon->setData(ic->getData(),ic->getOptions(),ic->getWidth(),ic->getHeight());

    ic->setOwned(false);
    delete ic;
    }
  else {
    icon=ic;
    }

  icon->blend(blendcolor);
  icon->create();
  }

void GMIconTheme::loadSmall(FXIconPtr & icon,const FXchar * value,const FXColor blendcolor){
  if (iconsets.no())
    loadIcon(icon,iconsets[set].small,smallsize,value,blendcolor);
  else
    loadIcon(icon,FXString::null,smallsize,value,blendcolor);
  }

void GMIconTheme::loadMedium(FXIconPtr & icon,const FXchar * value,const FXColor blendcolor){
  if (iconsets.no())
    loadIcon(icon,iconsets[set].medium,mediumsize,value,blendcolor);
  else
    loadIcon(icon,FXString::null,mediumsize,value,blendcolor);
  }

void GMIconTheme::loadLarge(FXIconPtr & icon,const FXchar * value,const FXColor blendcolor){
   if (iconsets.no())
    loadIcon(icon,iconsets[set].large,largesize,value,blendcolor);
  else
    loadIcon(icon,FXString::null,largesize,value,blendcolor);
  }

void GMIconTheme::loadResource(FXIconPtr & icon,const unsigned char * data,const char * type) {
  FXIconSource source(app);
  FXIcon * newicon = source.loadIconData(data,type);
  FXASSERT(newicon);
  if (icon) {
    icon->destroy();
    icon->setData(newicon->getData(),newicon->getOptions(),newicon->getWidth(),newicon->getHeight());
    newicon->setOwned(false);
    delete newicon;
    }
  else {
    icon=newicon;
    }
  }

FXImage* GMIconTheme::loadSmall(const char * value) {
  FXImage * image = NULL;
  if (iconsets.no()) {
    FXString name = FXPath::search(iconsets[set].small,value);
    if (!name.empty())
      image = loadImage(name);
    }
  return image;
  }


void GMIconTheme::load() {
  const FXColor basecolor = app->getBaseColor();
  const FXColor backcolor = app->getBackColor();

  loadSmall(icon_copy,"edit-copy",basecolor);
  loadSmall(icon_cut,"edit-cut",basecolor);
  loadSmall(icon_paste,"edit-paste",basecolor);
  loadSmall(icon_delete,"edit-delete",basecolor);
  loadSmall(icon_undo,"edit-undo",basecolor);
  loadSmall(icon_play,"media-playback-start",basecolor);
  loadSmall(icon_pause,"media-playback-pause",basecolor);
  loadSmall(icon_next,"media-skip-forward",basecolor);
  loadSmall(icon_prev,"media-skip-backward",basecolor);
  loadSmall(icon_stop,"media-playback-stop",basecolor);

  loadSmall(icon_import,"folder",basecolor);
  loadSmall(icon_importfile,"document-open",basecolor);
  loadSmall(icon_exit,"exit",basecolor);
  loadSmall(icon_close,"window-close",basecolor);
  loadSmall(icon_find,"edit-find",basecolor);
  loadSmall(icon_sync,"view-refresh",basecolor);
  loadSmall(icon_album,"media-optical",basecolor);
  loadSmall(icon_artist,"system-users",basecolor);
  loadSmall(icon_genre,"bookmark-new",basecolor);
  loadSmall(icon_export,"document-save",basecolor);
  loadSmall(icon_homepage,"applications-internet",basecolor);
  loadSmall(icon_info,"help-browser",basecolor);

  loadSmall(icon_fullscreen,"window-fullscreen",backcolor);

  loadSmall(icon_local_file,"text-x-generic",backcolor);
  loadSmall(icon_local_folder,"folder",backcolor);

  loadMedium(icon_source_library,"user-home",backcolor);
  loadMedium(icon_source_internetradio,"applications-internet",backcolor);
  loadMedium(icon_source_playlist,"user-bookmarks",backcolor);
  loadMedium(icon_source_playqueue,"x-office-presentation",backcolor);
  loadMedium(icon_source_local,"drive-harddisk",backcolor);

  loadSmall(icon_playlist,"user-bookmarks",basecolor);
  loadSmall(icon_playqueue,"x-office-presentation",basecolor);
  loadSmall(icon_settings,"preferences-desktop",basecolor);
  loadSmall(icon_edit,"accessories-text-editor",basecolor);
  loadSmall(icon_sort,"view-sort-descending",basecolor);

  loadResource(icon_applogo,gogglesmm_32_png,"png");
  loadResource(icon_applogo_small,gogglesmm_16_png,"png");
//  loadIcon(icon_equalizer,x16_categories_preferencesdesktop_png,"png");

  loadLarge(icon_nocover,"media-optical",backcolor);

  loadMedium(icon_audio_volume_high,"audio-volume-high",basecolor);
  loadMedium(icon_audio_volume_medium,"audio-volume-medium",basecolor);
  loadMedium(icon_audio_volume_low,"audio-volume-low",basecolor);
  loadMedium(icon_audio_volume_muted,"audio-volume-muted",basecolor);
  loadMedium(icon_play_toolbar,"media-playback-start",basecolor);
  loadMedium(icon_pause_toolbar,"media-playback-pause",basecolor);
  loadMedium(icon_next_toolbar,"media-skip-forward",basecolor);
  loadMedium(icon_prev_toolbar,"media-skip-backward",basecolor);
  loadMedium(icon_stop_toolbar,"media-playback-stop",basecolor);

  loadMedium(icon_customize,"preferences-system",basecolor);
  loadMedium(icon_document,"document-properties",basecolor);
  loadMedium(icon_create,"document-open",basecolor);
  loadMedium(icon_media,"applications-multimedia",basecolor);

  icon_applogo->blend(basecolor);
  icon_applogo_small->blend(basecolor);
  icon_applogo->create();
  icon_applogo_small->create();
///  icon_equalizer->blend(basecolor);
 // icon_equalizer->create();
  }


FXint GMIconTheme::getNumThemes() const{
  return iconsets.no();
  }

void GMIconTheme::setCurrentTheme(FXint s) {
  set=s;
  if (set>=0 && iconsets.no())
    app->reg().writeStringEntry("user-interface","icon-theme",iconsets[set].dir.text());
  else
    app->reg().writeStringEntry("user-interface","icon-theme","");

  clear_svg_cache();
  }

FXint GMIconTheme::getCurrentTheme() const {
  return set;
  }

FXString GMIconTheme::getThemeName(FXint i){
  FXASSERT(i>=0 && i<iconsets.no());
  return iconsets[i].name;
  }
