/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2015 by Sander Jansen. All Rights Reserved      *
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
#include "GMList.h"
#include "GMFontDialog.h"

FXDEFMAP(GMFontDialog) GMFontDialogMap[]={
  FXMAPFUNC(SEL_UPDATE,GMFontDialog::ID_PITCH,GMFontDialog::onUpdPitch),
  FXMAPFUNC(SEL_UPDATE,GMFontDialog::ID_SCALABLE,GMFontDialog::onUpdScalable),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_PITCH,GMFontDialog::onCmdPitch),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_SCALABLE,GMFontDialog::onCmdScalable),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_FAMILY,GMFontDialog::onCmdFamily),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_STYLE,GMFontDialog::onCmdStyle),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_SIZE,GMFontDialog::onCmdSize),
  FXMAPFUNC(SEL_COMMAND,GMFontDialog::ID_SIZE_TEXT,GMFontDialog::onCmdSizeText)
  };

FXIMPLEMENT(GMFontDialog,FXDialogBox,GMFontDialogMap,ARRAYNUMBER(GMFontDialogMap))


GMFontDialog::GMFontDialog(FXApp * a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h) :
  FXDialogBox(a,name,opts,x,y,w,h,3,3,3,3) {
  }

GMFontDialog::GMFontDialog(FXWindow* o,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h) :
  FXDialogBox(o,name,opts,x,y,w,h,4,4,4,4) {
  GMScrollFrame *sunken;
//  FXVerticalFrame * frm;

  FXHorizontalFrame *closebox=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,2,2);
  new GMButton(closebox," &Accept",NULL,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);
  new GMButton(closebox," &Cancel ",NULL,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 15,15);

  FXVerticalFrame * main = new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
