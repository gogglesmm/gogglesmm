/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2021 by Sander Jansen. All Rights Reserved      *
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
#include "icons.h"
#include "gmdefs.h"
#include <FXPNGIcon.h>
#include <FXPNGImage.h>
#include "GMTrack.h"
#include "GMList.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMCover.h"
#include "GMCoverManager.h"
#include "GMWindow.h"
#include "GMRemote.h"
#include "GMIconTheme.h"

// Map
FXDEFMAP(GMRemote) GMRemoteMap[]={
  FXMAPFUNC(SEL_COMMAND,						GMRemote::ID_VOLUME_SLIDER,			GMRemote::onCmdVolume),
  FXMAPFUNC(SEL_CHANGED,						GMRemote::ID_VOLUME_SLIDER,			GMRemote::onCmdVolume),
  FXMAPFUNC(SEL_MOUSEWHEEL,					GMRemote::ID_VOLUME_BUTTON,     GMRemote::onCmdVolumeButton),
  FXMAPFUNC(SEL_MOUSEWHEEL,					0,															GMRemote::onCmdVolumeButton),
  FXMAPFUNC(SEL_COMMAND,        		GMRemote::ID_TIMESLIDER,        GMRemote::onCmdTimeSlider),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS, 		GMRemote::ID_TIME_LABEL,        GMRemote::onCmdSetTimeLabelDirection),
  FXMAPFUNC(SEL_RIGHTBUTTONPRESS, 	GMRemote::ID_TIME_LABEL,        GMRemote::onCmdSetTimeLabelDirection),
  };

// Implementation
FXIMPLEMENT(GMRemote,FXMainWindow,GMRemoteMap,ARRAYNUMBER(GMRemoteMap))

