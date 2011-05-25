/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
#include <fxkeys.h>
#include "GMTrack.h"
#include "GMApp.h"
#include "GMList.h"
#include "GMAlbumList.h"
#include "GMTrackList.h"
#include "GMTrackView.h"
#include "GMWindow.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMIconTheme.h"
#include "GMClipboard.h"
#include "GMSourceView.h"
#include "GMColumnDialog.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"

#define HIDEBROWSER (FX4Splitter::ExpandBottomLeft)
#define SHOWBROWSER (FX4Splitter::ExpandTopLeft|FX4Splitter::ExpandTopRight|FX4Splitter::ExpandBottomLeft)


static inline FXbool begins_with_keyword(const FXString & t){
  for (FXint i=0;i<GMPlayerManager::instance()->getPreferences().gui_sort_keywords.no();i++){
    if (comparecase(t,GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i],GMPlayerManager::instance()->getPreferences().gui_sort_keywords[i].length())==0) return TRUE;
    }
  return FALSE;
  }






















static FXint album_selectionchanged=-1;
static FXint artist_selectionchanged=-1;
static FXint tag_selectionchanged=-1;

FXbool GMTrackView::reverse_artist=false;
FXbool GMTrackView::reverse_album=false;
FXbool GMTrackView::album_by_year=true;

class GMStaticMenuCheck : public GMMenuCheck {
FXDECLARE(GMStaticMenuCheck)
protected:
  GMStaticMenuCheck();
private:
  GMStaticMenuCheck(const GMStaticMenuCheck&);
  GMStaticMenuCheck &operator=(const GMStaticMenuCheck&);
public:
  long onButtonRelease(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
public:
  GMStaticMenuCheck(FXComposite* p,const FXString& text,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=0);
  virtual ~GMStaticMenuCheck();
  };

FXDEFMAP(GMStaticMenuCheck) GMStaticMenuCheckMap[]={
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,GMStaticMenuCheck::onButtonRelease),
  FXMAPFUNC(SEL_KEYRELEASE,0,GMStaticMenuCheck::onKeyRelease)
  };

FXIMPLEMENT(GMStaticMenuCheck,GMMenuCheck,GMStaticMenuCheckMap,ARRAYNUMBER(GMStaticMenuCheckMap))

GMStaticMenuCheck::GMStaticMenuCheck(){
  }

GMStaticMenuCheck::GMStaticMenuCheck(FXComposite* p,const FXString& text,FXObject* tgt,FXSelector sel,FXuint opts) : GMMenuCheck(p,text,tgt,sel,opts) {
  }

GMStaticMenuCheck::~GMStaticMenuCheck(){
  }


// Released button
long GMStaticMenuCheck::onButtonRelease(FXObject*,FXSelector,void*){
  FXbool active=isActive();
  if(!isEnabled()) return 0;
  if(active){
    setCheck(!check);
    if(target){ target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check); }
    }
  return 1;
  }

// Keyboard release
long GMStaticMenuCheck::onKeyRelease(FXObject*,FXSelector,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled() && (flags&FLAG_PRESSED)){
    FXTRACE((200,"%s::onKeyRelease %p keysym=0x%04x state=%04x\n",getClassName(),this,event->code,event->state));
    if(event->code==KEY_space || event->code==KEY_KP_Space || event->code==KEY_Return || event->code==KEY_KP_Enter){
      flags&=~FLAG_PRESSED;
      setCheck(!check);
      if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)(FXuval)check);
      return 1;
      }
    }
  return 0;
  }


FXDEFMAP(GMTrackView) GMTrackViewMap[]={
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_TOGGLE_BROWSER,GMTrackView::onUpdToggleBrowser),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_TOGGLE_TAGS,GMTrackView::onUpdToggleTags),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_TOGGLE_FILTER,GMTrackView::onUpdToggleFilter),

  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_COPY,GMTrackView::onUpdCopy),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_PASTE,GMTrackView::onUpdPaste),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_SHOW_CURRENT,GMTrackView::onUpdShowCurrent),

  FXMAPFUNCS(SEL_UPDATE,GMTrackView::ID_FILTER_TRACK,GMTrackView::ID_FILTER_LAST,GMTrackView::onUpdFilterMask),
  FXMAPFUNCS(SEL_COMMAND,GMTrackView::ID_FILTER_TRACK,GMTrackView::ID_FILTER_LAST,GMTrackView::onCmdFilterMask),


  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ARTIST_LIST_HEADER,GMTrackView::onCmdSortArtistList),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ALBUM_LIST_HEADER,GMTrackView::onCmdSortAlbumList),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_TAG_LIST_HEADER,GMTrackView::onCmdSortTagList),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_TOGGLE_BROWSER,GMTrackView::onCmdToggleBrowser),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_TOGGLE_TAGS,GMTrackView::onCmdToggleTags),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_TOGGLE_FILTER,GMTrackView::onCmdToggleFilter),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_CLOSE_FILTER,GMTrackView::onCmdToggleFilter),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_COPY,GMTrackView::onCmdCopy),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_PASTE,GMTrackView::onCmdPaste),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_SHOW_CURRENT,GMTrackView::onCmdShowCurrent),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_FILTER,GMTrackView::onCmdFilter),
  FXMAPFUNC(SEL_CHANGED,GMTrackView::ID_FILTER,GMTrackView::onCmdFilter),
  FXMAPFUNC(SEL_TIMEOUT,GMTrackView::ID_FILTER,GMTrackView::onCmdFilter),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_FILTER_MODE,GMTrackView::onCmdFilter),


  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_TAG_LIST,GMTrackView::onCmdTagSelected),
  FXMAPFUNC(SEL_SELECTED,GMTrackView::ID_TAG_LIST,GMTrackView::onCmdTagSelected),
  FXMAPFUNC(SEL_DESELECTED,GMTrackView::ID_TAG_LIST,GMTrackView::onCmdTagSelected),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdArtistSelected),
  FXMAPFUNC(SEL_SELECTED,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdArtistSelected),
  FXMAPFUNC(SEL_DESELECTED,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdArtistSelected),
  FXMAPFUNC(SEL_DOUBLECLICKED,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdArtistSelected),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdAlbumSelected),
  FXMAPFUNC(SEL_SELECTED,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdAlbumSelected),
  FXMAPFUNC(SEL_DESELECTED,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdAlbumSelected),
  FXMAPFUNC(SEL_DOUBLECLICKED,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdAlbumSelected),

  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_TAG_LIST,GMTrackView::onTagContextMenu),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_ARTIST_LIST,GMTrackView::onArtistContextMenu),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_ALBUM_LIST,GMTrackView::onAlbumContextMenu),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_TRACK_LIST,GMTrackView::onTrackContextMenu),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_TRACK_LIST_HEADER,GMTrackView::onTrackHeaderContextMenu),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,GMTrackView::ID_ALBUM_LIST_HEADER,GMTrackView::onAlbumHeaderContextMenu),


  FXMAPFUNC(SEL_KEYPRESS,GMTrackView::ID_TAG_LIST,GMTrackView::onCmdTagKeyPress),
  FXMAPFUNC(SEL_KEYPRESS,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdArtistKeyPress),
  FXMAPFUNC(SEL_KEYPRESS,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdAlbumKeyPress),
  FXMAPFUNC(SEL_KEYPRESS,GMTrackView::ID_TRACK_LIST,GMTrackView::onCmdTrackKeyPress),


  FXMAPFUNC(SEL_DOUBLECLICKED,GMTrackView::ID_TRACK_LIST,GMTrackView::onCmdPlayTrack),

  FXMAPFUNC(SEL_BEGINDRAG,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdBeginDrag),
  FXMAPFUNC(SEL_BEGINDRAG,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdBeginDrag),
  FXMAPFUNC(SEL_BEGINDRAG,GMTrackView::ID_TRACK_LIST,GMTrackView::onCmdBeginDrag),

  FXMAPFUNC(SEL_DRAGGED,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdDragged),
  FXMAPFUNC(SEL_DRAGGED,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdDragged),
  FXMAPFUNC(SEL_DRAGGED,GMTrackView::ID_TRACK_LIST,GMTrackView::onCmdDragged),

  FXMAPFUNC(SEL_ENDDRAG,GMTrackView::ID_ARTIST_LIST,GMTrackView::onCmdEndDrag),
  FXMAPFUNC(SEL_ENDDRAG,GMTrackView::ID_ALBUM_LIST,GMTrackView::onCmdEndDrag),
  FXMAPFUNC(SEL_ENDDRAG,GMTrackView::ID_TRACK_LIST,GMTrackView::onCmdEndDrag),

  FXMAPFUNC(SEL_DND_ENTER,GMTrackView::ID_TRACK_LIST,GMTrackView::onDndTrackEnter),
  FXMAPFUNC(SEL_DND_LEAVE,GMTrackView::ID_TRACK_LIST,GMTrackView::onDndTrackLeave),
  FXMAPFUNC(SEL_DND_MOTION,GMTrackView::ID_TRACK_LIST,GMTrackView::onDndTrackMotion),
  FXMAPFUNC(SEL_DND_DROP,GMTrackView::ID_TRACK_LIST,GMTrackView::onDndTrackDrop),


  FXMAPFUNC(SEL_DND_MOTION,GMTrackView::ID_TAG_LIST,GMTrackView::onDndMotion),
  FXMAPFUNC(SEL_DND_MOTION,GMTrackView::ID_ARTIST_LIST,GMTrackView::onDndMotion),
  FXMAPFUNC(SEL_DND_MOTION,GMTrackView::ID_ALBUM_LIST,GMTrackView::onDndMotion),
  FXMAPFUNC(SEL_DND_DROP,GMTrackView::ID_TAG_LIST,GMTrackView::onDndDrop),
  FXMAPFUNC(SEL_DND_DROP,GMTrackView::ID_ARTIST_LIST,GMTrackView::onDndDrop),
  FXMAPFUNC(SEL_DND_DROP,GMTrackView::ID_ALBUM_LIST,GMTrackView::onDndDrop),

  FXMAPFUNC(SEL_DND_REQUEST,GMTrackView::ID_ARTIST_LIST,GMTrackView::onDndRequest),
  FXMAPFUNC(SEL_DND_REQUEST,GMTrackView::ID_ALBUM_LIST,GMTrackView::onDndRequest),
  FXMAPFUNC(SEL_DND_REQUEST,GMTrackView::ID_TRACK_LIST,GMTrackView::onDndRequest),

  FXMAPFUNCS(SEL_COMMAND,GMTrackView::ID_COLUMN_FIRST,GMTrackView::ID_COLUMN_LAST,GMTrackView::onCmdShowColumn),
  FXMAPFUNCS(SEL_UPDATE,GMTrackView::ID_COLUMN_FIRST,GMTrackView::ID_COLUMN_LAST,GMTrackView::onUpdShowColumn),
  FXMAPFUNCS(SEL_COMMAND,GMTrackView::ID_SORT_FIRST,GMTrackView::ID_SORT_LAST,GMTrackView::onCmdSort),
  FXMAPFUNCS(SEL_UPDATE,GMTrackView::ID_SORT_FIRST,GMTrackView::ID_SORT_LAST,GMTrackView::onUpdSort),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_SORT_REVERSE,GMTrackView::onUpdSortReverse),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_SORT_BROWSE,GMTrackView::onUpdSortBrowse),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_SORT_SHUFFLE,GMTrackView::onUpdSortShuffle),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_SORT_SHUFFLE,GMTrackView::onCmdSortShuffle),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_SORT_BROWSE,GMTrackView::onCmdSortBrowse),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_SORT_DEFAULT,GMTrackView::onCmdSortDefault),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ALBUMS_VIEW_LIST,GMTrackView::onCmdAlbumListView),
  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_ALBUMS_VIEW_BROWSER,GMTrackView::onCmdAlbumListView),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_ALBUMS_VIEW_LIST,GMTrackView::onUpdAlbumListView),
  FXMAPFUNC(SEL_UPDATE,GMTrackView::ID_ALBUMS_VIEW_BROWSER,GMTrackView::onUpdAlbumListView),

  FXMAPFUNC(SEL_COMMAND,GMTrackView::ID_CONFIGURE_COLUMNS,GMTrackView::onCmdConfigureColumns),
  };