//frm=new FXVerticalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK|LAYOUT_BOTTOM,0,0,0,0,0,0,0,0);
  sunken=new GMScrollFrame(main);
  sunken->setLayoutHints(LAYOUT_BOTTOM|LAYOUT_FILL_X|LAYOUT_FILL_Y);

  FXScrollWindow *scrollwindow=new FXScrollWindow(sunken,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  GMScrollArea::replaceScrollbars(scrollwindow);
  preview=new FXLabel(scrollwindow,"ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789",NULL,LAYOUT_CENTER_X|LAYOUT_CENTER_Y);
  preview->setBackColor(getApp()->getBackColor());
//  new FXLabel(main,"Preview:",NULL,LAYOUT_BOTTOM);

  FXHorizontalFrame * filterframe = new FXHorizontalFrame(main,LAYOUT_FILL_X|LAYOUT_BOTTOM,0,0,0,0,0,0,3,3,0,0);
  new FXLabel(filterframe,"Pitch:",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y,0,0,0,0);
  pitchlist = new GMListBox(filterframe,this,ID_PITCH);
  pitchlist->appendItem("Any");
  pitchlist->appendItem("Fixed",NULL,(void*)(FXuval)FXFont::Fixed);
  pitchlist->appendItem("Variable",NULL,(void*)(FXuval)FXFont::Variable);
  pitchlist->setNumVisible(3);

  new FXLabel(filterframe,"  Type:",NULL,LABEL_NORMAL|LAYOUT_CENTER_Y);
  scalelist = new GMListBox(filterframe,this,ID_SCALABLE);
  scalelist->appendItem("Any");
  scalelist->appendItem("Scalable",NULL,(void*)(FXuval)FXFont::Scalable);
  scalelist->setNumVisible(2);

  FXHorizontalFrame * input = new FXHorizontalFrame(main,LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);

  FXVerticalFrame * sizeframe = new FXVerticalFrame(input,LAYOUT_RIGHT|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0,0,0);
  new FXLabel(sizeframe,"Size:",NULL,LABEL_NORMAL,0,0,0,0,0,0);

  FXVerticalFrame * sizeinputframe = new FXVerticalFrame(sizeframe,LAYOUT_RIGHT|LAYOUT_FILL_Y,0,0,0,0,0,0,0,0);
  sizefield=new GMTextField(sizeinputframe,3,this,ID_SIZE_TEXT,TEXTFIELD_NORMAL|TEXTFIELD_INTEGER|LAYOUT_FILL_X);
  sunken=new GMScrollFrame(sizeinputframe);
  sizelist=new GMList(sunken,this,ID_SIZE,LAYOUT_FILL_Y|LAYOUT_FILL_X|LIST_SINGLESELECT);

  FXHorizontalFrame * mainframe = new FXHorizontalFrame(input,LAYOUT_LEFT|LAYOUT_FILL_Y|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  FXSpring * familyframe = new FXSpring(mainframe,LAYOUT_FILL_X|LAYOUT_FILL_Y,4,0,0,0,0,0,0,0,0,0,0,0);
  new FXLabel(familyframe,"Family:",NULL,LABEL_NORMAL,0,0,0,0);
  sunken=new GMScrollFrame(familyframe);
  familylist = new GMList(sunken,this,ID_FAMILY,LAYOUT_FILL_Y|LAYOUT_FILL_X|LIST_BROWSESELECT);
  familylist->setSortFunc(FXList::ascending);

  FXSpring * styleframe  = new FXSpring(mainframe,LAYOUT_FILL_X|LAYOUT_FILL_Y,3,0,0,0,0,0,0,0,0,0,0,0);
  new FXLabel(styleframe,"Style:",NULL,LABEL_NORMAL,0,0,0,0);
  sunken=new GMScrollFrame(styleframe);
  stylelist=new GMList(sunken,this,ID_STYLE,LAYOUT_FILL_Y|LAYOUT_FILL_X|LIST_BROWSESELECT);
  stylelist->setSortFunc(FXList::ascending);

  selected = getApp()->getNormalFont()->getActualFontDesc();
  selected.flags|=FXFont::Scalable;
  }

GMFontDialog::~GMFontDialog(){
  }


void GMFontDialog::setFontDesc(const FXFontDesc& fontdesc){
  selected=fontdesc;
  }

const FXFontDesc& GMFontDialog::getFontDesc() const {
  return selected;
  }


void GMFontDialog::create() {
  FXDialogBox::create();
  listFontFamily();
  listFontStyle();
  listFontSize();
  }

// Preview
void GMFontDialog::previewFont(){
  FXFont *old;

  // Save old font
  old=previewfont;

  // Get new font
  previewfont=new FXFont(getApp(),selected);

  // Realize new font
  previewfont->create();

  // Set new font
  preview->setFont(previewfont);

  // Delete old font
  delete old;
  }


void GMFontDialog::listFontFamily(){
  FXFontDesc * fonts=NULL;
  FXuint numfonts,f;
  FXint lastitem=-1,selindex=-1;
  FXString face,family,pfamily;

  familylist->clearItems();
  if (FXFont::listFonts(fonts,numfonts,FXString::null,0,0,0,selected.encoding,selected.flags)) {
    for(f=0;f<numfonts;f++){
      family=FXString(fonts[f].face).before('[').trimEnd();
      if (pfamily==family && lastitem>=0) {
        familylist->setItemText(lastitem,fonts[f-1].face);
        lastitem=familylist->appendItem(fonts[f].face,NULL,(void*)(FXuval)fonts[f].flags);
        }
      else {
        lastitem=familylist->appendItem(family,NULL,(void*)(FXuval)fonts[f].flags);
        }
      pfamily.adopt(family);
      if(compare(selected.face,fonts[f].face)==0) selindex=f;
      }
    if(selindex==-1) selindex=0;
    if(0<familylist->getNumItems()){
      familylist->setCurrentItem(selindex);
      familylist->makeItemVisible(selindex);
      fxstrlcpy(selected.face,familylist->getItemText(selindex).text(),sizeof(selected.face));
      }
    FXFREE(&fonts);
    }
  familylist->sortItems();
  }

void GMFontDialog::listFontStyle(){
  FXFontDesc * fonts=NULL;
  FXint selindex=-1;
  FXuint numfonts,f;
  FXushort wi,ww,lastwi=0,lastww=0,sl,lastsl=0;
  const FXchar *wgt=NULL,*slt=NULL,*wid=NULL;
  stylelist->clearItems();
  if (FXFont::listFonts(fonts,numfonts,selected.face,0,0,0/*selected.weight,selected.slant,selected.setwidth*/,selected.encoding,selected.flags)) {
    for(f=0;f<numfonts;f++){
      ww=fonts[f].weight;
      sl=fonts[f].slant;
      wi=fonts[f].setwidth;
      if(ww!=lastww || sl!=lastsl || wi!=lastwi){
        if(wi!=lastwi){
          switch(wi){
            case FXFont::UltraCondensed : wid=tr("Ultra Condensed"); break;
            case FXFont::ExtraCondensed : wid=tr("Extra Condensed"); break;
            case FXFont::Condensed      : wid=tr("Condensed"); break;
            case FXFont::SemiCondensed  : wid=tr("Semi Condensed"); break;
            case FXFont::NonExpanded    : wid=NULL; break;
            case FXFont::SemiExpanded   : wid=tr("Semi Expanded"); break;
            case FXFont::Expanded       : wid=tr("Expanded"); break;
            case FXFont::ExtraExpanded  : wid=tr("Extra Expanded"); break;
            case FXFont::UltraExpanded  : wid=tr("Ultra Expanded"); break;
            default                     : wid=NULL; break;
            }
          lastwi=wi;
          }
        if(ww!=lastww){
          switch(ww){
            case FXFont::Thin       : wgt=tr("Thin"); break;
            case FXFont::ExtraLight : wgt=tr("Extra Light"); break;
            case FXFont::Light      : wgt=tr("Light"); break;
            case FXFont::Normal     : wgt=NULL; break;
            case FXFont::Medium     : wgt=tr("Medium"); break;
            case FXFont::DemiBold   : wgt=tr("Demibold"); break;
            case FXFont::Bold       : wgt=tr("Bold"); break;
            case FXFont::ExtraBold  : wgt=tr("Extra Bold"); break;
            case FXFont::Black      : wgt=tr("Heavy"); break;
            default                 : wgt=NULL; break;
            }
          lastww=ww;
          }
        if (sl!=lastsl) {
          switch(sl){
            case FXFont::ReverseOblique : slt=tr("Reverse Oblique"); break;
            case FXFont::ReverseItalic  : slt=tr("Reverse Italic"); break;
            case FXFont::Straight       : slt=NULL; break;
            case FXFont::Italic         : slt=tr("Italic"); break;
            case FXFont::Oblique        : slt=tr("Oblique"); break;
            default                     : slt=NULL; break;
            }
          lastsl=sl;
          }

        FXuint style=FXRGB((FXuchar)ww,(FXuchar)sl,(FXuchar)wi);
        if (wgt && slt && wid)
          stylelist->appendItem(FXString::value("%s %s %s",wgt,wid,slt),NULL,(void*)(FXuval)style);
        else if (wgt && slt)
          stylelist->appendItem(FXString::value("%s %s",wgt,slt),NULL,(void*)(FXuval)style);
        else if (wgt && wid)
          stylelist->appendItem(FXString::value("%s %s",wgt,wid),NULL,(void*)(FXuval)style);
        else if (wid && slt)
          stylelist->appendItem(FXString::value("%s %s",wid,slt),NULL,(void*)(FXuval)style);
        else if (slt)
          stylelist->appendItem(tr(slt),NULL,(void*)(FXuval)style);
        else if (wgt)
          stylelist->appendItem(tr(wgt),NULL,(void*)(FXuval)style);
        else if (wid)
          stylelist->appendItem(tr(wid),NULL,(void*)(FXuval)style);
        else
          stylelist->appendItem(tr("Normal"),NULL,(void*)(FXuval)style);

        if (ww==selected.weight && sl==selected.slant && wi==selected.setwidth) {
          selindex=stylelist->getNumItems()-1;
          }
        }
      }
    FXFREE(&fonts);
    if(selindex==-1) selindex=0;
    if(0<stylelist->getNumItems()){
      stylelist->setCurrentItem(selindex);
      FXuint style=(FXuint)(FXuval)stylelist->getItemData(selindex);
      selected.weight=FXREDVAL(style);
      selected.slant=FXGREENVAL(style);
      selected.setwidth=FXBLUEVAL(style);
      stylelist->makeItemVisible(selindex);
      //stylelist->sortItems();
      }
    }
  }


void GMFontDialog::listFontSize(){
  const FXuint sizeint[]={60,80,90,100,110,120,140,160,200,240,300,360,420,480,640,720};
  FXFontDesc *fonts;
  FXuint numfonts,f,s,lasts;
  FXint selindex=-1;
  sizelist->clearItems();
  sizefield->setText(FXString::null);
  FXString string;
  if(FXFont::listFonts(fonts,numfonts,selected.face,selected.weight,selected.slant,selected.setwidth,selected.encoding,selected.flags)){
    FXASSERT(0<numfonts);
    lasts=0;
    if(fonts[0].flags&FXFont::Scalable){
      FXuint style=sizelist->getListStyle();
      style&=~LIST_BROWSESELECT;
      style|=LIST_SINGLESELECT;
      sizelist->setListStyle(style);
      for(f=0; f<ARRAYNUMBER(sizeint); f++){
        s=sizeint[f];
        string.format("%g",0.1*s);
        sizelist->appendItem(string,NULL,(void*)(FXuval)s);
        if(selected.size == s) selindex=sizelist->getNumItems()-1;
        }
      }
    else{
      FXuint style=sizelist->getListStyle();
      style&=~LIST_SINGLESELECT;
      style|=LIST_BROWSESELECT;
      sizelist->setListStyle(style);
      for(f=0; f<numfonts; f++){
        s=fonts[f].size;
        if(s!=lasts){
          string.format("%.1f",0.1*s);
          sizelist->appendItem(string,NULL,(void*)(FXuval)s);
          if(selected.size==s) selindex=sizelist->getNumItems()-1;
          lasts=s;
          }
        }
      }
    if(selindex==-1) selindex=0;
    if(0<sizelist->getNumItems()){
      sizelist->selectItem(selindex);
      sizelist->makeItemVisible(selindex);
      sizefield->setText(sizelist->getItemText(selindex));
      selected.size=(FXuint)(FXuval)sizelist->getItemData(selindex);
      }
    FXFREE(&fonts);
    }
  }

long GMFontDialog::onCmdFamily(FXObject*,FXSelector,void* ptr){
  fxstrlcpy(selected.face,familylist->getItemText((FXint)(FXival)ptr).text(),sizeof(selected.face));
  listFontStyle();
  listFontSize();
  previewFont();
  return 1;
  }

long GMFontDialog::onCmdStyle(FXObject*,FXSelector,void* ptr){
  FXuint style=(FXuint)(FXuval)stylelist->getItemData((FXint)(FXival)ptr);
  selected.weight=FXREDVAL(style);
  selected.slant=FXGREENVAL(style);
  selected.setwidth=FXBLUEVAL(style);
  listFontSize();
  previewFont();
  return 1;
  }

long GMFontDialog::onCmdSize(FXObject*,FXSelector,void*ptr){
  selected.size=(FXuint)(FXuval)sizelist->getItemData((FXint)(FXival)ptr);
  sizefield->setText(sizelist->getItemText((FXint)(FXival)ptr));
  previewFont();
  return 1;
  }
long GMFontDialog::onCmdSizeText(FXObject*,FXSelector,void*){
  selected.size=(FXuint)(10.0f*sizefield->getText().toFloat());
  if(selected.size<60) selected.size=60;
  if(selected.size>2400) selected.size=2400;
  previewFont();
  return 1;
  }


// Changed pitch
long GMFontDialog::onCmdPitch(FXObject*,FXSelector,void*ptr){
  FXint index=(FXint)(FXival)ptr;
  selected.flags&=~(FXFont::Fixed|FXFont::Variable);
  selected.flags|=(FXuint)(FXuval)pitchlist->getItemData(index);
  listFontFamily();
  listFontStyle();
  listFontSize();
  previewFont();
  return 1;
  }

long GMFontDialog::onUpdPitch(FXObject*sender,FXSelector,void*){
  FXint value=(selected.flags&FXFont::Fixed) ? 1 : (selected.flags&FXFont::Variable) ? 2 : 0;
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETINTVALUE),&value);
  return 1;
  }

// Changed pitch
long GMFontDialog::onCmdScalable(FXObject*,FXSelector,void*ptr){
  FXint index=(FXint)(FXival)ptr;
  selected.flags&=~(FXFont::Scalable);
  selected.flags|=(FXuint)(FXuval)scalelist->getItemData(index);
  listFontFamily();
  listFontStyle();
  listFontSize();
  previewFont();
  return 1;
  }

long GMFontDialog::onUpdScalable(FXObject*sender,FXSelector,void*){
  FXint value=(selected.flags&FXFont::Scalable) ? 1 : 0;
  sender->handle(this,FXSEL(SEL_COMMAND,ID_SETINTVALUE),&value);
  return 1;
  }
