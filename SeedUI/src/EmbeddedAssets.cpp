#include "EmbeddedAssets.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cctype>

namespace seedui
{
    bool LoadEmbeddedResource(const char* name, std::vector<unsigned char>& out)
    {
        if (!name || !name[0]) return false;

        // O rc.exe grava os nomes dos recursos em MAIUSCULAS e entre aspas
        // (ex.: "INTER.TTF"). Tenta primeiro o nome direto e depois a forma
        // que o rc.exe efetivamente grava no executavel.
        HRSRC res = FindResourceA(nullptr, name, (LPCSTR)RT_RCDATA);
        if (!res)
        {
            std::string alt = "\"";
            for (const char* c = name; *c; ++c)
                alt += (char)(unsigned char)toupper((unsigned char)*c);
            alt += "\"";
            res = FindResourceA(nullptr, alt.c_str(), (LPCSTR)RT_RCDATA);
        }
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
