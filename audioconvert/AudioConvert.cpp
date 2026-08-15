/*******************************************************************************
*                             Audio Converter                                  *
********************************************************************************
*           Copyright (C) 2010-2010 by Sander Jansen. All Rights Reserved      *
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
#include "fox.h"
#include "ap.h"
#include "AudioConvert.h"
#include "GMTrack.h"
#include "GMFilename.h"
#include "GMCover.h"

#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


#define TASK_SUCCESS(s) (WIFEXITED(s) && (WEXITSTATUS(s)==0))


FXbool gm_make_path(const FXString & path,FXuint perm=FXIO::OwnerFull|FXIO::GroupFull|FXIO::OtherFull) {
  return FXDir::createDirectories(path,perm);
  }




TaskManager::TaskManager(FXint max) : maxtask(max) {
  }

TaskManager::~TaskManager() {
  }

void TaskManager::appendTask(Task * task,FXbool dryrun) {
  if (dryrun) {
    if (task->run()) {
      while(!task->done(true)) ;
      }
    delete task;
    }
  else {
    if (task->run()) {
      tasklist.append(task);
      wait();
      while(tasklist.no()==maxtask) {
        wait(true);
        }
      }
    else {
      delete task;
      }
    }
  }

void TaskManager::removeTask(FXint i) {
  Task * task = tasklist[i];
  tasklist.erase(i);
  delete task;
  }


void TaskManager::updateTask(pid_t pid,FXint status) {
  for (FXint i=0;i<tasklist.no();i++){
    if (tasklist[i]->pid==pid) {
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        if (WIFEXITED(status))
          printf("Info: process %d exited with status %d\n",pid,WEXITSTATUS(status));
        else
          printf("Info: process %d terminated with signal %d\n",pid,WTERMSIG(status));

        tasklist[i]->pid=0;
        if (tasklist[i]->done(TASK_SUCCESS(status)))
          removeTask(i);
        }
      else if (WIFSTOPPED(status)) {
        printf("Info: process %d was stopped with signal %d\n",pid,WSTOPSIG(status));
        }
      else if (WIFCONTINUED(status)) {
        printf("Info: process %d continued\n",pid);
        }
      else {
        FXASSERT(0);
        }
      return;
      }
    }
  }

void TaskManager::wait(FXbool block) {
  pid_t pid;
  int status;
  do {
    pid = waitpid(-1,&status,(block) ? 0 : WNOHANG);
    if (pid>0) {
      updateTask(pid,status);
      if (!block) continue;
      }
    else if (pid==-1) {
      if (errno==EINTR) continue;
      }
    break;
    }
  while(1);
  }

void TaskManager::waitall() {
  pid_t pid;
  int status;
  do {
    pid = waitpid(-1,&status,0);
    if (pid>0) {
      updateTask(pid,status);
      continue;
      }
    else if (pid==-1) {
      if (errno==EINTR) continue;
      }
    break;
    }
  while(1);
  }


static void print_argv(const FXString & operation) {
  FXint pos=0;
  do {
    printf("%s",operation.text()+pos);
    pos=operation.find('\0',pos);
    if (pos==-1 || pos+1>=operation.length()) break;
    printf(" ");
    pos+=1;
    }
  while(1);
  printf("\n");
  }


static void make_argv(FXArray<FXchar*> & argv,const FXString & operation){
  FXint pos=0;
  argv.no(1);
  do {
    argv[argv.no()-1]=(FXchar*)(operation.text()+pos);
    argv.no(argv.no()+1);
    pos=operation.find('\0',pos);
    if (pos==-1 || pos+1>=operation.length()) break;
    pos+=1;
    }
  while(1);
  argv[argv.no()-1]=NULL;
  }

Task::Task() : pid(0) {
  }

Task::~Task() {
  FXASSERT(pid==0);
  }

FXbool Task::run() {
  return false;
  }

FXbool Task::execute(const FXString & cmd) {
  pid = fork();
  if (pid==-1) {
    return false;
    }
  else if (pid==0) {
    int i = sysconf(_SC_OPEN_MAX);
    while (--i >= 3) {
      close(i);
      }
    FXArray<FXchar*> argv;
    make_argv(argv,cmd);
    execvp(argv[0],argv.data());
    exit(EXIT_FAILURE);
    }
  else {
    printf("Info: process %d started\n",pid);
    return true;
    }
  return false;
  }




class BaseTask : public Task {
private:
  static FXDictionary target_paths;
protected:
  AudioConverter * audioconvert;
protected:
  GMTrack  source_track;
  FXString source;
  FXString target;
  FXString target_path;
  FXuint   from;
  FXuint   to;
protected:
  FXbool target_format();
  FXbool target_check();
  FXbool target_update();
  FXbool target_make_path();
  FXbool target_extract_cover();
  FXbool target_clear_covers();
  FXbool copy_tags();
  FXTime target_cover_modified();
protected:
  FXbool execute(const FXString & command);
public:
  BaseTask(AudioConverter* audioconvert,const FXString & in,FXuint from, FXuint to);
  };

FXDictionary BaseTask::target_paths;

BaseTask::BaseTask(AudioConverter* ac, const FXString & in,FXuint f,FXuint t) : audioconvert(ac),source(in),from(f),to(t) {
  }

FXbool BaseTask::target_format() {
  if (audioconvert->getFormat()) {

    // Load Tag
    if (!source_track.loadTag(source)) {
      printf("Error: Failed to load tag from %s\n",source.text());
      return false;
      }

    // Get Format Template
    FXString path = FXPath::expand(audioconvert->getFormatTemplate());

    // Make absolute if necessary
    if (!FXPath::isAbsolute(path))
      path = FXPath::absolute(audioconvert->getTarget(),path);

    // Simplify
    path = FXPath::simplify(path);

    gm::TrackFormatter trackformat(path,
                                   audioconvert->getFormatCodec(),
                                   audioconvert->getFormatStrip(),
                                   audioconvert->getFormatOptions());

    target = trackformat.getPath(source_track);
    // Format name based on tag
    // target = GMFilename::format_track(source_track,path,audioconvert->getFormatStrip(),audioconvert->getFormatOptions(),audioconvert->getFormatCodec());

    // Add extension
    if (to==FILE_COPY)
      target += "." + FXPath::extension(source);
    else
      target += audioconvert->getExtension(to);

    // Get the path
    target_path = FXPath::directory(target);
    }
  else {
    FXString path = FXPath::directory(source);

    if (path!=audioconvert->getSource())
      target_path = FXPath::absolute(audioconvert->getTarget(),FXPath::relative(audioconvert->getSource(),path));
    else
      target_path = audioconvert->getTarget();

    if (to==FILE_COPY)
      target = target_path + PATHSEPSTRING + FXPath::name(source);
    else
#if FOXVERSION < FXVERSION(1, 7, 83)
      target = target_path + PATHSEPSTRING + FXPath::title(source) + audioconvert->getExtension(to);
#else
      target = target_path + PATHSEPSTRING + FXPath::stem(source) + audioconvert->getExtension(to);
#endif
    }
  return true;
  }

FXbool BaseTask::target_check() {
  if (source==target) {
    return false;
    }
  return true;
  }

FXbool BaseTask::target_update() {
  if (audioconvert->getOverWrite() || FXStat::modified(source) > FXStat::modified(target))
    return true;
  else
    return false;
  }


FXTime BaseTask::target_cover_modified(){
  FXTime m,recent=0;
  FXString * covers=NULL;
  FXint ncovers = FXDir::listFiles(covers,target_path,"cover.(jpg,jpeg,png,bmp,gif)",FXDir::NoDirs|FXDir::CaseFold); 
  for (FXint i=0;i<ncovers;i++) {
    m=FXStat::modified(target_path+PATHSEPSTRING+covers[i]);
    if (m>0) recent=FXMAX(recent,m);
    }
  return recent;
  }

FXbool BaseTask::target_clear_covers() {
  if (audioconvert->getExtractCover()){
    FXString * covers=NULL;
    printf("Clearing covers in %s\n",target_path.text());    
    FXint ncovers = FXDir::listFiles(covers,target_path,"*.(jpg,jpeg,png,bmp,gif)",FXDir::NoDirs|FXDir::CaseFold); 
    for (FXint i=0;i<ncovers;i++) {    
      if (audioconvert->getDryRun())
        printf("rm %s%s%s\n",target_path.text(),PATHSEPSTRING,covers[i].text());  
      else {
        printf("Removing %s%s%s\n",target_path.text(),PATHSEPSTRING,covers[i].text());
        FXFile::remove(target_path+PATHSEPSTRING+covers[i]);
        }
      }
    delete [] covers;
    }
  return true;
  }

FXbool BaseTask::target_extract_cover() {
  if (audioconvert->getExtractCover() && FXStat::modified(source) > target_cover_modified() ){
    if (audioconvert->getDryRun()) {
      if (!target_paths.has(target_path)) {
        printf("(internal) extract cover to %s\n",target_path.text());
        target_clear_covers();
        }
      target_paths.insert(target_path,(void*)(FXival)1);
      }
    else {
      GMCover * cover = GMCover::fromTag(source);
      if (cover) {
        target_clear_covers();
        FXString coverfilename = target_path+PATHSEPSTRING+"cover"+cover->fileExtension();
        printf("Extracting cover to %s\n",coverfilename.text());
        if (!cover->save(coverfilename))
          return false;
        }
      else {
        printf("No cover in %s\n",source.text());
        }
      }
    }
  return true;
  }

FXbool BaseTask::execute(const FXString & command) {
  if (audioconvert->getDryRun()) {
    print_argv(command);
    return true;
    }
  else if (!Task::execute(command)){
    audioconvert->stop();
    return false;
    }
  else {
    return true;
    }
  }

FXbool BaseTask::copy_tags() {
  if (audioconvert->getDryRun()) {
    printf("(internal) copy tags\n");
    }
  else {
    GMTrack target_track;

    /// Load source tag if we haven't already
    if (!audioconvert->getFormat()) {
      if (!source_track.loadTag(source)) {
        printf("Warning: failed to load tag from %s\n",source.text());
        return false;
        }
      }

    /// Load target tag
    if (!target_track.loadTag(target)) {
      printf("Warning: failed to load tag from %s\n",target.text());
      return false;
      }

    /// Update target tag if different from source tag
    if (source_track.title!=target_track.title) {
      source_track.saveTag(target);
      }
    }
  return true;
  }

FXbool BaseTask::target_make_path() {
  if (audioconvert->getDryRun()){
    printf("mkdir -p %s\n",target_path.text());
    return true;
    }
  else {
    return gm_make_path(target_path);
    }
  }



// Copies source to target
class CopyTask : public BaseTask {
public:
  CopyTask(AudioConverter * a,const FXString & source);
  FXbool run();
  };


CopyTask::CopyTask(AudioConverter * a,const FXString & i) :  BaseTask(a,i,FILE_COPY,FILE_COPY) {
  }

FXbool CopyTask::run() {
  if (!target_format()){
    audioconvert->stop();
    return false;
    }

  if (!target_check()){
    audioconvert->stop();
    return false;
    }

  if (!target_update()){
    if (!target_extract_cover()){
      audioconvert->stop();
      return false;
      }
    return false;
    }

  if (!target_make_path()){
    audioconvert->stop();
    return false;
    }

  if (!target_extract_cover()){
    audioconvert->stop();
    return false;
    }

  printf("Info:   task: copy\n");
  printf("      source: %s\n",source.text());
  printf("      target: %s\n\n",target.text());
  if (!audioconvert->getDryRun() && !FXFile::copyFiles(source,target,true)){
    audioconvert->stop();
    }
  // Don't add this task to the process manager
  return false;
  }


class DirectTask : public BaseTask {
public:
  DirectTask(AudioConverter * a,const FXString & i,FXuint f,FXuint t);

  FXbool run();

  FXbool done(FXbool status);
  };


DirectTask::DirectTask(AudioConverter * a,const FXString & i,FXuint f,FXuint t) :BaseTask(a,i,f,t) {
  }

FXbool DirectTask::run() {
  if (!target_format()){
    audioconvert->stop();
    return false;
    }

  if (!target_check()){
    audioconvert->stop();
    return false;
    }

  if (!target_update()){
    if (!target_extract_cover()){
      audioconvert->stop();
      return false;
      }
    return false;
    }

  if (!target_make_path()){
    audioconvert->stop();
    return false;
    }

  if (!target_extract_cover()){
    audioconvert->stop();
    return false;
    }


  printf("Info:   task: convert (direct)\n");
  printf("      source: %s\n",source.text());
  printf("      target: %s\n\n",target.text());
  return execute(audioconvert->getEncoder(to,source,target));
  }


FXbool DirectTask::done(FXbool success) {
  if (success){
    /// oggenc copies tags for us
    if (!(from==FILE_FLAC && to==FILE_OGG))
      copy_tags();
    }
  else {
    audioconvert->stop();
    }
  return true;
  }


class IndirectTask : public BaseTask {
protected:
  FXString temp;
  FXbool   decoding;
protected:
  FXbool target_temp();
public:
  IndirectTask(AudioConverter * a,const FXString & i,FXuint f,FXuint t);

  void clear();

  FXbool run();

  FXbool done(FXbool status);
  };


IndirectTask::IndirectTask(AudioConverter * a,const FXString & i,FXuint f,FXuint t) : BaseTask(a,i,f,t),decoding(true) {
  }

void IndirectTask::clear() {
  if (!temp.empty() && FXStat::exists(temp)){
    FXFile::remove(temp);
    }
  }

FXbool IndirectTask::target_temp() {
  temp = FXPath::unique(target_path+PATHSEPSTRING"audioconvert.wav");
  if (audioconvert->getDryRun())
    return true;
  else
    return FXFile::create(temp);
  }



FXbool IndirectTask::done(FXbool success) {
  if (success) {
    if (decoding) {
      if (!execute(audioconvert->getEncoder(to,temp,target))) {
        clear();
        audioconvert->stop();
        return true;
        }
      decoding=false;
      return false;
      }
    else {
      // finish up by copying tags
      copy_tags();
      clear();
      return true;
      }
    }
  else {
    clear();
    audioconvert->stop();
    }
  return true;
  }

FXbool IndirectTask::run() {

  if (!target_format()){
    audioconvert->stop();
    return false;
    }

  if (!target_check()){
    audioconvert->stop();
    return false;
    }

  if (!target_update()){
    if (!target_extract_cover()){
      audioconvert->stop();
      return false;
      }
    return false;
    }

  if (!target_make_path()){
    audioconvert->stop();
    return false;
    }

  if (!target_extract_cover()){
    audioconvert->stop();
    return false;
    }

  if (!target_temp()){
    audioconvert->stop();
    return false;
    }

  printf("Info:   task: convert (indirect)\n");
  printf("      source: %s\n",source.text());
  printf("         via: %s\n",temp.text());
  printf("      target: %s\n\n",target.text());
  return execute(audioconvert->getDecoder(from,source,temp));
  }





class RecodeTask : public BaseTask {
public:
  RecodeTask(AudioConverter * a,const FXString & i,FXuint f);

  FXbool run();

  FXbool done(FXbool status);
  };


RecodeTask::RecodeTask(AudioConverter * a,const FXString & i,FXuint f) :BaseTask(a,i,f,f) {
  }

FXbool RecodeTask::run() {
  if (!target_format()){
    audioconvert->stop();
    return false;
    }

  if (!target_update()){
    return false;
    }

  printf("Info:   task: recode\n");
  printf("      target: %s\n\n",target.text());
  return execute(audioconvert->getEncoder(to,source,target));
  }


FXbool RecodeTask::done(FXbool success) {
  if (!success){
    audioconvert->stop();
    }
  return true;
  }

enum {
  STATUS_ERROR=0,
  STATUS_OK=1
  };



AudioConverter::AudioConverter() :
  dryrun(false),
  overwrite(false),
  nodirect(false),
  reformat(false),
  extractcover(false),
  status(STATUS_OK),
  format_template("%P/%A?d< - disc %d>/%N %T"),
  format_strip("\'\\#~!\"$&();<>|`^*?[]/.:"),
  format_options(0),
  format_codec(NULL) {
  for (FXint i=0;i<FILE_NTYPES;i++)
    mode[i]=FILE_NONE;
  }

FXuint AudioConverter::getMode(const FXchar * cstr){
  FXString m = FXString(cstr).after('=');
  if (FXString::comparecase(m,"ogg")==0)
    return FILE_OGG;
  else if (FXString::comparecase(m,"mp3")==0)
    return FILE_MP3;
  else if (FXString::comparecase(m,"flac")==0)
    return FILE_FLAC;
  else if (FXString::comparecase(m,"mp4")==0)
    return FILE_MP4;
  else if (FXString::comparecase(m,"mpc")==0)
    return FILE_MPC;
  else if (FXString::comparecase(m,"opus")==0)
    return FILE_OPUS;		
  else if (FXString::comparecase(m,"copy")==0)
    return FILE_COPY;
  else if (FXString::comparecase(m,"none")==0 || FXString::comparecase(m,"off")==0)
    return FILE_NONE;
  else{
    printf("Error: Invalid conversion specified \"%s\"\n",cstr);
    return FILE_INVALID;
    }
  }

void AudioConverter::initConfig() {
  const FXchar * config_file = "audioconvert/config.rc";

  FXString xdg_config_dirs = FXSystem::getEnvironment("XDG_CONFIG_DIRS");
  if (xdg_config_dirs.empty()) xdg_config_dirs="/etc/xdg";

  FXString xdg_config_home = FXSystem::getEnvironment("XDG_CONFIG_HOME");
  if (xdg_config_home.empty()) xdg_config_home="$HOME/.config";

  FXString config = FXPath::search(xdg_config_home,config_file);
  if (config.empty()) config = FXPath::search(xdg_config_dirs,config_file);

  if (!config.empty()){
    printf("Info: Found config %s\n",config.text());
    FXSettings settings;
    if (settings.parseFile(config)) {
      tools.load_rc(settings);
      format_template = settings.readStringEntry("format","template",format_template.text());
      format_strip    = settings.readStringEntry("format","strip",format_strip.text());

      if (settings.readBoolEntry("format","lowercase",false))
        format_options|=gm::TrackFormatter::LOWERCASE;

      if (settings.readBoolEntry("format","nospaces",false))
        format_options|=gm::TrackFormatter::NOSPACE;
      }
    }
  else {
    config = FXPath::absolute(FXPath::expand(xdg_config_home),config_file);
    if (!gm_make_path(FXPath::directory(config))) return;
    printf("Creating config: %s\n",config.text());
    FILE * fp = fopen(config.text(),"w");
    if (fp) {
      fprintf(fp,"#[format]\n");
      fprintf(fp,"#template=%s\n",format_template.text());
      fprintf(fp,"#strip=%s\n\n",format_strip.text());
      fprintf(fp,"#lowercase=false\n\n");
      fprintf(fp,"#nospaces=false\n\n");
      tools.init_rc(fp);
      fclose(fp);
      }
    }
  }



FXbool AudioConverter::init(FXint argc,FXchar *argv[]) {
  FXint nargs=0;
  for (FXint i=1;i<argc;i++) {
    if (FXString::compare(argv[i],"--dry-run")==0 || FXString::compare(argv[i],"-n")==0) {
      dryrun=true;
      }
    else if (FXString::compare(argv[i],"--quiet")==0 || FXString::compare(argv[i],"-q")==0){
      tools.quiet();
      }
    else if (FXString::compare(argv[i],"--overwrite")==0) {
      overwrite=true;
      }
    else if (FXString::compare(argv[i],"--no-direct")==0) {
      nodirect=true;
      }
    else if (FXString::compare(argv[i],"--extract-cover")==0){
      extractcover=true;
      }
    else if (FXString::compare(argv[i],"--rename")==0) {
      reformat=true;
      }
    else if (FXString::compare(argv[i],"--format-template=",9)==0) {
      reformat=true;
      format_template = FXString(argv[i]).after('=');
      }
    else if (FXString::compare(argv[i],"--format-strip=",15)==0) {
      reformat=true;
      format_strip = FXString(argv[i]).after('=');
      }
    else if (FXString::compare(argv[i],"--format-encoding=",18)==0) {
      reformat=true;
      format_codec = ap_get_textcodec(FXString(argv[i]).after('='));
      if (format_codec == nullptr) {
        printf("Error: Invalid format encoding %s",argv[i]);
        return false;
        }
      }
    else if (FXString::compare(argv[i],"--format-no-spaces")==0) {
      reformat=true;
      format_options|=gm::TrackFormatter::NOSPACE;
      }
    else if (FXString::compare(argv[i],"--format-lowercase")==0) {
      reformat=true;
      format_options|=gm::TrackFormatter::LOWERCASE;
      }
    else if (FXString::compare(argv[i],"-j")==0) {
      if ((i+1)<argc) {
        FXint max = FXMAX(1,FXString(argv[i+1]).toInt());
        printf("Info: Using maximum %d parallel tasks\n",max);
        manager.setMaxTasks(max);
        i+=1;
        }
      else {
        printf("Error: Missing argument for -j <number>\n");
        return false;
        }
      }
    else if (FXString::compare(argv[i],"--ogg=",6)==0){
      if ((mode[FILE_OGG]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--opus=",7)==0){
      if ((mode[FILE_OPUS]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--mp3=",6)==0){
      if ((mode[FILE_MP3]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--flac=",7)==0){
      if ((mode[FILE_FLAC]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--mp4=",6)==0){
      if ((mode[FILE_MP4]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--mpc=",6)==0){
      if ((mode[FILE_MPC]=getMode(argv[i]))==FILE_INVALID)
        return false;
      }
    else if (FXString::compare(argv[i],"--all=",6)==0){
      FXuint m=getMode(argv[i]);
      if (m==FILE_INVALID)
        return false;

      for (FXint f=0;f<FILE_NTYPES;f++)
        mode[f]=m;
      }
    else {
      nargs++;

      if (nargs>2) {
        printf("Error: Unexpected argument (%d) %s\n",i,argv[i]);
        return false;
        }

      FXString path = argv[i];
      if (!FXPath::isAbsolute(path))
        path = FXPath::absolute(FXSystem::getCurrentDirectory(),path);

      if (nargs==1)
        source.adopt(path);
      else
        target.adopt(path);
      }
    }

  // Read configuration
  initConfig();

  // Make sure we're actually doing something
  FXbool nothing=true;
  for (FXint f=0;f<FILE_NTYPES;f++){
    if (mode[f]!=FILE_NONE) {
      nothing=false;
      break;
      }
    }

  // If no operation was specified, bail out
  if(nothing) {
    printf("Error: No output conversion specified\n");
    return false;
    }

  // Check if we have to tools to do the request operations
  for (FXint f=0;f<FILE_NTYPES;f++){
    if (mode[f]!=FILE_NONE && mode[f]!=FILE_COPY) {
      if (!tools.check(f,mode[f]))
        return false;
      }
    }

  // Make sure source has been specified
  if (source.empty()) {
    printf("Error: Missing argument source directory.\n");
    return false;
    }

  // Make sure source exists
  if (!FXStat::exists(source) || !FXStat::isDirectory(source)){
    printf("Error: source is not a directory\n");
    return false;
    }

  // If no target is specified, we use the source directory instead
  if (target.empty()) {
    target=source;
    }

  // Check if source and target are the same directory
  if (target==source){
    printf("Destination directory same as source directory! Are you sure? ");
    int answer = getc(stdin);
    if (!(answer=='y' || answer=='Y')) {
      printf("\n");
      return false;
      }
    printf("\n");
    }

  // Check if target exists and is a directory
  if (FXStat::exists(target) && !FXStat::isDirectory(target)){
    printf("Error: destination is not a directory\n");
    return false;
    }

  // Check if we have write permissions
  if (!FXStat::isWritable(target)){
    printf("Error: destination is not writable\n");
    return false;
    }

  // Success!
  return true;
  }


FXuint AudioConverter::getFileType(const FXString & path) const {
  FXString extension = FXPath::extension(path);

  if (FXString::comparecase(extension,"flac")==0 || FXString::comparecase(extension,"oga")==0)
    return FILE_FLAC;
  else if (FXString::comparecase(extension,"mp3")==0)
    return FILE_MP3;
  else if (FXString::comparecase(extension,"ogg")==0)
    return FILE_OGG;
  else if (FXString::comparecase(extension,"mp4")==0 || FXString::comparecase(extension,"m4a")==0)
    return FILE_MP4;
  else if (FXString::comparecase(extension,"mpc")==0)
    return FILE_MPC;
  else if (FXString::comparecase(extension,"opus")==0)
    return FILE_OPUS;	
  else
    return FILE_INVALID;
  }


void AudioConverter::stop() {
  status=STATUS_ERROR;
  }

FXint AudioConverter::run() {
  // Traverse into source directory
  traverse(source);

  // Wait untill all child processes are done
  manager.waitall();

  // Return success / failure
  if (status==STATUS_OK)
    return 0;
  else
    return -1;
  }


FXuint AudioConverter::enter(const FXString & /*path*/) {
  return status;
  }

