/********************************************************************************
*                                                                               *
*                  F i l e   S e l e c t i o n   W i d g e t                    *
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
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "fxascii.h"
#include "fxkeys.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXObjectList.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXPath.h"
#include "FXSystem.h"
#include "FXStat.h"
#include "FXFile.h"
#include "FXDir.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXFont.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXBMPIcon.h"
#include "FXRecentFiles.h"
#include "FXFrame.h"
#include "FXLabel.h"
#include "FXTextField.h"
#include "FXButton.h"
#include "FXToggleButton.h"
#include "FXCheckButton.h"
#include "FXMenuButton.h"
#include "FXPacker.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXMatrix.h"
#include "FXShell.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXScrollBar.h"
#include "FXScrollArea.h"
#include "FXList.h"
#include "FXTreeList.h"
#include "FXComboBox.h"
#include "FXTreeListBox.h"
#include "FXDirBox.h"
#include "FXHeader.h"
#include "FXIconList.h"
#include "FXFileList.h"
#include "FXFileSelector.h"
#include "FXMenuCaption.h"
#include "FXMenuCommand.h"
#include "FXMenuCascade.h"
#include "FXMenuRadio.h"
#include "FXMenuCheck.h"
#include "FXMenuSeparator.h"
#include "FXTopWindow.h"
#include "FXDialogBox.h"
#include "FXInputDialog.h"
#include "FXSeparator.h"
#include "FXMessageBox.h"
#include "icons.h"

/*
  Notes:
  - Getting a file name according to what we want:

    - Any filename for saving (but with existing dir part)
    - An existing file for loading
    - An existing directory
    - Multiple filenames.
    - Multiple filenames or directories.

  - Get network drives to work.

  - Change filter specification; below sets two filters:

      "Source Files (*.cpp,*.cc,*.C)\nHeader files (*.h,*.H)"

    Instead of ',' you should also be able to use '|' in the above.

  - In multi-file mode, filenames are presented in the text box differently
    from single-file mode; in order to separate filenames that may contain
    special characters (including space), such filenames will be enquoted
    while quotes and escapes are escaped.  If no special characters are
    found in a filename, no quotes will be needed.

  - Got nifty handling when entering path in text field:

      1) If its a directory you typed, switch to the directory.

      2) If the directory part of the file name exists:
         if SELECTFILE_ANY mode, then we're done.
         if SELECTFILE_EXISTING mode AND file exists, we're done.
         if SELECTFILE_MULTIPLE mode AND all files exist, we're done.

      3) Else use the fragment of the directory which still exists, and
         switch to that directory; leave the incorrect tail-end in the
         text field to be edited further

  - In directory mode, only way to return is by accept.

  - Switching directories zaps text field value, but not in SELECTFILE_ANY
    mode, because when saving a file you may want to give the same name
    even if directory changes.

  - We change file extension when switching file filters, but only in SELECTFILE_ANY
    mode, i.e. when we're saving the file, we change the extension appropriately.

  - The ".." is excluded from SELECTFILE_MULTIPLE_ALL selections.

  - Drag corner would be nice.

  - When copying, moving, deleting, linking multiple files, build the list
    of selected files first, to take care of FXFileList possibly updating
    before operation is finished.

  - FXDirBox should remember up to deepest visited path; when you go down
    another directory, remove knowledge of where we've been.  In other words,
    remember latest path only.
*/


#define FILELISTMASK  (ICONLIST_EXTENDEDSELECT|ICONLIST_SINGLESELECT|ICONLIST_BROWSESELECT|ICONLIST_MULTIPLESELECT)
#define FILESTYLEMASK (ICONLIST_DETAILED|ICONLIST_MINI_ICONS|ICONLIST_BIG_ICONS|ICONLIST_ROWS|ICONLIST_COLUMNS|ICONLIST_AUTOSIZE)


using namespace FX;

/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXFileSelector) FXFileSelectorMap[]={
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_ACCEPT,FXFileSelector::onCmdAccept),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_FILEFILTER,FXFileSelector::onCmdFilter),
  FXMAPFUNC(SEL_DOUBLECLICKED,FXFileSelector::ID_FILELIST,FXFileSelector::onCmdItemDblClicked),
  FXMAPFUNC(SEL_SELECTED,FXFileSelector::ID_FILELIST,FXFileSelector::onCmdItemSelected),
  FXMAPFUNC(SEL_DESELECTED,FXFileSelector::ID_FILELIST,FXFileSelector::onCmdItemDeselected),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,FXFileSelector::ID_FILELIST,FXFileSelector::onPopupMenu),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_DIRECTORY_UP,FXFileSelector::onCmdDirectoryUp),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_DIRECTORY_UP,FXFileSelector::onUpdDirectoryUp),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_DIRTREE,FXFileSelector::onUpdDirTree),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_DIRTREE,FXFileSelector::onCmdDirTree),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_HOME,FXFileSelector::onCmdHome),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_HOME,FXFileSelector::onUpdNavigable),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_WORK,FXFileSelector::onCmdWork),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_WORK,FXFileSelector::onUpdNavigable),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_VISIT,FXFileSelector::onCmdVisit),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_BOOKMARK,FXFileSelector::onCmdBookmark),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_UNBOOKMARK,FXFileSelector::onCmdUnBookmark),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_NEW,FXFileSelector::onCmdNew),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_NEW,FXFileSelector::onUpdNew),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_RENAME,FXFileSelector::onCmdRename),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_COPY,FXFileSelector::onCmdCopy),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_MOVE,FXFileSelector::onCmdMove),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_LINK,FXFileSelector::onCmdLink),
  FXMAPFUNC(SEL_COMMAND,FXFileSelector::ID_REMOVE,FXFileSelector::onCmdRemove),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_RENAME,FXFileSelector::onUpdSelected),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_COPY,FXFileSelector::onUpdSelected),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_MOVE,FXFileSelector::onUpdSelected),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_LINK,FXFileSelector::onUpdSelected),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_REMOVE,FXFileSelector::onUpdSelected),
  FXMAPFUNC(SEL_UPDATE,FXFileSelector::ID_BOOKMENU,FXFileSelector::onUpdNavigable),
  FXMAPFUNCS(SEL_COMMAND,FXFileSelector::ID_MINI_SIZE,FXFileSelector::ID_GIANT_SIZE,FXFileSelector::onCmdImageSize),
  FXMAPFUNCS(SEL_UPDATE,FXFileSelector::ID_MINI_SIZE,FXFileSelector::ID_GIANT_SIZE,FXFileSelector::onUpdImageSize),
  };


// Implementation
FXIMPLEMENT(FXFileSelector,FXPacker,FXFileSelectorMap,ARRAYNUMBER(FXFileSelectorMap))


/*******************************************************************************/

