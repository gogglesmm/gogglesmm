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
#ifndef GMREMOTE_H
#define GMREMOTE_H

class GMWindow;

class GMRemote : public FXMainWindow {
FXDECLARE(GMRemote)
private:
  FXTextField * title_label = nullptr;
  FXTextField * artistalbum_label = nullptr;
  FX7Segment  * time_label = nullptr;
  FXImageFrame* cover_label = nullptr;
  FXPopupPtr    volumecontrol;
  FXMenuButton* volumebutton = nullptr;
  FXSlider    * volumeslider = nullptr;
  FXFontPtr     font_title;
  FXImagePtr    img_default;
  FXImage*      cover = nullptr;
  GMTrackProgressBar* trackslider = nullptr;
  bool          is_remaining = false;
protected:
  virtual bool doesOverrideRedirect() const;
  GMRemote() {}
  void updateCover();
public:
  long onCmdVolume(FXObject*,FXSelector,void*);
  long onCmdVolumeButton(FXObject*,FXSelector,void*);
  long onCmdTimeSlider(FXObject*,FXSelector,void*);
  long onCmdSetTimeLabelDirection(FXObject*,FXSelector,void*);
public:
  enum {
    ID_VOLUME_SLIDER = FXMainWindow::ID_LAST,
    ID_VOLUME_BUTTON,
    ID_TIMESLIDER,
    ID_TIME_LABEL,
    };
public:
  /// Construct Remote Window
  GMRemote(FXApp* a,FXObject*,FXSelector);

  // Update Display
  void display(const GMTrack & track);

  void update_time(const TrackTime & current,const TrackTime & remaining,FXint position,FXbool playing,FXbool seekable);

  void update_volume_display(FXint l);

  void update_cover_display();

  void reset();

  void writeRegistry();

  /// Create
  virtual void create();

  /// Destroy calculator
  virtual ~GMRemote();
  };
#endif