FXIMPLEMENT(GMTrackView,FXPacker,GMTrackViewMap,ARRAYNUMBER(GMTrackViewMap))

GMTrackView::GMTrackView() {
  source = NULL;
  }

GMTrackView::GMTrackView(FXComposite* p) : FXPacker(p,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0) , source(NULL) {
  GMScrollFrame * sunkenframe;

#if FOXVERSION < FXVERSION(1,7,0)
  shuffle_seed = (FXuint)FXSystem::now();
#else
  shuffle_seed = (FXuint)FXThread::time();
#endif

  filtermask=FILTER_DEFAULT;

  updateFont();


  filtermenu       = new GMMenuPane(getShell());
                     new GMStaticMenuCheck(filtermenu,tr("Title"),this,ID_FILTER_TRACK);
                     new GMStaticMenuCheck(filtermenu,tr("Artist"),this,ID_FILTER_ARTIST);
                     new GMStaticMenuCheck(filtermenu,tr("Album"),this,ID_FILTER_ALBUM);
                     new GMStaticMenuCheck(filtermenu,tr("Tag"),this,ID_FILTER_TAG);


  filterframe      = new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|PACK_UNIFORM_HEIGHT,0,0,0,0,0,0,0,0);
                     new GMButton(filterframe,tr("\tClose Filter\tClose Filter"),GMIconTheme::instance()->icon_close,this,ID_CLOSE_FILTER,BUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_RIGHT);
                     new GMMenuButton(filterframe,tr("&Find"),GMIconTheme::instance()->icon_find,filtermenu,MENUBUTTON_NOARROWS|FRAME_RAISED|MENUBUTTON_TOOLBAR|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);
  filterfield      = new GMTextField(filterframe,20,this,ID_FILTER,LAYOUT_FILL_X|FRAME_LINE);

  browsersplit     = new FX4Splitter(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FOURSPLITTER_TRACKING);
  tagsplit         = new FX4Splitter(browsersplit,FOURSPLITTER_TRACKING);
  taglistframe     = new GMScrollFrame(tagsplit);
  taglistheader    = new GMHeaderButton(taglistframe,tr("Tags\tPress to change sorting order\tPress to change sorting order"),NULL,this,GMTrackView::ID_TAG_LIST_HEADER,LAYOUT_FILL_X|FRAME_RAISED|JUSTIFY_LEFT);
  taglist          = new GMList(taglistframe,this,ID_TAG_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LIST_EXTENDEDSELECT);

  sunkenframe      = new GMScrollFrame(tagsplit);
  artistlistheader = new GMHeaderButton(sunkenframe,tr("Artists\tPress to change sorting order\tPress to change sorting order"),NULL,this,GMTrackView::ID_ARTIST_LIST_HEADER,LAYOUT_FILL_X|FRAME_RAISED|JUSTIFY_LEFT);
  artistlist       = new GMList(sunkenframe,this,ID_ARTIST_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LIST_EXTENDEDSELECT);

  sunkenframe      = new GMScrollFrame(browsersplit);
  albumlistheader  = new GMHeaderButton(sunkenframe,tr("Albums\tPress to change sorting order\tPress to change sorting order"),NULL,this,GMTrackView::ID_ALBUM_LIST_HEADER,LAYOUT_FILL_X|FRAME_RAISED|JUSTIFY_LEFT);
  albumlist        = new GMAlbumList(sunkenframe,this,ID_ALBUM_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|ALBUMLIST_EXTENDEDSELECT|ALBUMLIST_COLUMNS);

  sunkenframe      = new GMScrollFrame(browsersplit);
  tracklist        = new GMTrackList(sunkenframe,this,ID_TRACK_LIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|TRACKLIST_EXTENDEDSELECT);



