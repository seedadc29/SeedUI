#include "EmbeddedAssets.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace seedui
{
    bool LoadEmbeddedResource(const char* name, std::vector<unsigned char>& out)
    {
        if (!name || !name[0]) return false;

        HRSRC res = FindResourceA(nullptr, name, (LPCSTR)RT_RCDATA);
        if (!res) return false;

        HGLOBAL hData = LoadResource(nullptr, res);
        if (!hData) return false;

        const DWORD size = SizeofResource(nullptr, res);
        const void* ptr = LockResource(hData);
        if (!ptr || size == 0) return false;

        out.assign(static_cast<const unsigned char*>(ptr),
                   static_cast<const unsigned char*>(ptr) + size);
        return true;
    }

    bool LoadEmbeddedText(const char* name, std::string& out)
    {
        std::vector<unsigned char> data;
        if (!LoadEmbeddedResource(name, data)) return false;
        out.assign(reinterpret_cast<const char*>(data.data()), data.size());
        return true;
    }
}
