/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMFetch.h"
#include "GMPlayerManager.h"
#include <curl/curl.h>

GMFetch * GMFetch::fetch = NULL;

GMFetch::GMFetch() : gui(FXApp::instance()) {
  }

static size_t download_write(void *buffer, size_t size, size_t nmemb,void *ptr_response){
  FXString * response = reinterpret_cast<FXString*>(ptr_response);
  response->append((const FXchar *)buffer,size*nmemb);
  return size*nmemb;
  }


static int download_progress(void *clientp,double /*dltotal*/,double /*dlnow*/,double /*ultotal*/,double /*ulnow*/){
  GMFetch * fetch = reinterpret_cast<GMFetch*>(clientp);
  if (fetch->runstatus==false)
    return 1;
  else
    return 0;
  }


FXint GMFetch::run() {
  FXchar errorbuf[CURL_ERROR_SIZE];
  GMFetchResponse * response = new GMFetchResponse;
  response->url = url;
  CURL * handle = curl_easy_init();
  curl_easy_setopt(handle,CURLOPT_NOSIGNAL,1);
  curl_easy_setopt(handle,CURLOPT_URL,url.text());
  curl_easy_setopt(handle,CURLOPT_WRITEFUNCTION,download_write);
  curl_easy_setopt(handle,CURLOPT_WRITEDATA,&response->data);
  curl_easy_setopt(handle,CURLOPT_PROGRESSFUNCTION,download_progress);
  curl_easy_setopt(handle,CURLOPT_PROGRESSDATA,this);
  curl_easy_setopt(handle,CURLOPT_NOPROGRESS,0);
  curl_easy_setopt(handle,CURLOPT_FOLLOWLOCATION,1); /// Perhaps update the database with the new url
#ifdef DEBUG
  curl_easy_setopt(handle,CURLOPT_VERBOSE,1); /// Perhaps update the database with the new url
#endif
  curl_easy_setopt(handle,CURLOPT_ERRORBUFFER,errorbuf);
  runstatus=true;
  if (curl_easy_perform(handle)) {
    errormsg=errorbuf;
    gui.message(GMPlayerManager::instance(),FXSEL(SEL_COMMAND,GMPlayerManager::ID_DOWNLOAD_COMPLETE));
    }
  else {
    FXchar * ct;
    if (curl_easy_getinfo(handle,CURLINFO_CONTENT_TYPE,&ct)==CURLE_OK){
      response->content_type = ct;
      }
    gui.message(GMPlayerManager::instance(),FXSEL(SEL_COMMAND,GMPlayerManager::ID_DOWNLOAD_COMPLETE),&response,sizeof(GMFetchResponse*));
    }
  curl_easy_cleanup(handle);
  return 0;
  }

void GMFetch::init() {
  curl_global_init(CURL_GLOBAL_NOTHING);
  }

void GMFetch::exit() {
  if (fetch) {

    if (fetch->running())
      fetch->join();

    delete fetch;
    fetch=NULL;
    }
  curl_global_cleanup();
  }


void GMFetch::download(const FXString & url) {
  if (fetch==NULL) {
    fetch = new GMFetch();
    }
  else {
    if (fetch->running())
    fetch->join();
    }
  fetch->url = url;
  fetch->start();
  }

void GMFetch::cancel_and_wait(){
  if (fetch && fetch->running() ) {
    fetch->runstatus=false;
    fetch->join();
    }
  }

FXbool GMFetch::busy(){
  if (fetch && fetch->running() )
    return true;
  else
    return false;
  }