GMRemote::GMRemote(FXApp* a,FXObject * tgt,FXSelector msg):FXMainWindow(a,"Goggles Music Manager",nullptr,nullptr,DECOR_BORDER|DECOR_TITLE|DECOR_CLOSE|DECOR_STRETCHABLE,0,0,0,0,3,3,3,3,3,3){
  flags|=FLAG_ENABLED;
  cover=nullptr;

  setTarget(tgt);
  setSelector(msg);

  setIcon(GMIconTheme::instance()->icon_applogo);
  setMiniIcon(GMIconTheme::instance()->icon_applogo_small);

  FXFontDesc fontdescription = getApp()->getNormalFont()->getFontDesc();
  fontdescription.weight = FXFont::Bold;
  fontdescription.size  += 10;
  font_title = new FXFont(getApp(),fontdescription);
  font_title->create();

  coversize = getApp()->reg().readUIntEntry("window","remote-cover-size",coversize);

  img_default = new FXPNGImage(getApp(),about_png);
  img_default->scale(coversize,coversize,FOX_SCALE_BEST);
  img_default->blend(getApp()->getBackColor());
  img_default->create();

  cover_label = new FXImageFrame(this,img_default,LAYOUT_SIDE_LEFT|FRAME_SUNKEN|LAYOUT_FIX_WIDTH|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_Y,0,0,coversize,coversize);
  cover_label->setBackColor(getApp()->getBackColor());

  /// Popup Volume Menu
  volumecontrol = new FXPopup(this,POPUP_VERTICAL|FRAME_RAISED|FRAME_THICK|POPUP_SHRINKWRAP);
  volumeslider = new FXSlider(volumecontrol,this,GMRemote::ID_VOLUME_SLIDER,LAYOUT_FIX_HEIGHT|LAYOUT_FIX_WIDTH|SLIDER_VERTICAL|SLIDER_TICKS_RIGHT|SLIDER_TICKS_LEFT|SLIDER_INSIDE_BAR,0,0,20,100);
  volumeslider->setTickDelta(10);
  volumeslider->setRange(0,100);
  volumeslider->setIncrement(10);

  FXHorizontalFrame * buttons = new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0,3,3,0,0);
  new FXButton(buttons,tr("\tShow Browser\tShow Browser"),GMIconTheme::instance()->icon_home,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_SHOW_BROWSER,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT);
  new FXVerticalSeparator(buttons,LAYOUT_FILL_Y|SEPARATOR_GROOVE);
  new FXButton(buttons,tr("\tStart Playback\tStart Playback"),GMIconTheme::instance()->icon_play,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_PLAYPAUSEMENU,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT);
  new FXButton(buttons,tr("\tStop Playback\tStop Playback"),GMIconTheme::instance()->icon_stop,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_STOP,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT);
  new FXVerticalSeparator(buttons,LAYOUT_FILL_Y|SEPARATOR_GROOVE);
  new FXButton(buttons,tr("\tPlay Previous Track\tPlay previous track."),GMIconTheme::instance()->icon_prev,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_PREV,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT);
  new FXButton(buttons,tr("\tPlay Next Track\tPlay next track."),GMIconTheme::instance()->icon_next,GMPlayerManager::instance()->getMainWindow(),GMWindow::ID_NEXT,BUTTON_TOOLBAR|FRAME_RAISED|ICON_ABOVE_TEXT);
  new FXVerticalSeparator(buttons,LAYOUT_FILL_Y|SEPARATOR_GROOVE);
  time_label =new FX7Segment(buttons,"--:--",SEVENSEGMENT_SHADOW|LAYOUT_CENTER_Y);
  time_label->setCellWidth(10);
  time_label->setCellHeight(15);
  time_label->setTarget(this);
  time_label->setSelector(ID_TIME_LABEL);
  new FXVerticalSeparator(buttons,LAYOUT_FILL_Y|SEPARATOR_GROOVE);
  volumebutton = new FXMenuButton(buttons,tr("\tAdjust Volume\tAdjust Volume"),nullptr,volumecontrol,MENUBUTTON_NOARROWS|MENUBUTTON_ATTACH_LEFT|MENUBUTTON_UP|MENUBUTTON_TOOLBAR|FRAME_RAISED|LAYOUT_CENTER_Y);
  volumebutton->setTarget(this);
  volumebutton->setSelector(ID_VOLUME_BUTTON);

  FXVerticalFrame * info = new FXVerticalFrame(this,LAYOUT_CENTER_Y|FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,2,2,2,2,0,0);
  title_label = new FXTextField(info,20,nullptr,0,FRAME_NONE|TEXTFIELD_READONLY,0,0,0,0,0,0,0,0);
  title_label->setBackColor(getApp()->getBaseColor());
  title_label->setFont(font_title);
  title_label->setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  title_label->setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
  title_label->disable();

  artistalbum_label = new FXTextField(info,30,nullptr,0,FRAME_NONE|TEXTFIELD_READONLY,0,0,0,0,0,0,0,0);
  artistalbum_label->setBackColor(getApp()->getBaseColor());
  artistalbum_label->setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));

  trackslider = new GMTrackProgressBar(info,this,ID_TIMESLIDER,LAYOUT_FILL_X|LAYOUT_CENTER_Y|FRAME_RAISED,0,0,0,0,0,0,0,0);
  trackslider->setTotal(100000);
  trackslider->setDefaultCursor(GMIconTheme::instance()->cursor_hand);
  trackslider->setDragCursor(GMIconTheme::instance()->cursor_hand);

  getAccelTable()->addAccel(parseAccel("F11"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_SHOW_BROWSER));
  getAccelTable()->addAccel(parseAccel("Ctrl-M"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_SHOW_BROWSER));
  getAccelTable()->addAccel(parseAccel("Ctrl-W"),this,FXSEL(SEL_CLOSE,0));
  getAccelTable()->addAccel(parseAccel("Ctrl-Q"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_QUIT));

  getAccelTable()->addAccel(parseAccel("Ctrl-P"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_PLAYPAUSEMENU));
  getAccelTable()->addAccel(parseAccel("Ctrl-\\"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_STOP));
  getAccelTable()->addAccel(parseAccel("Ctrl-["),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_PREV));
  getAccelTable()->addAccel(parseAccel("Ctrl-]"),GMPlayerManager::instance()->getMainWindow(),FXSEL(SEL_COMMAND,GMWindow::ID_NEXT));

  reset();
  }

// Destroy main window
GMRemote::~GMRemote(){
  volumeslider->setTarget(nullptr);
  volumeslider->setSelector(0);
  volumebutton->setMenu(nullptr);

  if (cover) {
    delete cover;
    cover=nullptr;
    }
  updateCover();
  }

void GMRemote::writeRegistry(){
  if (shown()) {
    getApp()->reg().writeIntEntry("window","remote-x",getX());
    getApp()->reg().writeIntEntry("window","remote-y",getY());
    getApp()->reg().writeIntEntry("window","remote-width",getWidth());
    getApp()->reg().writeIntEntry("window","remote-height",getHeight());
    getApp()->reg().writeBoolEntry("window","remote-time-remaining",is_remaining);
    }
  }

void GMRemote::updateCover() {
  if (cover==nullptr) {
    cover_label->setImage(img_default);
    }
  else {
    cover->create();
    cover_label->setImage(cover);
    }
  }


void GMRemote::display(const GMTrack & track){
  FXString tip = FXString::value("%s\n%s\n%s (%d)",track.title.text(),track.artist.text(),track.album.text(),track.year);

  title_label->setText(track.title);
  title_label->setJustify(JUSTIFY_LEFT);
  title_label->setCursorPos(0);
  title_label->setAnchorPos(0);
  title_label->makePositionVisible(0);
  title_label->setTipText(tip);

  artistalbum_label->setText("by " + track.artist + " from " + track.album);
  artistalbum_label->show();
  artistalbum_label->setCursorPos(0);
  artistalbum_label->setAnchorPos(0);
  artistalbum_label->makePositionVisible(0);
  artistalbum_label->setTipText(tip);
  recalc();
  }

