#include "ScreenCapture.h"

// Este arquivo inclui <windows.h> de propósito — fica isolado do raylib
// para não colidir com nomes (CloseWindow, ShowCursor, DrawText...).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib>

namespace seednotas
{
    bool CaptureScreenRGBA(int& width, int& height, unsigned char*& outPixels)
    {
        width = 0;
        height = 0;
        outPixels = nullptr;

        const int screenW = GetSystemMetrics(SM_CXSCREEN);
        const int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (screenW <= 0 || screenH <= 0) return false;

        HDC hdcScreen = GetDC(nullptr);
        if (!hdcScreen) return false;

        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenW, screenH);
        if (!hdcMem || !hBitmap)
        {
            if (hBitmap) DeleteObject(hBitmap);
            if (hdcMem) DeleteDC(hdcMem);
            ReleaseDC(nullptr, hdcScreen);
            return false;
        }

        HGDIOBJ oldBmp = SelectObject(hdcMem, hBitmap);
        BitBlt(hdcMem, 0, 0, screenW, screenH, hdcScreen, 0, 0, SRCCOPY);

        // Lê os pixels como BGRA 32bpp (formato nativo do GDI).
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = screenW;
        bmi.bmiHeader.biHeight = -screenH; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        const size_t count = (size_t)screenW * (size_t)screenH * 4u;
        unsigned char* bgra = (unsigned char*)malloc(count);
        bool ok = bgra && GetDIBits(hdcMem, hBitmap, 0, screenH, bgra, &bmi,
                                    DIB_RGB_COLORS) == screenH;

        SelectObject(hdcMem, oldBmp);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);

        if (!ok)
        {
            if (bgra) free(bgra);
            return false;
        }

        // BGRA -> RGBA
        unsigned char* rgba = (unsigned char*)malloc(count);
        if (!rgba)
        {
            free(bgra);
            return false;
        }
        for (size_t i = 0; i < count; i += 4)
        {
            rgba[i + 0] = bgra[i + 2];
            rgba[i + 1] = bgra[i + 1];
            rgba[i + 2] = bgra[i + 0];
            rgba[i + 3] = 255;
        }
        free(bgra);

        width = screenW;
        height = screenH;
        outPixels = rgba;
        return true;
    }
}
