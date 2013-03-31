/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2010 by Sander Jansen. All Rights Reserved      *
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
#include "GMDatabase.h"
#include <sqlite3.h>

/// 100ms
#define DATABASE_SLEEP 100000000

GMQuery::GMQuery() : statement(NULL) {
  }

GMQuery::GMQuery(sqlite3_stmt * s) : statement(s) {
  }

GMQuery::GMQuery(GMDatabase * database,const FXchar *query) : statement(NULL) {
  FXASSERT(database);
  statement=database->compile(query);
  }

GMQuery& GMQuery::operator=(sqlite3_stmt* s) {
  clear();
  statement=s;
  return *this;
  }

GMQuery::~GMQuery(){
  clear();
  }


void GMQuery::reset() {
  sqlite3_reset(statement);
  }

void GMQuery::clear(){
  sqlite3_finalize(statement);
  statement=NULL;
  }

void GMQuery::fatal() const {
  if (statement) {
#ifdef DEBUG
    fxerror("\tError: %s\n\tStatement:\"%s\"\n\n",sqlite3_errmsg(sqlite3_db_handle(statement)),sqlite3_sql(statement));
#else
    fxwarning("\tError: %s\n\tStatement:\"%s\"\n\n",sqlite3_errmsg(sqlite3_db_handle(statement)),sqlite3_sql(statement));
#endif
    }
  throw GMDatabaseException();
  }

/// Next row
FXbool GMQuery::row() {
  FXASSERT(statement);
  register FXint result;
  do {
    result = sqlite3_step(statement);
    switch(result) {
      case SQLITE_ROW   : return true;                            break;
      case SQLITE_DONE  : sqlite3_reset(statement); return false; break;
      case SQLITE_BUSY  : FXThread::sleep(DATABASE_SLEEP);        break;
      default           : fatal();                                break;
      }
    }
  while(1);
  return false;
  }
#if 0
/// Run
void GMQuery::runAll() {
  FXASSERT(statement);
  register FXint result;
  do {
    result = sqlite3_step(statement);
    switch(result) {
      case SQLITE_ROW   : continue;                         break;
      case SQLITE_DONE  : sqlite3_reset(statement); return; break;
      case SQLITE_BUSY  : FXThread::sleep(DATABASE_SLEEP);  break;
      default           : fatal();                          break;
      }
    }
  while(1);
  }
#endif

/// Run
void GMQuery::execute() {
  FXASSERT(statement);
  register FXint result;
  do {
    result = sqlite3_step(statement);
    switch(result) {
      case SQLITE_DONE  : sqlite3_reset(statement);   return; break;
      case SQLITE_BUSY  : FXThread::sleep(DATABASE_SLEEP);    break;
      default           : fatal();                            break;
      }
    }
  while(1);
  }



void GMQuery::execute(FXint & out){
  if (__likely(row()==true))
    get(0,out);
  sqlite3_reset(statement);
  }

void GMQuery::execute(FXString & out){
  if (__likely(row()==true))
    get(0,out);
  sqlite3_reset(statement);
  }


void GMQuery::update(const FXint in){
  set(0,in);
  execute();
  }

void GMQuery::update(const FXString & in){
  set(0,in);
  execute();
  }

FXint GMQuery::insert(const FXString & in){
  set(0,in);
  execute();
  return sqlite3_last_insert_rowid(sqlite3_db_handle(statement));
  }

FXint GMQuery::insert(){
  execute();
  return sqlite3_last_insert_rowid(sqlite3_db_handle(statement));
  }


void GMQuery::execute(const FXint in,FXint & out){
  set(0,in);
  if (__likely(row()==true))
    get(0,out);
  sqlite3_reset(statement);
  }

void GMQuery::execute(const FXint in,FXString & out){
  set(0,in);
  if (__likely(row()==true))
    get(0,out);
  sqlite3_reset(statement);
  }

void GMQuery::execute(const FXString & in,FXint & out){
  set(0,in);
  if (__likely(row()==true))
    get(0,out);
  sqlite3_reset(statement);
  }




