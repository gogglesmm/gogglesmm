#!/usr/bin/env python2
import sqlite3
import urlparse
import urllib
import os.path
import datetime
import codecs

v12       = sqlite3.connect(os.path.expanduser('~/.goggles/goggles.db'))
gogglesmm = sqlite3.connect(os.path.expanduser('~/.local/share/gogglesmm/goggles.db'))


vc  = v12.cursor()
vct = v12.cursor()
vcp = v12.cursor()
gc  = gogglesmm.cursor()

# Look up path in old db
def lookup_path(path,mrl):
  if path>0:
    vcp.execute("SELECT name FROM directories WHERE (select path || id || '/' from directories WHERE id = ? ) like path || id || '/%' order by path;",(path,))
    components = []
    for p in vcp:
      components.append(p[0])
    path = os.path.join(*components)
    return path,mrl
  else:
    path,file = os.path.split(mrl)
    return path,file

# Transfer streams
vc.execute('SELECT url,description,genres.name,bitrate,rating FROM streams,genres WHERE streams.genre == genres.id')
for row in vc:
  
  # Insert Tag
  gc.execute('SELECT id FROM tags WHERE name == ?',(row[2],))
  id = gc.fetchone()  
  if not id:
    gc.execute('INSERT INTO tags VALUES ( NULL, ?)',(row[2],))
    tag = gc.lastrowid
  else:
    tag = id[0]

  # Insert Stream
  gc.execute('INSERT INTO streams VALUES ( NULL, ?, ?, ?, ? ,?)',(row[0],row[1],tag,row[3],row[4],))
  gogglesmm.commit()


# Export Play Lists
vc.execute('SELECT id, name FROM playlists')
for playlist in vc:

  filename = '%d_%s.m3u' % (playlist[0],playlist[1])

  print 'Exporting playlist %s to %s' % (playlist[1],filename)
  m3u = codecs.open(filename, "w", "utf-8")

  vct.execute('SELECT path, mrl FROM playlist_tracks, tracks WHERE playlist_tracks.track == tracks.id AND playlist_tracks.playlist == ? ORDER BY playlist_tracks.queue',(playlist[0],))
  for track in vct:
    path,file = lookup_path(track[0],track[1])
    m3u.write("%s\n" % os.path.join(path,file))    
    
  m3u.close()

gogglesmm.close();
v12.close();
