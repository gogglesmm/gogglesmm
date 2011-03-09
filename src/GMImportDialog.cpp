/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2010 by Sander Jansen. All Rights Reserved      *
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
#include <tag.h>
#include "gmdefs.h"
#include <FXArray.h>
#include <fxkeys.h>
#include "icons.h"
#include "GMTrack.h"
#include "GMApp.h"
#include "GMIconTheme.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMImportDialog.h"


extern const FXchar gmfilepatterns[]="All Music (*.mp3,*.ogg,*.oga,*.flac,*.mpc"
#if defined(TAGLIB_WITH_MP4) && (TAGLIB_WITH_MP4==1)
",*.aac,*.m4a,*.m4p,*.mp4,*.m4b"
#endif
#if defined(TAGLIB_WITH_ASF) && (TAGLIB_WITH_ASF==1)
",*.wma,*.asf"
#endif
")\nFree Lossless Audio Codec (*.flac)\n"
"MPEG-1 Audio Layer 3 (*.mp3)\n"
#if defined(TAGLIB_WITH_MP4) && (TAGLIB_WITH_MP4==1)
"MPEG-4 Part 14 (*.mp4,*.m4a,*.m4p,*.aac,*.m4b)\n"
#endif
"Musepack (*.mpc)\n"
"Ogg Vorbis (*.ogg)\n"
"Ogg Audio (*.oga)\n"
#if defined(TAGLIB_WITH_ASF) && (TAGLIB_WITH_ASF==1)
"Window Media (*.wma,*,asf)\n"
#endif
"All Files (*.*)";


const FXchar playlist_patterns[]="All Playlists (*.xspf,*.pls,*.m3u)\nXML Shareable Playlist (*.xspf)\nPLS (*.pls)\nM3U (*.m3u)";