//                  for (FXint i=ID_COLUMN_FIRST;i<ID_COLUMN_LAST;i++)
//                     new GMMenuCheck(columnmenu,FXString::null,this,i);
//                     new FXMenuSeparator(columnmenu);


  sortmenu         = new GMMenuPane(getShell());
                     new GMMenuRadio(sortmenu,tr("Browse"),this,ID_SORT_BROWSE);
                     new GMMenuRadio(sortmenu,tr("Shuffle\tCtrl-R"),this,ID_SORT_SHUFFLE);
                     new FXMenuSeparator(sortmenu);

                  for (FXint i=ID_SORT_FIRST;i<ID_SORT_LAST;i++)
                     new GMMenuRadio(sortmenu,FXString::null,this,i);

                     new FXMenuSeparator(sortmenu);
                     new GMMenuCheck(sortmenu,tr("Reverse"),this,ID_SORT_REVERSE);


  columnmenu       = new GMMenuPane(getShell());
                     new GMMenuCommand(columnmenu,tr("&Configure Columns…") ,NULL,this,ID_CONFIGURE_COLUMNS);
                     new GMMenuCascade(columnmenu,tr("&Sort\t\tChange Sorting."),GMIconTheme::instance()->icon_sort,sortmenu);


  gm_set_window_cursor(sortmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  gm_set_window_cursor(columnmenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  gm_set_window_cursor(filtermenu,getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  taglist->setNumVisible(9);
  artistlist->setNumVisible(9);

  taglist->setThickFont(font_listhead);
  artistlist->setThickFont(font_listhead);
  albumlist->setListHeadFont(font_listhead);
  tracklist->setActiveFont(font_listhead);

  albumlist->setListIcon(GMIconTheme::instance()->icon_album);
  albumlist->setCoverThumbs(GMPlayerManager::instance()->getCoverThumbs());

  taglist->dropEnable();
  artistlist->dropEnable();
  albumlist->dropEnable();
  tracklist->dropEnable();

  taglist->setSortFunc(genre_list_sort);
  artistlist->setSortFunc(generic_name_sort);
  albumlist->setSortFunc(album_list_sort);

  taglistheader->setArrowState(ARROW_DOWN);
  artistlistheader->setArrowState(ARROW_DOWN);
  albumlistheader->setArrowState(ARROW_DOWN);

  browsersplit->setBarSize(7);
  tagsplit->setBarSize(7);



  getShell()->getAccelTable()->addAccel(parseAccel("Ctrl-N"),this,FXSEL(SEL_COMMAND,ID_SORT_DEFAULT));
  updateColors();
  }

GMTrackView::~GMTrackView(){
  getApp()->removeTimeout(this,ID_FILTER);
  }


FXbool GMTrackView::focusPrevious() {
  if (browsersplit->getExpanded()==SHOWBROWSER) {
    if (tracklist->hasFocus()){
      albumlist->setFocus();
      }
    else if (albumlist->hasFocus()){
      artistlist->setFocus();
      }
    else if (artistlist->hasFocus()) {
      if (taglistframe->shown()) {
        taglist->setFocus();
        }
      else {
        tracklist->setFocus();
        return false;
        }
      }
    else {
      tracklist->setFocus();
      }
    }
  else {
    tracklist->setFocus();
    }
  return true;
  }



FXbool GMTrackView::focusNext() {
  if (browsersplit->getExpanded()==SHOWBROWSER) {
    if (taglistframe->shown() && taglist->hasFocus()){
      artistlist->setFocus();
      artistlist->makeItemVisible(artistlist->getCurrentItem());      
      }
    else if (artistlist->hasFocus()) {
      albumlist->setFocus();
      albumlist->makeItemVisible(albumlist->getCurrentItem());      
      }
    else if (albumlist->hasFocus()){
      tracklist->setFocus();
      tracklist->makeItemVisible(tracklist->getCurrentItem());      
      }
    else {
      FXbool gotfocus = hasFocus();
      if (taglistframe->shown()) {
        taglist->setFocus();
        taglist->makeItemVisible(taglist->getCurrentItem());              
        }
      else {
        artistlist->setFocus();
        artistlist->makeItemVisible(artistlist->getCurrentItem());
        }
      return !gotfocus;
      }
    }
  else {
    if (!tracklist->hasFocus()) {
      tracklist->setFocus();
      tracklist->makeItemVisible(tracklist->getCurrentItem());      
      return true;
      }
    else {
      return false;
      }
    }
  return true;
  }



void GMTrackView::init_track_context_menu(FXMenuPane * pane,FXbool selected){
  if (selected && source->track_context_menu(pane))
    new FXMenuSeparator(pane);
  new GMMenuCommand(pane,tr("&Configure Columns""…"),NULL,this,GMTrackView::ID_CONFIGURE_COLUMNS);
  new GMMenuCascade(pane,tr("&Sort\t\tChange Sorting."),GMIconTheme::instance()->icon_sort,sortmenu);
  pane->create();
  ewmh_change_window_type(sortmenu,WINDOWTYPE_DROPDOWN_MENU);
  ewmh_change_window_type(pane,WINDOWTYPE_POPUP_MENU);
  }


void GMTrackView::updateFont() {
#if FOXVERSION < FXVERSION(1,7,17)
  FXFontDesc fontdescription;
  getApp()->getNormalFont()->getFontDesc(fontdescription);
#else
  FXFontDesc fontdescription = getApp()->getNormalFont()->getFontDesc();
#endif
  fontdescription.slant  = FXFont::Italic;
  fontdescription.weight = FXFont::Bold;
  //fontdescription.size  -= 10;

  if (font_listhead) {
    font_listhead->destroy();
    font_listhead->setFontDesc(fontdescription);
    font_listhead->create();
    }
  else {
    font_listhead = new FXFont(getApp(),fontdescription);
    font_listhead->create();
    }
  }


void GMTrackView::updateColors(){
  tracklist->setRowColor(GMPlayerManager::instance()->getPreferences().gui_row_color);
  tracklist->setActiveColor(GMPlayerManager::instance()->getPreferences().gui_play_color);
  tracklist->setActiveTextColor(GMPlayerManager::instance()->getPreferences().gui_playtext_color);
  taglist->setRowColor(GMPlayerManager::instance()->getPreferences().gui_row_color);
  artistlist->setRowColor(GMPlayerManager::instance()->getPreferences().gui_row_color);
  }

void GMTrackView::updateIcons(){
/*
  FXint i=0;
  FXIcon * icon_genre = NULL;
  FXIcon * icon_artist = NULL;
  FXIcon * icon_album = NULL;

  if (GMPlayerManager::instance()->getPreferences().gui_show_browser_icons) {
    icon_genre = GMIconTheme::instance()->icon_genre;
    icon_artist = GMIconTheme::instance()->icon_artist;
    icon_album = GMIconTheme::instance()->icon_album;
    }

  for (i=0;i<genrelist->getNumItems();i++){
    genrelist->setItemIcon(i,icon_genre);
    }

  for (i=0;i<artistlist->getNumItems();i++){
    artistlist->setItemIcon(i,icon_artist);
    }

  for (i=0;i<albumlist->getNumItems();i++){
    albumlist->setItemIcon(i,icon_album);
    }
*/
  }


void GMTrackView::clear() {
  taglist->clearItems();
  artistlist->clearItems();
  albumlist->clearItems();
  tracklist->clearItems();
  tracklist->setActiveItem(-1);
  }


void GMTrackView::mark(FXint item,FXbool show/*=true*/) {
  if (source && item>=0){
    source->markCurrent(tracklist->getItem(item));
    if (show) tracklist->setCurrentItem(item);
    }
  if (show) tracklist->setActiveItem(item);
  }

void GMTrackView::showCurrent() {
  source->findCurrent(tracklist,GMPlayerManager::instance()->getSource());
  }

FXint GMTrackView::getCurrent() const{
  return tracklist->getCurrentItem();
  }

//generates a psuedo-random integer between min and max
extern int randint(int min, int max,unsigned int * random_seed);

FXint GMTrackView::getNext(FXbool wrap){
  if (tracklist->getNumItems()==0) {
    return -1;
    }
  else if (tracklist->getNumItems()>2 && GMPlayerManager::instance()->getPreferences().play_shuffle) {
    if (tracklist->getActiveItem()>=0){
      for (FXint i=0,track=0;i<10;i++) {
        track = randint(0,tracklist->getNumItems()-1,&shuffle_seed);
        if (track!=tracklist->getActiveItem()) return track;
        }
      }
    return randint(0,tracklist->getNumItems()-1,&shuffle_seed);
    }
  else if (tracklist->getActiveItem()==tracklist->getNumItems()-1){
    if (GMPlayerManager::instance()->getPreferences().play_repeat==REPEAT_ALL || wrap)
      return 0;
    else {
      if (tracklist->getCurrentItem()==tracklist->getNumItems()-1){
        tracklist->setCurrentItem(0);
        }
      return -1;
      }
    }
  return tracklist->getActiveItem()+1;
  }

FXint GMTrackView::getPrevious(){
  if (tracklist->getNumItems()==0) {
    return -1;
    }
  else if (tracklist->getNumItems()>2 && GMPlayerManager::instance()->getPreferences().play_shuffle) {
    if (tracklist->getActiveItem()>=0){
      for (FXint i=0,track=0;i<10;i++) {
        track = randint(0,tracklist->getNumItems()-1,&shuffle_seed);
        if (track!=tracklist->getActiveItem()) return track;
        }
      }
    return randint(0,tracklist->getNumItems()-1,&shuffle_seed);
    }
  else if (tracklist->getActiveItem()==0) {
    return tracklist->getNumItems()-1;
    }
  else {
    return tracklist->getActiveItem()-1;
    }
  }


FXString GMTrackView::getTrackFilename(FXint item) const {
  FXint track = tracklist->getItemId(item);
  if (source && track!=-1 )
    return source->getTrackFilename(track);
  return FXString::null;
  }


void GMTrackView::getTracks(FXIntList & tracks) const{
  for (FXint i=0;i<tracklist->getNumItems();i++){
    tracks.append(tracklist->getItemId(i));
    }
  }


// Function to sort by name, weight, slant, and size
static int compareindex(const void *a,const void *b){
  register FXint * aa = (FXint*)a;
  register FXint * bb = (FXint*)b;
  if ((*aa) > (*bb)) return 1;
  else if ((*aa) < (*bb)) return -1;
  return 0;
  }



void GMTrackView::getSelectedTags(FXIntList & tags) const{
  if (taglist->getNumItems()==1) {
    if (taglist->isItemSelected(0) && ((FXint)(FXival)taglist->getItemData(0))!=-1)
      tags.append((FXint)(FXival)taglist->getItemData(0));
    }
  else {
    for (FXint i=1;i<taglist->getNumItems();i++){
      if (taglist->isItemSelected(i)) {
        tags.append((FXint)(FXival)taglist->getItemData(i));
        }
      }
    }
  qsort(tags.data(),tags.no(),sizeof(FXint),compareindex);
  }


void GMTrackView::getSelectedArtists(FXIntList & artists) const{
  if (artistlist->getNumItems()==1) {
    if (artistlist->isItemSelected(0) && ((FXint)(FXival)artistlist->getItemData(0))!=-1)
      artists.append((FXint)(FXival)artistlist->getItemData(0));
    }
  else {
    for (FXint i=1;i<artistlist->getNumItems();i++){
      if (artistlist->isItemSelected(i)) {
        artists.append((FXint)(FXival)artistlist->getItemData(i));
        }
      }

//      if (artistlist->getNumItems()>1 && artislist->isItemSelected(0))
//        artistlist->deselect


    }
  qsort(artists.data(),artists.no(),sizeof(FXint),compareindex);
  }


void GMTrackView::getSelectedAlbums(FXIntList & albums) const{
  register FXint i=0;
  if (albumlist->getNumItems()) {
    if (albumlist->getItemId(0)==-1) {
      if (albumlist->isItemSelected(0)){
        for (i=1;i<albumlist->getNumItems();i++){
          albumlist->getItem(i)->getIds(albums);
          }
        }
      else {
        for (i=1;i<albumlist->getNumItems();i++){
          if (albumlist->isItemSelected(i)) {
            albumlist->getItem(i)->getIds(albums);
            }
          }
        }
      }
    else {
      for (i=0;i<albumlist->getNumItems();i++){
        if (albumlist->isItemSelected(i)) {
          albumlist->getItem(i)->getIds(albums);
          }
        }
      }
    }
  qsort(albums.data(),albums.no(),sizeof(FXint),compareindex);
  }




void GMTrackView::getSelectedTracks(FXIntList & tracks) const{
  for (FXint i=0;i<tracklist->getNumItems();i++){
    if (tracklist->isItemSelected(i)) {
      tracks.append(tracklist->getItemId(i));
      }
    }
  }


FXint GMTrackView::numTrackSelected() const {
  FXint num=0;
  for (FXint i=0;i<tracklist->getNumItems();i++){
    if (tracklist->isItemSelected(i)) num++;
    }
  return num;
  }

FXbool GMTrackView::trackSelected() const {
  for (FXint i=0;i<tracklist->getNumItems();i++){
    if (tracklist->isItemSelected(i)) return true;
    }
  return false;
  }


FXbool GMTrackView::hasTracks() const {
  return tracklist->getNumItems()>0;
  }

void GMTrackView::selectTagItem(FXint item) {
  taglist->killSelection(true);
  taglist->selectItem(item,true);
  taglist->setCurrentItem(item,true);
  handle(this,FXSEL(SEL_COMMAND,ID_TAG_LIST),(void*)(FXival)item);

  }


void GMTrackView::selectArtistItem(FXint item) {
  artistlist->killSelection(true);
  artistlist->selectItem(item,true);
  artistlist->setCurrentItem(item,true);
  handle(this,FXSEL(SEL_COMMAND,ID_ARTIST_LIST),(void*)(FXival)item);
  }


void GMTrackView::selectAlbumItem(FXint item) {
  albumlist->killSelection(true);
  albumlist->selectItem(item,true);
  albumlist->setCurrentItem(item,true);
  handle(this,FXSEL(SEL_COMMAND,ID_ALBUM_LIST),(void*)(FXival)item);
  }


void GMTrackView::selectTrackItem(FXint item) {
  tracklist->killSelection();
  tracklist->selectItem(item);
  tracklist->setCurrentItem(item);
  }


void GMTrackView::setSource(GMSource * src) {
  if (source!=src) {

    if (source) {

      source->save(tracklist);

      saveSettings(source->settingKey());

      if (browsersplit->getExpanded()==SHOWBROWSER){
        saveSelection(taglist,"genre-list-selection",source->settingKey());
        saveSelection(artistlist,"artist-list-selection",source->settingKey());
        saveSelection(albumlist,"album-list-selection",source->settingKey());
        }
      }

    source=src;

    if (source) {

      columns.clear();
      source->configure(columns);

      loadSettings(source->settingKey());

      //source->show(tracklist);

      clear();

      if (browsersplit->getExpanded()==SHOWBROWSER) {
        listTags();
        initSelection(taglist,"genre-list-selection",source->settingKey());
        listArtists();
        initSelection(artistlist,"artist-list-selection",source->settingKey());
        listAlbums();
        initSelection(albumlist,"album-list-selection",source->settingKey());
        listTracks();
        }
      else {
        listTracks();
        }

      tracklist->setPosition(tracklist_posx,tracklist_posy);
      }
    }
  }

void GMTrackView::saveSelection(GMList * list,const char * key,const FXString & section) const{
  FXString value;
  for (FXint i=0;i<list->getNumItems();i++){
    if (list->isItemSelected(i))
      value+=GMStringVal(i)+";";
    }
  getApp()->reg().writeStringEntry(section.text(),key,value.text());
  }


void GMTrackView::saveSelection(GMAlbumList * list,const char * key,const FXString & section) const{
  FXString value;
  for (FXint i=0;i<list->getNumItems();i++){
    if (list->isItemSelected(i))
      value+=GMStringVal(i)+";";
    }
  getApp()->reg().writeStringEntry(section.text(),key,value.text());
  }


void GMTrackView::initSelection(GMList * list,const FXchar * key,const FXString & section){
  FXint i=0,x=0,nselected=0;
  FXString part;
  FXString view = getApp()->reg().readStringEntry(section.text(),key,"");
  if (!view.empty()){
    list->killSelection(false);
    part=view.section(';',i);
    while(!part.empty()){
#if FOXVERSION >= FXVERSION(1,7,12)
      x = part.toInt();
#else
      x = FXIntVal(part);
#endif
      if (x>=0 && x<list->getNumItems()){
        nselected++;
        list->selectItem(x);
        if (i==0) list->makeItemVisible(x);
        }
      part=view.section(';',++i);
      }
    if (nselected==0 && list->getNumItems()){
      list->selectItem(0);
      }
    }
  }


void GMTrackView::initSelection(GMAlbumList * list,const FXchar * key,const FXString & section){
  FXint i=0,x=0,nselected=0;
  FXString part;
  FXString view = getApp()->reg().readStringEntry(section.text(),key,"");
  if (!view.empty()){
    list->killSelection(false);
    part=view.section(';',i);
    while(!part.empty()){
#if FOXVERSION >= FXVERSION(1,7,12)
      x = part.toInt();
#else
      x = FXIntVal(part);
#endif
      if (x>=0 && x<list->getNumItems()){
        nselected++;
        list->selectItem(x);
        if (i==0) list->makeItemVisible(x);
        }
      part=view.section(';',++i);
      }
    if (nselected==0 && list->getNumItems()){
      list->selectItem(0);
      }
    }
  }


void GMTrackView::init(GMSource * src) {
  FXASSERT(source==NULL);
  FXASSERT(src);

  source=src;


  columns.clear();
  source->configure(columns);

  loadSettings(source->settingKey());

  clear();

  if (source) {
    if (browsersplit->getExpanded()==SHOWBROWSER) {
      listTags();
      initSelection(taglist,"genre-list-selection",source->settingKey());
      listArtists();
      initSelection(artistlist,"artist-list-selection",source->settingKey());
      listAlbums();
      initSelection(albumlist,"album-list-selection",source->settingKey());
      listTracks();
      }
    else {
      listTracks();
      }

    FXint active =  getApp()->reg().readIntEntry("window","track-list-current",-1);
    if (active>=0 && active<tracklist->getNumItems()) {
      tracklist->setCurrentItem(active);
      tracklist->selectItem(active);
      }
    tracklist->setPosition(tracklist_posx,tracklist_posy);
    }
  }



void GMTrackView::saveView() const {
  if (source) {

    source->save(tracklist);

    saveSettings(source->settingKey());

    if (browsersplit->getExpanded()==SHOWBROWSER){
      saveSelection(taglist,"genre-list-selection",source->settingKey());
      saveSelection(artistlist,"artist-list-selection",source->settingKey());
      saveSelection(albumlist,"album-list-selection",source->settingKey());
      }

    getApp()->reg().writeIntEntry("window","track-list-current",tracklist->getActiveItem());
    }
  }


void GMTrackView::redrawAlbumList() {
  albumlist->update();
  }

void GMTrackView::refresh() {
  clear();

  if (source) {
    if (browsersplit->getExpanded()==SHOWBROWSER) {
      fxmessage("begin refresh()\n");
      listTags();
      listArtists();
      listAlbums();
      listTracks();
      fxmessage("done\n");
      }
    else {
      listTracks();
      }
    }

  }


void GMTrackView::resort() {
  sortTags();
  sortArtists();
  sortAlbums();
  sortTracks();
  }


FXbool GMTrackView::listTags() {
  if (source) {

    if (!source->listTags(taglist,GMIconTheme::instance()->icon_genre))
      return false;

    taglist->sortItems();
    if (taglist->getNumItems()>1)
      taglist->prependItem(GMStringFormat(fxtrformat("All %d Tags"),taglist->getNumItems()),GMIconTheme::instance()->icon_genre,(void*)(FXival)-1);
    else
      taglist->prependItem(tr("All Tags"),GMIconTheme::instance()->icon_genre,(void*)(FXival)-1);

    taglist->setCurrentItem(0,false);
    taglist->selectItem(0,false);
    }
  return true;
  }


FXbool GMTrackView::listArtists(){
  if (source) {

    FXIntList tagselection;

    getSelectedTags(tagselection);

    if (!source->listArtists(artistlist,GMIconTheme::instance()->icon_artist,tagselection))
      return false;

    artistlist->sortItems();
    if (artistlist->getNumItems()>1) {
      artistlist->prependItem(GMStringFormat(fxtrformat("All %d Artists"),artistlist->getNumItems()),GMIconTheme::instance()->icon_artist,(void*)(FXival)-1);
      }

    if (GMPlayerManager::instance()->playing() && GMPlayerManager::instance()->getSource()) {
      if (source->findCurrentArtist(artistlist,GMPlayerManager::instance()->getSource())) {
        return true;
        }
      }

    if (artistlist->getNumItems()){
      artistlist->setCurrentItem(0,false);
      artistlist->selectItem(0,false);
      }
    }
  return true;
  }


FXbool GMTrackView::listAlbums(){
  if (source) {

    FXIntList tagselection;
    FXIntList artistselection;

    getSelectedTags(tagselection);
    getSelectedArtists(artistselection);

    if (!source->listAlbums(albumlist,artistselection,tagselection))
      return false;

    albumlist->sortItems();

     if (albumlist->getNumItems()>1){
      albumlist->prependItem(new GMAlbumListItem(GMStringFormat(fxtrformat("All %d Albums"),albumlist->getNumItems()),GMStringFormat(fxtrformat("All %d Albums"),albumlist->getNumItems()),0,-1));
      }

    if (GMPlayerManager::instance()->playing() && GMPlayerManager::instance()->getSource()) {
      if (source->findCurrentAlbum(albumlist,GMPlayerManager::instance()->getSource())) {
        return true;
        }
      }

    if (albumlist->getNumItems()) {
      albumlist->setCurrentItem(0,false);
      albumlist->selectItem(0,false);
      }
    }
  return true;
  }


FXbool GMTrackView::listTracks(){
  if (source) {

    tracklist->setActiveItem(-1);

    FXIntList tagselection;
    FXIntList albumselection;

    if (browsersplit->getExpanded()==SHOWBROWSER){
      getSelectedTags(tagselection);
      getSelectedAlbums(albumselection);
      if (albumselection.no()==0)
        return false;
      }

    if (!source->listTracks(tracklist,albumselection,tagselection))
      return false;

    layout();

    tracklist->setCurrentItem(-1);

    if (tracklist->getNumItems()) {
      sortTracks();
      if (tracklist->getCurrentItem()==-1)
        tracklist->setCurrentItem(0);
      }
    }
  return true;
  }


void GMTrackView::sortTags() const{
  taglist->sortItems();

  /// Make sure "All" is on top.
  FXint data=-1;
  FXint all = taglist->findItemByData((void*)(FXival)(FXint)data);
  if (all>0) {
    taglist->moveItem(0,all);
    }
  }

void GMTrackView::sortArtists() const{
  artistlist->sortItems();

  /// Make sure "All" is on top.
  FXint data=-1;
  FXint all = artistlist->findItemByData((void*)(FXival)(FXint)data);
  if (all>0) {
    artistlist->moveItem(0,all);
    }
  }

void GMTrackView::sortAlbums() const {
  albumlist->sortItems();

  /// Make sure "All" is on top.
  FXint data=-1;
  FXint all = albumlist->findItemById(data);
  if (all>0) {
    albumlist->moveItem(0,all);
    }
  }

void GMTrackView::sortTracks() const{
  if (tracklist->getSortMethod()==HEADER_SHUFFLE)
    source->shuffle(tracklist,sort_seed);
  else
    tracklist->sortItems();

  if (GMPlayerManager::instance()->playing() && GMPlayerManager::instance()->getSource())
    source->findCurrent(tracklist,GMPlayerManager::instance()->getSource());

  }

void GMTrackView::setSortMethod(FXint def,FXbool reverse) {

  /// Tell source about pending sort
  source->sorted(tracklist,def);

  if (def==HEADER_BROWSE) {
    tracklist->setSortMethod(HEADER_BROWSE);
    tracklist->setSortFunc(source->getSortBrowse());
    }
  else if (def==HEADER_SHUFFLE) {
    tracklist->setSortMethod(HEADER_SHUFFLE);
    tracklist->setSortFunc(NULL);
    }
  else {
    for (FXint i=0;i<columns.no();i++){
      if (columns[i].type==def) {
        tracklist->setSortMethod(columns[i].type);
        if (reverse)
          tracklist->setSortFunc(columns[i].descending);
        else
          tracklist->setSortFunc(columns[i].ascending);
        break;
        }
      }
    }
  }

FXbool GMTrackView::getSortReverse() const {
  for (FXint i=0;i<columns.no();i++){
    if (columns[i].type==tracklist->getSortMethod()) {
      if (columns[i].descending==tracklist->getSortFunc())
        return true;
      break;
      }
    }
  return false;
  }



void GMTrackView::loadSettings(const FXString & key) {
  FXbool sort_reverse,shown;
  FXint split;

  sort_reverse = getApp()->reg().readBoolEntry(key.text(),"genre-list-sort-reverse",false);
  if (sort_reverse) {
    taglist->setSortFunc(genre_list_sort_reverse);
    taglistheader->setArrowState(ARROW_UP);
    }
  else {
    taglist->setSortFunc(genre_list_sort);
    taglistheader->setArrowState(ARROW_DOWN);
    }

  reverse_artist = getApp()->reg().readBoolEntry(key.text(),"artist-list-sort-reverse",false);
  if (reverse_artist) {
    artistlist->setSortFunc(generic_name_sort_reverse);
    artistlistheader->setArrowState(ARROW_UP);
    }
  else {
    artistlist->setSortFunc(generic_name_sort);
    artistlistheader->setArrowState(ARROW_DOWN);
    }

  if (getApp()->reg().readBoolEntry(key.text(),"album-list-browser",false)){
    FXuint opts=albumlist->getListStyle();
    albumlist->setListStyle(opts|ALBUMLIST_BROWSER);
    }
  else {
    FXuint opts=albumlist->getListStyle();
    albumlist->setListStyle(opts&~ALBUMLIST_BROWSER);
    }

  album_by_year = getApp()->reg().readBoolEntry(key.text(),"album-list-sort-by-year",false);
  reverse_album = getApp()->reg().readBoolEntry(key.text(),"album-list-sort-reverse",false);
  if (reverse_album) {
    albumlist->setSortFunc(album_list_sort_reverse);
    albumlistheader->setArrowState(ARROW_UP);
    }
  else {
    albumlist->setSortFunc(album_list_sort);
    albumlistheader->setArrowState(ARROW_DOWN);
    }

  shown = getApp()->reg().readBoolEntry(key.text(),"genre-list",false);
  if (shown)
    taglistframe->show();
  else
    taglistframe->hide();

  shown = getApp()->reg().readBoolEntry(key.text(),"browser",source->defaultBrowse());
  if (shown && source->canBrowse())
    browsersplit->setExpanded(SHOWBROWSER);
  else
    browsersplit->setExpanded(HIDEBROWSER);


  split = getApp()->reg().readIntEntry(key.text(),"browser-track-split",-1);
  if (split!=-1) browsersplit->setVSplit(split);

  split = getApp()->reg().readIntEntry(key.text(),"artist-album-split",-1);
  if (split!=-1) browsersplit->setHSplit(split);

  split = getApp()->reg().readIntEntry(key.text(),"genre-artist-split",-1);
  if (split!=-1) tagsplit->setHSplit(split);


//  split = getApp()->reg().readIntEntry(key.text(),"browser-split",-1);
//  if (split!=-1) setSplit(0,split);

  tracklist_posx = getApp()->reg().readIntEntry(key.text(),"track-list-posx",0);
  tracklist_posy = getApp()->reg().readIntEntry(key.text(),"track-list-posy",0);

  shown = getApp()->reg().readBoolEntry(key.text(),"filter",true);
  if (shown && source && source->canFilter())
    filterframe->show();
  else
    filterframe->hide();

  filterfield->setText(getApp()->reg().readStringEntry(key.text(),"filter-text",""));
  if (getApp()->reg().existingEntry(key.text(),"filter-mode")) {
    FXuint mode = getApp()->reg().readIntEntry(key.text(),"filter-mode",0);
    switch(mode){
      case 0 : filtermask=FILTER_TRACK|FILTER_ALBUM|FILTER_ARTIST; break;
      case 1 : filtermask=FILTER_ARTIST; break;
      case 2 : filtermask=FILTER_ALBUM; break;
      case 3 : filtermask=FILTER_TRACK; break;
      default: filtermask=FILTER_DEFAULT; break;
      }
    getApp()->reg().deleteEntry(key.text(),"filter-mode");
#if FOXVERSION < FXVERSION(1,7,0)
    getApp()->reg().writeUnsignedEntry(key.text(),"filter-mask",filtermask);
#else
    getApp()->reg().writeUIntEntry(key.text(),"filter-mask",filtermask);
#endif
    }
  else {
#if FOXVERSION < FXVERSION(1,7,0)
    filtermask = getApp()->reg().readUnsignedEntry(key.text(),"filter-mask",FILTER_DEFAULT);
#else
    filtermask = getApp()->reg().readUIntEntry(key.text(),"filter-mask",FILTER_DEFAULT);
#endif
    }

  if (source) source->setFilter(filterfield->getText().trim().simplify(),filtermask);


  loadTrackSettings(key);
  }



void GMTrackView::loadTrackSettings(const FXString & key) {
  FXString browseprefix = (browsersplit->getExpanded()==SHOWBROWSER) ? "browse" : "list";
  FXString name;
  tracklist->clearHeaders();
  for (FXint i=0;i<columns.no();i++){
    name = columns[i].name;
    name.lower();
    columns[i].show = getApp()->reg().readBoolEntry(key.text(),FXString(browseprefix+"-showcolumn-"+name).text(),(browsersplit->getExpanded()==SHOWBROWSER) ? columns[i].default_browser_show : columns[i].default_show);
    columns[i].size = getApp()->reg().readIntEntry(key.text(),FXString(browseprefix+"-columnwidth-"+name).text(),columns[i].size);
    columns[i].index = getApp()->reg().readIntEntry(key.text(),FXString(browseprefix+"-columnindex-"+name).text(),columns[i].index);
    if (columns[i].show) {
      tracklist->appendHeader(fxtr(columns[i].name.text()),columns[i].size,&columns[i]);
      }
    }
  FXint sort = getApp()->reg().readIntEntry(key.text(),FXString(browseprefix+"-sort-column").text(),source->getSortColumn(browsersplit->getExpanded()==SHOWBROWSER));
  FXbool reverse = getApp()->reg().readBoolEntry(key.text(),FXString(browseprefix+"-sort-reverse").text(),false);

#if FOXVERSION < FXVERSION(1,7,0)
  sort_seed = getApp()->reg().readUnsignedEntry(key.text(),FXString(browseprefix+"-sort-seed").text(),(FXuint)FXSystem::now());
#else
  sort_seed = getApp()->reg().readUIntEntry(key.text(),FXString(browseprefix+"-sort-seed").text(),(FXuint)FXThread::time());
#endif

  setSortMethod(sort,reverse);
  }















void GMTrackView::saveSettings(const FXString & key) const {
  getApp()->reg().writeBoolEntry(key.text(),"genre-list-sort-reverse",taglist->getSortFunc()==genre_list_sort_reverse);
  getApp()->reg().writeBoolEntry(key.text(),"artist-list-sort-reverse",artistlist->getSortFunc()==generic_name_sort_reverse);
  getApp()->reg().writeBoolEntry(key.text(),"album-list-sort-reverse",albumlist->getSortFunc()==album_list_sort_reverse);
  getApp()->reg().writeBoolEntry(key.text(),"album-list-sort-by-year",album_by_year);
  getApp()->reg().writeBoolEntry(key.text(),"album-list-browser",(albumlist->getListStyle()&ALBUMLIST_BROWSER));
  getApp()->reg().writeBoolEntry(key.text(),"genre-list",taglistframe->shown());
  getApp()->reg().writeBoolEntry(key.text(),"browser",browsersplit->getExpanded()==SHOWBROWSER);
  getApp()->reg().writeIntEntry(key.text(),"browser-track-split",browsersplit->getVSplit());
  getApp()->reg().writeIntEntry(key.text(),"artist-album-split",browsersplit->getHSplit());
  getApp()->reg().writeIntEntry(key.text(),"genre-artist-split",tagsplit->getHSplit());
#if FOXVERSION < FXVERSION(1,7,0)
  getApp()->reg().writeIntEntry(key.text(),"track-list-posx",tracklist->getXPosition());
  getApp()->reg().writeIntEntry(key.text(),"track-list-posy",tracklist->getYPosition());
#else
  getApp()->reg().writeIntEntry(key.text(),"track-list-posx",tracklist->getContentX());
  getApp()->reg().writeIntEntry(key.text(),"track-list-posy",tracklist->getContentY());
#endif

  getApp()->reg().writeBoolEntry(key.text(),"filter",filterframe->shown());

  if (filterframe->shown() && !filterfield->getText().empty()) {
    getApp()->reg().writeStringEntry(key.text(),"filter-text",filterfield->getText().text());
#if FOXVERSION < FXVERSION(1,7,0)
    getApp()->reg().writeUnsignedEntry(key.text(),"filter-mask",filtermask);
#else
    getApp()->reg().writeUIntEntry(key.text(),"filter-mask",filtermask);
#endif
    }
  else{
    getApp()->reg().writeStringEntry(key.text(),"filter-text","");
#if FOXVERSION < FXVERSION(1,7,0)
    getApp()->reg().writeUnsignedEntry(key.text(),"filter-mask",FILTER_DEFAULT);
#else
    getApp()->reg().writeUIntEntry(key.text(),"filter-mask",FILTER_DEFAULT);
#endif
    }

  saveTrackSettings(key);
  }


void GMTrackView::saveTrackSettings(const FXString & key) const {
  tracklist->saveHeaders();
  FXString browseprefix = (browsersplit->getExpanded()==SHOWBROWSER) ? "browse" : "list";
  FXString name;
  for (FXint i=0;i<columns.no();i++){
    name = columns[i].name;
    getApp()->reg().writeBoolEntry(key.text(),FXString(browseprefix+"-showcolumn-"+name.lower()).text(),columns[i].show);
    getApp()->reg().writeIntEntry(key.text(),FXString(browseprefix+"-columnwidth-"+name.lower()).text(),columns[i].size);
    getApp()->reg().writeIntEntry(key.text(),FXString(browseprefix+"-columnindex-"+name.lower()).text(),columns[i].index);
    }
  getApp()->reg().writeIntEntry(key.text(),FXString(browseprefix+"-sort-column").text(),tracklist->getSortMethod());
  getApp()->reg().writeBoolEntry(key.text(),FXString(browseprefix+"-sort-reverse").text(),getSortReverse());
#if FOXVERSION < FXVERSION(1,7,0)
  getApp()->reg().writeUnsignedEntry(key.text(),FXString(browseprefix+"-sort-seed").text(),sort_seed);
#else
  getApp()->reg().writeUIntEntry(key.text(),FXString(browseprefix+"-sort-seed").text(),sort_seed);
#endif
  }



long GMTrackView::onCmdShowColumn(FXObject*,FXSelector sel,void*){
  FXint no=FXSELID(sel)-ID_COLUMN_FIRST;

  for (FXint i=0;i<columns.no();i++) {
    if (columns[i].index==no) {
      columns[i].show=!columns[i].show;
      }
    }

  tracklist->clearHeaders();
  for (FXint i=0;i<columns.no();i++){
    if (columns[i].show) {
      tracklist->appendHeader(fxtr(columns[i].name.text()),columns[i].size,&columns[i]);
      }
    }
  return 1;
  }

long GMTrackView::onUpdShowColumn(FXObject*sender,FXSelector sel,void*){
  if (columns.no()) {
    FXint no=FXSELID(sel)-ID_COLUMN_FIRST;
    for (FXint i=0;i<columns.no();i++) {
      if (columns[i].index==no) {
        FXString column = fxtr(columns[i].name.text());
        sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),&column);
        sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SHOW),NULL);
        if (columns[i].show)
          sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
        else
          sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
        return 1;
        }
      }
    }
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),NULL);
  return 1;
  }


