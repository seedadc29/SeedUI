#include "GameObjects/PlayerObject.h"
#include "Scene.h"

#include "raylib.h"
#include "raymath.h"

#include <Jolt/Physics/Character/CharacterBase.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <algorithm>

namespace game
{
    namespace
    {
        constexpr float ModelForwardYawOffsetDegrees = 180.0f;
        constexpr const char *IdleAnimation = "Idle_Loop";
        constexpr const char *WalkAnimation = "Walk_Loop";
        constexpr const char *RunAnimation = "Sprint_Loop";
        constexpr const char *JumpAnimation = "Jump_Loop";
    }

    PlayerObject::PlayerObject(const glm::vec3 &spawnPosition)
        : mSpawnPosition(spawnPosition)
    {
        mPosition = spawnPosition;
    }

    PlayerObject::~PlayerObject()
    {
        delete mCharacter;
        // Shape owned by RefConst; mCharacter releases its refcount on destruction.
    }

    void PlayerObject::OnSpawn(Scene &scene)
    {
        JPH::CharacterVirtualSettings settings;
        settings.mMaxSlopeAngle = JPH::DegreesToRadians(50.0f);
        settings.mUp = JPH::Vec3(0,1,0);
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3(0,1,0), -mCapsuleHeight*0.5f);
        JPH::CapsuleShape *capsule = new JPH::CapsuleShape(mCapsuleHeight*0.5f, mCapsuleRadius);
        settings.mShape = capsule;

        mCharacter = new JPH::CharacterVirtual(&settings,
            JPH::RVec3(mSpawnPosition.x, mSpawnPosition.y, mSpawnPosition.z),
            JPH::Quat::sIdentity(),
            &scene.GetPhysics().GetSystem());

        mShape = capsule;

