/********************************************************************************
*                                                                               *
*                R e s o u r c e   W r a p p i n g   U t i l i t y              *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2023 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This program is free software: you can redistribute it and/or modify          *
* it under the terms of the GNU General Public License as published by          *
* the Free Software Foundation, either version 3 of the License, or             *
* (at your option) any later version.                                           *
*                                                                               *
* This program is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 *
* GNU General Public License for more details.                                  *
*                                                                               *
* You should have received a copy of the GNU General Public License             *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.         *
********************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <time.h>
#include "ctype.h"



/*

  Notes:
  - Can now also generate output as a (possibly escaped) text string.
  - Options to suffixes and prefixes; for example:

        reswrap -o icons.cpp  --prefix tb_ --suffix _gif *.gif --suffix _bmp *.bmp

    Given file names a.gif, b.gif, c.bmp, d.bmp, generates names like:

        const unsigned char tb_a_gif[];
        const unsigned char tb_b_gif[];
        const unsigned char tb_c_bmp[];
        const unsigned char tb_d_bmp[];

  - Added ability to switch output files allows both icons.h and icons.cpp
    to be generated in one command like:

        reswrap -h -o icons.h $(ICONS) -s -i icons.h -o icons.cpp $(ICONS)

    which will kill issues with dependencies; this will also embed the
    string:

        #include "icons.h"

    Into the icons.cpp file.
*/

#define MODE_DECIMAL   0
#define MODE_HEX       1
#define MODE_TEXT      2
#define MODE_ASCII     3

#define LINKAGE_NONE   0
#define LINKAGE_EXTERN 1
#define LINKAGE_STATIC 2

#define TYPE_SOURCE    0
#define TYPE_HEADER    1

#define MAX_RESOURCE   512

#define NUM_COLUMNS    16

/*******************************************************************************/


const char version[]="6.0.0";


typedef struct {
  const char *outfilename;
  const char *prefix;
  const char *suffix;
  const char *scope;
  const char *header;
  FILE       *outfile;
  int         append;
  int         maxcols;
  int         colsset;
  int         linkage;
  int         filetype;
  int         declaresize;
  int         forceunsigned;
  int         constant;
  int         comments;
  int         keepext;
  int         binary;
  int         verbose;
  int         mode;
  } OPTIONS;

/*******************************************************************************/

/* Print short help */
static void printhelp(const char *option){
  printf("reswrap: invalid option: -- %s\n",option);
  printf("Usage: reswrap [options] [-o[a] outfile] files...\n");
  printf("Try \"reswrap --help\" for more information.\n");
  }


/* Print some help */
static void printusage(){
  printf("Usage: reswrap [options] [-o[a] outfile] files...\n");
  printf("Convert files containing images, text, or binary data into C/C++ data arrays.\n");
  printf("\n");
  printf("Options:\n");
  printf("  -?, --help                Print this help\n");
  printf("  -v, --version             Print version number\n");
  printf("  -h, --header              Create header file containing only declarations\n");
  printf("  -s, --source              Create source file containing data arrays (default)\n");
  printf("  -V, --verbose             Show which resource files are being processed\n");
  printf("  -i file, --include file   Generate #include \"file\" in output file\n");
  printf("  -o file, --output file    Output to file instead of stdout\n");
  printf("  -oa file, --append file   Append to file instead of stdout\n");
  printf("  -e, --extern              Generate extern reference declarations (default for headers)\n");
  printf("  -S, --static              Generate static reference declarations\n");
  printf("  -z, --size                Output size in array declarations\n");
  printf("  -d, --decimal             Output as decimal\n");
  printf("  -x, --hex                 Output as hex (default)\n");
  printf("  -t, --text                Output as hexadecimal text string\n");
  printf("  -a, --ascii               Output as ascii text string\n");
  printf("  -k, --keep-ext            Keep file extension, replacing period by underscore\n");
  printf("  -nk, --drop-ext           Drop extension (default)\n");
  printf("  -m, --msdos               Read files with MS-DOS mode\n");
  printf("  -b, --binary              Read files using binary mode (default)\n");
  printf("  -u, --unsigned            Force unsigned char even for text mode\n");
  printf("  -N  --no-const            Don't output const specifier in declaration\n");
  printf("  -C  --const               Force const specifier in declaration\n");
  printf("  -cc, --comments           Add comments to the output files (default)\n");
  printf("  -nc, --no-comments        Remove comments from the output files\n");
  printf("  -p name, --prefix name    Prepend name in front of names of declarations and definitions\n");
  printf("  -f name, --suffix name    Append name in after names of declarations and definitions\n");
  printf("  -n name, --namespace name Place declarations and definitions inside namespace name\n");
  printf("  -c cols, --columns cols   Change number of columns in output from %d to cols\n",NUM_COLUMNS);
  printf("  -r name, --resource name  Override resource name of following resource file\n");
  printf("\n");
  }