long GMTrackView::onCmdSort(FXObject*,FXSelector sel,void*){
  FXint no=FXSELID(sel)-ID_SORT_FIRST;
  setSortMethod(columns[no].type);
  sortTracks();
  return 1;
  }


long GMTrackView::onUpdSort(FXObject*sender,FXSelector sel,void*){
  if (columns.no()) {
    FXMenuCommand * cmd = dynamic_cast<FXMenuCommand*>(sender);
    FXASSERT(cmd);
    FXint no=FXSELID(sel)-ID_SORT_FIRST;
    if (no<columns.no()) {
      cmd->setText(GMStringFormat(fxtrformat("By %s"),fxtr(columns[no].name.text())));
      if (columns[no].type==source->getSortColumn(browsersplit->getExpanded()==SHOWBROWSER))
        cmd->setAccelText("Ctrl-N");
      else
        cmd->setAccelText(FXString::null);

      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SHOW),NULL);
      if (tracklist->getSortMethod()==columns[no].type)
        sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
      else
        sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
      }
    else {
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),NULL);
      }
    }
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_HIDE),NULL);
  return 1;
  }


long GMTrackView::onUpdSortReverse(FXObject*sender,FXSelector,void*){
  for (FXint i=0;i<columns.no();i++){
    if (tracklist->getSortMethod()==columns[i].type) {
      sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
      if (tracklist->getSortFunc()==columns[i].descending)
        sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
      else
        sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
      return 1;
      }
    }
  sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }

