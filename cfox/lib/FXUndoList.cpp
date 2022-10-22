/********************************************************************************
*                                                                               *
*                  U n d o / R e d o - a b l e   C o m m a n d                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or modify          *
* it under the terms of the GNU Lesser General Public License as published by   *
* the Free Software Foundation; either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU Lesser General Public License for more details.                           *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public License      *
* along with this program.  If not, see <http://www.gnu.org/licenses/>          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxmath.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXMutex.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXStringDictionary.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXEvent.h"
#include "FXWindow.h"
#include "FXApp.h"
#include "FXUndoList.h"

/*
  Notes:

  - When a command is undone, its moved to the redo list.

  - When a command is redone, its moved back to the undo list.

  - Whenever adding a new command, the redo list is deleted.

  - At any time, you can trim down the undo list down to a given
    maximum size or a given number of undo records.  This should
    keep the memory overhead within sensible bounds.

  - To keep track of when we get back to an "unmodified" state, a mark
    can be set.  The mark is basically a counter which is incremented
    with every undo record added, and decremented when undoing a command.
    When we get back to 0, we are back to the "unmodified" state.

    If, after setting the mark, we have called undo(), then the mark can be
    reached by calling redo().

    If the marked position is in the redo-list, then adding a new undo
    record will cause the redo-list to be deleted, and the marked position
    will become unreachable.

    The marked state may also become unreachable when the undo list is trimmed.

  - You can call also kill the redo list without adding a new command
    to the undo list, although this may cause the marked position to
    become unreachable.

  - We measure the size of the undo-records in the undo-list; when the
    records are moved to the redo-list, they usually contain different
    information!

  - Because we may need to know during execution of a command whether this
    was due to undoing or directly issued by the user, we keep a flag "working"
    which is true during a undo or redo operation.

  - Command groups are collections of smaller undo/redo records which are
    executed as a unit, so that large operations may be broken up into
    smaller units.  This allows written fewer, simpler undo/redo records
    for some basic operations where otherwise one would have to write
    many, complex undo/redo records.

  - Command groups may be arbitrarily nested.

  - FXCommand is now derived from FXObject; this means (1) FXCommands can
    send messages, and (2) can also receive messages.  It is hoped that
    this can be used to interface FXCommand directly with targets via
    message exchange, i.e. obviate the need to write glue code.
*/

#define NOMARK 2147483647       // No mark is set

using namespace FX;

/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT_ABSTRACT(FXCommand,FXObject,nullptr,0)


// Default implementation of undo name is just "Undo"
FXString FXCommand::undoName() const { return "Undo"; }


// Default implementation of redo name is just "Redo"
FXString FXCommand::redoName() const { return "Redo"; }


// Allow merging is false by default
FXbool FXCommand::canMerge() const { return false; }


// Don't merge by default
FXbool FXCommand::mergeWith(FXCommand*){ return false; }


// Default returns size of undo record itself
FXuint FXCommand::size() const { return sizeof(FXCommand); }


/*******************************************************************************/

// Object implementation
FXIMPLEMENT(FXCommandGroup,FXCommand,nullptr,0)


// Undoing a command group undoes each sub command
void FXCommandGroup::undo(){
  FXCommand *command;
  while(undolist){
    command=undolist;
    undolist=undolist->next;
    command->undo();
    command->next=redolist;
    redolist=command;
    }
  }


// Undoing a command group undoes each sub command
void FXCommandGroup::redo(){
  FXCommand *command;
  while(redolist){
    command=redolist;
    redolist=redolist->next;
    command->redo();
    command->next=undolist;
    undolist=command;
    }
  }


// Return the size of the information in the undo command group.
FXuint FXCommandGroup::size() const {
  FXuint result=sizeof(FXCommandGroup);
  FXCommand *command;
  for(command=undolist; command; command=command->next){
    result+=command->size();
    }
  for(command=redolist; command; command=command->next){
    result+=command->size();
    }
  return result;
  }


// Destrying the command group destroys the subcommands
FXCommandGroup::~FXCommandGroup(){
  FXCommand *command;
  while(redolist){
    command=redolist;
    redolist=redolist->next;
    delete command;
    }
  while(undolist){
    command=undolist;
    undolist=undolist->next;
    delete command;
    }
  delete group;
  }


