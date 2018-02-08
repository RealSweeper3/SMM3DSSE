#include <cstdlib>
#include <sfse.h>
#include "shit.h"

using MM::SHIT::Progress;

SaveEditor se;

void edit_lives() {
    SwkbdState swkbd;
    char lives[4];
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
    swkbdInputText(&swkbd, lives, sizeof(lives));
    int converted = std::atoi(lives);
    if (converted > 100) {
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
    printf("Fixing checksum...");
    se.Position = 0;
    u8 *bytes = se.ReadBytes(FILESIZE_PROGRESS);
    Progress p(bytes);
    p.FixChecksum();
    se.Position = 0;
    se.WriteBytes(p.bufptr, FILESIZE_PROGRESS);
}

void emptyfunc() {}

int main(int argc, char **argv) {
  gfxInitDefault();
  consoleInit(GFX_TOP, NULL);
  se = SaveEditor("/Progress", Endian::Little);
  if (!se.FileOpen()) {
    printf("Progress not found in SD card!\n");
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
  sfse_menu menu("Super Mario Maker for Nintendo 3DS Save Editor\nSelect a option.");
  menu.AddOption(sfse_menu_option{"Edit Lives", edit_lives});
  menu.AddOption(sfse_menu_option{"Exit", emptyfunc});
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
