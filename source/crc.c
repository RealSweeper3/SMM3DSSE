#include <3ds.h>
#include "crc.h"

u32 addcrc(const void* inptr, u32 i, u32 diff) {
    const u16* ptr = inptr;
    i >>= 1;
    u32 hash = 1;
    while (i--)
        hash += *(ptr++);
    return diff + hash;
}
