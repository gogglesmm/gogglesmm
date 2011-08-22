#include "gmdefs.h"
#include "gmutils.h"
#include "GMCover.h"
#include "GMTrack.h"
#include "GMTag.h"
#include "GMList.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackItem.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMSourceView.h"
#include "GMClipboard.h"
#include "GMDatabaseSource.h"
#include "GMPlayListSource.h"
#include "GMPlayQueue.h"

#include "GMPlayerManager.h"
#include "GMFilename.h"
#include "GMIconTheme.h"
#include "GMTaskManager.h"
#include "GMTrackEditor.h"
#include "GMWindow.h"


#include "FXPNGIcon.h"
#include "icons.h"

class GMTagUpdateTask : public GMTask {
protected:
  GMTrackDatabase * database;
  FXIntList         tracks;
public:
  FXint run();
public:
  GMTagUpdateTask(GMTrackDatabase * db,const FXIntList & t);
  };

GMTagUpdateTask::GMTagUpdateTask(GMTrackDatabase * db,const FXIntList & t) : database(db),tracks(t){
  }

FXint GMTagUpdateTask::run() {
  try {
    GMTrack info;

    for (FXint i=0;i<tracks.no() && processing;i++) {
      if (!database->getTrack(tracks[i],info)) break;
      taskmanager->setStatus(FXString::value("Writng Tags %d/%d..",i+1,tracks.no()));

      info.saveTag(info.mrl);
      database->beginTask();
      database->setTrackImported(tracks[i],FXThread::time());
      database->commitTask();
      }
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }





class GMUpdateTask : public GMTask {
protected:
  GMTrackDatabase * database;
  GMTrackArray      tracks;
  FXIntList         ids;
public:
  FXint run();
public:
  GMUpdateTask(GMTrackDatabase * db,GMTrackArray & t,FXIntList & i);
  };

GMUpdateTask::GMUpdateTask(GMTrackDatabase * db,GMTrackArray & t,FXIntList & i) : database(db) {
  tracks.adopt(t);
  ids.adopt(i);
  }

FXint GMUpdateTask::run() {
  try {
    for (FXint i=0;i<tracks.no() && processing;i++) {
      taskmanager->setStatus(FXString::value("Writing Tags %d/%d..",i+1,tracks.no()));
      tracks[i].saveTag(tracks[i].mrl);

      database->beginTask();
      database->setTrackImported(ids[i],FXThread::time());
      database->commitTask();
      }
    }
  catch(GMDatabaseException&) {
    database->rollbackTask();
    return 1;
    }
  return 0;
  }













class GMRenameTask : public GMTask {
protected:
  GMTrackDatabase * database;
  FXIntList    tracks;
  FXStringList to;
  FXStringList from;
public:
  FXint run();
public:
  GMRenameTask(GMTrackDatabase * db,const FXIntList & t,const FXStringList & n,const FXStringList & o);
  };

GMRenameTask::GMRenameTask(GMTrackDatabase * db,const FXIntList & t,const FXStringList & n,const FXStringList & o) : database(db),tracks(t),to(n),from(o) {
  }

FXint GMRenameTask::run() {
  try {
    for (FXint i=0;i<from.no() && processing;i++) {
      if (to[i].empty()) continue;
      if (!FXDir::createDirectories(FXPath::directory(to[i]))) continue;
      if (FXStat::exists(to[i])) continue;
      fxmessage("Updating Filename %s\n",from[i].text());
      if (FXFile::rename(from[i],to[i])){
        database->setTrackFilename(tracks[i],to[i]);
        }
      }
    }
  catch(GMDatabaseException&) {
    fxmessage("Database Error\n");
    }
  return 0;
  }





static FXbool updateTrackFilenames(GMTrackDatabase * db,FXIntList & tracks) {
  register FXint i=0;
  FXint numchanges=0;
  FXString mrl;
  GMTrack trackinfo;
  FXStringList newmrls;
  FXStringList oldmrls;

  if (!GMPlayerManager::instance()->getPreferences().export_format_template.contains("%T")) {
    FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Invalid Template"),fxtr("The provided template is invalid. The track title %%T needs to be specified.\nPlease fix the filename template in the preference panel."));
    return false;
    }

  FXTextCodec * codec = GMFilename::findcodec(GMPlayerManager::instance()->getPreferences().export_encoding);
  FXuint options=0;

  if (GMPlayerManager::instance()->getPreferences().export_lowercase)
    options|=GMFilename::LOWERCASE;

  if (GMPlayerManager::instance()->getPreferences().export_lowercase_extension)
    options|=GMFilename::LOWERCASE_EXTENSION;

  if (GMPlayerManager::instance()->getPreferences().export_underscore)
    options|=GMFilename::NOSPACES;

  /// Create New Mrls.
  for (i=0;i<tracks.no();i++) {
    if (!db->getTrack(tracks[i],trackinfo)) {
      FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Database Error"),fxtr("Oops. Database Error"));
      return true;
      }
    if (GMFilename::create(mrl,trackinfo,GMPlayerManager::instance()->getPreferences().export_format_template,GMPlayerManager::instance()->getPreferences().export_character_filter,options,codec) && mrl!=trackinfo.mrl) {
      newmrls.append(mrl);
      oldmrls.append(trackinfo.mrl);
      numchanges++;
      }
    else {
      newmrls.append(FXString::null);
      oldmrls.append(FXString::null);
      }
    }

  if (numchanges==0){
    FXMessageBox::information(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("No changes"),fxtr("Filenames did not require any changes"));
    delete codec;
    return false;
    }


  /// Ask For Permission
  FXDialogBox dialog(GMPlayerManager::instance()->getMainWindow(),fxtr("Rename Audio Files?"),DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE|DECOR_CLOSE,0,0,600,400,0,0,0,0,0,0);
  GMPlayerManager::instance()->getMainWindow()->create_dialog_header(&dialog,fxtr("Renaming Audio Files…"),fxtr("The following audio files are going to be renamed"));

  FXHorizontalFrame *closebox=new FXHorizontalFrame(&dialog,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  new GMButton(closebox,fxtr("&Rename"),NULL,&dialog,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox,fxtr("&Cancel"),NULL,&dialog,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);

  FXVerticalFrame * main = new FXVerticalFrame(&dialog,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  GMScrollFrame * sunken = new GMScrollFrame(main);
  GMList * list = new GMList(sunken,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  for (i=0;i<tracks.no();i++) {
    if (!newmrls[i].empty()) {
      list->appendItem(newmrls[i]);
      }
    }





  if (dialog.execute()) {

    GMRenameTask * task = new GMRenameTask(db,tracks,newmrls,oldmrls);
//    task->setTarget(GMPlayerManager::instance()->getTrackView()->getSource());
 //   task->setSelector(

    GMPlayerManager::instance()->runTask(task);
/*

    for (i=0;i<tracks.no();i++){
      if (!newmrls[i].empty()) {
        if (gm_make_path(FXPath::directory(newmrls[i]))) {
          if (!FXStat::exists(newmrls[i])) {
            if (FXFile::rename(oldmrls[i],newmrls[i])){
              db->setTrackFilename(tracks[i],newmrls[i]);
              }
            else {
              if (i+1<tracks.no()) {
                if (FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_YES_NO,fxtr("Unable to rename file"),fxtrformat("Unable to rename:\n%s\n\nto:%s\nContinue renaming files?"),oldmrls[i].text(),newmrls[i].text())==MBOX_CLICKED_NO)
                  break;
                }
              else {
                FXMessageBox::error(GMPlayerManager::instance()->getMainWindow(),MBOX_OK,fxtr("Unable to rename file"),fxtrformat("Unable to rename:\n%s\n\nto:%s"),oldmrls[i].text(),newmrls[i].text());
                }
              }
            }
          }
        }
      }
*/
    return true;
    }
  delete codec;
  return false;
  }


class GMFilenameTemplateDialog : public FXDialogBox {
FXDECLARE(GMFilenameTemplateDialog)
protected:
  FXFontPtr    font_fixed;
  FXDataTarget target_format_template;
  FXDataTarget target_export_lowercase;
  FXDataTarget target_export_lowercase_extension;
  FXDataTarget target_export_underscore;
  FXDataTarget target_export_encoding;
  FXDataTarget target_export_filter;
protected:
  GMFilenameTemplateDialog(){}
private:
  GMFilenameTemplateDialog(const GMFilenameTemplateDialog&);
  GMFilenameTemplateDialog &operator=(const GMFilenameTemplateDialog&);
public:
  GMFilenameTemplateDialog(FXWindow*);
  ~GMFilenameTemplateDialog();
  };

FXIMPLEMENT(GMFilenameTemplateDialog,FXDialogBox,0,0);

GMFilenameTemplateDialog::GMFilenameTemplateDialog(FXWindow*p) : FXDialogBox(p,FXString::null,DECOR_TITLE|DECOR_BORDER,0,0,0,0,0,0,0,0,0,0) {
  setTitle(tr("Filename Template"));

  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;

  target_format_template.connect(GMPlayerManager::instance()->getPreferences().export_format_template);
  target_export_lowercase.connect(GMPlayerManager::instance()->getPreferences().export_lowercase);
  target_export_lowercase_extension.connect(GMPlayerManager::instance()->getPreferences().export_lowercase_extension);
  target_export_underscore.connect(GMPlayerManager::instance()->getPreferences().export_underscore);
  target_export_encoding.connect(GMPlayerManager::instance()->getPreferences().export_encoding);
  target_export_filter.connect(GMPlayerManager::instance()->getPreferences().export_character_filter);

  /// Create a fixed font, about the same size as the normal font
  FXint size = FXApp::instance()->getNormalFont()->getSize();
  font_fixed = new FXFont(FXApp::instance(),"mono",(int)size/10,FXFont::Normal,FXFont::Straight,FONTENCODING_UNICODE,FXFont::NonExpanded,FXFont::Modern|FXFont::Fixed);

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  new FXLabel(main,tr("Template may contain absolute or relative path, environment variables\nand ~. Relative paths are based on the location of the original file. The\nfile extension gets automatically added. The following macros\nmay be used:"),NULL,JUSTIFY_LEFT);
  FXLabel * label = new FXLabel(main,tr("%T - title                   %A - album name\n"
                                        "%P - album artist name       %p - track artist name\n"
                                        "%w - composer                %c - conductor\n"
                                        "%y - year                    %d - disc number\n"
                                        "%N - track number (2 digits) %n - track number      \n%G - genre"),NULL,JUSTIFY_LEFT,0,0,0,0,30);
  label->setFont(font_fixed);
  new FXSeparator(main,SEPARATOR_GROOVE|LAYOUT_FILL_X);

  FXMatrix * matrix = new FXMatrix(main,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,0,0,4,0);
  new FXLabel(matrix,tr("Template:"),NULL,labelstyle);
  GMTextField * textfield = new GMTextField(matrix,20,&target_format_template,FXDataTarget::ID_VALUE,LAYOUT_FILL_X|TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
  textfield->setFont(font_fixed);

  new FXLabel(matrix,tr("Encoding:"),NULL,labelstyle);
  GMListBox * list_codecs = new GMListBox(matrix,&target_export_encoding,FXDataTarget::ID_VALUE,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
  for (int i=0;gmcodecnames[i]!=NULL;i++)
    list_codecs->appendItem(gmcodecnames[i]);
  list_codecs->setNumVisible(9);

  new FXLabel(matrix,tr("Exclude:"),NULL,labelstyle);
  textfield = new GMTextField(matrix,15,&target_export_filter,FXDataTarget::ID_VALUE,LAYOUT_FILL_X|TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
  textfield->setFont(font_fixed);

  new FXLabel(matrix,tr("Options:"),NULL,labelstyle);
  new GMCheckButton(matrix,tr("Replace spaces with underscores"),&target_export_underscore,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,fxtr("Lower case"),&target_export_lowercase,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXFrame(matrix,FRAME_NONE);
  new GMCheckButton(matrix,fxtr("Lower case extension"),&target_export_lowercase_extension,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXSeparator(main,SEPARATOR_GROOVE|LAYOUT_FILL_X);

  FXHorizontalFrame *closebox=new FXHorizontalFrame(main,LAYOUT_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new GMButton(closebox,fxtr("&Close"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  }

GMFilenameTemplateDialog::~GMFilenameTemplateDialog(){
  GMPlayerManager::instance()->getPreferences().export_format_template.trim();
  }



static const FXchar defaults_section[] = "dialog defaults";

FXDEFMAP(GMEditTrackDialog) GMEditTrackDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,GMEditTrackDialog::ID_FILENAME_TEMPLATE,GMEditTrackDialog::onCmdFilenameTemplate),
  FXMAPFUNC(SEL_COMMAND,GMEditTrackDialog::ID_ACCEPT,GMEditTrackDialog::onCmdAccept),
  FXMAPFUNC(SEL_COMMAND,GMEditTrackDialog::ID_RESET,GMEditTrackDialog::onCmdResetTrack),
  FXMAPFUNCS(SEL_COMMAND,GMEditTrackDialog::ID_NEXT_TRACK,GMEditTrackDialog::ID_PREV_TRACK,GMEditTrackDialog::onCmdSwitchTrack)
  };

FXIMPLEMENT(GMEditTrackDialog,FXDialogBox,GMEditTrackDialogMap,ARRAYNUMBER(GMEditTrackDialogMap));


static const FXchar update_tags_key[] = "track-update-tags";
static const FXchar update_filenames_key[] = "track-update-filenames";

GMEditTrackDialog::GMEditTrackDialog(FXWindow*p,GMTrackDatabase * d) : FXDialogBox(p,FXString::null,DECOR_TITLE|DECOR_BORDER|DECOR_RESIZE,0,0,0,0,0,0,0,0,0,0), db(d) {
  FXPacker * main=NULL;
  GMTextField * textfield = NULL;
  FXHorizontalFrame * hframe = NULL;
  GMTabBook * tabbook = NULL;
  FXMatrix * matrix = NULL;
  GMTabFrame * tabframe = NULL;

  titlefield=NULL;
  discfield=NULL;
  discspinner=NULL;
  art=NULL;

  GMTrack other;

  getTrackSelection();

  setTitle(tr("Edit Track Information"));
  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0);
  if (tracks.no()==1) { /* only show spinner when one track is selected */
    new GMButton(closebox,tr("&Previous"),NULL,this,ID_PREV_TRACK,BUTTON_DEFAULT|LAYOUT_LEFT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,tr("&Next"),NULL,this,ID_NEXT_TRACK,BUTTON_INITIAL|LAYOUT_LEFT|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,tr("&Close"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,tr("&Reset"),NULL,this,ID_RESET,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    }
  else {
    new GMButton(closebox,tr("&Save"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|LAYOUT_RIGHT|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    new GMButton(closebox,tr("&Close"),NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
    }

  if (tracks.no()==1) { /* only show spinner when one track is selected */
    main = new FXPacker(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
    }
  else {
    new FXSeparator(this,SEPARATOR_GROOVE|LAYOUT_FILL_X|LAYOUT_SIDE_BOTTOM);
    main = new FXPacker(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,5,5,5,5);
    }

  if (tracks.no()==1) { /* only show spinner when one track is selected */

    tabbook = new GMTabBook(main,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);

    new GMTabItem(tabbook,tr("&Tag"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
    tabframe = new GMTabFrame(tabbook);

    FXMatrix * tagmatrix = new FXMatrix(tabframe,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS,0,0,0,0,10,10,10,10);

    new GMTabItem(tabbook,tr("&Properties"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
    tabframe = new GMTabFrame(tabbook);

    matrix = new FXMatrix(tabframe,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS,0,0,0,0,10,10,10,10);

    new FXLabel(matrix,tr("Filename"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
    textfield->setText(info.mrl);

    new FXLabel(matrix,tr("Type"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
    textfield->setText(FXPath::extension(info.mrl).upper());

    new FXLabel(matrix,tr("Size"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
#if defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64)
      textfield->setText(FXString::value("%'ld",FXStat::size(info.mrl)));
#else
      textfield->setText(FXString::value("%'lld",FXStat::size(info.mrl)));
#endif

    new FXLabel(matrix,tr("Bitrate"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
    textfield->setText(FXString::value("%dkbs",properties.bitrate));

    new FXLabel(matrix,tr("Samplerate"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
    textfield->setText(FXString::value("%dHz",properties.samplerate));

    new FXLabel(matrix,tr("Channels"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    textfield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK|TEXTFIELD_READONLY);
    textfield->setText(FXString::value("%d",properties.channels));

    if (tracks.no()==1){
      covertab = new GMTabItem(tabbook,tr("Co&ver"),NULL,TAB_TOP_NORMAL,0,0,0,0,5,5);
      tabframe = new GMTabFrame(tabbook);

      GMScrollFrame * scrollframe = new GMScrollFrame(tabframe);
      coverview = new FXImageView(scrollframe,NULL,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
      GMScrollArea::replaceScrollbars(coverview);
      }

    matrix = tagmatrix;

    new FXLabel(matrix,tr("Trac&k"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);

    trackspinner = new GMSpinner(hframe,4,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_LEFT);
    trackspinner->setRange(0,1000);

    new FXLabel(hframe,tr("&Disc"),NULL,LABEL_NORMAL|LAYOUT_LEFT|LAYOUT_CENTER_Y);
    discspinner = new GMSpinner(hframe,3,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_LEFT);
    discspinner->setRange(0,100);

    yearfield = new GMTextField(hframe,4,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_RIGHT|TEXTFIELD_INTEGER|TEXTFIELD_LIMITED);
    new FXLabel(hframe,tr("Y&ear"),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y|LAYOUT_RIGHT);

    }
  else {
    matrix = new FXMatrix(main,2,LAYOUT_FILL_X|MATRIX_BY_COLUMNS,0,0,0,0,0,0,0,0);

    if (samemask&SAME_DISC) {
      new FXLabel(matrix,tr("&Disc"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
      hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
      discspinner = new GMSpinner(hframe,3,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_LEFT);
      discspinner->setRange(0,100);
      }
    else {
      new FXLabel(matrix,tr("&Disc"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
      hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
      discfield = new GMTextField(hframe,3,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_LEFT|TEXTFIELD_INTEGER|TEXTFIELD_LIMITED|JUSTIFY_RIGHT);
      }

    yearfield = new GMTextField(hframe,4,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_RIGHT|TEXTFIELD_INTEGER|TEXTFIELD_LIMITED|JUSTIFY_RIGHT);
    new FXLabel(hframe,tr("Y&ear"),NULL,LABEL_NORMAL|LAYOUT_CENTER_Y|LAYOUT_RIGHT);
    }

  if (tracks.no()==1) {
    new FXLabel(matrix,tr("T&itle"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
    titlefield = new GMTextField(matrix,20,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_SUNKEN|FRAME_THICK);
    }

  new FXLabel(matrix,tr("&Artist"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  trackartistbox = new GMComboBox(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_LINE);

  new FXLabel(matrix,tr("Album A&rtist"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  albumartistbox = new GMComboBox(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_LINE);

  new FXLabel(matrix,tr("A&lbum"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  albumbox = new GMComboBox(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_LINE);

  new FXLabel(matrix,tr("C&omposer"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  composerbox = new GMComboBox(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_LINE);

  new FXLabel(matrix,tr("Cond&uctor"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
  conductorbox = new GMComboBox(matrix,30,NULL,0,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN|FRAME_LINE);


//  new FXLabel(matrix,tr("&Genre"),NULL,LABEL_NORMAL|LAYOUT_RIGHT|LAYOUT_CENTER_Y);
//  hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
//  genrebox = new GMComboBox(hframe,20,NULL,0,LAYOUT_FILL_X|FRAME_LINE);

  if (tracks.no()>1 && tracks.no()<=0xFFFF) {
    new FXFrame(matrix,FRAME_NONE);
    hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
    autonumber = new GMCheckButton(hframe,tr("Auto track number. Offset:"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL|LAYOUT_CENTER_Y);
    autonumberoffset = new GMSpinner(hframe,2,NULL,0,FRAME_SUNKEN|FRAME_THICK|LAYOUT_LEFT);
    autonumber->setTarget(autonumberoffset);
    autonumber->setSelector(FXWindow::ID_TOGGLEENABLED);
    autonumberoffset->disable();
    autonumberoffset->setRange(1,99);
    }

  new FXFrame(matrix,FRAME_NONE);
  updatetags = new GMCheckButton(matrix,tr("Update Tag in File"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);
  new FXFrame(matrix,FRAME_NONE);

  hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_X|LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);

  updatefilename = new GMCheckButton(hframe,fxtr("Update Filename"),NULL,0,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL|LAYOUT_CENTER_Y);
  new GMButton(hframe,tr("Set export template…") ,NULL,this,ID_FILENAME_TEMPLATE,FRAME_RAISED,0,0,0,0);

  updatetags->setCheck(getApp()->reg().readBoolEntry(defaults_section,update_tags_key,false));
  updatefilename->setCheck(getApp()->reg().readBoolEntry(defaults_section,update_filenames_key,false));


  db->listArtists(trackartistbox);
  trackartistbox->setSortFunc(generic_name_sort);
  trackartistbox->setNumVisible(FXMIN(10,trackartistbox->getNumItems()));
  trackartistbox->sortItems();
  trackartistbox->setCurrentItem(-1);

  db->listArtists(albumartistbox);
  albumartistbox->setSortFunc(generic_name_sort);
  albumartistbox->setNumVisible(FXMIN(10,albumartistbox->getNumItems()));
  albumartistbox->sortItems();
  albumartistbox->setCurrentItem(-1);

  db->listArtists(composerbox);
  composerbox->setSortFunc(generic_name_sort);
  composerbox->setNumVisible(FXMIN(10,composerbox->getNumItems()));
  composerbox->sortItems();
  composerbox->setCurrentItem(-1);

  db->listArtists(conductorbox);
  conductorbox->setSortFunc(generic_name_sort);
  conductorbox->setNumVisible(FXMIN(10,conductorbox->getNumItems()));
  conductorbox->sortItems();
  conductorbox->setCurrentItem(-1);

  displayTracks();
  }


GMEditTrackDialog::~GMEditTrackDialog() {
  if (art) delete art;
  }

long GMEditTrackDialog::onCmdFilenameTemplate(FXObject*,FXSelector,void*){
  GMFilenameTemplateDialog dialog(this);
  dialog.execute();
  return 1;
  }


void GMEditTrackDialog::getTrackSelection() {

  tracks.clear();

  GMPlayerManager::instance()->getTrackView()->getSelectedTracks(tracks);
  db->getTrack(tracks[0],info);
  if (tracks.no()==1) {
    if (art) {
      delete art;
      art=NULL;
      }
    properties.load(info.mrl);
    art = GMCover::toImage(GMCover::fromTag(info.mrl,0));
    if (art) art->create();
    }

  samemask=SAME_ALBUM|SAME_ARTIST|SAME_ALBUMARTIST|SAME_GENRE|SAME_YEAR|SAME_DISC|SAME_COMPOSER|SAME_CONDUCTOR;
  if (tracks.no()>1) {
    GMTrack other;
    for (FXint i=1;i<tracks.no() && samemask ;i++) {
      db->getTrack(tracks[i],other);
      if (other.album!=info.album) samemask&=~SAME_ALBUM;
      if (other.artist!=info.artist) samemask&=~SAME_ARTIST;
      if (other.album_artist!=info.album_artist) samemask&=~SAME_ALBUMARTIST;
//      if (other.genre!=info.genre) samemask&=~SAME_GENRE;
      if (other.year!=info.year) samemask&=~SAME_YEAR;
      if (GMDISCNO(other.no)!=GMDISCNO(info.no)) samemask&=~SAME_DISC;
      if (other.composer!=info.composer) samemask&=~SAME_COMPOSER;
      if (other.conductor!=info.conductor) samemask&=~SAME_CONDUCTOR;
      }
    }
  }

void GMEditTrackDialog::displayTracks() {

  albumbox->clearItems();
  db->listAlbums(albumbox,tracks[0]);
  albumbox->setSortFunc(generic_name_sort);
  albumbox->setNumVisible(FXMIN(10,albumbox->getNumItems()));
  albumbox->sortItems();
  albumbox->setCurrentItem(-1);


/*
  db->listGenres(genrebox,false);
  genrebox->setSortFunc(genre_list_sort);
  genrebox->setNumVisible(FXMIN(10,genrebox->getNumItems()));
  genrebox->sortItems();
  genrebox->setCurrentItem(-1);
*/

  if (tracks.no()==1) {
    trackspinner->setValue(GMTRACKNO(info.no));
    albumbox->setCurrentItem(albumbox->findItem(info.album));
    albumbox->setText(info.album);

    trackartistbox->setCurrentItem(trackartistbox->findItem(info.artist));
    albumartistbox->setCurrentItem(albumartistbox->findItem(info.album_artist));
    trackartistbox->setText(info.artist);
    albumartistbox->setText(info.album_artist);

    composerbox->setCurrentItem(composerbox->findItem(info.composer));
    conductorbox->setCurrentItem(conductorbox->findItem(info.conductor));
    composerbox->setText(info.composer);
    conductorbox->setText(info.conductor);


    //genrebox->setCurrentItem(genrebox->findItem(info.genre));
    yearfield->setText(FXString::value(info.year));
    titlefield->setText(info.title);
    discspinner->setValue(GMDISCNO(info.no));

    coverview->setImage(art);
    if (art)
      covertab->show();
    else
      covertab->hide();
    }
  else {
    if (samemask&SAME_ALBUM) albumbox->setCurrentItem(albumbox->findItem(info.album));
    if (samemask&SAME_ARTIST) trackartistbox->setCurrentItem(trackartistbox->findItem(info.artist));
    if (samemask&SAME_ALBUMARTIST) albumartistbox->setCurrentItem(albumartistbox->findItem(info.album_artist));
    if (samemask&SAME_COMPOSER) composerbox->setCurrentItem(composerbox->findItem(info.composer));
    if (samemask&SAME_CONDUCTOR) conductorbox->setCurrentItem(conductorbox->findItem(info.conductor));
//    if (samemask&SAME_GENRE) genrebox->setCurrentItem(genrebox->findItem(info.genre));
    if (samemask&SAME_YEAR) yearfield->setText(FXString::value(info.year));
    if (samemask&SAME_DISC) discspinner->setValue(GMDISCNO(info.no));
    }
  }

long GMEditTrackDialog::onCmdSwitchTrack(FXObject*,FXSelector sel,void*){
  saveTracks();
  if (FXSELID(sel)==ID_NEXT_TRACK)
    GMPlayerManager::instance()->getTrackView()->selectNext();
  else
    GMPlayerManager::instance()->getTrackView()->selectPrevious();
  getTrackSelection();
  displayTracks();
  return 1;
  }

long GMEditTrackDialog::onCmdResetTrack(FXObject*,FXSelector,void*){
  getTrackSelection();
  displayTracks();
  return 1;
  }



FXbool GMEditTrackDialog::saveTracks() {
  FXbool changed=false;
  FXbool sync=false;
  FXString field;
  FXString altfield;

  try {
    db->begin();

    /// Update Title
    if (tracks.no()==1) {
      field=titlefield->getText().trim().simplify();
      if (!field.empty() && info.title!=field){
        db->setTrackTitle(tracks[0],field);
        changed=true;
        }
      }


    /// Track Artist
    field=trackartistbox->getText().trim().simplify();
    if (( !field.empty())   && (
        ( tracks.no()>1 && ( (!(samemask&SAME_ARTIST)) || field!=info.artist )) ||
        ( tracks.no()==1 && info.artist!=field ) )) {
      db->setTrackArtist(tracks,field);
      changed=true;
      sync=true;
      }

    /// Composer
    field=composerbox->getText().trim().simplify();
    if (( !field.empty())   && (
        ( tracks.no()>1 && ( (!(samemask&SAME_COMPOSER)) || field!=info.composer )) ||
        ( tracks.no()==1 && info.composer!=field ) )) {
      db->setTrackComposer(tracks,field);
      changed=true;
      sync=true;
      }

    /// Conductor
    field=conductorbox->getText().trim().simplify();
    if (( !field.empty())   && (
        ( tracks.no()>1 && ( (!(samemask&SAME_CONDUCTOR)) || field!=info.conductor )) ||
        ( tracks.no()==1 && info.conductor!=field ) )) {
      db->setTrackConductor(tracks,field);
      changed=true;
      sync=true;
      }


    /// YEAR
    field=yearfield->getText().trim().simplify();
    if (!field.empty()){
      FXint year=yearfield->getText().toInt();
      if ( ( tracks.no()>1 && ( (!(samemask&SAME_YEAR)) || info.year!=year ) ) ||
           ( tracks.no()==1 && info.year!=year )) {
        db->setTrackYear(tracks,year);
        changed=true;
        }
      }

    /// DISC and TRACK number
    if (tracks.no()==1) {
      if (GMTRACKNO(info.no)!=trackspinner->getValue() || GMDISCNO(info.no)!=discspinner->getValue() ) {
        db->setTrackNumber(tracks,discspinner->getValue(),trackspinner->getValue());
        changed=true;
        sync=true;
        }
      }
    else {
      if (autonumber->getCheck()) {
        if (discspinner) {
          db->setTrackNumber(tracks,discspinner->getValue(),autonumberoffset->getValue(),true);
          }
        else if (discfield->getText().empty()) {
          db->setTrackTrackNumber(tracks,autonumberoffset->getValue(),true);
          }
        else {
          db->setTrackNumber(tracks,discfield->getText().toUInt(),autonumberoffset->getValue(),true);
          }
        changed=true;
        sync=true;
        }
      else {
        if (discspinner) {
          db->setTrackDiscNumber(tracks,discspinner->getValue());
          changed=true;
          sync=true;
          }
        else {
          field=discfield->getText().trim().simplify();
          if (!field.empty()) {
            FXint disc=discfield->getText().toInt();
            db->setTrackDiscNumber(tracks,disc);
            changed=true;
            sync=true;
            }
          }
        }
      }

    /// ALBUM ARTIST / ALBUM
    field=albumartistbox->getText().trim().simplify();
    altfield=albumbox->getText().trim().simplify();

    if (( !field.empty()) && (
        ( tracks.no()>1 && ( (!(samemask&SAME_ALBUMARTIST)) || field!=info.album_artist )) ||
        ( tracks.no()==1 && info.album_artist!=field ) )) {

      if (altfield.empty() && (samemask&SAME_ALBUM)) {
        altfield=info.album;
        }
      db->setTrackAlbumArtist(tracks,field,altfield);
      changed=true;
      sync=true;
      }
    else if (( !altfield.empty()) && (
             ( tracks.no()>1 && ( (!(samemask&SAME_ALBUM)) || altfield!=info.album )) ||
             ( tracks.no()==1 && info.album!=altfield ) )) {
      db->setTrackAlbum(tracks,altfield,(samemask&SAME_ALBUMARTIST));
      changed=true;
      sync=true;
      }

    db->sync_tracks_removed();
    db->commit();
    }
  catch(GMDatabaseException&) {
    db->rollback();
    return false;
    }

  if (updatetags->getCheck()) {
    if (changed || (FXMessageBox::question(GMPlayerManager::instance()->getMainWindow(),MBOX_YES_NO,fxtr("Update Tags?"),fxtr("No tracks were updated.\nWould you still like to write the tags for the selected tracks?"))==MBOX_CLICKED_YES)) {
      GMTagUpdateTask * task = new GMTagUpdateTask(db,tracks);
      GMPlayerManager::instance()->runTask(task);
      }
    }

  if (updatefilename->getCheck()) {
    if (updateTrackFilenames(db,tracks))
      changed=true;
    }

  getApp()->reg().writeBoolEntry(defaults_section,update_tags_key,updatetags->getCheck());
  getApp()->reg().writeBoolEntry(defaults_section,update_filenames_key,updatefilename->getCheck());

  return (sync || changed);
  }

long GMEditTrackDialog::onCmdAccept(FXObject*sender,FXSelector sel,void*ptr){
  FXDialogBox::onCmdAccept(sender,sel,ptr);
  if (saveTracks())
    GMPlayerManager::instance()->getTrackView()->refreshUpdate();
  return 1;
  }
