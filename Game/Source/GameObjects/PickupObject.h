#ifndef PICKUPOBJECT_H
#define PICKUPOBJECT_H

#include "GameObject.h"
#include "Inventory.h"
#include "Assets/FbxModel.h"

namespace game
{
    class PickupObject : public GameObject
    {
    public:
        PickupObject(const glm::vec3 &position, ItemType type, int count = 1);

        void OnSpawn(Scene &scene) override;
        void Update(Scene &scene, float deltaTime) override;
        void Draw(Scene &scene) override;

        ItemType GetItemType() const { return mType; }
        int GetCount() const { return mCount; }
        bool IsCollected() const { return mCollected; }
        void Collect() { mCollected = true; }
        void SetEditorTransform(const glm::vec3 &position, const glm::vec3 &rotation,
                                const glm::vec3 &scale)
        { mPosition = position; mRotation = rotation; mEditorScale = scale; }

    private:
        ItemType mType = ItemType::None;
        int mCount = 1;
        float mTime = 0.0f;
        bool mCollected = false;
        FbxModel mItemModel;
        glm::vec3 mEditorScale = glm::vec3(1.0f);
    };
}

#endif // PICKUPOBJECT_H
