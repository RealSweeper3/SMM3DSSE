#include <cstdlib>
#include "ctrlib.hpp"

extern "C" {
#include "crc.h"
}

struct Impl {
    void Start() {
        ctr::ui::Application app(COLOR_DEFAULT, COLOR_DEFAULT);
        ctr::ui::Text* text1 = new ctr::ui::Text(&app, "text1", GFX_BOTTOM, 71, 5, "Select the region of the game.");
        ctr::ui::ComboBox* comboBox1 = new ctr::ui::ComboBox(&app, "comboBox1", 125.260009765625, 21.5, 60, 25, {"EUR", "USA", "JPN", "KOR"});
        ctr::fs::File* file = nullptr;
        auto FixChecksum = [&] {
            file->SetOffset(0);
            u8* bytes = file->ReadBytes(FILESIZE_PROGRESS);
            u32* checksum = (u32*)bytes;
            *checksum = addcrc((u16*)(bytes + 0x18), FILESIZE_PROGRESS - 0x18, ADDIFF_PROGRESS);
            file->SetOffset(0);
            file->WriteBytes(bytes, FILESIZE_PROGRESS);
        };
        auto AllItems = [&] {
            file->SetOffset(0x4252);
            file->Write<s8>(19);
        };
        auto AllMedals = [&] {
            for (int i = 0; i <= 99; ++i) {
                file->SetOffset(0x4700 + i);
                file->Write<s8>(7);
            }
        };
        auto BackupMedals = [&] {
            ctr::fs::File medals("/medals.bin");
            if (!medals.IsOpen())
                medals.Create(99);
            file->SetOffset(0x4700);
            medals.SetOffset(0);
            medals.WriteBytes(file->ReadBytes(99), 99);
            medals.Close();
        };
        auto RestoreMedals = [&] {
            ctr::fs::File medals("/medals.bin");
            if (!medals.IsOpen()) {
                ctr::util::ShowError("medals.bin not found");
                return;
            }
            file->SetOffset(0x4700);
            medals.SetOffset(0);
            file->WriteBytes(medals.ReadBytes(99), 99);
            medals.Close();
        };
        auto EditLives = [&] {
            unsigned long lives = ctr::util::GetNumber("Lives", 3, false);
            if ((lives == 0) || (lives > 100)) {
                ctr::util::ShowError("Invalid lives value");
                return;
            }
            file->SetOffset(0x4250);
            file->Write<s8>(static_cast<s8>(lives));
        };
        ctr::ui::Button* button2 = new ctr::ui::Button(&app, "button2", 1, 1, "All medals", [&] {
            AllMedals();
        }, 100, 30, COLOR_WHITE);
        ctr::ui::Button* button3 = new ctr::ui::Button(&app, "button3", 103, 1, "Backup medals", [&] {
            BackupMedals();
        }, 100, 30, COLOR_WHITE);
        ctr::ui::Button* button4 = new ctr::ui::Button(&app, "button4", 205, 1, "Restore medals", [&] {
            RestoreMedals();
        }, 100, 30, COLOR_WHITE);
        ctr::ui::Button* button5 = new ctr::ui::Button(&app, "button5", 1, 34, "Edit lives", [&] {
            EditLives();
        }, 100, 30, COLOR_WHITE);
        ctr::ui::Button* button6 = new ctr::ui::Button(&app, "button6", 103, 34, "Unlock all items", [&] {
            AllItems();
        }, 100, 30, COLOR_WHITE);
        button2->Hide();
        button3->Hide();
        button4->Hide();
        button5->Hide();
        button6->Hide();
        ctr::ui::Button* button1 = new ctr::ui::Button(&app, "button1", 125.260009765625, 50.5, "OK", [&] {
            u64 title_ids[] = {0x00040000001A0500, 0x00040000001A0400, 0x00040000001A0300,
                               0x00040000001BB800};
            file = new ctr::fs::File(title_ids[comboBox1->GetIndex()], "/Progress");
            text1->Hide();
            comboBox1->Hide();
            button1->Hide();
            if (!file->IsOpen()) {
                ctr::util::ShowError("Save not found");
                app.Exit();
            } else {
                button2->Show();
                button3->Show();
                button4->Show();
                button5->Show();
                button6->Show();
            }
        }, 60, 31, COLOR_WHITE);
        app.AddControls({text1, comboBox1, button1, button2, button3, button4, button5, button6});
        app.Start([&] (u32 buttons) -> bool {
            if (buttons & KEY_START) {
                FixChecksum();
                file->Close();
                return true;
            }
            return false;
        });
    }
};

int main(int argc, char** argv) {
    Impl impl;
    impl.Start();
    return 0;
}
