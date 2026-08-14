#ifndef NATUREOBJECT_H
#define NATUREOBJECT_H

#include "GameObject.h"
#include "raylib.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <string>
#include <vector>
#include <filesystem>
#include <cstddef>

namespace physics { class Physics; }

namespace game
{
    class TerrainObject;

    class NatureObject : public GameObject
    {
    public:
        explicit NatureObject(TerrainObject *terrain);
        ~NatureObject() override;

        void OnSpawn(Scene &scene) override;
        void Update(Scene &scene, float deltaTime) override;
        void Draw(Scene &scene) override;
        int GetAssetCount() const { return (int)mAssets.size(); }
        const char *GetAssetName(int index) const;
        Texture2D GetThumbnail(int index);
        int ImportFBX(const std::filesystem::path &sourcePath);
        unsigned int AddEditorInstance(Scene &scene, int assetIndex, const glm::vec3 &position,
                                       const glm::vec3 &scale = glm::vec3(1.0f),
                                       const glm::vec3 &rotation = glm::vec3(0.0f),
                                       bool collisionEnabled = true);
        bool TransformEditorInstance(unsigned int id, const glm::vec3 &position,
                                     const glm::vec3 &scale, const glm::vec3 &rotation);
        bool SetEditorCollision(unsigned int id, bool enabled);
        bool GetEditorCollision(unsigned int id) const;
        // size == (0,0,0) means auto-fit to the mesh bounds. offset is local-space
        // (rotated with the object) relative to the instance position.
        bool SetEditorCollisionBox(unsigned int id, const glm::vec3 &size,
                                   const glm::vec3 &offset = glm::vec3(0.0f));
        bool GetEditorCollisionBox(unsigned int id, glm::vec3 &size, glm::vec3 &offset) const;
        bool RemoveEditorInstance(unsigned int id);
        void ClearEditorInstances();
        bool RemoveNearestEditorInstance(const glm::vec3 &position, float radius);
        void DrawPreview(int assetIndex, const glm::vec3 &position,
                         const glm::vec3 &scale = glm::vec3(1.0f),
                         const glm::vec3 &rotation = glm::vec3(0.0f));

    private:
        struct Asset
        {
            std::string name;
            std::filesystem::path path;
            Model model = {};
            float sourceHeight = 1.0f;
            float desiredHeight = 1.0f;
            bool tree = false;
            bool imported = false;
            bool loaded = false;
            RenderTexture2D thumbnail = {};
            // Unique textures owned by imported FBX assets (UnloadModel does
            // not release textures, so the owner must unload them explicitly).
            std::vector<Texture2D> importedTextures;
        };

        struct Instance
        {
            int assetIndex = 0;
            glm::vec3 position = glm::vec3(0.0f);
            float yaw = 0.0f;
            float scale = 1.0f;
            glm::vec3 editorScale = glm::vec3(1.0f);
            glm::vec3 editorRotation = glm::vec3(0.0f);
            bool editorPlaced = false;
            unsigned int editorId = 0;
            bool collisionEnabled = true;
            bool collisionBoxCustom = false;
            glm::vec3 collisionBoxSize = glm::vec3(0.0f);
            glm::vec3 collisionBoxOffset = glm::vec3(0.0f);
            JPH::BodyID bodyId;
        };

        void LoadAsset(Asset &asset);
        void UnloadAssetModel(Asset &asset);
        void LoadImportedAssets();
        void RebuildEditorCollision(Instance &instance);
        void AddInstance(Scene &scene, int assetIndex, float x, float z, float scale, float yaw);

        TerrainObject *mTerrain = nullptr;
        physics::Physics *mPhysics = nullptr;
        std::vector<Asset> mAssets;
        std::vector<Instance> mInstances;
        std::size_t mStreamingAssetCursor = 0;
        double mNextStreamingLoadTime = 0.0;
        unsigned int mNextEditorId = 1;
    };
}

#endif