long GMTrackView::onCmdSortDefault(FXObject*,FXSelector,void*){
  setSortMethod(source->getSortColumn(browsersplit->getExpanded()==SHOWBROWSER));
  sortTracks();
  return 1;
  }


long GMTrackView::onCmdSortBrowse(FXObject*,FXSelector,void*){
  setSortMethod(HEADER_BROWSE);
  sortTracks();
  return 1;
  }


long GMTrackView::onUpdSortBrowse(FXObject*sender,FXSelector,void*){
  if ((browsersplit->getExpanded()==SHOWBROWSER) && source && source->getSortBrowse()) {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SHOW),NULL);
    if (tracklist->getSortMethod()==HEADER_BROWSE)
      sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);

    FXMenuCommand * cmd = dynamic_cast<FXMenuCommand*>(sender);
    FXASSERT(cmd);
    if (HEADER_BROWSE==source->getSortColumn(browsersplit->getExpanded()==SHOWBROWSER))
      cmd->setAccelText("Ctrl-N");
    else
      cmd->setAccelText(FXString::null);

    }
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_HIDE),NULL);
  return 1;
  }


long GMTrackView::onCmdSortShuffle(FXObject*,FXSelector,void*){
  setSortMethod(HEADER_SHUFFLE);
#if FOXVERSION < FXVERSION(1,7,0)
  sort_seed = (FXuint)FXSystem::now();
#else
  sort_seed = (FXuint)FXThread::time();
#endif
  sortTracks();
  return 1;
  }


