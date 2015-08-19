/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*                        Icon List Widget (under LGPL3)                        *
*      Copyright (C) 1999,2009 by Jeroen van der Zijp. All Rights Reserved.    *
*                               ---                                            *
*                           Modifications                                      *
*           Copyright (C) 2006-2015 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMTRACKLIST_H
#define GMTRACKLIST_H

enum {
  COLUMN_JUSTIFY_NORMAL=0,
  COLUMN_JUSTIFY_CENTER_RIGHT_ALIGNED,
  COLUMN_JUSTIFY_LEFT_RIGHT_ALIGNED,
  COLUMN_JUSTIFY_RIGHT,
  };


/// Icon list styles
enum {
  TRACKLIST_EXTENDEDSELECT = 0,                /// Extended selection mode
  TRACKLIST_SINGLESELECT   = 0x00100000,       /// At most one selected item
  TRACKLIST_BROWSESELECT   = 0x00200000,       /// Always exactly one selected item
  TRACKLIST_MULTIPLESELECT = 0x00300000,       /// Multiple selection mode
  TRACKLIST_NORMAL         = ICONLIST_EXTENDEDSELECT
  };

class GMTrackList;
class GMColumn;


class GMTrackItem {
  friend class GMTrackList;
protected:
  FXint   id = 0;
  FXuchar state = 0;
protected:
  virtual const FXString * getColumnData(FXint,FXString &,FXuint &,FXint &) const { return NULL; }
  virtual FXIcon * getIcon() const { return NULL; }
public:
  enum {
    SELECTED      = 0x01,  /// Selected
    FOCUS         = 0x02,  /// Focus
    DRAGGABLE     = 0x04,  /// Draggable
    DONOTPLAY     = 0x08,  /// Playable
    SHADED        = 0x10   /// Shaded
    };
public:
  GMTrackItem() {}
  GMTrackItem(FXint tid) : id(tid),state(0) {}

  /// Return track item id
  FXint getId() const { return id; }

  /// Select item
  virtual void setSelected(FXbool selected);

  /// Return true if this item is selected
  FXbool isSelected() const { return (state&SELECTED)!=0; }

  /// Make item draw as focused
  virtual void setFocus(FXbool focus);

  /// Return true if item has focus
  FXbool hasFocus() const { return (state&FOCUS)!=0; }

  /// Make item draggable
  virtual void setDraggable(FXbool draggable);

  /// Return true if this item is draggable
  FXbool isDraggable() const { return (state&DRAGGABLE)!=0; }

  /// Return true if this item is playable
  FXbool canPlay() const { return (state&DONOTPLAY)==0; }

  /// Return true if this item is shaded
  FXbool isShaded() const { return (state&SHADED)!=0; }

  /// Destructor
  virtual ~GMTrackItem() {}
  };

class GMDBTrackItem;


/// Icon item collate function
typedef FXint (*GMTrackListSortFunc)(const GMTrackItem*,const GMTrackItem*);

class GMColumn {
  public:
  FXString            name;
  FXint               type  = 0;
  FXint               size  = 60;
  FXint               index = 0;
  GMTrackListSortFunc ascending = nullptr;
  GMTrackListSortFunc descending = nullptr;
  FXbool              show = false;
  FXbool              default_show = false;
  FXbool              default_browser_show = false;
  FXObject*           target = nullptr;
  FXSelector          message = 0;
  GMColumn() {}
  GMColumn(const FXchar * n,FXuint t,GMTrackListSortFunc a,GMTrackListSortFunc b,FXint sz=60,FXbool def_show=true,FXbool def_browser_show=true,FXint idx=0,FXObject* tgt=NULL,FXSelector sel=0) : name(n),type(t),size(sz),index(idx),ascending(a),descending(b),show(true),default_show(def_show),default_browser_show(def_browser_show),target(tgt),message(sel) {}
  };

typedef FXArray<GMColumn> GMColumnList;

/// List of FXIconItem's
typedef FXArray<GMTrackItem*> GMTrackItemList;


