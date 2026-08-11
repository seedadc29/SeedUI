#include "GameObjects/PickupObject.h"
#include "Scene.h"

#include "raylib.h"

#include <cmath>

namespace game
{
    PickupObject::PickupObject(const glm::vec3 &position, ItemType type, int count)
        : mType(type), mCount(count)
    {
        mPosition = position;
    }

    void PickupObject::OnSpawn(Scene &)
    {
        std::string modelPath = std::string("Assets/Items/") + GetItemModelFile(mType);
        if (!FileExists(modelPath.c_str()))
            modelPath = std::string("Game/Assets/UltimateRPg Items Pack/FBX/") + GetItemModelFile(mType);
        if (!mItemModel.Load(modelPath, std::string()))
            TraceLog(LOG_WARNING, "PICKUP: Could not load %s: %s", GetItemName(mType), mItemModel.GetLastError().c_str());
    }

    void PickupObject::Update(Scene &, float deltaTime)
    {
        mTime += deltaTime;
    }

    void PickupObject::Draw(Scene &scene)
    {
        if (mCollected) return;

        const float bob = 0.22f + sinf(mTime * 2.2f) * 0.09f;
        const Vector3 p = { mPosition.x, mPosition.y + bob, mPosition.z };
        const Color color = GetItemColor(mType);
        DrawCircle3D({ p.x, mPosition.y + 0.012f, p.z }, 0.35f, { 0.0f, 1.0f, 0.0f }, 90.0f,
                     ColorAlpha(color, 0.22f));

        if (mItemModel.IsLoaded())
        {
            const float size = mType == ItemType::IronAxe ? 0.78f :
                               mType == ItemType::HealthPotion ? 0.52f : 0.68f;
            const float drawHeight = mItemModel.GetSourceHeight() * size /
                                     mItemModel.GetSourceMaxDimension();
            mItemModel.Draw(p, mTime * 28.0f + mRotation.y, drawHeight, WHITE,
                            scene.GetCamera().position, { 0.72f, 0.48f, 0.31f }, 1.55f,
                            false, 0.0f, true,
                            {mEditorScale.x, mEditorScale.y, mEditorScale.z},
                            {mRotation.x, 0.0f, mRotation.z});
        }
        else
        {
            DrawSphere(p, 0.12f, color);
        }
    }
}