long GMTrackView::onUpdSortShuffle(FXObject*sender,FXSelector,void*){
  if (tracklist->getSortMethod()==HEADER_SHUFFLE)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


long GMTrackView::onCmdSortTagList(FXObject*,FXSelector,void*){
  if (taglist->getSortFunc()==genre_list_sort) {
    taglist->setSortFunc(genre_list_sort_reverse);
    taglistheader->setArrowState(ARROW_UP);
    }
  else {
    taglist->setSortFunc(genre_list_sort);
    taglistheader->setArrowState(ARROW_DOWN);
    }
  sortTags();
  return 1;
  }

long GMTrackView::onCmdSortArtistList(FXObject*,FXSelector,void*){
  if (artistlist->getSortFunc()==generic_name_sort) {
    artistlist->setSortFunc(generic_name_sort_reverse);
    reverse_artist=true;
    artistlistheader->setArrowState(ARROW_UP);
    }
  else {
    artistlist->setSortFunc(generic_name_sort);
    reverse_artist=false;
    artistlistheader->setArrowState(ARROW_DOWN);
    }

  sortArtists();

  if (tracklist->getSortMethod()==HEADER_BROWSE)
    sortTracks();
  return 1;
  }


long GMTrackView::onCmdSortAlbumList(FXObject*,FXSelector,void*){
  if (albumlist->getSortFunc()==album_list_sort) {
    albumlist->setSortFunc(album_list_sort_reverse);
    reverse_album=true;
    albumlistheader->setArrowState(ARROW_UP);
    }
  else {
    albumlist->setSortFunc(album_list_sort);
    reverse_album=false;
    albumlistheader->setArrowState(ARROW_DOWN);
    }
  sortAlbums();

  if (tracklist->getSortMethod()==HEADER_BROWSE)
    sortTracks();

  return 1;
  }


long GMTrackView::onCmdTagSelected(FXObject*,FXSelector sel,void*ptr){
  if ( FXSELTYPE(sel)==SEL_COMMAND ) {
    if (tag_selectionchanged>=0) {
      artistlist->clearItems();
      albumlist->clearItems();
      tracklist->clearItems();
      listArtists();
      listAlbums();
      listTracks();
      tag_selectionchanged=-1;
      }
    }
  else {

    /*
      Make sure "All Tag" is not selected at the
      same time as the other items and vice-versa.
    */

    if ( (FXSELTYPE(sel)==SEL_SELECTED) && taglist->getNumItems()>1 ){
      if (((FXint)(FXival)ptr)==0) {
        taglist->killSelection();
        taglist->selectItem(0);
        }
      else if (artistlist->isItemSelected(0)){
        taglist->deselectItem(0);
        }
      }

    if ((FXSELTYPE(sel)==SEL_DESELECTED) && taglist->getNumItems()==1){
      taglist->selectItem(0);
      return 1;
      }

    if (tag_selectionchanged==(FXint)(FXival)ptr)
      tag_selectionchanged=-1;
    else
      tag_selectionchanged=(FXint)(FXival)ptr;
    }
  return 1;
  }


long GMTrackView::onCmdArtistSelected(FXObject*,FXSelector sel,void*ptr){
  if ( FXSELTYPE(sel)==SEL_DOUBLECLICKED) {
    if (GMPlayerManager::instance()->getPreferences().play_from_queue) {
      FXIntList tracks;
      getTracks(tracks);
      GMPlayerManager::instance()->getPlayQueue()->addTracks(source,tracks);
      }
    if (GMPlayerManager::instance()->can_play())
      GMPlayerManager::instance()->play();
    }
  else if ( FXSELTYPE(sel)==SEL_COMMAND ) {
    if (artist_selectionchanged>=0) {
      albumlist->clearItems();
      tracklist->clearItems();
      listAlbums();
      listTracks();
      artist_selectionchanged=-1;
      }
    }
  else {

    /*
      Make sure "All Artist" is not selected at the
      same time as the other items and vice-versa.
    */

    if ( (FXSELTYPE(sel)==SEL_SELECTED) && artistlist->getNumItems()>1 ){
      if (((FXint)(FXival)ptr)==0) {
        artistlist->killSelection();
        artistlist->selectItem(0);
        }
      else if (artistlist->isItemSelected(0)){
        artistlist->deselectItem(0);
        }
      }

    if ((FXSELTYPE(sel)==SEL_DESELECTED) && artistlist->getNumItems()==1){
      artistlist->selectItem(0);
      return 1;
      }


    if (artist_selectionchanged==(FXint)(FXival)ptr)
      artist_selectionchanged=-1;
    else
      artist_selectionchanged=(FXint)(FXival)ptr;
    }
  return 1;
  }


long GMTrackView::onCmdAlbumSelected(FXObject*,FXSelector sel,void*ptr){
  if ( FXSELTYPE(sel)==SEL_DOUBLECLICKED) {
    if (GMPlayerManager::instance()->getPreferences().play_from_queue) {
      FXIntList tracks;
      getTracks(tracks);
      GMPlayerManager::instance()->getPlayQueue()->addTracks(source,tracks);
      }
    if (GMPlayerManager::instance()->can_play())
      GMPlayerManager::instance()->play();
    }
  else if ( FXSELTYPE(sel)==SEL_COMMAND ) {
    if (album_selectionchanged>=0) {
      tracklist->clearItems();
      listTracks();
      album_selectionchanged=-1;
      }
    }
  else {
    /*
      Make sure "All Albums" is not selected at the
      same time as the other items and vice-versa.
    */
    if ( (FXSELTYPE(sel)==SEL_SELECTED) && albumlist->getNumItems()>1 ){
      if (((FXint)(FXival)ptr)==0) {
        albumlist->killSelection();
        albumlist->selectItem(0);
        }
      else if (albumlist->isItemSelected(0)){
        albumlist->deselectItem(0);
        }
      }
    if (album_selectionchanged==(FXint)(FXival)ptr)
      album_selectionchanged=-1;
    else
      album_selectionchanged=(FXint)(FXival)ptr;
    }
  return 1;
  }


long GMTrackView::onTagContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  if (source && !event->moved) {
    FXint item = taglist->getItemAt(event->win_x,event->win_y);
    if (item>=0 && getTag(item)!=-1) {
      GMMenuPane pane(this);
      if (source->genre_context_menu(&pane)) {
        selectTagItem(item);
        pane.create();
        ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
        pane.popup(NULL,event->root_x+3,event->root_y+3);
        getApp()->runPopup(&pane);
        }
      }
    return 1;
    }
  return 0;
  }

long GMTrackView::onArtistContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  if (source && !event->moved) {
    FXint item = artistlist->getItemAt(event->win_x,event->win_y);
    if (item>=0 && getArtist(item)!=-1) {
      GMMenuPane pane(this);
      if (source->artist_context_menu(&pane)) {
        selectArtistItem(item);
        selectAlbumItem(0);
        pane.create();
        ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
        pane.popup(NULL,event->root_x+3,event->root_y+3);
        getApp()->runPopup(&pane);
        }
      }
    return 1;
    }
  return 0;
  }


long GMTrackView::onAlbumContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  FXbool old        = album_by_year;
  FXbool old_merge  = GMPlayerManager::instance()->getPreferences().gui_merge_albums;
  FXDataTarget target_yearsort(album_by_year);
  FXDataTarget target_merge(GMPlayerManager::instance()->getPreferences().gui_merge_albums);
  if (source && !event->moved) {
    GMMenuPane pane(this);
    FXint item = albumlist->getItemAt(event->win_x,event->win_y);
    if (item>=0 && getAlbum(item)!=-1 && source->album_context_menu(&pane))
      selectAlbumItem(item);

    new GMMenuCheck(&pane,fxtr("Show Album Year"),albumlist,GMAlbumList::ID_YEAR);
    new GMMenuCheck(&pane,fxtr("Sort by Album Year"),&target_yearsort,FXDataTarget::ID_VALUE);
    new GMMenuCheck(&pane,fxtr("Merge Albums"),&target_merge,FXDataTarget::ID_VALUE);
    new FXMenuSeparator(&pane),
    new GMMenuRadio(&pane,fxtr("List View"),this,ID_ALBUMS_VIEW_LIST);
    new GMMenuRadio(&pane,fxtr("Cover View"),this,ID_ALBUMS_VIEW_BROWSER);
    new GMMenuRadio(&pane,fxtr("Arrange By Rows"),albumlist,GMAlbumList::ID_ARRANGE_BY_ROWS);
    new GMMenuRadio(&pane,fxtr("Arrange By COlumns"),albumlist,GMAlbumList::ID_ARRANGE_BY_COLUMNS);


    pane.create();
    pane.forceRefresh();
    ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
    pane.popup(NULL,event->root_x+3,event->root_y+3);
    getApp()->runPopup(&pane);

    if (old_merge!=GMPlayerManager::instance()->getPreferences().gui_merge_albums){
      refresh();
      }
    else if (old!=album_by_year){
      sortAlbums();
      sortTracks();
      }
    return 1;
    }
  return 0;
  }

long GMTrackView::onAlbumHeaderContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  FXbool old = album_by_year;
  FXDataTarget target_yearsort(album_by_year);

  if (source && !event->moved) {
    GMMenuPane pane(this);
    new GMMenuCheck(&pane,fxtr("Sort by Album Year"),&target_yearsort,FXDataTarget::ID_VALUE);
    pane.create();
    pane.forceRefresh();
    ewmh_change_window_type(&pane,WINDOWTYPE_POPUP_MENU);
    pane.popup(NULL,event->root_x+3,event->root_y+3);
    getApp()->runPopup(&pane);

    if (old!=album_by_year){
      sortAlbums();
      sortTracks();
      }
    return 1;
    }
  return 0;
  }





long GMTrackView::onTrackContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  if (source && !event->moved) {
    FXint item = tracklist->getItemAt(event->win_x,event->win_y);
    GMMenuPane pane(this);

    if (item>=0 && !tracklist->isItemSelected(item))
      selectTrackItem(item);

    init_track_context_menu(&pane,(item>=0));

    gm_run_popup_menu(&pane,event->root_x+3,event->root_y+3);
    return 1;
    }
  return 0;
  }


long GMTrackView::onTrackHeaderContextMenu(FXObject*,FXSelector,void*ptr){
  FXEvent * event = reinterpret_cast<FXEvent*>(ptr);
  if (source && !event->moved) {
    columnmenu->create();
    ewmh_change_window_type(columnmenu,WINDOWTYPE_POPUP_MENU);
    columnmenu->popup(NULL,event->root_x+3,event->root_y+3);
    getApp()->runPopup(columnmenu);
    return 1;
    }
  return 0;
  }


long GMTrackView::onCmdTagKeyPress(FXObject*,FXSelector,void*ptr){
  FXEvent* event=reinterpret_cast<FXEvent*>(ptr);
  if (event->state&(CONTROLMASK) && (event->code==KEY_A || event->code==KEY_a)) {
    if (taglist->getNumItems()) {
      selectTagItem(0);
      taglist->makeItemVisible(0);
      }
    return 1;
    }
  else if (event->code==KEY_Delete || event->code==KEY_KP_Delete) {
    if (source && taglist->getNumItems()) {
      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_DELETE_TAG),NULL);
      }
    return 1;
    }
