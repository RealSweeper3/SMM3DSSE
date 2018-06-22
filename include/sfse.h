#pragma once

#pragma GCC diagnostic ignored "-Wnarrowing"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <3ds.h>

enum class Endian { Big, Little };
enum class SEMode { SaveData, SD, FileHandle };

namespace SFSE {
class SaveEditor {
public:
    explicit SaveEditor() : mode(SEMode::SD) {}

    explicit SaveEditor(const std::string& path, Endian endian) : mode(SEMode::SD), endian(endian) {
        FSUSER_OpenArchive(&arch, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
        Result res = FSUSER_OpenFile(&file_handle, arch, fsMakePath(PATH_ASCII, path.data()),
                                     FS_OPEN_READ | FS_OPEN_WRITE, 0);
        if (R_SUCCEEDED(res)) {
            open = true;
            FSFILE_GetSize(file_handle, &file_size);
        } else {
            open = false;
        }
    }

    explicit SaveEditor(u64 title_id, const std::string& path, Endian endian)
        : mode(SEMode::SaveData), endian(endian) {
        u32 cardPath[3] = {MEDIATYPE_GAME_CARD, title_id, title_id >> 32};
        if (R_FAILED(
                FSUSER_OpenArchive(&arch, ARCHIVE_USER_SAVEDATA, {PATH_BINARY, 0xC, cardPath}))) {
            u32 sdPath[3] = {MEDIATYPE_SD, title_id, title_id >> 32};
            FSUSER_OpenArchive(&arch, ARCHIVE_USER_SAVEDATA, {PATH_BINARY, 0xC, sdPath});
        }
        Result res = FSUSER_OpenFile(&file_handle, arch, fsMakePath(PATH_ASCII, path.data()),
                                     FS_OPEN_READ | FS_OPEN_WRITE, 0);
        if (R_SUCCEEDED(res)) {
            open = true;
            FSFILE_GetSize(file_handle, &file_size);
        } else {
            open = false;
        }
    }

    explicit SaveEditor(Handle handle, Endian endian)
        : open(true), mode(SEMode::FileHandle), endian(endian), file_handle(handle) {
        FSFILE_GetSize(file_handle, &file_size);
    }

    void CloseFile() {
        if (open) {
            FSFILE_Close(file_handle);
            if (mode == SEMode::SaveData)
                FSUSER_ControlArchive(arch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
            if (mode != SEMode::FileHandle)
                FSUSER_CloseArchive(arch);
            open = false;
        }
    }

    bool FileOpen() {
        return open;
    }

    u64 GetFileSize() {
        return file_size;
    }

    u64 GetOffset() {
        return offset;
    }

    void SetOffset(u64 offset) {
        this->offset = offset;
    }

    char ReadChar() {
        char c;
        FSFILE_Read(file_handle, NULL, offset, &c, 1);
        ++offset;
        return c;
    }

    char16_t ReadChar16() {
        char16_t c;
        FSFILE_Read(file_handle, NULL, offset, &c, 2);
        ++offset;
        return c;
    }

    s8 ReadInt8() {
        s8 b;
        FSFILE_Read(file_handle, NULL, offset, &b, 1);
        ++offset;
        return b;
    }

    u8 ReadUInt8() {
        u8 b;
        FSFILE_Read(file_handle, NULL, offset, &b, 1);
        ++offset;
        return b;
    }

    u8* ReadBytes(size_t count) {
        u8* bytes = new u8[count];
        FSFILE_Read(file_handle, NULL, offset, bytes, count);
        if (endian == Endian::Big) {
            u8 c;
            u8* s0 = bytes - 1;
            u8* s1 = bytes;
            while (*s1)
                ++s1;
            while (s1-- > ++s0) {
                c = *s0;
                *s0 = *s1;
                *s1 = c;
            }
        }
        offset += count;
        return bytes;
    }

    bool ReadBool() {
        bool value;
        std::memcpy(&value, ReadBytes(sizeof(bool)), sizeof(bool));
        return value;
    }

    s16 ReadInt16() {
        s16 value;
        std::memcpy(&value, ReadBytes(sizeof(s16)), sizeof(s16));
        return value;
    }

    u16 ReadUInt16() {
        u16 value;
        std::memcpy(&value, ReadBytes(sizeof(u16)), sizeof(u16));
        return value;
    }

    s32 ReadInt32() {
        s32 value;
        std::memcpy(&value, ReadBytes(sizeof(s32)), sizeof(s32));
        return value;
    }

    u32 ReadUInt32() {
        u32 value;
        std::memcpy(&value, ReadBytes(sizeof(u32)), sizeof(u32));
        return value;
    }

    s64 ReadInt64() {
        s64 value;
        std::memcpy(&value, ReadBytes(sizeof(s64)), sizeof(s64));
        return value;
    }

    u64 ReadUInt64() {
        u64 value;
        std::memcpy(&value, ReadBytes(sizeof(u64)), sizeof(u64));
        return value;
    }

    std::string ReadString(size_t length) {
        return std::string(reinterpret_cast<const char*>(const_cast<const u8*>(ReadBytes(length))));
    }

    std::u16string ReadU16String(size_t length) {
        return std::u16string(
            reinterpret_cast<const char16_t*>(const_cast<const u8*>(ReadBytes(length * 2))));
    }

    std::string ReadHexString(size_t length) {
        std::string str = ReadString(length);
        std::ostringstream ret;

        for (size_t i = 0; i < str.length(); ++i)
            ret << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << str[i];

        return ret.str();
    }

    void WriteInt8(s8 value) {
        FSFILE_Write(file_handle, NULL, offset, &value, 1, FS_WRITE_FLUSH);
        ++offset;
    }

    void WriteUInt8(u8 value) {
        FSFILE_Write(file_handle, NULL, offset, &value, 1, FS_WRITE_FLUSH);
        ++offset;
    }

    void WriteBool(bool value) {
        WriteInt8(static_cast<s8>(value));
    }

    void WriteBytes(u8* bytes, size_t count) {
        if (endian == Endian::Big) {
            u8 c;
            u8* s0 = bytes - 1;
            u8* s1 = bytes;
            while (*s1)
                ++s1;
            while (s1-- > ++s0) {
                c = *s0;
                *s0 = *s1;
                *s1 = c;
            }
        }
        FSFILE_Write(file_handle, NULL, offset, bytes, count, FS_WRITE_FLUSH);
        offset += count;
    }

    void WriteInt16(s16 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(s16));
    }

    void WriteUInt16(u16 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(u16));
    }

