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
#include "icons.h"

#include "GMTrack.h"
#include "GMPlayer.h"
#include "GMWindow.h"
#include "GMTrackList.h"
#include "GMList.h"
#include "GMRemote.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMDatabaseSource.h"
#include "GMTrackView.h"
#include "GMSourceView.h"
#include "GMIconTheme.h"
#include "GMEQDialog.h"

const FXchar * default_presets[]={
  "Classical,0,0,0,0,0,0,-2.4,-2.4,-2.4,-3.0",
  "Club, 0,0,1.2,1.8,1.8,1.8,1.2,0,0,0",
  "Dance,3.0,2.1,0.6,0,0,-1.8,-2.4,-2.4,0,0",
  "Full Bass,4.2,4.2,4.2,2.4,1.2,-1.2,-2.7,-3.0,-3.3,-3.3",
  "Full Treble,-3.0,-3.0,-3.0,-1.5,0.9,3.3,4.8,4.8,4.8,5.1",
  "Full Bass+Treble,2.1,1.8,0,-2.4,-1.5,0.6,2.7,3.3,3.6,3.6",
  "Laptop/Headphones,1.5,3.0,1.5,-1.2,0,-1.8,-2.4,-2.4,0,0",
  "Large Hall,3.0,3.0,1.8,1.8,0,-1.5,-1.5,-1.5,0,0",
  "Live,-1.5,0,1.2,1.5,1.8,1.8,1.2,0.9,0.9,0.6",
  "Party,2.1,2.1,0,0,0,0,0,0,2.1,2.1",
  "Pop,-0.6,1.5,2.1,2.4,1.5,-0.3,-0.9,-0.9,-0.6,-0.6",
  "Reggae,0,0,-0.3,-1.8,0,-2.1,-2.1,0,0,0",
  "Rock,2.4,1.5,-1.8,-2.4,-1.2,1.2,2.7,3.3,3.3,3.3",
  "Soft,1.5,0.6,-0.3,-0.9,-0.3,1.2,2.7,3.0,3.3,3.6",
  "Ska,-0.9,-1.5,-1.5,-0.3,1.2,1.8,2.7,3.0,3.3,3.0",
  "Soft Rock,1.2,1.2,0.6,-0.3,-1.5,-1.8,-1.2,-0.3,0.9,2.7",
  "Techno,2.4,1.8,0,-1.8,-1.5,0,2.4,3.0,3.0,2.7",
  "Zero,0,0,0,0,0,0,0,0,0,0"
 };



GMEQDialog * GMEQDialog::myself = NULL;


FXDEFMAP(GMEQDialog) GMEQDialogMap[]={
  FXMAPFUNCS(SEL_UPDATE,GMEQDialog::ID_EQ_30HZ,GMEQDialog::ID_EQ_16000HZ,GMEQDialog::onUpdEQ),
  FXMAPFUNCS(SEL_COMMAND,GMEQDialog::ID_EQ_30HZ,GMEQDialog::ID_EQ_16000HZ,GMEQDialog::onCmdEQ),
  FXMAPFUNCS(SEL_CHANGED,GMEQDialog::ID_EQ_30HZ,GMEQDialog::ID_EQ_16000HZ,GMEQDialog::onCmdEQ),

  FXMAPFUNC(SEL_COMMAND,GMEQDialog::ID_PRESET_EQ,GMEQDialog::onCmdPresetEQ),
#if FOXVERSION >= FXVERSION(1,7,0)
  FXMAPFUNC(SEL_CHANGED,GMEQDialog::ID_PRESET_EQ,GMEQDialog::onCmdPresetEQ),
#endif
  FXMAPFUNC(SEL_COMMAND,GMEQDialog::ID_RESET,GMEQDialog::onCmdReset),
  FXMAPFUNC(SEL_COMMAND,GMEQDialog::ID_SAVE,GMEQDialog::onCmdSave),
  FXMAPFUNC(SEL_COMMAND,GMEQDialog::ID_DELETE,GMEQDialog::onCmdDelete),

  FXMAPFUNC(SEL_UPDATE,GMEQDialog::ID_RESET,GMEQDialog::onUpdReset),
  FXMAPFUNC(SEL_UPDATE,GMEQDialog::ID_SAVE,GMEQDialog::onUpdSave),
  FXMAPFUNC(SEL_UPDATE,GMEQDialog::ID_DELETE,GMEQDialog::onUpdDelete),

  };