//  else if (event->code==KEY_F2) {
//    if (source && genrelist->getNumItems()) {
//      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_EDIT_TAGS),NULL);
//      }
//    return 1;
//    }
  return 0;
  }

long GMTrackView::onCmdArtistKeyPress(FXObject*,FXSelector,void*ptr){
  FXEvent* event=reinterpret_cast<FXEvent*>(ptr);
  if (event->state&(CONTROLMASK) && (event->code==KEY_A || event->code==KEY_a)) {
    if (artistlist->getNumItems()) {
      selectArtistItem(0);
      artistlist->makeItemVisible(0);
      }
    return 1;
    }
  else if (event->code==KEY_Delete || event->code==KEY_KP_Delete) {
    if (source && artistlist->getNumItems()) {
      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_DELETE_ARTIST),NULL);
      }
    return 1;
    }
/*
  else if (event->code==KEY_Menu) {
    FXDataTarget target_yearsort(album_by_year);
    GMMenuPane pane(this);
    new GMMenuCheck(&pane,fxtr("Sort by Album Year"),&target_yearsort,FXDataTarget::ID_VALUE);
    FXint rootx,rooty;
    albumlist->translateCoordinatesTo(rootx,rooty,getApp()->getRootWindow(),0,0);
    gm_run_popup_menu(&pane,rootx-1,rooty-1);
    return 1;
    }
*/

//  else if (event->code==KEY_F2) {
//    if (source && artistlist->getNumItems()) {
//      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_EDIT_ARTIST),NULL);
//      }
//    return 1;
//    }
  else if (!(event->state&CONTROLMASK)){
    FXString text;
    FXint a;
    for (FXint i=0;i<artistlist->getNumItems();i++){
      text=artistlist->getItemText(i);
      if (begins_with_keyword(text))
        a=FXMIN(text.length()-1,text.find(' ')+1);
      else
        a=0;
      if (comparecase(&text[a],event->text,1)==0){
        selectArtistItem(i);
        artistlist->makeItemVisible(i);
        break;
        }
      }
    }
  return 0;
  }


long GMTrackView::onCmdAlbumKeyPress(FXObject*,FXSelector,void*ptr){
  FXEvent* event=reinterpret_cast<FXEvent*>(ptr);
  if (event->state&(CONTROLMASK) && (event->code==KEY_A || event->code==KEY_a)) {
    if (albumlist->getNumItems()) {
      selectAlbumItem(0);
      albumlist->makeItemVisible(0);
      }
    return 1;
    }
  else if (event->code==KEY_Delete || event->code==KEY_KP_Delete) {
    if (source && albumlist->getNumItems()) {
      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_DELETE_ALBUM),NULL);
      }
    return 1;
    }
  else if (event->code==KEY_Menu) {
#if 0
    FXDataTarget target_yearsort(album_by_year);
    GMMenuPane pane(this);
    new GMMenuCheck(&pane,fxtr("Sort by Album Year"),&target_yearsort,FXDataTarget::ID_VALUE);
    FXint rootx,rooty;
    albumlist->translateCoordinatesTo(rootx,rooty,getApp()->getRootWindow(),0,0);
    gm_run_popup_menu(&pane,rootx-1,rooty-1);
#endif
    return 1;
    }


//  else if (event->code==KEY_F2) {
//    if (source && albumlist->getNumItems()) {
//      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_EDIT_ALBUM),NULL);
//      }
//    return 1;
//    }
  return 0;
  }


long GMTrackView::onCmdTrackKeyPress(FXObject*,FXSelector,void*ptr){
  FXEvent* event=reinterpret_cast<FXEvent*>(ptr);
  if (event->state&(CONTROLMASK) ) {
    if (event->code==KEY_A || event->code==KEY_a) {
      if (tracklist->getNumItems()){
        tracklist->setAnchorItem(0);
        tracklist->selectItem(0);
        tracklist->extendSelection(tracklist->getNumItems()-1);
        }
      return 1;
      }
    else if (event->code==KEY_C || event->code==KEY_c) {
      if (source && numTrackSelected() ) {
        source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_COPY_TRACK),NULL);
        }
      return 1;
      }
    }
  else if (event->code==KEY_Delete || event->code==KEY_KP_Delete) {
    if (source && numTrackSelected() ) {
      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_DELETE_TRACK),NULL);
      }
    return 1;
    }
  else if (event->code==KEY_F2) {
    if (source && numTrackSelected()) {
      source->handle(this,FXSEL(SEL_COMMAND,GMSource::ID_EDIT_TRACK),NULL);
      }
    return 1;
    }
  else if (event->code==KEY_Menu) {
    GMMenuPane pane(this);
    init_track_context_menu(&pane,trackSelected());
    FXint rootx,rooty;
    tracklist->translateCoordinatesTo(rootx,rooty,getApp()->getRootWindow(),0,0);
    gm_run_popup_menu(&pane,rootx-1,rooty-1);
    return 1;
    }

  else {
    // toggle filter and send keystroke to textfield
    //handle(this,FXSEL(SEL_COMMAND,ID_TOGGLE_FILTER),NULL);
    }
  return 0;
  }


long GMTrackView::onCmdPlayTrack(FXObject*,FXSelector,void*){
  if (!source->track_double_click()) {


  if (GMPlayerManager::instance()->getPlayQueue()) {
    FXIntList tracks;
    tracks.append(tracklist->getItemId(tracklist->getCurrentItem()));
    GMPlayerManager::instance()->getPlayQueue()->addTracks(source,tracks);
    if (GMPlayerManager::instance()->can_play())
      GMPlayerManager::instance()->play();
    }
  else {
    GMPlayerManager::instance()->play();
    tracklist->deselectItem(tracklist->getCurrentItem());
    }

    }
  return 1;
  }

long GMTrackView::onCmdShowCurrent(FXObject*,FXSelector,void*){
  if (!source->findCurrent(tracklist,GMPlayerManager::instance()->getSource())){
    if (source->hasCurrentTrack(GMPlayerManager::instance()->getSource())){
      refresh();
      }
    else {
      GMPlayerManager::instance()->getSourceView()->setSource(GMPlayerManager::instance()->getSource());
      if (!source->findCurrent(tracklist,GMPlayerManager::instance()->getSource())){
        if (source->hasCurrentTrack(GMPlayerManager::instance()->getSource())){
          refresh();
          }
        }
      }
    }
  return 1;
  }

