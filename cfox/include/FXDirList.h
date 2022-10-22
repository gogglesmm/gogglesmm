/********************************************************************************
*                                                                               *
*                     D i r e c t o r y   L i s t   W i d g e t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
********************************************************************************/
#ifndef FXDIRLIST_H
#define FXDIRLIST_H

#ifndef FXTREELIST_H
#include "FXTreeList.h"
#endif

namespace FX {


class FXDirList;
class FXFileAssociations;
struct FXFileAssoc;


/// Directory List options
enum {
  DIRLIST_SHOWFILES     = 0x08000000, /// Show files as well as directories
  DIRLIST_SHOWHIDDEN    = 0x10000000, /// Show hidden files or directories
  DIRLIST_NO_OWN_ASSOC  = 0x20000000  /// Do not create associations for files
  };


/// Directory item
class FXAPI FXDirItem : public FXTreeItem {
  FXDECLARE(FXDirItem)
  friend class FXDirList;
protected:
  FXFileAssoc *assoc;           // File association
  FXDirItem   *link;            // Link to next item
  FXDirItem   *list;            // List of child items
  FXlong       size;            // File size (if a file)
  FXTime       date;            // Time of item
  FXuint       mode;            // Mode flags
private:
  FXDirItem(const FXDirItem&);
  FXDirItem& operator=(const FXDirItem&);
protected:
  FXDirItem():assoc(nullptr),link(nullptr),list(nullptr),size(0L),date(0),mode(0){}
public:

  /// Constructor
  FXDirItem(const FXString& text,FXIcon* oi=nullptr,FXIcon* ci=nullptr,void* ptr=nullptr):FXTreeItem(text,oi,ci,ptr),assoc(nullptr),link(nullptr),list(nullptr),size(0),date(0),mode(0){}

  /// Return true if this is a file item
  FXbool isFile() const { return (mode&(FXIO::File))!=0; }

  /// Return true if this is a directory item
  FXbool isDirectory() const { return (mode&FXIO::Directory)!=0; }

  /// Return true if this is an executable item
  FXbool isExecutable() const { return (mode&FXIO::File)!=0 && (mode&FXIO::AllExec)!=0; }

  /// Return true if this is a symbolic link item
  FXbool isSymlink() const { return (mode&FXIO::SymLink)!=0; }

  /// Return true if this is a character device item
  FXbool isChardev() const { return (mode&FXIO::Character)!=0; }

  /// Return true if this is a block device item
  FXbool isBlockdev() const { return (mode&FXIO::Block)!=0; }

  /// Return true if this is an FIFO item
  FXbool isFifo() const { return (mode&FXIO::Fifo)!=0; }

  /// Return true if this is a socket
  FXbool isSocket() const { return (mode&FXIO::Socket)!=0; }

  /// Set the file-association object for this item
  void setAssoc(FXFileAssoc* a){ assoc=a; }

  /// Return the file-association object for this item
  FXFileAssoc* getAssoc() const { return assoc; }

  /// Set the file size for this item
  void setSize(FXlong s){ size=s; }

  /// Return the file size for this item
  FXlong getSize() const { return size; }

  /// Set the date for this item, in nanoseconds
  void setDate(FXTime d){ date=d; }

  /// Return the date for this item, in nanoseconds
  FXTime getDate() const { return date; }

  /// Set file mode bits
  void setMode(FXuint m){ mode=m; }

