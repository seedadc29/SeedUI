#ifndef TERRAINOBJECT_H
#define TERRAINOBJECT_H

#include "GameObject.h"
#include "raylib.h"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <vector>

namespace game
{
    class TerrainObject : public GameObject
    {
    public:
        TerrainObject(int tilesX = 128, int tilesZ = 128, float tileSize = 2.0f);
        ~TerrainObject() override;

        void OnSpawn(Scene &scene) override;
        void Draw(Scene &scene) override;

        float GetHeightAt(float x, float z) const;
        void Terraform(Scene &scene, float x, float z, float radius, float amount);
        void PaintTexture(float x, float z, float radius, float strength, int layer);
        void EraseTexture(float x, float z, float radius, float strength);
        const std::vector<float> &GetHeights() const { return mHeights; }
        void SetHeights(Scene &scene, const std::vector<float> &heights);
        const std::vector<Color> &GetStencilMap() const { return mStencilPixels; }
        void SetStencilMap(const std::vector<Color> &pixels);
        const float *GetUVScales() const { return mUVScales; }
        void SetUVScales(const float scales[4]);
        Texture2D GetLayerTexture(int layer) const;
        const Mesh &GetMesh() const { return mRayMesh; }
        Material &GetMaterial() { return mMaterial; }
        const Material &GetMaterial() const { return mMaterial; }
        int GetTilesX() const { return mTilesX; }
        int GetTilesZ() const { return mTilesZ; }

    private:
        void GenerateHeightmap();
        void BuildMesh();
        void UploadMeshToJolt(Scene &scene);
        void Rebuild(Scene &scene);
        void InitTerrainMaterial();
        void UploadStencil();

        int   mTilesX;
        int   mTilesZ;
        float mTileSize;
        float mHeightScale = 3.5f;

        std::vector<float>       mHeights;
        std::vector<Vector3>     mVertices;
        std::vector<unsigned int> mIndices;
        std::vector<Vector3>     mNormals;

        Mesh       mRayMesh = {};
        Material   mMaterial = {};
        Shader     mTerrainShader = {};
        Texture2D  mLayerTextures[4] = {};
        Texture2D  mStencilTexture = {};
        std::vector<Color> mStencilPixels;
        float mUVScales[4] = { 18.0f, 18.0f, 18.0f, 18.0f };
        int mStencilWidth = 0;
        int mStencilHeight = 0;
        bool       mMeshLoaded = false;
        JPH::BodyID mBodyID = JPH::BodyID(-1);
    };
}

#endif // TERRAINOBJECT_H
