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

  FXDictionary pathdict;

  if (FXStat::exists(userdir)) {
    pathdict.insert(userdir,(void*)(FXival)1);
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

    if (pathdict.has(dir) || !FXStat::exists(dir) ) continue;
    basedirs.append(dir);
    pathdict.insert(dir,(void*)(FXival)1);
    }

  if (pathdict.has("/usr/share/pixmaps") && FXStat::exists("/usr/share/pixmaps"))
    basedirs.append("/usr/share/pixmaps");

#ifdef DEBUG
  fxmessage("basedirs:\n");
  for (FXint i=0;i<basedirs.no();i++){
    fxmessage("\t%s\n",basedirs[i].text());
    }
#endif
  }

static void init_themedict(FXStringList & basedirs,FXStringDictionary & themedict){
  FXString * dirs=NULL;
  for (FXint i=0;i<basedirs.no();i++) {
    FXint no = FXDir::listFiles(dirs,basedirs[i],"*",FXDir::AllDirs|FXDir::NoParent|FXDir::NoFiles);
    if (no) {
      for (FXint j=0;j<no;j++) {
        if (!themedict.has(dirs[j]) && !FXStat::isLink(basedirs[i]+PATHSEPSTRING+dirs[j])) {
          FXString index = basedirs[i]+PATHSEPSTRING+dirs[j]+PATHSEPSTRING+"index.theme";
          if (FXStat::exists(index)) {
            themedict.insert(dirs[j],index);
            }
          }
        }
      delete [] dirs;
      dirs=NULL;
      }
    }
#ifdef DEBUG
  fxmessage("themes:\n");
  for (FXint i=0;i<themedict.no();i++){
    if (!themedict.empty(i))
      fxmessage("\t%s\n",themedict.key(i).text());
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
    FXint n = iconsets.no();
    store << n;
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
  FXFileStream store;
  FXTime theme_dir_date=0;
  FXTime cache_dir_date=FXStat::modified(cache_file);

  /// Find last modified date for themedirs
  for (FXint i=0;i<basedirs.no();i++) {
    FXTime tm = FXStat::modified(basedirs[i]);
    if (tm>theme_dir_date) theme_dir_date=tm;
    }

  /// Any dirs newer, then reload.
  if (theme_dir_date==0 || cache_dir_date==0 || theme_dir_date>cache_dir_date)
    goto failed;

  if (store.open(GMApp::getCacheDirectory()+PATHSEPSTRING+"icontheme.cache",FXStreamLoad)) {

    store >> cache_file_version;
    if (cache_file_version!=CACHE_FILE_VERSION)
      goto failed;

    for (FXint i=0;i<basedirs.no();i++) {
      dirs+=basedirs[i] + ":";
      }

    store >> cache_dirs;
    if (dirs!=cache_dirs)
      goto failed;

    store >> cache_size;
    if (cache_size!=smallsize)
      goto failed;

    store >> cache_size;
    if (cache_size!=mediumsize)
      goto failed;

    store >> cache_size;
    if (cache_size!=largesize)
      goto failed;

    store >> n;
    iconsets.no(n);
    for (FXint i=0;i<iconsets.no();i++){
      iconsets[i].load(store);
      }

    return true;
    }

failed:
  GM_DEBUG_PRINT("GMIconTheme::load_cache() - failed\n");
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

  init_basedirs(basedirs);
  if (!load_cache()) {
    clear_svg_cache();
    build();
    save_cache();
    }


#if 0
  XDGIconThemeList xdg;
  XDGIconTheme::setup(xdg);

  for (FXint i=0;i<xdg.no();i++)
    xdg[i].debug();
#endif
#if 0

  init_basedirs(basedirs);
  if (!load_cache()) {
    clear_svg_cache();
    build();
    save_cache();
    }
#endif
  const FXString theme = app->reg().readStringEntry("user-interface","icon-theme","");
  if (!theme.empty()) {

    for (FXint i=0;i<iconsets.no();i++){
      if (compare(iconsets[i].dir,theme)==0) {
        set=i;
        break;
        }
      }
    }

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

  FXStringDictionary themedict;
  FXDictionary       indexmap;
  FXint              s,i,j,xx;

  init_themedict(basedirs,themedict);

  if (themedict.no()) {
    FXSettings   * index    = new FXSettings[themedict.no()];
    FXDictionary * inherits = new FXDictionary[themedict.no()];

    /// Parse Index Files
    for (i=0,j=0;i<themedict.no();i++){
      if (!themedict.empty(i)){
        index[j++].parseFile(themedict.data(i),true);
        indexmap.insert(themedict.key(i),(void*)(FXival)(j));
        }
      }

    for (i=0;i<themedict.no();i++){
      if (themedict.empty(i)) continue;

      const FXString themedir  = themedict.key(i);
      const FXint            x = (FXint)(FXival)indexmap.find(themedir.text()) - 1;

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

        inherits[x].insert(base,(void*)(FXival)1);

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
          if (base.empty() || inherits[x].has(base))
            break;
          xx = (FXint)(FXival)indexmap.find(base.text()) - 1;
          }
        }

      if (smallpath.empty() && mediumpath.empty() && largepath.empty())
        continue;

      /// Finally add the theme
      const FXint current=iconsets.no();
      iconsets.no(current+1);
      iconsets[current].name        = index[x].readStringEntry("Icon Theme","Name",themedir.text());
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
  GM_DEBUG_PRINT("GMIconTheme::clear_svg_cache()\n");
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

          GM_DEBUG_PRINT("GMIconTheme::loadIcon() - rsvg-convert %s",target.text());

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

  if (ic==NULL) {
    //fxmessage("%s\n",value);
    ic = new FXIcon(app,NULL,0,IMAGE_OWNED,size,size);
    ic->fill(blendcolor);
    }
//  else {
//    FXFile::copyFiles(name,FXString::value("/home/sxj/gmm/x%d_%s",size,FXPath::name(name).substitute('-','_').text()),true);
//    }


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

void GMIconTheme::loadResource(FXIconPtr & icon,const void * data,const FXColor blendcolor,const char * type) {
  FXIconSource source;
  FXIcon * newicon = source.loadIconData(app,data,type);
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
  icon->blend(blendcolor);
  icon->create();
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
  if (set>=0)
    loadExternal();
  else
    loadInternal();
  }

void GMIconTheme::loadInternal() {
  const FXColor basecolor = app->getBaseColor();
  const FXColor backcolor = app->getBackColor();

  smallsize  = SMALL_SIZE;
  mediumsize = MEDIUM_SIZE;
  largesize  = LARGE_SIZE;

  loadResource(icon_copy,x16_edit_copy_png,basecolor);
  loadResource(icon_cut,x16_edit_cut_png,basecolor);
  loadResource(icon_paste,x16_edit_paste_png,basecolor);
  loadResource(icon_delete,x16_edit_delete_png,basecolor);
  loadResource(icon_undo,x16_edit_undo_png,basecolor);

  loadResource(icon_import,x16_folder_png,basecolor);
  loadResource(icon_exit,x16_exit_png,basecolor);
  loadResource(icon_close,x16_window_close_png,basecolor);
  loadResource(icon_find,x16_edit_find_png,basecolor);
  loadResource(icon_sync,x16_view_refresh_png,basecolor);
  loadResource(icon_album,x16_media_optical_png,basecolor);
  loadResource(icon_artist,x16_system_users_png,basecolor);
  loadResource(icon_genre,x16_bookmark_new_png,basecolor);
  loadResource(icon_export,x16_document_save_png,basecolor);
  loadResource(icon_info,x16_help_browser_png,basecolor);

  loadResource(icon_file_small,x16_text_x_generic_png,backcolor);
  loadResource(icon_file_big,x22_text_x_generic_png,backcolor);
  loadResource(icon_audio_small,x16_audio_x_generic_png,backcolor);
  loadResource(icon_audio_big,x22_audio_x_generic_png,backcolor);
  loadResource(icon_image_small,x16_image_x_generic_png,backcolor);
  loadResource(icon_image_big,x22_image_x_generic_png,backcolor);
  loadResource(icon_folder_open_small,x16_folder_open_png,backcolor);
  loadResource(icon_folder_small,x16_folder_png,backcolor);
  loadResource(icon_folder_big,x22_folder_png,backcolor);


  loadResource(icon_home,x16_go_home_png,basecolor);
  loadResource(icon_playqueue,x16_x_office_presentation_png,basecolor);
  loadResource(icon_settings,x16_preferences_desktop_png,basecolor);
  loadResource(icon_edit,x16_accessories_text_editor_png,basecolor);
  loadResource(icon_sort,x16_view_sort_descending_png,basecolor);

  // Play Back Icons
  loadResource(icon_play,         x16_media_playback_start_png,basecolor);
  loadResource(icon_pause,        x16_media_playback_pause_png,basecolor);
  loadResource(icon_next,         x16_media_skip_forward_png,  basecolor);
  loadResource(icon_prev,         x16_media_skip_backward_png, basecolor);
  loadResource(icon_stop,         x16_media_playback_stop_png, basecolor);
  loadResource(icon_volume_high,  x16_audio_volume_high_png,      basecolor);
  loadResource(icon_volume_medium,x16_audio_volume_medium_png,    basecolor);
  loadResource(icon_volume_low,   x16_audio_volume_low_png,       basecolor);
  loadResource(icon_volume_muted, x16_audio_volume_muted_png,     basecolor);

  loadResource(icon_play_toolbar,         x22_media_playback_start_png,   basecolor);
  loadResource(icon_pause_toolbar,        x22_media_playback_pause_png,   basecolor);
  loadResource(icon_next_toolbar,         x22_media_skip_forward_png,     basecolor);
  loadResource(icon_prev_toolbar,         x22_media_skip_backward_png,    basecolor);
  loadResource(icon_stop_toolbar,         x22_media_playback_stop_png,    basecolor);
  loadResource(icon_volume_high_toolbar,  x22_audio_volume_high_png,      basecolor);
  loadResource(icon_volume_medium_toolbar,x22_audio_volume_medium_png,    basecolor);
  loadResource(icon_volume_low_toolbar,   x22_audio_volume_low_png,       basecolor);
  loadResource(icon_volume_muted_toolbar, x22_audio_volume_muted_png,     basecolor);
  loadResource(icon_customize,            x22_preferences_system_png,     basecolor);
  loadResource(icon_document,             x22_document_properties_png,    basecolor);
  loadResource(icon_create,               x22_document_open_png,          basecolor);
  loadResource(icon_media,                x22_applications_multimedia_png,basecolor);

  loadResource(icon_source_library,x22_user_home_png,backcolor);
  loadResource(icon_source_internetradio,x22_applications_internet_png,backcolor);
  loadResource(icon_source_playlist,x22_folder_png,backcolor);
  loadResource(icon_source_playqueue,x22_x_office_presentation_png,backcolor);
  loadResource(icon_source_local,x22_drive_harddisk_png,backcolor);
  loadResource(icon_source_podcast,x22_applications_rss_xml_png,basecolor);

  loadResource(icon_nocover,x48_media_optical_png,backcolor);
  loadResource(icon_applogo,gogglesmm_32_png,basecolor);
  loadResource(icon_applogo_small,gogglesmm_16_png,basecolor);
  loadResource(icon_progress,x16_process_working_png,basecolor);
  }


void GMIconTheme::loadExternal() {
  const FXColor basecolor = app->getBaseColor();
  const FXColor backcolor = app->getBackColor();

  smallsize  = app->reg().readIntEntry("user-interface","icon-theme-small-size",SMALL_SIZE);
  mediumsize = app->reg().readIntEntry("user-interface","icon-theme-medium-size",MEDIUM_SIZE);
  largesize  = app->reg().readIntEntry("user-interface","icon-theme-large-size",LARGE_SIZE);

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
  //loadSmall(icon_importfile,"document-open",basecolor);
  loadSmall(icon_exit,"exit",basecolor);
  loadSmall(icon_close,"window-close",basecolor);
  loadSmall(icon_find,"edit-find",basecolor);
  loadSmall(icon_sync,"view-refresh",basecolor);
  loadSmall(icon_album,"media-optical",basecolor);
  loadSmall(icon_artist,"system-users",basecolor);
  loadSmall(icon_genre,"bookmark-new",basecolor);
  loadSmall(icon_export,"document-save",basecolor);
  //loadSmall(icon_homepage,"applications-internet",basecolor);
  loadSmall(icon_info,"help-browser",basecolor);

  loadSmall(icon_home,"go-home",basecolor);

  loadMedium(icon_source_library,"user-home",backcolor);
  loadMedium(icon_source_internetradio,"applications-internet",backcolor);
  loadMedium(icon_source_playlist,"user-bookmarks",backcolor);
  loadMedium(icon_source_playqueue,"x-office-presentation",backcolor);
  loadMedium(icon_source_local,"drive-harddisk",backcolor);
  loadMedium(icon_source_podcast,"application-rss+xml",basecolor);

  //loadSmall(icon_playlist,"user-bookmarks",basecolor);
  loadSmall(icon_playqueue,"x-office-presentation",basecolor);
  loadSmall(icon_settings,"preferences-desktop",basecolor);
  loadSmall(icon_edit,"accessories-text-editor",basecolor);
  loadSmall(icon_sort,"view-sort-descending",basecolor);

  loadResource(icon_applogo,gogglesmm_32_png,basecolor);
  loadResource(icon_applogo_small,gogglesmm_16_png,basecolor);

  loadLarge(icon_nocover,"media-optical",backcolor);

  loadMedium(icon_volume_high_toolbar,"audio-volume-high",basecolor);
  loadMedium(icon_volume_medium_toolbar,"audio-volume-medium",basecolor);
  loadMedium(icon_volume_low_toolbar,"audio-volume-low",basecolor);
  loadMedium(icon_volume_muted_toolbar,"audio-volume-muted",basecolor);
  loadMedium(icon_play_toolbar,"media-playback-start",basecolor);
  loadMedium(icon_pause_toolbar,"media-playback-pause",basecolor);
  loadMedium(icon_next_toolbar,"media-skip-forward",basecolor);
  loadMedium(icon_prev_toolbar,"media-skip-backward",basecolor);
  loadMedium(icon_stop_toolbar,"media-playback-stop",basecolor);

  loadMedium(icon_customize,"preferences-system",basecolor);
  loadMedium(icon_document,"document-properties",basecolor);
  loadMedium(icon_create,"document-open",basecolor);
  loadMedium(icon_media,"applications-multimedia",basecolor);


  loadSmall(icon_file_small,"text-x-generic",backcolor);
  loadMedium(icon_file_big,"text-x-generic",backcolor);
  loadSmall(icon_audio_small,"audio-x-generic",backcolor);
  loadMedium(icon_audio_big,"audio-x-generic",backcolor);
  loadSmall(icon_image_small,"image-x-generic",backcolor);
  loadMedium(icon_image_big,"image-x-generic",backcolor);
  loadSmall(icon_folder_open_small,"folder-open",backcolor);
  loadSmall(icon_folder_small,"folder",backcolor);
  loadMedium(icon_folder_big,"folder",backcolor);

  loadSmall(icon_progress,"process-working",basecolor);
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
