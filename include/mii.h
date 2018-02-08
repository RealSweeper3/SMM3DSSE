#pragma once

typedef struct
{
    u32 MiiID;                // 0000
    u64 SysID;                // 0004
    u32 MiiDate;              // 000C
    u8 MAC[6];                // 0010
    u16 padding_zero0;        // 0016
    u16 Mii_opt;              // 0018
    u16 name[10];             // 001A
    u16 size;                 // 002E
    u8 design[0x10];          // 0030
    u8 copying;               // 0040
    u8 padding_zero1[7];      // 0041
    u16 authorname[10];       // 0048
    u32 something;            // 005C
} __attribute__((packed)) Mii;// 0060
