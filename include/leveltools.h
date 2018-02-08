#pragma once

#include "mii.h"

#define FILESIZE_COURSE 0x4301C
#define FILESIZE_PROGRESS 0x8018
#define FILESIZE_DUMMY 0x1C1C

#define ADDIFF_COURSE 0x4F014
#define ADDIFF_PROGRESS 0x8000
#define ADDIFF_DUMMY 0x8C04

//below struct member names were found out by @smb123w64gb
typedef struct
{
    u8 type;                  // 0000
    u8 unknown0;              // 0001
    u8 subtype;               // 0002
    u8 x;                     // 0003
    u8 z;                     // 0004
    u8 unknown1[3];           // 0005
} MM_Effect;

typedef struct
{
    u32 x;                    // 0000
    u32 z;                    // 0004
    u16 y;                    // 0008
    u8 w;                     // 000A
    u8 h;                     // 000B
    u32 flags;                // 000C
    u32 childflags;           // 0010
    u32 something;            // 0014
    u8 type;                  // 0018
    u8 child;                 // 0019
    u16 link;                 // 001A
    u16 effect;               // 001C
    u8 unknown;               // 001E
    u8 childtrans;            // 001F
} __attribute__((packed)) MM_Tile;

typedef struct
{
    u32 unk_zero;             // 0000
    u32 filever;              // 0004
    u32 crc;                  // 0008
    u32 padding_zero0;        // 000C
    u16 f_year;               // 0010
    u8 f_month;               // 0012
    u8 f_day;                 // 0013
    u8 f_hour;                // 0014
    u8 f_minute;              // 0015
    u8 levelunk1;             // 0016
    u8 levelunk2;             // 0017
    u64 levelcode;            // 0018
    u8 unknown1;              // 0020
    u8 unknown2;              // 0021
    u8 ThumbnailFlags;        // 0022
    u8 ForeignUserFlag;       // 0023
    u32 unknown3;             // 0024 // effect counter ?
    u16 lvlname[0x21];        // 0028
    u16 theme;                // 006A // "M1", "M3", "MW", "WU"
    u8 padding_zero1;         // 006C
    u8 leveltype;             // 006D
    u8 unknown4;              // 006E //mostly 00; one level has 01
    u8 unknownflag;           // 006F //v0 is 2 or 3 but not 4, this is set to 1
    u16 lvltime;              // 0070
    u8 scroll;                // 0072
    u8 flags;                 // 0073
    u32 lvlsize;              // 0074
    Mii creator;              // 0078
    u32 creator_country;      // 00D8
    u32 unknown5;             // 00DC
    u32 unknown_ID;           // 00E0
    u32 unknown6;             // 00E4
    u32 unknown7;             // 00E8
    u32 numtiles;             // 00EC
    MM_Tile tiles[2600];      // 00F0 //how fucking generous
    MM_Effect sfx[300];       //145F0 //again, so fucking generous ¬_¬
    u64 CompleteSeed[2];      //14F50 //cfgGetTransferableID(666 e1); [owner, guest]
    u8 unknown_padding[0xA0]; //14F60
} __attribute__((packed)) MM_Course;//15000



typedef struct
{
    u32 checksum;
    u32 unknown1[3];
    u64 LockoutID;
} __attribute__((packed)) MM_LockoutHdr;

typedef struct
{
    u32 crc;                  // 0000
    u32 version;              // 0004
    u8 dummy[0x38];           // 0008
    u8 data[0x15780];         // 0040
    //TODO: more detailed data
} __attribute__((packed)) MM_ThumbnailCTR;//157C0

typedef struct
{
    u32 crc;                  // 0000
    u32 size;                 // 0004
    u8 data[0xC7F8];          // 0008
} __attribute__((packed)) MM_ThumbnailWUP;// C800

typedef struct
{
    MM_LockoutHdr hdr;        // 
    u32 dummypadding;         // This has sizeof 0x1C
    MM_Course course;         // 0000
    MM_Course subcourse;      //15000
    //TODO: thumbnail union
    union                     //2A000
    {
        MM_ThumbnailCTR ctr;
        MM_ThumbnailWUP wup[2];
        u8 thumbnail_raw[0x19000];
    } thumbnail;
} __attribute__((packed)) MM_CourseFile;//43000



typedef struct
{
    int nth;
    char lvlname[0x41];
    char miin[13];
    char miia[13];
    u32 code2;
    u64 lock;
} LevelEntry;

u32 crc32(const void* inptr, u32 i, u32 n);
u32 addcrc(const void* inptr, u32 i, u32 n);
void lt_chend(MM_Course* course);
void lt_mkle(MM_Course* course);
void lt_mkbe(MM_Course* course);
void lt_parsele(LevelEntry* le, u8 cy, u8* ptr);
