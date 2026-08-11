#ifndef INVENTORY_H
#define INVENTORY_H

#include "raylib.h"

#include <array>
#include <utility>

namespace game
{
    enum class ItemType : unsigned char
    {
        None,
        HealthPotion,
        IronAxe,
        LeatherArmor,
        RangerArmor,
    };

    enum class EquipmentSlot : unsigned char
    {
        Weapon,
        Armor,
        Potion,
    };

    struct InventorySlot
    {
        ItemType type = ItemType::None;
        int count = 0;

        bool IsEmpty() const { return type == ItemType::None || count <= 0; }
        void Clear() { type = ItemType::None; count = 0; }
    };

    inline const char *GetItemName(ItemType type)
    {
        switch (type)
        {
        case ItemType::HealthPotion: return "Poção de vida";
        case ItemType::IronAxe: return "Machado de ferro";
        case ItemType::LeatherArmor: return "Armadura camponesa";
        case ItemType::RangerArmor: return "Armadura ranger";
        default: return "Vazio";
        }
    }

    inline const char *GetItemIconFile(ItemType type)
    {
        switch (type)
        {
        case ItemType::HealthPotion: return "Assets/Items/Potion1_Filled_Red.png";
        case ItemType::IronAxe: return "Assets/Items/Axe_Double.png";
        case ItemType::LeatherArmor: return "Assets/Items/Armor_Leather.png";
        case ItemType::RangerArmor: return "Assets/Items/Armor_Metal.png";
        default: return "";
        }
    }

    inline const char *GetItemModelFile(ItemType type)
    {
        switch (type)
        {
        case ItemType::HealthPotion: return "Potion1_Filled.fbx";
        case ItemType::IronAxe: return "Axe_Double.fbx";
        case ItemType::LeatherArmor: return "Armor_Leather.fbx";
        case ItemType::RangerArmor: return "Armor_Metal.fbx";
        default: return "";
        }
    }

    inline Color GetItemColor(ItemType type)
    {
        switch (type)
        {
        case ItemType::HealthPotion: return { 224, 65, 77, 255 };
        case ItemType::IronAxe: return { 150, 166, 181, 255 };
        case ItemType::LeatherArmor: return { 155, 105, 57, 255 };
        case ItemType::RangerArmor: return { 75, 130, 92, 255 };
        default: return { 70, 74, 85, 255 };
        }
    }

    inline bool IsConsumable(ItemType type) { return type == ItemType::HealthPotion; }
    inline bool IsWeapon(ItemType type) { return type == ItemType::IronAxe; }
    inline bool IsArmor(ItemType type)
    {
        return type == ItemType::LeatherArmor || type == ItemType::RangerArmor;
    }

    inline bool CanEquipInSlot(ItemType type, EquipmentSlot slot)
    {
        switch (slot)
        {
        case EquipmentSlot::Weapon: return IsWeapon(type);
        case EquipmentSlot::Armor: return IsArmor(type);
        case EquipmentSlot::Potion: return IsConsumable(type);
        default: return false;
        }
    }

    class Inventory
    {
    public:
        static constexpr int SlotCount = 16;

        bool Add(ItemType type, int count = 1)
        {
            for (InventorySlot &slot : mSlots)
            {
                if (slot.type == type && !slot.IsEmpty())
                {
                    slot.count += count;
                    return true;
                }
            }
            for (InventorySlot &slot : mSlots)
            {
                if (slot.IsEmpty())
                {
                    slot.type = type;
                    slot.count = count;
                    return true;
                }
            }
            return false;
        }

        InventorySlot &GetSlot(int index) { return mSlots[index]; }
        const InventorySlot &GetSlot(int index) const { return mSlots[index]; }
        void Swap(int a, int b) { std::swap(mSlots[a], mSlots[b]); }

    private:
        std::array<InventorySlot, SlotCount> mSlots = {};
    };
}

#endif // INVENTORY_H
