#pragma once

#pragma GCC diagnostic ignored "-Wnarrowing"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <3ds.h>
#include <citro3d.h>

namespace ctr {
namespace fs {

enum class Endian { Big, Little };
enum class Mode { SaveData, SD, FileHandle };

class File {
public:
    explicit File() : mode(Mode::SD) {}

    explicit File(const std::string& path, Endian endian) : mode(Mode::SD), endian(endian) {
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

    explicit File(u64 title_id, const std::string& path, Endian endian)
        : mode(Mode::SaveData), endian(endian) {
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

    explicit File(Handle handle, Endian endian)
        : open(true), mode(Mode::FileHandle), endian(endian), file_handle(handle) {
        FSFILE_GetSize(file_handle, &file_size);
    }

    void Close() {
        if (open) {
            FSFILE_Close(file_handle);
            if (mode == Mode::SaveData)
                FSUSER_ControlArchive(arch, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
            if (mode != Mode::FileHandle)
                FSUSER_CloseArchive(arch);
            open = false;
        }
    }

    bool IsOpen() {
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
    Mode mode;
    Endian endian;
    FS_Archive arch;
    Handle file_handle;
    u64 file_size = 0, offset = 0;
};
} // namespace fs

namespace ui {

unsigned char vshader_shbin[820] = {
    0x3B, 0x20, 0x45, 0x78, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x20, 0x50, 0x49,
    0x43, 0x41, 0x32, 0x30, 0x30, 0x20, 0x76, 0x65, 0x72, 0x74, 0x65, 0x78,
    0x20, 0x73, 0x68, 0x61, 0x64, 0x65, 0x72, 0x0D, 0x0A, 0x0D, 0x0A, 0x3B,
    0x20, 0x55, 0x6E, 0x69, 0x66, 0x6F, 0x72, 0x6D, 0x73, 0x0D, 0x0A, 0x2E,
    0x66, 0x76, 0x65, 0x63, 0x20, 0x70, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74,
    0x69, 0x6F, 0x6E, 0x5B, 0x34, 0x5D, 0x0D, 0x0A, 0x0D, 0x0A, 0x3B, 0x20,
    0x43, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74, 0x73, 0x0D, 0x0A, 0x2E,
    0x63, 0x6F, 0x6E, 0x73, 0x74, 0x66, 0x20, 0x6D, 0x79, 0x63, 0x6F, 0x6E,
    0x73, 0x74, 0x28, 0x30, 0x2E, 0x30, 0x2C, 0x20, 0x31, 0x2E, 0x30, 0x2C,
    0x20, 0x2D, 0x31, 0x2E, 0x30, 0x2C, 0x20, 0x30, 0x2E, 0x31, 0x29, 0x0D,
    0x0A, 0x2E, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x66, 0x20, 0x6D, 0x79, 0x63,
    0x6F, 0x6E, 0x73, 0x74, 0x32, 0x28, 0x30, 0x2E, 0x33, 0x2C, 0x20, 0x30,
    0x2E, 0x30, 0x2C, 0x20, 0x30, 0x2E, 0x30, 0x2C, 0x20, 0x30, 0x2E, 0x30,
    0x29, 0x0D, 0x0A, 0x2E, 0x61, 0x6C, 0x69, 0x61, 0x73, 0x20, 0x20, 0x7A,
    0x65, 0x72, 0x6F, 0x73, 0x20, 0x6D, 0x79, 0x63, 0x6F, 0x6E, 0x73, 0x74,
    0x2E, 0x78, 0x78, 0x78, 0x78, 0x20, 0x3B, 0x20, 0x56, 0x65, 0x63, 0x74,
    0x6F, 0x72, 0x20, 0x66, 0x75, 0x6C, 0x6C, 0x20, 0x6F, 0x66, 0x20, 0x7A,
    0x65, 0x72, 0x6F, 0x73, 0x0D, 0x0A, 0x2E, 0x61, 0x6C, 0x69, 0x61, 0x73,
    0x20, 0x20, 0x6F, 0x6E, 0x65, 0x73, 0x20, 0x20, 0x6D, 0x79, 0x63, 0x6F,
    0x6E, 0x73, 0x74, 0x2E, 0x79, 0x79, 0x79, 0x79, 0x20, 0x3B, 0x20, 0x56,
    0x65, 0x63, 0x74, 0x6F, 0x72, 0x20, 0x66, 0x75, 0x6C, 0x6C, 0x20, 0x6F,
    0x66, 0x20, 0x6F, 0x6E, 0x65, 0x73, 0x0D, 0x0A, 0x0D, 0x0A, 0x3B, 0x20,
    0x4F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x73, 0x0D, 0x0A, 0x2E, 0x6F, 0x75,
    0x74, 0x20, 0x6F, 0x75, 0x74, 0x70, 0x6F, 0x73, 0x20, 0x70, 0x6F, 0x73,
    0x69, 0x74, 0x69, 0x6F, 0x6E, 0x0D, 0x0A, 0x2E, 0x6F, 0x75, 0x74, 0x20,
    0x6F, 0x75, 0x74, 0x63, 0x6C, 0x72, 0x20, 0x63, 0x6F, 0x6C, 0x6F, 0x72,
    0x0D, 0x0A, 0x2E, 0x6F, 0x75, 0x74, 0x20, 0x6F, 0x75, 0x74, 0x74, 0x63,
    0x30, 0x20, 0x74, 0x65, 0x78, 0x63, 0x6F, 0x6F, 0x72, 0x64, 0x30, 0x0D,
    0x0A, 0x0D, 0x0A, 0x3B, 0x20, 0x49, 0x6E, 0x70, 0x75, 0x74, 0x73, 0x20,
    0x28, 0x64, 0x65, 0x66, 0x69, 0x6E, 0x65, 0x64, 0x20, 0x61, 0x73, 0x20,
    0x61, 0x6C, 0x69, 0x61, 0x73, 0x65, 0x73, 0x20, 0x66, 0x6F, 0x72, 0x20,
    0x63, 0x6F, 0x6E, 0x76, 0x65, 0x6E, 0x69, 0x65, 0x6E, 0x63, 0x65, 0x29,
    0x0D, 0x0A, 0x2E, 0x61, 0x6C, 0x69, 0x61, 0x73, 0x20, 0x69, 0x6E, 0x70,
    0x6F, 0x73, 0x20, 0x76, 0x30, 0x0D, 0x0A, 0x2E, 0x61, 0x6C, 0x69, 0x61,
    0x73, 0x20, 0x69, 0x6E, 0x74, 0x65, 0x78, 0x20, 0x76, 0x31, 0x0D, 0x0A,
    0x0D, 0x0A, 0x2E, 0x62, 0x6F, 0x6F, 0x6C, 0x20, 0x74, 0x65, 0x73, 0x74,
    0x0D, 0x0A, 0x0D, 0x0A, 0x2E, 0x70, 0x72, 0x6F, 0x63, 0x20, 0x6D, 0x61,
    0x69, 0x6E, 0x0D, 0x0A, 0x09, 0x3B, 0x20, 0x46, 0x6F, 0x72, 0x63, 0x65,
    0x20, 0x74, 0x68, 0x65, 0x20, 0x77, 0x20, 0x63, 0x6F, 0x6D, 0x70, 0x6F,
    0x6E, 0x65, 0x6E, 0x74, 0x20, 0x6F, 0x66, 0x20, 0x69, 0x6E, 0x70, 0x6F,
    0x73, 0x20, 0x74, 0x6F, 0x20, 0x62, 0x65, 0x20, 0x31, 0x2E, 0x30, 0x0D,
    0x0A, 0x09, 0x6D, 0x6F, 0x76, 0x20, 0x72, 0x30, 0x2E, 0x78, 0x79, 0x7A,
    0x2C, 0x20, 0x69, 0x6E, 0x70, 0x6F, 0x73, 0x0D, 0x0A, 0x09, 0x6D, 0x6F,
    0x76, 0x20, 0x72, 0x30, 0x2E, 0x77, 0x2C, 0x20, 0x20, 0x20, 0x6F, 0x6E,
    0x65, 0x73, 0x0D, 0x0A, 0x0D, 0x0A, 0x09, 0x3B, 0x20, 0x6F, 0x75, 0x74,
    0x70, 0x6F, 0x73, 0x20, 0x3D, 0x20, 0x70, 0x72, 0x6F, 0x6A, 0x65, 0x63,
    0x74, 0x69, 0x6F, 0x6E, 0x4D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x20, 0x2A,
    0x20, 0x69, 0x6E, 0x70, 0x6F, 0x73, 0x0D, 0x0A, 0x09, 0x64, 0x70, 0x34,
    0x20, 0x6F, 0x75, 0x74, 0x70, 0x6F, 0x73, 0x2E, 0x78, 0x2C, 0x20, 0x70,
    0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x5B, 0x30, 0x5D,
    0x2C, 0x20, 0x72, 0x30, 0x0D, 0x0A, 0x09, 0x64, 0x70, 0x34, 0x20, 0x6F,
    0x75, 0x74, 0x70, 0x6F, 0x73, 0x2E, 0x79, 0x2C, 0x20, 0x70, 0x72, 0x6F,
    0x6A, 0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x5B, 0x31, 0x5D, 0x2C, 0x20,
    0x72, 0x30, 0x0D, 0x0A, 0x09, 0x64, 0x70, 0x34, 0x20, 0x6F, 0x75, 0x74,
    0x70, 0x6F, 0x73, 0x2E, 0x7A, 0x2C, 0x20, 0x70, 0x72, 0x6F, 0x6A, 0x65,
    0x63, 0x74, 0x69, 0x6F, 0x6E, 0x5B, 0x32, 0x5D, 0x2C, 0x20, 0x72, 0x30,
    0x0D, 0x0A, 0x09, 0x64, 0x70, 0x34, 0x20, 0x6F, 0x75, 0x74, 0x70, 0x6F,
    0x73, 0x2E, 0x77, 0x2C, 0x20, 0x70, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74,
    0x69, 0x6F, 0x6E, 0x5B, 0x33, 0x5D, 0x2C, 0x20, 0x72, 0x30, 0x0D, 0x0A,
    0x0D, 0x0A, 0x09, 0x3B, 0x20, 0x6F, 0x75, 0x74, 0x63, 0x6C, 0x72, 0x20,
    0x3D, 0x20, 0x77, 0x68, 0x69, 0x74, 0x65, 0x0D, 0x0A, 0x09, 0x6D, 0x6F,
    0x76, 0x20, 0x6F, 0x75, 0x74, 0x63, 0x6C, 0x72, 0x2C, 0x20, 0x6F, 0x6E,
    0x65, 0x73, 0x0D, 0x0A, 0x0D, 0x0A, 0x09, 0x3B, 0x20, 0x6F, 0x75, 0x74,
    0x74, 0x63, 0x30, 0x20, 0x3D, 0x20, 0x69, 0x6E, 0x74, 0x65, 0x78, 0x0D,
    0x0A, 0x09, 0x6D, 0x6F, 0x76, 0x20, 0x6F, 0x75, 0x74, 0x74, 0x63, 0x30,
    0x2C, 0x20, 0x69, 0x6E, 0x74, 0x65, 0x78, 0x0D, 0x0A, 0x0D, 0x0A, 0x09,
    0x3B, 0x20, 0x57, 0x65, 0x27, 0x72, 0x65, 0x20, 0x66, 0x69, 0x6E, 0x69,
    0x73, 0x68, 0x65, 0x64, 0x0D, 0x0A, 0x09, 0x65, 0x6E, 0x64, 0x0D, 0x0A,
    0x2E, 0x65, 0x6E, 0x64
};

int vshader_shbin_size = 820;

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float position[3]; float texcoord[2]; } textVertex_s;

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection;
static C3D_Mtx projectiontop, projectionbot;
static C3D_Tex* glyphSheets;
static textVertex_s* textVtxArray;
static int textVtxArrayPos;
C3D_RenderTarget* top, *bot;

#define TEXT_VTX_ARRAY_COUNT (4 * 1024)

void startframe() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    textVtxArrayPos = 0; // Clear the text vertex array
}

void endframe() {
    C3D_FrameEnd(0);
}

void sceneInit(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    fontEnsureMapped();
    top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetClear(top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_RenderTargetSetOutput(top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    bot = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    C3D_RenderTargetSetClear(bot, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
    C3D_RenderTargetSetOutput(bot, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
    // Load the vertex shader, create a shader program and bind it
    vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);
    // Get the location of the uniforms
    uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
    // Configure attributes for use with the vertex shader
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
    AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
    // Compute the projection matrices
    Mtx_OrthoTilt(&projectiontop, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0, true);
    Mtx_OrthoTilt(&projectionbot, 0.0, 320.0, 240.0, 0.0, 0.0, 1.0, true);
    // Configure depth test to overwrite pixels with the same depth (needed to draw overlapping glyphs)
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    // Load the glyph texture sheets
    int i;
    TGLP_s* glyphInfo = fontGetGlyphInfo();
    glyphSheets = (C3D_Tex*)malloc(sizeof(C3D_Tex)*glyphInfo->nSheets);
    for (i = 0; i < glyphInfo->nSheets; i ++) {
        C3D_Tex* tex = &glyphSheets[i];
        tex->data = fontGetGlyphSheetTex(i);
        tex->fmt = (GPU_TEXCOLOR)glyphInfo->sheetFmt        tex->size = glyphInfo->sheetSize;
        tex->width = glyphInfo->sheetWidth;
        tex->height = glyphInfo->sheetHeight;
        tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
            | GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
        tex->border = 0;
        tex->lodParam = 0;
    }
    // Create the text vertex array
    textVtxArray = (textVertex_s*)linearAlloc(sizeof(textVertex_s)*TEXT_VTX_ARRAY_COUNT);
}

void drawon(gfxScreen_t screen) {
    C3D_FrameDrawOn(screen == GFX_TOP ? top : bot);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, screen == GFX_TOP ? &projectiontop : &projectionbot);
}

void setTextColor(u32 color) {
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
    C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
    C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
    C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
    C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
    C3D_TexEnvColor(env, color);
}

static void addTextVertex(float vx, float vy, float tx, float ty) {
    textVertex_s* vtx = &textVtxArray[textVtxArrayPos++];
    vtx->position[0] = vx;
    vtx->position[1] = vy;
    vtx->position[2] = 0.5f;
    vtx->texcoord[0] = tx;
    vtx->texcoord[1] = ty;
}

void renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text) {
    ssize_t  units;
    uint32_t code;
    // Configure buffers
    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, textVtxArray, sizeof(textVertex_s), 2, 0x10);
    const uint8_t* p = (const uint8_t*)text;
    float firstX = x;
    u32 flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
    int lastSheet = -1;
    do {
        if (!*p)
            break;
        units = decode_utf8(&code, p);
        if (units == -1)
            break;
        p += units;
        if (code == '\n') {
            x = firstX;
            y += scaleY * fontGetInfo()->lineFeed;
        }
        else if (code > 0) {
            int glyphIdx = fontGlyphIndexFromCodePoint(code);
            fontGlyphPos_s data;
            fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);
            // Bind the correct texture sheet
            if (data.sheetIndex != lastSheet) {
                lastSheet = data.sheetIndex;
                C3D_TexBind(0, &glyphSheets[lastSheet]);
            }
            int arrayIndex = textVtxArrayPos;
            if ((arrayIndex+4) >= TEXT_VTX_ARRAY_COUNT)
                break; // We can't render more characters
            // Add the vertices to the array
            addTextVertex(x + data.vtxcoord.left, y+data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
            addTextVertex(x + data.vtxcoord.right, y+data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
            addTextVertex(x + data.vtxcoord.left, y+data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
            addTextVertex(x + data.vtxcoord.right, y+data.vtxcoord.top, data.texcoord.right, data.texcoord.top);
            // Draw the glyph
            C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);
            x += data.xAdvance;
        }
    } while (code > 0);
}

void sceneExit() {
    // Free the textures
    free(glyphSheets);
    // Free the shader program
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
}

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
        //consoleClear();
        sceneInit();
        setTextColor(0xFF000000);
        auto DrawMenu = [&] {
            /*consoleClear();
            if (!message.empty())
                printf("%s\n", message.c_str());
            for (size_t i = 0; i < options.size(); ++i) {
                if (selected == i)
                    printf("-> %s\n", options[i].c_str());
                else
                    printf("   %s\n", options[i].c_str());
            }*/
            startframe();
            drawon(GFX_TOP);
            float y = 0;
            if (!message.empty()) {
                renderText(0, 0, 1, 1, false, message.c_str());
                y = 25;
            }
            for (size_t i = 0; i < options.size(); ++i) {
                if (selected == i)
                    renderText(0, y, false, std::string("-> " + options[i]).c_str());
                else
                    renderText(0, y, false, std::string("   " + options[i]).c_str());
                y += 25;
            }
            endframe();
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
        sceneExit();
        return selected;
    }

private:
    std::string message;
    std::vector<std::string> options;
};
} // namespace ui

namespace title {
Result Install(const std::string& path, FS_MediaType media_type) {
    // Initialize AM service
    amInit();

    u8* buffer = NULL;
    FILE* file = fopen(path.c_str(), "rb");

    if (file == NULL) {
        return -1;
    }

    fseek(file, 0, SEEK_END);
    off_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = (u8*)malloc(size);

    if (!buffer) {
        return -1;
    }

    off_t bytesRead = fread(buffer, 1, size, file);
    fclose(file);

    if (size != bytesRead) {
        return -1;
    }

    Handle cia_handle;
    Result res;
    res = AM_StartCiaInstall(media_type, &cia_handle);

    if (R_FAILED(res)) {
        return res;
    }

    res = FSFILE_Write(cia_handle, NULL, 0, buffer, size, 0);

    if (R_FAILED(res)) {
        svcCloseHandle(cia_handle);
        return res;
    }

    res = AM_FinishCiaInstall(cia_handle);

    if (R_FAILED(res)) {
        svcCloseHandle(cia_handle);
        return res;
    }

    // Exit AM service
    amExit();
    return 0;
}
} // namespace title

namespace network {
namespace http {

using Headers = std::unordered_map<std::string, std::string>;

struct Request {
    std::string method;
    std::string path;
    std::string raw;
    std::string ip;
    u16 port;
};

struct Response {
    Result result;
    u32 status_code = 200;
    u32 size = 0;
    std::vector<u8> data;
    std::string text;
    Headers headers;