class GMTrackList : public FXScrollArea {
FXDECLARE(GMTrackList)
friend class GMTrackItem;
protected:
  FXHeader*          header;            // Header control
  GMTrackItemList    items;             // Item List
  FXint              anchor;            // Anchor item
  FXint              current;           // Current item
  FXint              extent;            // Extent item
  FXint              cursor;            // Cursor item
  FXint              viewable;          // Visible item
  FXint              active;
  FXFont            *font;              // Font
  FXFont            *activeFont;
  GMTrackListSortFunc sortfunc;         // Item sort function
  FXColor            textColor;         // Text color
  FXColor            selbackColor;      // Selected back color
  FXColor            seltextColor;      // Selected text color
  FXColor            rowColor;
  FXColor            activeColor;
  FXColor            activeTextColor;
  FXColor            shadowColor;
  FXint              lineHeight;        // Item height
  FXint              anchorx;           // Rectangular selection
  FXint              anchory;
  FXint              currentx;
  FXint              currenty;
  FXint              ratingx;
  FXint              ratingy;
  FXint              ratingl;
  FXint              grabx;             // Grab point x
  FXint              graby;             // Grab point y
  FXString           help;              // Help text
  FXbool             state;             // State of item
  FXint              sortMethod;
  FXString           starset;
  FXString           starunset;
protected:
  GMTrackList();
  void draw(FXDC& dc,FXEvent *event,FXint index,FXint x,FXint y,FXint w,FXint h,FXint dw) const;
  void recompute();
  virtual void moveContents(FXint x,FXint y);
  void clearRating();
private:
  GMTrackList(const GMTrackList&);
  GMTrackList &operator=(const GMTrackList&);
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
  long onCmdShowDetails(FXObject*,FXSelector,void*);
  long onUpdShowDetails(FXObject*,FXSelector,void*);
  long onCmdShowBigIcons(FXObject*,FXSelector,void*);
  long onUpdShowBigIcons(FXObject*,FXSelector,void*);
  long onCmdShowMiniIcons(FXObject*,FXSelector,void*);
  long onUpdShowMiniIcons(FXObject*,FXSelector,void*);
  long onChgHeader(FXObject*,FXSelector,void*);
  long onClkHeader(FXObject*,FXSelector,void*);
  long onCmdHeader(FXObject*,FXSelector,void*);
  long onUpdHeader(FXObject*,FXSelector,void*);
  long onHeaderRightBtnRelease(FXObject*,FXSelector,void*);
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
  long onMouseLeave(FXObject*,FXSelector,void*);
  long onWheelTimeout(FXObject*,FXSelector,void*);
public:
  enum {
    ID_HEADER=FXScrollArea::ID_LAST,
    ID_SELECT_ALL,
    ID_DESELECT_ALL,
    ID_SELECT_INVERSE,
    ID_WHEEL_TIMEOUT,
    ID_LAST
    };
public:
  /// Construct icon list with no items in it initially
  GMTrackList(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=TRACKLIST_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Find Item by Id
  FXint findItemById(FXint id) const;

  /// Get the unique item id
  FXint getItemId(FXint index) const { return items[index]->id; }

  /// Set the sort method
  void setSortMethod(FXint m) { sortMethod=m; }

  /// Get the sort method
  FXint getSortMethod() const { return sortMethod; }

  /// Mark the list as unsorted
  void markUnsorted();

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Recalculate layout
  virtual void recalc();

  /// Perform layout
  virtual void layout();

  /// Return visible area y position
  virtual FXint getVisibleY() const;

  /// Return visible area height
  virtual FXint getVisibleHeight() const;

  /// Compute and return content width
  virtual FXint getContentWidth();

  /// Return content height
  virtual FXint getContentHeight();

  /// Icon list can receive focus
  virtual FXbool canFocus() const;

  /// Move the focus to this window
  virtual void setFocus();

  /// Remove the focus from this window
  virtual void killFocus();

  /// Return number of items
  FXint getNumItems() const { return items.no(); }

  /// Return header control
  FXHeader* getHeader() const { return header; }

  /// Return the header data.
  GMColumn * getHeaderData(FXint i) const { return static_cast<GMColumn*>(header->getItemData(i)); }

  /// Return the header type
  FXuint getHeaderType(FXint i) const { return ( (i>=0 && i<header->getNumItems()) ? ((static_cast<GMColumn*>(header->getItemData(i)))->type) : -1); }

  /// Append header with given text, size and column data
  void appendHeader(const FXString & label,FXint size,GMColumn * data);

  /// Remove header at index
  void removeHeader(FXint index);

  /// Return number of headers
  FXint getNumHeaders() const;

  /// Return index of given header type if displayed, otherwise -1
  FXint getHeaderByType(FXuint type) const;

  /// Remove All Headers
  void clearHeaders();

  /// Save Header Configuration
  void saveHeaders();

  /// Return the item at the given index
  GMTrackItem *getItem(FXint index) const;

  /// Replace the item with a [possibly subclassed] item
  FXint setItem(FXint index,GMTrackItem* item,FXbool notify=false);

  /// Insert a new [possibly subclassed] item at the give index
  FXint insertItem(FXint index,GMTrackItem* item,FXbool notify=false);

  /// Append a [possibly subclassed] item to the end of the list
  FXint appendItem(GMTrackItem* item,FXbool notify=false);

  /// Prepend a [possibly subclassed] item to the end of the list
  FXint prependItem(GMTrackItem* item,FXbool notify=false);

  /// Move item from oldindex to newindex
  FXint moveItem(FXint newindex,FXint oldindex,FXbool notify=false);

  /// Extract item from list
  GMTrackItem* extractItem(FXint index,FXbool notify=false);

  /// Remove item from list
  void removeItem(FXint index,FXbool notify=false);

  /// Remove all items from list
  void clearItems(FXbool notify=false);

  /// Return item height
  FXint getLineHeight() const { return lineHeight; }

  /// Return index of item at x,y, or -1 if none
  virtual FXint getItemAt(FXint x,FXint y) const;

  /// Scroll to make item at index visible
  virtual void makeItemVisible(FXint index);

  /// Return true if item at index is selected
  FXbool isItemSelected(FXint index) const;

  /// Return true if item at index is current
  FXbool isItemCurrent(FXint index) const;

  /// Return true if item at index is visible
  FXbool isItemVisible(FXint index) const;

  /// Return true if item at index is enabled
  FXbool isItemEnabled(FXint index) const;

  /// Check if item is Playable
  FXbool isItemPlayable(FXint index) const;

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

  /// Change active item index
  void setActiveItem(FXint index);

  /// Return active item index, or -1 if none
  FXint getActiveItem() const { return active; }

  /// Sort items
  void sortItems();

  /// Return sort function
  GMTrackListSortFunc getSortFunc() const { return sortfunc; }

  /// Change sort function
  void setSortFunc(GMTrackListSortFunc func){ sortfunc=func; }

  /// Change text font
  void setFont(FXFont* fnt);

  /// Return text font
  FXFont* getFont() const { return font; }

  /// Change active text font
  void setActiveFont(FXFont* fnt);

  /// Return active text font
  FXFont* getActiveFont() const { return activeFont; }

  /// Return normal text color
  FXColor getTextColor() const { return textColor; }

  /// Change normal text color
  void setTextColor(FXColor clr);

  /// Return selected text background
  FXColor getSelBackColor() const { return selbackColor; }

  /// Change selected text background
  void setSelBackColor(FXColor clr);

  /// Return selected text color
  FXColor getSelTextColor() const { return seltextColor; }

  /// Change selected text color
  void setSelTextColor(FXColor clr);

  /// Return active text color
  FXColor getActiveTextColor() const { return activeTextColor; }

  /// Change active text color
  void setActiveTextColor(FXColor clr);

  /// Return row color
  FXColor getRowColor() const { return rowColor; }

  /// Change the row color
  void setRowColor(FXColor clr);

  /// Return active color
  FXColor getActiveColor() const { return activeColor; }

  /// Change the active color
  void setActiveColor(FXColor clr);

  /// Change shadow color
  void setShadowColor(FXColor clr);

  /// Get shadow color
  FXColor getShadowColor() const { return shadowColor; }

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
  virtual ~GMTrackList();
  };
#endif