class GMDirSelector : public FXDirSelector {
FXDECLARE(GMDirSelector)
protected:
  FXFileDict * filedict;
  FXSettings * fileassoc;
protected:
  FXIconPtr icon_folder;
  FXIconPtr icon_folderopen;
protected:
  GMDirSelector(){}
private:
  GMDirSelector(const GMDirSelector&);
  GMDirSelector &operator=(const GMDirSelector&);
public:
  GMDirSelector(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  ~GMDirSelector();
  void initFileDict();
  };


FXIMPLEMENT(GMDirSelector,FXDirSelector,NULL,0);

GMDirSelector::GMDirSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDirSelector(p,tgt,sel,opts,x,y,w,h) {
  fileassoc=new FXSettings();
  filedict=new FXFileDict(getApp(),fileassoc);
  initFileDict();

  delete accept->getParent();
  delete dirbox->getParent();
  delete dirname->getParent();


//  FXHorizontalFrame *buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH);
//  accept=new FXButton(buttons,tr("&OK"),NULL,NULL,0,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
//  cancel=new FXButton(buttons,tr("&Cancel"),NULL,NULL,0,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  FXHorizontalFrame *field=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(field,tr("&Directory:"),NULL,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  dirname=new GMTextField(field,25,this,ID_DIRNAME,LAYOUT_FILL_X|LAYOUT_CENTER_Y|FRAME_SUNKEN|FRAME_THICK);

  GMScrollFrame *frame=new GMScrollFrame(this);
  dirbox=new FXDirList(frame,this,ID_DIRLIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|TREELIST_SHOWS_LINES|TREELIST_SHOWS_BOXES|TREELIST_BROWSESELECT);




  GMScrollArea::replaceScrollbars(dirbox);
  ((FXVerticalFrame*)dirbox->getParent())->setFrameStyle(FRAME_LINE);

#if FOXVERSION >= FXVERSION(1,7,11)
  dirbox->setAssociations(filedict,false);
#else
  FXFileDict * old = dirbox->getAssociations();
  dirbox->setAssociations(filedict);
  delete old;
#endif
/*
  getFirst()->hide();
  FXFrame * frame=(FXFrame*)getFirst()->getNext();
  frame->setPadLeft(0);
  frame->setPadRight(0);
  frame->setPadTop(0);
  frame->setPadBottom(0);
*/
  }

GMDirSelector::~GMDirSelector(){
  delete fileassoc;
#if FOXVERSION >= FXVERSION(1,7,11)
  delete filedict;
#endif
  }



class GMFileSelector : public FXFileSelector {
FXDECLARE(GMFileSelector)
protected:
  FXFileDict * filedict;
  FXSettings * fileassoc;
protected:
  FXIconPtr icon_file_small;
  FXIconPtr icon_file_big;
  FXIconPtr icon_audio_small;
  FXIconPtr icon_audio_big;
  FXIconPtr icon_folder_small;
  FXIconPtr icon_folder_big;
protected:
  GMFileSelector(){}
private:
  GMFileSelector(const GMFileSelector&);
  GMFileSelector &operator=(const GMFileSelector&);
public:
  GMFileSelector(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  ~GMFileSelector();

  void initFileDict();
  void hideButtons();
  void getSelectedFiles(FXStringList & files);
  
  FXMatrix * optionFrame() const { return entryblock; }  
  };

FXIMPLEMENT(GMFileSelector,FXFileSelector,NULL,0);

GMFileSelector::GMFileSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXFileSelector(p,tgt,sel,opts,x,y,w,h) {
  FXWindow * window=NULL;
  while((window=getFirst())!=NULL) delete window;
  delete bookmarkmenu; bookmarkmenu=NULL;

  FXAccelTable *table=getShell()->getAccelTable();

  navbuttons=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0, 0,0,0,0, 0,0);
  entryblock=new FXMatrix(this,3,MATRIX_BY_COLUMNS|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(entryblock,tr("&File Name:"),NULL,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  filename=new GMTextField(entryblock,25,this,ID_ACCEPT,TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y);
  new GMButton(entryblock,tr("&OK"),NULL,this,ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  accept=new FXButton(navbuttons,FXString::null,NULL,NULL,0,LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,0,0, 0,0,0,0);
  new FXLabel(entryblock,tr("File F&ilter:"),NULL,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  FXHorizontalFrame *filterframe=new FXHorizontalFrame(entryblock,LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);
  filefilter=new GMComboBox(filterframe,10,this,ID_FILEFILTER,COMBOBOX_STATIC|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y);
  filefilter->setNumVisible(4);
  readonly=new GMCheckButton(filterframe,tr("Read Only"),NULL,0,ICON_BEFORE_TEXT|JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  cancel=new GMButton(entryblock,tr("&Cancel"),NULL,NULL,0,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  fileboxframe=new GMScrollHFrame(this);
  filebox=new FXFileList(fileboxframe,this,ID_FILELIST,ICONLIST_MINI_ICONS|ICONLIST_BROWSESELECT|ICONLIST_AUTOSIZE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  GMScrollArea::replaceScrollbars(filebox);
  new FXLabel(navbuttons,tr("Directory:"),NULL,LAYOUT_CENTER_Y);
  dirbox=new FXDirBox(navbuttons,this,ID_DIRTREE,DIRBOX_NO_OWN_ASSOC|FRAME_LINE|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,1,1,1,1);
  dirbox->setNumVisible(5);
  dirbox->setAssociations(filebox->getAssociations());
  GMTreeListBox::replace(dirbox);

  bookmarkmenu=new GMMenuPane(this,POPUP_SHRINKWRAP);
  new GMMenuCommand(bookmarkmenu,tr("&Set bookmark\t\tBookmark current directory."),markicon,this,ID_BOOKMARK);
  new GMMenuCommand(bookmarkmenu,tr("&Clear bookmarks\t\tClear bookmarks."),clearicon,&bookmarks,FXRecentFiles::ID_CLEAR);
  FXMenuSeparator* sep1=new FXMenuSeparator(bookmarkmenu);
  sep1->setTarget(&bookmarks);
  sep1->setSelector(FXRecentFiles::ID_ANYFILES);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_1);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_2);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_3);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_4);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_5);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_6);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_7);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_8);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_9);
  new GMMenuCommand(bookmarkmenu,FXString::null,NULL,&bookmarks,FXRecentFiles::ID_FILE_10);

  new FXFrame(navbuttons,LAYOUT_FIX_WIDTH,0,0,4,1);
  new GMButton(navbuttons,tr("\tGo up one directory\tMove up to higher directory."),updiricon,this,ID_DIRECTORY_UP,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tGo to home directory\tBack to home directory."),homeicon,this,ID_HOME,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tGo to work directory\tBack to working directory."),workicon,this,ID_WORK,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  GMMenuButton *bookmenu=new GMMenuButton(navbuttons,tr("\tBookmarks\tVisit bookmarked directories."),markicon,bookmarkmenu,MENUBUTTON_NOARROWS|MENUBUTTON_ATTACH_LEFT|MENUBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  bookmenu->setTarget(this);
  bookmenu->setSelector(ID_BOOKMENU);
  new GMButton(navbuttons,tr("\tCreate new directory\tCreate new directory."),newicon,this,ID_NEW,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow list\tDisplay directory with small icons."),listicon,filebox,FXFileList::ID_SHOW_MINI_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow icons\tDisplay directory with big icons."),iconsicon,filebox,FXFileList::ID_SHOW_BIG_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow details\tDisplay detailed directory listing."),detailicon,filebox,FXFileList::ID_SHOW_DETAILS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMToggleButton(navbuttons,tr("\tShow hidden files\tShow hidden files and directories."),tr("\tHide Hidden Files\tHide hidden files and directories."),hiddenicon,shownicon,filebox,FXFileList::ID_TOGGLE_HIDDEN,TOGGLEBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  bookmarks.setTarget(this);
  bookmarks.setSelector(ID_VISIT);
  readonly->hide();
  if(table){
    table->addAccel(MKUINT(KEY_BackSpace,0),this,FXSEL(SEL_COMMAND,ID_DIRECTORY_UP));
    table->addAccel(MKUINT(KEY_Delete,0),this,FXSEL(SEL_COMMAND,ID_DELETE));
    table->addAccel(MKUINT(KEY_h,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_HOME));
    table->addAccel(MKUINT(KEY_w,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_WORK));
    table->addAccel(MKUINT(KEY_n,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_NEW));
    table->addAccel(MKUINT(KEY_a,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SELECT_ALL));
    table->addAccel(MKUINT(KEY_b,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_BIG_ICONS));
    table->addAccel(MKUINT(KEY_s,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_MINI_ICONS));
    table->addAccel(MKUINT(KEY_l,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_DETAILS));
    }
  setSelectMode(SELECTFILE_ANY);    // For backward compatibility, this HAS to be the default!
  //setPatternList(allfiles);
  setDirectory(FXSystem::getCurrentDirectory());
  filebox->setFocus();
  accept->hide();
  navigable=TRUE;




  fileassoc=new FXSettings();
  filedict=new FXFileDict(getApp(),fileassoc);
  initFileDict();
#if FOXVERSION >= FXVERSION(1,7,11)
  filebox->setAssociations(filedict,false);
  dirbox->setAssociations(filedict,false);
#else
  FXFileDict * old = filebox->getAssociations();
  filebox->setAssociations(filedict);
  dirbox->setAssociations(filedict);
  delete old;
#endif
/*
  entryblock->childAtIndex(2)->hide();
  cancel->hide();
  entryblock->setPadLeft(0);
  entryblock->setPadRight(0);
  entryblock->setPadTop(0);
  entryblock->setPadBottom(0);

  navbuttons->setPadLeft(0);
  navbuttons->setPadRight(0);
  navbuttons->setPadTop(0);
  navbuttons->setPadBottom(0);

*/
  }