    void WriteInt32(s32 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(s32));
    }

    void WriteUInt32(u32 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(u32));
    }

    void WriteInt64(s64 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(s64));
    }

    void WriteUInt64(u64 value) {
        WriteBytes(reinterpret_cast<u8*>(&value), sizeof(u64));
    }

    void WriteString(const std::string& str) {
        WriteBytes(reinterpret_cast<u8*>(const_cast<char*>(str.data())), str.size());
    }

    void WriteU16String(const std::u16string& str) {
        WriteBytes(reinterpret_cast<u8*>(const_cast<char16_t*>(str.data())), str.size());
    }

    void WriteHexString(std::string str) {
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
        str.erase(std::remove(str.begin(), str.end(), '-'), str.end());
        if (endian == Endian::Big) {
            std::reverse(str.begin(), str.end());
            for (auto it = str.begin(); it != str.end(); it += 2)
                std::swap(it[0], it[1]);
        }
        const char* c = str.c_str();
        size_t len = str.length();
        u8* array = new u8[((len / 2) + 1) * sizeof(array)];
        for (size_t a = 0, b = 0; b < len / 2; a += 2, b++)
            array[b] = (c[a] % 32 + 9) % 25 * 16 + (c[a + 1] % 32 + 9) % 25;
        WriteBytes(array, str.length());
    }

private:
    bool open;
    SEMode mode;
    Endian endian;
    FS_Archive arch;
    Handle file_handle;
    u64 file_size = 0, offset = 0;
};

class Menu {
public:
    explicit Menu(std::string message) : message(message) {}

    void AddOption(const std::string& name) {
        options.push_back(name);
    }

    size_t OptionCount() {
        return options.size();
    }

    size_t GetOption() {
        size_t selected = 0;
        consoleClear();
        if (!message.empty())
            printf("%s\n", message.c_str());
        auto DrawMenu = [&] {
            consoleClear();
            if (!message.empty())
                printf("%s\n", message.c_str());
            for (size_t i = 0; i < options.size(); ++i) {
                if (selected == i)
                    printf("-> %s\n", options[i].c_str());
                else
                    printf("   %s\n", options[i].c_str());
            }
        };
        DrawMenu();
        while (true) {
            hidScanInput();
            if (hidKeysDown() & KEY_A)
                break;
            if (hidKeysDown() & KEY_UP) {
                if (selected > 0) {
                    --selected;
                    DrawMenu();
                }
            }
            if (hidKeysDown() & KEY_DOWN) {
                if (selected < (options.size() - 1)) {
                    ++selected;
                    DrawMenu();
                }
            }
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        return selected;
    }

private:
    std::string message;
    std::vector<std::string> options;
};
} // namespace SFSE
