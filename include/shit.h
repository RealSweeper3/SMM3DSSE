#include <3ds.h>

extern "C"
{
#include "leveltools.h"
}

namespace MM
{
    namespace SHIT
    {
        class Progress
        {
        public:
            u8* bufptr;
            
            u32* checksum;
            u64* LockoutID;
            u8* slots;
            
            
            Progress(u8* ptr)
            {
                bufptr = ptr;
                
                checksum = (u32*)ptr;
                LockoutID = (u64*)(ptr + 0x10);
                slots = (u8*)(ptr + 0x2268);
            }
            
            ~Progress()
            {
                delete[] bufptr;
            }
            
            void FixChecksum()
            {
                //*(u32*)(bufptr + 0x20) = crc32(bufptr + 0x28, 0x4760);
                *checksum = addcrc((u16*)(bufptr + 0x18), FILESIZE_PROGRESS - 0x18, ADDIFF_PROGRESS);
            }
        };
    }
}
