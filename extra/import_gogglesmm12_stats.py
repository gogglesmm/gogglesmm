#!/usr/bin/env python2
import sqlite3
import urlparse
import urllib
import os.path
import datetime

v12       = sqlite3.connect(os.path.expanduser('~/.goggles/goggles.db'))
gogglesmm = sqlite3.connect(os.path.expanduser('~/.local/share/gogglesmm/goggles.db'))
vc = v12.cursor()
vcp = v12.cursor()
gc = gogglesmm.cursor()

vc.execute('SELECT path,mrl,playcount,playdate FROM tracks')
for row in vc:
  playcount = row[2]
  if playcount > 0:
    playdate  = row[3]
    path      = ""
    file			= row[1]

    if row[0]>0:
      vcp.execute("SELECT name FROM directories WHERE (select path || id || '/' from directories WHERE id = ? ) like path || id || '/%' order by path;",(row[0],))
      components = []
      for p in vcp:
        components.append(p[0])
      path = os.path.join(*components)
    else:
      path,file = os.path.split(mrl)

    gc.execute('SELECT tracks.id FROM tracks,pathlist WHERE tracks.path == pathlist.id AND pathlist.name = ? AND tracks.mrl = ?',(path,file,))
    id = gc.fetchone()
    if id:
      print 'Update playcount %d, playdate %s for \'%s/%s\'' % (playcount,datetime.datetime.fromtimestamp(playdate/1000000000),path,file)
      gc.execute('UPDATE tracks SET playcount = playcount + ?, playdate = max(playdate,?) WHERE tracks.id == ?',(playcount,playdate,id[0],))
      gogglesmm.commit()
    else:
      print 'Not found in database %s/%s' % (path,file)

gogglesmm.close();
v12.close();
