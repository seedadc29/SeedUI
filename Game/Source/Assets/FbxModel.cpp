#include "Assets/FbxModel.h"
#include "Assets/TextureLoader.h"
#include "raymath.h"
#include "rlgl.h"

#include <assimp/anim.h>
#include <assimp/cimport.h>
#include <assimp/material.h>
#include <assimp/matrix3x3.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <unordered_map>
#include <vector>

namespace game
{
    namespace
    {
        constexpr const char *CharacterVertexShader = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    vec4 worldPosition = matModel * vec4(vertexPosition, 1.0);
    fragPosition = worldPosition.xyz;
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";

        constexpr const char *CharacterFragmentShader = R"GLSL(
#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform int isSkinMaterial;
uniform vec3 skinToneColor;
uniform float lightIntensity;
uniform float materialRoughness;
uniform int clipBelowEnabled;
uniform float clipBelowWorldY;

out vec4 finalColor;

void main()
{
    if (clipBelowEnabled == 1 && fragPosition.y < clipBelowWorldY) discard;
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec3 normal = normalize(fragNormal);
    vec3 viewDirection = normalize(viewPos - fragPosition);
    // Keep the key light above and close to the camera so the visible side of
    // the character always receives light instead of becoming a silhouette.
    vec3 lightDirection = normalize(viewDirection + vec3(-0.28, 0.48, 0.08));
    vec3 halfDirection = normalize(lightDirection + viewDirection);

    vec3 baseColor = albedo.rgb;
    if (isSkinMaterial == 1)
    {
        float warmChroma = baseColor.r - baseColor.b;
        float skinMask = smoothstep(0.07, 0.18, warmChroma) *
                         smoothstep(0.12, 0.28, baseColor.r);
        float sourceDetail = clamp(dot(baseColor, vec3(0.299, 0.587, 0.114)) /
                                   0.46, 0.68, 1.28);
        vec3 recoloredSkin = skinToneColor * sourceDetail;
        baseColor = mix(baseColor, recoloredSkin, skinMask * 0.92);
    }

    // Textures are authored in sRGB. Lighting them in linear space prevents
    // midtones from becoming unnaturally dark.
    vec3 albedoLinear = pow(max(baseColor, vec3(0.0)), vec3(2.2));

    float direct = max(dot(normal, lightDirection), 0.0);
    float wrappedDiffuse = direct * 0.88 + 0.12;
    float skyAmbient = normal.y * 0.5 + 0.5;
    float backFill = max(dot(normal, normalize(vec3(0.55, 0.25, 0.72))), 0.0);
    float specularPower = mix(54.0, 12.0, materialRoughness);
    float specular = pow(max(dot(normal, halfDirection), 0.0), specularPower) *
                     direct * (1.0 - materialRoughness);
    float rim = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);

    // Exposicao/ambiente mais alta: os termos abaixo so multiplicam o albedo
    // (mesmas cores, apenas mais luz), recuperando detalhes nas areas escuras.
    vec3 warmSun = vec3(1.00, 0.95, 0.87) * wrappedDiffuse * 1.10;
    vec3 coolSky = vec3(0.32, 0.40, 0.54) * (0.60 + skyAmbient * 0.32);
    vec3 fillLight = vec3(0.22, 0.28, 0.38) * backFill * 0.46;
    vec3 highlights = vec3(1.00, 0.92, 0.78) * specular * 0.26;
    vec3 rimLight = vec3(0.42, 0.52, 0.68) * rim * 0.28;