        ApplyCharacterAppearance(scene.GetSettings().characterGender,
                                 scene.GetSettings().skinHue,
                                 scene.GetSettings().skinSaturation,
                                 scene.GetSettings().characterLightIntensity);
    }

    void PlayerObject::ApplyCharacterAppearance(CharacterGender gender, float skinHue,
                                                float skinSaturation, float lightIntensity)
    {
        const bool mustReloadModel = mCharacterGender != gender || !mPlayerModel.IsLoaded();

        mCharacterGender = gender;
        mSkinHue = skinHue;
        mSkinSaturation = skinSaturation;
        mLightIntensity = lightIntensity;
        if (!mustReloadModel) return;
        const bool female = gender == CharacterGender::Female;
        std::string modelPath = female
            ? "Assets/Characters/PlayerFemale.fbx"
            : "Assets/Characters/PlayerMale.fbx";
        std::string animationPath = "Assets/Characters/PlayerAnimations.fbx";
        if (!FileExists(modelPath.c_str()))
            modelPath = female
                ? "Game/Assets/Kit assets/Universal Base Characters[Standard]/Base Characters/Unity/Superhero_Female_FullBody.fbx"
                : "Game/Assets/Kit assets/Universal Base Characters[Standard]/Base Characters/Unity/Superhero_Male_FullBody.fbx";
        if (!FileExists(animationPath.c_str()))
            animationPath = "Game/Assets/Kit assets/Universal Animation Library[Standard]/Unity/UAL1_Standard.fbx";

        if (!mPlayerModel.Load(modelPath, animationPath))
            TraceLog(LOG_WARNING, "PLAYER: %s. Falling back to debug cube.", mPlayerModel.GetLastError().c_str());
        RefreshOutfitModel();
    }

    void PlayerObject::RefreshOutfitModel()
    {
        if (mEquippedArmor.IsEmpty())
        {
            mOutfitModel.Unload();
            return;
        }

        const ItemType armor = mEquippedArmor.type;
        const bool ranger = armor == ItemType::RangerArmor;
        const bool female = mCharacterGender == CharacterGender::Female;
        const char *outfitName = ranger ? (female ? "Female_Ranger.fbx" : "Male_Ranger.fbx")
                                        : (female ? "Female_Peasant.fbx" : "Male_Peasant.fbx");
        std::string outfitPath = std::string("Assets/Characters/Outfits/") + outfitName;
        if (!FileExists(outfitPath.c_str()))
        {
            outfitPath = std::string("Game/Assets/Kit assets/Modular Character Outfits - Fantasy[Standard]") +
                         "/Exports/FBX (Unity)/Outfits/" + outfitName;
        }
        std::string animationPath = "Assets/Characters/PlayerAnimations.fbx";
        if (!FileExists(animationPath.c_str()))
            animationPath = "Game/Assets/Kit assets/Universal Animation Library[Standard]/Unity/UAL1_Standard.fbx";
        if (!mOutfitModel.Load(outfitPath, animationPath))
            TraceLog(LOG_WARNING, "PLAYER: Could not load equipped outfit: %s", mOutfitModel.GetLastError().c_str());
    }

    void PlayerObject::RefreshWeaponModel()
    {
        if (mEquippedWeapon.IsEmpty())
        {
            mWeaponModel.Unload();
            return;
        }
        std::string modelPath = std::string("Assets/Items/") + GetItemModelFile(mEquippedWeapon.type);
        if (!FileExists(modelPath.c_str()))
            modelPath = std::string("Game/Assets/UltimateRPg Items Pack/FBX/") + GetItemModelFile(mEquippedWeapon.type);
        if (!mWeaponModel.Load(modelPath, std::string()))
            TraceLog(LOG_WARNING, "PLAYER: Could not load equipped weapon: %s", mWeaponModel.GetLastError().c_str());
    }

    void PlayerObject::HandleInput()
    {
        mInputBits = 0;
        if (IsKeyDown(KEY_W)) mInputBits |= (unsigned char)InputState::Forward;
        if (IsKeyDown(KEY_S)) mInputBits |= (unsigned char)InputState::Backward;
        if (IsKeyDown(KEY_A)) mInputBits |= (unsigned char)InputState::Left;
        if (IsKeyDown(KEY_D)) mInputBits |= (unsigned char)InputState::Right;

        const bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // Double tap detection: within mShiftTapWindow seconds of the previous release,
        // toggle sticky run until a new tap or release of W happens.
        if (shiftDown && !mShiftHeld)
        {
            if (mShiftTapTime > 0.0f && mShiftTapTime <= mShiftTapWindow)
            {
                mShiftStickyRun = true;
            }
            mShiftTapTime = -1.0f; // reset window while held
        }
        if (!shiftDown && mShiftHeld)
        {
            mShiftTapTime = (float)GetTime();
        }
        mShiftHeld = shiftDown;

        mIsRunning = mShiftStickyRun || shiftDown;

        // Cancel sticky run if not moving forward anymore.
        if ((mInputBits & (unsigned char)InputState::Forward) == 0 && mShiftStickyRun && !shiftDown)
        {
            mShiftStickyRun = false;
        }

        if (IsKeyPressed(KEY_SPACE))
            mInputBits |= (unsigned char)InputState::Jump;
    }

    void PlayerObject::UpdateMovement(Scene &scene, float deltaTime)
    {
        HandleInput();

        Camera &cam = scene.GetCamera();
        // Build a flat forward / right from camera yaw.
        Vector3 camForward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
        Vector3 camRight   = Vector3Normalize(Vector3CrossProduct(camForward, { 0,1,0 }));
        Vector3 wishDir = { 0,0,0 };

        if (mInputBits & (unsigned char)InputState::Forward)  wishDir = Vector3Add(wishDir, camForward);
        if (mInputBits & (unsigned char)InputState::Backward) wishDir = Vector3Subtract(wishDir, camForward);
        if (mInputBits & (unsigned char)InputState::Right)    wishDir = Vector3Add(wishDir, camRight);
        if (mInputBits & (unsigned char)InputState::Left)     wishDir = Vector3Subtract(wishDir, camRight);

        // Flatten (no vertical contribution from camera): zero the Y (world up).
        wishDir.y = 0.0f;
        mMovingBackward = (mInputBits & (unsigned char)InputState::Backward) != 0 &&
                          (mInputBits & (unsigned char)InputState::Forward) == 0;
        float wishLen = sqrtf(wishDir.x*wishDir.x + wishDir.z*wishDir.z);
        if (wishLen > 0.0001f)
        {
            wishDir = Vector3Scale(wishDir, 1.0f / wishLen);
            if (scene.GetCameraController().GetView() == CameraView::TopDown)
            {
                const Vector3 facingDirection = mMovingBackward
                    ? Vector3Scale(wishDir, -1.0f) : wishDir;
                mFacingYawDegrees = atan2f(facingDirection.x, facingDirection.z) * RAD2DEG +
                                    ModelForwardYawOffsetDegrees;
            }
            else
            {
                const float cameraYaw = scene.GetCameraController().GetYaw();
                mFacingYawDegrees = cameraYaw * RAD2DEG + ModelForwardYawOffsetDegrees;
            }
        }
        else
        {
            wishDir = { 0,0,0 };
        }

        const float forwardSpeed = mIsRunning ? mRunSpeed : mWalkSpeed;
        float speed = mMovingBackward
            ? (mIsRunning ? mRunSpeed * 1.12f : mWalkSpeed * 0.78f)
            : forwardSpeed;
        JPH::Vec3 desiredHorizontal = JPH::Vec3(wishDir.x * speed, 0.0f, wishDir.z * speed);

        const JPH::Vec3 gravity(0.0f, -19.62f, 0.0f);

        JPH::Vec3 currentVelocity = mCharacter->GetLinearVelocity();

        // Determine ground state.
        const JPH::CharacterBase::EGroundState groundState = mCharacter->GetGroundState();
        bool onGround = groundState == JPH::CharacterBase::EGroundState::OnGround ||
                        groundState == JPH::CharacterBase::EGroundState::OnSteepGround;
        mOnGround = onGround;

        JPH::Vec3 newVelocity;
        if (onGround)
        {
            JPH::Vec3 groundVelocity = mCharacter->GetGroundVelocity();
            newVelocity = groundVelocity + desiredHorizontal;
            if (mInputBits & (unsigned char)InputState::Jump)
            {
                newVelocity += JPH::Vec3(0, mJumpSpeed, 0);
            }
        }
        else
        {
            newVelocity = JPH::Vec3(currentVelocity.GetX(), currentVelocity.GetY(), currentVelocity.GetZ());
            newVelocity.SetX(desiredHorizontal.GetX());
            newVelocity.SetZ(desiredHorizontal.GetZ());
            newVelocity += gravity * deltaTime;
        }

        mCharacter->SetLinearVelocity(newVelocity);

        // Store velocity as glm for camera use.
        mVelocity = glm::vec3(newVelocity.GetX(), newVelocity.GetY(), newVelocity.GetZ());
        mHorizontalSpeed = sqrtf(newVelocity.GetX() * newVelocity.GetX() +
                                 newVelocity.GetZ() * newVelocity.GetZ());

        JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
        updateSettings.mStickToFloorStepDown = JPH::Vec3(0, -0.5f, 0);
        updateSettings.mWalkStairsStepUp     = JPH::Vec3(0, 0.4f, 0);
        updateSettings.mWalkStairsMinStepForward = 0.02f;
        updateSettings.mWalkStairsStepForwardTest = 0.15f;

        mCharacter->ExtendedUpdate(
            deltaTime,
            gravity,
            updateSettings,
            JPH::BroadPhaseLayerFilter(), JPH::ObjectLayerFilter(),
            JPH::BodyFilter(), JPH::ShapeFilter(),
            scene.GetPhysics().GetTempAllocator());

        // ExtendedUpdate resolves the contacts for this frame. Use its result
        // for animation selection instead of the stale pre-update state.
        const JPH::CharacterBase::EGroundState resolvedGroundState = mCharacter->GetGroundState();
        mOnGround = resolvedGroundState == JPH::CharacterBase::EGroundState::OnGround ||
                    resolvedGroundState == JPH::CharacterBase::EGroundState::OnSteepGround;

        JPH::RVec3 pos = mCharacter->GetPosition();
        mPosition = glm::vec3((float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ());
    }

    void PlayerObject::Update(Scene &scene, float deltaTime)
    {
        mHitFlash = std::max(0.0f, mHitFlash - deltaTime);
        if (IsDead())
        {
            mVelocity = glm::vec3(0.0f);
            mPlayerModel.PlayFirstAnimationContaining("death", false, 1.0f);
            mPlayerModel.Update(deltaTime);
            if (mOutfitModel.IsLoaded())
                mOutfitModel.SynchronizeAnimationFrom(mPlayerModel);
            return;
        }
        UpdateMovement(scene, deltaTime);

        mAttackCooldown = std::max(0.0f, mAttackCooldown - deltaTime);
        if (mAttackTime > 0.0f) mAttackTime -= deltaTime;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mAttackCooldown <= 0.0f)
        {
            mAttackCooldown = 0.46f;
            mAttackTime = 0.52f;
            mAttackClipFound = mPlayerModel.PlayFirstAnimationContaining("attack", false, 1.15f);
            if (!mAttackClipFound)
                mAttackClipFound = mPlayerModel.PlayFirstAnimationContaining("slash", false, 1.15f);
            if (!mAttackClipFound)
                mAttackClipFound = mPlayerModel.PlayFirstAnimationContaining("melee", false, 1.15f);
            if (!mAttackClipFound)
                mAttackClipFound = mPlayerModel.PlayFirstAnimationContaining("sword", false, 1.15f);
            if (!mAttackClipFound)
                mPlayerModel.PlayAnimation(JumpAnimation, false, 1.45f);
        }

        if (mAttackTime <= 0.0f)
            PlayLocomotionAnimation();
        mPlayerModel.Update(deltaTime);
        if (mOutfitModel.IsLoaded())
            mOutfitModel.SynchronizeAnimationFrom(mPlayerModel);
    }

    void PlayerObject::PlayLocomotionAnimation()
    {
        if (!mOnGround)
            mPlayerModel.PlayAnimation(JumpAnimation, true, 1.0f);
        else if (mHorizontalSpeed > 0.2f)
        {
            if (mMovingBackward)
            {
                const float backwardPlayback = mIsRunning ? 1.65f : 0.9f;
                if (!mPlayerModel.PlayFirstAnimationContaining("backward", true, backwardPlayback))
                    mPlayerModel.PlayAnimation(WalkAnimation, true, -backwardPlayback);
            }
            else
                mPlayerModel.PlayAnimation(mIsRunning ? RunAnimation : WalkAnimation, true, 1.0f);
        }
        else
            mPlayerModel.PlayAnimation(IdleAnimation, true, 1.0f);
    }

    void PlayerObject::Draw(Scene &scene)
    {
        float groundY = mPosition.y - mCapsuleHeight * 0.5f;
        if (mPlayerModel.IsLoaded())
        {
            Color selected = ColorFromHSV(mSkinHue * 360.0f, mSkinSaturation, 0.90f);
            Vector3 skinColor = { selected.r / 255.0f, selected.g / 255.0f, selected.b / 255.0f };
            // O player usa numeros e particulas como feedback de dano; preserve
            // suas cores naturais em vez de tingir pele e roupa de vermelho.
            const Color damageTint = WHITE;
            const Vector3 footPosition = { mPosition.x, groundY, mPosition.z };
            if (mOutfitModel.IsLoaded())
            {
                // Outfit bounds exclude the head, so it must use the base model's
                // scale rather than being stretched independently to player height.
                const float baseScale = mVisualHeight / mPlayerModel.GetSourceHeight();
                mOutfitModel.Draw(footPosition, mFacingYawDegrees,
                                  mOutfitModel.GetSourceHeight() * baseScale, damageTint,
                                  scene.GetCamera().position, skinColor, mLightIntensity);
                Vector3 headLocal = {};
                float headCutoff = groundY + mVisualHeight * 0.84f;
                if (mPlayerModel.GetBoneLocalPosition("head", headLocal))
                    headCutoff = groundY + headLocal.y * baseScale - 0.04f;
                // Female FBXs provide separate face meshes. The male head belongs
                // to the body mesh, so it uses a bone-relative height clip instead.
                mPlayerModel.Draw(footPosition, mFacingYawDegrees, mVisualHeight, damageTint,
                                  scene.GetCamera().position, skinColor, mLightIntensity,
                                  true, headCutoff,
                                  mCharacterGender == CharacterGender::Female);
            }
            else
            {
                mPlayerModel.Draw(footPosition, mFacingYawDegrees, mVisualHeight, damageTint,
                                  scene.GetCamera().position, skinColor, mLightIntensity);
            }

            if (!mEquippedWeapon.IsEmpty())
            {
                Vector3 handLocal = {};
                bool handFound = mPlayerModel.GetBoneLocalPosition("hand_r", handLocal) ||
                                 mPlayerModel.GetBoneLocalPosition("righthand", handLocal) ||
                                 mPlayerModel.GetBoneLocalPosition("right_hand", handLocal);
                Vector3 forearmLocal = {};
                const bool forearmFound = mPlayerModel.GetBoneLocalPosition("lowerarm_r", forearmLocal) ||
                                          mPlayerModel.GetBoneLocalPosition("rightforearm", forearmLocal);
                Vector3 hand = { mPosition.x + sinf(mFacingYawDegrees * DEG2RAD) * 0.34f,
                                 groundY + 1.02f,
                                 mPosition.z - cosf(mFacingYawDegrees * DEG2RAD) * 0.34f };
                // Same rotation as Draw(): model local space (Y-up) -> world space.
                const float cosYaw = cosf(mFacingYawDegrees * DEG2RAD);
                const float sinYaw = sinf(mFacingYawDegrees * DEG2RAD);
                auto localToWorld = [&](Vector3 local)
                {
                    const float scale = mVisualHeight / mPlayerModel.GetSourceHeight();
                    const float localX = local.x * scale;
                    const float localZ = local.z * scale;
                    return Vector3{ mPosition.x + cosYaw * localX - sinYaw * localZ,
                                    groundY + local.y * scale,
                                    mPosition.z + sinYaw * localX + cosYaw * localZ };
                };
                if (handFound) hand = localToWorld(handLocal);
                if (mWeaponModel.IsLoaded())
                {
                    Vector3 longAxis = { 0.0f, 1.0f, 0.0f };
                    Vector3 armOut = { 0.0f, -1.0f, 0.0f };
                    if (handFound && forearmFound)
                    {
                        // Point the weapon head away from the elbow so it leads
                        // the swing beyond the fist. Blend toward straight-up so
                        // the axe rests head-up at the character's side. The blend
                        // ramps down during the windup, stays low through the
                        // strike, and returns to the resting hold during recovery.
                        armOut = Vector3Normalize(Vector3Subtract(hand, localToWorld(forearmLocal)));
                        float upBlend = 0.85f;
                        if (mAttackTime > 0.0f)
                        {
                            const float attackDuration = 0.52f;
                            const float progress = 1.0f - mAttackTime / attackDuration;
                            const float rampIn = Clamp(progress * (1.0f / 0.3f), 0.0f, 1.0f);
                            const float rampOut = Clamp((1.0f - progress) * (1.0f / 0.3f), 0.0f, 1.0f);
                            const float swingBlend = Lerp(0.85f, 0.30f, rampIn);
                            upBlend = Lerp(swingBlend, 0.85f, rampOut);
                        }
                        longAxis = Vector3Normalize(Vector3Lerp(armOut, { 0.0f, 1.0f, 0.0f }, upBlend));
                    }
                    // Seat the pommel just past the fist so the handle passes
                    // through the palm instead of resting along the back of the
                    // hand, a short distance away from the actual grip.
                    hand = Vector3Add(hand, Vector3Scale(armOut, 0.10f));
                    Vector3 rollReference = Vector3Subtract(localToWorld({ 1.0f, 0.0f, 0.0f }),
                                                            { mPosition.x, mPosition.y, mPosition.z });
                    mWeaponModel.DrawAttached(hand, longAxis, rollReference, 0.92f, WHITE,
                                              scene.GetCamera().position, skinColor, mLightIntensity);
                }
            }
        }
        else
        {
            float cubeSize = mCapsuleRadius * 2.0f;
            Vector3 center = { mPosition.x, groundY + cubeSize * 0.5f, mPosition.z };
            DrawCube(center, cubeSize, cubeSize, cubeSize, RED);
            DrawCubeWires(center, cubeSize, cubeSize, cubeSize, MAROON);
        }
    }

    void PlayerObject::DrawDebug(Scene &scene)
    {
        Vector3 foot = { mPosition.x, mPosition.y - mCapsuleHeight*0.5f, mPosition.z };
        Vector3 head = { mPosition.x, mPosition.y + mCapsuleHeight*0.5f, mPosition.z };
        DrawLine3D(foot, head, GREEN);
        DrawSphere(foot, 0.05f, GREEN);
        DrawSphere(head, 0.05f, BLUE);
    }

    glm::vec3 PlayerObject::GetCameraTarget() const
    {
        return glm::vec3(mPosition.x, mPosition.y + mCameraEyeHeight - mCapsuleHeight*0.5f, mPosition.z);
    }

    bool PlayerObject::UseInventorySlot(int index)
    {
        if (index < 0 || index >= Inventory::SlotCount) return false;
        InventorySlot &slot = mInventory.GetSlot(index);
        if (!IsConsumable(slot.type)) return false;
        if (mHealth >= mMaxHealth) return false;

        mHealth = std::min(mMaxHealth, mHealth + 38.0f);
        if (--slot.count <= 0) slot.Clear();
        return true;
    }

    const InventorySlot &PlayerObject::GetEquipment(EquipmentSlot slot) const
    {
        switch (slot)
        {
        case EquipmentSlot::Weapon: return mEquippedWeapon;
        case EquipmentSlot::Armor: return mEquippedArmor;
        case EquipmentSlot::Potion: return mEquippedPotion;
        default: return mEquippedWeapon;
        }
    }

    bool PlayerObject::UseEquippedPotion()
    {
        if (mEquippedPotion.IsEmpty() || !IsConsumable(mEquippedPotion.type) || mHealth >= mMaxHealth)
            return false;
        mHealth = std::min(mMaxHealth, mHealth + 38.0f);
        if (--mEquippedPotion.count <= 0) mEquippedPotion.Clear();
        return true;
    }

    bool PlayerObject::MoveInventoryToEquipment(int inventoryIndex, EquipmentSlot equipmentSlot)
    {
        if (inventoryIndex < 0 || inventoryIndex >= Inventory::SlotCount) return false;
        InventorySlot &source = mInventory.GetSlot(inventoryIndex);
        if (source.IsEmpty() || !CanEquipInSlot(source.type, equipmentSlot)) return false;

        InventorySlot *target = nullptr;
        switch (equipmentSlot)
        {
        case EquipmentSlot::Weapon: target = &mEquippedWeapon; break;
        case EquipmentSlot::Armor: target = &mEquippedArmor; break;
        case EquipmentSlot::Potion: target = &mEquippedPotion; break;
        }
        if (!target) return false;
        std::swap(source, *target);
        if (equipmentSlot == EquipmentSlot::Armor) RefreshOutfitModel();
        if (equipmentSlot == EquipmentSlot::Weapon) RefreshWeaponModel();
        return true;
    }

    bool PlayerObject::MoveEquipmentToInventory(EquipmentSlot equipmentSlot, int inventoryIndex)
    {
        if (inventoryIndex < 0 || inventoryIndex >= Inventory::SlotCount) return false;
        InventorySlot *source = nullptr;
        switch (equipmentSlot)
        {
        case EquipmentSlot::Weapon: source = &mEquippedWeapon; break;
        case EquipmentSlot::Armor: source = &mEquippedArmor; break;
        case EquipmentSlot::Potion: source = &mEquippedPotion; break;
        }
        if (!source || source->IsEmpty()) return false;

        InventorySlot &target = mInventory.GetSlot(inventoryIndex);
        if (target.IsEmpty())
        {
            std::swap(*source, target);
        }
        else if (target.type == source->type)
        {
            target.count += source->count;
            source->Clear();
        }
        else return false;

        if (equipmentSlot == EquipmentSlot::Armor) RefreshOutfitModel();
        if (equipmentSlot == EquipmentSlot::Weapon) RefreshWeaponModel();
        return true;
    }

    bool PlayerObject::EquipInventorySlot(int index)
    {
        if (index < 0 || index >= Inventory::SlotCount) return false;
        const InventorySlot &slot = mInventory.GetSlot(index);
        if (IsWeapon(slot.type))
        {
            return MoveInventoryToEquipment(index, EquipmentSlot::Weapon);
        }
        if (IsArmor(slot.type))
        {
            return MoveInventoryToEquipment(index, EquipmentSlot::Armor);
        }
        return false;
    }

    bool PlayerObject::DropInventorySlot(int index, ItemType &type, int &count)
    {
        if (index < 0 || index >= Inventory::SlotCount) return false;
        InventorySlot &slot = mInventory.GetSlot(index);
        if (slot.IsEmpty()) return false;
        type = slot.type;
        count = slot.count;
        slot.Clear();
        return true;
    }

    bool PlayerObject::DropEquipmentSlot(EquipmentSlot equipmentSlot, ItemType &type, int &count)
    {
        InventorySlot *slot = nullptr;
        switch (equipmentSlot)
        {
        case EquipmentSlot::Weapon: slot = &mEquippedWeapon; break;
        case EquipmentSlot::Armor: slot = &mEquippedArmor; break;
        case EquipmentSlot::Potion: slot = &mEquippedPotion; break;
        }
        if (!slot || slot->IsEmpty()) return false;
        type = slot->type;
        count = slot->count;
        slot->Clear();
        if (equipmentSlot == EquipmentSlot::Armor) RefreshOutfitModel();
        if (equipmentSlot == EquipmentSlot::Weapon) RefreshWeaponModel();
        return true;
    }

    void PlayerObject::AddExperience(float amount)
    {
        mExperience += amount;
        while (mExperience >= mExperienceToNextLevel)
        {
            mExperience -= mExperienceToNextLevel;
            ++mLevel;
            mExperienceToNextLevel = 100.0f + (mLevel - 1) * 45.0f;
            mMaxHealth += 12.0f;
            mHealth = mMaxHealth;
        }
    }

    float PlayerObject::TakeDamage(float amount)
    {
        if (amount <= 0.0f || IsDead()) return 0.0f;
        const float armorReduction = mEquippedArmor.IsEmpty() ? 0.0f :
            (mEquippedArmor.type == ItemType::RangerArmor ? 0.18f : 0.12f);
        const float previousHealth = mHealth;
        mHealth = std::max(0.0f, mHealth - amount * (1.0f - armorReduction));
        mHitFlash = 0.18f;
        return previousHealth - mHealth;
    }

    void PlayerObject::Heal(float amount)
    {
        if (amount > 0.0f && !IsDead()) mHealth = std::min(mMaxHealth, mHealth + amount);
    }

    void PlayerObject::DevSetHealth(float value)
    {
        mHealth = std::clamp(value, 0.0f, mMaxHealth);
    }

    void PlayerObject::DevSetMaxHealth(float value)
    {
        mMaxHealth = std::max(1.0f, value);
        mHealth = std::min(mHealth, mMaxHealth);
    }

    void PlayerObject::DevSetLevel(int value)
    {
        mLevel = std::max(1, value);
    }

    void PlayerObject::DevSetExperience(float value)
    {
        mExperience = std::max(0.0f, value);
    }

    void PlayerObject::DevSetMoney(int value)
    {
        mMoney = std::max(0, value);
    }

    float PlayerObject::GetAttackDamage() const
    {
        return 14.0f + (float)mLevel * 2.0f + (mEquippedWeapon.IsEmpty() ? 0.0f : 12.0f);
    }
}
