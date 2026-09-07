/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2026 by Sander Jansen. All Rights Reserved      *
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
*                               ---                                            *
* SPDX-License-Identifier: GPL-3.0-or-later                                    *
********************************************************************************/
#include <fx.h>

typedef FXArray<FXString> FXStringList;
#include <FXTextCodec.h>
#include <ap.h>


FXbool gm_parse_iso8601(const FXString & str, FXTime & timestamp) {
  /// 1 second expresed in nanoseconds
  const FXlong seconds = 1000000000;
  FXint year,month,day,hour,minute,second;
  if (str.scan("%d-%d-%dT%d:%d:%dZ",&year,&month,&day,&hour,&minute,&second)==6) {
    timestamp  = FXDate(year,month,day).getTime();
    timestamp += seconds * ((hour*3600)+(minute*60)+(second));
    return true;
    }
  return false;
  }


void check_lastversion() {
  FXString version;
  FXTime date;

  HttpClient client;
  client.setAcceptEncoding(HttpClient::AcceptEncodingGZip);
  if (client.basic("GET","https://api.github.com/repos/gogglesmm/gogglesmm/releases/latest","User-Agent: gogglesmm/1.1\r\n")) {
    FXString data = client.textBody();
    FXVariant info;
    FXJSON json(data.text(),data.length());
    if (json.load(info)==FXJSON::ErrOK) {
      version = info["tag_name"];
      if (!gm_parse_iso8601(info["published_at"],date))
        return;
      fxmessage("gogglesmm version: %s\ndate: %s\n%s\n",version.text(),info["published_at"].asChars(),FXSystem::localTime(date).text());
      }
    }
  }


int main(int /*argc*/,char ** /*argv*/) {
  ap_init_crypto();
  check_lastversion();
  ap_free_crypto();
  return 1;
  }