FXuint AudioConverter::leave(const FXString & /*path*/) {
  return status;
  }

FXuint AudioConverter::visit(const FXString & path) {
  FXuint filetype = getFileType(path);
  if (filetype!=FILE_INVALID) {
    switch(mode[filetype]) {
      case FILE_FLAC:
      case FILE_MP3 :
      case FILE_OGG :
      case FILE_MP4 :
      case FILE_MPC :
      case FILE_OPUS: if (mode[filetype]==filetype) {
                        if (tools.encoder_supports(mode[filetype],filetype))
                          manager.appendTask(new RecodeTask(this,path,filetype),dryrun);
                        else
                          stop();
                        }
                      else if (tools.encoder_supports(mode[filetype],filetype) && canDirectConvert(path,filetype)) {
                        manager.appendTask(new DirectTask(this,path,filetype,mode[filetype]),dryrun);
                        }
                      else {
                        manager.appendTask(new IndirectTask(this,path,filetype,mode[filetype]),dryrun);
                        }
                      break;

      case FILE_COPY: manager.appendTask(new CopyTask(this,path)); break;
      case FILE_NONE: break;
      default       : FXASSERT(0); stop(); break;
      }
    }
  return status;
  }


FXbool AudioConverter::canDirectConvert(const FXString & path,FXuint filetype) {

  // Should we use the direct convert
  if (nodirect)
    return false;

  // oggenc can't handle flac files starting with a ID3v2 tag.
  if (filetype==FILE_FLAC && mode[filetype]==FILE_OGG){
    FXchar buffer[4]={0};
    FXFile file;
    if (file.open(path)) {
      if (file.readBlock(buffer,4)==4) {
        if (buffer[0]=='f' && buffer[1]=='L' && buffer[2]=='a' && buffer[3]=='C'){
          return true;
          }
        }
      }
    printf("Info: found flac file with possible id3v2 tag, forcing indirect conversion\n");
    // couldn't verify it is really a flac file.
    return false;
    }

  // It's ok.
  return true;
  }

