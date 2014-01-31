/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2014 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTRACKVIEW_H
#define GMTRACKVIEW_H

class GMList;
class GMTrackList;
class GMSource;
class GMTrackItem;
class GMAlbumList;


class GMTrackView : public FXPacker {
FXDECLARE(GMTrackView)
protected:
  FX4Splitter       * browsersplit;
  FX4Splitter       * tagsplit;
  FXVerticalFrame   * taglistframe;
  FXHorizontalFrame * filterframe;
  GMTextField       * filterfield;
  GMTrackList       * tracklist;
  GMList            * taglist;
  GMList            * artistlist;
  GMAlbumList       * albumlist;
  GMHeaderButton    * taglistheader;
  GMHeaderButton    * artistlistheader;
  GMHeaderButton    * albumlistheader;
  FXMenuPtr           columnmenu;
  FXMenuPtr           sortmenu;
  FXMenuPtr           filtermenu;
protected:
  FXFontPtr           font_listhead;
protected:
  GMColumnList        columns;
  GMSource          * source;
  FXuint              sort_seed;
  FXuint              shuffle_seed;
  FXuint              filtermask;
  FXuint              view;
public:
  static FXbool reverse_artist;
  static FXbool reverse_album;
  static FXbool album_by_year;
protected:
  FXint tracklist_dragging;
  FXint tracklist_lastline;
  FXint tracklist_lastposy;
  FXint tracklist_dropitem;
  FXint tracklist_posx;
  FXint tracklist_posy;
protected:
  GMTrackView();
  void initSelection(GMList * list,const FXchar *,const FXString & section="window");
  void initSelection(GMAlbumList * list,const FXchar *,const FXString & section="window");
  void saveSelection(GMList * list,const FXchar *,const FXString & section="window") const;
  void saveSelection(GMAlbumList * list,const FXchar *,const FXString & section="window") const;
  void configureView(FXuint);
protected:
  void init_track_context_menu(FXMenuPane *pane,FXbool selected);
private:
  GMTrackView(const GMTrackView&);
  GMTrackView& operator=(const GMTrackView&);
public:
  enum {
    ID_ARTIST_LIST_HEADER = FX4Splitter::ID_LAST,
    ID_ALBUM_LIST_HEADER,
    ID_TAG_LIST_HEADER,
    ID_ARTIST_LIST,
    ID_ALBUM_LIST,
    ID_TAG_LIST,
    ID_TRACK_LIST,
    ID_TRACK_LIST_HEADER,
    ID_FILTER,
    ID_FILTER_MODE,
    ID_TOGGLE_BROWSER,
    ID_TOGGLE_TAGS,
    ID_TOGGLE_FILTER,
    ID_CLOSE_FILTER,
    ID_COPY,
    ID_CUT,
    ID_PASTE,
    ID_SHOW_CURRENT,
    ID_COLUMN_FIRST,
    ID_COLUMN_LAST=ID_COLUMN_FIRST+20,
    ID_SORT_FIRST,
    ID_SORT_LAST=ID_SORT_FIRST+20,
    ID_SORT_SHUFFLE,
    ID_SORT_BROWSE,
    ID_SORT_REVERSE,
    ID_SORT_DEFAULT,
    ID_CONFIGURE_COLUMNS,
    ID_FILTER_TRACK,
    ID_FILTER_ALBUM,
    ID_FILTER_ARTIST,
    ID_FILTER_TAG,
    ID_FILTER_LAST = ID_FILTER_TAG,
    ID_ALBUMS_VIEW_LIST,
    ID_ALBUMS_VIEW_BROWSER,
    ID_LAST,
    };
public:
  long onCmdSortTagList(FXObject*,FXSelector,void*);
  long onCmdSortArtistList(FXObject*,FXSelector,void*);
  long onCmdSortAlbumList(FXObject*,FXSelector,void*);

  long onCmdTagSelected(FXObject*,FXSelector,void*);
  long onCmdArtistSelected(FXObject*,FXSelector,void*);
  long onCmdAlbumSelected(FXObject*,FXSelector,void*);

  long onTagContextMenu(FXObject*,FXSelector,void*);
  long onArtistContextMenu(FXObject*,FXSelector,void*);
  long onAlbumContextMenu(FXObject*,FXSelector,void*);
  long onAlbumHeaderContextMenu(FXObject*,FXSelector,void*);
  long onTrackContextMenu(FXObject*,FXSelector,void*);
  long onTrackHeaderContextMenu(FXObject*,FXSelector,void*);

  long onCmdTagKeyPress(FXObject*,FXSelector,void*);
  long onCmdArtistKeyPress(FXObject*,FXSelector,void*);
  long onCmdAlbumKeyPress(FXObject*,FXSelector,void*);
  long onCmdTrackKeyPress(FXObject*,FXSelector,void*);

  long onCmdFilter(FXObject*,FXSelector,void*);

  long onCmdFilterMask(FXObject*,FXSelector,void*);
  long onUpdFilterMask(FXObject*,FXSelector,void*);


