#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <iterator>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include <3ds.h>

enum class Endian { Big, Little };
enum class Encoding { UTF8, UTF16 };

class SaveEditor {
public:
  u64 Position;
  SaveEditor() {}
  SaveEditor(std::string path, Endian e) {
    FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    Result res =
        FSUSER_OpenFile(&fileHandle, arch, fsMakePath(PATH_ASCII, path.data()),
                        FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if (R_SUCCEEDED(res)) {
      open = true;
      FSFILE_GetSize(fileHandle, &fileSize);
      _endian = e;
    } else {
      open = false;
    }
  }

  void Close() {
    if (open) {
      FSFILE_Close(fileHandle);
      FSUSER_CloseArchive(arch);
      open = false;
    }
  }

  bool FileOpen() { return open; }
  u64 FileSize() { return fileSize; }

  s8 ReadInt8() {
    s8 b;
    FSFILE_Read(fileHandle, NULL, Position, &b, 1);
    Position++;
    return b;
  }

  u8 ReadUInt8() {
    u8 b;
    FSFILE_Read(fileHandle, NULL, Position, &b, 1);
    Position++;
    return b;
  }

  u8 *ReadBytes(int count) {
    u8 *bytes = new u8[count];
    for (int i = 0; i < count; i++)
      bytes[i] = ReadUInt8();
    if (_endian == Endian::Big) {
      u8 c;
      u8 *s0 = bytes - 1;
      u8 *s1 = bytes;
      while (*s1)
        ++s1;
      while (s1-- > ++s0) {
        c = *s0;
        *s0 = *s1;
        *s1 = c;
      }
    }
    return bytes;
  }

  bool ReadBool() {
    s8 b = ReadInt8();
    if (b == 1)
      return true;
    return false;
  }

  s16 ReadInt16() {
    s8 b1, b2;
    b1 = ReadInt8();
    b2 = ReadInt8();
    return ((b2 ^ 128) - 128) * 256 + b1;
  }

  u16 ReadUInt16() {
    u8 b1, b2;
    b1 = ReadUInt8();
    b2 = ReadUInt8();
    return ((b2 ^ 128) - 128) * 256 + b1;
  }

  s32 ReadInt32() {
    s8 b1, b2, b3, b4;
    b1 = ReadInt8();
    b2 = ReadInt8();
    b3 = ReadInt8();
    b4 = ReadInt8();
    return ((b4 ^ 128) - 128) * 256 * 256 * 256 + b3 * 256 * 256 + b2 * 256 +
           b1;
  }

  u32 ReadUInt32() {
    u8 b1, b2, b3, b4;
    b1 = ReadUInt8();
    b2 = ReadUInt8();
    b3 = ReadUInt8();
    b4 = ReadUInt8();
    return ((b4 ^ 128) - 128) * 256 * 256 * 256 + b3 * 256 * 256 + b2 * 256 +
           b1;
  }

  s64 ReadInt64() {
    s8 b1, b2, b3, b4, b5, b6, b7, b8;
    b1 = ReadInt8();
    b2 = ReadInt8();
    b3 = ReadInt8();
    b4 = ReadInt8();
    b5 = ReadInt8();
    b6 = ReadInt8();
    b7 = ReadInt8();
    b8 = ReadInt8();
    return ((b8 ^ 128) - 128) * 256 * 256 * 256 + b7 * 256 * 256 + b6 * 256 +
           b5 + b4 * 256 + b3 * 256 + b2 * 256 + b1;
  }

  u64 ReadUInt64() {
    u8 b1, b2, b3, b4, b5, b6, b7, b8;
    b1 = ReadUInt8();
    b2 = ReadUInt8();
    b3 = ReadUInt8();
    b4 = ReadUInt8();
    b5 = ReadUInt8();
    b6 = ReadUInt8();
    b7 = ReadUInt8();
    b8 = ReadUInt8();
    return ((b8 ^ 128) - 128) * 256 * 256 * 256 + b7 * 256 * 256 + b6 * 256 +
           b5 + b4 * 256 + b3 * 256 + b2 * 256 + b1;
  }

  std::string ReadString(int length, Encoding e) {
    u8 *bytes = ReadBytes(length);
    std::string s(reinterpret_cast<char *>(bytes));
    if (e == Encoding::UTF16)
      s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
    return s;
  }

  std::string ReadHexString(int byteCount) {
    u8 *bytes = ReadBytes(byteCount);
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    for (int i = 0; i < byteCount; i++)
      oss << std::setw(2) << bytes[i];
    return oss.str();
  }

  void WriteInt8(s8 value) {
    FSFILE_Write(fileHandle, NULL, Position, &value, 1, FS_WRITE_FLUSH);
    Position++;
  }

  void WriteUInt8(u8 value) {
    FSFILE_Write(fileHandle, NULL, Position, &value, 1, FS_WRITE_FLUSH);
    Position++;
  }

  void WriteBool(bool value) {
    if (value)
      WriteInt8(1);
    else
      WriteInt8(0);
  }

  void WriteBytes(u8 *bytes, int byteCount) {
    if (_endian == Endian::Big) {
      u8 c;
      u8 *s0 = bytes - 1;
      u8 *s1 = bytes;
      while (*s1)
        ++s1;
      while (s1-- > ++s0) {
        c = *s0;
        *s0 = *s1;
        *s1 = c;
      }
    }
    for (int i = 0; i < byteCount; i++) {
      FSFILE_Write(fileHandle, NULL, Position, &bytes[i], 1, FS_WRITE_FLUSH);
      Position++;
    }
  }

  void WriteInt16(s16 value) {
    if (_endian == Endian::Little) {
      WriteInt8((s16)(value & 0xFF));
      WriteInt8((s16)(value >> 8 & 0xFF));
    } else {
      WriteInt8((s16)(value >> 8 & 0xFF));
      WriteInt8((s16)(value & 0xFF));
    }
  }

  void WriteUInt16(u16 value) {
    if (_endian == Endian::Little) {
      WriteUInt8((u16)(value & 0xFF));
      WriteUInt8((u16)(value >> 8 & 0xFF));
    } else {
      WriteUInt8((u16)(value >> 8 & 0xFF));
      WriteUInt8((u16)(value & 0xFF));
    }
  }

  void WriteInt32(s32 value) {
    if (_endian == Endian::Little) {
      WriteInt8((s32)(value & 0xFF));
      WriteInt8((s32)(value >> 8 & 0xFF));
      WriteInt8((s32)(value >> 16 & 0xFF));
      WriteInt8((s32)(value >> 24 & 0xFF));
    } else {
      WriteInt8((s32)(value >> 24 & 0xFF));
      WriteInt8((s32)(value >> 16 & 0xFF));
      WriteInt8((s32)(value >> 8 & 0xFF));
      WriteInt8((s32)(value & 0xFF));
    }
  }

  void WriteUInt32(u32 value) {
    if (_endian == Endian::Little) {
      WriteUInt8((u32)(value & 0xFF));
      WriteUInt8((u32)(value >> 8 & 0xFF));
      WriteUInt8((u32)(value >> 16 & 0xFF));
      WriteUInt8((u32)(value >> 24 & 0xFF));
    } else {
      WriteUInt8((u32)(value >> 24 & 0xFF));
      WriteUInt8((u32)(value >> 16 & 0xFF));
      WriteUInt8((u32)(value >> 8 & 0xFF));
      WriteUInt8((u32)(value & 0xFF));
    }
  }

  void WriteInt64(s64 value) {
    if (_endian == Endian::Little) {
      WriteInt8((s64)(value & 0xFF));
      WriteInt8((s64)(value >> 8 & 0xFF));
      WriteInt8((s64)(value >> 16 & 0xFF));
      WriteInt8((s64)(value >> 24 & 0xFF));
      WriteInt8((s64)(value >> 32 & 0xFF));
      WriteInt8((s64)(value >> 40 & 0xFF));
      WriteInt8((s64)(value >> 48 & 0xFF));
      WriteInt8((s64)(value >> 56 & 0xFF));
    } else {
      WriteInt8((s64)(value >> 56 & 0xFF));
      WriteInt8((s64)(value >> 48 & 0xFF));
      WriteInt8((s64)(value >> 40 & 0xFF));
      WriteInt8((s64)(value >> 32 & 0xFF));
      WriteInt8((s64)(value >> 24 & 0xFF));
      WriteInt8((s64)(value >> 16 & 0xFF));
      WriteInt8((s64)(value >> 8 & 0xFF));
      WriteInt8((s64)(value & 0xFF));
    }
  }

  void WriteUInt64(u64 value) {
    if (_endian == Endian::Little) {
      WriteUInt8((u64)(value & 0xFF));
      WriteUInt8((u64)(value >> 8 & 0xFF));
      WriteUInt8((u64)(value >> 16 & 0xFF));
      WriteUInt8((u64)(value >> 24 & 0xFF));
      WriteUInt8((u64)(value >> 32 & 0xFF));
      WriteUInt8((u64)(value >> 40 & 0xFF));
      WriteUInt8((u64)(value >> 48 & 0xFF));
      WriteUInt8((u64)(value >> 56 & 0xFF));
    } else {
      WriteUInt8((u64)(value >> 56 & 0xFF));
      WriteUInt8((u64)(value >> 48 & 0xFF));
      WriteUInt8((u64)(value >> 40 & 0xFF));
      WriteUInt8((u64)(value >> 32 & 0xFF));
      WriteUInt8((u64)(value >> 24 & 0xFF));
      WriteUInt8((u64)(value >> 16 & 0xFF));
      WriteUInt8((u64)(value >> 8 & 0xFF));
      WriteUInt8((u64)(value & 0xFF));
    }
  }

  void WriteString(std::string s, Encoding e) {
    switch (e) {
    case Encoding::UTF8:
      WriteBytes((u8 *)s.data(), s.length());
      break;
    case Encoding::UTF16: {
      for (char ch : s) {
        WriteUInt8((u8)ch);
        WriteUInt8(0x00);
      }
      break;
    }
    }
  }

  void WriteHexString(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    s.erase(std::remove(s.begin(), s.end(), '-'), s.end());
    if (_endian == Endian::Big) {
      std::reverse(s.begin(), s.end());
      for (auto it = s.begin(); it != s.end(); it += 2)
        std::swap(it[0], it[1]);
    }
    const char *c = s.data();
    size_t len = std::strlen(c);
    u8 *array = new u8[((len / 2) + 1) * sizeof(array)];
    for (size_t a = 0, b = 0; b < len / 2; a += 2, b++)
      array[b] = (c[a] % 32 + 9) % 25 * 16 + (c[a + 1] % 32 + 9) % 25;
    WriteBytes(array, s.length() / 2);
  }

private:
  bool open;
  Endian _endian;
  FS_Archive arch;
  Handle fileHandle;
  u64 fileSize = 0;
};

struct sfse_menu_option {
  std::string name;
  void (*func)();
};

class sfse_menu {
public:
  std::vector<void (*)()> functions;
  sfse_menu(std::string select_message) {
    option_count = 0;
    message = select_message;
  }

  void AddOption(sfse_menu_option option) {
    options.push_back(option);
    functions.push_back(option.func);
    option_count++;
  }

  int OptionCount() { return option_count; }

  int GetOption() {
    int selected = 0;
    consoleClear();
    printf("%s\n", message.c_str());
    printf("%s\n", options[selected].name.c_str());
    while (aptMainLoop()) {
      gspWaitForVBlank();
      hidScanInput();
      if (hidKeysDown() & KEY_A)
        break;
      if (hidKeysDown() & KEY_DUP) {
        if (selected < option_count - 1) {
          selected++;
          consoleClear();
          printf("%s\n", message.c_str());
          printf("%s\n", options[selected].name.c_str());
          gfxFlushBuffers();
          gfxSwapBuffers();
        }
      }
      if (hidKeysDown() & KEY_DDOWN) {
        if (selected > 0) {
          selected--;
          consoleClear();
          printf("%s\n", message.c_str());
          printf("%s\n", options[selected].name.c_str());
        }
      }
      gfxFlushBuffers();
      gfxSwapBuffers();
    }
    return selected;
  }

private:
  int option_count;
  std::string message;
  std::vector<sfse_menu_option> options;
};
