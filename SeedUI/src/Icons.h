#ifndef SEEDUI_ICONS_H
#define SEEDUI_ICONS_H

#include <cstdint>

#include "imgui.h"

namespace seedui
{
    constexpr ImU32 kIconTintDefault = IM_COL32(236, 236, 236, 255);
    // A ordem deste enum deve espelhar a ordem da tabela em Icons.cpp
    enum class IconId
    {
        Select,
        Move,
        Text,
        ZoomIn,
        ZoomOut,
        Pan,
        Color,
        Grid,
        New,
        Open,
        Save,
        Undo,
        Redo,
        Copy,
        Paste,
        Duplicate,
        Trash,
        Gear,
        Question,
        Plus,
        Check,
        X,
        Eye,
        Lock,
        Image,
        Palette,
        Sparkle,
        Heart,
        Warning,
        CaretDown,
        CaretRight,
        Download,
        Hierarchy,
        Inspector,
        Library,
        Directives,
        Assets,
        History,
        Model,
        Magnifier,
        Magnet,
        List,
        Annotate,
        Rectangle,
        Ellipse,
        Polygon,
        Contour,
        FlipH,
        FlipV,
        Transparency,
        CornersOut,
        Count,
    };

    void Load();
    void Unload();
    void DrawIcon(IconId id, float size = 16.0f, ImU32 tint = kIconTintDefault);
    void DrawIconAt(IconId id, float x, float y, float size = 16.0f,
                    ImU32 tint = kIconTintDefault);
    bool IconButton(IconId id, const char* tooltip, float buttonSize = 34.0f,
                    ImU32 tint = kIconTintDefault);
}

#endif // SEEDUI_ICONS_H
