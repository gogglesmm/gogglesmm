/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2016 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMLIST_H
#define GMLIST_H

extern FXint genre_list_sort(const FXListItem* pa,const FXListItem* pb);
extern FXint genre_list_sort_reverse(const FXListItem* pa,const FXListItem* pb);

extern FXint generic_name_sort(const FXListItem* pa,const FXListItem* pb);
extern FXint generic_name_sort_reverse(const FXListItem* pa,const FXListItem* pb);

extern FXint source_list_sort(const FXTreeItem* pa,const FXTreeItem* pb);
extern FXint source_list_sort_reverse(const FXTreeItem* pa,const FXTreeItem* pb);

class GMList;

class GMListItem : public FXListItem {
  FXDECLARE(GMListItem)
  friend class GMList;
private:
  GMListItem(const GMListItem&);
  GMListItem& operator=(const GMListItem&);
protected:
  GMListItem() {}

  // unhide to keep compiler happy over hiding virtual
  using FXListItem::draw;

  virtual void draw(const GMList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
public:
  /// Construct new item with given text, icon, and user-data
  GMListItem(const FXString& text,FXIcon* ic=NULL,void* ptr=NULL): FXListItem(text,ic,ptr) { }
  };



class GMList : public FXList {
  FXDECLARE(GMList)
protected:
  FXFont * thickfont = nullptr;
  FXColor rowcolor = 0;
protected:
  GMList(){}
  void recompute();
  virtual FXListItem *createItem(const FXString& text,FXIcon* icon,void* ptr);
private:
  GMList(const GMList&);
  GMList &operator=(const GMList&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  /// Construct a list with initially no items in it
  GMList(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=LIST_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  FXColor getRowColor() const { return rowcolor; }

  void setRowColor(FXColor c) { rowcolor=c; update(); }

  void setThickFont(FXFont * f) { thickfont=f;}

  virtual ~GMList();
  };

/// Tree list Item
class GMTreeItem : public FXTreeItem {
  FXDECLARE(GMTreeItem)
  friend class GMTreeList;
protected:
  GMTreeItem();
private:
  GMTreeItem(const GMTreeItem&);
  GMTreeItem& operator=(const GMTreeItem&);
protected:
  virtual void draw(const FXTreeList* list,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
public:
  /// Constructor
  GMTreeItem(const FXString& text,FXIcon* oi=NULL,FXIcon* ci=NULL,void* ptr=NULL): FXTreeItem(text,oi,ci,ptr){}

  /// Return height of item as drawn in list
  virtual FXint getHeight(const FXTreeList* list) const;
  };

class GMTreeList : public FXTreeList {
  FXDECLARE(GMTreeList)
protected:
  FXColor rowcolor = 0;
protected:
  GMTreeList() {}
  virtual FXTreeItem* createItem(const FXString& text,FXIcon* oi,FXIcon* ci,void* ptr);
private:
  GMTreeList(const GMTreeList&);
  GMTreeList& operator=(const GMTreeList&);
public:
  long onPaint(FXObject*,FXSelector,void*);
public:
  /// Construct a new, initially empty tree list
  GMTreeList(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=TREELIST_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  void setRowColor(FXColor c) { rowcolor=c; update(); }
  };
#endif
