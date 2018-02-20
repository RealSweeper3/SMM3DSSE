#include <cstdlib>
#include <sfse.h>
#include "shit.h"

using MM::SHIT::Progress;

SaveEditor se;

u64 ids[] = {0x00040000001A0500, 0x00040000001A0400, 0x00040000001A0300, 0x00040000001BB800};

void all_items() {
  se.Position = 0x4252;
  se.WriteInt8(19);
  se.Position = 0;
  u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
  Progress p(bytes);
  p.FixChecksum();
  se.Position = 0;
  se.WriteBytes(p.bufptr, FILESIZE_PROGRESS);
}

void all_medals() {
  for (int i = 0; i <= 99; i++) {
    se.Position = 0x4700 + i;
    se.WriteInt8(7);
  }
  se.Position = 0;
  u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
  Progress p(bytes);
  p.FixChecksum();
  se.Position = 0;
  se.WriteBytes(p.bufptr, FILESIZE_PROGRESS);
}

void backup_medals() {
  SaveEditor medals("/medals.bin", Endian::Little);
  if (!medals.FileOpen()) {
    FS_Archive arch;
    FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    FSUSER_CreateFile(
        arch, fsMakePath(PATH_ASCII, std::string("/medals.bin").data()), 0, 99);
    FSUSER_CloseArchive(arch);
    medals = SaveEditor("/medals.bin", Endian::Little);
  }
  se.Position = 0x4700;
  medals.Position = 0;
  medals.WriteBytes(se.ReadBytes(99), 99);
  medals.Close();
}

void restore_medals() {
  SaveEditor medals("/medals.bin", Endian::Little);
  if (!medals.FileOpen()) {
    printf("medals.bin not found!");
    while (aptMainLoop()) {
      hidScanInput();
      if (hidKeysDown() & KEY_A)
        break;
    }
    return;
  }
  se.Position = 0x4700;
  medals.Position = 0;
  se.WriteBytes(medals.ReadBytes(99), 99);
  medals.Close();
  se.Position = 0;
  u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
  Progress p(bytes);
  p.FixChecksum();
  se.Position = 0;
  se.WriteBytes(p.bufptr, FILESIZE_PROGRESS);
}

void edit_lives() {
  SwkbdState swkbd;
  char lives[4];
  swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
  swkbdInputText(&swkbd, lives, sizeof(lives));
  int converted = std::atoi(lives);
  if (converted == 0 || converted > 100) {
    printf("Invalid lives value!");
    while (aptMainLoop()) {
      hidScanInput();
      if (hidKeysDown() & KEY_A)
        break;
    }
    return;
  }
  se.Position = 0x4250;
  se.WriteInt8((s8)converted);
  se.Position = 0;
  u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
  Progress p(bytes);
  p.FixChecksum();
  se.Position = 0;
  se.WriteBytes(p.bufptr, FILESIZE_PROGRESS);
}

int main(int argc, char **argv) {
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  sfse_menu mreg(
      "Super Mario Maker for Nintendo 3DS Save Editor\nSelect a option.");
  mreg.AddOption(sfse_menu_option{"EUR", nullptr});
  mreg.AddOption(sfse_menu_option{"USA", nullptr});
  mreg.AddOption(sfse_menu_option{"JPN", nullptr});
  mreg.AddOption(sfse_menu_option{"KOR", nullptr});
  int reg = mreg.GetOption();
  se = SaveEditor(ids[reg], "/Progress", Endian::Little);
  if (!se.FileOpen()) {
    printf("Save not found!\n");
    printf("Press START to exit.\n");
    while (aptMainLoop()) {
      gspWaitForVBlank();
      hidScanInput();
      if (hidKeysDown() & KEY_START)
        break;
      gfxFlushBuffers();
      gfxSwapBuffers();
    }
    return -1;
  }
  sfse_menu menu(
      "Super Mario Maker for Nintendo 3DS Save Editor\nSelect a option.");
  menu.AddOption(sfse_menu_option{"All medals", all_medals});
  menu.AddOption(sfse_menu_option{"Backup medals", backup_medals});
  menu.AddOption(sfse_menu_option{"Restore medals", restore_medals});
  menu.AddOption(sfse_menu_option{"Edit lives", edit_lives});
  menu.AddOption(sfse_menu_option{"Unlock all items", all_items});
  menu.AddOption(sfse_menu_option{"Exit", nullptr});
  while (aptMainLoop()) {
    int option = menu.GetOption();
    if (option == menu.OptionCount() - 1)
      break;
    menu.functions[option]();
  }
  se.Close();
  gfxExit();
  return 0;
}