  long onCmdBeginDrag(FXObject*,FXSelector,void*);
  long onCmdDragged(FXObject*,FXSelector,void*);
  long onCmdEndDrag(FXObject*,FXSelector,void*);

  long onDndTrackEnter(FXObject*,FXSelector,void*);
  long onDndTrackLeave(FXObject*,FXSelector,void*);
  long onDndTrackMotion(FXObject*,FXSelector,void*);
  long onDndTrackDrop(FXObject*,FXSelector,void*);

  long onDndMotion(FXObject*,FXSelector,void*);
  long onDndDrop(FXObject*,FXSelector,void*);
  long onDndRequest(FXObject*,FXSelector,void*);

  long onCmdPlayTrack(FXObject*,FXSelector,void*);

  long onCmdPaste(FXObject*,FXSelector,void*);
  long onUpdPaste(FXObject*,FXSelector,void*);

  long onCmdCopy(FXObject*,FXSelector,void*);
  long onUpdCopy(FXObject*,FXSelector,void*);

  long onCmdToggleBrowser(FXObject*,FXSelector,void*);
  long onUpdToggleBrowser(FXObject*,FXSelector,void*);

  long onCmdToggleTags(FXObject*,FXSelector,void*);
  long onUpdToggleTags(FXObject*,FXSelector,void*);

  long onCmdToggleFilter(FXObject*,FXSelector,void*);
  long onUpdToggleFilter(FXObject*,FXSelector,void*);

  long onCmdShowCurrent(FXObject*,FXSelector,void*);
  long onUpdShowCurrent(FXObject*,FXSelector,void*);

  long onCmdShowColumn(FXObject*,FXSelector,void*);
  long onUpdShowColumn(FXObject*,FXSelector,void*);

  long onCmdSortDefault(FXObject*,FXSelector,void*);
  long onCmdSort(FXObject*,FXSelector,void*);
  long onUpdSort(FXObject*,FXSelector,void*);
  long onUpdSortReverse(FXObject*,FXSelector,void*);
  long onCmdSortBrowse(FXObject*,FXSelector,void*);
  long onUpdSortBrowse(FXObject*,FXSelector,void*);
  long onCmdSortShuffle(FXObject*,FXSelector,void*);
  long onUpdSortShuffle(FXObject*,FXSelector,void*);

  long onCmdAlbumListView(FXObject*,FXSelector,void*);
  long onUpdAlbumListView(FXObject*,FXSelector,void*);

  long onCmdConfigureColumns(FXObject*,FXSelector,void*);
public:
  GMTrackView(FXComposite* p);

  void updateFont();

  void updateColors();

  void updateIcons();

  FXMenuPane * getColumnMenu() const { return columnmenu; }

  FXMenuPane * getSortMenu() const { return sortmenu; }

  FXint numTrackSelected() const;

  FXbool trackSelected() const;

  FXbool hasTracks() const;

  FXbool hasBrowser() const;

  void getSelectedTags(FXIntList & tags) const;

  void getSelectedArtists(FXIntList & artists) const;

  void getSelectedAlbums(FXIntList & albums) const;

  void getSelectedTracks(FXIntList & tracks) const;

  void getTracks(FXIntList & tracks) const;

  GMTrackItem * getTrackItem(FXint i) const;

  GMTrackItem * getCurrentTrackItem() const;

  FXbool isTrackItemSelected(FXint i) const;

  FXint getNumTracks() const;

  void selectTagItem(FXint item);

  void selectAlbumItem(FXint item);

  void selectArtistItem(FXint item);

  void selectTrackItem(FXint item);

  FXint getTag(FXint index) const;

  FXint getArtist(FXint index) const;

  FXint getAlbum(FXint index) const;

  FXint getTrack(FXint index) const;

  FXbool listTags();

  FXbool listArtists();

  FXbool listAlbums();

  FXbool listTracks();

  FXint getActive() const;

  FXint getCurrent() const;

  FXint getNextPlayable(FXint,FXbool) const;

  FXint getNext(FXbool wrap=false);

  FXint getPreviousPlayable(FXint,FXbool) const;

  FXint getPrevious();

  void setSource(GMSource * src);

  GMSource * getSource() const { return source; }

  //FXString getTrackFilename(FXint i) const;

  void sortTags() const;

  void sortArtists() const;

  void sortAlbums() const;

  void sortTracks() const;

  void resort();

  void init(GMSource * src);

  void refreshUpdate();

  void refresh();

  void clear();

  void redrawAlbumList();

  void setActive(FXint item,FXbool show=true);

  void showCurrent();

  void setSortMethod(FXint hdr,FXbool reverse=false);

  FXbool getSortReverse() const;

  void loadSettings(const FXString & key);

  void saveSettings(const FXString & key) const;

  void loadTrackSettings(const FXString & key);

  void saveTrackSettings(const FXString & key) const;

  void saveView() const;

  FXbool focusNext();

  FXbool focusPrevious();

  void selectNext();

  void selectPrevious();

  virtual ~GMTrackView();
  };



#endif