#if 0
FXbool GMQuery::execute_with_result(FXint & value) {
  FXASSERT(statement);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement); return false; break;
    case SQLITE_ROW   : get(0,value); reset(); return true; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:
    default					  :  gm_print_backtrace(statement,"bug: database was locked"); reset(); throw GMDatabaseException(); break;
    }
  return false;
  }

FXbool GMQuery::execute_with_result(FXString & value) {
  FXASSERT(statement);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement);  return false; break;
    case SQLITE_ROW   : get(0,value); reset(); return true; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:
    default					  : gm_print_backtrace(statement,"bug: database was locked"); reset(); throw GMDatabaseException(); break;
    }
  return false;
  }


FXbool GMQuery::execute_simple(FXint in,FXString & out) {
  FXASSERT(statement);
  set(0,in);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement); return false; break;
    case SQLITE_ROW   : get(0,out); reset(); return true; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:

    default					  : gm_print_backtrace(statement,"bug: database was locked"); reset(); throw GMDatabaseException(); break;
    }
  return false;
  }

FXbool GMQuery::execute_simple(FXint in,FXint & out) {
  FXASSERT(statement);
  set(0,in);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement);return false; break;
    case SQLITE_ROW   : get(0,out); sqlite3_reset(statement); return true; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:

    default					  : gm_print_backtrace(statement,"bug: database was locked"); reset(); throw GMDatabaseException(); break;
    }
  return false;
  }



FXbool GMQuery::execute_simple(const FXString & in,FXint & out) {
  FXASSERT(statement);
  set(0,in);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement); return false; break;
    case SQLITE_ROW   : get(0,out); sqlite3_reset(statement); return true; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:

    default					  : gm_print_backtrace(statement,"bug: database was locked"); reset(); throw GMDatabaseException(); break;
    }
  return false;
  }


FXbool GMQuery::execute_simple(FXint in) {
  FXASSERT(statement);
  set(0,in);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement); return true; break;
    case SQLITE_ROW   : reset(); FXASSERT(0); return false; break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:

    default					  : gm_print_backtrace(statement,"bug: database was locked");
                        reset();FXASSERT(0); throw GMDatabaseException(); break;
    }
  return false;
  }

void GMQuery::execute_simple(const FXString & in) {
  FXASSERT(statement);
  set(0,in);
  FXint result = sqlite3_step(statement);
  switch(result) {
    case SQLITE_DONE  : sqlite3_reset(statement); break;
    case SQLITE_BUSY  :
    case SQLITE_LOCKED:
    default           : gm_print_backtrace(statement,"bug: database was locked");
                        throw GMDatabaseException();
    }
  }

#endif