FXIMPLEMENT(GMEQDialog,FXDialogBox,GMEQDialogMap,ARRAYNUMBER(GMEQDialogMap))

GMEQDialog::GMEQDialog(FXWindow * p) : FXDialogBox(p,FXString::null,DECOR_BORDER|DECOR_TITLE|DECOR_CLOSE,0,0,0,0,2,2,2,2) {

  FXASSERT(myself==NULL);
  myself=this;
  //FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
 // new FXButton(closebox,tr("&Close"),NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  //new FXSeparator(this,LAYOUT_FILL_X|SEPARATOR_GROOVE|LAYOUT_SIDE_BOTTOM);


  setTitle(tr("Equalizer"));


  /// Create a fixed font, about the same size as the normal font
  FXint size = FXApp::instance()->getNormalFont()->getSize() - 10;
  font_small = new FXFont(FXApp::instance(),"mono",(int)size/10,FXFont::Normal,FXFont::Straight,FONTENCODING_UNICODE,FXFont::NonExpanded,FXFont::Modern|FXFont::Fixed);


  FXRealSlider *slider;
  FXLabel * label;
  FXTextField * textfield;

  {

  FXStringDict * dict = getApp()->reg().find("equalizer-presets");

  /// Add default presets if they don't exist.
  if (!dict ||  dict->no()==0){
    for (FXint i=0;i<(FXint)ARRAYNUMBER(default_presets);i++){
      getApp()->reg().writeStringEntry("equalizer-presets",GMStringVal(i).text(),default_presets[i]);
      }
    }

  FXString entry;
  dict = getApp()->reg().find("equalizer-presets");
  if (dict && dict->no()>0) {
    presets.no(dict->no());
    for (FXint pos=dict->first(),i=0;pos<dict->size();pos=dict->next(pos),i++){
      entry=dict->data(pos);
      presets[i].name = entry.section(',',0);
      presets[i].bands.parse(entry.after(',',1));
      }
    }

  }



  FXHorizontalFrame * presetframe = new FXHorizontalFrame(this,LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(presetframe,tr("Equalizer:"),NULL,LAYOUT_CENTER_Y);
  presetlist = new GMListBox(presetframe,this,ID_PRESET_EQ,LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK);
  new FXButton(presetframe,tr("\tSave"),GMIconTheme::instance()->icon_export,this,ID_SAVE,FRAME_RAISED|BUTTON_TOOLBAR);
  new FXButton(presetframe,tr("\tReset"),GMIconTheme::instance()->icon_undo,this,ID_RESET,FRAME_RAISED|BUTTON_TOOLBAR);
  new FXButton(presetframe,tr("\tRemove"),GMIconTheme::instance()->icon_delete,this,ID_DELETE,FRAME_RAISED|BUTTON_TOOLBAR);


  listPresets();


  const FXuint sliderstyle = LAYOUT_FILL_Y|SLIDER_VERTICAL|LAYOUT_CENTER_X|SLIDER_TICKS_LEFT|SLIDER_TICKS_RIGHT|LAYOUT_FILL_ROW;
  const FXchar * labels[]={"30","60","125","250","500","1k","2k","4k","8k","16k"};


  new FXSeparator(this,LAYOUT_FILL_X|SEPARATOR_GROOVE);

  FXMatrix * eqmatrix = new FXMatrix(this,3,MATRIX_BY_ROWS|LAYOUT_CENTER_X|LAYOUT_CENTER_Y|LAYOUT_FIX_HEIGHT,0,0,0,200,0,0,0,0,3,2);


  new FXFrame(eqmatrix,FRAME_NONE);
   FXVerticalFrame * vframe = new FXVerticalFrame(eqmatrix,LAYOUT_FILL_Y|LAYOUT_FILL_ROW,0,0,0,0,0,0,0,0,0,0);
  label = new FXLabel(vframe,"+12db",NULL,LAYOUT_TOP|LAYOUT_RIGHT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  label = new FXLabel(vframe,"0",NULL,LAYOUT_CENTER_Y|LAYOUT_RIGHT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  label = new FXLabel(vframe,"-12db",NULL,LAYOUT_BOTTOM|LAYOUT_RIGHT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  new FXFrame(eqmatrix,FRAME_NONE);

  textfield=new FXTextField(eqmatrix,4,GMPlayerManager::instance()->getPlayer(),GMPlayer::ID_PREAMP,LAYOUT_CENTER_X|TEXTFIELD_INTEGER|FRAME_SUNKEN|JUSTIFY_RIGHT,0,0,0,0,2,2,0,0);
  textfield->setFont(font_small);

  FXRealSlider * rslider = new FXRealSlider(eqmatrix,GMPlayerManager::instance()->getPlayer(),GMPlayer::ID_PREAMP,sliderstyle,0,0,0,0,0,0,0,0);
  rslider->setRange(-12.0,12.0);
  rslider->setTickDelta(12.0);
  rslider->setIncrement(1);
  rslider->setValue(0.0);
  label = new FXLabel(eqmatrix,tr("Pre-amp"),NULL,LAYOUT_CENTER_X,0,0,0,0,2,2,0,0);
  label->setFont(font_small);

  for (FXint i=0;i<10;i++) {
    textfield = new FXTextField(eqmatrix,4,this,ID_EQ_30HZ+i,LAYOUT_CENTER_X|TEXTFIELD_REAL|FRAME_SUNKEN|JUSTIFY_RIGHT,0,0,0,0,2,2,0,0);
    textfield->setFont(font_small);
    slider = new FXRealSlider(eqmatrix,this,ID_EQ_30HZ+i,sliderstyle,0,0,0,0,0,0,0,0);
    slider->setRange(-12,12);
    slider->setTickDelta(6);
    slider->setIncrement(0.5);
#if FOXVERSION >= FXVERSION(1,7,0)
    slider->setGranularity(0.1);
#endif
    slider->setValue(0);
    eqslider[i]=slider;
    FXLabel * label = new FXLabel(eqmatrix,labels[i],NULL,LAYOUT_CENTER_X,0,0,0,0,2,2,0,0);
    label->setFont(font_small);
    }

  new FXFrame(eqmatrix,FRAME_NONE);
  vframe = new FXVerticalFrame(eqmatrix,LAYOUT_FILL_Y|LAYOUT_FILL_ROW,0,0,0,0,0,0,0,0,0,0);
  label = new FXLabel(vframe,"+6db",NULL,LAYOUT_TOP|LAYOUT_LEFT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  label = new FXLabel(vframe,"0",NULL,LAYOUT_CENTER_Y|LAYOUT_LEFT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  label = new FXLabel(vframe,"-6db",NULL,LAYOUT_BOTTOM|LAYOUT_LEFT,0,0,0,0,2,2,0,0);
  label->setFont(font_small);
  new FXFrame(eqmatrix,FRAME_NONE);

  if (getApp()->reg().readIntEntry("eqdialog","x",-1)!=-1) {
    FXint xx=getApp()->reg().readIntEntry("eqdialog","x",getX());
    FXint yy=getApp()->reg().readIntEntry("eqdialog","y",getY());
    move(xx,yy);
    }
  else {
    place(PLACEMENT_OWNER);
    }
  }

GMEQDialog::~GMEQDialog(){
  myself=NULL;
  FXString entry;
  getApp()->reg().deleteSection("equalizer-presets");
  for (FXint i=0;i<presets.no();i++){
    entry.clear();
    presets[i].bands.unparse(entry);
    getApp()->reg().writeFormatEntry("equalizer-presets",GMStringVal(i).text(),"%s,%s",presets[i].name.text(),entry.text());
    }
  }




void GMEQDialog::listPresets() {

  for (FXint i=0;i<presets.no();i++){
    presetlist->appendItem(presets[i].name,NULL,(void*)(FXival)(i+1));
    }

  GMEqualizer eq;
  GMPlayerManager::instance()->getPlayer()->getEqualizer(eq);

  if (eq.enabled) {
    FXbool found=false;
    for (FXint i=0;!found && i<presets.no();i++){
      if (eq.bands==presets[i].bands) {
        presetlist->setCurrentItem(i);
        found=true;
        }
      }
    presetlist->setSortFunc(FXList::ascending);
    presetlist->sortItems();
    presetlist->prependItem(tr("Disabled"));

    if (!found) {
      presetlist->insertItem(1,tr("Manual"),NULL,(void*)(FXival)-1);
      presetlist->setCurrentItem(1);
      }
    }
  else {
    presetlist->setSortFunc(FXList::ascending);
    presetlist->sortItems();
    presetlist->prependItem(tr("Disabled"));
    presetlist->setCurrentItem(0);
    }
  presetlist->setNumVisible(FXMIN(9,presetlist->getNumItems()));
  }


void GMEQDialog::hide() {
  FXDialogBox::hide();

  /// Save Position
  getApp()->reg().writeIntEntry("eqdialog","x",getX());
  getApp()->reg().writeIntEntry("eqdialog","y",getY());


  delete this;
  }


GMEQDialog * GMEQDialog::instance() {
  return myself;
  }


long GMEQDialog::onCmdEQ(FXObject*sender,FXSelector sel,void*ptr){
  long result = GMPlayerManager::instance()->getPlayer()->handle(sender,FXSEL(FXSELTYPE(sel),GMPlayer::ID_EQ_30HZ+(FXSELID(sel)-ID_EQ_30HZ)),ptr);

  GMEqualizer eq;
  GMPlayerManager::instance()->getPlayer()->getEqualizer(eq);
  if (eq.enabled) {
    FXbool found=false;
    for (FXint i=0;!found && i<presets.no();i++){
      if (eq.bands==presets[i].bands) {
        presetlist->setCurrentItem(presetlist->findItemByData((void*)(FXival)(i+1)));
        FXint item = presetlist->findItemByData((void*)(FXival)(-1));
        if (item>=0) presetlist->removeItem(item);
        found=true;
        }
      }
    if (!found) {
      FXint item = presetlist->findItemByData((void*)(FXival)(-1));
      if (item>=0) {
        presetlist->setCurrentItem(item);
        }
      else {
        presetlist->insertItem(1,tr("Manual"),NULL,(void*)(FXival)-1);
        presetlist->setCurrentItem(1);
        }
      }
    }
  return result;
  }

long GMEQDialog::onUpdEQ(FXObject*sender,FXSelector sel,void*ptr){
  return GMPlayerManager::instance()->getPlayer()->handle(sender,FXSEL(SEL_UPDATE,GMPlayer::ID_EQ_30HZ+(FXSELID(sel)-ID_EQ_30HZ)),ptr);
  }


long GMEQDialog::onCmdDelete(FXObject*,FXSelector,void*){
  FXint p = ((FXint)(FXival)presetlist->getItemData(presetlist->getCurrentItem()))-1;
  if (FXMessageBox::question(this,MBOX_YES_NO,tr("Delete Preset"),fxtrformat("Are you sure you want to delete %s preset?"),presets[p].name.text())==MBOX_CLICKED_YES){
    presets.erase(p);
    presetlist->clearItems();
    listPresets();
    }
  return 1;
  }

long GMEQDialog::onUpdDelete(FXObject*sender,FXSelector,void*){
  FXint p = (FXint)(FXival)presetlist->getItemData(presetlist->getCurrentItem());
  if (0<p)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


long GMEQDialog::onCmdSave(FXObject*,FXSelector,void*){
  FXString name;
  if (FXInputDialog::getString(name,this,tr("Preset Name"),tr("Please enter preset name:"),NULL)){
    if (!name.empty()) {
      for (FXint i=0;i<presets.no();i++){
        if (presets[i].name==name) {
          if (FXMessageBox::question(this,MBOX_YES_NO,tr("Overwrite Preset"),fxtrformat("Preset %s already exists. Would you like to overwrite it?"),name.text())==MBOX_CLICKED_YES){
            /// overwrite settings
            for (FXint b=0;b<10;b++){
              presets[i].bands[b]=eqslider[b]->getValue();
              }
            }
          /// cancel
          return 1;
          }
        }
      /// Everything ok

      presets.no(presets.no()+1);
      presets[presets.no()-1].name=name;
      for (FXint b=0;b<10;b++){
        presets[presets.no()-1].bands[b]=eqslider[b]->getValue();
        }

      presetlist->appendItem(name,NULL,(void*)(FXival)(presets.no()));
      presetlist->setCurrentItem(presetlist->getNumItems()-1);
      presetlist->setNumVisible(FXMIN(9,presetlist->getNumItems()));
      presetlist->sortItems();
      presetlist->moveItem(0,presetlist->findItemByData(NULL));
      }
    }
  return 1;
  }

long GMEQDialog::onUpdSave(FXObject*sender,FXSelector,void*){
  FXint p = (FXint)(FXival)presetlist->getItemData(presetlist->getCurrentItem());
  if (p!=0)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


long GMEQDialog::onCmdReset(FXObject*,FXSelector,void*){
  FXint p = (FXint)(FXival)presetlist->getItemData(presetlist->getCurrentItem());
  FXString entry;
  for (FXint i=0;i<(FXint)ARRAYNUMBER(default_presets);i++){
    entry = default_presets[i];
    if (presets[p-1].name==entry.before(',')){
      presets[p-1].bands.parse(entry.after(',',1));
      GMPlayerManager::instance()->getPlayer()->setEqualizer(presets[p-1].bands);
      for (FXint i=0;i<10;i++){
        eqslider[i]->setValue(presets[p-1].bands[i]);
        }
      break;
      }
    }
  return 1;
  }

long GMEQDialog::onUpdReset(FXObject*sender,FXSelector,void*){
  FXint p = (FXint)(FXival)presetlist->getItemData(presetlist->getCurrentItem());
  if (0<p)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }

#if FOXVERSION >= FXVERSION(1,7,0)
long GMEQDialog::onCmdPresetEQ(FXObject*sender,FXSelector sel,void*ptr){
  GMListBox * list = reinterpret_cast<GMListBox*>(sender);
  if (FXSELTYPE(sel)==SEL_CHANGED && list->isMenuShown()) return 1;
#else
long GMEQDialog::onCmdPresetEQ(FXObject*sender,FXSelector,void*ptr){
  GMListBox * list = reinterpret_cast<GMListBox*>(sender);
#endif
  FXint item = (FXint)(FXival)ptr;
  FXint p = ((FXint)(FXival)list->getItemData(item))-1;
  if (p>=0) {
    GMPlayerManager::instance()->getPlayer()->setEqualizer(presets[p].bands);
    for (FXint i=0;i<10;i++){
      eqslider[i]->setValue(presets[p].bands[i]);
      }
    item = presetlist->findItemByData((void*)(FXival)(-1));
    if (item>=0) presetlist->removeItem(item);
    }
  else if (p==-1) {
    GMPlayerManager::instance()->getPlayer()->disableEqualizer();
    item = presetlist->findItemByData((void*)(FXival)(-1));
    if (item>=0) presetlist->removeItem(item);
    }
  return 1;
  }