GMFileSelector::~GMFileSelector(){
  delete fileassoc;
#if FOXVERSION >= FXVERSION(1,7,11)
  delete filedict;
#endif
  }


void GMFileSelector::hideButtons() {
  entryblock->childAtIndex(2)->hide();
  cancel->hide();
  }

void GMFileSelector::getSelectedFiles(FXStringList & files) {
  if(selectmode==SELECTFILE_MULTIPLE_ALL){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(filebox->isItemSelected(i) && filebox->getItemFilename(i)!=".." && filebox->getItemFilename(i)!="."){
        files.append(filebox->getItemPathname(i));
        }
      }
    }
  else {
    /// implement me
    FXASSERT(0);
    }
  }





static const FXchar * const filetypes[]={
  "mp3",";MPEG-1 Audio Layer 3",
  "ogg",";Ogg Vorbis",
  "oga",";Ogg Audio",
  "flac",";Free Lossless Audio Codec",
  "mpc",";Musepack",
#if defined(TAGLIB_WITH_ASF) && (TAGLIB_WITH_ASF==1)
  "wma",";Windows Media",
  "asf",";Windows Media",
#endif
#if defined(TAGLIB_WITH_MP4) && (TAGLIB_WITH_MP4==1)
  "m4a",";MPEG-4 Part 14",
  "m4b",";MPEG-4 Part 14",
  "m4p",";MPEG-4 Part 14",
  "aac",";MPEG-4 Part 14",
#endif
  "m3u",";M3U Playlist",
  "pls",";PLS Playlist",
  "xspf",";XML Shareable Playlist"
  };