/* Print version information */
static void printversion(){
  printf("reswrap %s\n\n",version);
  printf("Copyright (C) 1997,2022 Jeroen van der Zijp. All Rights Reserved.\n");
  printf("Please visit: http://www.fox-toolkit.org for further information.\n");
  printf("\n");
  printf("This program is free software: you can redistribute it and/or modify\n");
  printf("it under the terms of the GNU General Public License as published by\n");
  printf("the Free Software Foundation, either version 3 of the License, or\n");
  printf("(at your option) any later version.\n");
  printf("\n");
  printf("This program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  printf("GNU General Public License for more details.\n");
  printf("\n");
  printf("You should have received a copy of the GNU General Public License\n");
  printf("along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
  }

/*******************************************************************************/

/* Build resource name */
static const char* resourcename(char *name,const char* filename,int keepdot){
  const char* begin=name;
  const char* ptr;

  /* Get name only; take care of mixed path separator characters on mswindows */
  if((ptr=strrchr(filename,'/'))!=0) filename=ptr;
  if((ptr=strrchr(filename,'\\'))!=0) filename=ptr;

  /* Copy filename */
  while(*filename){

    /* C++ identifier may contain _, alpha, digit (if not first), or namespace separator : */
    if(*filename==':' || *filename=='_' || isalpha(*filename) || (isdigit(*filename) && (begin!=name))){
      *name++=*filename;
      }

    /* We can squash dot extension to _ */
    else if(*filename=='.'){
      if(!keepdot) break;
      *name++='_';
      }

    filename++;
    }
  *name=0;
  return begin;
  }


/* Process prologue */
static void prologue(OPTIONS* opts){
  char date[100];
  time_t clock;
  if(opts->outfilename){
    if(opts->append){
      opts->outfile=fopen(opts->outfilename,"a");
      if(!opts->outfile){
        fprintf(stderr,"reswrap: unable to open output file: %s\n",opts->outfilename);
        exit(1);
        }
      if(ftell(opts->outfile)>0) return;
      }
    else{
      opts->outfile=fopen(opts->outfilename,"w");
      if(!opts->outfile){
        fprintf(stderr,"reswrap: unable to open output file: %s\n",opts->outfilename);
        exit(1);
        }
      }
    }
  if(opts->comments){
    clock=time(NULL);
    strftime(date,sizeof(date),"%Y/%m/%d %H:%M:%S",localtime(&clock));
    fprintf(opts->outfile,"/*********** Generated on %s by reswrap version %s *********/\n\n",date,version);
    }
  if(opts->header){
    fprintf(opts->outfile,"#include \"%s\"\n\n",opts->header);
    }
  if(opts->scope){
    fprintf(opts->outfile,"namespace %s {\n\n",opts->scope);
    }
  }


/* Process epilogue */
static void epilogue(OPTIONS* opts){
  if(opts->scope){
    fprintf(opts->outfile,"}\n");
    }
  if(opts->outfile!=stdout){
    fclose(opts->outfile);
    opts->outfile=stdout;
    }
  }


/* Process resource file */
static int processresourcefile(const char* filename,const char* name,OPTIONS* opts){
  const char *typestr="unsigned char ";
  const char *sizestr="[]";
  const char *conststr="";
  const char *linkstr="";
  char resource[MAX_RESOURCE],size[10];
  int ressize,first,col,hex,b;
  FILE *file;

  /* Open resource file; always open as binary */
  file=fopen(filename,"rb");
  if(file){

    /* Get the size */
    fseek(file,0,SEEK_END);
    ressize=ftell(file);
    fseek(file,0,SEEK_SET);

    /* Add one if text mode, for end of string */
    if(opts->mode>=MODE_TEXT){
      ressize++;
      }

    /* Output header */
    if(opts->comments){
      fprintf(opts->outfile,"/* Created by reswrap from file %s */\n",filename);
      }

    /* Generate external reference */
    if(opts->linkage==LINKAGE_EXTERN){
      linkstr="extern ";
      }

    /* Generate static reference */
    else if(opts->linkage==LINKAGE_STATIC){
      linkstr="static ";
      }

    /* Generate const declaration */
    if(opts->constant){
      conststr="const ";
      }

    /* In text mode, output a 'char' declaration */
    if((opts->mode>=MODE_TEXT) && !opts->forceunsigned){
      typestr="char ";
      }

    /* Compute resource name from filename if no override */
    if(!name){
      name=resourcename(resource,filename,opts->keepext);
      }

    /* Size specifier */
    if(opts->declaresize){
      sprintf(size,"[%d]",ressize);
      sizestr=size;
      }

    /* Generate resource name */
    fprintf(opts->outfile,"%s%s%s%s%s%s%s",linkstr,conststr,typestr,opts->prefix,name,opts->suffix,sizestr);

    /* Generating source file */
    if(opts->filetype==TYPE_SOURCE){

      /* Text Mode */
      if(opts->mode>=MODE_TEXT){
        col=0;
        hex=0;
        fprintf(opts->outfile,"=\n  \"");
        while((b=fgetc(file))!=EOF){
          if(!opts->binary && (b=='\r')) continue;
          if(col>=opts->maxcols && opts->maxcols){
            fprintf(opts->outfile,"\"\n  \"");
            col=0;
            }
          if(opts->mode==MODE_ASCII){
            if(b=='\\'){ fprintf(opts->outfile,"\\\\"); col+=2; hex=0; }
            else if(b=='\a'){ fprintf(opts->outfile,"\\a"); col+=2; hex=0; }
            else if(b=='\t'){ fprintf(opts->outfile,"\\t"); col+=2; hex=0; }
            else if(b=='\r'){ fprintf(opts->outfile,"\\r"); col+=2; hex=0; }
            else if(b=='\f'){ fprintf(opts->outfile,"\\f"); col+=2; hex=0; }
            else if(b=='\v'){ fprintf(opts->outfile,"\\v"); col+=2; hex=0; }
            else if(b=='\"'){ fprintf(opts->outfile,"\\\""); col+=2; hex=0; }
            else if(b=='\n'){ fprintf(opts->outfile,"\\n\"\n  \""); col=0; hex=0; }
            else if(b<32 || b>=127){ fprintf(opts->outfile,"\\x%02x",b); col+=4; hex=1; }
            else if(hex && isxdigit(b)){ fprintf(opts->outfile,"\\x%02x",b); col+=4; hex=1; }
            else{ fprintf(opts->outfile,"%c",b); col+=1; hex=0; }
            }
          else{
            fprintf(opts->outfile,"\\x%02x",b); col+=4;
            }
          }
        fprintf(opts->outfile,"\"\n  ");
        }

      /* Normal Mode */
      else{
        col=0;
        first=1;
        fprintf(opts->outfile,"={");
        if(opts->maxcols) fprintf(opts->outfile,"\n  ");
        while((b=fgetc(file))!=EOF){
          if(!opts->binary && (b=='\r')) continue;
          if(!first){
            fprintf(opts->outfile,",");
            }
          if(col>=opts->maxcols && opts->maxcols){
            fprintf(opts->outfile,"\n  ");
            col=0;
            }
          if(opts->mode==MODE_HEX){
            fprintf(opts->outfile,"0x%02x",b);
            }
          else{
            fprintf(opts->outfile,"%3d",b);
            }
          first=0;
          col++;
          }
        if(opts->maxcols) fprintf(opts->outfile,"\n  ");
        fprintf(opts->outfile,"}");
        }
      }

    /* Append ; */
    fprintf(opts->outfile,";\n\n");

    /* Verbose mode */
    if(opts->verbose){
      fprintf(stderr,"%-30s  %s%s%s%s%s%s%s\n",filename,linkstr,conststr,typestr,opts->prefix,name,opts->suffix,sizestr);
      }

    /* Close resource file */
    fclose(file);
    return 1;
    }
  return 0;
  }

/*******************************************************************************/

/* Main */
int main(int argc,char **argv){
  const char *resource=0;
  int         needprologue=1;
  int         needepilogue=0;
  OPTIONS     opts;
  int         arg;

  /* Initialize */
  opts.outfilename=0;
  opts.prefix="";
  opts.suffix="";
  opts.scope=0;
  opts.append=0;
  opts.header=0;
  opts.constant=1;
  opts.outfile=stdout;
  opts.maxcols=NUM_COLUMNS;
  opts.colsset=0;
  opts.linkage=LINKAGE_NONE;
  opts.filetype=TYPE_SOURCE;
  opts.declaresize=0;
  opts.forceunsigned=0;
  opts.comments=1;
  opts.keepext=0;
  opts.binary=1;
  opts.verbose=0;
  opts.mode=MODE_HEX;

  /* Process all options first, except for the -r option */
  for(arg=1; arg<argc; arg++){

    /* Handle options */
    if(argv[arg][0]=='-'){

      /* Change output file */
      if(strcmp(argv[arg],"-o")==0 || strcmp(argv[arg],"--output")==0){

        /* Output epilogue */
        if(needepilogue){
          epilogue(&opts);
          }

        /* Check if argument provided */
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -o or --output option\n");
          exit(1);
          }

        /* Get new filename */
        opts.outfilename=argv[arg];
        opts.append=0;
        needprologue=1;
        needepilogue=0;
        }

      /* Change output file and append */
      else if(strcmp(argv[arg],"-oa")==0 || strcmp(argv[arg],"--append")==0){

        /* Output epilogue */
        if(needepilogue){
          epilogue(&opts);
          }

        /* Check if argument provided */
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -oa or --append option\n");
          exit(1);
          }

        /* Get new filename */
        opts.outfilename=argv[arg];
        opts.append=1;
        needprologue=1;
        needepilogue=1;
        }

      /* Print help */
      else if(strcmp(argv[arg],"-?")==0 || strcmp(argv[arg],"--help")==0 || (strcmp(argv[arg],"-h")==0 && arg==argc-1)){
        printusage();
        exit(0);
        }

      /* Building include file */
      else if(strcmp(argv[arg],"-h")==0 || strcmp(argv[arg],"--header")==0){
        opts.filetype=TYPE_HEADER;
        opts.linkage=LINKAGE_EXTERN;
        }

      /* Building source file */
      else if(strcmp(argv[arg],"-s")==0 || strcmp(argv[arg],"--source")==0){
        opts.filetype=TYPE_SOURCE;
        opts.linkage=LINKAGE_NONE;
        }

      /* Include header file */
      else if(strcmp(argv[arg],"-i")==0 || strcmp(argv[arg],"--include")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing filename for -i or --include option\n");
          exit(1);
          }
        opts.header=0;
        if(strcmp(argv[arg],"-")!=0){
          opts.header=argv[arg];
          }
        }

      /* Print version */
      else if(strcmp(argv[arg],"--version")==0){
        printversion();
        exit(0);
        }

      /* Print version */
      else if(strcmp(argv[arg],"-v")==0){
        printf("reswrap %s\n",version);
        exit(0);
        }

      /* Switch to decimal */
      else if(strcmp(argv[arg],"-d")==0 || strcmp(argv[arg],"--decimal")==0){
        opts.mode=MODE_DECIMAL;
        if(!opts.colsset) opts.maxcols=10;
        }

      /* Switch to hex */
      else if(strcmp(argv[arg],"-x")==0 || strcmp(argv[arg],"--hex")==0){
        opts.mode=MODE_HEX;
        if(!opts.colsset) opts.maxcols=16;
        }

      /* Switch to hexadecimal text */
      else if(strcmp(argv[arg],"-t")==0 || strcmp(argv[arg],"--text")==0){
        opts.mode=MODE_TEXT;
        if(!opts.colsset) opts.maxcols=80;
        }

      /* Switch to ascii text */
      else if(strcmp(argv[arg],"-a")==0 || strcmp(argv[arg],"--ascii")==0){
        opts.mode=MODE_ASCII;
        if(!opts.colsset) opts.maxcols=80;
        }

      /* Switch to ascii text */
      else if(strcmp(argv[arg],"-ta")==0){
        fprintf(stderr,"reswrap: -ta option is deprecated; please use -a or --ascii\n");
        opts.mode=MODE_ASCII;
        if(!opts.colsset) opts.maxcols=80;
        }

      /* Generate as external reference */
      else if(strcmp(argv[arg],"-e")==0 || strcmp(argv[arg],"--extern")==0){
        opts.linkage=LINKAGE_EXTERN;
        }

      /* Generate as static reference */
      else if(strcmp(argv[arg],"-S")==0 || strcmp(argv[arg],"--static")==0){
        opts.linkage=LINKAGE_STATIC;
        }

      /* Force unsigned */
      else if(strcmp(argv[arg],"-u")==0 || strcmp(argv[arg],"--unsigned")==0){
        opts.forceunsigned=1;
        }

      /* No constant declaration */
      else if(strcmp(argv[arg],"-N")==0 || strcmp(argv[arg],"--no-const")==0){
        opts.constant=0;
        }

      /* Force constant declaration */
      else if(strcmp(argv[arg],"-C")==0 || strcmp(argv[arg],"--const")==0){
        opts.constant=1;
        }

      /* Declare size */
      else if(strcmp(argv[arg],"-z")==0 || strcmp(argv[arg],"--size")==0){
        opts.declaresize=1;
        }

      /* Read resource with MS-DOS mode */
      else if(strcmp(argv[arg],"-m")==0 || strcmp(argv[arg],"--msdos")==0){
        opts.binary=0;
        }

      /* Read resource with BINARY mode */
      else if(strcmp(argv[arg],"-b")==0 || strcmp(argv[arg],"--binary")==0){
        opts.binary=1;
        }

      /* Keep extension */
      else if(strcmp(argv[arg],"-k")==0 || strcmp(argv[arg],"--keep-ext")==0){
        opts.keepext=1;
        }

      /* Base name only */
      else if(strcmp(argv[arg],"-nk")==0 || strcmp(argv[arg],"--drop-ext")==0){
        opts.keepext=0;
        }

      /* Change number of columns */
      else if(strcmp(argv[arg],"-c")==0 || strcmp(argv[arg],"--columns")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -c or --columns option\n");
          exit(1);
          }
        if(sscanf(argv[arg],"%d",&opts.maxcols)==1 && opts.maxcols<0){
          fprintf(stderr,"reswrap: illegal argument for number of columns\n");
          exit(1);
          }
        opts.colsset=1;
        }

      /* Add comments */
      else if(strcmp(argv[arg],"-cc")==0 || strcmp(argv[arg],"--comments")==0){
        opts.comments=1;
        }

      /* No comments */
      else if(strcmp(argv[arg],"-nc")==0 || strcmp(argv[arg],"--no-comments")==0){
        opts.comments=0;
        }

      /* Verbose */
      else if(strcmp(argv[arg],"-V")==0 || strcmp(argv[arg],"--verbose")==0){
        opts.verbose=1;
        }

      /* Embed in namespace */
      else if(strcmp(argv[arg],"-n")==0 || strcmp(argv[arg],"--namespace")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -n option\n");
          exit(1);
          }
        opts.scope=0;
        if(strcmp(argv[arg],"-")!=0){
          opts.scope=argv[arg];
          }
        }

      /* Prefix in front of declarations */
      else if(strcmp(argv[arg],"-p")==0 || strcmp(argv[arg],"--prefix")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -p or --prefix option\n");
          exit(1);
          }
        opts.prefix="";
        if(strcmp(argv[arg],"-")!=0){
          opts.prefix=argv[arg];
          }
        }

      /* Suffix behind declarations */
      else if(strcmp(argv[arg],"-f")==0 || strcmp(argv[arg],"--suffix")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -f or --suffix option\n");
          exit(1);
          }
        opts.suffix="";
        if(strcmp(argv[arg],"-")!=0){
          opts.suffix=argv[arg];
          }
        }

      /* Explicitly named resource for this filename */
      else if(strcmp(argv[arg],"-r")==0 || strcmp(argv[arg],"--resource")==0){
        if(++arg>=argc){
          fprintf(stderr,"reswrap: missing argument for -r or --resource option\n");
          exit(1);
          }
        resource=argv[arg];
        }

      /* Don't know this option */
      else{
        printhelp(argv[arg]);
        exit(1);
        }
      }

    /* Handle resource file */
    else{

      /* Output prologue */
      if(needprologue){
        prologue(&opts);
        needprologue=0;
        }

      /* Process resource file */
      if(!processresourcefile(argv[arg],resource,&opts)){
        fprintf(stderr,"reswrap: error reading resource file: \"%s\"\n",argv[arg]);
        break;
        }

      /* Reset for next time */
      needepilogue=1;
      resource=0;
      }
    }

  /* Output epilogue */
  if(needepilogue){
    epilogue(&opts);
    }

  return 0;
  }
