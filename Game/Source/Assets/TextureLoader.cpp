#include "Assets/TextureLoader.h"

#include <algorithm>
#include <string>
#include <unordered_map>

// QOI codec vendored from https://github.com/phoboslab/qoi (MIT license).
// Only the declarations are included here; the implementation is compiled once
// by raylib (rtextures.c), so the binary keeps a single decoder.
#include "qoi.h"

namespace game
{
    namespace
    {
        struct CachedTexture
        {
            Texture2D texture = {};
            int refCount = 0;
        };

        std::unordered_map<std::string, CachedTexture> gTextureCache;

        std::string LowerExtension(const std::string &path)
        {
            const size_t dot = path.find_last_of('.');
            const size_t slash = path.find_last_of("/\\");
            if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
                return "";
            std::string ext = path.substr(dot);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            return ext;
        }

        bool IsImageExtension(const std::string &ext)
        {
            return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
                   ext == ".bmp" || ext == ".tga" || ext == ".gif" || ext == ".qoi";
        }

        std::string WithQoiExtension(const std::string &path)
        {
            const size_t dot = path.find_last_of('.');
            const size_t slash = path.find_last_of("/\\");
            std::string base = path;
            if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
                base.resize(dot);
            return base + ".qoi";
        }

        // Decodes a QOI file into an RGBA buffer, uploads it to the GPU and
        // frees the CPU copy. Returns an empty texture on failure.
        Texture2D LoadQoiTexture(const std::string &path)
        {
            int fileSize = 0;
            unsigned char *fileData = LoadFileData(path.c_str(), &fileSize);
            if (!fileData || fileSize < 22)
            {
                if (fileData) MemFree(fileData);
                return {};
            }

            qoi_desc desc = {};
            // Always decode to RGBA: one canonical format for the upload path.
            void *pixels = qoi_decode(fileData, (int)fileSize, &desc, 4);
            MemFree(fileData);
            if (!pixels) return {};

            Image image = {};
            image.data = pixels;
            image.width = desc.width;
            image.height = desc.height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

            // LoadTextureFromImage uploads the data; it does not take ownership.
            Texture2D texture = LoadTextureFromImage(image);
            MemFree(pixels);
            return texture;
        }

        Texture2D LoadAny(const std::string &path)
        {
            const std::string ext = LowerExtension(path);

            if (ext == ".qoi") return LoadQoiTexture(path);

            const std::string qoiPath = WithQoiExtension(path);
            if (IsImageExtension(ext) && FileExists(qoiPath.c_str()))
                return LoadQoiTexture(qoiPath);

            // Fall back to the engine loaders (png/jpg/etc.) when no cooked
            // .qoi exists, e.g. when running from the source asset tree.
            return LoadTexture(path.c_str());
        }
    }

    Texture2D TextureAcquire(const std::string &path)
    {
        auto found = gTextureCache.find(path);
        if (found != gTextureCache.end())
        {
            ++found->second.refCount;
            return found->second.texture;
        }

        Texture2D texture = LoadAny(path);
        if (texture.id != 0) gTextureCache[path] = { texture, 1 };
        return texture;
    }

    void TextureRelease(Texture2D texture)
    {
        if (texture.id == 0) return;
        for (auto it = gTextureCache.begin(); it != gTextureCache.end(); ++it)
        {
            if (it->second.texture.id != texture.id) continue;
            if (--it->second.refCount <= 0)
            {
                UnloadTexture(it->second.texture);
                gTextureCache.erase(it);
            }
            return;
        }
        // Not tracked by the cache; unload directly so nothing leaks.
        UnloadTexture(texture);
    }

    bool TextureFileExists(const std::string &path)
    {
        if (FileExists(path.c_str())) return true;
        const std::string ext = LowerExtension(path);
        if (IsImageExtension(ext) && ext != ".qoi")
            return FileExists(WithQoiExtension(path).c_str());
        return false;
    }

    void TextureCacheClear()
    {
        for (auto &entry : gTextureCache)
            if (entry.second.texture.id != 0) UnloadTexture(entry.second.texture);
        gTextureCache.clear();
    }
}