void GMFileSelector::initFileDict() {
  FXFileAssoc * assoc=NULL;
  const FXColor backcolor = FXApp::instance()->getBackColor();

  GMIconTheme::instance()->loadSmall(icon_file_small,"text-x-generic",backcolor);
  GMIconTheme::instance()->loadMedium(icon_file_big,"text-x-generic",backcolor);
  GMIconTheme::instance()->loadSmall(icon_audio_small,"audio-x-generic",backcolor);
  GMIconTheme::instance()->loadMedium(icon_audio_big,"audio-x-generic",backcolor);
  GMIconTheme::instance()->loadSmall(icon_folder_small,"folder",backcolor);
  GMIconTheme::instance()->loadMedium(icon_folder_big,"folder",backcolor);

  for (FXuint i=0;i<ARRAYNUMBER(filetypes);i+=2) {
    assoc = filedict->replace(filetypes[i],filetypes[i+1]);
    assoc->bigicon  = icon_audio_big;
    assoc->miniicon = icon_audio_small;
    assoc = filedict->replace(FXString(filetypes[i]).upper().text(),filetypes[i+1]);
    assoc->bigicon  = icon_audio_big;
    assoc->miniicon = icon_audio_small;
    }

  assoc = filedict->replace(FXFileDict::defaultFileBinding,";");
  assoc->bigicon  = icon_file_big;
  assoc->miniicon = icon_file_small;

  assoc = filedict->replace(FXFileDict::defaultDirBinding,";");
  assoc->bigicon      = icon_folder_big;
  assoc->miniicon     = icon_folder_small;
  }

void GMDirSelector::initFileDict() {
  FXFileAssoc * assoc=NULL;
  const FXColor backcolor = FXApp::instance()->getBackColor();
  GMIconTheme::instance()->loadSmall(icon_folder,"folder",backcolor);
  GMIconTheme::instance()->loadSmall(icon_folderopen,"folder-open",backcolor);

  assoc = filedict->replace(FXFileDict::defaultDirBinding,";");
  assoc->miniiconopen = icon_folderopen;
  assoc->miniicon     = icon_folder;
  }











FXIMPLEMENT(GMFileDialog,FXFileDialog,NULL,0);

