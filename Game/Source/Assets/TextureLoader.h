#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include "raylib.h"

#include <string>

namespace game
{
    // Loads a texture from disk, transparently preferring a cooked .qoi file
    // when the requested image path has a .qoi sibling (e.g. "foo.png" loads
    // "foo.qoi" when present). The GPU texture is cached with reference
    // counting: each successful call must be paired with TextureRelease().
    //
    // The QOI file is decoded straight to an RGBA buffer that is uploaded to
    // the GPU and then freed, so disk -> decode -> VRAM never keeps a second
    // CPU copy of the pixels around.
    Texture2D TextureAcquire(const std::string &path);

    // Releases one reference acquired with TextureAcquire(). The GPU texture
    // is unloaded when the last reference is released.
    void TextureRelease(Texture2D texture);

    // True when the file exists or a cooked .qoi sibling exists. Used by
    // callers that probe several candidate paths before loading.
    bool TextureFileExists(const std::string &path);

    // Unloads every texture still in the cache. Call at shutdown.
    void TextureCacheClear();
}

#endif // TEXTURELOADER_H