/*******************************************************************************/

// Map
FXDEFMAP(FXUndoList) FXUndoListMap[]={
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_CLEAR,      FXUndoList::onCmdClear),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_CLEAR,      FXUndoList::onUpdClear),
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_REVERT,     FXUndoList::onCmdRevert),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_REVERT,     FXUndoList::onUpdRevert),
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_UNDO,       FXUndoList::onCmdUndo),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_UNDO,       FXUndoList::onUpdUndo),
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_REDO,       FXUndoList::onCmdRedo),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_REDO,       FXUndoList::onUpdRedo),
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_UNDO_ALL,   FXUndoList::onCmdUndoAll),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_UNDO_ALL,   FXUndoList::onUpdUndo),
  FXMAPFUNC(SEL_COMMAND, FXUndoList::ID_REDO_ALL,   FXUndoList::onCmdRedoAll),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_REDO_ALL,   FXUndoList::onUpdRedo),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_UNDO_COUNT, FXUndoList::onUpdUndoCount),
  FXMAPFUNC(SEL_UPDATE,  FXUndoList::ID_REDO_COUNT, FXUndoList::onUpdRedoCount),
  };


// Object implementation
FXIMPLEMENT(FXUndoList,FXCommandGroup,FXUndoListMap,ARRAYNUMBER(FXUndoListMap))



// Make new empty undo list
FXUndoList::FXUndoList(){
  undocount=0;
  redocount=0;
  marker=NOMARK;
  space=0;
  working=false;
  }


// Mark current state
void FXUndoList::mark(){
  marker=0;
  }


// Unmark undo list
void FXUndoList::unmark(){
  marker=NOMARK;
  }


// Check if marked
FXbool FXUndoList::marked() const {
  return (group==nullptr) && (marker==0);
  }


// Cut the redo list; can no longer revert to marked
// state if mark is inside the redo list.
void FXUndoList::cut(){
  FXCommand *command;
  if(marker<0) marker=NOMARK;
  while(redolist){
    command=redolist;
    redolist=redolist->next;
    delete command;
    }
  redolist=nullptr;
  redocount=0;
  }


// Add new command, executing if desired
void FXUndoList::add(FXCommand* command,FXbool doit,FXbool merge){
  FXCommandGroup* g=this;
  FXuint oldsize=0;

  // Must pass a command
  if(!command){ fxerror("FXCommandGroup::add: NULL command argument.\n"); }

  // Adding undo while in the middle of doing something!
  if(working){ fxerror("FXCommandGroup::add: already working on undo or redo.\n"); }

  working=true;

  // Cut redo list
  cut();

  // Execute command
  if(doit) command->redo();

  // Hunt for end of group chain
  while(g->group){ g=g->group; }

  // Old size of previous record
  if(g->undolist) oldsize=g->undolist->size();

  // Try to merge commands when desired and possible
  if(merge && g->undolist && !marked() && command->canMerge() && g->undolist->mergeWith(command)){

    // Account for merge
    if(this==g){

      // Update space, which is the new size less the old size
      space+=g->undolist->size()-oldsize;
      }

    // Delete incoming command that was merged
    delete command;
    }

  // Append new command to undo list
  else{

    // Append incoming command
    command->next=g->undolist;
    g->undolist=command;

    // Account for one more undo step
    if(this==g){

      // Update space, add the size of the new command
      space+=command->size();

      // Update marker and undo counter
      if(marker!=NOMARK) marker++;
      undocount++;
      }
    }

  FXTRACE((100,"FXUndoList::add: space=%d undocount=%d marker=%d\n",space,undocount,marker));

  working=false;
  }


// Begin a new undo command group
void FXUndoList::begin(FXCommandGroup *command){
  FXCommandGroup* g=this;

  // Must pass a command group
  if(!command){ fxerror("FXCommandGroup::begin: NULL command argument.\n"); }

  // Calling begin while in the middle of doing something!
  if(working){ fxerror("FXCommandGroup::begin: already working on undo or redo.\n"); }

  // Cut redo list
  cut();

  // Hunt for end of group chain
  while(g->group){ g=g->group; }

  // Add to end
  g->group=command;
  }


