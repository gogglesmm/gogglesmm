/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
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
#include "GMPlayerManager.h"

/*
  Display Help/Version
  returns true if application may exit
  TODO expand to all possible options
*/
static bool gm_display_help(int argc,char * argv[]) {
  for (int i=1;i<argc;i++){
    if ( (comparecase(argv[i],"--help")==0) || (comparecase(argv[i],"-h")==0) ) {
      if (argc>0)
        fxmessage("Usage: %s [options]\n\n",FXPath::name(argv[0]).text());
      else
        fxmessage("Usage: gogglesmm [options]\n\n");

      fxmessage("General options:\n"
                " -h, --help            Display this help page\n"
                " -v, --version         Display version information\n"
                "     --tray            Start minimized to tray\n"
#ifdef HAVE_OPENGL
                "     --disable-opengl  Disables opengl based features\n"
#endif
                "\n"
                "Control running music manager:\n"
                "     --play            Start playback\n"
                "     --play-pause      Toggle pause / playback.\n"
                "     --pause           Pause playback\n"
                "     --previous        Play previous track\n"
                "     --next            Play next track\n"
                "     --stop            Stop playback\n"
                "     --raise           Try to raise the main window\n"
                "     --toggle-shown    Show or Hide the main window\n"
                "     --now-playing     Show now playing notification\n"
                "\n"
                );
      return true;
      }
    else if ( (comparecase(argv[i],"--version")==0) || (comparecase(argv[i],"-v")==0) ) {
      fxmessage("Goggles Music Manager %s\n",APPLICATION_VERSION_STRING);
      return true;
      }
    }
  return false;
  }
int main(int argc,char *argv[]){

  /// Check and make sure we're linked correctly to FOX
  /// If we're not linked correctly, we cannot obviously popup a dialog...
  if (fxversion[0]==1 && (fxversion[1]==6) ) { /// Test for Stable Version of FOX 1.6
    if (FOX_MAJOR!=fxversion[0] || FOX_MINOR!=fxversion[1]){
      fxwarning("FOX Header (v%d.%d.%d) / Library (v%d.%d.%d) mismatch!  -\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL,fxversion[0],fxversion[1],fxversion[2]);
      return 1;
      }
    }
  else if (fxversion[0]==1 && ( fxversion[1]==7)) { /// Test for Development version of FOX 1.7
    if (FOX_MAJOR!=fxversion[0] || FOX_MINOR!=fxversion[1] || FOX_LEVEL!=fxversion[2]) {
      fxwarning("FOX Header (v%d.%d.%d) / Library (v%d.%d.%d) mismatch!  -\n",FOX_MAJOR,FOX_MINOR,FOX_LEVEL,fxversion[0],fxversion[1],fxversion[2]);
      return 1;
      }
    }
  else {
    fxwarning("Goggles Music Manager linked to a unknown/unsupported version of the FOX Library (v%d.%d.%d)",fxversion[0],fxversion[1],fxversion[2]);
    return 1;
    }

  /// Display Help
  if (gm_display_help(argc,argv))
    return 0;

  /// Main Application
  GMPlayerManager gogglesmm;
  return gogglesmm.run(argc,argv);
  }