    Response& status(u32 status) {
        status_code = status;
        return *this;
    }

    void end(const std::string& body, const Headers& headers) {
        text = body;
        this->headers = headers;
    }

    std::string raw() {
        std::string str = "HTTP/1.1 ";
        str += std::to_string(status_code);
        str += " ";
        switch (status_code) {
            case 100:
                str += "Continue";
                break;
            case 101:
                str += "Switching Protocols";
                break;
            case 200:
                str += "OK";
                break;
            case 201:
                str += "Created";
                break;
            case 202:
                str += "Accepted";
                break;
            case 203:
                str += "Non-Authoritative Information";
                break;
            case 204:
                str += "No Content";
                break;
            case 205:
                str += "Reset Content";
                break;
            case 206:
                str += "Partial Content";
                break;
            case 300:
                str += "Multiple Choices";
                break;
            case 301:
                str += "Moved Permanently";
                break;
            case 302:
                str += "Found";
                break;
            case 303:
                str += "See Other";
                break;
            case 304:
                str += "Not Modified";
                break;
            case 305:
                str += "Use Proxy";
                break;
            case 307:
                str += "Temporary Redirect";
                break;
            case 400:
                str += "Bad Request";
                break;
            case 401:
                str += "Unauthorized";
                break;
            case 402:
                str += "Payment Required";
                break;
            case 403:
                str += "Forbidden";
                break;
            case 404:
                str += "Not Found";
                break;
            case 405:
                str += "Method Not Allowed";
                break;
            case 406:
                str += "Not Acceptable";
                break;
            case 407:
                str += "Proxy Authentication Required";
                break;
            case 408:
                str += "Request Timeout";
                break;
            case 409:
                str += "Conflict";
                break;
            case 410:
                str += "Gone";
                break;
            case 411:
                str += "Length Required";
                break;
            case 412:
                str += "Precondition Failed";
                break;
            case 413:
                str += "Payload Too Large";
                break;
            case 414:
                str += "URI Too Long";
                break;
            case 415:
                str += "Unsupported Media Type";
                break;
            case 416:
                str += "Requested Range Not Satisfiable";
                break;
            case 417:
                str += "Expectation Failed";
                break;
            case 426:
                str += "Upgrade Required";
                break;
            case 500:
                str += "Internal Server Error";
                break;
            case 501:
                str += "Not Implemented";
                break;
            case 502:
                str += "Bad Gateway";
                break;
            case 503:
                str += "Service Unavailable";
                break;
            case 504:
                str += "Gateway Timeout";
                break;
            case 505:
                str += "HTTP Version Not Supported";
                break;
        }
        str += "\r\n";
        for (auto& header : headers) {
            str += header.first;
            str += ": ";
            str += header.second;
            str += "\r\n";
        }
        str += "\r\n";
        str += text;
        str += "\r\n";
        return str;
    }
};

struct Server {
    Server(u16 port, std::function<void (Request, Response&)> handler) {
        gfxInitDefault();
        consoleInit(GFX_TOP, NULL);
        u32* buffer = (u32*)memalign(0x1000, 0x100000);
        socInit(buffer, 0x100000);
        s32 sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        struct sockaddr_in client;
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        memset(&client, 0, sizeof(client));
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = gethostid();
        std::cout << "IP: " << std::string(inet_ntoa(server.sin_addr)) << std::endl;
        std::cout << "Port: " << port << std::endl;
        bind(sock, (struct sockaddr*)&server, sizeof(server));
        fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
        listen(sock, 5);
        s32 csock = -1;
        u32 clientlen;
        char* data = new char[1026];
        while (aptMainLoop()) {
            gspWaitForVBlank();
            hidScanInput();
            csock = accept(sock, (struct sockaddr*)&client, &clientlen);
            if (csock > 0) {
                fcntl(csock, F_SETFL, fcntl(csock, F_GETFL, 0) & ~O_NONBLOCK);
                memset(data, 0, 1026);
                recv(csock, data, 1024, 0);
                auto Split = [&](const std::string& text, char sep) -> std::vector<std::string> {
                    std::vector<std::string> tokens;
                    std::size_t start = 0, end = 0;
                    while ((end = text.find(sep, start)) != std::string::npos) {
                        tokens.push_back(text.substr(start, end - start));
                        start = end + 1;
                    }
                    tokens.push_back(text.substr(start));
                    return tokens;
                };
                std::string first_line = Split(std::string(data), '\n')[0];
                std::vector<std::string> v = Split(first_line, ' ');
                std::string method = v[0];
                std::string path = v[1];
                Response response;
                handler(Request{method, path, std::string(data), std::string(inet_ntoa(client.sin_addr)), client.sin_port}, response);
                send(csock, response.raw().c_str(), response.raw().length(), 0);
                close(csock);
                csock = -1;
            }
            if (hidKeysDown() & KEY_START)
                break;
        }
        close(sock);
        socExit();
        gfxExit();
    }
};

#define RETURN_ERROR                                                                               \
    Response res;                                                                                  \
    res.result = ret;                                                                              \
    res.status_code = statuscode;                                                                  \
    res.size = 0;                                                                                  \
    res.data = std::vector<u8>();                                                                  \
    res.text = std::string();                                                                      \
    res.headers = Headers();                                                                       \
    return res;

void Init() {
    httpcInit(4 * 1024 * 1024);
}

void Exit() {
    httpcExit();
}

Response Send(HTTPC_RequestMethod method, const std::string& url, const std::string& body,
              bool response_has_body = true, bool keep_alive = false, Headers headers = Headers(),
              std::vector<std::string> response_headers = std::vector<std::string>()) {
    httpcContext context;
    const char* url_ptr = url.c_str();
    char* newurl = nullptr;
    Result ret = 0;
    u32 statuscode = 0;
    u32 readsize = 0, size = 0;

    do {
        httpcOpenContext(&context, method, url_ptr, 1);
        httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
        httpcSetKeepAlive(&context,
                          keep_alive ? HTTPC_KEEPALIVE_ENABLED : HTTPC_KEEPALIVE_DISABLED);
        for (auto& header : headers)
            httpcAddRequestHeaderField(&context, header.first.c_str(), header.second.c_str());
        if ((body.length() != 0) && (method == HTTPC_METHOD_POST))
            httpcAddPostDataRaw(&context, (u32*)body.c_str(), body.length());
        ret = httpcBeginRequest(&context);
        if (ret != 0) {
            httpcCloseContext(&context);
            if (newurl != nullptr)
                delete newurl;
            RETURN_ERROR
        }
        ret = httpcGetResponseStatusCode(&context, &statuscode);
        if (ret != 0) {
            httpcCloseContext(&context);
            if (newurl != nullptr)
                delete newurl;
            RETURN_ERROR
        }
        if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
            if (newurl == nullptr) {
                newurl = new char[0x1000];
                httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
                url_ptr = newurl;
                httpcCloseContext(&context);
            }
        }
    } while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

