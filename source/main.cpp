#include <cstdlib>
#include "ctrlib.hpp"

extern "C" {
#include "crc.h"
}

ctr::fs::File file;

void FixChecksum() {
    file.SetOffset(0);
    u8* bytes = file.ReadBytes(FILESIZE_PROGRESS);
    u32* checksum = (u32*)bytes;
    *checksum = addcrc((u16*)(bytes + 0x18), FILESIZE_PROGRESS - 0x18, ADDIFF_PROGRESS);
    file.SetOffset(0);
    file.WriteBytes(bytes, FILESIZE_PROGRESS);
}

void AllItems() {
    file.SetOffset(0x4252);
    file.WriteInt8(19);
    FixChecksum();
}

void AllMedals() {
    for (int i = 0; i <= 99; ++i) {
        file.SetOffset(0x4700 + i);
        file.WriteInt8(7);
    }
    FixChecksum();
}

void BackupMedals() {
    ctr::fs::File medals("/medals.bin", ctr::fs::Endian::Little);
    if (!medals.IsOpen()) {
        FS_Archive arch;
        FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
        FSUSER_CreateFile(arch, fsMakePath(PATH_ASCII, std::string("/medals.bin").data()), 0, 99);
        FSUSER_CloseArchive(arch);
        medals = ctr::fs::File("/medals.bin", ctr::fs::Endian::Little);
    }
    file.SetOffset(0x4700);
    medals.SetOffset(0);
    medals.WriteBytes(file.ReadBytes(99), 99);
    medals.Close();
}

void RestoreMedals() {
    ctr::fs::File medals("/medals.bin", ctr::fs::Endian::Little);
    if (!medals.IsOpen()) {
        printf("medals.bin not found!");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_A)
                break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        return;
    }
    file.SetOffset(0x4700);
    medals.SetOffset(0);
    file.WriteBytes(medals.ReadBytes(99), 99);
    medals.Close();
    FixChecksum();
}

void EditLives() {
    SwkbdState swkbd;
    char lives[4];
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
    swkbdInputText(&swkbd, lives, sizeof(lives));
    int i = std::atoi(lives);
    s8 converted = static_cast<s8>((i > 127) ? 0 : i);
    if ((converted == 0) || (converted > 100)) {
        printf("Invalid lives value!");
        while (true) {
            hidScanInput();
            if (hidKeysDown() & KEY_A)
                break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        return;
    }
    file.SetOffset(0x4250);
    file.WriteInt8(converted);
    FixChecksum();
}

int main(int argc, char** argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    ctr::ui::Menu menu_region("Select a option.");
    menu_region.AddOption("EUR");
    menu_region.AddOption("USA");
    menu_region.AddOption("JPN");
    menu_region.AddOption("KOR");
    size_t region = menu_region.GetOption();
    u64 title_ids[] = {0x00040000001A0500, 0x00040000001A0400, 0x00040000001A0300,
                       0x00040000001BB800};
    file = ctr::fs::File(title_ids[region], "/Progress", ctr::fs::Endian::Little);
    if (!file.IsOpen()) {
        printf("Save not found!\n");
        printf("Press START to exit.\n");
        while (!(hidKeysDown() & KEY_START)) {
            hidScanInput();
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        return -1;
    }
    ctr::ui::Menu menu("Select a option.");
    menu.AddOption("All medals");
    menu.AddOption("Backup medals");
    menu.AddOption("Restore medals");
    menu.AddOption("Edit lives");
    menu.AddOption("Unlock all items");
    menu.AddOption("Exit");
    bool exit_ = false;
    while (aptMainLoop() && !exit_) {
        size_t option = menu.GetOption();
        switch (option) {
        case 0:
            AllMedals();
            break;
        case 1:
            BackupMedals();
            break;
        case 2:
            RestoreMedals();
            break;
        case 3:
            EditLives();
            break;
        case 4:
            AllItems();
            break;
        case 5:
            exit_ = true;
            break;
        }
    }
    file.Close();
    gfxExit();
    return 0;
}