// End undo command group
void FXUndoList::end(){
  FXCommandGroup *command;
  FXCommandGroup *g=this;

  // Must have called begin
  if(!g->group){ fxerror("FXCommandGroup::end: no matching call to begin.\n"); }

  // Calling end while in the middle of doing something!
  if(working){ fxerror("FXCommandGroup::end: already working on undo or redo.\n"); }

  // Hunt for one above end of group chain
  while(g->group->group){ g=g->group; }

  // Unlink from group chain
  command=g->group;
  g->group=nullptr;

  // Add to group if non-empty
  if(!command->empty()){

    // Append new command to undo list
    command->next=g->undolist;
    g->undolist=command;

    // Update marker and undo count
    if(this==g){

      // Update space of completed command group
      space+=command->size();

      // Update marker and undo counter
      if(marker!=NOMARK) marker++;
      undocount++;
      }
    }

  // Or delete if empty
  else{

    // Delete bottom group
    delete command;
    }
  }


// Abort undo command group
void FXUndoList::abort(){
  FXCommandGroup *g=this;

  // Must be called after begin
  if(!g->group){ fxerror("FXCommandGroup::abort: no matching call to begin.\n"); }

  // Calling abort while in the middle of doing something!
  if(working){ fxerror("FXCommandGroup::abort: already working on undo or redo.\n"); }

  // Hunt for one above end of group chain
  while(g->group->group){ g=g->group; }

  // Delete bottom group
  delete g->group;

  // New end of chain
  g->group=nullptr;
  }


// Undo last command
void FXUndoList::undo(){
  FXCommand *command;
  if(group){ fxerror("FXCommandGroup::undo: cannot call undo inside begin-end block.\n"); }
  if(undolist){
    working=true;
    command=undolist;                   // Remove from undolist BEFORE undo
    undolist=undolist->next;
    space-=command->size();		// Measure BEFORE undo!
    command->undo();
    command->next=redolist;             // Hang into redolist AFTER undo
    redolist=command;
    undocount--;
    redocount++;
    if(marker!=NOMARK) marker--;
    FXTRACE((100,"FXUndoList::undo: space=%d undocount=%d redocount=%d marker=%d\n",space,undocount,redocount,marker));
    working=false;
    }
  }


// Redo next command
void FXUndoList::redo(){
  FXCommand *command;
  if(group){ fxerror("FXCommandGroup::redo: cannot call undo inside begin-end block.\n"); }
  if(redolist){
    working=true;
    command=redolist;                   // Remove from redolist BEFORE redo
    redolist=redolist->next;
    command->redo();
    space+=command->size();		// Measure AFTER redo!
    command->next=undolist;             // Hang into undolist AFTER redo
    undolist=command;
    undocount++;
    redocount--;
    if(marker!=NOMARK) marker++;
    FXTRACE((100,"FXUndoList::redo: space=%d undocount=%d redocount=%d marker=%d\n",space,undocount,redocount,marker));
    working=false;
    }
  }


// Undo all commands
void FXUndoList::undoAll(){
  while(canUndo()) undo();
  }


// Redo all commands
void FXUndoList::redoAll(){
  while(canRedo()) redo();
  }


// Revert to marked
void FXUndoList::revert(){
  if(marker!=NOMARK){
    while(marker>0) undo();
    while(marker<0) redo();
    }
  }


// Can we undo more commands
FXbool FXUndoList::canUndo() const {
  return undolist!=nullptr;
  }


// Can we redo more commands
FXbool FXUndoList::canRedo() const {
  return redolist!=nullptr;
  }


// Can revert to marked
FXbool FXUndoList::canRevert() const {
  return marker!=NOMARK && marker!=0;
  }


// Return name of the first undo command available, if any
FXString FXUndoList::undoName() const {
  if(undolist) return undolist->undoName();
  return FXString::null;
  }


// Return name of the first redo command available, if any
FXString FXUndoList::redoName() const {
  if(redolist) return redolist->redoName();
  return FXString::null;
  }