    std::vector<u8> buffer(0x1000);

    if (response_has_body) {
        do {
            ret = httpcDownloadData(&context, buffer.data() + size, 0x1000, &readsize);
            size += readsize;
            if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
                buffer.resize(size + 0x1000);
            }
        } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

        if (ret != 0) {
            httpcCloseContext(&context);
            if (newurl != nullptr)
                delete newurl;
            buffer.clear();
            RETURN_ERROR
        }

        buffer.resize(size);
    }

    Response res;
    res.result = ret;
    res.status_code = statuscode;
    res.size = size;
    if (response_has_body) {
        res.data = buffer;
        res.text = std::string(reinterpret_cast<const char*>(buffer.data()));
    }
    for (auto& name : response_headers) {
        char* value = new char[0x1000];
        httpcGetResponseHeader(&context, name.c_str(), value, 0x1000);
        res.headers.emplace(name, value);
    }
    httpcCloseContext(&context);
    return res;
}

Response Get(const std::string& url, bool keep_alive = false, Headers headers = Headers(),
             std::vector<std::string> response_headers = std::vector<std::string>()) {
    return Send(HTTPC_METHOD_GET, url, std::string(), true, keep_alive, headers, response_headers);
}

Response Post(const std::string& url, const std::string& body, bool keep_alive = false,
              Headers headers = Headers(),
              std::vector<std::string> response_headers = std::vector<std::string>()) {
    return Send(HTTPC_METHOD_POST, url, body, true, keep_alive, headers, response_headers);
}

Response Put(const std::string& url, const std::string& body, bool keep_alive = false,
             Headers headers = Headers(),
             std::vector<std::string> response_headers = std::vector<std::string>()) {
    return Send(HTTPC_METHOD_PUT, url, body, false, keep_alive, headers, response_headers);
}

Response Delete(const std::string& url, const std::string& body, bool keep_alive = false,
                Headers headers = Headers(),
                std::vector<std::string> response_headers = std::vector<std::string>()) {
    return Send(HTTPC_METHOD_DELETE, url, body, true, keep_alive, headers, response_headers);
}

Response Head(const std::string& url, bool keep_alive = false, Headers headers = Headers(),
              std::vector<std::string> response_headers = std::vector<std::string>()) {
    return Send(HTTPC_METHOD_HEAD, url, std::string(), false, keep_alive, headers,
                response_headers);
}
} // namespace http
} // namespace network

namespace hid {

inline void poll() {
    hidScanInput();
}

inline bool released(u32 buttons) {
    return (hidKeysUp() & buttons) != 0;
}

inline bool pressed(u32 buttons) {
    return (hidKeysDown() & buttons) != 0;
}

inline bool held(u32 buttons) {
    return (hidKeysHeld() & buttons) != 0;
}

inline touchPosition ctr::hid::touch() {
    touchPosition pos;
    hidTouchRead(&pos);
    return pos;
}

inline circlePosition circlePad() {
    circlePosition pos;
    hidCircleRead(&pos);
    return pos;
}

inline circlePosition ctr::hid::cStick() {
    circlePosition pos;
    irrstCstickRead(&pos);
    return pos;
}

Result enableAccelerometer() {
    return HIDUSER_EnableAccelerometer();
}

Result disableAccelerometer() {
    return HIDUSER_DisableAccelerometer();
}

inline accelVector accelerometer() {
    accelVector vec;
    hidAccelRead(&vec);
    return vec;
}

Result enableGyroscope() {
    return HIDUSER_EnableGyroscope();
}

Result disableGyroscope() {
    return HIDUSER_DisableGyroscope();
}

inline angularRate gyroscope() {
    angularRate rate;
    hidGyroRead(&rate);
    return rate;
}
} // namespace hid

namespace news {

Result init() {
    return newsInit();
}

void exit() {
    newsExit();
}

Result add() {
    return NEWS_AddNotification((const u16*)title.c_str(), title.length(), (const u16*)message.c_str(), message.length(), image, imageSize, jpeg);
}
} // namespace news

namespace battery {

Result init() {
    return ptmuInit() & mcuHwcInit();
}

void exit() {
    ptmuExit();
    mcuHwcExit();
}

bool charging() {
    u8 charging;
    PTMU_GetBatteryChargeState(&charging);
    return charging != 0;
}

inline u8 level(bool mcu) {
    u8 level;
    if (mcu)
        MCUHWC_GetBatteryLevel(&level);
    else
        PTMU_GetBatteryLevel(&level);
    return level;
}
} // namespace battery
} // namespace ctr
