/********************************************************************************
*                                                                               *
*                         I c o n   L i s t   W i d g e t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2009 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: GMAlbumList.h,v 1.120 2009/01/06 13:07:25 fox Exp $                       *
********************************************************************************/
#ifndef FXALBUMBROWSER_H
#define FXALBUMBROWSER_H


/// Icon list styles
enum {
  ALBUMLIST_EXTENDEDSELECT = 0,                /// Extended selection mode
  ALBUMLIST_SINGLESELECT   = 0x00100000,       /// At most one selected item
  ALBUMLIST_BROWSESELECT   = 0x00200000,       /// Always exactly one selected item
  ALBUMLIST_MULTIPLESELECT = 0x00300000,       /// Multiple selection mode
  ALBUMLIST_ROWS           = 0,                /// Row-wise mode
  ALBUMLIST_COLUMNS        = 0x02000000,       /// Column-wise mode
  ALBUMLIST_BROWSER        = 0x04000000,
  ALBUMLIST_YEAR           = 0x08000000,       /// Display Year
  ALBUMLIST_NORMAL         = ALBUMLIST_EXTENDEDSELECT
  };


class GMAlbumList;
class GMCoverThumbs;


/// Icon item
class FXAPI GMAlbumListItem : public FXObject {
  FXDECLARE(GMAlbumListItem)
  friend class GMAlbumList;
protected:
  FXString  artist;
  FXString  title;
  FXint     year;
  FXint     id;
  FXIntList ids;
  FXuint    state;      // State flags
private:
  GMAlbumListItem(const GMAlbumListItem&);
  GMAlbumListItem& operator=(const GMAlbumListItem&);
protected:
  GMAlbumListItem(): state(0){}
  virtual void draw(const GMAlbumList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual void drawList(const GMAlbumList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual FXint hitItem(const GMAlbumList* list,FXint rx,FXint ry,FXint rw=1,FXint rh=1) const;
protected:
public:
  enum {
    SELECTED      = 1,  /// Selected
    FOCUS         = 2,  /// Focus
    DRAGGABLE     = 4,  /// Draggable
    };
public:
  /// Construct new item with given text, icons, and user-data
  GMAlbumListItem(const FXString& a,const FXString & t,FXint y,FXint i):artist(a),title(t),year(y),id(i),state(0){}

  /// Return item's id
  FXint getId() const { return id; }

  /// Make item draw as focused
  virtual void setFocus(FXbool focus);

  /// Return true if item has focus
  FXbool hasFocus() const { return (state&FOCUS)!=0; }

  /// Select item
  virtual void setSelected(FXbool selected);

  /// Return true if this item is selected
  FXbool isSelected() const { return (state&SELECTED)!=0; }

  /// Make item draggable
  virtual void setDraggable(FXbool draggable);

  /// Return true if this item is draggable
  FXbool isDraggable() const { return (state&DRAGGABLE)!=0; }

  /// Return width of this item
  FXint getWidth(const GMAlbumList* list);

  /// Return tip text
  FXString getTipText() const;

  /// Return Title
  FXString getTitle() const { return title; }

  /// Return Year
  FXint getYear() const { return year; }

  /// Append id
  void append(FXint id) { ids.append(id); }

  /// Append ids to list
  void getIds(FXIntList & list) const;

  /// Destroy item and free icons if owned
  virtual ~GMAlbumListItem();
  };


/// Icon item collate function
typedef FXint (*GMAlbumListSortFunc)(const GMAlbumListItem*,const GMAlbumListItem*);


extern FXint album_list_sort(const GMAlbumListItem* pa,const GMAlbumListItem* pb);
extern FXint album_list_sort_reverse(const GMAlbumListItem* pa,const GMAlbumListItem* pb);

/// List of GMAlbumListItem's
typedef FXObjectListOf<GMAlbumListItem> GMAlbumListItemList;


class FXAPI GMAlbumList : public FXScrollArea {
  FXDECLARE(GMAlbumList)
protected:
  GMCoverThumbs     * thumbs;
  GMAlbumListItemList items;		// Item list
  FXint              nrows;             // Number of rows
  FXint              ncols;             // Number of columns
  FXint              anchor;            // Anchor item
  FXint              current;           // Current item
  FXint              extent;            // Extent item
  FXint              cursor;            // Cursor item
  FXint              viewable;          // Visible item

  FXIcon            *listicon;          // list icon
  FXFont            *listbasefont;      // list base font
  FXFont            *listheadfont;      // list head font
  FXFont            *listtailfont;      // list tail font
  FXFont            *coverheadfont;     // cover head font
  FXFont            *coverbasefont;     // cover base font
  GMAlbumListSortFunc sortfunc;          // Item sort function
  FXColor            textColor;         // Text color
  FXColor            selbackColor;      // Selected back color
  FXColor            seltextColor;      // Selected text color
  FXColor            altbackColor;      // Alternative Back Color
  FXint              itemWidth;         // Item width
  FXint              itemHeight;        // Item height
  FXint              anchorx;           // Rectangular selection
  FXint              anchory;
  FXint              currentx;
  FXint              currenty;
  FXint              grabx;             // Grab point x
  FXint              graby;             // Grab point y
  FXString           lookup;            // Lookup string
  FXString           help;              // Help text
  FXbool             state;             // State of item
protected:
  GMAlbumList();
  void recompute();
  void startLasso(FXint ax,FXint ay);
  void updateLasso(FXint cx,FXint cy);
  void endLasso();
  void getrowscols(FXint& nr,FXint& nc,FXint w,FXint h) const;
  void lassoChanged(FXint ox,FXint oy,FXint ow,FXint oh,FXint nx,FXint ny,FXint nw,FXint nh,FXbool notify);
private:
  GMAlbumList(const GMAlbumList&);
  GMAlbumList &operator=(const GMAlbumList&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onEnter(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onRightBtnPress(FXObject*,FXSelector,void*);
  long onRightBtnRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onTipTimer(FXObject*,FXSelector,void*);
  long onCmdSelectAll(FXObject*,FXSelector,void*);
  long onCmdDeselectAll(FXObject*,FXSelector,void*);
  long onCmdSelectInverse(FXObject*,FXSelector,void*);
  long onCmdArrangeByRows(FXObject*,FXSelector,void*);
  long onUpdArrangeByRows(FXObject*,FXSelector,void*);
  long onCmdArrangeByColumns(FXObject*,FXSelector,void*);
  long onUpdArrangeByColumns(FXObject*,FXSelector,void*);
  long onFocusIn(FXObject*,FXSelector,void*);
  long onFocusOut(FXObject*,FXSelector,void*);
  long onClicked(FXObject*,FXSelector,void*);
  long onDoubleClicked(FXObject*,FXSelector,void*);
  long onTripleClicked(FXObject*,FXSelector,void*);
  long onCommand(FXObject*,FXSelector,void*);
  long onAutoScroll(FXObject*,FXSelector,void*);
  long onLookupTimer(FXObject*,FXSelector,void*);
  long onCmdSetValue(FXObject*,FXSelector,void*);
  long onCmdGetIntValue(FXObject*,FXSelector,void*);
  long onCmdSetIntValue(FXObject*,FXSelector,void*);
  long onCmdShowYear(FXObject*,FXSelector,void*);
  long onUpdShowYear(FXObject*,FXSelector,void*);
public:
  enum {
    ID_LOOKUPTIMER=FXScrollArea::ID_LAST,
    ID_HEADER,
    ID_ARRANGE_BY_ROWS,
    ID_ARRANGE_BY_COLUMNS,
    ID_SELECT_ALL,
    ID_DESELECT_ALL,
    ID_SELECT_INVERSE,
    ID_YEAR,
    ID_LAST
    };
public:

  /// Construct icon list with no items in it initially
  GMAlbumList(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=ICONLIST_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Get Thumbs
  GMCoverThumbs * getCoverThumbs() const { return thumbs; }

  /// Set thumbs
  void setCoverThumbs(GMCoverThumbs* t) { thumbs=t; }

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Recalculate layout
  virtual void recalc();

  /// Perform layout
  virtual void layout();

#if FOXVERSION < FXVERSION(1,7,0)
  virtual FXint getViewportHeight();
#else
  /// Return visible area y position
  virtual FXint getVisibleY() const;

  /// Return visible area height
  virtual FXint getVisibleHeight() const;
#endif

  /// Compute and return content width
  virtual FXint getContentWidth();

  /// Return content height
  virtual FXint getContentHeight();

  /// Icon list can receive focus
#if FOXVERSION < FXVERSION(1,7,0)
  virtual bool canFocus() const;
#else
  virtual FXbool canFocus() const;
#endif

  /// Move the focus to this window
  virtual void setFocus();

  /// Remove the focus from this window
  virtual void killFocus();

  /// Resize this window to the specified width and height
  virtual void resize(FXint w,FXint h);

  /// Move and resize this window in the parent's coordinates
  virtual void position(FXint x,FXint y,FXint w,FXint h);

  /// Return number of items
  FXint getNumItems() const { return items.no(); }

  /// Return number of rows
  FXint getNumRows() const { return nrows; }

  /// Return number of columns
  FXint getNumCols() const { return ncols; }

  /// Return the item at the given index
  GMAlbumListItem *getItem(FXint index) const;

  /// Replace the item with a [possibly subclassed] item
  FXint setItem(FXint index,GMAlbumListItem* item,FXbool notify=false);

  /// Insert a new [possibly subclassed] item at the give index
  FXint insertItem(FXint index,GMAlbumListItem* item,FXbool notify=false);

  /// Append a [possibly subclassed] item to the end of the list
  FXint appendItem(GMAlbumListItem* item,FXbool notify=false);

  /// Prepend a [possibly subclassed] item to the end of the list
  FXint prependItem(GMAlbumListItem* item,FXbool notify=false);

  /// Move item from oldindex to newindex
  FXint moveItem(FXint newindex,FXint oldindex,FXbool notify=false);

  /// Extract item from list
  GMAlbumListItem* extractItem(FXint index,FXbool notify=false);

  /// Remove item from list
  void removeItem(FXint index,FXbool notify=false);

  /// Remove all items from list
  void clearItems(FXbool notify=false);

  /// Return item width
  FXint getItemWidth() const { return itemWidth; }

  /// Return item height
  FXint getItemHeight() const { return itemHeight; }

  /// Return index of item at x,y, or -1 if none
  virtual FXint getItemAt(FXint x,FXint y) const;

  /// Scroll to make item at index visible
  virtual void makeItemVisible(FXint index);

  /// Return item id
  FXint getItemId(FXint index) const;

  /// Return true if item at index is selected
  FXbool isItemSelected(FXint index) const;

  /// Return true if item at index is current
  FXbool isItemCurrent(FXint index) const;

  /// Return true if item at index is visible
  FXbool isItemVisible(FXint index) const;

  /// Return item hit code: 0 outside, 1 icon, 2 text
  FXint hitItem(FXint index,FXint x,FXint y,FXint ww=1,FXint hh=1) const;

  /// Repaint item at index
  void updateItem(FXint index) const;

  /// Select item at index
  virtual FXbool selectItem(FXint index,FXbool notify=false);

  /// Deselect item at index
  virtual FXbool deselectItem(FXint index,FXbool notify=false);

  /// Toggle item at index
  virtual FXbool toggleItem(FXint index,FXbool notify=false);

  /// Select items in rectangle
  virtual FXbool selectInRectangle(FXint x,FXint y,FXint w,FXint h,FXbool notify=false);

  /// Extend selection from anchor index to index
  virtual FXbool extendSelection(FXint index,FXbool notify=false);

  /// Deselect all items
  virtual FXbool killSelection(FXbool notify=false);

  /// Change current item index
  virtual void setCurrentItem(FXint index,FXbool notify=false);

  /// Return current item index, or -1 if none
  FXint getCurrentItem() const { return current; }

  /// Change anchor item index
  void setAnchorItem(FXint index);

  /// Return anchor item index, or -1 if none
  FXint getAnchorItem() const { return anchor; }

  /// Return index of item under cursor, or -1 if none
  FXint getCursorItem() const { return cursor; }

  /// Find Item By Id
  FXint findItemById(const FXint id,FXint start=-1,FXuint flags=SEARCH_FORWARD|SEARCH_WRAP) const;

  /// Sort items
  void sortItems();

  /// Return sort function
  GMAlbumListSortFunc getSortFunc() const { return sortfunc; }

  /// Change sort function
  void setSortFunc(GMAlbumListSortFunc func){ sortfunc=func; }

  void setListIcon(FXIcon*);

  FXIcon * getListIcon() const { return listicon; }

  /// Set List Base Font
  void setListBaseFont(FXFont*);

  /// Return base font
  FXFont* getListBaseFont() const { return listbasefont; }

  /// Set List Head Font
  void setListHeadFont(FXFont*);

  /// Return head font
  FXFont* getListHeadFont() const { return listheadfont; }

  /// Set List Tail Font
  void setListTailFont(FXFont*);

  /// Return suffix font
  FXFont* getListTailFont() const { return listtailfont; }

  /// Set Cover Head Font
  void setCoverHeadFont(FXFont*);

  /// Return head font
  FXFont* getCoverHeadFont() const { return coverheadfont; }

  /// Set Cover Base Font
  void setCoverBaseFont(FXFont*);

  /// Return base font
  FXFont* getCoverBaseFont() const { return coverbasefont; }

  /// Return normal text color
  FXColor getTextColor() const { return textColor; }

  /// Change normal text color
  void setTextColor(FXColor clr);

  /// Return selected text background
  FXColor getAltBackColor() const { return altbackColor; }

  /// Change selected text background
  void setAltBackColor(FXColor clr);

  /// Return selected text background
  FXColor getSelBackColor() const { return selbackColor; }

  /// Change selected text background
  void setSelBackColor(FXColor clr);

  /// Return selected text color
  FXColor getSelTextColor() const { return seltextColor; }

  /// Change selected text color
  void setSelTextColor(FXColor clr);

  /// Get the current icon list style
  FXuint getListStyle() const;

  /// Set the current icon list style.
  void setListStyle(FXuint style);

  /// Set the status line help text for this widget
  void setHelpText(const FXString& text);

  /// Get the status line help text for this widget
  const FXString& getHelpText() const { return help; }

  /// Save list to a stream
  virtual void save(FXStream& store) const;

  /// Load list from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~GMAlbumList();
  };

#endif
