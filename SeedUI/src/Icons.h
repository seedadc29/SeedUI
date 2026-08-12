#ifndef SEEDUI_ICONS_H
#define SEEDUI_ICONS_H

namespace seedui
{
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
        List,
        Annotate,
        Rectangle,
        Ellipse,
        Polygon,
        Transparency,
        Count,
    };

    void Load();
    void Unload();
    void DrawIcon(IconId id, float size = 16.0f);
    void DrawIconAt(IconId id, float x, float y, float size = 16.0f);
    bool IconButton(IconId id, const char* tooltip, float buttonSize = 34.0f);
}

#endif // SEEDUI_ICONS_H