// Construct file fialog box
GMFileDialog::GMFileDialog(FXWindow* owner,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXFileDialog(owner,name,opts,x,y,w,h){
  delete filebox;
  filebox=new GMFileSelector(this,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->acceptButton()->setTarget(this);
  filebox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  filebox->cancelButton()->setTarget(this);
  filebox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }

// Construct free-floating file dialog box
GMFileDialog::GMFileDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXFileDialog(a,name,opts,x,y,w,h){
  delete filebox;
  filebox=new GMFileSelector(this,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->acceptButton()->setTarget(this);
  filebox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  filebox->cancelButton()->setTarget(this);
  filebox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }



FXIMPLEMENT(GMExportDialog,GMFileDialog,NULL,0);

// Construct file fialog box
GMExportDialog::GMExportDialog(FXWindow* owner,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  GMFileDialog(owner,name,opts,x,y,w,h){
  GMFileSelector * fileselector = dynamic_cast<GMFileSelector*>(filebox);
  FXMatrix * entryblock = fileselector->optionFrame();
  new FXLabel(entryblock,tr("&Options:"),NULL,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  check_relative=new GMCheckButton(entryblock,"Relative Paths",NULL,0,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);
  new FXFrame(entryblock,FRAME_NONE);
  }

// Construct free-floating file dialog box
GMExportDialog::GMExportDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  GMFileDialog(a,name,opts,x,y,w,h){
  GMFileSelector * fileselector = dynamic_cast<GMFileSelector*>(filebox);
  FXMatrix * entryblock = fileselector->optionFrame();
  new FXLabel(entryblock,tr("&Options:"),NULL,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  check_relative=new GMCheckButton(entryblock,"Relative Paths",NULL,0,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);
  new FXFrame(entryblock,FRAME_NONE);
  }



FXDEFMAP(GMImportDialog) GMImportDialogMap[]={
  FXMAPFUNC(SEL_UPDATE,FXDialogBox::ID_ACCEPT,GMImportDialog::onUpdAccept),
  FXMAPFUNCS(SEL_UPDATE,GMImportDialog::ID_SYNC_NEW,GMImportDialog::ID_SYNC_REMOVE_ALL,GMImportDialog::onUpdSync),
  FXMAPFUNCS(SEL_COMMAND,GMImportDialog::ID_SYNC_NEW,GMImportDialog::ID_SYNC_REMOVE_ALL,GMImportDialog::onCmdSync),
  FXMAPFUNC(SEL_COMMAND,GMImportDialog::ID_PARSE_METHOD,GMImportDialog::onCmdParseMethod)

  };

FXIMPLEMENT(GMImportDialog,FXDialogBox,GMImportDialogMap,ARRAYNUMBER(GMImportDialogMap))

GMImportDialog::GMImportDialog(FXWindow *p,FXuint m) : FXDialogBox(p,FXString::null,DECOR_BORDER|DECOR_TITLE|DECOR_RESIZE,0,0,500,400,0,0,0,0,0,0), mode(m) {


  if (mode&IMPORT_SYNC)
    setTitle(tr("Synchronize Folder"));
  else if (mode&IMPORT_PLAYLIST)
    setTitle(tr("Import Playlist"));
  else
    setTitle(tr("Import Music"));


  /// Create a fixed font, about the same size as the normal font
  FXint size = FXApp::instance()->getNormalFont()->getSize();
  font_fixed = new FXFont(FXApp::instance(),"mono",(int)size/10,FXFont::Normal,FXFont::Straight,FONTENCODING_UNICODE,FXFont::NonExpanded,FXFont::Modern|FXFont::Fixed);

  dirselector=NULL;
  fileselector=NULL;

  target_track_from_filelist.connect(GMPlayerManager::instance()->getPreferences().import.track_from_filelist);
  target_replace_underscores.connect(GMPlayerManager::instance()->getPreferences().import.replace_underscores);
  target_default_field.connect(GMPlayerManager::instance()->getPreferences().import.default_field);
  target_parse_method.connect(GMPlayerManager::instance()->getPreferences().import.parse_method,this,ID_PARSE_METHOD);
  target_filename_template.connect(GMPlayerManager::instance()->getPreferences().import.filename_template);

  target_exclude_dir.connect(GMPlayerManager::instance()->getPreferences().import.exclude_folder);
  target_exclude_file.connect(GMPlayerManager::instance()->getPreferences().import.exclude_file);

  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;
  const FXuint textfieldstyle=TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN;

  FXGroupBox * grpbox;
  FXMatrix   * matrix;
  FXVerticalFrame * vframe;
  FXHorizontalFrame * hframe;

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  GMTabBook * tabbook=new GMTabBook(main,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT,0,0,0,0,0,0,0,0);


  if (mode&IMPORT_FROMFILE) {
    new GMTabItem(tabbook,tr("&File(s)"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);
    fileselector = new GMFileSelector(vframe,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    FXString searchdir = getApp()->reg().readStringEntry("directories","last-import-files",NULL);
    if (searchdir.empty()) getDefaultSearchDirectory(searchdir);
    if (mode&IMPORT_PLAYLIST) {
      fileselector->setPatternList(playlist_patterns);
      fileselector->setSelectMode(SELECTFILE_EXISTING);
      }
    else {
      fileselector->setPatternList(gmfilepatterns);
      fileselector->setSelectMode(SELECTFILE_MULTIPLE);
      }
    fileselector->setCurrentPattern(0);
#if FOXVERSION < FXVERSION(1,7,20)
    fileselector->setMatchMode(FILEMATCH_CASEFOLD);
#else
    fileselector->setMatchMode(FXPath::CaseFold);
#endif
    fileselector->setDirectory(searchdir);
    fileselector->hideButtons();
    fileselector->acceptButton()->setTarget(this);
    fileselector->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
    fileselector->cancelButton()->setTarget(this);
    fileselector->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
    }
  else if (mode&IMPORT_FROMDIR) {
    new GMTabItem(tabbook,tr("&Directory"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);

    GMCheckButton * excludetoggle = new GMCheckButton(vframe,tr("Exclude Filter\tFilter out directories and/or files based on pattern"),NULL,0,CHECKBUTTON_NORMAL|CHECKBUTTON_PLUS);

    matrix = new FXMatrix(vframe,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,16,4,0,0);
    new FXLabel(matrix,tr("Folders:"),NULL,labelstyle);
    new GMTextField(matrix,20,&target_exclude_dir,FXDataTarget::ID_VALUE,textfieldstyle);
    new FXLabel(matrix,tr("Files:"),NULL,labelstyle);
    new GMTextField(matrix,20,&target_exclude_file,FXDataTarget::ID_VALUE,textfieldstyle);

    excludetoggle->setTarget(matrix);
    excludetoggle->setSelector(FXWindow::ID_TOGGLESHOWN);

    if (GMPlayerManager::instance()->getPreferences().import.exclude_folder.empty() && GMPlayerManager::instance()->getPreferences().import.exclude_file.empty())
      matrix->hide();

    dirselector = new GMDirSelector(vframe,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    FXString searchdir = getApp()->reg().readStringEntry("directories","last-import-dirs",NULL);
    if (searchdir.empty()) getDefaultSearchDirectory(searchdir);
    dirselector->setDirectory(searchdir);
    }

  if (mode&IMPORT_SYNC) {
    new GMTabItem(tabbook,tr("&Sync"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);
    grpbox =  new FXGroupBox(vframe,tr("Sync Operation"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
    grpbox->setFont(GMApp::instance()->getThickFont());

    new GMCheckButton(grpbox,tr("Import new tracks\tImports files not yet in the database."),this,ID_SYNC_NEW);
    new GMCheckButton(grpbox,tr("Remove tracks that have been deleted from disk"),this,ID_SYNC_REMOVE_MISSING);
    matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
    new GMCheckButton(matrix,tr("Update existing tracks:"),this,ID_SYNC_UPDATE);
    new FXRadioButton(matrix,tr("Modified since last import\tOnly reread the tag when the file has been modified."),this,ID_SYNC_UPDATE_MODIFIED,RADIOBUTTON_NORMAL|LAYOUT_CENTER_Y|LAYOUT_LEFT,0,0,0,0);
    new FXFrame(matrix,FRAME_NONE);
    new FXRadioButton(matrix,tr("All\tAlways read the tags"),this,ID_SYNC_UPDATE_ALL,RADIOBUTTON_NORMAL|LAYOUT_CENTER_Y|LAYOUT_LEFT,0,0,0,0);
    new GMCheckButton(grpbox,tr("Remove tracks found in folder from database"),this,ID_SYNC_REMOVE_ALL);
    }

  new GMTabItem(tabbook,tr("&Track"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
  vframe = new GMTabFrame(tabbook);

  grpbox = new FXGroupBox(vframe,tr("Parse Settings"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  grpbox->setFont(GMApp::instance()->getThickFont());

  matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X);
  new FXLabel(matrix,tr("Parse info from:"),NULL,labelstyle);

  hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
  new GMRadioButton(hframe,tr("Tag"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_TAG,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);
  new GMRadioButton(hframe,tr("Filename"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_FILENAME,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);
  new GMRadioButton(hframe,tr("Both"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_BOTH,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);

  new FXLabel(matrix,tr("Default value:"),NULL,labelstyle);
  new GMTextField(matrix,10,&target_default_field,FXDataTarget::ID_VALUE,textfieldstyle|LAYOUT_FILL_COLUMN);

  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,tr("Set track number based on scan order."),&target_track_from_filelist,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

  template_grpbox =  new FXGroupBox(vframe,tr("Filename Template"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
  template_grpbox->setFont(GMApp::instance()->getThickFont());

  FXLabel * label = new FXLabel(template_grpbox,tr("%T - title              %A - album name\n"
                                          "%P - album artist name  %p - track artist name \n"
                                          "%N - track number       %G - genre"),NULL,FRAME_LINE|JUSTIFY_LEFT,0,0,0,0,30);
  label->setFont(font_fixed);
  label->setBackColor(getApp()->getTipbackColor());
  label->setBorderColor(getApp()->getShadowColor());

  new FXSeparator(template_grpbox,SEPARATOR_GROOVE|LAYOUT_FILL_X);
  matrix = new FXMatrix(template_grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X);
  new FXLabel(matrix,tr("Template:"),NULL,labelstyle);
  new GMTextField(matrix,10,&target_filename_template,FXDataTarget::ID_VALUE,textfieldstyle);
  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,tr("Replace underscores with spaces"),&target_replace_underscores,FXDataTarget::ID_VALUE,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);

  if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_TAG) {
    template_grpbox->hide();
    }
  else {
    template_grpbox->show();
    }

  FXHorizontalFrame *closebox=new FXHorizontalFrame(main,LAYOUT_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  if (mode&IMPORT_SYNC)
    new GMButton(closebox,tr("&Sync"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  else
    new GMButton(closebox,tr("&Import"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new GMButton(closebox,tr("&Cancel"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  }

long GMImportDialog::onCmdAccept(FXObject*sender,FXSelector sel,void*ptr){
  return FXDialogBox::onCmdAccept(sender,sel,ptr);
  }

long GMImportDialog::onUpdAccept(FXObject*sender,FXSelector,void*){
  if (mode&IMPORT_SYNC) {
    if (GMPlayerManager::instance()->getPreferences().sync.import_new ||
        GMPlayerManager::instance()->getPreferences().sync.remove_missing ||
        GMPlayerManager::instance()->getPreferences().sync.update ||
        GMPlayerManager::instance()->getPreferences().sync.remove_all)
      sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
    }
  return 1;
  }

long GMImportDialog::onCmdSync(FXObject*,FXSelector sel,void*ptr){
  FXbool check=((FXint)(FXival)ptr)!=0;
  switch(FXSELID(sel)){
    case ID_SYNC_NEW            : GMPlayerManager::instance()->getPreferences().sync.import_new=check;
                                  break;
    case ID_SYNC_REMOVE_MISSING : GMPlayerManager::instance()->getPreferences().sync.remove_missing=check;
                                  break;
    case ID_SYNC_UPDATE         : GMPlayerManager::instance()->getPreferences().sync.update=check;
                                  break;
    case ID_SYNC_UPDATE_ALL     : GMPlayerManager::instance()->getPreferences().sync.update_always=check;
                                  break;
    case ID_SYNC_UPDATE_MODIFIED: GMPlayerManager::instance()->getPreferences().sync.update_always=!check;
                                  break;
    case ID_SYNC_REMOVE_ALL     : GMPlayerManager::instance()->getPreferences().sync.remove_all=check;
                                  break;
    }
  return 1;
  }


long GMImportDialog::onUpdSync(FXObject*sender,FXSelector sel,void*){
  FXbool enable=true;
  FXbool check=false;
  switch(FXSELID(sel)){
    case ID_SYNC_NEW            : check= (GMPlayerManager::instance()->getPreferences().sync.remove_all) ?  false : GMPlayerManager::instance()->getPreferences().sync.import_new;
                                  enable=!GMPlayerManager::instance()->getPreferences().sync.remove_all;
                                  break;
    case ID_SYNC_REMOVE_MISSING : check= (GMPlayerManager::instance()->getPreferences().sync.remove_all) ?  false : GMPlayerManager::instance()->getPreferences().sync.remove_missing;
                                  enable=!GMPlayerManager::instance()->getPreferences().sync.remove_all;
                                  break;
    case ID_SYNC_UPDATE         : check= (GMPlayerManager::instance()->getPreferences().sync.remove_all) ?  false : GMPlayerManager::instance()->getPreferences().sync.update;
                                  enable=!GMPlayerManager::instance()->getPreferences().sync.remove_all;
                                  break;
    case ID_SYNC_UPDATE_ALL     : check= (GMPlayerManager::instance()->getPreferences().sync.remove_all) ?  false :  GMPlayerManager::instance()->getPreferences().sync.update_always;
                                  enable=(!GMPlayerManager::instance()->getPreferences().sync.remove_all && GMPlayerManager::instance()->getPreferences().sync.update);
                                  break;
    case ID_SYNC_UPDATE_MODIFIED: check= (GMPlayerManager::instance()->getPreferences().sync.remove_all) ?  false : !GMPlayerManager::instance()->getPreferences().sync.update_always;
                                  enable=(!GMPlayerManager::instance()->getPreferences().sync.remove_all && GMPlayerManager::instance()->getPreferences().sync.update);
                                  break;
    case ID_SYNC_REMOVE_ALL     : check=GMPlayerManager::instance()->getPreferences().sync.remove_all;
                                  break;
    }
  if (enable)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);

  if (check)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


void GMImportDialog::getSelectedFiles(FXStringList & files){
  if (dirselector) {
    getApp()->reg().writeStringEntry("directories","last-import-dirs",dirselector->getDirectory().text());
    files.append(dirselector->getDirectory());
    }
  else if (fileselector){
    getApp()->reg().writeStringEntry("directories","last-import-files",fileselector->getDirectory().text());
    fileselector->getSelectedFiles(files);
    }
  }

FXString GMImportDialog::getFilename() const{
  getApp()->reg().writeStringEntry("directories","last-import-files",fileselector->getDirectory().text());
  return fileselector->getFilename();
  }

long GMImportDialog::onCmdParseMethod(FXObject*,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_TAG) {
    template_grpbox->hide();
    }
  else {
    template_grpbox->show();
    }
  template_grpbox->getParent()->recalc();
  template_grpbox->getParent()->layout();
  return 1;
  }



void GMImportDialog::getDefaultSearchDirectory(FXString & directory){
  const FXchar * musicdir[]={
    "Music",
    "music",
    "mp3",
    "MP3",
    "Mp3",
    "ogg",
    "OGG",
    "Ogg",
#if defined(TAGLIB_WITH_MP4) && (TAGLIB_WITH_MP4==1)
    "mp4",
    "MP4",
    "Mp4",
#endif
    NULL
    };
  FXString test;
  FXString home=FXSystem::getHomeDirectory();
  for (int i=0;musicdir[i]!=NULL;i++){
    test = home + PATHSEPSTRING + musicdir[i];
    if (FXStat::exists(test) && FXStat::isDirectory(test)){
      directory=test;
      break;
      }
    }
  if (directory.empty()) directory=home;
  }