void GMQuery::set(FXint p,FXint v){
  if (__unlikely(sqlite3_bind_int(statement,(p+1),v)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,FXuint v){
  if (__unlikely(sqlite3_bind_int(statement,(p+1),(FXint)v)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,FXlong v){
  if (__unlikely(sqlite3_bind_int64(statement,(p+1),v)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,FXfloat v){
  if (__unlikely(sqlite3_bind_double(statement,(p+1),v)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,FXdouble v){
  if (__unlikely(sqlite3_bind_double(statement,(p+1),v)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,const FXString & text) {
  if (__unlikely(sqlite3_bind_text(statement,(p+1),text.text(),-1,SQLITE_TRANSIENT)!=SQLITE_OK))
    fatal();
  }
void GMQuery::set(FXint p,const void * data,FXuval size){
  if (__unlikely(sqlite3_bind_blob(statement,(p+1),data,size,SQLITE_TRANSIENT)!=SQLITE_OK))
    fatal();
  }

void GMQuery::get(FXint p,FXint & v){
  FXASSERT(sqlite3_data_count(statement));
  v = sqlite3_column_int(statement,p);
  }

void GMQuery::get(FXint p,FXuint & v){
  FXASSERT(sqlite3_data_count(statement));
  v = (FXuint) sqlite3_column_int(statement,p);
  }

void GMQuery::get(FXint p,FXlong & v){
  FXASSERT(sqlite3_data_count(statement));
  v = sqlite3_column_int64(statement,p);
  }

void GMQuery::get(FXint p,FXfloat & v){
  FXASSERT(sqlite3_data_count(statement));
  if (sqlite3_column_type(statement,p)==SQLITE_FLOAT)
    v=(FXfloat)sqlite3_column_double(statement,p);
  else
    v=NAN;
  }

void GMQuery::get(FXint p,FXdouble & v){
  FXASSERT(sqlite3_data_count(statement));
  if (sqlite3_column_type(statement,p)==SQLITE_FLOAT)
    v=sqlite3_column_double(statement,p);
  else
    v=NAN;
  }

void GMQuery::get(FXint p,FXString & v){
  FXASSERT(sqlite3_data_count(statement));
  v = (const FXchar*)sqlite3_column_text(statement,p);
  }

const FXchar * GMQuery::get(FXint p){
  FXASSERT(sqlite3_data_count(statement));
  return (const FXchar *) sqlite3_column_text(statement,p);
  }

void GMQuery::get(FXint p,FXuchar *& data,FXuval & size){
  FXASSERT(sqlite3_data_count(statement));
  data = (FXuchar*)sqlite3_column_blob(statement,p);
  size = sqlite3_column_bytes(statement,p);
  }

























FXMutex         GMDatabase::mutex;
FXCondition     GMDatabase::condition;
volatile FXbool GMDatabase::interrupt = false;


GMDatabase::GMDatabase() : db(NULL) {
  }

GMDatabase::~GMDatabase(){
  close();
  }

void GMDatabase::close(){
  sqlite3_close(db);
  db=NULL;
  }

void GMDatabase::fatal(const FXchar * query) const {
  if (db) {
    if (query) {
#ifdef DEBUG
      fxerror("Statement: %s\n\tError: %s\n\n",query,sqlite3_errmsg(db));
#else
      fxwarning("Statement: %s\n\tError: %s\n\n",query,sqlite3_errmsg(db));
#endif
      }
    else {
#ifdef DEBUG
      fxerror("\tError: %s\n\n",sqlite3_errmsg(db));
#else
      fxwarning("\tError: %s\n\n",sqlite3_errmsg(db));
#endif
      }
    }
  else {
#ifdef DEBUG
    fxerror("\tFatal Database Error\n\n");
#else
    fxwarning("\tFatal Database Error\n\n");
#endif
    }
  throw GMDatabaseException();
  }

sqlite3_stmt * GMDatabase::compile(const FXchar * query){
  FXASSERT(db);
  register FXint result;
  sqlite3_stmt * statement=NULL;
  do {
    result = sqlite3_prepare_v2(db,query,-1,&statement,NULL);
    if (__likely(result==SQLITE_OK))
      return statement;
    else if (result==SQLITE_BUSY)
      FXThread::sleep(DATABASE_SLEEP);
    else
      fatal(query);
    }
  while(1);
  return statement;
  }

FXbool GMDatabase::open(const FXString & filename){
  GM_DEBUG_PRINT("Open Database %s\n",filename.text());
  if (sqlite3_open_v2(filename.text(),&db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_FULLMUTEX,NULL)!=SQLITE_OK){
    sqlite3_close(db);
    db=NULL;
    return false;
    }
  return true;
  }


void GMDatabase::reset() {
  FXStringList tables;
  FXString table;
  GMQuery q(this,"SELECT name FROM sqlite_master WHERE type='table';");
  while(q.row()) {
    q.get(0,table);
    tables.append(table);
    }
  for (FXint i=0;i<tables.no();i++) {
    ("DROP TABLE IF EXISTS " + tables[i] + ";");
    }
  }

FXint GMDatabase::rowid() const {
  return sqlite3_last_insert_rowid(db);
  }

FXint GMDatabase::changes() const {
  return sqlite3_changes(db);
  }

void GMDatabase::execute(const FXchar * statement){
  GMQuery q(this,statement);
  q.execute();
  }

void GMDatabase::execute(const FXString & statement){
  GMQuery q(this,statement.text());
  q.execute();
  }


void GMDatabase::executeFormat(const FXchar * fmt,...) {
  FXString statement;
  va_list args;
  va_start(args,fmt);
  statement.vformat(fmt,args);
  va_end(args);
  GMQuery q(this,statement.text());
  q.execute();
  }

FXint GMDatabase::insert(GMQuery & q) {
  q.execute();
  return rowid();
  }

void GMDatabase::execute(const FXchar * statement,FXint & out){
  GMQuery q(this,statement);
  q.execute(out);
  }

void GMDatabase::execute(const FXchar * statement,const FXString & in,FXint & out){
  GMQuery q(this,statement);
  q.execute(in,out);
  }

void GMDatabase::execute(const FXchar * statement,const FXint in,FXString & out){
  GMQuery q(this,statement);
  q.execute(in,out);
  }


#if 0



/// Execute a simple query





/// Execute a simple query
void GMDatabase::execute_simple(const FXchar * statement,FXint & val){
  GMQuery q(compile(statement));
  q.execute_with_result(val);
  }


/// Execute a simple query
void GMDatabase::execute_simple(const FXchar * statement,FXint in,FXString & out){
  GMQuery q(compile(statement));
  q.execute_simple(in,out);
  }

FXint GMDatabase::execute_insert(GMQuery & q) {
  q.execute();
  q.reset();
  return sqlite3_last_insert_rowid(db);
  }
#endif



void GMDatabase::begin() {
  GM_DEBUG_PRINT("begin()\n");
  lock();
  execute("BEGIN TRANSACTION;");
  }

void GMDatabase::commit() {
  GM_DEBUG_PRINT("commit()\n");
  execute("COMMIT TRANSACTION;");
  unlock();
  }

void GMDatabase::rollback() {
  GM_DEBUG_PRINT("rollback()\n");
  execute("ROLLBACK TRANSACTION;");
  unlock();
  }

void GMDatabase::beginTask() {
  GM_DEBUG_PRINT("beginTask()\n");
  mutex.lock();
  execute("BEGIN IMMEDIATE TRANSACTION;");
  }

void GMDatabase::commitTask() {
  GM_DEBUG_PRINT("commitTask() %d\n",mutex.locked());
  execute("COMMIT TRANSACTION;");
  mutex.unlock();
  }

void GMDatabase::rollbackTask() {
  GM_DEBUG_PRINT("rollbackTask()\n");
  execute("ROLLBACK TRANSACTION;");
  mutex.unlock();
  }

void GMDatabase::waitTask() {
  GM_DEBUG_PRINT("waitTask()\n");
  execute("COMMIT TRANSACTION;");
  condition.wait(mutex);
  execute("BEGIN IMMEDIATE TRANSACTION;");
  }

void GMDatabase::lock() {
//  fxmessage("lock %d %d\n",FXThread::self()==NULL,mutex.locked());
  if (FXThread::self()==NULL) {
//    fxmessage("trylock %d\n",mutex.locked());
    if (!mutex.trylock()) {
      GM_DEBUG_PRINT("Failed to lock mutex\n");
      interrupt=true;
      mutex.lock();
      }
    }
  }

void GMDatabase::unlock() {
//  fxmessage("unlock %d\n",FXThread::self()==NULL);
  if (FXThread::self()==NULL) {
    if (interrupt) {
      interrupt=false;
      condition.signal();
      }
    mutex.unlock();
    }
  }

void GMDatabase::setVersion(FXint v){
  executeFormat("PRAGMA user_version=%d",v);
  }

FXint GMDatabase::getVersion(){
  FXint version=0;
  execute("PRAGMA user_version;",version);
  return version;
  }

FXbool GMDatabase::threadsafe() {
  if (sqlite3_threadsafe())
    return true;
  else
    return false;
  }
const FXchar * GMDatabase::version() {
  return sqlite3_libversion();
  }

