#!/usr/bin/env python2
import sqlite3
import urlparse
import urllib
import os.path
import datetime

banshee   = sqlite3.connect(os.path.expanduser('~/.config/banshee-1/banshee.db'))
gogglesmm = sqlite3.connect(os.path.expanduser('~/.local/share/gogglesmm/goggles.db'))
bc = banshee.cursor()
gc = gogglesmm.cursor()

bc.execute('SELECT Uri, PlayCount,SkipCount, Rating, Score, LastPlayedStamp FROM CoreTracks')
for row in bc:
  rating    = row[3] * 51
  playcount = row[1]
  url       = urlparse.urlsplit(row[0])
  if url.scheme == 'file' and (rating>0 or playcount>0) :

    # Get path and filename
    path = urllib.unquote(url.path)
    dir,file = os.path.split(path)

    # Get lastplayed date
    if row[5]:  
      playdate    = long(row[5])
      playdate_ns = playdate*1000000000  
    else:
      playdate    = 0
      playdate_ns = 0

    # find track in gogglesmm
    args = (dir,file,)
    gc.execute('SELECT tracks.id FROM tracks,pathlist WHERE tracks.path == pathlist.id AND pathlist.name = ? AND tracks.mrl = ?',args)
    id = gc.fetchone()

    # Update track in gogglesmm if found 
    if id:
      print 'Update playcount %d, rating %d, playdate %s for \'%s/%s\'' % (playcount,row[3],datetime.datetime.fromtimestamp(playdate),dir,file)
      args = (playcount,rating,playdate_ns,id[0],)
      gc.execute('UPDATE tracks SET playcount = playcount + ?, rating = ?, playdate = max(playdate,?) WHERE tracks.id == ?',args)
      gogglesmm.commit()
    else:
      print 'Not found in database %s/%s' % (dir,file)

gogglesmm.close();
banshee.close();
