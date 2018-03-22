#include <cstdlib>
#include <sfse.h>

extern "C" {
#include "crc.h"
}

SFSE::SaveEditor se;

void FixChecksum() {
  se.Position = 0;
  u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
  u32 *checksum = (u32 *)bytes;
  *checksum =
      addcrc((u16 *)(bytes + 0x18), FILESIZE_PROGRESS - 0x18, ADDIFF_PROGRESS);
  se.Position = 0;
  se.WriteBytes(bytes, FILESIZE_PROGRESS);
}

void all_items() {
  se.Position = 0x4252;
  se.WriteInt8(19);
  FixChecksum();
}

void all_medals() {
  for (int i = 0; i <= 99; i++) {
    se.Position = 0x4700 + i;
    se.WriteInt8(7);
  }
  FixChecksum();
}

void backup_medals() {
  SFSE::SaveEditor medals("/medals.bin", Endian::Little);
  if (!medals.FileOpen()) {
    FS_Archive arch;
    FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    FSUSER_CreateFile(
        arch, fsMakePath(PATH_ASCII, std::string("/medals.bin").data()), 0, 99);
    FSUSER_CloseArchive(arch);
    medals = SFSE::SaveEditor("/medals.bin", Endian::Little);
  }
  se.Position = 0x4700;
  medals.Position = 0;
  medals.WriteBytes(se.ReadBytes(99), 99);
  medals.CloseFile();
}

void restore_medals() {
  SFSE::SaveEditor medals("/medals.bin", Endian::Little);
  if (!medals.FileOpen()) {
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
  se.Position = 0x4700;
  medals.Position = 0;
  se.WriteBytes(medals.ReadBytes(99), 99);
  medals.CloseFile();
  FixChecksum();
}

void edit_lives() {
  SwkbdState swkbd;
  char lives[4];
  swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
  swkbdInputText(&swkbd, lives, sizeof(lives));
  s8 converted = (s8)std::atoll(lives);
  if (converted == 0 || converted > 100) {
    printf("Invalid lives value!");
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
  se.Position = 0x4250;
  se.WriteInt8(converted);
  FixChecksum();
}

int main(int argc, char **argv) {
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  SFSE::Menu menu_region("Select a option.");
  menu_region.AddOption(SFSE::MenuOption{"EUR", nullptr});
  menu_region.AddOption(SFSE::MenuOption{"USA", nullptr});
  menu_region.AddOption(SFSE::MenuOption{"JPN", nullptr});
  menu_region.AddOption(SFSE::MenuOption{"KOR", nullptr});
  int region = menu_region.GetOption();
  u64 title_ids[] = {0x00040000001A0500, 0x00040000001A0400, 0x00040000001A0300,
                     0x00040000001BB800};
  se = SFSE::SaveEditor(title_ids[region], "/Progress", Endian::Little);
  if (!se.FileOpen()) {
    printf("Save not found!\n");
    printf("Press START to exit.\n");
    while (aptMainLoop()) {
      hidScanInput();
      if (hidKeysDown() & KEY_START)
        break;
      gfxFlushBuffers();
      gfxSwapBuffers();
      gspWaitForVBlank();
    }
    return -1;
  }
  SFSE::Menu menu(
      "Super Mario Maker for Nintendo 3DS Save Editor\nSelect a option.");
  menu.AddOption(SFSE::MenuOption{"All medals", all_medals});
  menu.AddOption(SFSE::MenuOption{"Backup medals", backup_medals});
  menu.AddOption(SFSE::MenuOption{"Restore medals", restore_medals});
  menu.AddOption(SFSE::MenuOption{"Edit lives", edit_lives});
  menu.AddOption(SFSE::MenuOption{"Unlock all items", all_items});
  menu.AddOption(SFSE::MenuOption{"Exit", nullptr});
  while (aptMainLoop()) {
    int option = menu.GetOption();
    if (option == menu.OptionCount() - 1)
      break;
    menu.functions[option]();
  }
  se.CloseFile();
  gfxExit();
  return 0;
}