// Clear list
void FXUndoList::clear(){
  FXCommand *command;
  FXTRACE((100,"FXUndoList::clear: space=%d undocount=%d redocount=%d marker=%d\n",space,undocount,redocount,marker));
  while(redolist){
    command=redolist;
    redolist=redolist->next;
    delete command;
    }
  while(undolist){
    command=undolist;
    undolist=undolist->next;
    delete command;
    }
  delete group;
  redolist=nullptr;
  undolist=nullptr;
  marker=NOMARK;
  undocount=0;
  redocount=0;
  group=nullptr;
  space=0;
  }


// Clear undo list
long FXUndoList::onCmdClear(FXObject*,FXSelector,void*){
  clear();
  return 1;
  }


// Update Clear undo list
long FXUndoList::onUpdClear(FXObject* sender,FXSelector,void*){
  sender->handle(this,(canUndo()||canRedo())?FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE):FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),nullptr);
  return 1;
  }


// Revert to marked
long FXUndoList::onCmdRevert(FXObject*,FXSelector,void*){
  revert();
  return 1;
  }


// Update revert to marked
long FXUndoList::onUpdRevert(FXObject* sender,FXSelector,void*){
  sender->handle(this,canRevert()?FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE):FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),nullptr);
  return 1;
  }


// Undo last command
long FXUndoList::onCmdUndo(FXObject*,FXSelector,void*){
  undo();
  return 1;
  }


// Undo all commands
long FXUndoList::onCmdUndoAll(FXObject*,FXSelector,void*){
  undoAll();
  return 1;
  }


// Update undo last command
long FXUndoList::onUpdUndo(FXObject* sender,FXSelector,void*){
  sender->handle(this,canUndo()?FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE):FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),nullptr);
  return 1;
  }


// Redo last command
long FXUndoList::onCmdRedo(FXObject*,FXSelector,void*){
  redo();
  return 1;
  }


// Redo all commands
long FXUndoList::onCmdRedoAll(FXObject*,FXSelector,void*){
  redoAll();
  return 1;
  }


// Update redo last command
long FXUndoList::onUpdRedo(FXObject* sender,FXSelector,void*){
  sender->handle(this,canRedo()?FXSEL(SEL_COMMAND,FXWindow::ID_ENABLE):FXSEL(SEL_COMMAND,FXWindow::ID_DISABLE),nullptr);
  return 1;
  }


// Update undo count
long FXUndoList::onUpdUndoCount(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETINTVALUE),(void*)&undocount);
  return 1;
  }


// Update redo count
long FXUndoList::onUpdRedoCount(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETINTVALUE),(void*)&redocount);
  return 1;
  }


// Size of undo information
FXuint FXUndoList::size() const {
  return space;
  }


// Trim undo list down to at most nc records
void FXUndoList::trimCount(FXint nc){
  FXTRACE((100,"FXUndoList::trimCount: was: space=%d undocount=%d; marker=%d ",space,undocount,marker));
  if(undocount>nc){
    FXCommand **pp=&undolist;
    FXCommand *p=*pp;
    FXint i=0;
    while(p && i<nc){
      pp=&p->next;
      p=*pp;
      i++;
      }
    while(*pp){
      p=*pp;
      *pp=p->next;
      space-=p->size();
      undocount--;
      delete p;
      }
    if(marker>undocount) marker=NOMARK;
    }
  FXTRACE((100,"now: space=%d undocount=%d; marker=%d\n",space,undocount,marker));
  }


// Trim undo list down to at most size sz
void FXUndoList::trimSize(FXuint sz){
  FXTRACE((100,"FXUndoList::trimSize: was: space=%d undocount=%d; marker=%d ",space,undocount,marker));
  if(space>sz){
    FXCommand **pp=&undolist;
    FXCommand *p=*pp;
    FXuint s=0;
    while(p && (s=s+p->size())<=sz){
      pp=&p->next;
      p=*pp;
      }
    while(*pp){
      p=*pp;
      *pp=p->next;
      space-=p->size();
      undocount--;
      delete p;
      }
    if(marker>undocount) marker=NOMARK;
    }
  FXTRACE((100,"now: space=%d undocount=%d; marker=%d\n",space,undocount,marker));
  }


}

