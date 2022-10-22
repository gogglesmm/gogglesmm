/********************************************************************************
*                                                                               *
*                L o a d   I c o n   F r o m   E x e c u t a b l e              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2014,2022 by Jeroen van der Zijp.   All Rights Reserved.        *
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
#include "fxascii.h"
#include "fxunicode.h"
#include "FXArray.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXElement.h"
#include "FXStream.h"


/*
  Notes:
  - Load icon from resource in Microsoft Windows executable.

  - Useful to display correct icon inside file list widgets.

  - Operation:

     o Scan portable execution header for resource segment.
     o Burrow down resource directories matching resource type and id.
     o Then, depending on resource type, use fxloadBMP() or fxloadICO() to pluck
       the resource apart into the desired image pixel data.

  - Not the entire executable image is scanned; thus, potential problem exists
    because we have no way of determining the end of the stream. Thus there is
    no way these can be embedded in a stream of other things as the stream-
    pointer is left somewhere in the resource segment of the executable image.

  - Thankfully this is not a major issue in most cases.
*/

// Maximum recursion level
#define MAXRECLEVEL 3

// Executable file signatures
#define IMAGE_DOS_SIGNATURE             0x5A4D          // MZ
#define IMAGE_OS2_SIGNATURE             0x454E          // NE
#define IMAGE_OS2_SIGNATURE_LE          0x454C          // LE
#define IMAGE_VXD_SIGNATURE             0x454C          // LE
#define IMAGE_NT_SIGNATURE              0x00004550      // PE00

// Optional header magic numbers
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC   0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC   0x20b

// Image data directory entries
#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

// Image resource directory entry flag bits
#define IMAGE_RESOURCE_NAME_IS_STRING        0x80000000
#define IMAGE_RESOURCE_DATA_IS_DIRECTORY     0x80000000

// Resource types
#define RST_CURSOR        1
#define RST_BITMAP        2
#define RST_ICON          3
#define RST_MENU          4
#define RST_DIALOG        5
#define RST_STRING        6
#define RST_FONTDIR       7
#define RST_FONT          8
#define RST_ACCELERATOR   9
#define RST_RCDATA        10
#define RST_MESSAGELIST   11
#define RST_GROUP_CURSOR  12
#define RST_GROUP_ICON    14

using namespace FX;

/*******************************************************************************/

