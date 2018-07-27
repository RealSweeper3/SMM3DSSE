#include <cstdlib>
#include "ctrlib.hpp"

extern "C" {
#include "crc.h"
}

struct Impl {
    int Start() {
        ctr::ui::Init();
        ctr::ui::Menu menu_region("Select a option.");
        menu_region.AddOption("EUR");
        menu_region.AddOption("USA");
        menu_region.AddOption("JPN");
        menu_region.AddOption("KOR");
        size_t region = menu_region.GetOption();
        u64 title_ids[] = {0x00040000001A0500, 0x00040000001A0400, 0x00040000001A0300,
                           0x00040000001BB800};
        ctr::fs::File file(title_ids[region], "/Progress");
        if (!file.IsOpen()) {
            ctr::ui::Show("Save not found!\nPress START to exit.", KEY_START);
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
                AllMedals(file);
                break;
            case 1:
                BackupMedals(file);
                break;
            case 2:
                RestoreMedals(file);
                break;
            case 3:
                EditLives(file);
                break;
            case 4:
                AllItems(file);
                break;
            case 5:
                exit_ = true;
                break;
            }
        }
        file.Close();
        ctr::ui::Exit();
    }

    void FixChecksum(ctr::fs::File& file) {
        file.SetOffset(0);
        u8* bytes = file.ReadBytes(FILESIZE_PROGRESS);
        u32* checksum = (u32*)bytes;
        *checksum = addcrc((u16*)(bytes + 0x18), FILESIZE_PROGRESS - 0x18, ADDIFF_PROGRESS);
        file.SetOffset(0);
        file.WriteBytes(bytes, FILESIZE_PROGRESS);
    }

    void AllItems(ctr::fs::File& file) {
        file.SetOffset(0x4252);
        file.Write<s8>(19);
        FixChecksum(file);
    }

    void AllMedals(ctr::fs::File& file) {
        for (int i = 0; i <= 99; ++i) {
            file.SetOffset(0x4700 + i);
            file.Write<s8>(7);
        }
        FixChecksum(file);
    }

    void BackupMedals(ctr::fs::File& file) {
        ctr::fs::File medals("/medals.bin");
        if (!medals.IsOpen())
            medals.Create(99);
        file.SetOffset(0x4700);
        medals.SetOffset(0);
        medals.WriteBytes(file.ReadBytes(99), 99);
        medals.Close();
    }

    void RestoreMedals(ctr::fs::File& file) {
        ctr::fs::File medals("/medals.bin");
        if (!medals.IsOpen()) {
            ctr::ui::Show("medals.bin not found!", KEY_A);
            return;
        }
        file.SetOffset(0x4700);
        medals.SetOffset(0);
        file.WriteBytes(medals.ReadBytes(99), 99);
        medals.Close();
        FixChecksum(file);
    }

    void EditLives(ctr::fs::File& file) {
        SwkbdState swkbd;
        char lives[4];
        swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
        swkbdInputText(&swkbd, lives, sizeof(lives));
        int i = std::atoi(lives);
        s8 converted = static_cast<s8>((i > 127) ? 0 : i);
        if ((converted == 0) || (converted > 100)) {
            ctr::ui::Show("Invalid lives value!", KEY_A);
            return;
        }
        file.SetOffset(0x4250);
        file.Write<s8>(converted);
        FixChecksum(file);
    }
};

int main(int argc, char** argv) {
    Impl impl;
    return impl.Start();
}
