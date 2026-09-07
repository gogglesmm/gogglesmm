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

int main(int argc,char * argv[]) {
  if (argc==2) {

    ap_init_crypto();

    HttpClient client;

    client.setAcceptEncoding(HttpClient::AcceptEncodingGZip);

    if (client.basic("GET",argv[1])) {
      FXString page = client.textBody();
      printf("%d\n",page.length());
      }

    ap_free_crypto();
    return 0;
    }

  return 1;
  }
