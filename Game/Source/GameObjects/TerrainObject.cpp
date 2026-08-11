#include "GameObjects/TerrainObject.h"
#include "Assets/TextureLoader.h"
#include "Scene.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include <cmath>
#include <string>
#include <algorithm>

namespace game
{
    static const char *kTerrainVertexShader = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPosition;
void main() {
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
})GLSL";

    static const char *kTerrainFragmentShader = R"GLSL(
#version 330
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPosition;
uniform sampler2D grassTexture;
uniform sampler2D dirtTexture;
uniform sampler2D sandTexture;
uniform sampler2D pavementTexture;
// O stencil usa a unidade principal do material (GL_TEXTURE0). As quatro
// unidades adicionais do batch ficam reservadas para as quatro camadas.
uniform sampler2D texture0;
uniform vec4 uvScales;
out vec4 finalColor;

void main() {
    vec4 weights = max(texture(texture0, fragTexCoord), vec4(0.0));
    float rawCoverage = dot(weights, vec4(1.0));
    if (rawCoverage > 1.0) weights /= rawCoverage;
    float coverage = clamp(rawCoverage, 0.0, 1.0);
    vec3 painted = texture(grassTexture, fragTexCoord * uvScales.x).rgb * weights.r
                 + texture(dirtTexture, fragTexCoord * uvScales.y).rgb * weights.g
                 + texture(sandTexture, fragTexCoord * uvScales.z).rgb * weights.b
                 + texture(pavementTexture, fragTexCoord * uvScales.w).rgb * weights.a;
    // Sem tinta no stencil, preserva exatamente o aspecto verde-base do mundo.
    vec3 albedo = vec3(0.275, 0.608, 0.255) * (1.0 - coverage) + painted;
    vec3 lightDir = normalize(vec3(-0.3, 1.0, -0.2));
    // Exposicao/ambiente: piso de luz mais alto, mantendo as cores originais
    // do albedo (sombras desligadas).
    float diffuse = 0.72 + max(dot(normalize(fragNormal), lightDir), 0.0) * 0.60;
    finalColor = vec4(albedo * diffuse, 1.0);
})GLSL";

    static float SmoothNoise2D(int ix, int iz)
    {
        const float scale = 0.08f;
        float s = 0.0f, amp = 0.0f, freq = 1.0f, weight = 1.0f;
        for (int oct = 0; oct < 4; ++oct)
        {
            float nx = ix * scale * freq;
            float nz = iz * scale * freq;
            s   += sinf(nx * 1.3f) * cosf(nz * 1.7f + nx * 0.5f) * weight;
            amp += weight;
            weight *= 0.5f;
            freq  *= 2.0f;
        }
        return s / amp;
    }

    TerrainObject::TerrainObject(int tilesX, int tilesZ, float tileSize)
        : mTilesX(tilesX), mTilesZ(tilesZ), mTileSize(tileSize)
    {
        mMaterial = LoadMaterialDefault();
    }

    TerrainObject::~TerrainObject()
    {
        if (mMeshLoaded) UnloadMesh(mRayMesh);
        for (Texture2D &texture : mLayerTextures) if (texture.id) TextureRelease(texture);
        if (mStencilTexture.id) UnloadTexture(mStencilTexture);
        if (mTerrainShader.id) UnloadShader(mTerrainShader);
    }

    void TerrainObject::OnSpawn(Scene &scene)
    {
        GenerateHeightmap();
        InitTerrainMaterial();
        BuildMesh();
        UploadMeshToJolt(scene);

        mPosition.x = 0.0f;
        mPosition.y = 0.0f;
        mPosition.z = 0.0f;
    }

    void TerrainObject::InitTerrainMaterial()
    {
        const char *names[] = {"grass.png", "dirt.png", "sand.png", "pavement.png"};
        for (int i = 0; i < 4; ++i)
        {
            std::string path = std::string("Assets/Terrain/") + names[i];
            if (!TextureFileExists(path)) path = std::string("Game/Assets/Terrain/") + names[i];
            if (!TextureFileExists(path)) path = std::string("Game/Assets/Terrian/") + names[i];
            mLayerTextures[i] = TextureAcquire(path);
            if (mLayerTextures[i].id)
            {
                SetTextureFilter(mLayerTextures[i], TEXTURE_FILTER_BILINEAR);
                SetTextureWrap(mLayerTextures[i], TEXTURE_WRAP_REPEAT);
            }
        }
        mStencilWidth = mTilesX + 1;
        mStencilHeight = mTilesZ + 1;
        // RGBA vazio significa que nenhuma textura foi pintada ainda.
        mStencilPixels.assign((size_t)mStencilWidth * mStencilHeight, Color{0,0,0,0});
        Image stencil = {}; stencil.data = mStencilPixels.data(); stencil.width = mStencilWidth;
        stencil.height = mStencilHeight; stencil.mipmaps = 1;
        stencil.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        mStencilTexture = LoadTextureFromImage(stencil);
        SetTextureFilter(mStencilTexture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(mStencilTexture, TEXTURE_WRAP_CLAMP);
        mTerrainShader = LoadShaderFromMemory(kTerrainVertexShader, kTerrainFragmentShader);
        mMaterial.shader = mTerrainShader;
        mMaterial.maps[MATERIAL_MAP_ALBEDO].texture = mStencilTexture;
        mMaterial.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    }

    void TerrainObject::UploadStencil()
    {
        if (mStencilTexture.id && !mStencilPixels.empty())
            UpdateTexture(mStencilTexture, mStencilPixels.data());
    }

    void TerrainObject::PaintTexture(float x, float z, float radius, float strength, int layer)
    {
        if (layer < 0 || layer > 3 || radius <= 0.0f || strength <= 0.0f) return;
        const float halfWidth = (float)mTilesX * mTileSize * 0.5f;
        const float halfDepth = (float)mTilesZ * mTileSize * 0.5f;
        bool affected = false;
        for (int iz = 0; iz < mStencilHeight; ++iz)
        {
            const float worldZ = (float)iz / (mStencilHeight - 1) * (mTilesZ * mTileSize) - halfDepth;
            for (int ix = 0; ix < mStencilWidth; ++ix)
            {
                const float worldX = (float)ix / (mStencilWidth - 1) * (mTilesX * mTileSize) - halfWidth;
                const float dx = worldX - x, dz = worldZ - z;
                const float distance = sqrtf(dx*dx + dz*dz);
                if (distance >= radius) continue;
                const float falloff = 1.0f - distance / radius;
                const float blend = Clamp(strength * falloff * falloff, 0.0f, 1.0f);
                Color &pixel = mStencilPixels[(size_t)iz * mStencilWidth + ix];
                unsigned char *channels = &pixel.r;
                for (int c = 0; c < 4; ++c)
                {
                    const float target = c == layer ? 255.0f : 0.0f;
                    channels[c] = (unsigned char)Clamp(channels[c] + (target - channels[c]) * blend, 0.0f, 255.0f);
                }
                affected = true;
            }
        }
        // Um brush menor que o espacamento do stencil ainda deve pintar o texel sob o cursor.
        if (!affected)
        {
            const int ix = std::clamp((int)roundf((x + halfWidth) /
                (mTilesX * mTileSize) * (mStencilWidth - 1)), 0, mStencilWidth - 1);
            const int iz = std::clamp((int)roundf((z + halfDepth) /
                (mTilesZ * mTileSize) * (mStencilHeight - 1)), 0, mStencilHeight - 1);
            Color &pixel = mStencilPixels[(size_t)iz * mStencilWidth + ix];
            unsigned char *channels = &pixel.r;
            const float blend = Clamp(strength, 0.0f, 1.0f);
            for (int c = 0; c < 4; ++c)
            {
                const float target = c == layer ? 255.0f : 0.0f;
                channels[c] = (unsigned char)Clamp(channels[c] + (target - channels[c]) * blend, 0.0f, 255.0f);
            }
        }
        UploadStencil();
    }

    void TerrainObject::EraseTexture(float x, float z, float radius, float strength)
    {
        if (radius <= 0.0f || strength <= 0.0f) return;
        const float halfWidth = (float)mTilesX * mTileSize * 0.5f;
        const float halfDepth = (float)mTilesZ * mTileSize * 0.5f;
        bool affected = false;
        for (int iz = 0; iz < mStencilHeight; ++iz)
        {
            const float worldZ = (float)iz / (mStencilHeight - 1) * (mTilesZ * mTileSize) - halfDepth;
            for (int ix = 0; ix < mStencilWidth; ++ix)
            {
                const float worldX = (float)ix / (mStencilWidth - 1) * (mTilesX * mTileSize) - halfWidth;
                const float dx = worldX - x, dz = worldZ - z;
                const float distance = sqrtf(dx*dx + dz*dz);
                if (distance >= radius) continue;
                const float falloff = 1.0f - distance / radius;
                const float blend = Clamp(strength * falloff * falloff, 0.0f, 1.0f);
                Color &pixel = mStencilPixels[(size_t)iz * mStencilWidth + ix];
                unsigned char *channels = &pixel.r;
                for (int c = 0; c < 4; ++c)
                    channels[c] = (unsigned char)Clamp(channels[c] * (1.0f - blend), 0.0f, 255.0f);
                affected = true;
            }
        }
        // Um brush menor que o espacamento do stencil ainda deve apagar o texel sob o cursor.
        if (!affected)
        {
            const int ix = std::clamp((int)roundf((x + halfWidth) /
                (mTilesX * mTileSize) * (mStencilWidth - 1)), 0, mStencilWidth - 1);
            const int iz = std::clamp((int)roundf((z + halfDepth) /
                (mTilesZ * mTileSize) * (mStencilHeight - 1)), 0, mStencilHeight - 1);
            Color &pixel = mStencilPixels[(size_t)iz * mStencilWidth + ix];
            unsigned char *channels = &pixel.r;
            const float blend = Clamp(strength, 0.0f, 1.0f);
            for (int c = 0; c < 4; ++c)
                channels[c] = (unsigned char)Clamp(channels[c] * (1.0f - blend), 0.0f, 255.0f);
        }
        UploadStencil();
    }

    void TerrainObject::SetStencilMap(const std::vector<Color> &pixels)
    {
        if (pixels.size() != mStencilPixels.size()) return;
        mStencilPixels = pixels; UploadStencil();
    }

    void TerrainObject::SetUVScales(const float scales[4])
    {
        for (int i = 0; i < 4; ++i) mUVScales[i] = std::max(0.001f, scales[i]);
    }

    Texture2D TerrainObject::GetLayerTexture(int layer) const
    {
        return layer >= 0 && layer < 4 ? mLayerTextures[layer] : Texture2D{};
    }

    void TerrainObject::GenerateHeightmap()
    {
        mHeights.assign((mTilesX + 1) * (mTilesZ + 1), 0.0f);
        for (int z = 0; z <= mTilesZ; ++z)
        {
            for (int x = 0; x <= mTilesX; ++x)
            {
                float h = SmoothNoise2D(x, z);
                h += SmoothNoise2D(x * 2, z * 2) * 0.4f;
                h += SmoothNoise2D(x * 4, z * 4) * 0.2f;
                mHeights[z * (mTilesX + 1) + x] = h * mHeightScale;
            }
        }
    }

    float TerrainObject::GetHeightAt(float x, float z) const
    {
        const float halfWidth = (float)mTilesX * mTileSize * 0.5f;
        const float halfDepth = (float)mTilesZ * mTileSize * 0.5f;
        float worldX = (x + halfWidth) / mTileSize;
        float worldZ = (z + halfDepth) / mTileSize;

        if (worldX < 0.0f) worldX = 0.0f;
        if (worldZ < 0.0f) worldZ = 0.0f;
        if (worldX > mTilesX) worldX = (float)mTilesX;
        if (worldZ > mTilesZ) worldZ = (float)mTilesZ;

        int x0 = (int)worldX;
        int z0 = (int)worldZ;
        if (x0 >= mTilesX) x0 = mTilesX - 1;
        if (z0 >= mTilesZ) z0 = mTilesZ - 1;

        int x1 = x0 + 1;
        int z1 = z0 + 1;

        float fx = worldX - x0;
        float fz = worldZ - z0;

        float h00 = mHeights[z0 * (mTilesX + 1) + x0];
        float h10 = mHeights[z0 * (mTilesX + 1) + x1];
        float h01 = mHeights[z1 * (mTilesX + 1) + x0];
        float h11 = mHeights[z1 * (mTilesX + 1) + x1];

        float h0 = h00 + (h10 - h00) * fx;
        float h1 = h01 + (h11 - h01) * fx;
        return h0 + (h1 - h0) * fz;
    }

    void TerrainObject::Terraform(Scene &scene, float x, float z, float radius, float amount)
    {
        if (radius <= 0.0f || amount == 0.0f) return;
        const float halfWidth = (float)mTilesX * mTileSize * 0.5f;
        const float halfDepth = (float)mTilesZ * mTileSize * 0.5f;
        bool affected = false;
        for (int iz = 0; iz <= mTilesZ; ++iz)
        {
            const float worldZ = (float)iz * mTileSize - halfDepth;
            for (int ix = 0; ix <= mTilesX; ++ix)
            {
                const float worldX = (float)ix * mTileSize - halfWidth;
                const float dx = worldX - x;
                const float dz = worldZ - z;
                const float distance = sqrtf(dx * dx + dz * dz);
                if (distance >= radius) continue;
                const float falloff = 1.0f - distance / radius;
                mHeights[iz * (mTilesX + 1) + ix] += amount * falloff * falloff;
                affected = true;
            }
        }
        // A menor escala possivel continua funcional mesmo entre vertices da malha.
        if (!affected)
        {
            const int ix = std::clamp((int)roundf((x + halfWidth) / mTileSize), 0, mTilesX);
            const int iz = std::clamp((int)roundf((z + halfDepth) / mTileSize), 0, mTilesZ);
            mHeights[iz * (mTilesX + 1) + ix] += amount;
        }
        Rebuild(scene);
    }

    void TerrainObject::SetHeights(Scene &scene, const std::vector<float> &heights)
    {
        if (heights.size() != mHeights.size()) return;
        mHeights = heights;
        Rebuild(scene);
    }

    void TerrainObject::Rebuild(Scene &scene)
    {
        if (!mBodyID.IsInvalid()) scene.GetPhysics().RemoveBody(mBodyID);
        if (mMeshLoaded) UnloadMesh(mRayMesh);
        mRayMesh = {};
        mMeshLoaded = false;
        mVertices.clear();
        mNormals.clear();
        mIndices.clear();
        BuildMesh();
        UploadMeshToJolt(scene);
    }

    void TerrainObject::BuildMesh()
    {
        int vertsPerRow = mTilesX + 1;
        int totalVerts  = vertsPerRow * (mTilesZ + 1);
        int totalTris   = mTilesX * mTilesZ * 2;
        const float halfWidth = (float)mTilesX * mTileSize * 0.5f;
        const float halfDepth = (float)mTilesZ * mTileSize * 0.5f;

        mVertices.resize(totalVerts);
        mNormals.resize(totalVerts, { 0, 1, 0 });
        mIndices.resize(totalTris * 3);

        for (int z = 0; z <= mTilesZ; ++z)
        {
            for (int x = 0; x <= mTilesX; ++x)
            {
                int idx = z * vertsPerRow + x;
                mVertices[idx] = {
                    (float)x * mTileSize - halfWidth,
                    mHeights[idx],
                    (float)z * mTileSize - halfDepth
                };
            }
        }

        unsigned int idxCounter = 0;
        for (int z = 0; z < mTilesZ; ++z)
        {
            for (int x = 0; x < mTilesX; ++x)
            {
                int i00 = z * vertsPerRow + x;
                int i10 = z * vertsPerRow + x + 1;
                int i01 = (z + 1) * vertsPerRow + x;
                int i11 = (z + 1) * vertsPerRow + x + 1;

                // Counter-clockwise from above so rendering and collision normals face up.
                mIndices[idxCounter++] = i00;
                mIndices[idxCounter++] = i01;
                mIndices[idxCounter++] = i10;

                mIndices[idxCounter++] = i01;
                mIndices[idxCounter++] = i11;
                mIndices[idxCounter++] = i10;
            }
        }

        // Calculate per-vertex normals by accumulating face normals.
        std::vector<Vector3> accum(mVertices.size(), { 0,0,0 });
        for (size_t t = 0; t < mIndices.size(); t += 3)
        {
            unsigned int a = mIndices[t + 0];
            unsigned int b = mIndices[t + 1];
            unsigned int c = mIndices[t + 2];

            Vector3 v1 = { mVertices[b].x - mVertices[a].x, mVertices[b].y - mVertices[a].y, mVertices[b].z - mVertices[a].z };
            Vector3 v2 = { mVertices[c].x - mVertices[a].x, mVertices[c].y - mVertices[a].y, mVertices[c].z - mVertices[a].z };
            float nx = v1.y * v2.z - v1.z * v2.y;
            float ny = v1.z * v2.x - v1.x * v2.z;
            float nz = v1.x * v2.y - v1.y * v2.x;
            accum[a].x += nx; accum[a].y += ny; accum[a].z += nz;
            accum[b].x += nx; accum[b].y += ny; accum[b].z += nz;
            accum[c].x += nx; accum[c].y += ny; accum[c].z += nz;
        }
        for (size_t i = 0; i < mNormals.size(); ++i)
        {
            float len = sqrtf(accum[i].x*accum[i].x + accum[i].y*accum[i].y + accum[i].z*accum[i].z);
            if (len > 0.0001f)
            {
                mNormals[i] = { accum[i].x/len, accum[i].y/len, accum[i].z/len };
            }
        }

        // Build the raylib mesh.
        mRayMesh.vertexCount = totalVerts;
        mRayMesh.triangleCount = totalTris;

        mRayMesh.vertices  = (float *)RL_CALLOC(totalVerts * 3, sizeof(float));
        mRayMesh.normals   = (float *)RL_CALLOC(totalVerts * 3, sizeof(float));
        mRayMesh.texcoords = (float *)RL_CALLOC(totalVerts * 2, sizeof(float));
        mRayMesh.indices   = (unsigned short *)RL_CALLOC(totalTris * 3, sizeof(unsigned short));

        for (int z = 0; z <= mTilesZ; ++z)
        {
            for (int x = 0; x <= mTilesX; ++x)
            {
                int i = z * vertsPerRow + x;
                mRayMesh.vertices[i*3 + 0] = mVertices[i].x;
                mRayMesh.vertices[i*3 + 1] = mVertices[i].y;
                mRayMesh.vertices[i*3 + 2] = mVertices[i].z;
                mRayMesh.normals[i*3 + 0] = mNormals[i].x;
                mRayMesh.normals[i*3 + 1] = mNormals[i].y;
                mRayMesh.normals[i*3 + 2] = mNormals[i].z;
                mRayMesh.texcoords[i*2 + 0] = (float)x / mTilesX;
                mRayMesh.texcoords[i*2 + 1] = (float)z / mTilesZ;
            }
        }
        for (int i = 0; i < totalTris * 3; ++i)
            mRayMesh.indices[i] = (unsigned short)mIndices[i];

        UploadMesh(&mRayMesh, false);
        mMeshLoaded = true;
    }

    void TerrainObject::UploadMeshToJolt(Scene &scene)
    {
        std::vector<JPH::Vec3> joltVerts(mVertices.size());
        for (size_t i = 0; i < mVertices.size(); ++i)
        {
            joltVerts[i] = JPH::Vec3(mVertices[i].x, mVertices[i].y, mVertices[i].z);
        }

        std::vector<JPH::IndexedTriangle> joltTris(mIndices.size() / 3);
        for (size_t i = 0; i < joltTris.size(); ++i)
        {
            joltTris[i].mIdx[0] = mIndices[i*3 + 0];
            joltTris[i].mIdx[1] = mIndices[i*3 + 1];
            joltTris[i].mIdx[2] = mIndices[i*3 + 2];
        }

        mBodyID = scene.GetPhysics().AddStaticMesh(joltVerts, joltTris, glm::vec3(0.0f));
    }

    void TerrainObject::Draw(Scene &scene)
    {
        if (!mMeshLoaded) return;
        if (mTerrainShader.id)
        {
            SetShaderValueTexture(mTerrainShader, GetShaderLocation(mTerrainShader, "grassTexture"), mLayerTextures[0]);
            SetShaderValueTexture(mTerrainShader, GetShaderLocation(mTerrainShader, "dirtTexture"), mLayerTextures[1]);
            SetShaderValueTexture(mTerrainShader, GetShaderLocation(mTerrainShader, "sandTexture"), mLayerTextures[2]);
            SetShaderValueTexture(mTerrainShader, GetShaderLocation(mTerrainShader, "pavementTexture"), mLayerTextures[3]);
            SetShaderValue(mTerrainShader, GetShaderLocation(mTerrainShader, "uvScales"), mUVScales, SHADER_UNIFORM_VEC4);
        }
        DrawMesh(mRayMesh, mMaterial, MatrixIdentity());
    }
}
