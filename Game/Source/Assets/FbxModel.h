#ifndef FBXMODEL_H
#define FBXMODEL_H

#include "raylib.h"

#include <string>
#include <vector>

namespace game
{
    // Loads a static (non-animated) FBX file into a raylib Model using assimp.
    // Geometry, normals, UVs and materials are baked into the Model; textures
    // are resolved relative to the FBX location and returned in outTextures so
    // the caller owns their lifetime. Returns false and fills error on failure.
    bool LoadStaticFbxModel(const std::string &modelPath, Model &outModel,
                            std::vector<Texture2D> &outTextures, std::string &error);

    class FbxModel
    {
    public:
        FbxModel();
        ~FbxModel();

        bool Load(const std::string &modelPath, const std::string &animationPath);
        void Unload();

        void PlayAnimation(const char *animationName, bool loop, float playbackSpeed = 1.0f);
        bool PlayFirstAnimationContaining(const char *fragment, bool loop,
                                          float playbackSpeed = 1.0f);
        void SynchronizeAnimationFrom(const FbxModel &source);
        void Update(float deltaTime);
        // Position relative to the model's feet, in its animated local space.
        bool GetBoneLocalPosition(const char *nameFragment, Vector3 &position) const;
        void Draw(Vector3 footPosition, float yawDegrees, float targetHeight, Color tint,
                  Vector3 cameraPosition, Vector3 skinColor, float lightIntensity,
                  bool headOnly = false, float headCutoffWorldY = 0.0f,
                  bool preferNamedHeadMeshes = true,
                  Vector3 extraScale = {1.0f, 1.0f, 1.0f},
                  Vector3 extraRotationDegrees = {0.0f, 0.0f, 0.0f}) const;
        void DrawAttached(Vector3 anchorWorld, Vector3 longAxisWorld, Vector3 rollReferenceWorld,
                          float targetMaxDimension, Color tint, Vector3 cameraPosition,
                          Vector3 skinColor, float lightIntensity) const;

        bool IsLoaded() const;
        float GetSourceHeight() const;
        float GetSourceMaxDimension() const;
        const std::string &GetLastError() const { return mLastError; }

    private:
        struct Impl;
        Impl *mImpl = nullptr;
        std::string mLastError;
    };
}

#endif // FBXMODEL_H