void GMRemote::reset(){
  title_label->setText("Goggles Music Manager");
  title_label->setJustify(JUSTIFY_CENTER_X);
  title_label->setLayoutHints(LAYOUT_CENTER_Y|LAYOUT_FILL_X);
  title_label->setTipText(FXString::null);
  artistalbum_label->setText(FXString::null);
  artistalbum_label->setTipText(FXString::null);
  artistalbum_label->hide();
  time_label->setText("--:--");

  trackslider->disable();
  trackslider->setProgress(0);

  if (cover) {
    delete cover;
    cover=nullptr;
    }
  updateCover();

  recalc();
  layout();
  }

void GMRemote::update_time(const TrackTime & c,const TrackTime & r,FXint progress,FXbool playing,FXbool seekable){
  if (playing) {
    if (is_remaining == false) {
      if (c.hours>0)
        time_label->setText(FXString::value("%d:%.2d:%.2d",c.hours,c.minutes,c.seconds));
      else
        time_label->setText(FXString::value("%.2d:%.2d",c.minutes,c.seconds));
      }
    else {
      if (c.hours>0)
        time_label->setText(FXString::value("-%d:%.2d:%.2d",r.hours,r.minutes,r.seconds));
      else
        time_label->setText(FXString::value("-%.2d:%.2d",r.minutes,r.seconds));
      }
    if (seekable) {
      if (!trackslider->grabbed()){
        trackslider->setProgress(progress);
        }
      trackslider->enable();
      }
    else {
      trackslider->disable();
      }
    }
  else {
    time_label->setText("--:--");
    trackslider->disable();
    trackslider->setProgress(0);
    }
  }

void GMRemote::update_volume_display(FXint level) {
  if (level<0) {
    volumebutton->setIcon(GMIconTheme::instance()->icon_volume_muted);
    volumebutton->disable();
    }
  else {
    if (level<=0)
      volumebutton->setIcon(GMIconTheme::instance()->icon_volume_muted);
    else if (level<=33)
      volumebutton->setIcon(GMIconTheme::instance()->icon_volume_low);
    else if (level<=66)
      volumebutton->setIcon(GMIconTheme::instance()->icon_volume_medium);
    else
      volumebutton->setIcon(GMIconTheme::instance()->icon_volume_high);

    volumebutton->enable();
    }

  if (!volumeslider->grabbed())
    volumeslider->setValue(level);
  }

void GMRemote::update_cover_display() {
  if (cover) {
    delete cover;
    cover=nullptr;
    }
  cover = GMCover::copyToImage(GMPlayerManager::instance()->getCoverManager()->getCover(),coversize);
  updateCover();
  }


// Create and show window
void GMRemote::create(){
  FXMainWindow::create();

  if (getApp()->reg().readIntEntry("window","remote-x",-1)!=-1) {
    FXint xx=getApp()->reg().readIntEntry("window","remote-x",getX());
    FXint yy=getApp()->reg().readIntEntry("window","remote-y",getY());
    if (getApp()->reg().readIntEntry("window","remote-width",-1)!=-1) {
      FXint ww=getApp()->reg().readIntEntry("window","remote-width",getDefaultWidth());
      FXint hh=getApp()->reg().readIntEntry("window","remote-height",getDefaultHeight());
      position(xx,yy,ww,hh);
      }
    else {
      move(xx,yy);
      }
    }
  else {
    place(PLACEMENT_SCREEN);
    }
  is_remaining = getApp()->reg().readBoolEntry("window","remote-time-remaining",false);
  gm_set_application_icon(this);
  }

bool GMRemote::doesOverrideRedirect() const { return false; }


long GMRemote::onCmdVolume(FXObject*,FXSelector,void*ptr){
  FXint level = (FXint)(FXival)ptr;
  GMPlayerManager::instance()->volume(level);
  GMPlayerManager::instance()->getMainWindow()->update_volume_display(level);
  update_volume_display(level);
  return 1;
  }

long GMRemote::onCmdVolumeButton(FXObject*,FXSelector sel,void*ptr){
  volumeslider->handle(this,FXSEL(FXSELTYPE(sel),0),ptr);
  return 1;
  }

long GMRemote::onCmdTimeSlider(FXObject*,FXSelector,void*ptr){
  FXdouble pos = *(FXdouble*)ptr;
  GMPlayerManager::instance()->seek(pos);
  return 1;
  }

long GMRemote::onCmdSetTimeLabelDirection(FXObject*,FXSelector,void*){
  is_remaining = !is_remaining;
  return 1;
}
