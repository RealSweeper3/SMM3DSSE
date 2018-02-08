#include <3ds.h>

#include "leveltools.h"

#define SWAP16(o) *(u16*)(bufptr + o) = __builtin_bswap16(*(u16*)(bufptr + o))
#define SWAP32(o) *(u32*)(bufptr + o) = __builtin_bswap32(*(u32*)(bufptr + o))
#define SWAP64(o) *(u64*)(bufptr + o) = __builtin_bswap64(*(u64*)(bufptr + o))

#define SW16(o) course->o = __builtin_bswap16(course->o)
#define SW32(o) course->o = __builtin_bswap32(course->o)
#define SW64(o) course->o = __builtin_bswap64(course->o)

u32 crc32(const void* inptr, u32 i, u32 n)
{
    const u8* ptr = inptr;
    while(i--)
    {
        n = n ^ *ptr++;
        int j;
        for(j = 0; j != 8; j++)
            if(n & 1)
                n = (n >> 1) ^ 0xEDB88320; 
            else
                n >>= 1;
    }
    
    return n;
}

u32 addcrc(const void* inptr, u32 i, u32 diff)
{
    const u16* ptr = inptr;
    i >>= 1;
    u32 hash = 1;
    while(i--) hash += *(ptr++);
    return diff + hash;
}

void lt_chend(MM_Course* course)
{
    u8* bufptr = course;
    
    int i = 0;
    
    SWAP32(0x04);
    SWAP16(0x10);
    
    //SWAP32(0x1C);
    *(u64*)(bufptr + 0x18) = 0; //TODO: unbodge
    
    bufptr[0x22] = 0; //TODO: fix logic
    
    //*(u64*)(bufptr + 0x20) = 0; //TODO: figure out these flags more
    
    SWAP32(0x24);
    
    for(i = 0x28; i != 0x6A; i += 2)
        SWAP16(i);
    
    SWAP16(0x70);
    SWAP32(0x74);
    
    SWAP32(0xD8);
    SWAP32(0xDC);
    *(u64*)(bufptr + 0xE0) = 0; //TODO: fix me
    *(u32*)(bufptr + 0xE8) = 0; //TODO: fix me
    //SWAP32(0xE8);
    SWAP32(0xEC);
    
    for(i = 0xF0; i != 0x145F0; i += 0x20)
    {
        SWAP32(i + 0x0);
        SWAP32(i + 0x4);
        SWAP16(i + 0x8);
        SWAP32(i + 0xC);
        SWAP32(i + 0x10);
        SWAP32(i + 0x14);
        SWAP16(i + 0x1A);
        SWAP16(i + 0x1C);
    }
    
    /*for(i = 0x145F0; i != 0x14F50; i += 8)
    {
        SWAP16(i + 2);
        SWAP16(i + 6);
    }*/
    
    /*MM_Effect sfx;
    sfx.type = 0xFF;
    sfx.unknown0 = 0xFF;
    sfx.subtype = 0;
    sfx.x = 0xFF;
    sfx.z = 0xFF;
    sfx.unknown1[0] = 0;
    sfx.unknown1[1] = 0;
    sfx.unknown1[2] = 0;
    
    for(i = 0; i != 300; i++)
    {
        memcpy(&course->sfx[i], &sfx, 8);
    }*/
    
    //memset(bufptr + 0x145F0, 0, 300 * 8);
    
    *(u64*)(bufptr + 0x14F50) = 0; //TODO: fix logic
    *(u64*)(bufptr + 0x14F58) = 0; //TODO: fix logic
    
    memset(bufptr + 0x14F60, 0, 0xA0);
}

void lt_mkle(MM_Course* course)
{
    u8* ptr = course;
    if(!*(u16*)(ptr + 4)) lt_chend(ptr);
    *(u32*)(ptr + 8) = ~crc32(ptr + 0x10, 0x15000 - 0x10, 0xFFFFFFFF);
}

void lt_mkbe(MM_Course* course)
{
    u8* ptr = course;
    if(!*(u16*)(ptr + 6)) lt_chend(ptr);
    *(u32*)(ptr + 8) = __builtin_bswap32(crc32(ptr + 0x10, 0x15000 - 0x10, 0));
}

void lt_parsele(LevelEntry* e, u8 cy, u8* bufptr)
{
    
    if(cy == 220)
    {
        strcpy(e->lvlname, "<LockoutDummy220>");
        strcpy(e->miin, "Nintendo");
        strcpy(e->miia, "Nintendo");
        e->code2 = -1;
        e->nth = cy;
        e->lock = -1ULL;
    }
    else 
    {
        memset(e->lvlname, 0, sizeof(e->lvlname));
        memset(e->miin, 0, sizeof(e->miin));
        memset(e->miia, 0, sizeof(e->miia));
        utf16_to_utf8(e->lvlname, bufptr + 0x44, sizeof(e->lvlname) - 1);
        utf16_to_utf8(e->miin, bufptr + 0xAE, sizeof(e->miin) - 1);
        utf16_to_utf8(e->miia, bufptr + 0xDC, sizeof(e->miia) - 1);
        e->code2 = *(u32*)(bufptr + 0x38);
        e->nth = cy;
        e->lock = *(u64*)(bufptr + 0x10);
    }
}


