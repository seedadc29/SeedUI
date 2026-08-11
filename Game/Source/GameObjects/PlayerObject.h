#ifndef PLAYEROBJECT_H
#define PLAYEROBJECT_H

#include "GameObject.h"
#include "Scene.h"
#include "Assets/FbxModel.h"
#include "Inventory.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include "glm/glm.hpp"

namespace game
{
    class PlayerObject : public GameObject
    {
    public:
        enum class InputState : unsigned char
        {
            None = 0,
            Forward = 1 << 0,
            Backward = 1 << 1,
            Left = 1 << 2,
            Right = 1 << 3,
            Jump = 1 << 4,
        };

        PlayerObject(const glm::vec3 &spawnPosition);
        ~PlayerObject() override;

        void OnSpawn(Scene &scene) override;
        void Update(Scene &scene, float deltaTime) override;
        void Draw(Scene &scene) override;
        void DrawDebug(Scene &scene) override;
        void ApplyCharacterAppearance(CharacterGender gender, float skinHue,
                                      float skinSaturation, float lightIntensity);

        glm::vec3 GetCameraTarget() const;
        glm::vec3 GetCameraOffset() const { return { 0.0f, mCameraEyeHeight, 0.0f }; }

        const glm::vec3 &GetVelocity() const { return mVelocity; }
        bool IsRunning() const { return mIsRunning; }
        bool IsOnGround() const { return mOnGround; }
        bool IsAttacking() const { return mAttackTime > 0.0f; }
        bool IsDead() const { return mHealth <= 0.0f; }

        float GetHealth() const { return mHealth; }
        float GetMaxHealth() const { return mMaxHealth; }
        int GetLevel() const { return mLevel; }
        float GetExperience() const { return mExperience; }
        float GetExperienceToNextLevel() const { return mExperienceToNextLevel; }
        int GetMoney() const { return mMoney; }
        const Inventory &GetInventory() const { return mInventory; }
        Inventory &GetInventory() { return mInventory; }
        const InventorySlot &GetEquipment(EquipmentSlot slot) const;

        bool UseInventorySlot(int index);
        bool UseEquippedPotion();
        bool MoveInventoryToEquipment(int inventoryIndex, EquipmentSlot equipmentSlot);
        bool MoveEquipmentToInventory(EquipmentSlot equipmentSlot, int inventoryIndex);
        bool EquipInventorySlot(int index);
        bool DropInventorySlot(int index, ItemType &type, int &count);
        bool DropEquipmentSlot(EquipmentSlot equipmentSlot, ItemType &type, int &count);
        void AddExperience(float amount);
        void AddMoney(int amount) { mMoney += amount; }
        float TakeDamage(float amount);
        void Heal(float amount);
        float GetAttackDamage() const;
        // Ferramentas de desenvolvedor (F2): ajuste direto dos atributos.
        void DevSetHealth(float value);
        void DevSetMaxHealth(float value);
        void DevSetLevel(int value);
        void DevSetExperience(float value);
        void DevSetMoney(int value);

    private:
        void HandleInput();
        void UpdateMovement(Scene &scene, float deltaTime);
        void RefreshOutfitModel();
        void RefreshWeaponModel();
        void PlayLocomotionAnimation();

        glm::vec3 mSpawnPosition;
        glm::vec3 mVelocity = glm::vec3(0.0f);

        JPH::CharacterVirtual *mCharacter = nullptr;
        JPH::CapsuleShape *    mShape = nullptr;

        unsigned char mInputBits = 0;
        bool mIsRunning = false;
        bool mMovingBackward = false;
        bool mOnGround = false;
        float mShiftTapTime = 0.0f;
        float mShiftTapWindow = 0.3f;
        bool mShiftHeld = false;
        bool mShiftStickyRun = false;

        FbxModel mPlayerModel;
        FbxModel mOutfitModel;
        FbxModel mWeaponModel;
        Inventory mInventory;
        InventorySlot mEquippedWeapon;
        InventorySlot mEquippedArmor;
        InventorySlot mEquippedPotion;
        CharacterGender mCharacterGender = CharacterGender::Male;
        float mSkinHue = 0.06f;
        float mSkinSaturation = 0.72f;
        float mLightIntensity = 1.0f;
        float mFacingYawDegrees = 180.0f;
        float mHorizontalSpeed = 0.0f;
        float mVisualHeight = 1.78f;
        float mHealth = 72.0f;
        float mMaxHealth = 100.0f;
        int mLevel = 1;
        float mExperience = 0.0f;
        float mExperienceToNextLevel = 100.0f;
        int mMoney = 0;
        float mAttackCooldown = 0.0f;
        float mAttackTime = 0.0f;
        bool mAttackClipFound = false;
        float mHitFlash = 0.0f;
        float mWalkSpeed = 4.0f;
        float mRunSpeed = 8.0f;
        float mJumpSpeed = 7.0f;
        float mCameraEyeHeight = 1.6f;
        float mCapsuleHeight = 1.8f;
        float mCapsuleRadius = 0.35f;
    };
}

#endif // PLAYEROBJECT_H