    vec3 directLighting = warmSun + fillLight;
    vec3 litLinear = albedoLinear * (coolSky + directLighting * lightIntensity) +
                     (highlights + rimLight) * lightIntensity;
    vec3 litColor = pow(max(litLinear, vec3(0.0)), vec3(1.0/2.2));
    finalColor = vec4(litColor, albedo.a);
}
)GLSL";

        struct VertexInfluence
        {
            int boneIndices[4] = { -1, -1, -1, -1 };
            float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        };

        struct MeshSkinData
        {
            std::vector<aiVector3D> vertices;
            std::vector<aiVector3D> normals;
            std::vector<VertexInfluence> influences;
            std::unordered_map<int, aiMatrix4x4> boneOffsets;
        };

        struct BoneData
        {
            int index = -1;
        };

        static const aiNodeAnim *FindNodeChannel(const aiAnimation *animation, const aiString &nodeName)
        {
            for (unsigned int i = 0; i < animation->mNumChannels; ++i)
            {
                if (animation->mChannels[i]->mNodeName == nodeName)
                    return animation->mChannels[i];
            }
            return nullptr;
        }

        static void CollectBindRotations(const aiNode *node,
                                         std::unordered_map<std::string, aiQuaternion> &rotations)
        {
            aiVector3D scale;
            aiQuaternion rotation;
            aiVector3D position;
            node->mTransformation.Decompose(scale, rotation, position);
            rotations[node->mName.C_Str()] = rotation;

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                CollectBindRotations(node->mChildren[i], rotations);
        }

        static aiQuaternion InterpolateRotation(double time, const aiQuatKey *keys, unsigned int keyCount)
        {
            if (keyCount == 0) return aiQuaternion();
            if (keyCount == 1 || time <= keys[0].mTime) return keys[0].mValue;

            unsigned int index = 0;
            while (index + 1 < keyCount && time >= keys[index + 1].mTime) ++index;
            if (index + 1 >= keyCount) return keys[keyCount - 1].mValue;

            const double span = keys[index + 1].mTime - keys[index].mTime;
            const float factor = span > 0.0 ? (float)((time - keys[index].mTime) / span) : 0.0f;
            aiQuaternion result;
            aiQuaternion::Interpolate(result, keys[index].mValue, keys[index + 1].mValue, factor);
            result.Normalize();
            return result;
        }

        static aiVector3D InterpolateVector(double time, const aiVectorKey *keys,
                                            unsigned int keyCount, const aiVector3D &fallback)
        {
            if (keyCount == 0) return fallback;
            if (keyCount == 1 || time <= keys[0].mTime) return keys[0].mValue;

            unsigned int index = 0;
            while (index + 1 < keyCount && time >= keys[index + 1].mTime) ++index;
            if (index + 1 >= keyCount) return keys[keyCount - 1].mValue;

            const double span = keys[index + 1].mTime - keys[index].mTime;
            const float factor = span > 0.0 ? (float)((time - keys[index].mTime) / span) : 0.0f;
            return keys[index].mValue + (keys[index + 1].mValue - keys[index].mValue) * factor;
        }

        static std::filesystem::path ResolveTexturePath(const std::filesystem::path &modelPath,
                                                        const aiString &assimpPath)
        {
            std::string raw = assimpPath.C_Str();
            std::replace(raw.begin(), raw.end(), '\\', '/');
            const std::filesystem::path texturePath(raw);
            const std::filesystem::path modelDirectory = modelPath.parent_path();
            const std::filesystem::path fileName = texturePath.filename();
            const std::filesystem::path candidates[] = {
                modelDirectory / texturePath,
                modelDirectory / fileName,
                modelDirectory / "Textures" / fileName,
                modelDirectory.parent_path() / "Textures" / fileName
            };

            for (const std::filesystem::path &candidate : candidates)
            {
                if (std::filesystem::exists(candidate)) return candidate;
                // Cooked builds replace PNG textures with .qoi siblings.
                std::filesystem::path qoi = candidate;
                qoi.replace_extension(".qoi");
                if (std::filesystem::exists(qoi)) return qoi;
            }
            return {};
        }
    }

    // Cache estatico dos dados CSM da cena. Cada FbxModel tem o proprio
    // programa de shader, entao os uniforms sao aplicados por modelo no momento
    struct FbxModel::Impl
    {
        const aiScene *modelScene = nullptr;
        const aiScene *animationScene = nullptr;
        Model model = {};
        std::vector<MeshSkinData> meshSkinData;
        std::vector<aiMatrix4x4> meshTransforms;
        std::vector<std::string> meshNames;
        std::unordered_map<std::string, BoneData> bones;
        std::unordered_map<std::string, aiMatrix4x4> nodeGlobalTransforms;
        std::unordered_map<std::string, aiQuaternion> animationBindRotations;
        std::vector<aiMatrix4x4> boneTransforms;
        std::vector<Texture2D> textures;
        std::unordered_map<std::string, Texture2D> textureCache;
        Shader lightingShader = {};
        int viewPositionLocation = -1;
        int skinMaterialLocation = -1;
        int skinToneColorLocation = -1;
        int lightIntensityLocation = -1;
        int materialRoughnessLocation = -1;
        int clipBelowEnabledLocation = -1;
        int clipBelowWorldYLocation = -1;
        int skinMaterialIndex = -1;
        const aiAnimation *currentAnimation = nullptr;
        std::string currentAnimationName;
        float animationSeconds = 0.0f;
        float playbackSpeed = 1.0f;
        bool loop = true;
        bool loaded = false;
        bool nativeAnimation = false;
        bool raylibAnimation = false;
        ModelAnimation *raylibAnimations = nullptr;
        int raylibAnimationCount = 0;
        int raylibCurrentAnimation = -1;
        float raylibAnimationFrame = 0.0f;
        float sourceMinY = 0.0f;
        float sourceHeight = 1.0f;
        float sourceMaxDimension = 1.0f;
        Vector3 sourceMin = {};
        Vector3 sourceMax = {};
        aiMatrix4x4 globalInverseTransform;
        aiMatrix4x4 coordinateTransform;
        aiMatrix3x3 coordinateNormalTransform;

        void CollectMeshTransforms(const aiNode *node, const aiMatrix4x4 &parentTransform)
        {
            const aiMatrix4x4 globalTransform = parentTransform * node->mTransformation;
            const aiMatrix4x4 rootRelativeTransform = globalInverseTransform * globalTransform;
            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                const unsigned int meshIndex = node->mMeshes[i];
                if (meshIndex < meshTransforms.size())
                    meshTransforms[meshIndex] = rootRelativeTransform;
            }
            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                CollectMeshTransforms(node->mChildren[i], globalTransform);
        }

        const aiAnimation *FindAnimation(const char *requestedName) const
        {
            if (!animationScene || !requestedName) return nullptr;

            for (unsigned int i = 0; i < animationScene->mNumAnimations; ++i)
            {
                const aiAnimation *animation = animationScene->mAnimations[i];
                const std::string fullName = animation->mName.C_Str();
                if (fullName == requestedName) return animation;

                const size_t separator = fullName.find_last_of('|');
                if (separator != std::string::npos && fullName.substr(separator + 1) == requestedName)
                    return animation;
            }
            return nullptr;
        }

        void ReadNodeHierarchy(double animationTime, const aiNode *node,
                               const aiMatrix4x4 &parentTransform)
        {
            aiMatrix4x4 nodeTransform = node->mTransformation;
            const aiNodeAnim *channel = FindNodeChannel(currentAnimation, node->mName);

            if (channel)
            {
                aiVector3D bindScale;
                aiQuaternion bindRotation;
                aiVector3D bindPosition;
                node->mTransformation.Decompose(bindScale, bindRotation, bindPosition);

                aiQuaternion rotation = channel->mNumRotationKeys > 0
                    ? InterpolateRotation(animationTime, channel->mRotationKeys, channel->mNumRotationKeys)
                    : bindRotation;

                if (nativeAnimation)
                {
                    // Animacoes embutidas no mesmo FBX pertencem exatamente a este
                    // esqueleto. Preservar os canais completos evita separar e esticar
                    // partes de monstros que animam controllers por translacao/escala.
                    const aiVector3D position = InterpolateVector(
                        animationTime, channel->mPositionKeys, channel->mNumPositionKeys, bindPosition);
                    const aiVector3D scale = InterpolateVector(
                        animationTime, channel->mScalingKeys, channel->mNumScalingKeys, bindScale);
                    nodeTransform = aiMatrix4x4(scale, rotation, position);
                }
                else
                {
                    auto sourceBind = animationBindRotations.find(node->mName.C_Str());
                    if (sourceBind != animationBindRotations.end())
                    {
                        aiQuaternion inverseSourceBind = sourceBind->second;
                        inverseSourceBind.Conjugate();
                        rotation = bindRotation * inverseSourceBind * rotation;
                        rotation.Normalize();
                    }

                    // Retarget entre esqueletos compativeis: transfere rotacao,
                    // mantendo comprimento e escala do personagem que recebe o clip.
                    nodeTransform = aiMatrix4x4(bindScale, rotation, bindPosition);
                }
            }

            const aiMatrix4x4 globalTransform = parentTransform * nodeTransform;
            nodeGlobalTransforms[node->mName.C_Str()] = globalTransform;
            auto boneIt = bones.find(node->mName.C_Str());
            if (boneIt != bones.end())
            {
                boneTransforms[boneIt->second.index] =
                    globalInverseTransform * globalTransform;
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                ReadNodeHierarchy(animationTime, node->mChildren[i], globalTransform);
        }

        Texture2D LoadCachedTexture(const std::filesystem::path &texturePath)
        {
            if (texturePath.empty()) return {};
            const std::string key = texturePath.lexically_normal().string();
            auto found = textureCache.find(key);
            if (found != textureCache.end()) return found->second;

            Texture2D texture = TextureAcquire(key);
            if (texture.id != 0)
            {
                GenTextureMipmaps(&texture);
                SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_8X);
                textureCache[key] = texture;
                textures.push_back(texture);
            }
            return texture;
        }

        void ApplyMaterialTextures(const std::filesystem::path &modelPath)
        {
            for (unsigned int i = 0; i < modelScene->mNumMaterials; ++i)
            {
                aiMaterial *source = modelScene->mMaterials[i];
                Material &destination = model.materials[i];

                aiColor4D color;
                if (aiGetMaterialColor(source, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
                {
                    destination.maps[MATERIAL_MAP_ALBEDO].color = {
                        (unsigned char)(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
                        (unsigned char)(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
                        (unsigned char)(std::clamp(color.b, 0.0f, 1.0f) * 255.0f),
                        (unsigned char)(std::clamp(color.a, 0.0f, 1.0f) * 255.0f)
                    };
                }

                aiString textureName;
                if (source->GetTexture(aiTextureType_DIFFUSE, 0, &textureName) == AI_SUCCESS)
                {
                    std::filesystem::path texturePath = ResolveTexturePath(modelPath, textureName);
                    const std::string fileName = texturePath.filename().string();
                    if (fileName.find("T_Superhero_") == 0)
                        skinMaterialIndex = (int)i;

                    Texture2D texture = LoadCachedTexture(texturePath);
                    if (texture.id != 0)
                    {
                        destination.maps[MATERIAL_MAP_ALBEDO].texture = texture;
                        // Base-color textures already contain their intended colors. Applying the
                        // FBX diffuse tint a second time makes skin and hair unnecessarily dark.
                        destination.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
                    }
                }

                if (source->GetTexture(aiTextureType_NORMALS, 0, &textureName) == AI_SUCCESS)
                {
                    Texture2D texture = LoadCachedTexture(ResolveTexturePath(modelPath, textureName));
                    if (texture.id != 0) destination.maps[MATERIAL_MAP_NORMAL].texture = texture;
                }
            }
        }

        void ApplyLightingShader()
        {
            lightingShader = LoadShaderFromMemory(CharacterVertexShader, CharacterFragmentShader);
            if (lightingShader.id == 0) return;

            viewPositionLocation = GetShaderLocation(lightingShader, "viewPos");
            skinMaterialLocation = GetShaderLocation(lightingShader, "isSkinMaterial");
            skinToneColorLocation = GetShaderLocation(lightingShader, "skinToneColor");
            lightIntensityLocation = GetShaderLocation(lightingShader, "lightIntensity");
            materialRoughnessLocation = GetShaderLocation(lightingShader, "materialRoughness");
            clipBelowEnabledLocation = GetShaderLocation(lightingShader, "clipBelowEnabled");
            clipBelowWorldYLocation = GetShaderLocation(lightingShader, "clipBelowWorldY");
            lightingShader.locs[SHADER_LOC_VECTOR_VIEW] = viewPositionLocation;
            for (int i = 0; i < model.materialCount; ++i)
                model.materials[i].shader = lightingShader;
        }

        void DrawMeshes(Matrix transform, Color tint, Vector3 cameraPosition,
                        Vector3 skinColor, float lightIntensity,
                        bool headOnly, float headCutoffWorldY,
                        bool preferNamedHeadMeshes) const
        {
            if (lightingShader.id != 0 && viewPositionLocation >= 0)
                SetShaderValue(lightingShader, viewPositionLocation,
                               &cameraPosition, SHADER_UNIFORM_VEC3);
            if (skinToneColorLocation >= 0)
                SetShaderValue(lightingShader, skinToneColorLocation,
                               &skinColor, SHADER_UNIFORM_VEC3);
            if (lightIntensityLocation >= 0)
                SetShaderValue(lightingShader, lightIntensityLocation,
                               &lightIntensity, SHADER_UNIFORM_FLOAT);
            if (materialRoughnessLocation >= 0)
            {
                const float roughness = raylibAnimation ? 0.88f : 0.74f;
                SetShaderValue(lightingShader, materialRoughnessLocation,
                               &roughness, SHADER_UNIFORM_FLOAT);
            }

            bool hasFaceMeshes = false;
            if (headOnly && preferNamedHeadMeshes)
            {
                for (const std::string &name : meshNames)
                {
                    if (name.find("face") != std::string::npos)
                    {
                        hasFaceMeshes = true;
                        break;
                    }
                }
            }
            const int clipBelowEnabled = headOnly && !hasFaceMeshes ? 1 : 0;
            if (clipBelowEnabledLocation >= 0)
                SetShaderValue(lightingShader, clipBelowEnabledLocation,
                               &clipBelowEnabled, SHADER_UNIFORM_INT);
            if (clipBelowWorldYLocation >= 0)
                SetShaderValue(lightingShader, clipBelowWorldYLocation,
                               &headCutoffWorldY, SHADER_UNIFORM_FLOAT);

            for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
            {
                if (headOnly && hasFaceMeshes &&
                    meshNames[meshIndex].find("face") == std::string::npos)
                    continue;

                const int materialIndex = model.meshMaterial[meshIndex];
                Material material = model.materials[materialIndex];
                Color color = material.maps[MATERIAL_MAP_ALBEDO].color;
                material.maps[MATERIAL_MAP_ALBEDO].color = {
                    (unsigned char)((int)color.r * tint.r / 255),
                    (unsigned char)((int)color.g * tint.g / 255),
                    (unsigned char)((int)color.b * tint.b / 255),
                    (unsigned char)((int)color.a * tint.a / 255)
                };

                const int isSkin = materialIndex == skinMaterialIndex ? 1 : 0;
                if (skinMaterialLocation >= 0)
                    SetShaderValue(lightingShader, skinMaterialLocation,
                                   &isSkin, SHADER_UNIFORM_INT);
                DrawMesh(model.meshes[meshIndex], material, transform);
            }
        }

        bool BuildModel(const std::filesystem::path &modelPath, std::string &error)
        {
            model.meshCount = (int)modelScene->mNumMeshes;
            model.materialCount = std::max(1, (int)modelScene->mNumMaterials);
            model.meshes = (Mesh *)MemAlloc(sizeof(Mesh) * model.meshCount);
            model.materials = (Material *)MemAlloc(sizeof(Material) * model.materialCount);
            model.meshMaterial = (int *)MemAlloc(sizeof(int) * model.meshCount);
            std::memset(model.meshes, 0, sizeof(Mesh) * model.meshCount);
            std::memset(model.materials, 0, sizeof(Material) * model.materialCount);
            model.transform = MatrixIdentity();
            meshSkinData.resize(model.meshCount);
            meshTransforms.resize(model.meshCount, aiMatrix4x4());
            meshNames.resize(model.meshCount);
            CollectMeshTransforms(modelScene->mRootNode, aiMatrix4x4());

            for (int materialIndex = 0; materialIndex < model.materialCount; ++materialIndex)
                model.materials[materialIndex] = LoadMaterialDefault();

            float minY = std::numeric_limits<float>::max();
            float maxY = std::numeric_limits<float>::lowest();
            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            float minZ = std::numeric_limits<float>::max();
            float maxZ = std::numeric_limits<float>::lowest();

            for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
            {
                const aiMesh *source = modelScene->mMeshes[meshIndex];
                meshNames[meshIndex] = source->mName.C_Str();
                std::transform(meshNames[meshIndex].begin(), meshNames[meshIndex].end(),
                               meshNames[meshIndex].begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
                if (source->mNumVertices > 65535)
                {
                    error = "FBX mesh exceeds the 16-bit raylib index limit";
                    return false;
                }

                Mesh &mesh = model.meshes[meshIndex];
                MeshSkinData &skin = meshSkinData[meshIndex];
                mesh.vertexCount = (int)source->mNumVertices;
                mesh.triangleCount = (int)source->mNumFaces;
                mesh.vertices = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);
                mesh.normals = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);
                mesh.texcoords = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 2);
                mesh.indices = (unsigned short *)MemAlloc(sizeof(unsigned short) * mesh.triangleCount * 3);
                mesh.animVertices = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);
                mesh.animNormals = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);

                skin.vertices.resize(mesh.vertexCount);
                skin.normals.resize(mesh.vertexCount);
                skin.influences.resize(mesh.vertexCount);

                for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex)
                {
                    const aiVector3D vertex = source->mVertices[vertexIndex];
                    const aiVector3D normal = source->HasNormals()
                        ? source->mNormals[vertexIndex] : aiVector3D(0.0f, 1.0f, 0.0f);
                    const aiMatrix4x4 &meshTransform = meshTransforms[meshIndex];
                    const aiVector3D displayVertex = coordinateTransform * meshTransform * vertex;
                    aiMatrix3x3 meshNormalTransform(meshTransform);
                    meshNormalTransform.Inverse().Transpose();
                    aiVector3D displayNormal = coordinateNormalTransform * meshNormalTransform * normal;
                    displayNormal.Normalize();
                    skin.vertices[vertexIndex] = vertex;
                    skin.normals[vertexIndex] = normal;
                    minY = std::min(minY, displayVertex.y);
                    maxY = std::max(maxY, displayVertex.y);
                    minX = std::min(minX, displayVertex.x);
                    maxX = std::max(maxX, displayVertex.x);
                    minZ = std::min(minZ, displayVertex.z);
                    maxZ = std::max(maxZ, displayVertex.z);

                    for (int component = 0; component < 3; ++component)
                    {
                        mesh.vertices[vertexIndex * 3 + component] = displayVertex[component];
                        mesh.normals[vertexIndex * 3 + component] = displayNormal[component];
                        mesh.animVertices[vertexIndex * 3 + component] = displayVertex[component];
                        mesh.animNormals[vertexIndex * 3 + component] = displayNormal[component];
                    }

                    if (source->HasTextureCoords(0))
                    {
                        mesh.texcoords[vertexIndex * 2] = source->mTextureCoords[0][vertexIndex].x;
                        mesh.texcoords[vertexIndex * 2 + 1] = source->mTextureCoords[0][vertexIndex].y;
                    }
                    else
                    {
                        mesh.texcoords[vertexIndex * 2] = 0.0f;
                        mesh.texcoords[vertexIndex * 2 + 1] = 0.0f;
                    }
                }

                int index = 0;
                for (unsigned int faceIndex = 0; faceIndex < source->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = source->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        error = "Assimp returned a non-triangulated FBX face";
                        return false;
                    }
                    mesh.indices[index++] = (unsigned short)face.mIndices[0];
                    mesh.indices[index++] = (unsigned short)face.mIndices[1];
                    mesh.indices[index++] = (unsigned short)face.mIndices[2];
                }

                for (unsigned int sourceBoneIndex = 0; sourceBoneIndex < source->mNumBones; ++sourceBoneIndex)
                {
                    const aiBone *sourceBone = source->mBones[sourceBoneIndex];
                    if (!sourceBone)
                    {
                        error = "Assimp returned a null FBX bone";
                        return false;
                    }

                    const std::string boneName = sourceBone->mName.C_Str();
                    auto found = bones.find(boneName);
                    if (found == bones.end())
                    {
                        BoneData data;
                        data.index = (int)bones.size();
                        found = bones.emplace(boneName, data).first;
                    }
                    // O offset pertence ao par malha/osso, nao apenas ao osso.
                    // Modelos com varias partes podem reutilizar o mesmo nome de
                    // osso com offsets diferentes em cada aiMesh.
                    skin.boneOffsets[found->second.index] = sourceBone->mOffsetMatrix;

                    if (sourceBone->mNumWeights > 0 && !sourceBone->mWeights)
                    {
                        error = "Assimp returned an FBX bone without its weight data";
                        return false;
                    }

                    for (unsigned int weightIndex = 0; weightIndex < sourceBone->mNumWeights; ++weightIndex)
                    {
                        const aiVertexWeight &weight = sourceBone->mWeights[weightIndex];
                        if (weight.mVertexId >= skin.influences.size())
                        {
                            error = "Assimp returned an FBX bone weight with an invalid vertex index";
                            return false;
                        }

                        VertexInfluence &influence = skin.influences[weight.mVertexId];
                        for (int slot = 0; slot < 4; ++slot)
                        {
                            if (influence.boneIndices[slot] < 0)
                            {
                                influence.boneIndices[slot] = found->second.index;
                                influence.boneWeights[slot] = weight.mWeight;
                                break;
                            }
                        }
                    }
                }

                for (VertexInfluence &influence : skin.influences)
                {
                    float sum = 0.0f;
                    for (float weight : influence.boneWeights) sum += weight;
                    if (sum > 0.0f)
                    {
                        for (float &weight : influence.boneWeights) weight /= sum;
                    }
                }

                model.meshMaterial[meshIndex] = source->mMaterialIndex < (unsigned int)model.materialCount
                    ? (int)source->mMaterialIndex : 0;
                UploadMesh(&mesh, true);
            }

            sourceMinY = minY;
            sourceHeight = std::max(0.001f, maxY - minY);
            sourceMaxDimension = std::max({ 0.001f, maxX - minX, maxY - minY, maxZ - minZ });
            sourceMin = { minX, minY, minZ };
            sourceMax = { maxX, maxY, maxZ };
            boneTransforms.resize(bones.size());
            ApplyMaterialTextures(modelPath);
            ApplyLightingShader();
            return true;
        }

        void UpdateSkinning()
        {
            for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
            {
                Mesh &mesh = model.meshes[meshIndex];
                const MeshSkinData &skin = meshSkinData[meshIndex];

                for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex)
                {
                    const VertexInfluence &influence = skin.influences[vertexIndex];
                    aiVector3D position(0.0f);
                    aiVector3D normal(0.0f);
                    float totalWeight = 0.0f;

                    for (int slot = 0; slot < 4; ++slot)
                    {
                        const int boneIndex = influence.boneIndices[slot];
                        const float weight = influence.boneWeights[slot];
                        if (boneIndex < 0 || weight <= 0.0f) continue;

                        const aiMatrix4x4 &transform = boneTransforms[boneIndex];
                        auto offsetIt = skin.boneOffsets.find(boneIndex);
                        const aiMatrix4x4 skinTransform = offsetIt != skin.boneOffsets.end()
                            ? transform * offsetIt->second : transform;
                        position += (skinTransform * skin.vertices[vertexIndex]) * weight;
                        aiMatrix3x3 normalTransform(skinTransform);
                        normalTransform.Inverse().Transpose();
                        normal += (normalTransform * skin.normals[vertexIndex]) * weight;
                        totalWeight += weight;
                    }

                    if (totalWeight <= 0.0f)
                    {
                        position = meshTransforms[meshIndex] * skin.vertices[vertexIndex];
                        aiMatrix3x3 meshNormalTransform(meshTransforms[meshIndex]);
                        meshNormalTransform.Inverse().Transpose();
                        normal = meshNormalTransform * skin.normals[vertexIndex];
                    }
                    normal.Normalize();

                    position = coordinateTransform * position;
                    normal = coordinateNormalTransform * normal;
                    normal.Normalize();

                    mesh.animVertices[vertexIndex * 3] = position.x;
                    mesh.animVertices[vertexIndex * 3 + 1] = position.y;
                    mesh.animVertices[vertexIndex * 3 + 2] = position.z;
                    mesh.animNormals[vertexIndex * 3] = normal.x;
                    mesh.animNormals[vertexIndex * 3 + 1] = normal.y;
                    mesh.animNormals[vertexIndex * 3 + 2] = normal.z;
                }

                UpdateMeshBuffer(mesh, 0, mesh.animVertices, mesh.vertexCount * 3 * (int)sizeof(float), 0);
                UpdateMeshBuffer(mesh, 2, mesh.animNormals, mesh.vertexCount * 3 * (int)sizeof(float), 0);
            }
        }
    };

    FbxModel::FbxModel() = default;

    FbxModel::~FbxModel()
    {
        Unload();
    }

    bool FbxModel::Load(const std::string &modelPath, const std::string &animationPath)
    {
        Unload();
        mImpl = new Impl();
        std::string extension = std::filesystem::path(modelPath).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (extension == ".gltf" || extension == ".glb")
        {
            mImpl->model = LoadModel(modelPath.c_str());
            if (mImpl->model.meshCount <= 0)
            {
                mLastError = std::string("Could not import native animated model: ") + modelPath;
                return false;
            }
            mImpl->raylibAnimations = LoadModelAnimations(modelPath.c_str(), &mImpl->raylibAnimationCount);
            BoundingBox bounds = GetModelBoundingBox(mImpl->model);
            mImpl->sourceMin = bounds.min;
            mImpl->sourceMax = bounds.max;
            mImpl->sourceMinY = bounds.min.y;
            mImpl->sourceHeight = std::max(0.001f, bounds.max.y - bounds.min.y);
            mImpl->sourceMaxDimension = std::max({0.001f, bounds.max.x - bounds.min.x,
                                                  bounds.max.y - bounds.min.y,
                                                  bounds.max.z - bounds.min.z});
            mImpl->meshNames.resize(mImpl->model.meshCount);
            mImpl->raylibAnimation = true;
            mImpl->loaded = true;
            mImpl->ApplyLightingShader();
            TraceLog(LOG_INFO, "MODEL: Loaded native glTF (%i meshes, %i animations)",
                     mImpl->model.meshCount, mImpl->raylibAnimationCount);
            return true;
        }
        std::error_code pathError;
        const std::filesystem::path normalizedModel = std::filesystem::weakly_canonical(modelPath, pathError);
        pathError.clear();
        const std::filesystem::path normalizedAnimation = std::filesystem::weakly_canonical(animationPath, pathError);
        mImpl->nativeAnimation = !pathError && normalizedModel == normalizedAnimation;

        const unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_LimitBoneWeights | aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType | aiProcess_ValidateDataStructure |
            aiProcess_FlipUVs;

        mImpl->modelScene = aiImportFile(modelPath.c_str(), flags);
        if (!mImpl->modelScene || !mImpl->modelScene->mRootNode || mImpl->modelScene->mNumMeshes == 0)
        {
            mLastError = std::string("Could not import player FBX: ") + aiGetErrorString();
            return false;
        }

        mImpl->coordinateTransform = mImpl->modelScene->mRootNode->mTransformation;
        mImpl->coordinateNormalTransform = aiMatrix3x3(mImpl->coordinateTransform);
        mImpl->coordinateNormalTransform.Inverse().Transpose();
        mImpl->globalInverseTransform = mImpl->modelScene->mRootNode->mTransformation;
        mImpl->globalInverseTransform.Inverse();

        if (!mImpl->BuildModel(std::filesystem::path(modelPath), mLastError)) return false;

        // Props e itens estaticos usam o mesmo pipeline de materiais/escala,
        // mas nao precisam importar uma biblioteca inteira de animacoes.
        if (animationPath.empty())
        {
            mImpl->loaded = true;
            TraceLog(LOG_INFO, "FBX: Loaded static model (%i meshes)", mImpl->model.meshCount);
            return true;
        }

        mImpl->animationScene = aiImportFile(animationPath.c_str(), flags);
        if (!mImpl->animationScene || !mImpl->animationScene->mRootNode ||
            mImpl->animationScene->mNumAnimations == 0)
        {
            mLastError = std::string("Could not import animation FBX: ") + aiGetErrorString();
            return false;
        }

        CollectBindRotations(mImpl->animationScene->mRootNode, mImpl->animationBindRotations);

        mImpl->loaded = true;
        PlayAnimation("Idle_Loop", true);
        Update(0.0f);
        TraceLog(LOG_INFO, "FBX: Loaded player (%i meshes, %i bones, %i animations)",
                 mImpl->model.meshCount, (int)mImpl->bones.size(), (int)mImpl->animationScene->mNumAnimations);
        return true;
    }

    void FbxModel::Unload()
    {
        if (!mImpl) return;

        for (Texture2D texture : mImpl->textures)
        {
            if (texture.id != 0) TextureRelease(texture);
        }
        if (mImpl->lightingShader.id != 0) UnloadShader(mImpl->lightingShader);
        if (mImpl->raylibAnimations)
            UnloadModelAnimations(mImpl->raylibAnimations, mImpl->raylibAnimationCount);
        if (mImpl->model.meshes) UnloadModel(mImpl->model);
        if (mImpl->animationScene) aiReleaseImport(mImpl->animationScene);
        if (mImpl->modelScene) aiReleaseImport(mImpl->modelScene);
        delete mImpl;
        mImpl = nullptr;
    }

    void FbxModel::PlayAnimation(const char *animationName, bool loop, float playbackSpeed)
    {
        if (!mImpl || !mImpl->loaded) return;
        if (mImpl->raylibAnimation)
        {
            if (!animationName) return;
            std::string requested(animationName);
            std::transform(requested.begin(), requested.end(), requested.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            for (int i = 0; i < mImpl->raylibAnimationCount; ++i)
            {
                std::string name(mImpl->raylibAnimations[i].name);
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
                if (name == requested || name.find(requested) != std::string::npos)
                {
                    if (mImpl->raylibCurrentAnimation != i) mImpl->raylibAnimationFrame = 0.0f;
                    mImpl->raylibCurrentAnimation = i;
                    mImpl->loop = loop;
                    mImpl->playbackSpeed = playbackSpeed;
                    mImpl->currentAnimationName = animationName;
                    return;
                }
            }
            return;
        }
        const aiAnimation *animation = mImpl->FindAnimation(animationName);
        if (!animation)
        {
            TraceLog(LOG_WARNING, "FBX: Animation not found: %s", animationName);
            return;
        }

        if (mImpl->currentAnimation != animation)
        {
            mImpl->currentAnimation = animation;
            mImpl->currentAnimationName = animationName;
            mImpl->animationSeconds = 0.0f;
#ifdef _DEBUG
            TraceLog(LOG_INFO, "FBX: Playing animation: %s", animationName);
#endif
        }
        mImpl->loop = loop;
        mImpl->playbackSpeed = playbackSpeed;
    }

    bool FbxModel::PlayFirstAnimationContaining(const char *fragment, bool loop,
                                                float playbackSpeed)
    {
        if (!mImpl || !mImpl->loaded || !fragment) return false;

        std::string needle(fragment);
        std::transform(needle.begin(), needle.end(), needle.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (mImpl->raylibAnimation)
        {
            for (int i = 0; i < mImpl->raylibAnimationCount; ++i)
            {
                std::string name(mImpl->raylibAnimations[i].name);
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c) { return (char)std::tolower(c); });
                if (name.find(needle) != std::string::npos)
                {
                    PlayAnimation(mImpl->raylibAnimations[i].name, loop, playbackSpeed);
                    return true;
                }
            }
            return false;
        }
        for (unsigned int i = 0; i < mImpl->animationScene->mNumAnimations; ++i)
        {
            const aiAnimation *animation = mImpl->animationScene->mAnimations[i];
            std::string name = animation->mName.C_Str();
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            if (name.find(needle) != std::string::npos)
            {
                PlayAnimation(animation->mName.C_Str(), loop, playbackSpeed);
                return true;
            }
        }
        return false;
    }

    void FbxModel::SynchronizeAnimationFrom(const FbxModel &source)
    {
        if (!mImpl || !mImpl->loaded || !source.mImpl || !source.mImpl->loaded ||
            source.mImpl->currentAnimationName.empty()) return;
        PlayAnimation(source.mImpl->currentAnimationName.c_str(),
                      source.mImpl->loop, source.mImpl->playbackSpeed);
        mImpl->animationSeconds = source.mImpl->animationSeconds;
        Update(0.0f);
    }

    void FbxModel::Update(float deltaTime)
    {
        if (!mImpl || !mImpl->loaded) return;
        if (mImpl->raylibAnimation)
        {
            if (mImpl->raylibCurrentAnimation < 0) return;
            ModelAnimation &animation = mImpl->raylibAnimations[mImpl->raylibCurrentAnimation];
            mImpl->raylibAnimationFrame += deltaTime * 60.0f * mImpl->playbackSpeed;
            if (animation.keyframeCount > 0)
            {
                if (mImpl->loop)
                    mImpl->raylibAnimationFrame = std::fmod(mImpl->raylibAnimationFrame, (float)animation.keyframeCount);
                else
                    mImpl->raylibAnimationFrame = std::min(mImpl->raylibAnimationFrame, (float)animation.keyframeCount - 1.0f);
                UpdateModelAnimation(mImpl->model, animation, mImpl->raylibAnimationFrame);
            }
            return;
        }
        if (!mImpl->currentAnimation) return;

        mImpl->animationSeconds += deltaTime * mImpl->playbackSpeed;
        const double ticksPerSecond = mImpl->currentAnimation->mTicksPerSecond > 0.0
            ? mImpl->currentAnimation->mTicksPerSecond : 30.0;
        double animationTime = mImpl->animationSeconds * ticksPerSecond;
        const double duration = mImpl->currentAnimation->mDuration;
        if (duration > 0.0)
        {
            if (mImpl->loop)
            {
                animationTime = std::fmod(animationTime, duration);
                if (animationTime < 0.0) animationTime += duration;
            }
            else animationTime = std::min(animationTime, duration);
        }

        std::fill(mImpl->boneTransforms.begin(), mImpl->boneTransforms.end(), aiMatrix4x4());
        mImpl->nodeGlobalTransforms.clear();
        mImpl->ReadNodeHierarchy(animationTime, mImpl->modelScene->mRootNode, aiMatrix4x4());
        mImpl->UpdateSkinning();
    }

    bool FbxModel::GetBoneLocalPosition(const char *nameFragment, Vector3 &position) const
    {
        if (!mImpl || !mImpl->loaded || !nameFragment || !*nameFragment) return false;

        std::string fragment(nameFragment);
        std::transform(fragment.begin(), fragment.end(), fragment.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        for (const auto &entry : mImpl->nodeGlobalTransforms)
        {
            std::string name = entry.first;
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });
            if (name.find(fragment) == std::string::npos) continue;

            const aiVector3D rootRelative = mImpl->globalInverseTransform * entry.second * aiVector3D();
            const aiVector3D displayPosition = mImpl->coordinateTransform * rootRelative;
            // Internal model space is Y-up, so bone positions are returned in
            // that same space, relative to the model's feet (sourceMinY).
            position = Vector3{ displayPosition.x, displayPosition.y - mImpl->sourceMinY,
                                displayPosition.z };
            return true;
        }
        return false;
    }

    void FbxModel::Draw(Vector3 footPosition, float yawDegrees, float targetHeight, Color tint,
                        Vector3 cameraPosition, Vector3 skinColor, float lightIntensity,
                        bool headOnly, float headCutoffWorldY,
                        bool preferNamedHeadMeshes, Vector3 extraScale,
                        Vector3 extraRotationDegrees) const
    {
        if (!mImpl || !mImpl->loaded) return;
        const float scale = targetHeight / mImpl->sourceHeight;
        Vector3 foot = footPosition;
        foot.y -= mImpl->sourceMinY * scale * extraScale.y;

        Matrix matScale = MatrixScale(scale * extraScale.x, scale * extraScale.y, scale * extraScale.z);
        Matrix matRotation = MatrixRotateXYZ({extraRotationDegrees.x * DEG2RAD,
                                              extraRotationDegrees.y * DEG2RAD,
                                              extraRotationDegrees.z * DEG2RAD});
        matRotation = MatrixMultiply(matRotation,
            MatrixRotate({ 0.0f, 1.0f, 0.0f }, yawDegrees * DEG2RAD));
        Matrix matTranslation = MatrixTranslate(foot.x, foot.y, foot.z);
        Matrix transform = MatrixMultiply(mImpl->model.transform,
                           MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation));
        mImpl->DrawMeshes(transform, tint, cameraPosition, skinColor, lightIntensity,
                          headOnly, headCutoffWorldY, preferNamedHeadMeshes);
    }

    void FbxModel::DrawAttached(Vector3 anchorWorld, Vector3 longAxisWorld,
                                Vector3 rollReferenceWorld, float targetMaxDimension,
                                Color tint, Vector3 cameraPosition, Vector3 skinColor,
                                float lightIntensity) const
    {
        if (!mImpl || !mImpl->loaded || targetMaxDimension <= 0.0f) return;

        const Vector3 anchor = anchorWorld;
        Vector3 axisZ = Vector3Normalize(longAxisWorld);
        if (Vector3LengthSqr(axisZ) < 0.0001f) axisZ = { 0.0f, 1.0f, 0.0f };
        const Vector3 rollReference = rollReferenceWorld;

        // Keep the weapon's local X (the axe blades) aligned with the roll
        // reference, which the caller supplies as the character's side.
        Vector3 axisX = Vector3Subtract(rollReference,
                                        Vector3Scale(axisZ, Vector3DotProduct(rollReference, axisZ)));
        axisX = Vector3Normalize(axisX);
        if (Vector3LengthSqr(axisX) < 0.0001f)
        {
            Vector3 fallback = fabsf(axisZ.y) > 0.9f ? Vector3{ 1.0f, 0.0f, 0.0f }
                                                     : Vector3{ 0.0f, 1.0f, 0.0f };
            axisX = Vector3Subtract(fallback, Vector3Scale(axisZ, Vector3DotProduct(fallback, axisZ)));
            axisX = Vector3Normalize(axisX);
        }
        Vector3 axisY = Vector3Normalize(Vector3CrossProduct(axisZ, axisX));

        const float scale = targetMaxDimension / mImpl->sourceMaxDimension;

        // Grip point: the pommel at the minimum end of the long axis, centered
        // on the two remaining axes. For Axe_Double that is (0.0, 0.0, -0.66).
        const Vector3 localAnchor = { (mImpl->sourceMin.x + mImpl->sourceMax.x) * 0.5f,
                                      (mImpl->sourceMin.y + mImpl->sourceMax.y) * 0.5f,
                                      mImpl->sourceMin.z };
        Matrix transform = MatrixIdentity();
        transform.m0 = axisX.x * scale; transform.m1 = axisX.y * scale; transform.m2 = axisX.z * scale;
        transform.m4 = axisY.x * scale; transform.m5 = axisY.y * scale; transform.m6 = axisY.z * scale;
        transform.m8 = axisZ.x * scale; transform.m9 = axisZ.y * scale; transform.m10 = axisZ.z * scale;
        transform.m12 = anchor.x - scale * (axisX.x * localAnchor.x + axisY.x * localAnchor.y + axisZ.x * localAnchor.z);
        transform.m13 = anchor.y - scale * (axisX.y * localAnchor.x + axisY.y * localAnchor.y + axisZ.y * localAnchor.z);
        transform.m14 = anchor.z - scale * (axisX.z * localAnchor.x + axisY.z * localAnchor.y + axisZ.z * localAnchor.z);
        mImpl->DrawMeshes(transform, tint, cameraPosition, skinColor, lightIntensity,
                          false, 0.0f, false);
    }

    bool FbxModel::IsLoaded() const
    {
        return mImpl && mImpl->loaded;
    }

    float FbxModel::GetSourceHeight() const
    {
        return mImpl ? mImpl->sourceHeight : 0.0f;
    }

    float FbxModel::GetSourceMaxDimension() const
    {
        return mImpl ? mImpl->sourceMaxDimension : 0.0f;
    }

    namespace
    {
        void CollectStaticMeshTransforms(const aiNode *node, const aiMatrix4x4 &parent,
                                         std::vector<aiMatrix4x4> &transforms)
        {
            const aiMatrix4x4 global = parent * node->mTransformation;
            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                const unsigned int meshIndex = node->mMeshes[i];
                if (meshIndex < transforms.size()) transforms[meshIndex] = global;
            }
            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                CollectStaticMeshTransforms(node->mChildren[i], global, transforms);
        }
    }

    bool LoadStaticFbxModel(const std::string &modelPath, Model &outModel,
                            std::vector<Texture2D> &outTextures, std::string &error)
    {
        outModel = {};
        outTextures.clear();
        error.clear();

        const unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_ImproveCacheLocality | aiProcess_SortByPType | aiProcess_FlipUVs;
        const aiScene *scene = aiImportFile(modelPath.c_str(), flags);
        if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
        {
            error = std::string("Could not import FBX: ") + aiGetErrorString();
            if (scene) aiReleaseImport(scene);
            return false;
        }

        auto fail = [&](const char *message) -> bool
        {
            error = message;
            aiReleaseImport(scene);
            for (Texture2D &texture : outTextures)
                if (texture.id != 0) TextureRelease(texture);
            outTextures.clear();
            UnloadModel(outModel);
            outModel = {};
            return false;
        };

        outModel.meshCount = (int)scene->mNumMeshes;
        outModel.materialCount = std::max(1, (int)scene->mNumMaterials);
        outModel.meshes = (Mesh *)MemAlloc(sizeof(Mesh) * outModel.meshCount);
        outModel.materials = (Material *)MemAlloc(sizeof(Material) * outModel.materialCount);
        outModel.meshMaterial = (int *)MemAlloc(sizeof(int) * outModel.meshCount);
        std::memset(outModel.meshes, 0, sizeof(Mesh) * outModel.meshCount);
        std::memset(outModel.materials, 0, sizeof(Material) * outModel.materialCount);
        outModel.transform = MatrixIdentity();
        for (int i = 0; i < outModel.materialCount; ++i)
            outModel.materials[i] = LoadMaterialDefault();

        std::vector<aiMatrix4x4> globalTransforms(outModel.meshCount);
        CollectStaticMeshTransforms(scene->mRootNode, aiMatrix4x4(), globalTransforms);
        const aiMatrix4x4 rootTransform = scene->mRootNode->mTransformation;
        aiMatrix4x4 rootInverse = rootTransform;
        rootInverse.Inverse();
        aiMatrix3x3 coordinateNormalTransform(rootTransform);
        coordinateNormalTransform.Inverse().Transpose();

        for (int meshIndex = 0; meshIndex < outModel.meshCount; ++meshIndex)
        {
            const aiMesh *source = scene->mMeshes[meshIndex];
            if (source->mNumVertices > 65535)
                return fail("FBX mesh exceeds the 16-bit raylib index limit");

            Mesh &mesh = outModel.meshes[meshIndex];
            mesh.vertexCount = (int)source->mNumVertices;
            mesh.triangleCount = (int)source->mNumFaces;
            mesh.vertices = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);
            mesh.normals = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 3);
            mesh.texcoords = (float *)MemAlloc(sizeof(float) * mesh.vertexCount * 2);
            mesh.indices = (unsigned short *)MemAlloc(sizeof(unsigned short) * mesh.triangleCount * 3);

            const aiMatrix4x4 relativeTransform = rootInverse * globalTransforms[meshIndex];
            aiMatrix3x3 normalTransform(relativeTransform);
            normalTransform.Inverse().Transpose();

            for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex)
            {
                const aiVector3D vertex = source->mVertices[vertexIndex];
                const aiVector3D normal = source->HasNormals() ? source->mNormals[vertexIndex]
                                                               : aiVector3D(0.0f, 1.0f, 0.0f);
                const aiVector3D displayVertex = globalTransforms[meshIndex] * vertex;
                aiVector3D displayNormal = coordinateNormalTransform * normalTransform * normal;
                displayNormal.Normalize();

                mesh.vertices[vertexIndex * 3 + 0] = displayVertex.x;
                mesh.vertices[vertexIndex * 3 + 1] = displayVertex.y;
                mesh.vertices[vertexIndex * 3 + 2] = displayVertex.z;
                mesh.normals[vertexIndex * 3 + 0] = displayNormal.x;
                mesh.normals[vertexIndex * 3 + 1] = displayNormal.y;
                mesh.normals[vertexIndex * 3 + 2] = displayNormal.z;

                if (source->HasTextureCoords(0))
                {
                    mesh.texcoords[vertexIndex * 2] = source->mTextureCoords[0][vertexIndex].x;
                    mesh.texcoords[vertexIndex * 2 + 1] = source->mTextureCoords[0][vertexIndex].y;
                }
                else
                {
                    mesh.texcoords[vertexIndex * 2] = 0.0f;
                    mesh.texcoords[vertexIndex * 2 + 1] = 0.0f;
                }
            }

            int index = 0;
            for (unsigned int faceIndex = 0; faceIndex < source->mNumFaces; ++faceIndex)
            {
                const aiFace &face = source->mFaces[faceIndex];
                if (face.mNumIndices != 3)
                    return fail("Assimp returned a non-triangulated FBX face");
                mesh.indices[index++] = (unsigned short)face.mIndices[0];
                mesh.indices[index++] = (unsigned short)face.mIndices[1];
                mesh.indices[index++] = (unsigned short)face.mIndices[2];
            }        outModel.meshMaterial[meshIndex] =
            source->mMaterialIndex < (unsigned int)outModel.materialCount
            ? (int)source->mMaterialIndex : 0;
            UploadMesh(&mesh, true);
        }

        std::unordered_map<std::string, Texture2D> textureCache;
        const aiTextureType albedoTypes[] = { aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR };
        for (int materialIndex = 0; materialIndex < outModel.materialCount; ++materialIndex)
        {
            const aiMaterial *source = scene->mMaterials ? scene->mMaterials[materialIndex] : nullptr;
            if (!source) continue;
            Material &destination = outModel.materials[materialIndex];

            aiColor4D color;
            if (aiGetMaterialColor(source, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
            {
                destination.maps[MATERIAL_MAP_ALBEDO].color = {
                    (unsigned char)(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
                    (unsigned char)(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
                    (unsigned char)(std::clamp(color.b, 0.0f, 1.0f) * 255.0f),
                    (unsigned char)(std::clamp(color.a, 0.0f, 1.0f) * 255.0f)
                };
            }

            aiString textureName;
            for (aiTextureType type : albedoTypes)
            {
                if (source->GetTexture(type, 0, &textureName) != AI_SUCCESS) continue;
                std::filesystem::path texturePath = ResolveTexturePath(modelPath, textureName);
                if (texturePath.empty()) continue;
                const std::string key = texturePath.lexically_normal().string();
                Texture2D texture = {};
                auto found = textureCache.find(key);
                if (found != textureCache.end()) texture = found->second;
                else
                {
                    texture = TextureAcquire(key);
                    if (texture.id != 0)
                    {
                        GenTextureMipmaps(&texture);
                        SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_8X);
                        textureCache[key] = texture;
                        outTextures.push_back(texture);
                    }
                }
                if (texture.id != 0)
                {
                    destination.maps[MATERIAL_MAP_ALBEDO].texture = texture;
                    destination.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
                }
                break;
            }

            if (source->GetTexture(aiTextureType_NORMALS, 0, &textureName) == AI_SUCCESS)
            {
                std::filesystem::path texturePath = ResolveTexturePath(modelPath, textureName);
                if (!texturePath.empty())
                {
                    const std::string key = texturePath.lexically_normal().string();
                    Texture2D texture = {};
                    auto found = textureCache.find(key);
                    if (found != textureCache.end()) texture = found->second;
                    else
                    {
                        texture = TextureAcquire(key);
                        if (texture.id != 0)
                        {
                            GenTextureMipmaps(&texture);
                            SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_8X);
                            textureCache[key] = texture;
                            outTextures.push_back(texture);
                        }
                    }
                    if (texture.id != 0) destination.maps[MATERIAL_MAP_NORMAL].texture = texture;
                }
            }
        }

        aiReleaseImport(scene);
        return true;
    }
}