long GMTrackView::onUpdShowCurrent(FXObject*sender,FXSelector,void*){
  if (GMPlayerManager::instance()->playing() && GMPlayerManager::instance()->getSource())
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


long GMTrackView::onCmdPaste(FXObject*sender,FXSelector,void*ptr){
  return source && source->handle(sender,FXSEL(SEL_COMMAND,GMSource::ID_PASTE),ptr);
  }

long GMTrackView::onUpdPaste(FXObject*sender,FXSelector,void*ptr){
  return source && source->handle(sender,FXSEL(SEL_UPDATE,GMSource::ID_PASTE),ptr);
  }


long GMTrackView::onCmdCopy(FXObject*sender,FXSelector,void*ptr){
  return source && source->handle(sender,FXSEL(SEL_COMMAND,GMSource::ID_COPY_TRACK),ptr);
  }

long GMTrackView::onUpdCopy(FXObject*sender,FXSelector,void*){
  if (trackSelected() && source)
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }


long GMTrackView::onCmdFilter(FXObject*,FXSelector sel,void*){
  if (FXSELID(sel)==ID_FILTER_MODE && filterfield->getText().empty())
    return 1;
  if (FXSELTYPE(sel)==SEL_CHANGED) {
    getApp()->addTimeout(this,ID_FILTER,TIME_MSEC(500));
    return 1;
    }
  if (FXSELTYPE(sel)==SEL_COMMAND){
    getApp()->removeTimeout(this,ID_FILTER);
    }
  if (source->setFilter(filterfield->getText().trim().simplify(),filtermask))
    refresh();
  return 1;
  }

long GMTrackView::onCmdFilterMask(FXObject*,FXSelector sel,void*){
  switch(FXSELID(sel)){
    case ID_FILTER_TRACK  : filtermask ^= FILTER_TRACK; break;
    case ID_FILTER_ALBUM  : filtermask ^= FILTER_ALBUM; break;
    case ID_FILTER_ARTIST : filtermask ^= FILTER_ARTIST; break;
    case ID_FILTER_TAG    : filtermask ^= FILTER_TAG; break;
    }
  getApp()->addTimeout(this,ID_FILTER,TIME_MSEC(500));
  return 1;
  }


long GMTrackView::onUpdFilterMask(FXObject*sender,FXSelector sel,void*){
  FXbool check=false;
  switch(FXSELID(sel)){
    case ID_FILTER_TRACK : check = (filtermask&FILTER_TRACK)!=0; break;
    case ID_FILTER_ALBUM : check = (filtermask&FILTER_ALBUM)!=0; break;
    case ID_FILTER_ARTIST: check = (filtermask&FILTER_ARTIST)!=0; break;
    case ID_FILTER_TAG   : check = (filtermask&FILTER_TAG)!=0; break;
    }
  if (check)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }



long GMTrackView::onCmdBeginDrag(FXObject*sender,FXSelector sel,void*){
  FXWindow * window = (FXWindow*)sender;
  if (FXSELID(sel)==ID_ALBUM_LIST) {

    /* we don't seem to get a SEL_COMMAND message when we start dragging */
    if (album_selectionchanged>=0) {
      tracklist->clearItems();
      listTracks();
      album_selectionchanged=-1;
      }

    FXDragType types[]={GMClipboard::kdeclipboard,GMClipboard::urilistType,GMClipboard::alltracks};
    if (window->beginDrag(types,3)){
      }
    }
  else if (FXSELID(sel)==ID_TRACK_LIST){
    FXDragType types[4];
    FXuint ntypes=source->dnd_provides(types);
    if (ntypes) window->beginDrag(types,ntypes);
    }
  else if (FXSELID(sel)==ID_ARTIST_LIST){

    /* we don't seem to get a SEL_COMMAND message when we start dragging */
    if (artist_selectionchanged>=0) {
      albumlist->clearItems();
      tracklist->clearItems();
      listAlbums();
      listTracks();
      artist_selectionchanged=-1;
      }
    FXDragType types[]={GMClipboard::kdeclipboard,GMClipboard::urilistType,GMClipboard::alltracks};
    if (window->beginDrag(types,3)){
      }
    }
  else {
    return 0;
    }
  return 1;
  }

long GMTrackView::onCmdDragged(FXObject*sender,FXSelector,void*ptr){
  FXWindow * window = (FXWindow*)sender;
  FXEvent* event=(FXEvent*)ptr;
  FXDragAction action=DRAG_COPY;
  if(event->state&ALTMASK) action=DRAG_LINK;
  window->handleDrag(event->root_x,event->root_y,action);
  action=window->didAccept();
  if (window->didAccept()!=DRAG_REJECT) {
    if (action==DRAG_MOVE)
      window->setDragCursor(getApp()->getDefaultCursor(DEF_DNDMOVE_CURSOR));
    else if (action==DRAG_LINK)
      window->setDragCursor(getApp()->getDefaultCursor(DEF_DNDLINK_CURSOR));
    else
      window->setDragCursor(getApp()->getDefaultCursor(DEF_DNDCOPY_CURSOR));
    }
  else {
    window->setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
    }
  return 1;
  }

long GMTrackView::onCmdEndDrag(FXObject*sender,FXSelector,void*){
  FXWindow * window = (FXWindow*)sender;
  if (getApp()->getDragWindow()==tracklist)
    window->endDrag(window->didAccept()!=DRAG_REJECT);
  else
    window->endDrag((window->didAccept()!=DRAG_MOVE && window->didAccept()!=DRAG_REJECT));
  window->setDragCursor(window->getDefaultCursor());
  return 1;
  }


long GMTrackView::onDndTrackEnter(FXObject*,FXSelector,void*){
  tracklist_dragging=1;
  tracklist_lastline=-1;
  return 0;
  }


long GMTrackView::onDndTrackLeave(FXObject*,FXSelector,void*){
  tracklist_dragging=0;
  if (tracklist_lastline>=0) {
    tracklist->update(0,tracklist_lastline,tracklist->getHeader()->getDefaultWidth(),1);
    }
  tracklist_lastline=-1;
  return 0;
  }


long GMTrackView::onDndTrackMotion(FXObject*,FXSelector,void*ptr){
  FXEvent* event=(FXEvent*)ptr;
  if (getApp()->getDragWindow()==tracklist) {
    if (tracklist->offeredDNDType(FROM_DRAGNDROP,GMClipboard::selectedtracks)){
      FXint pos_x,pos_y,item;
      tracklist->getPosition(pos_x,pos_y);

      item = (event->win_y-pos_y-tracklist->getHeader()->getDefaultHeight())/tracklist->getLineHeight();

      if (item<0) item=0;
      if (item>=tracklist->getNumItems())  {
        tracklist->acceptDrop(DRAG_REJECT);
        return 1;
        }

      FXint ds   = pos_y+item*tracklist->getLineHeight()+tracklist->getHeader()->getDefaultHeight();
      FXint dm   = ds + (tracklist->getLineHeight()/2);
      FXint dl=0;

      tracklist_dropitem=item;

      if (event->win_y<=dm) {
        dl = ds;
        }
      else {
        dl = ds+tracklist->getLineHeight();
        tracklist_dropitem=FXMIN(item+1,tracklist->getNumItems());
        }

      FXDCWindow dc(tracklist);
      dc.setForeground(FXRGB(0,0,0));
      dc.fillRectangle(0,dl,tracklist->getHeader()->getDefaultWidth(),1);

      if (tracklist_lastline>=0 && (tracklist_lastline!=dl || tracklist_lastposy!=pos_y)) {
        tracklist->update(0,tracklist_lastline-(tracklist_lastposy-pos_y),tracklist->getHeader()->getDefaultWidth(),1);
        }

      tracklist_lastline=dl;
      tracklist_lastposy=pos_y;

      tracklist->acceptDrop(DRAG_ACCEPT);
      return 1;
      }
    }
  else if (getApp()->getDragWindow()==NULL) {
    FXDragType * types;
    FXuint ntypes;
    if (tracklist->inquireDNDTypes(FROM_DRAGNDROP,types,ntypes)){
      if (source->dnd_accepts(types,ntypes)){
        tracklist->acceptDrop(DRAG_ACCEPT);
        freeElms(types);
        return 1;
        }
      }
    freeElms(types);
    }
  tracklist->acceptDrop(DRAG_REJECT);
  return 1;
  }




long GMTrackView::onDndTrackDrop(FXObject*sender,FXSelector,void*ptr){
  if (getApp()->getDragWindow()==tracklist) {

    if (!tracklist->offeredDNDType(FROM_DRAGNDROP,GMClipboard::selectedtracks)) return 1;

    if (tracklist_lastline>=0) {
      tracklist->update(0,tracklist_lastline,tracklist->getHeader()->getDefaultWidth(),1);
      }

    tracklist_dragging=1;
    tracklist_lastline=-1;

    FXint i,tgt;
    FXbool moved=false;
    for (i=tracklist->getNumItems()-1;i>=0;i--){
      if (tracklist->isItemSelected(i) && (i+1)!=tracklist_dropitem) {
        if (i<tracklist_dropitem) tgt=tracklist_dropitem-1; else tgt=tracklist_dropitem;
        tracklist->moveItem(tgt,i);
        tracklist->deselectItem(tgt);
        if (i<tracklist_dropitem) tracklist_dropitem-=1;
        i+=1;
        moved=true;
        }
      }

    if (moved) {
      source->dragged(tracklist);
      }

    tracklist->dropFinished(DRAG_LINK);

    if (GMPlayerManager::instance()->playing() && GMPlayerManager::instance()->getSource())
      source->findCurrent(tracklist,GMPlayerManager::instance()->getSource());

    return 1;
    }
  else if (source && getApp()->getDragWindow()==NULL) {
    return source->handle(sender,FXSEL(SEL_DND_DROP,GMSource::ID_DROP),ptr);
    }
  return 0;
  }


long GMTrackView::onDndMotion(FXObject*,FXSelector,void*){
  if (getApp()->getDragWindow()==NULL && source) {
    FXDragType * types;
    FXuint ntypes;
    if (tracklist->inquireDNDTypes(FROM_DRAGNDROP,types,ntypes)){
      if (source->dnd_accepts(types,ntypes)){
        tracklist->acceptDrop(DRAG_ACCEPT);
        freeElms(types);
        return 1;
        }
      }
    freeElms(types);
    }
  return 0;
  }

long GMTrackView::onDndDrop(FXObject*sender,FXSelector,void*ptr){
  if (getApp()->getDragWindow()==NULL && source) {
    return source->handle(sender,FXSEL(SEL_DND_DROP,GMSource::ID_DROP),ptr);
    }
  return 0;
  }

long GMTrackView::onDndRequest(FXObject*sender,FXSelector sel,void*ptr){
  if (source) {
    switch(FXSELID(sel)){
      case ID_TRACK_LIST : return source->handle(sender,FXSEL(SEL_DND_REQUEST,GMSource::ID_COPY_TRACK),ptr); break;
      case ID_ALBUM_LIST : return source->handle(sender,FXSEL(SEL_DND_REQUEST,GMSource::ID_COPY_ALBUM),ptr); break;
      case ID_ARTIST_LIST: return source->handle(sender,FXSEL(SEL_DND_REQUEST,GMSource::ID_COPY_ARTIST),ptr); break;
      default            : break;
      }
    }
  return 0;
  }


long GMTrackView::onCmdToggleBrowser(FXObject*,FXSelector,void*){

  if (source)
    saveTrackSettings(source->settingKey());

  if (browsersplit->getExpanded()==SHOWBROWSER)
    browsersplit->setExpanded(HIDEBROWSER);
  else
    browsersplit->setExpanded(SHOWBROWSER);


  if (source)
    loadTrackSettings(source->settingKey());

  refresh();
  return 1;
  }

long GMTrackView::onUpdToggleBrowser(FXObject*sender,FXSelector,void*){
  if (source && source->canBrowse()) {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
    if (browsersplit->getExpanded()==SHOWBROWSER)
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
    }
  return 1;
  }



long GMTrackView::onCmdToggleTags(FXObject*,FXSelector,void*){
  if (taglistframe->shown()) {
    taglistframe->hide();
    taglist->killSelection(false);
    taglist->selectItem(0,true);
    taglistframe->recalc();
    browsersplit->setHSplit(5000);
    }
  else {
    taglistframe->show();
    taglistframe->recalc();
    tagsplit->setHSplit(5000);
    browsersplit->setHSplit(6666);
    }
  return 1;
  }

long GMTrackView::onUpdToggleTags(FXObject*sender,FXSelector,void*){
  if (source && source->canBrowse() && browsersplit->getExpanded()==SHOWBROWSER) {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
    if (taglistframe->shown())
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_CHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_UNCHECK),NULL);
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
    }
  return 1;
  }

long GMTrackView::onCmdToggleFilter(FXObject*,FXSelector sel,void*){
  if (filterframe->shown() && FXSELID(sel)==ID_CLOSE_FILTER) {
    filterframe->hide();
    if (!filterfield->getText().empty()){
      source->setFilter(FXString::null,filtermask);
      refresh();
      }
    recalc();
    }
  else {
    filterframe->show();
    filterfield->setFocus();
    if (!filterfield->getText().empty())
      filterfield->setSelection(0,filterfield->getText().length());
    recalc();
    }
  return 1;
  }

long GMTrackView::onUpdToggleFilter(FXObject*sender,FXSelector,void*){
  if (source && source->canFilter())
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),NULL);
  return 1;
  }


long GMTrackView::onCmdAlbumListView(FXObject*,FXSelector sel,void*){
  if (FXSELID(sel)==ID_ALBUMS_VIEW_LIST) {
    FXuint opts=albumlist->getListStyle();
    albumlist->setListStyle(opts&~ALBUMLIST_BROWSER);
    }
  else {
    FXuint opts=albumlist->getListStyle();
    albumlist->setListStyle(opts|ALBUMLIST_BROWSER);
    }
  return 1;
  }
long GMTrackView::onUpdAlbumListView(FXObject*sender,FXSelector sel,void*){
  if (FXSELID(sel)==ID_ALBUMS_VIEW_LIST) {
    FXuint opts=albumlist->getListStyle();
    if (opts&ALBUMLIST_BROWSER)
      sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
    }
  else {
    FXuint opts=albumlist->getListStyle();
    if (opts&ALBUMLIST_BROWSER)
      sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
    }
  return 1;
  }

long GMTrackView::onCmdConfigureColumns(FXObject*,FXSelector,void*){
  GMColumnDialog dialog(getShell(),columns);
  if (dialog.execute()) {
    dialog.saveIndex();

    tracklist->clearHeaders();
    for (FXint i=0;i<columns.no();i++){
      if (columns[i].show) {
        tracklist->appendHeader(fxtr(columns[i].name.text()),columns[i].size,&columns[i]);
        }
      }


    }
  return 1;
  }


GMTrackItem * GMTrackView::getTrackItem(FXint i) const {
  return (GMTrackItem*)tracklist->getItem(i);
  }

GMTrackItem * GMTrackView::getCurrentTrackItem() const {
  FXASSERT(tracklist->getCurrentItem()>=0); return (GMTrackItem*)tracklist->getItem(tracklist->getCurrentItem());
  }

FXbool GMTrackView::isTrackItemSelected(FXint i) const {
  return tracklist->isItemSelected(i);
  }

FXint GMTrackView::getNumTracks() const {
  return tracklist->getNumItems() ;
  }


FXint GMTrackView::getTag(FXint index) const { return (FXint)(FXival)taglist->getItemData(index); }

FXint GMTrackView::getArtist(FXint index) const { return (FXint)(FXival)artistlist->getItemData(index); }

FXint GMTrackView::getAlbum(FXint index) const {
  return albumlist->getItemId(index);

//  return (FXint)(FXival)albumlist->getItemData(index);
  }

FXint GMTrackView::getTrack(FXint index) const { return (FXint)(FXival)tracklist->getItemId(index); }
