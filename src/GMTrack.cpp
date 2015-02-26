/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2010-2014 by Sander Jansen. All Rights Reserved      *
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
#include "GMTrack.h"
#include "GMTag.h"

GMTrack::GMTrack() :
  year(0),
  no(0),
  time(0),
  bitrate(0){
  }

void GMTrack::adopt(GMTrack & t) {
  url.adopt(t.url);
  title.adopt(t.title);
  artist.adopt(t.artist);
  album.adopt(t.album);
  album_artist.adopt(t.album_artist);
  composer.adopt(t.composer);
  conductor.adopt(t.conductor);
  tags.adopt(t.tags);
  year=t.year;
  no=t.no;
  time=t.time;
  bitrate=t.bitrate;
  }

void GMTrack::clear() {
  title.clear();
  artist.clear();
  album.clear();
  album_artist.clear();
  tags.clear();
  composer.clear();
  conductor.clear();
  year=0;
  no=0;
  time=0;
  bitrate=0;
  }


static void gmsplit(const FXString & in,FXStringList & output) {
  FXint s=0;
  FXint e=0;
  FXint n=0;
  const FXchar sep=',';

  while(s<in.length()) {

    // trim leading white space
    while(in[s]==' ') s++;

    e=s;

    // find end
    while(in[e]!=sep && in[e]!='\0') e++;

    n=e+1;
    e=e-1;

    // trim end
    while(e>=s && in[e]==' ') e--;

    if (e>=s) {
      output.no(output.no()+1);
      output[output.no()-1].assign(&in[s],(e-s)+1);
      }
    s=n;
    }
  }

void GMTrack::setTagsFromString(const FXString & str){
  tags.clear();
  gmsplit(str,tags);
  }


FXbool GMTrack::saveTag(const FXString & filename,FXuint /*opts=0*/) {
  GMFileTag filetags;

  if (!FXStat::isWritable(filename))
    return false;

  if (!filetags.open(filename,FILETAG_TAGS))
    return false;

  filetags.setTitle(title);
  filetags.setArtist(artist);
  filetags.setAlbum(album);
  filetags.setYear(year);
  filetags.setTrackNumber(getTrackNumber());
  filetags.setDiscNumber(getDiscNumber());
  filetags.setComposer(composer);
  filetags.setConductor(conductor);
  filetags.setTags(tags);
  if (album_artist!=artist && !album_artist.empty())
    filetags.setAlbumArtist(album_artist);
  else
    filetags.setAlbumArtist(FXString::null);

  return filetags.save();
  }


FXbool GMTrack::loadTag(const FXString & filename) {
//  GM_TICKS_START();
  GMFileTag filetags;

  if (!filetags.open(filename,FILETAG_TAGS|FILETAG_AUDIOPROPERTIES)){
    clear();
    return false;
    }

  url = filename;

  filetags.getTitle(title);
  filetags.getAlbum(album);
  filetags.getArtist(artist);
  filetags.getAlbumArtist(album_artist);
  filetags.getComposer(composer);
  filetags.getConductor(conductor);
  filetags.getTags(tags);

  year         = filetags.getYear();
  no           = filetags.getTrackNumber();

  time         = filetags.getTime();
  bitrate      = filetags.getBitRate();
  sampleformat = filetags.getSampleSize();
  samplerate   = filetags.getSampleRate();
  channels     = filetags.getChannels();
  filetype     = filetags.getFileType();


  setDiscNumber(filetags.getDiscNumber());
//  GM_TICKS_END();
  return true;
  }


FXbool GMTrack::loadProperties(const FXString & filename) {
  GMFileTag filetags;

  if (!filetags.open(filename,FILETAG_AUDIOPROPERTIES))
    return false;

  url          = filename;

  time         = filetags.getTime();
  bitrate      = filetags.getBitRate();
  sampleformat = filetags.getSampleSize();
  samplerate   = filetags.getSampleRate();
  channels     = filetags.getChannels();
  filetype     = filetags.getFileType();
  return true;
  }