// Separator item
FXFileSelector::FXFileSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXPacker(p,opts,x,y,w,h,DEFAULT_SPACING,DEFAULT_SPACING,DEFAULT_SPACING,DEFAULT_SPACING,8,8),bookmarks(p->getApp(),"Visited Directories"),selectmode(SELECTFILE_ANY){
  FXAccelTable *table=getShell()->getAccelTable();
  target=tgt;
  message=sel;
  navbuttons=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0, 0,0,0,0, 0,0);
  entryblock=new FXMatrix(this,3,MATRIX_BY_COLUMNS|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0, 0,0,0,0);
  new FXLabel(entryblock,tr("&File Name:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  filename=new FXTextField(entryblock,25,this,ID_ACCEPT,TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK);
  new FXButton(entryblock,tr("&OK"),nullptr,this,ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  accept=new FXButton(navbuttons,FXString::null,nullptr,nullptr,0,LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,0,0, 0,0,0,0);
  accept->hide();
  new FXLabel(entryblock,tr("File F&ilter:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  FXHorizontalFrame *filterframe=new FXHorizontalFrame(entryblock,LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);
  filefilter=new FXComboBox(filterframe,10,this,ID_FILEFILTER,COMBOBOX_STATIC|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK);
  filefilter->setNumVisible(4);
  readonly=new FXCheckButton(filterframe,tr("Read Only"),nullptr,0,ICON_BEFORE_TEXT|JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  cancel=new FXButton(entryblock,tr("&Cancel"),nullptr,nullptr,0,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  fileboxframe=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0,0,0,0,0);
  filebox=new FXFileList(fileboxframe,this,ID_FILELIST,ICONLIST_MINI_ICONS|ICONLIST_BROWSESELECT|ICONLIST_AUTOSIZE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->setDraggableFiles(false);
  filebox->setFocus();
  new FXLabel(navbuttons,tr("Directory:"),nullptr,LAYOUT_CENTER_Y);
  updiricon=new FXGIFIcon(getApp(),dirupicon);
  listicon=new FXGIFIcon(getApp(),showsmallicons);
  iconsicon=new FXGIFIcon(getApp(),showbigicons);
  detailicon=new FXGIFIcon(getApp(),showdetails);
  homeicon=new FXGIFIcon(getApp(),gotohome);
  workicon=new FXGIFIcon(getApp(),gotowork);
  shownicon=new FXGIFIcon(getApp(),fileshown);
  hiddenicon=new FXGIFIcon(getApp(),filehidden);
  bookmarkicon=new FXGIFIcon(getApp(),bookset);
  bookaddicon=new FXBMPIcon(getApp(),bookadd,0,IMAGE_ALPHAGUESS);
  bookdelicon=new FXBMPIcon(getApp(),bookdel,0,IMAGE_ALPHAGUESS);
  bookclricon=new FXGIFIcon(getApp(),bookclr);
  sortingicon=new FXBMPIcon(getApp(),sorting,0,IMAGE_ALPHAGUESS);
  newicon=new FXGIFIcon(getApp(),foldernew);
  renameicon=new FXGIFIcon(getApp(),filerename);
  copyicon=new FXGIFIcon(getApp(),filecopy);
  moveicon=new FXGIFIcon(getApp(),filemove);
  linkicon=new FXGIFIcon(getApp(),filelink);
  deleteicon=new FXGIFIcon(getApp(),filedelete);
  dirbox=new FXDirBox(navbuttons,this,ID_DIRTREE,DIRBOX_NO_OWN_ASSOC|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,1,1,1,1);
  dirbox->setNumVisible(5);
  dirbox->setAssociations(filebox->getAssociations(),false);    // Shared file associations
  bookmarkmenu=new FXMenuPane(this,POPUP_SHRINKWRAP);
  new FXMenuCommand(bookmarkmenu,tr("&Set bookmark\t\tBookmark current directory."),bookaddicon,this,ID_BOOKMARK);
  new FXMenuCommand(bookmarkmenu,tr("&Unset bookmark\t\tRemove current directory bookmark."),bookdelicon,this,ID_UNBOOKMARK);
  new FXMenuCommand(bookmarkmenu,tr("&Clear all bookmarks\t\tClear all bookmarks."),bookclricon,&bookmarks,FXRecentFiles::ID_CLEAR);
  FXMenuSeparator* sep1=new FXMenuSeparator(bookmarkmenu);
  sep1->setTarget(&bookmarks);
  sep1->setSelector(FXRecentFiles::ID_ANYFILES);
  new FXMenuCommand(bookmarkmenu,"&1",nullptr,&bookmarks,FXRecentFiles::ID_FILE_1);
  new FXMenuCommand(bookmarkmenu,"&2",nullptr,&bookmarks,FXRecentFiles::ID_FILE_2);
  new FXMenuCommand(bookmarkmenu,"&3",nullptr,&bookmarks,FXRecentFiles::ID_FILE_3);
  new FXMenuCommand(bookmarkmenu,"&4",nullptr,&bookmarks,FXRecentFiles::ID_FILE_4);
  new FXMenuCommand(bookmarkmenu,"&5",nullptr,&bookmarks,FXRecentFiles::ID_FILE_5);
  new FXMenuCommand(bookmarkmenu,"&6",nullptr,&bookmarks,FXRecentFiles::ID_FILE_6);
  new FXMenuCommand(bookmarkmenu,"&7",nullptr,&bookmarks,FXRecentFiles::ID_FILE_7);
  new FXMenuCommand(bookmarkmenu,"&8",nullptr,&bookmarks,FXRecentFiles::ID_FILE_8);
  new FXMenuCommand(bookmarkmenu,"&9",nullptr,&bookmarks,FXRecentFiles::ID_FILE_9);
  new FXMenuCommand(bookmarkmenu,"1&0",nullptr,&bookmarks,FXRecentFiles::ID_FILE_10);
  new FXMenuCommand(bookmarkmenu,"11",nullptr,&bookmarks,FXRecentFiles::ID_FILE_11);
  new FXMenuCommand(bookmarkmenu,"12",nullptr,&bookmarks,FXRecentFiles::ID_FILE_12);
  new FXMenuCommand(bookmarkmenu,"13",nullptr,&bookmarks,FXRecentFiles::ID_FILE_13);
  new FXMenuCommand(bookmarkmenu,"14",nullptr,&bookmarks,FXRecentFiles::ID_FILE_14);
  new FXMenuCommand(bookmarkmenu,"15",nullptr,&bookmarks,FXRecentFiles::ID_FILE_15);
  new FXMenuCommand(bookmarkmenu,"16",nullptr,&bookmarks,FXRecentFiles::ID_FILE_16);
  new FXMenuCommand(bookmarkmenu,"17",nullptr,&bookmarks,FXRecentFiles::ID_FILE_17);
  new FXMenuCommand(bookmarkmenu,"18",nullptr,&bookmarks,FXRecentFiles::ID_FILE_18);
  new FXMenuCommand(bookmarkmenu,"19",nullptr,&bookmarks,FXRecentFiles::ID_FILE_19);
  new FXMenuCommand(bookmarkmenu,"20",nullptr,&bookmarks,FXRecentFiles::ID_FILE_20);
  new FXFrame(navbuttons,LAYOUT_FIX_WIDTH,0,0,4,1);
  new FXButton(navbuttons,tr("\tGo up one directory\tMove up to higher directory."),updiricon,this,ID_DIRECTORY_UP,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXButton(navbuttons,tr("\tGo to home directory\tBack to home directory."),homeicon,this,ID_HOME,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXButton(navbuttons,tr("\tGo to work directory\tBack to working directory."),workicon,this,ID_WORK,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  FXMenuButton *bookmenu=new FXMenuButton(navbuttons,tr("\tBookmarks\tVisit bookmarked directories."),bookmarkicon,bookmarkmenu,MENUBUTTON_NOARROWS|MENUBUTTON_ATTACH_LEFT|MENUBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  bookmenu->setTarget(this);
  bookmenu->setSelector(ID_BOOKMENU);
  new FXButton(navbuttons,tr("\tCreate new directory\tCreate new directory."),newicon,this,ID_NEW,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXButton(navbuttons,tr("\tShow list\tDisplay directory with small icons."),listicon,filebox,FXFileList::ID_SHOW_MINI_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXButton(navbuttons,tr("\tShow icons\tDisplay directory with big icons."),iconsicon,filebox,FXFileList::ID_SHOW_BIG_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXButton(navbuttons,tr("\tShow details\tDisplay detailed directory listing."),detailicon,filebox,FXFileList::ID_SHOW_DETAILS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new FXToggleButton(navbuttons,tr("\tShow hidden files\tShow hidden files and directories."),tr("\tHide Hidden Files\tHide hidden files and directories."),hiddenicon,shownicon,filebox,FXFileList::ID_TOGGLE_HIDDEN,TOGGLEBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  readonly->hide();
  if(table){
    table->addAccel(MKUINT(KEY_BackSpace,0),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_DIRECTORY_UP));
    table->addAccel(MKUINT(KEY_Delete,0),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_REMOVE));
    table->addAccel(MKUINT(KEY_h,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_HOME));
    table->addAccel(MKUINT(KEY_w,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_WORK));
    table->addAccel(MKUINT(KEY_n,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_NEW));
    table->addAccel(MKUINT(KEY_a,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SELECT_ALL));
    table->addAccel(MKUINT(KEY_b,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_BIG_ICONS));
    table->addAccel(MKUINT(KEY_s,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_MINI_ICONS));
    table->addAccel(MKUINT(KEY_l,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_DETAILS));
    table->addAccel(MKUINT(KEY_c,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_COPY_SEL));
    table->addAccel(MKUINT(KEY_x,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_CUT_SEL));
    table->addAccel(MKUINT(KEY_v,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_PASTE_SEL));
    }

  // Now use up to 15 bookmarked directories
  bookmarks.setMaxFiles(20);
  bookmarks.setTarget(this);
  bookmarks.setSelector(ID_VISIT);

  // For backward compatibility, this HAS to be the default!
  setSelectMode(SELECTFILE_ANY);

  // Initial pattern
  setPatternList(tr("All Files (*)"));

  // Consistent value in dir box; don't rescan just yet!
  dirbox->setDirectory(filebox->getDirectory());

  // Default is navigation is allowed
  navigable=true;
  }


// Create server-side resources
void FXFileSelector::create(){
  FXPacker::create();
  if(selectmode==SELECTFILE_ANY){
    filename->setFocus();
    filename->selectAll();
    }
  }


// Count number of files in encoded list of filenames
FXint FXFileSelector::countFilenames(const FXString& string){
  FXint result=0,p=0;
  do{
    while(string[p]){
      if(string[p]==','){
        if(string[p+1]!=',') break;
        p++;
        }
      p++;
      }
    result++;
    }
  while(string[p++]==',');
  return result;
  }


// Decode the n-th filename from string containing multiple filenames
// Return the empty string if n exceeds the number of filenames present
FXString FXFileSelector::decodeFilename(const FXString& string,FXint n){
  FXString result;
  FXint p=0,pp,q;
  do{
    pp=p;
    q=0;
    while(string[p]){
      if(string[p]==','){
        if(string[p+1]!=',') break;
        p++;
        }
      p++;
      q++;
      }
    if(--n<0){
      result.length(q);
      p=pp;
      q=0;
      while(string[p]){
        if(string[p]==','){
          if(string[p+1]!=',') break;
          p++;
          }
        result[q++]=string[p++];
        }
      FXASSERT(q<=result.length());
      break;
      }
    }
  while(string[p++]==',');
  return result;
  }


// Encode a filename
// Filenames that contain a ',' will replace it with ',,'.
FXString FXFileSelector::encodeFilename(const FXString& string){
  FXString result;
  FXint p=0,q=0;
  while(string[p]){
    if(string[p]==',') q++;
    p++;
    q++;
    }
  result.length(q);
  p=q=0;
  while(string[p]){
    if(string[p]==','){ result[q++]=','; }
    result[q++]=string[p++];
    }
  FXASSERT(q<=result.length());
  return result;
  }


// Change in items which are selected
long FXFileSelector::onCmdItemSelected(FXObject*,FXSelector,void* ptr){
  FXint index=(FXint)(FXival)ptr;
  FXString text;
  if(selectmode==SELECTFILE_MULTIPLE){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(!filebox->isItemDirectory(i) && filebox->isItemSelected(i)){
        if(!text.empty()) text+=',';
        text+=encodeFilename(filebox->getItemFilename(i));
        }
      }
    filename->setText(text);
    }
  else if(selectmode==SELECTFILE_MULTIPLE_ALL){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(!filebox->isItemNavigational(i) && filebox->isItemSelected(i)){
        if(!text.empty()) text+=',';
        text+=encodeFilename(filebox->getItemFilename(i));
        }
      }
    filename->setText(text);
    }
  else if(selectmode==SELECTFILE_DIRECTORY){
    if(filebox->isItemDirectory(index)){
      text=filebox->getItemFilename(index);
      filename->setText(text);
      }
    }
  else{
    if(!filebox->isItemDirectory(index)){
      text=filebox->getItemFilename(index);
      filename->setText(text);
      }
    }
  return 1;
  }


// Change in items which are selected
long FXFileSelector::onCmdItemDeselected(FXObject*,FXSelector,void*){
  FXString text;
  if(selectmode==SELECTFILE_MULTIPLE){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(!filebox->isItemDirectory(i) && filebox->isItemSelected(i)){
        if(!text.empty()) text+=',';
        text+=encodeFilename(filebox->getItemFilename(i));
        }
      }
    filename->setText(text);
    }
  else if(selectmode==SELECTFILE_MULTIPLE_ALL){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(!filebox->isItemNavigational(i) && filebox->isItemSelected(i)){
        if(!text.empty()) text+=',';
        text+=encodeFilename(filebox->getItemFilename(i));
        }
      }
    filename->setText(text);
    }
  else if(selectmode!=SELECTFILE_ANY){
    filename->setText(FXString::null);
    }
  return 1;
  }


// Double-clicked item in file list
long FXFileSelector::onCmdItemDblClicked(FXObject*,FXSelector,void* ptr){
  FXObject *tgt=accept->getTarget();
  FXSelector sel=accept->getSelector();
  FXint index=(FXint)(FXival)ptr;
  if(0<=index){

    // If directory, open the directory
    if(filebox->isItemDirectory(index)){
      if(allowNavigation()) setDirectory(filebox->getItemPathname(index));
      return 1;
      }

    // Only return if we wanted a file
    if(selectmode!=SELECTFILE_DIRECTORY){
      if(tgt) tgt->handle(accept,FXSEL(SEL_COMMAND,sel),(void*)(FXuval)1);
      }
    }
  return 1;
  }


// Hit the accept button or enter in text field
long FXFileSelector::onCmdAccept(FXObject*,FXSelector,void*){
  FXObject *tgt=accept->getTarget();
  FXSelector sel=accept->getSelector();

  // Get (first) filename or directory
  FXString path=getFilename();

  // Only do something if a selection was made
  if(!path.empty()){

    // Is directory?
    if(FXStat::isDirectory(path)){

      // In directory mode:- we got our answer!
      if(selectmode==SELECTFILE_DIRECTORY || selectmode==SELECTFILE_MULTIPLE_ALL){
        if(tgt) tgt->handle(accept,FXSEL(SEL_COMMAND,sel),(void*)(FXuval)1);
        return 1;
        }

      // No navigation allowed
      if(!allowNavigation()){
        filename->setText(FXPath::relative(getDirectory(),path));
        filename->selectAll();
        getApp()->beep();
        return 1;
        }

      // Hop over to that directory
      setDirectory(path);
      return 1;
      }

    // Get directory part of path
    FXString dir=FXPath::directory(path);

    // In file mode, directory part of path should exist
    if(FXStat::isDirectory(dir)){

      // In any mode, existing directory part is good enough
      if(selectmode==SELECTFILE_ANY){
        if(tgt) tgt->handle(accept,FXSEL(SEL_COMMAND,sel),(void*)(FXuval)1);
        return 1;
        }

      // Otherwise, the whole filename must exist and be a file
      if(FXStat::exists(path)){
        if(tgt) tgt->handle(accept,FXSEL(SEL_COMMAND,sel),(void*)(FXuval)1);
        return 1;
        }
      }

    // No navigation allowed
    if(!allowNavigation()){
      filename->setText(FXPath::relative(getDirectory(),path));
      filename->selectAll();
      getApp()->beep();
      return 1;
      }

    // Go up to the lowest directory which still exists
    dir=FXPath::validPath(dir);

    // Switch as far as we could go
    setDirectory(dir);

    // Put the tail end back for further editing
    FXASSERT(dir.length()<=path.length());
    if(ISPATHSEP(path[dir.length()]))
      path.erase(0,dir.length()+1);
    else
      path.erase(0,dir.length());

    // Replace text box with new stuff
    filename->setText(path);
    filename->selectAll();
    }

  // Beep
  getApp()->beep();
  return 1;
  }


// User clicked up directory button; we move to the next higher directory,
// and select the directory we just came from in that directory; this allows
// a quick jump back into the original directory in case we went up too far.
long FXFileSelector::onCmdDirectoryUp(FXObject*,FXSelector,void*){
  if(allowNavigation() && !FXPath::isTopDirectory(getDirectory())){
    FXString dir(getDirectory());
    setDirectory(FXPath::upLevel(dir));
    filebox->setCurrentFile(dir);
    return 1;
    }
  getApp()->beep();
  return 1;
  }


// Can we still go up
long FXFileSelector::onUpdDirectoryUp(FXObject* sender,FXSelector,void*){
  sender->handle(this,allowNavigation() && !FXPath::isTopDirectory(getDirectory()) ? FXSEL(SEL_COMMAND,ID_ENABLE) : FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Can we navigate
long FXFileSelector::onUpdNavigable(FXObject* sender,FXSelector,void*){
  sender->handle(this,allowNavigation() ? FXSEL(SEL_COMMAND,ID_ENABLE) : FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Back to home directory
long FXFileSelector::onCmdHome(FXObject*,FXSelector,void*){
  if(allowNavigation()){
    setDirectory(FXSystem::getHomeDirectory());
    return 1;
    }
  getApp()->beep();
  return 1;
  }


// Back to current working directory
long FXFileSelector::onCmdWork(FXObject*,FXSelector,void*){
  if(allowNavigation()){
    setDirectory(FXSystem::getCurrentDirectory());
    return 1;
    }
  getApp()->beep();
  return 1;
  }


// Move to recent directory
long FXFileSelector::onCmdVisit(FXObject*,FXSelector,void* ptr){
  if(allowNavigation()){
    FXString path((const FXchar*)ptr);
    if(FXStat::exists(path)){
      setDirectory(path);
      return 1;
      }
    bookmarks.removeFile(path);
    }
  getApp()->beep();
  return 1;
  }


// Bookmark this directory
long FXFileSelector::onCmdBookmark(FXObject*,FXSelector,void*){
  bookmarks.appendFile(getDirectory());
  return 1;
  }


// Un-bookmark this directory
long FXFileSelector::onCmdUnBookmark(FXObject*,FXSelector,void*){
  bookmarks.removeFile(getDirectory());
  return 1;
  }


// Switched directories using directory tree
long FXFileSelector::onUpdDirTree(FXObject*,FXSelector,void*){
  dirbox->setDirectory(filebox->getDirectory());
  return 1;
  }


// Switched directories using directory tree
long FXFileSelector::onCmdDirTree(FXObject*,FXSelector,void* ptr){
  if(allowNavigation()){
    filebox->setDirectory((FXchar*)ptr,true);
    if(selectmode==SELECTFILE_DIRECTORY) filename->setText(FXString::null);
    }
  else{
    dirbox->setDirectory(filebox->getDirectory());
    }
  return 1;
  }


// Create new directory
long FXFileSelector::onCmdNew(FXObject*,FXSelector,void*){
  FXBMPIcon newfoldericon(getApp(),newfolder,0,IMAGE_ALPHAGUESS);
  FXString name(tr("folder"));
  if(FXInputDialog::getString(name,this,tr("Create New Directory"),tr("Create new directory with name: "),&newfoldericon)){
    FXString dirname=FXPath::absolute(getDirectory(),name);
    if(FXStat::exists(dirname)){
      FXMessageBox::error(this,MBOX_OK,tr("Already Exists"),tr("File or directory %s already exists.\n"),dirname.text());
      return 1;
      }
    if(!FXDir::create(dirname)){
      FXMessageBox::error(this,MBOX_OK,tr("Cannot Create"),tr("Cannot create directory %s.\n"),dirname.text());
      return 1;
      }
    setDirectory(dirname);
    }
  return 1;
  }


// Update create new directory
long FXFileSelector::onUpdNew(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXStat::isAccessible(getDirectory(),FXIO::Writing)?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// FIXME
// Why call getFilenames() when we can loop through the text with decodeFilename now?

// Rename file or directory
long FXFileSelector::onCmdRename(FXObject*,FXSelector,void*){
  FXString *filenamelist=getFilenames();
  FXString renamemessage;
  FXString oldname;
  FXString newname;
  if(filenamelist){
    for(FXint i=0; !filenamelist[i].empty(); i++){
      oldname=FXPath::name(filenamelist[i]);
      renamemessage.format(tr("Rename file from:\n\n%s\n\nto: "),oldname.text());
      FXInputDialog inputdialog(this,tr("Rename File"),renamemessage,nullptr,INPUTDIALOG_STRING,0,0,0,0);
      inputdialog.setText(oldname);
      inputdialog.setNumColumns(60);
      if(inputdialog.execute()){
        newname=inputdialog.getText();
        if(!FXFile::moveFiles(filenamelist[i],FXPath::absolute(FXPath::directory(filenamelist[i]),newname),false)){
          if(FXMessageBox::error(this,MBOX_YES_NO,tr("Error Renaming File"),tr("Unable to rename file:\n\n%s  to:  %s\n\nContinue with operation?"),oldname.text(),newname.text())==MBOX_CLICKED_NO) break;
          }
        }
      }
    delete [] filenamelist;
    }
  return 1;
  }


// Copy file or directory
long FXFileSelector::onCmdCopy(FXObject*,FXSelector,void*){
  FXString *filenamelist=getFilenames();
  FXString copymessage;
  if(filenamelist){
    for(FXint i=0; !filenamelist[i].empty(); i++){
      copymessage.format(tr("Copy file from location:\n\n%s\n\nto location: "),filenamelist[i].text());
      FXInputDialog inputdialog(this,tr("Copy File"),copymessage,nullptr,INPUTDIALOG_STRING,0,0,0,0);
      inputdialog.setText(FXPath::absolute(FXPath::directory(filenamelist[i]),"CopyOf"+FXPath::name(filenamelist[i])));
      inputdialog.setNumColumns(60);
      if(inputdialog.execute()){
        FXString newname=inputdialog.getText();
        if(!FXFile::copyFiles(filenamelist[i],newname,false)){
          if(FXMessageBox::error(this,MBOX_YES_NO,tr("Error Copying File"),tr("Unable to copy file:\n\n%s  to:  %s\n\nContinue with operation?"),filenamelist[i].text(),newname.text())==MBOX_CLICKED_NO) break;
          }
        }
      }
    delete [] filenamelist;
    }
  return 1;
  }


// Move file or directory
long FXFileSelector::onCmdMove(FXObject*,FXSelector,void*){
  FXString *filenamelist=getFilenames();
  FXString movemessage;
  if(filenamelist){
    for(FXint i=0; !filenamelist[i].empty(); i++){
      movemessage.format(tr("Move file from location:\n\n%s\n\nto location: "),filenamelist[i].text());
      FXInputDialog inputdialog(this,tr("Move File"),movemessage,nullptr,INPUTDIALOG_STRING,0,0,0,0);
      inputdialog.setText(filenamelist[i]);
      inputdialog.setNumColumns(60);
      if(inputdialog.execute()){
        FXString newname=inputdialog.getText();
        if(!FXFile::moveFiles(filenamelist[i],newname,false)){
          if(FXMessageBox::error(this,MBOX_YES_NO,tr("Error Moving File"),tr("Unable to move file:\n\n%s  to:  %s\n\nContinue with operation?"),filenamelist[i].text(),newname.text())==MBOX_CLICKED_NO) break;
          }
        }
      }
    delete [] filenamelist;
    }
  return 1;
  }


// Link file or directory
long FXFileSelector::onCmdLink(FXObject*,FXSelector,void*){
  FXString *filenamelist=getFilenames();
  FXString linkmessage;
  if(filenamelist){
    for(FXint i=0; !filenamelist[i].empty(); i++){
      linkmessage.format(tr("Link file from location:\n\n%s\n\nto location: "),filenamelist[i].text());
      FXInputDialog inputdialog(this,tr("Link File"),linkmessage,nullptr,INPUTDIALOG_STRING,0,0,0,0);
      inputdialog.setText(FXPath::absolute(FXPath::directory(filenamelist[i]),"LinkTo"+FXPath::name(filenamelist[i])));
      inputdialog.setNumColumns(60);
      if(inputdialog.execute()){
        FXString newname=inputdialog.getText();
        if(!FXFile::symlink(filenamelist[i],newname)){
          if(FXMessageBox::error(this,MBOX_YES_NO,tr("Error Linking File"),tr("Unable to link file:\n\n%s  to:  %s\n\nContinue with operation?"),filenamelist[i].text(),newname.text())==MBOX_CLICKED_NO) break;
          }
        }
      }
    delete [] filenamelist;
    }
  return 1;
  }


// Remove file or directory
long FXFileSelector::onCmdRemove(FXObject*,FXSelector,void*){
  FXString *filenamelist=getFilenames();
  FXuint answer;
  if(filenamelist){
    for(FXint i=0; !filenamelist[i].empty(); i++){
      answer=FXMessageBox::warning(this,MBOX_YES_NO_CANCEL,tr("Deleting files"),tr("Are you sure you want to delete the file:\n\n%s"),filenamelist[i].text());
      if(answer==MBOX_CLICKED_CANCEL) break;
      if(answer==MBOX_CLICKED_NO) continue;
      if(!FXFile::removeFiles(filenamelist[i],true)){
        if(FXMessageBox::error(this,MBOX_YES_NO,tr("Error Deleting File"),tr("Unable to delete file:\n\n%s\n\nContinue with operation?"),filenamelist[i].text())==MBOX_CLICKED_NO) break;
        }
      }
    delete [] filenamelist;
    }
  return 1;
  }


// Sensitize when files are selected
long FXFileSelector::onUpdSelected(FXObject* sender,FXSelector,void*){
  for(FXint i=0; i<filebox->getNumItems(); i++){
    if(filebox->isItemSelected(i) && !filebox->isItemNavigational(i)){
      sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
      return 1;
      }
    }
  sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
  return 1;
  }


// Change image size
long FXFileSelector::onCmdImageSize(FXObject*,FXSelector sel,void*){
  switch(FXSELID(sel)){
    case ID_MINI_SIZE: setImageSize(16); break;
    case ID_NORMAL_SIZE: setImageSize(32); break;
    case ID_MEDIUM_SIZE: setImageSize(64); break;
    case ID_GIANT_SIZE: setImageSize(128); break;
    }
  return 1;
  }


// Update image size
long FXFileSelector::onUpdImageSize(FXObject* sender,FXSelector sel,void*){
  FXbool check=false;
  switch(FXSELID(sel)){
    case ID_MINI_SIZE: check=(getImageSize()==16); break;
    case ID_NORMAL_SIZE: check=(getImageSize()==32); break;
    case ID_MEDIUM_SIZE: check=(getImageSize()==64); break;
    case ID_GIANT_SIZE: check=(getImageSize()==128); break;
    }
  sender->handle(this,check?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


// Popup menu for item in file list
long FXFileSelector::onPopupMenu(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(!event->moved){
    FXMenuPane filemenu(this);
    new FXMenuCommand(&filemenu,tr("Up one level"),updiricon,this,ID_DIRECTORY_UP);
    new FXMenuCommand(&filemenu,tr("Home directory"),homeicon,this,ID_HOME);
    new FXMenuCommand(&filemenu,tr("Work directory"),workicon,this,ID_WORK);
    new FXMenuSeparator(&filemenu);

    FXMenuPane bookmenu(this);
    FXMenuCascade* bookcasc=new FXMenuCascade(&filemenu,tr("Bookmarks"),bookmarkicon,&bookmenu);
    bookcasc->setTarget(this);
    bookcasc->setSelector(ID_BOOKMENU);
    new FXMenuCommand(&bookmenu,tr("Set bookmark"),bookaddicon,this,ID_BOOKMARK);
    new FXMenuCommand(&bookmenu,tr("&Unset bookmark"),bookdelicon,this,ID_UNBOOKMARK);
    new FXMenuCommand(&bookmenu,tr("Clear all bookmarks"),bookclricon,&bookmarks,FXRecentFiles::ID_CLEAR);
    FXMenuSeparator* sep1=new FXMenuSeparator(&bookmenu);
    sep1->setTarget(&bookmarks);
    sep1->setSelector(FXRecentFiles::ID_ANYFILES);
    new FXMenuCommand(&bookmenu,"&1",nullptr,&bookmarks,FXRecentFiles::ID_FILE_1);
    new FXMenuCommand(&bookmenu,"&2",nullptr,&bookmarks,FXRecentFiles::ID_FILE_2);
    new FXMenuCommand(&bookmenu,"&3",nullptr,&bookmarks,FXRecentFiles::ID_FILE_3);
    new FXMenuCommand(&bookmenu,"&4",nullptr,&bookmarks,FXRecentFiles::ID_FILE_4);
    new FXMenuCommand(&bookmenu,"&5",nullptr,&bookmarks,FXRecentFiles::ID_FILE_5);
    new FXMenuCommand(&bookmenu,"&6",nullptr,&bookmarks,FXRecentFiles::ID_FILE_6);
    new FXMenuCommand(&bookmenu,"&7",nullptr,&bookmarks,FXRecentFiles::ID_FILE_7);
    new FXMenuCommand(&bookmenu,"&8",nullptr,&bookmarks,FXRecentFiles::ID_FILE_8);
    new FXMenuCommand(&bookmenu,"&9",nullptr,&bookmarks,FXRecentFiles::ID_FILE_9);
    new FXMenuCommand(&bookmenu,"1&0",nullptr,&bookmarks,FXRecentFiles::ID_FILE_10);
    new FXMenuCommand(&bookmenu,"11",nullptr,&bookmarks,FXRecentFiles::ID_FILE_11);
    new FXMenuCommand(&bookmenu,"12",nullptr,&bookmarks,FXRecentFiles::ID_FILE_12);
    new FXMenuCommand(&bookmenu,"13",nullptr,&bookmarks,FXRecentFiles::ID_FILE_13);
    new FXMenuCommand(&bookmenu,"14",nullptr,&bookmarks,FXRecentFiles::ID_FILE_14);
    new FXMenuCommand(&bookmenu,"15",nullptr,&bookmarks,FXRecentFiles::ID_FILE_15);
    new FXMenuCommand(&bookmenu,"16",nullptr,&bookmarks,FXRecentFiles::ID_FILE_16);
    new FXMenuCommand(&bookmenu,"17",nullptr,&bookmarks,FXRecentFiles::ID_FILE_17);
    new FXMenuCommand(&bookmenu,"18",nullptr,&bookmarks,FXRecentFiles::ID_FILE_18);
    new FXMenuCommand(&bookmenu,"19",nullptr,&bookmarks,FXRecentFiles::ID_FILE_19);
    new FXMenuCommand(&bookmenu,"20",nullptr,&bookmarks,FXRecentFiles::ID_FILE_20);

    FXMenuPane sortmenu(this);
    new FXMenuCascade(&filemenu,tr("Sort by"),sortingicon,&sortmenu);
    new FXMenuRadio(&sortmenu,tr("Name"),filebox,FXFileList::ID_SORT_BY_NAME);
    new FXMenuRadio(&sortmenu,tr("Type"),filebox,FXFileList::ID_SORT_BY_TYPE);
    new FXMenuRadio(&sortmenu,tr("Size"),filebox,FXFileList::ID_SORT_BY_SIZE);
    new FXMenuRadio(&sortmenu,tr("Time"),filebox,FXFileList::ID_SORT_BY_TIME);
    new FXMenuRadio(&sortmenu,tr("User"),filebox,FXFileList::ID_SORT_BY_USER);
    new FXMenuRadio(&sortmenu,tr("Group"),filebox,FXFileList::ID_SORT_BY_GROUP);
    new FXMenuSeparator(&sortmenu);
    new FXMenuCheck(&sortmenu,tr("Reverse"),filebox,FXFileList::ID_SORT_REVERSE);
    new FXMenuCheck(&sortmenu,tr("Ignore case"),filebox,FXFileList::ID_SORT_CASE);

    FXMenuPane viewmenu(this);
    new FXMenuCascade(&filemenu,tr("View"),iconsicon,&viewmenu);
    new FXMenuRadio(&viewmenu,tr("Small icons"),filebox,FXFileList::ID_SHOW_MINI_ICONS);
    new FXMenuRadio(&viewmenu,tr("Big icons"),filebox,FXFileList::ID_SHOW_BIG_ICONS);
    new FXMenuRadio(&viewmenu,tr("Details"),filebox,FXFileList::ID_SHOW_DETAILS);
    new FXMenuSeparator(&viewmenu);
    new FXMenuRadio(&viewmenu,tr("Rows"),filebox,FXFileList::ID_ARRANGE_BY_ROWS);
    new FXMenuRadio(&viewmenu,tr("Columns"),filebox,FXFileList::ID_ARRANGE_BY_COLUMNS);
    new FXMenuSeparator(&viewmenu);
    new FXMenuCheck(&viewmenu,tr("Hidden files"),filebox,FXFileList::ID_TOGGLE_HIDDEN);
    new FXMenuCheck(&viewmenu,tr("Preview images"),filebox,FXFileList::ID_TOGGLE_IMAGES);
    new FXMenuSeparator(&viewmenu);
    new FXMenuRadio(&viewmenu,tr("Mini images"),this,ID_MINI_SIZE);
    new FXMenuRadio(&viewmenu,tr("Normal images"),this,ID_NORMAL_SIZE);
    new FXMenuRadio(&viewmenu,tr("Medium images"),this,ID_MEDIUM_SIZE);
    new FXMenuRadio(&viewmenu,tr("Giant images"),this,ID_GIANT_SIZE);

    new FXMenuSeparator(&filemenu);
    new FXMenuCommand(&filemenu,tr("New directory..."),newicon,this,ID_NEW);
    new FXMenuCommand(&filemenu,tr("Rename..."),renameicon,this,ID_RENAME);
    new FXMenuCommand(&filemenu,tr("Copy..."),copyicon,this,ID_COPY);
    new FXMenuCommand(&filemenu,tr("Move..."),moveicon,this,ID_MOVE);
    new FXMenuCommand(&filemenu,tr("Link..."),linkicon,this,ID_LINK);
    new FXMenuCommand(&filemenu,tr("Delete..."),deleteicon,this,ID_REMOVE);

    filemenu.create();
    filemenu.popup(nullptr,event->root_x,event->root_y);
    getApp()->runModalWhileShown(&filemenu);
    }
  return 1;
  }


// Given string of the form "GIF Format (*.gif)", return the pattern: "*.gif".
FXString FXFileSelector::patternFromText(const FXString& pattern){
  FXint beg,end;
  end=pattern.rfind(')');         // Search from the end so we can allow ( ) in the pattern name itself
  beg=pattern.rfind('(',end-1);
  if(0<=beg && beg<end) return pattern.mid(beg+1,end-beg-1);
  return pattern;
  }


// Return the first extension "ext1" found in the pattern if the
// pattern is of the form "*.ext1,*.ext2,..." or the empty string
// if the pattern contains other wildcard combinations.
FXString FXFileSelector::extensionFromPattern(const FXString& pattern){
  FXint beg=0,end=0,c;
  while(pattern[beg]!='\0'){

    // Looks like a wildcard followed by extension
    if(pattern[beg]=='*'){
      beg++;
      if(pattern[beg]=='.'){
        end=++beg;

        // Snarf extension, but bail if it contains a wildcard
        while((c=pattern[end])!='\0' && c!=',' && c!='|'){
          if(c=='*' || c=='?' || c=='[' || c==']' || c=='^' || c=='!') goto nxt;
          end++;
          }

        // OK, got normal extension
        return pattern.mid(beg,end-beg);
        }
      }

    // Skip to next alternative
nxt:while((c=pattern[beg])!='\0'){
      if(c=='|' || c==','){ beg++; break; }
      beg++;
      }
    }
  return FXString::null;
  }


// Change the pattern; change the filename to the suggested extension
// Logic changed: change filename extension only if in save-file (ANY) mode.
long FXFileSelector::onCmdFilter(FXObject*,FXSelector,void* ptr){
  FXString pat=patternFromText((FXchar*)ptr);   // List of patterns
  filebox->setPattern(pat,true);
  if(selectmode==SELECTFILE_ANY){
    FXString ext=extensionFromPattern(pat);       // Get extension from pattern
    if(!ext.empty()){
      FXString name=FXPath::stripExtension(filename->getText());
      if(!name.empty()) filename->setText(name+"."+ext);
      }
    }
  return 1;
  }


// Set directory
void FXFileSelector::setDirectory(const FXString& path){
  FXString abspath(FXPath::absolute(path));
  filebox->setDirectory(abspath,true);
  dirbox->setDirectory(filebox->getDirectory());
  if(selectmode!=SELECTFILE_ANY){
    filename->setText(FXString::null);
    }
  }


// Get directory
FXString FXFileSelector::getDirectory() const {
  return filebox->getDirectory();
  }


// Set file name
void FXFileSelector::setFilename(const FXString& path){
  FXString fullname(FXPath::absolute(path));
  FXString name(FXPath::name(fullname));
  filebox->setCurrentFile(fullname);
  dirbox->setDirectory(filebox->getDirectory());
  if(selectmode==SELECTFILE_MULTIPLE_ALL || selectmode==SELECTFILE_MULTIPLE){
    filename->setText(encodeFilename(name));
    }
  else{
    filename->setText(name);
    }
  }


// Get complete path + filename
FXString FXFileSelector::getFilename() const {
  FXString name=filename->getText();
  if(!name.empty()){
    if(selectmode==SELECTFILE_MULTIPLE_ALL || selectmode==SELECTFILE_MULTIPLE){
      return FXPath::absolute(getDirectory(),decodeFilename(name));
      }
    else{
      return FXPath::absolute(getDirectory(),name);
      }
    }
  return FXString::null;
  }


// Return empty-string terminated list of selected file names, or NULL
FXString* FXFileSelector::getFilenames() const {
  FXString names=filename->getText();
  FXString *filelist=nullptr;
  if(!names.empty()){
    if(selectmode==SELECTFILE_MULTIPLE || selectmode==SELECTFILE_MULTIPLE_ALL){
      FXint n=countFilenames(names);
      if(n){
        filelist=new FXString [n+1];
        for(FXint i=0; i<n; ++i){
          filelist[i]=FXPath::absolute(getDirectory(),decodeFilename(names,i));
          }
        filelist[n]=FXString::null;
        }
      }
    else{
      filelist=new FXString [2];
      filelist[0]=FXPath::absolute(getDirectory(),names);
      filelist[1]=FXString::null;
      }
    }
  return filelist;
  }


// Change patterns, each pattern separated by newline
void FXFileSelector::setPatternList(const FXString& patterns){
  filefilter->clearItems();
  filefilter->fillItems(patterns);
  if(!filefilter->getNumItems()) filefilter->appendItem(tr("All Files (*)"));
  filefilter->setNumVisible(FXMIN(filefilter->getNumItems(),12));
  setCurrentPattern(0);
  }


// Return list of patterns
FXString FXFileSelector::getPatternList() const {
  FXString pat;
  for(FXint i=0; i<filefilter->getNumItems(); i++){
    if(!pat.empty()) pat+='\n';
    pat+=filefilter->getItemText(i);
    }
  return pat;
  }


// Set current filter pattern
void FXFileSelector::setPattern(const FXString& ptrn){
  filefilter->setText(ptrn);
  filebox->setPattern(ptrn);
  }


// Get current filter pattern
FXString FXFileSelector::getPattern() const {
  return filebox->getPattern();
  }


// Set current file pattern from the list
void FXFileSelector::setCurrentPattern(FXint patno){
  if(patno<0 || patno>=filefilter->getNumItems()){ fxerror("%s::setCurrentPattern: index out of range.\n",getClassName()); }
  filefilter->setCurrentItem(patno);
  filebox->setPattern(patternFromText(filefilter->getItemText(patno)));
  }


// Return current pattern
FXint FXFileSelector::getCurrentPattern() const {
  return filefilter->getCurrentItem();
  }


// Change pattern for pattern number patno
void FXFileSelector::setPatternText(FXint patno,const FXString& text){
  if(patno<0 || patno>=filefilter->getNumItems()){ fxerror("%s::setPatternText: index out of range.\n",getClassName()); }
  filefilter->setItemText(patno,text);
  if(patno==filefilter->getCurrentItem()){
    setPattern(patternFromText(text));
    }
  }


// Return pattern text of pattern patno
FXString FXFileSelector::getPatternText(FXint patno) const {
  if(patno<0 || patno>=filefilter->getNumItems()){ fxerror("%s::getPatternText: index out of range.\n",getClassName()); }
  return filefilter->getItemText(patno);
  }


// Return number of patterns
FXint FXFileSelector::getNumPatterns() const {
  return filefilter->getNumItems();
  }


// Allow pattern entry
void FXFileSelector::allowPatternEntry(FXbool flag){
  filefilter->setComboStyle(flag?COMBOBOX_NORMAL:COMBOBOX_STATIC);
  }


// Return true if pattern entry is allowed
FXbool FXFileSelector::allowPatternEntry() const {
  return (filefilter->getComboStyle()!=COMBOBOX_STATIC);
  }


// Change space for item
void FXFileSelector::setItemSpace(FXint s){
  filebox->setItemSpace(s);
  }


// Get space for item
FXint FXFileSelector::getItemSpace() const {
  return filebox->getItemSpace();
  }


// Change File List style
void FXFileSelector::setFileBoxStyle(FXuint style){
  filebox->setListStyle((filebox->getListStyle()&~FILESTYLEMASK) | (style&FILESTYLEMASK));
  }


// Return File List style
FXuint FXFileSelector::getFileBoxStyle() const {
  return filebox->getListStyle()&FILESTYLEMASK;
  }


// Change file selection mode
void FXFileSelector::setSelectMode(FXuint mode){
  switch(mode){
    case SELECTFILE_EXISTING:
      filebox->showOnlyDirectories(false);
      filebox->setListStyle((filebox->getListStyle()&~FILELISTMASK)|ICONLIST_BROWSESELECT);
      break;
    case SELECTFILE_MULTIPLE:
    case SELECTFILE_MULTIPLE_ALL:
      filebox->showOnlyDirectories(false);
      filebox->setListStyle((filebox->getListStyle()&~FILELISTMASK)|ICONLIST_EXTENDEDSELECT);
      break;
    case SELECTFILE_DIRECTORY:
      filebox->showOnlyDirectories(true);
      filebox->setListStyle((filebox->getListStyle()&~FILELISTMASK)|ICONLIST_BROWSESELECT);
      break;
    default:
      filebox->showOnlyDirectories(false);
      filebox->setListStyle((filebox->getListStyle()&~FILELISTMASK)|ICONLIST_SINGLESELECT);
      break;
    }
  selectmode=mode;
  }


// Change wildcard matching mode
void FXFileSelector::setMatchMode(FXuint mode){
  filebox->setMatchMode(mode);
  }


// Return wildcard matching mode
FXuint FXFileSelector::getMatchMode() const {
  return filebox->getMatchMode();
  }


// Return true if showing hidden files
FXbool FXFileSelector::showHiddenFiles() const {
  return filebox->showHiddenFiles();
  }


// Show or hide hidden files
void FXFileSelector::showHiddenFiles(FXbool flag){
  filebox->showHiddenFiles(flag);
  }


// Return true if image preview on
FXbool FXFileSelector::showImages() const {
  return filebox->showImages();
  }


// Show or hide preview images
void FXFileSelector::showImages(FXbool flag){
  filebox->showImages(flag);
  }


// Return images preview size
FXint FXFileSelector::getImageSize() const {
  return filebox->getImageSize();
  }


// Change images preview size
void FXFileSelector::setImageSize(FXint size){
  filebox->setImageSize(size);
  }


// Show readonly button
void FXFileSelector::showReadOnly(FXbool flag){
  flag ? readonly->show() : readonly->hide();
  }


// Return true if readonly is shown
FXbool FXFileSelector::shownReadOnly() const {
  return readonly->shown();
  }



// Set initial state of readonly button
void FXFileSelector::setReadOnly(FXbool flag){
  readonly->setCheck(flag);
  }


// Get readonly state
FXbool FXFileSelector::getReadOnly() const {
  return readonly->getCheck()==true;
  }


// Allow or disallow navigation
void FXFileSelector::allowNavigation(FXbool flag){
  filebox->showParents(flag,true);
  navigable=flag;
  }


// Set draggable files
void FXFileSelector::setDraggableFiles(FXbool flag){
  filebox->setDraggableFiles(flag);
  }


// Are files draggable?
FXbool FXFileSelector::getDraggableFiles() const {
  return filebox->getDraggableFiles();
  }


// Set file time format
void FXFileSelector::setTimeFormat(const FXString& fmt){
  filebox->setTimeFormat(fmt);
  }


// Return file time format
FXString FXFileSelector::getTimeFormat() const {
  return filebox->getTimeFormat();
  }


// Change file associations.
// Share associations between filebox and dirbox widgets.
// Optionally, share associations between other parts of the application as well.
void FXFileSelector::setAssociations(FXFileAssociations* assoc,FXbool owned){
  filebox->setAssociations(assoc,owned);
  dirbox->setAssociations(assoc,false);
  }


// Return file associations
FXFileAssociations* FXFileSelector::getAssociations() const {
  return filebox->getAssociations();
  }


// Change icon loader
void FXFileSelector::setIconSource(FXIconSource* src){
  filebox->setIconSource(src);
  }


// Return icon loader
FXIconSource* FXFileSelector::getIconSource() const {
  return filebox->getIconSource();
  }


// Save data
void FXFileSelector::save(FXStream& store) const {
  FXPacker::save(store);
  store << filebox;
  store << filename;
  store << filefilter;
  store << bookmarkmenu;
  store << navbuttons;
  store << fileboxframe;
  store << entryblock;
  store << readonly;
  store << dirbox;
  store << accept;
  store << cancel;
  store << updiricon;
  store << listicon;
  store << detailicon;
  store << iconsicon;
  store << homeicon;
  store << workicon;
  store << shownicon;
  store << hiddenicon;
  store << bookmarkicon;
  store << bookaddicon;
  store << bookdelicon;
  store << bookclricon;
  store << sortingicon;
  store << newicon;
  store << renameicon;
  store << copyicon;
  store << moveicon;
  store << linkicon;
  store << deleteicon;
  store << selectmode;
  store << navigable;
  }


// Load data
void FXFileSelector::load(FXStream& store){
  FXPacker::load(store);
  store >> filebox;
  store >> filename;
  store >> filefilter;
  store >> bookmarkmenu;
  store >> navbuttons;
  store >> fileboxframe;
  store >> entryblock;
  store >> readonly;
  store >> dirbox;
  store >> accept;
  store >> cancel;
  store >> updiricon;
  store >> listicon;
  store >> detailicon;
  store >> iconsicon;
  store >> homeicon;
  store >> workicon;
  store >> shownicon;
  store >> hiddenicon;
  store >> bookmarkicon;
  store >> bookaddicon;
  store >> bookdelicon;
  store >> bookclricon;
  store >> sortingicon;
  store >> newicon;
  store >> renameicon;
  store >> copyicon;
  store >> moveicon;
  store >> linkicon;
  store >> deleteicon;
  store >> selectmode;
  store >> navigable;
  }


// Cleanup; icons must be explicitly deleted
FXFileSelector::~FXFileSelector(){
  FXAccelTable *table=getShell()->getAccelTable();
  if(table){
    table->removeAccel(MKUINT(KEY_BackSpace,0));
    table->removeAccel(MKUINT(KEY_Delete,0));
    table->removeAccel(MKUINT(KEY_h,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_w,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_n,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_a,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_b,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_s,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_l,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_c,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_x,CONTROLMASK));
    table->removeAccel(MKUINT(KEY_v,CONTROLMASK));
    }
  delete bookmarkmenu;
  delete updiricon;
  delete listicon;
  delete detailicon;
  delete iconsicon;
  delete homeicon;
  delete workicon;
  delete shownicon;
  delete hiddenicon;
  delete bookmarkicon;
  delete bookaddicon;
  delete bookdelicon;
  delete bookclricon;
  delete sortingicon;
  delete newicon;
  delete renameicon;
  delete copyicon;
  delete moveicon;
  delete linkicon;
  delete deleteicon;
  filebox=(FXFileList*)-1L;
  filename=(FXTextField*)-1L;
  filefilter=(FXComboBox*)-1L;
  bookmarkmenu=(FXMenuPane*)-1L;
  navbuttons=(FXHorizontalFrame*)-1L;
  fileboxframe=(FXHorizontalFrame*)-1L;
  entryblock=(FXMatrix*)-1L;
  readonly=(FXCheckButton*)-1L;
  dirbox=(FXDirBox*)-1L;
  accept=(FXButton*)-1L;
  cancel=(FXButton*)-1L;
  updiricon=(FXIcon*)-1L;
  listicon=(FXIcon*)-1L;
  detailicon=(FXIcon*)-1L;
  iconsicon=(FXIcon*)-1L;
  homeicon=(FXIcon*)-1L;
  workicon=(FXIcon*)-1L;
  shownicon=(FXIcon*)-1L;
  hiddenicon=(FXIcon*)-1L;
  bookmarkicon=(FXIcon*)-1L;
  bookaddicon=(FXIcon*)-1L;
  bookdelicon=(FXIcon*)-1L;
  bookclricon=(FXIcon*)-1L;
  sortingicon=(FXIcon*)-1L;
  newicon=(FXIcon*)-1L;
  renameicon=(FXIcon*)-1L;
  copyicon=(FXIcon*)-1L;
  moveicon=(FXIcon*)-1L;
  linkicon=(FXIcon*)-1L;
  deleteicon=(FXIcon*)-1L;
  }

}