namespace FX {

// Declarations
extern FXAPI FXbool fxcheckEXE(FXStream& store);
extern FXAPI FXbool fxloadEXE(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint type,FXint id);

// From BMP loader
extern FXAPI FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxloadDIB(FXStream& store,FXColor*& data,FXint& width,FXint& height);

// From ICO loader
extern FXAPI FXbool fxloadICO(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& xspot,FXint& yspot);
extern FXAPI FXbool fxloadICOStream(FXStream& store,FXColor*& data,FXint& width,FXint& height);


// DOS file header
struct ImageDosHeader {
  FXushort   magic;                     // Magic number
  FXushort   cblp;                      // Bytes on last page of file
  FXushort   cp;                        // Pages in file
  FXushort   crlc;                      // Relocations
  FXushort   cparhdr;                   // Size of header in paragraphs
  FXushort   minalloc;                  // Minimum extra paragraphs needed
  FXushort   maxalloc;                  // Maximum extra paragraphs needed
  FXushort   ss;                        // Initial (relative) SS value
  FXushort   sp;                        // Initial SP value
  FXushort   csum;                      // Checksum
  FXushort   ip;                        // Initial IP value
  FXushort   cs;                        // Initial (relative) CS value
  FXushort   lfarlc;                    // File address of relocation table
  FXushort   ovno;                      // Overlay number
  FXushort   res[4];                    // Reserved words
  FXushort   oemid;                     // OEM identifier (for e_oeminfo)
  FXushort   oeminfo;                   // OEM information; e_oemid specific
  FXushort   res2[10];                  // Reserved words
  FXuint     lfanew;                    // File address of new exe header
  };


// Image file header
struct ImageFileHeader {
  FXushort   machine;                   // CPU architecture
  FXushort   numberOfSections;          // Number of sections in section table
  FXuint     timeDateStamp;             // Date and time of program link
  FXuint     pointerToSymbolTable;      // RVA of symbol table
  FXuint     numberOfSymbols;           // Number of symbols in table
  FXushort   sizeOfOptionalHeader;      // Size of IMAGE_OPTIONAL_HEADER in bytes
  FXushort   characteristics;           // Flag bits
  };


// Portable executable file header
struct ImageNTHeader {
  FXuint          signature;
  ImageFileHeader fileHeader;
  };


// Data Directory
struct ImageDataDirectory {
  FXuint     virtualAddress;            // RVA of table
  FXuint     size;                      // Size of table
  };


// Optional Header
struct ImageOptionalHeader32 {
  FXushort   magic;                     // IMAGE_NT_OPTIONAL_HDR32_MAGIC
  FXuchar    majorLinkerVersion;        // Linker version
  FXuchar    minorLinkerVersion;
  FXuint     sizeOfCode;                // Size of .text in bytes
  FXuint     sizeOfInitializedData;     // Size of .bss (and others) in bytes
  FXuint     sizeOfUninitializedData;   // Size of .data,.sdata etc in bytes
  FXuint     addressOfEntryPoint;       // RVA of entry point
  FXuint     baseOfCode;                // Base of .text
  FXuint     baseOfData;                // Base of .data
  FXuint     imageBase;                 // Image base VA
  FXuint     sectionAlignment;          // File section alignment
  FXuint     fileAlignment;             // File alignment
  FXushort   majorOperatingSystemVersion;
  FXushort   minorOperatingSystemVersion;
  FXushort   majorImageVersion;         // Version of program
  FXushort   minorImageVersion;
  FXushort   majorSubsystemVersion;     // Windows specific. Version of SubSystem
  FXushort   minorSubsystemVersion;
  FXuint     win32VersionValue;
  FXuint     sizeOfImage;               // Size of image in bytes
  FXuint     sizeOfHeaders;             // Size of headers (and stub program) in bytes
  FXuint     checksum;
  FXushort   subsystem;
  FXushort   dllCharacteristics;        // DLL properties
  FXuint     sizeOfStackReserve;        // Size of stack, in bytes
  FXuint     sizeOfStackCommit;         // Size of stack to commit
  FXuint     sizeOfHeapReserve;         // Size of heap, in bytes
  FXuint     sizeOfHeapCommit;          // Size of heap to commit
  FXuint     loaderFlags;
  FXuint     numberOfRvaAndSizes;       // Number of entries in dataDirectory
  ImageDataDirectory dataDirectory[16];
  };


// Optional Header
struct ImageOptionalHeader64 {
  FXushort   magic;                     // IMAGE_NT_OPTIONAL_HDR64_MAGIC
  FXuchar    majorLinkerVersion;        // Linker version
  FXuchar    minorLinkerVersion;
  FXuint     sizeOfCode;                // Size of .text in bytes
  FXuint     sizeOfInitializedData;     // Size of .bss (and others) in bytes
  FXuint     sizeOfUninitializedData;   // Size of .data,.sdata etc in bytes
  FXuint     addressOfEntryPoint;       // RVA of entry point
  FXuint     baseOfCode;                // Base of .text
  FXulong    imageBase;                 // Base of .data
  FXuint     sectionAlignment;          // File section alignment
  FXuint     fileAlignment;             // File alignment
  FXushort   majorOperatingSystemVersion;
  FXushort   minorOperatingSystemVersion;
  FXushort   majorImageVersion;         // Version of program
  FXushort   minorImageVersion;
  FXushort   majorSubsystemVersion;
  FXushort   minorSubsystemVersion;
  FXuint     win32VersionValue;
  FXuint     sizeOfImage;               // Size of image in bytes
  FXuint     sizeOfHeaders;             // Size of headers (and stub program) in bytes
  FXuint     checksum;
  FXushort   subsystem;
  FXushort   dllCharacteristics;        // DLL properties
  FXulong    sizeOfStackReserve;        // Size of stack, in bytes
  FXulong    sizeOfStackCommit;         // Size of stack to commit
  FXulong    sizeOfHeapReserve;         // Size of heap, in bytes
  FXulong    sizeOfHeapCommit;          // Size of heap to commit
  FXuint     loaderFlags;
  FXuint     numberOfRvaAndSizes;       // Number of entries in dataDirectory
  ImageDataDirectory dataDirectory[16];
  };


// Optional header
union ImageOptionalHeader {
  ImageOptionalHeader32 pe32;           // 32 bit version
  ImageOptionalHeader64 pe64;           // 64 bit version
  };


// Section header
struct ImageSectionHeader {
  FXchar     name[8];                   // Name, e.g. .text
  FXuint     virtualSize;               // Virtual size (may be bigger than sizeOfRawData)
  FXuint     virtualAddress;            // Offset of first byte relative to imageBase
  FXuint     sizeOfRawData;             // Size of raw data
  FXuint     pointerToRawData;          // Pointer to raw data in COFF
  FXuint     pointerToRelocations;
  FXuint     pointerToLinenumbers;
  FXushort   numberOfRelocations;
  FXushort   numberOfLinenumbers;
  FXuint     characteristics;
  };


// Resource Name (8-bit characters)
struct ImageResourceDirectoryString {
  FXushort   length;
  FXchar     nameString[1];
  };


// Resource Name (16-bit characters)
struct ImageResourceDirectoryStringW {
  FXushort   length;
  FXnchar    nameString[1];
  };


// Resource directory entry
struct ImageResourceDirectoryEntry {
  FXuint name;
  FXuint data;
  };


// Resource directory
struct ImageResourceDirectory {
  FXuint     characteristics;
  FXuint     timeDateStamp;
  FXushort   majorVersion;
  FXushort   minorVersion;
  FXushort   numberOfNamedEntries;
  FXushort   numberOfIdEntries;
  };


// Resource directory entry
struct ImageResourceDataEntry {
  FXuint     offsetToData;
  FXuint     size;
  FXuint     codePage;
  FXuint     reserved;
  };


// Resource loader context
struct Context {
  FXuint pointer;               // Pointer in pe
  FXuint address;               // Relocated virtual address
  FXuint size;                  // Size of resource
  FXint  wanted[MAXRECLEVEL];   // Items to match at each level
  };

/*******************************************************************************/

// Check if stream represents windows executable
FXbool fxcheckEXE(FXStream& store){
  FXuchar ss[2];
  store.load(ss,2);
  store.position(-2,FXFromCurrent);
  return ss[0]=='M' && ss[1]=='Z';
  }


// Scan the data
static FXbool scandata(FXStream& store,FXColor*& data,FXint& width,FXint& height,const Context& ctx,FXlong start){
  ImageResourceDataEntry res;
  FXlong pos=store.position();
  FXbool result=false;

  FXTRACE((100,"scandata: %llu (%#llx)\n",start,start));

  // Read resource data entry, at start
  store.position(start);
  store >> res.offsetToData;
  store >> res.size;
  store >> res.codePage;
  store >> res.reserved;

  FXTRACE((100,"res.offsetToData: %u (%#x)\n",res.offsetToData,res.offsetToData));
  FXTRACE((100,"res.size: %u\n",res.size));
  FXTRACE((100,"res.codePage: %u\n",res.codePage));
  FXTRACE((100,"res.reserved: %u\n",res.reserved));

  // Bail if at end
  if(store.eof()) goto x;

  // Now try read the image, or icon
  store.position(ctx.pointer+res.offsetToData-ctx.address);
  if(ctx.wanted[0]==RST_BITMAP){
    result=fxloadDIB(store,data,width,height);
    }
  else if(ctx.wanted[0]==RST_ICON){
    result=fxloadICOStream(store,data,width,height);
    }
  FXTRACE((100,"result: %u\n",result));
x:store.position(pos);
  return result;
  }


// Scan resource directory until we hit the jackpot
static FXbool scanresources(FXStream& store,FXColor*& data,FXint& width,FXint& height,const Context& ctx,FXlong start,FXint lev){
  FXbool result=false;
  if(ctx.pointer<=start && start<ctx.pointer+ctx.size && lev<MAXRECLEVEL){
    ImageResourceDirectory dir;
    FXlong pos=store.position();

    // Jump to start of directory
    store.position(start);
    store >> dir.characteristics;
    store >> dir.timeDateStamp;
    store >> dir.majorVersion;
    store >> dir.minorVersion;
    store >> dir.numberOfNamedEntries;
    store >> dir.numberOfIdEntries;

    FXTRACE((100,"%*sdir.characteristics: %#08x\n",lev<<2,"",dir.characteristics));
    FXTRACE((100,"%*sdir.timeDateStamp: %u\n",lev<<2,"",dir.timeDateStamp));
    FXTRACE((100,"%*sdir.majorVersion: %u\n",lev<<2,"",dir.majorVersion));
    FXTRACE((100,"%*sdir.minorVersion: %u\n",lev<<2,"",dir.minorVersion));
    FXTRACE((100,"%*sdir.numberOfNamedEntries: %u\n",lev<<2,"",dir.numberOfNamedEntries));
    FXTRACE((100,"%*sdir.numberOfIdEntries: %u\n\n",lev<<2,"",dir.numberOfIdEntries));

    // We're still good?
    if(!store.eof()){
      ImageResourceDirectoryEntry entry;
      FXint want=ctx.wanted[lev];

      // Loop over the entries
      for(FXint s=0; !result && s<dir.numberOfNamedEntries+dir.numberOfIdEntries; ++s){

        // Read resource directory entry
        store >> entry.name;
        store >> entry.data;

        // Still good?
        if(store.eof()) goto x;

        FXTRACE((100,"%*sentry.name: %u\n",lev<<2,"",entry.name));

        // If don't care match or matching ID then check the rest, otherwise move on to next
        if(want==-1 || (!(entry.name&IMAGE_RESOURCE_NAME_IS_STRING) && (entry.name&0xFFFF)==want)){
          if(entry.data&IMAGE_RESOURCE_DATA_IS_DIRECTORY){
            entry.data&=~IMAGE_RESOURCE_DATA_IS_DIRECTORY;
            result=scanresources(store,data,width,height,ctx,ctx.pointer+entry.data,lev+1);
            }
          else{
            result=scandata(store,data,width,height,ctx,ctx.pointer+entry.data);
            }
          }
        }
      }

    // Restore
x:  store.position(pos);
    }
  return result;
  }


// Load resource from executable or dll
FXbool fxloadEXE(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint type,FXint id){
  FXbool result=false;

  FXTRACE((100,"fxloadEXE(data,width,height,type:%d,id:%d)\n\n",type,id));

  // Null out
  data=nullptr;
  width=0;
  height=0;

  // Loading other than these is not gonna work
  if(type==RST_BITMAP || type==RST_ICON){
    FXbool swap=store.swapBytes();
    ImageDosHeader dos;

    // Bitmaps are little-endian
    store.setBigEndian(false);

    // Load DOS header
    store >> dos.magic;                  // must contain "MZ"
    store >> dos.cblp;                   // number of bytes on the last page of the file
    store >> dos.cp;                     // number of pages in file
    store >> dos.crlc;                   // relocations
    store >> dos.cparhdr;                // size of the header in paragraphs
    store >> dos.minalloc;               // minimum and maximum paragraphs to allocate
    store >> dos.maxalloc;
    store >> dos.ss;                     // initial SS:SP to set by Loader
    store >> dos.sp;
    store >> dos.csum;                   // checksum
    store >> dos.ip;                     // initial CS:IP
    store >> dos.cs;
    store >> dos.lfarlc;                 // address of relocation table
    store >> dos.ovno;                   // overlay number
    store.load(dos.res,4);
    store >> dos.oemid;                  // OEM id
    store >> dos.oeminfo;                // OEM info
    store.load(dos.res2,10);
    store >> dos.lfanew;                 // address of new EXE header

    FXTRACE((100,"dos.magic: %#04x\n",dos.magic));
    FXTRACE((100,"dos.cblp: %d\n",dos.cblp));
    FXTRACE((100,"dos.cp: %d\n",dos.cp));
    FXTRACE((100,"dos.crlc: %d\n",dos.crlc));
    FXTRACE((100,"dos.cparhdr: %d\n",dos.cparhdr));
    FXTRACE((100,"dos.minalloc: %d\n",dos.minalloc));
    FXTRACE((100,"dos.maxalloc: %d\n",dos.maxalloc));
    FXTRACE((100,"dos.ss: %d\n",dos.ss));
    FXTRACE((100,"dos.sp: %d\n",dos.sp));
    FXTRACE((100,"dos.csum: %d\n",dos.csum));
    FXTRACE((100,"dos.ip: %d\n",dos.ip));
    FXTRACE((100,"dos.cs: %d\n",dos.cs));
    FXTRACE((100,"dos.lfarlc: %d\n",dos.lfarlc));
    FXTRACE((100,"dos.oemid: %d\n",dos.oemid));
    FXTRACE((100,"dos.oeminfo: %d\n",dos.oeminfo));
    FXTRACE((100,"dos.lfanew: %d\n\n",dos.lfanew));

    // Expect MZ
    if((dos.magic==IMAGE_DOS_SIGNATURE) && !store.eof()){
      ImageNTHeader nt;

      // Skip to NT header
      store.position(dos.lfanew);

      // File header
      store >> nt.signature;
      store >> nt.fileHeader.machine;
      store >> nt.fileHeader.numberOfSections;            // Number of sections in section table
      store >> nt.fileHeader.timeDateStamp;               // Date and time of program link
      store >> nt.fileHeader.pointerToSymbolTable;        // RVA of symbol table
      store >> nt.fileHeader.numberOfSymbols;             // Number of symbols in table
      store >> nt.fileHeader.sizeOfOptionalHeader;        // Size of IMAGE_OPTIONAL_HEADER in bytes
      store >> nt.fileHeader.characteristics;

      // Dump NT header
      FXTRACE((100,"nt.signature: %#04x\n",nt.signature));
      FXTRACE((100,"nt.fileHeader.machine: %4x\n",nt.fileHeader.machine));
      FXTRACE((100,"nt.fileHeader.numberOfSections: %u\n",nt.fileHeader.numberOfSections));
      FXTRACE((100,"nt.fileHeader.timeDateStamp: %u\n",nt.fileHeader.timeDateStamp));
      FXTRACE((100,"nt.fileHeader.pointerToSymbolTable: %u\n",nt.fileHeader.pointerToSymbolTable));
      FXTRACE((100,"nt.fileHeader.numberOfSymbols: %u\n",nt.fileHeader.numberOfSymbols));
      FXTRACE((100,"nt.fileHeader.sizeOfOptionalHeader: %u\n",nt.fileHeader.sizeOfOptionalHeader));
      FXTRACE((100,"nt.fileHeader.characteristics: %#04x\n\n",nt.fileHeader.characteristics));

      // Check NT signature
      if((nt.signature==IMAGE_NT_SIGNATURE) && !store.eof()){
        FXushort optionalheadermagic;

        // Start of optional header here
        FXlong optbase=store.position();

        // Read magic number
        store >> optionalheadermagic;

        // Check if its PE or PE+
        if((optionalheadermagic==IMAGE_NT_OPTIONAL_HDR32_MAGIC || optionalheadermagic==IMAGE_NT_OPTIONAL_HDR64_MAGIC) && !store.eof()){
          ImageSectionHeader sec;

          // Skip over to section headers
          store.position(optbase+nt.fileHeader.sizeOfOptionalHeader);

          // Hunt for the .rsrc section now
          for(FXint s=0; s<nt.fileHeader.numberOfSections; ++s){

            // Load section header info
            store.load(sec.name,sizeof(sec.name));
            store >> sec.virtualSize;
            store >> sec.virtualAddress;
            store >> sec.sizeOfRawData;
            store >> sec.pointerToRawData;
            store >> sec.pointerToRelocations;
            store >> sec.pointerToLinenumbers;
            store >> sec.numberOfRelocations;
            store >> sec.numberOfLinenumbers;
            store >> sec.characteristics;

            // Bail if at end
            if(store.eof()) break;

            FXTRACE((100,"sec%d.name: %.8s\n",s,sec.name));
            FXTRACE((100,"sec%d.virtualSize: %u\n",s,sec.virtualSize));
            FXTRACE((100,"sec%d.virtualAddress: %u\n",s,sec.virtualAddress));
            FXTRACE((100,"sec%d.sizeOfRawData: %u\n",s,sec.sizeOfRawData));
            FXTRACE((100,"sec%d.pointerToRawData: %u (%#08x)\n",s,sec.pointerToRawData,sec.pointerToRawData));
            FXTRACE((100,"sec%d.pointerToRelocations: %u\n",s,sec.pointerToRelocations));
            FXTRACE((100,"sec%d.pointerToLinenumbers: %u\n",s,sec.pointerToLinenumbers));
            FXTRACE((100,"sec%d.numberOfRelocations: %u\n",s,sec.numberOfRelocations));
            FXTRACE((100,"sec%d.numberOfLinenumbers: %u\n",s,sec.numberOfLinenumbers));
            FXTRACE((100,"sec%d.characteristics: %#08x\n\n",s,sec.characteristics));

            // Found the resource section in the pe file
            if(FXString::compare(sec.name,".rsrc")==0){
              Context context={sec.pointerToRawData,sec.virtualAddress,sec.sizeOfRawData,{type,id,-1}};

              // Scan resources in resource section
              result=scanresources(store,data,width,height,context,sec.pointerToRawData,0);
              break;
              }
            }
          }
        }
      }
    store.swapBytes(swap);
    }
  return result;
  }


}