  /// Return file mode flags
  FXuint getMode() const { return mode; }
  };


/**
* A Directory List widget provides a tree-structured view of the file system.
* It automatically updates itself periodically by re-scanning the file system
* for any changes.  As it scans the displayed directories and files, it automatically
* determines the icons to be displayed by consulting the file-associations registry
* settings.  A number of messages can be sent to the Directory List to control the
* filter pattern, sorting order, case sensitivity, and hidden file display mode.
* The Directory list widget supports drags and drops of files.
*/
class FXAPI FXDirList : public FXTreeList {
  FXDECLARE(FXDirList)
protected:
  FXFileAssociations *associations;     // Association table
  FXDirItem          *list;             // Root item list
  FXIcon             *opendiricon;      // Open folder icon
  FXIcon             *closeddiricon;    // Closed folder icon
  FXIcon             *documenticon;     // Document icon
  FXIcon             *applicationicon;  // Application icon
  FXIcon             *cdromicon;        // CDROM icon
  FXIcon             *harddiskicon;     // Hard drive icon
  FXIcon             *networkicon;      // Network icon
  FXIcon             *floppyicon;       // Floppy icon
  FXIcon             *zipdiskicon;      // Zip disk icon
  FXString            pattern;          // Pattern of file names
  FXString            dropdirectory;    // Drop directory
  FXString            dragfiles;        // Dragged file names
  FXString            dropfiles;        // Dropped file names
  FXDragAction        dropaction;       // Drop action
  FXuint              matchmode;        // File wildcard match mode
  FXuint              counter;          // Refresh counter
  FXbool              draggable;        // Dragable files
protected:
  FXDirList();
  void listItems(FXbool force,FXbool notify);
  void listRootItems(FXbool force,FXbool notify);
  FXbool listChildItems(FXDirItem *par,FXbool force,FXbool notify);
  FXString getSelectedFiles() const;
  virtual FXTreeItem* createItem(const FXString& text,FXIcon* oi,FXIcon* ci,void* ptr);
  FXTreeItem* expandPath(const FXString& path,FXbool notify);
private:
  FXDirList(const FXDirList&);
  FXDirList &operator=(const FXDirList&);
public:
  long onRefreshTimer(FXObject*,FXSelector,void*);
  long onBeginDrag(FXObject*,FXSelector,void*);
  long onEndDrag(FXObject*,FXSelector,void*);
  long onDragged(FXObject*,FXSelector,void*);
  long onDNDEnter(FXObject*,FXSelector,void*);
  long onDNDLeave(FXObject*,FXSelector,void*);
  long onDNDMotion(FXObject*,FXSelector,void*);
  long onDNDDrop(FXObject*,FXSelector,void*);
  long onDNDRequest(FXObject*,FXSelector,void*);
  long onCmdSetValue(FXObject*,FXSelector,void*);
  long onCmdSetStringValue(FXObject*,FXSelector,void*);
  long onCmdGetStringValue(FXObject*,FXSelector,void*);
  long onCmdToggleHidden(FXObject*,FXSelector,void*);
  long onUpdToggleHidden(FXObject*,FXSelector,void*);
  long onCmdShowHidden(FXObject*,FXSelector,void*);
  long onUpdShowHidden(FXObject*,FXSelector,void*);
  long onCmdHideHidden(FXObject*,FXSelector,void*);
  long onUpdHideHidden(FXObject*,FXSelector,void*);
  long onCmdToggleFiles(FXObject*,FXSelector,void*);
  long onUpdToggleFiles(FXObject*,FXSelector,void*);
  long onCmdShowFiles(FXObject*,FXSelector,void*);
  long onUpdShowFiles(FXObject*,FXSelector,void*);
  long onCmdHideFiles(FXObject*,FXSelector,void*);
  long onUpdHideFiles(FXObject*,FXSelector,void*);
  long onCmdSetPattern(FXObject*,FXSelector,void*);
  long onUpdSetPattern(FXObject*,FXSelector,void*);
  long onCmdSortReverse(FXObject*,FXSelector,void*);
  long onUpdSortReverse(FXObject*,FXSelector,void*);
  long onCmdSortCase(FXObject*,FXSelector,void*);
  long onUpdSortCase(FXObject*,FXSelector,void*);
  long onCmdRefresh(FXObject*,FXSelector,void*);
  long onUpdHaveSel(FXObject*,FXSelector,void*);
  long onCmdDeleteSel(FXObject*,FXSelector,void*);
  long onCmdDropAsk(FXObject*,FXSelector,void*);
  long onCmdDropCopy(FXObject*,FXSelector,void*);
  long onCmdDropMove(FXObject*,FXSelector,void*);
  long onCmdDropLink(FXObject*,FXSelector,void*);
public:
  static FXint ascending(const FXTreeItem* a,const FXTreeItem* b);
  static FXint descending(const FXTreeItem* a,const FXTreeItem* b);
  static FXint ascendingCase(const FXTreeItem* a,const FXTreeItem* b);
  static FXint descendingCase(const FXTreeItem* a,const FXTreeItem* b);
public:
  enum {
    ID_REFRESHTIMER=FXTreeList::ID_LAST,
    ID_DROPASK,
    ID_DROPCOPY,
    ID_DROPMOVE,
    ID_DROPLINK,
    ID_SHOW_FILES,      /// Show files
    ID_HIDE_FILES,      /// Hide files
    ID_TOGGLE_FILES,    /// Toggle show files
    ID_SHOW_HIDDEN,     /// Show hidden files
    ID_HIDE_HIDDEN,     /// Hide hidden files
    ID_TOGGLE_HIDDEN,   /// Toggle display of hidden files
    ID_SET_PATTERN,     /// Set match pattern
    ID_SORT_REVERSE,    /// Reverse sort order
    ID_SORT_CASE,       /// Toggle sort case sensitivity
    ID_REFRESH,         /// Refresh immediately
    ID_DELETE_SEL,      /// Delete selected files
    ID_LAST
    };
public:

  /// Construct a directory list
  FXDirList(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Destroy server-side resources
  virtual void destroy();

  /// Set current file; return true if success
  FXbool setCurrentFile(const FXString& file,FXbool notify=false);

  /// Return current file
  FXString getCurrentFile() const;

  /// Set current directory; return true if success
  FXbool setDirectory(const FXString& path,FXbool notify=false);

  /// Return current directory
  FXString getDirectory() const;

  /// Return absolute pathname of item
  FXString getItemPathname(const FXTreeItem* item) const;

  /// Return the (closest) item from the absolute pathname
  FXTreeItem* getPathnameItem(const FXString& path) const;

  /// Return true if item is a file
  FXbool isItemFile(const FXTreeItem* item) const;

  /// Return true if item is a directory
  FXbool isItemDirectory(const FXTreeItem* item) const;

  /// Return true if item is executable
  FXbool isItemExecutable(const FXTreeItem* item) const;

  /// Return true if this is a symbolic link item
  FXbool isItemSymlink(const FXTreeItem* item) const;

  /// Return file association of item
  FXFileAssoc* getItemAssoc(const FXTreeItem* item) const;

  /// Return the file size for this item
  FXlong getItemSize(const FXTreeItem* item) const;

  /// Return the date for this item, in nanoseconds
  FXTime getItemDate(const FXTreeItem* item) const;

  /// Return the mode bits for this item
  FXuint getItemMode(const FXTreeItem* item) const;

  /// Collapse tree
  virtual FXbool collapseTree(FXTreeItem* tree,FXbool notify=false);

  /// Expand tree
  virtual FXbool expandTree(FXTreeItem* tree,FXbool notify=false);

  /// Change wildcard matching pattern
  FXbool selectMatching(const FXString& ptrn="*",FXuint mode=FXPath::PathName|FXPath::NoEscape,FXbool notify=false);

  /// Change wildcard matching pattern
  void setPattern(const FXString& ptrn="*",FXbool notify=false);

  /// Return wildcard pattern
  FXString getPattern() const { return pattern; }

  /// Change wildcard matching mode (see FXPath)
  void setMatchMode(FXuint mode=FXPath::PathName|FXPath::NoEscape,FXbool notify=false);

  /// Return wildcard matching mode
  FXuint getMatchMode() const { return matchmode; }

  /// Show or hide normal files
  void showFiles(FXbool flag,FXbool notify=false);

  /// Return true if showing files as well as directories
  FXbool showFiles() const;

  /// Show or hide hidden files and directories
  void showHiddenFiles(FXbool flag,FXbool notify=false);

  /// Return true if showing hidden files and directories
  FXbool showHiddenFiles() const;

  /// Change file associations; delete the old one unless it was shared
  void setAssociations(FXFileAssociations* assoc,FXbool owned=false,FXbool notify=false);

  /// Return file associations
  FXFileAssociations* getAssociations() const { return associations; }

  /// Set draggable files
  void setDraggableFiles(FXbool flag,FXbool notify=false);

  /// Are files draggable
  FXbool getDraggableFiles() const { return draggable; }

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXDirList();
  };

}

#endif
