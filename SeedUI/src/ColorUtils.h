#ifndef SEEDUI_COLORUTILS_H
#define SEEDUI_COLORUTILS_H

// Utilitários de cor compartilhados (canvas, paleta e seletor). O formato do
// projeto guarda cores como texto "#rrggbb" em estilos.cor_fundo/cor_borda.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace seedui
{
    namespace ColorUtils
    {
        // Suporta #RGB, #RGBA, #RRGGBB e #RRGGBBAA, além de "none" e "transparent".
        inline bool ParseHexWithAlpha(const std::string& text, float rgb[3], float& alpha)
        {
            alpha = 1.0f;
            if (text == "none" || text == "transparent" || text == "transparente" || text.empty())
            {
                rgb[0] = rgb[1] = rgb[2] = 0.0f;
                alpha = 0.0f;
                return true;
            }
            const char* p = text.c_str();
            if (*p == '#') ++p;
            unsigned int value = 0;
            int digits = 0;
            while (*p && digits < 8)
            {
                const char c = *p;
                int v = -1;
                if (c >= '0' && c <= '9') v = c - '0';
                else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
                else break;
                value = value * 16 + (unsigned int)v;
                ++p;
                ++digits;
            }
            if (digits == 8)
            {
                rgb[0] = ((value >> 24) & 0xFF) / 255.0f;
                rgb[1] = ((value >> 16) & 0xFF) / 255.0f;
                rgb[2] = ((value >> 8) & 0xFF) / 255.0f;
                alpha = (value & 0xFF) / 255.0f;
                return true;
            }
            if (digits == 6)
            {
                rgb[0] = ((value >> 16) & 0xFF) / 255.0f;
                rgb[1] = ((value >> 8) & 0xFF) / 255.0f;
                rgb[2] = (value & 0xFF) / 255.0f;
                alpha = 1.0f;
                return true;
            }
            if (digits == 3)
            {
                int r = (value >> 8) & 0xF;
                int g = (value >> 4) & 0xF;
                int b = value & 0xF;
                rgb[0] = ((r << 4) | r) / 255.0f;
                rgb[1] = ((g << 4) | g) / 255.0f;
                rgb[2] = ((b << 4) | b) / 255.0f;
                alpha = 1.0f;
                return true;
            }
            if (digits == 4)
            {
                int r = (value >> 12) & 0xF;
                int g = (value >> 8) & 0xF;
                int b = (value >> 4) & 0xF;
                int a = value & 0xF;
                rgb[0] = ((r << 4) | r) / 255.0f;
                rgb[1] = ((g << 4) | g) / 255.0f;
                rgb[2] = ((b << 4) | b) / 255.0f;
                alpha = ((a << 4) | a) / 255.0f;
                return true;
            }
            return false;
        }

        // "#rrggbb" (ou "rrggbb") -> rgb em 0..1. Retorna false se inválido.
        inline bool ParseHex(const std::string& text, float rgb[3])
        {
            float alpha = 1.0f;
            return ParseHexWithAlpha(text, rgb, alpha);
        }

        inline std::string ToHex(float r, float g, float b)
        {
            char buf[8];
            snprintf(buf, sizeof buf, "#%02X%02X%02X",
                     (int)(std::max(0.0f, std::min(1.0f, r)) * 255.0f + 0.5f),
                     (int)(std::max(0.0f, std::min(1.0f, g)) * 255.0f + 0.5f),
                     (int)(std::max(0.0f, std::min(1.0f, b)) * 255.0f + 0.5f));
            return buf;
        }

        inline void RgbToHsv(float r, float g, float b, float& h, float& s, float& v)
        {
            const float maxc = std::max(r, std::max(g, b));
            const float minc = std::min(r, std::min(g, b));
            v = maxc;
            const float delta = maxc - minc;
            s = maxc > 0.0f ? delta / maxc : 0.0f;
            if (delta < 0.0001f) { h = 0.0f; return; }
            float hue = 0.0f;
            if (maxc == r) hue = 60.0f * fmodf((g - b) / delta, 6.0f);
            else if (maxc == g) hue = 60.0f * ((b - r) / delta + 2.0f);
            else hue = 60.0f * ((r - g) / delta + 4.0f);
            if (hue < 0.0f) hue += 360.0f;
            h = hue;
        }

        inline void HsvToRgb(float h, float s, float v, float& r, float& g, float& b)
        {
            h = fmodf(h, 360.0f);
            if (h < 0.0f) h += 360.0f;
            const float c = v * s;
            const float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
            const float m = v - c;
            if (h < 60.0f)       { r = c; g = x; b = 0.0f; }
            else if (h < 120.0f) { r = x; g = c; b = 0.0f; }
            else if (h < 180.0f) { r = 0.0f; g = c; b = x; }
            else if (h < 240.0f) { r = 0.0f; g = x; b = c; }
            else if (h < 300.0f) { r = x; g = 0.0f; b = c; }
            else                 { r = c; g = 0.0f; b = x; }
            r += m; g += m; b += m;
        }
    }
}

#endif // SEEDUI_COLORUTILS_H
