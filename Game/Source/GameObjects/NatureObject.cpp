#include "GameObjects/NatureObject.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/PlayerObject.h"
#include "Assets/FbxModel.h"
#include "Assets/TextureLoader.h"
#include "Scene.h"
#include "Physics/Physics.h"
#include "raymath.h"
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <system_error>

namespace game
{
    namespace
    {
        const char *NatureNames[] = {
            "Bush_Common", "Bush_Common_Flowers", "Clover_1", "Clover_2",
            "CommonTree_1", "CommonTree_2", "CommonTree_3", "CommonTree_4", "CommonTree_5",
            "DeadTree_1", "DeadTree_2", "DeadTree_3", "DeadTree_4", "DeadTree_5",
            "Fern_1", "Flower_3_Group", "Flower_3_Single", "Flower_4_Group", "Flower_4_Single",
            "Grass_Common_Short", "Grass_Common_Tall", "Grass_Wispy_Short", "Grass_Wispy_Tall",
            "Mushroom_Common", "Mushroom_Laetiporus",
            "Pebble_Round_1", "Pebble_Round_2", "Pebble_Round_3", "Pebble_Round_4", "Pebble_Round_5",
            "Pebble_Square_1", "Pebble_Square_2", "Pebble_Square_3", "Pebble_Square_4", "Pebble_Square_5", "Pebble_Square_6",
            "Petal_1", "Petal_2", "Petal_3", "Petal_4", "Petal_5",
            "Pine_1", "Pine_2", "Pine_3", "Pine_4", "Pine_5",
            "Plant_1", "Plant_1_Big", "Plant_7", "Plant_7_Big",
            "RockPath_Round_Small_1", "RockPath_Round_Small_2", "RockPath_Round_Small_3",
            "RockPath_Round_Thin", "RockPath_Round_Wide",
            "RockPath_Square_Small_1", "RockPath_Square_Small_2", "RockPath_Square_Small_3",
            "RockPath_Square_Thin", "RockPath_Square_Wide",
            "Rock_Medium_1", "Rock_Medium_2", "Rock_Medium_3",
            "TwistedTree_1", "TwistedTree_2", "TwistedTree_3", "TwistedTree_4", "TwistedTree_5"
        };

        bool Contains(const std::string &value, const char *part)
        {
            return value.find(part) != std::string::npos;
        }

        bool IsTextureFile(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
                   extension == ".tga" || extension == ".bmp" || extension == ".dds";
        }

        std::string SafeFileName(std::string name)
        {
            for (char &character : name)
                if (!std::isalnum((unsigned char)character) && character != '-' && character != '_')
                    character = '_';
            return name.empty() ? "modelo" : name;
        }

        std::filesystem::path FindProjectAssetRoot()
        {
            std::error_code error;
            std::filesystem::path cursor = std::filesystem::current_path(error);
            for (int depth = 0; depth < 6; ++depth)
            {
                // is_directory() sets the error_code when the candidate does not
                // exist; clear it per candidate so a missing one does not abort
                // the walk up the directory tree.
                error.clear();
                const std::filesystem::path candidate = cursor / "Game" / "Assets";
                if (std::filesystem::is_directory(candidate, error) && !error) return candidate;
                if (!cursor.has_parent_path()) break;
                cursor = cursor.parent_path();
            }
            return {};
        }
    }

    NatureObject::NatureObject(TerrainObject *terrain) : mTerrain(terrain) {}

    NatureObject::~NatureObject()
    {
        if (mPhysics)
            for (const Instance &instance : mInstances)
                if (!instance.bodyId.IsInvalid()) mPhysics->RemoveBody(instance.bodyId);
        for (Asset &asset : mAssets)
        {
            UnloadAssetModel(asset);
            if (asset.thumbnail.id != 0) UnloadRenderTexture(asset.thumbnail);
        }
    }

    void NatureObject::OnSpawn(Scene &scene)
    {
        mPhysics = &scene.GetPhysics();
        for (const char *name : NatureNames)
        {
            Asset asset;
            asset.name = name;
            asset.path = std::filesystem::path("Assets/Nature") / (asset.name + ".gltf");
            asset.tree = Contains(asset.name, "Tree") || Contains(asset.name, "Pine");
            asset.desiredHeight = asset.tree ? 7.0f :
                Contains(asset.name, "Bush") ? 1.25f :
                Contains(asset.name, "RockPath") ? 0.18f :
                Contains(asset.name, "Rock_Medium") ? 1.1f :
                Contains(asset.name, "Pebble") ? 0.28f :
                Contains(asset.name, "Plant_1_Big") || Contains(asset.name, "Plant_7_Big") ? 1.0f :
                Contains(asset.name, "Mushroom") ? 0.45f : 0.42f;
            mAssets.push_back(asset);
        }
        const int builtInAssetCount = (int)mAssets.size();

        // Garante ao menos uma instancia de cada prop do pacote.
        for (int i = 0; i < builtInAssetCount; ++i)
        {
            const float angle = i * 2.39996323f;
            const float radius = 18.0f + sqrtf((float)i / (float)builtInAssetCount) * 92.0f;
            AddInstance(scene, i, cosf(angle) * radius, sinf(angle) * radius,
                        (float)GetRandomValue(85, 120) / 100.0f, (float)GetRandomValue(0, 359));
        }

        // Densidade de teste para avaliar leitura visual e desempenho.
        for (int i = 0; i < 420; ++i)
        {
            const int assetIndex = GetRandomValue(0, builtInAssetCount - 1);
            const float angle = (float)GetRandomValue(0, 10000) / 10000.0f * 2.0f * PI;
            const float radius = 10.0f + sqrtf((float)GetRandomValue(0, 10000) / 10000.0f) * 112.0f;
            AddInstance(scene, assetIndex, cosf(angle) * radius, sinf(angle) * radius,
                        (float)GetRandomValue(72, 135) / 100.0f, (float)GetRandomValue(0, 359));
        }
        // Imports entram apenas no catalogo; nunca sao espalhados automaticamente pelo mundo.
        LoadImportedAssets();
    }

    void NatureObject::AddInstance(Scene &scene, int assetIndex, float x, float z, float scale, float yaw)
    {
        if (!mTerrain) return;
        Instance instance;
        instance.assetIndex = assetIndex;
        instance.position = {x, mTerrain->GetHeightAt(x, z), z};
        instance.scale = scale;
        instance.yaw = yaw;
        mInstances.push_back(instance);

        const Asset &asset = mAssets[assetIndex];
        if (asset.tree)
        {
            const float height = asset.desiredHeight * scale;
            const float radius = std::max(0.28f, height * 0.055f);
            mInstances.back().bodyId = scene.GetPhysics().AddStaticBox(
                {radius, height * 0.43f, radius},
                {x, instance.position.y + height * 0.43f, z});
        }
    }

    const char *NatureObject::GetAssetName(int index) const
    {
        return index >= 0 && index < (int)mAssets.size() ? mAssets[index].name.c_str() : "";
    }

    void NatureObject::LoadImportedAssets()
    {
        std::filesystem::path root = std::filesystem::path("Assets") / "ImportedProps";
        std::error_code error;
        if (!std::filesystem::is_directory(root, error))
        {
            const std::filesystem::path projectAssets = FindProjectAssetRoot();
            if (!projectAssets.empty()) root = projectAssets / "ImportedProps";
        }
        error.clear();
        if (!std::filesystem::is_directory(root, error)) return;

        std::vector<std::filesystem::path> files;
        for (std::filesystem::recursive_directory_iterator iterator(root, error), end;
             !error && iterator != end; iterator.increment(error))
        {
            if (!iterator->is_regular_file()) continue;
            std::string extension = iterator->path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension == ".fbx") files.push_back(iterator->path());
        }
        std::sort(files.begin(), files.end());
        for (const std::filesystem::path &file : files)
        {
            Asset asset;
            asset.name = std::string("Importado: ") + file.stem().string();
            asset.path = file;
            asset.imported = true;
            mAssets.push_back(std::move(asset));
        }
    }

    int NatureObject::ImportFBX(const std::filesystem::path &sourcePath)
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(sourcePath, error)) return -1;
        std::string extension = sourcePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension != ".fbx") return -1;

        const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const std::string folderName = std::to_string(stamp) + "_" + SafeFileName(sourcePath.stem().string());
        const std::filesystem::path runtimeRoot = std::filesystem::path("Assets") / "ImportedProps" / folderName;
        const std::filesystem::path projectAssets = FindProjectAssetRoot();
        const std::filesystem::path projectRoot = projectAssets.empty() ? std::filesystem::path{} :
            projectAssets / "ImportedProps" / folderName;

        // Side textures are best-effort: the FBX geometry is what makes the
        // import valid, so a missing/unreadable texture must not abort it.
        auto copyPayload = [&](const std::filesystem::path &destinationRoot)
        {
            if (destinationRoot.empty()) return true;
            error.clear();
            std::filesystem::create_directories(destinationRoot, error);
            if (error) return false;
            std::filesystem::copy_file(sourcePath, destinationRoot / sourcePath.filename(),
                std::filesystem::copy_options::overwrite_existing, error);
            if (error) return false;
            for (const std::filesystem::directory_entry &entry :
                 std::filesystem::directory_iterator(sourcePath.parent_path(), error))
            {
                if (error) break;
                if (entry.is_regular_file() && IsTextureFile(entry.path()))
                    std::filesystem::copy_file(entry.path(), destinationRoot / entry.path().filename(),
                        std::filesystem::copy_options::overwrite_existing, error);
            }
            // A missing "Textures" subfolder is normal for many packs and must
            // not fail the import; side textures are best-effort. Only the FBX
            // copy above is required.
            error.clear();
            const std::filesystem::path textureFolder = sourcePath.parent_path() / "Textures";
            if (std::filesystem::is_directory(textureFolder, error))
                std::filesystem::copy(textureFolder, destinationRoot / "Textures",
                    std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::overwrite_existing, error);
            error.clear();
            return true;
        };

        if (!copyPayload(runtimeRoot)) return -1;
        if (!projectRoot.empty())
        {
            error.clear();
            const std::filesystem::path canonProject = std::filesystem::weakly_canonical(projectRoot, error);
            error.clear();
            const std::filesystem::path canonRuntime = std::filesystem::weakly_canonical(runtimeRoot, error);
            if (canonProject != canonRuntime && !copyPayload(projectRoot)) return -1;
        }

        Asset asset;
        asset.name = std::string("Importado: ") + sourcePath.stem().string();
        asset.path = runtimeRoot / sourcePath.filename();
        asset.imported = true;
        LoadAsset(asset);
        if (!asset.loaded) return -1;
        mAssets.push_back(std::move(asset));
        return (int)mAssets.size() - 1;
    }

    Texture2D NatureObject::GetThumbnail(int index)
    {
        if (index < 0 || index >= (int)mAssets.size()) return {};
        Asset &asset = mAssets[index];
        if (asset.thumbnail.id != 0) return asset.thumbnail.texture;
        const bool wasLoaded = asset.loaded;
        LoadAsset(asset);
        if (!asset.loaded) return {};

        asset.thumbnail = LoadRenderTexture(96, 96);
        const float height = asset.desiredHeight;
        Camera3D camera = {};
        camera.position = {height * 1.15f, height * 0.78f, height * 1.15f};
        camera.target = {0.0f, height * 0.42f, 0.0f};
        camera.up = {0,1,0};
        camera.fovy = 42.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        const float scale = height / asset.sourceHeight;
        BeginTextureMode(asset.thumbnail);
            ClearBackground({24, 29, 36, 255});
            BeginMode3D(camera);
                DrawModelEx(asset.model, {0,0,0}, {0,1,0}, 25.0f,
                            {scale, scale, scale}, WHITE);
            EndMode3D();
        EndTextureMode();
        if (!wasLoaded)
        {
            UnloadAssetModel(asset);
        }
        return asset.thumbnail.texture;
    }

    unsigned int NatureObject::AddEditorInstance(Scene &scene, int assetIndex, const glm::vec3 &position,
                                                  const glm::vec3 &scale, const glm::vec3 &rotation,
                                                  bool collisionEnabled)
    {
        if (assetIndex < 0 || assetIndex >= (int)mAssets.size()) return 0;
        AddInstance(scene, assetIndex, position.x, position.z, 1.0f, rotation.y);
        mInstances.back().editorPlaced = true;
        mInstances.back().editorId = mNextEditorId++;
        mInstances.back().position = position;
        mInstances.back().editorScale = scale;
        mInstances.back().editorRotation = rotation;
        mInstances.back().collisionEnabled = collisionEnabled;
        TransformEditorInstance(mInstances.back().editorId, position, scale, rotation);
        return mInstances.back().editorId;
    }

    bool NatureObject::TransformEditorInstance(unsigned int id, const glm::vec3 &position,
                                                const glm::vec3 &scale, const glm::vec3 &rotation)
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        if (found == mInstances.end()) return false;
        found->position = position;
        found->editorScale = scale;
        found->editorRotation = rotation;
        found->yaw = rotation.y;
        if (mPhysics && !found->bodyId.IsInvalid()) mPhysics->RemoveBody(found->bodyId);
        found->bodyId = JPH::BodyID();
        if (found->collisionEnabled) RebuildEditorCollision(*found);
        return true;
    }

    bool NatureObject::SetEditorCollision(unsigned int id, bool enabled)
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        if (found == mInstances.end()) return false;
        if (found->collisionEnabled == enabled) return true;
        found->collisionEnabled = enabled;
        if (mPhysics && !found->bodyId.IsInvalid()) mPhysics->RemoveBody(found->bodyId);
        found->bodyId = JPH::BodyID();
        if (enabled) RebuildEditorCollision(*found);
        return true;
    }

    bool NatureObject::GetEditorCollision(unsigned int id) const
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        return found != mInstances.end() && found->collisionEnabled;
    }

    bool NatureObject::SetEditorCollisionBox(unsigned int id, const glm::vec3 &size,
                                             const glm::vec3 &offset)
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        if (found == mInstances.end()) return false;
        found->collisionBoxSize = size;
        found->collisionBoxOffset = offset;
        found->collisionBoxCustom = (size != glm::vec3(0.0f));
        if (!found->collisionEnabled) return true;
        if (mPhysics && !found->bodyId.IsInvalid()) mPhysics->RemoveBody(found->bodyId);
        found->bodyId = JPH::BodyID();
        RebuildEditorCollision(*found);
        return true;
    }

    bool NatureObject::GetEditorCollisionBox(unsigned int id, glm::vec3 &size, glm::vec3 &offset) const
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        if (found == mInstances.end()) return false;
        size = found->collisionBoxSize;
        offset = found->collisionBoxOffset;
        return true;
    }

    void NatureObject::RebuildEditorCollision(Instance &instance)
    {
        if (!mPhysics || instance.assetIndex < 0 || instance.assetIndex >= (int)mAssets.size()) return;
        Asset &asset = mAssets[instance.assetIndex];
        LoadAsset(asset);
        if (!asset.loaded) return;

        const float baseScale = asset.desiredHeight * instance.scale / asset.sourceHeight;
        Matrix localTransform = MatrixScale(baseScale * instance.editorScale.x,
            baseScale * instance.editorScale.y, baseScale * instance.editorScale.z);
        localTransform = MatrixMultiply(asset.model.transform, localTransform);

        // Auto-fit: caixa calculada a partir dos limites do mesh (sem rotacao,
        // nos eixos locais do objeto). Quando o usuario customiza a caixa
        // (collisionBoxCustom), o tamanho/offset salvos sao mantidos.
        if (!instance.collisionBoxCustom)
        {
            Vector3 min = {1e30f, 1e30f, 1e30f}, max = {-1e30f, -1e30f, -1e30f};
            bool any = false;
            for (int meshIndex = 0; meshIndex < asset.model.meshCount; ++meshIndex)
            {
                const Mesh &mesh = asset.model.meshes[meshIndex];
                const float *positions = mesh.vertices ? mesh.vertices : mesh.animVertices;
                if (!positions || mesh.vertexCount < 3) continue;
                for (int vertex = 0; vertex < mesh.vertexCount; ++vertex)
                {
                    const Vector3 transformed = Vector3Transform({positions[vertex * 3],
                        positions[vertex * 3 + 1], positions[vertex * 3 + 2]}, localTransform);
                    min.x = std::min(min.x, transformed.x); min.y = std::min(min.y, transformed.y);
                    min.z = std::min(min.z, transformed.z);
                    max.x = std::max(max.x, transformed.x); max.y = std::max(max.y, transformed.y);
                    max.z = std::max(max.z, transformed.z);
                    any = true;
                }
            }
            if (!any) return;
            instance.collisionBoxSize = glm::vec3(max.x - min.x, max.y - min.y, max.z - min.z);
            instance.collisionBoxOffset = glm::vec3((min.x + max.x) * 0.5f,
                                                    (min.y + max.y) * 0.5f,
                                                    (min.z + max.z) * 0.5f);
        }

        const glm::vec3 half = instance.collisionBoxSize * 0.5f;
        if (half.x <= 0.0f || half.y <= 0.0f || half.z <= 0.0f) return;
        const Vector3 rotRad = {instance.editorRotation.x * DEG2RAD,
                                instance.editorRotation.y * DEG2RAD,
                                instance.editorRotation.z * DEG2RAD};
        const Vector3 offsetWorld = Vector3Transform({instance.collisionBoxOffset.x,
            instance.collisionBoxOffset.y, instance.collisionBoxOffset.z},
            MatrixRotateXYZ(rotRad));
        instance.bodyId = mPhysics->AddStaticBoxRotated(
            {half.x, half.y, half.z},
            {instance.position.x + offsetWorld.x, instance.position.y + offsetWorld.y,
             instance.position.z + offsetWorld.z},
            {rotRad.x, rotRad.y, rotRad.z});
    }

    bool NatureObject::RemoveEditorInstance(unsigned int id)
    {
        auto found = std::find_if(mInstances.begin(), mInstances.end(),
            [id](const Instance &instance) { return instance.editorPlaced && instance.editorId == id; });
        if (found == mInstances.end()) return false;
        if (mPhysics && !found->bodyId.IsInvalid()) mPhysics->RemoveBody(found->bodyId);
        mInstances.erase(found);
        return true;
    }

    void NatureObject::ClearEditorInstances()
    {
        for (auto it = mInstances.begin(); it != mInstances.end();)
        {
            if (!it->editorPlaced) { ++it; continue; }
            if (mPhysics && !it->bodyId.IsInvalid()) mPhysics->RemoveBody(it->bodyId);
            it = mInstances.erase(it);
        }
    }

    bool NatureObject::RemoveNearestEditorInstance(const glm::vec3 &position, float radius)
    {
        float closestSq = radius * radius;
        auto closest = mInstances.end();
        for (auto it = mInstances.begin(); it != mInstances.end(); ++it)
        {
            if (!it->editorPlaced) continue;
            const glm::vec2 delta(it->position.x - position.x, it->position.z - position.z);
            const float distanceSq = glm::dot(delta, delta);
            if (distanceSq < closestSq) { closestSq = distanceSq; closest = it; }
        }
        if (closest == mInstances.end()) return false;
        if (mPhysics && !closest->bodyId.IsInvalid()) mPhysics->RemoveBody(closest->bodyId);
        mInstances.erase(closest);
        return true;
    }

    void NatureObject::DrawPreview(int assetIndex, const glm::vec3 &position,
                                   const glm::vec3 &scale, const glm::vec3 &rotation)
    {
        if (assetIndex < 0 || assetIndex >= (int)mAssets.size()) return;
        Asset &asset = mAssets[assetIndex];
        LoadAsset(asset);
        if (!asset.loaded) return;
        const float baseScale = asset.desiredHeight / asset.sourceHeight;
        Matrix transform = MatrixScale(baseScale * scale.x, baseScale * scale.y, baseScale * scale.z);
        transform = MatrixMultiply(transform, MatrixRotateXYZ({rotation.x * DEG2RAD,
            rotation.y * DEG2RAD, rotation.z * DEG2RAD}));
        transform = MatrixMultiply(transform, MatrixTranslate(position.x, position.y, position.z));
        transform = MatrixMultiply(asset.model.transform, transform);
        for (int mesh = 0; mesh < asset.model.meshCount; ++mesh)
        {
            Material material = asset.model.materials[asset.model.meshMaterial[mesh]];
            material.maps[MATERIAL_MAP_ALBEDO].color = ColorAlpha(WHITE, 0.58f);
            DrawMesh(asset.model.meshes[mesh], material, transform);
        }
    }

    void NatureObject::LoadAsset(Asset &asset, float polygonRatio)
    {
        if (asset.loaded) return;
        std::string path = asset.path.string();
        if (!FileExists(path.c_str()) && !asset.imported)
            path = std::string("Game/Assets/Kit assets/Stylized Nature MegaKit[Standard]/glTF/") + asset.name + ".gltf";

        std::string extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension == ".fbx")
        {
            std::string error;
            asset.model = {};
            asset.importedTextures.clear();
            asset.loaded = LoadStaticFbxModel(path, asset.model, asset.importedTextures, error);
            if (!asset.loaded)
                TraceLog(LOG_WARNING, "NATURE: Could not load %s (%s)", path.c_str(), error.c_str());
        }
        else
        {
            asset.model = LoadModel(path.c_str());
            asset.loaded = asset.model.meshCount > 0;
            if (!asset.loaded)
                TraceLog(LOG_WARNING, "NATURE: Could not load %s", path.c_str());
        }
        if (asset.loaded)
        {
            if (polygonRatio < 0.999f)
                ReduceModelTriangles(asset.model, polygonRatio);
            const BoundingBox bounds = GetModelBoundingBox(asset.model);
            asset.sourceHeight = std::max(0.001f, bounds.max.y - bounds.min.y);
            // Props importados preservam o tamanho nativo do arquivo original.
            if (asset.imported) asset.desiredHeight = asset.sourceHeight;
        }
    }

    void NatureObject::UnloadAssetModel(Asset &asset)
    {
        if (!asset.loaded) return;
        UnloadModel(asset.model);
        for (Texture2D &texture : asset.importedTextures)
            if (texture.id != 0) TextureRelease(texture);
        asset.importedTextures.clear();
        asset.model = {};
        asset.loaded = false;
    }

    void NatureObject::Update(Scene &scene, float)
    {
        const Vector3 cameraPosition = scene.GetCamera().position;
        const glm::vec3 playerPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z);
        const bool reducedProfile = scene.GetSettings().crtMode || scene.GetSettings().synthMode;
        const float loadRadius = reducedProfile
            ? 14.0f : (scene.GetSettings().retroMode ? 34.0f : 42.0f);
        const float unloadRadius = reducedProfile
            ? 21.0f : (scene.GetSettings().retroMode ? 42.0f : 52.0f);
        bool loadedThisFrame = false;
        int loadedAssetCount = 0;
        for (const Asset &asset : mAssets) if (asset.loaded) ++loadedAssetCount;
        const int detailedAssetBudget = scene.GetSettings().synthMode ? 2 : INT_MAX;
        for (int assetIndex = 0; assetIndex < (int)mAssets.size(); ++assetIndex)
        {
            float closestSq = FLT_MAX;
            for (const Instance &instance : mInstances)
            {
                if (instance.assetIndex != assetIndex) continue;
                const glm::vec2 delta(instance.position.x - playerPosition.x,
                                      instance.position.z - playerPosition.z);
                closestSq = std::min(closestSq, glm::dot(delta, delta));
            }
            Asset &asset = mAssets[assetIndex];
            if (closestSq <= loadRadius * loadRadius && !asset.loaded && !loadedThisFrame &&
                loadedAssetCount < detailedAssetBudget)
            {
                LoadAsset(asset, scene.GetSettings().synthMode
                    ? scene.GetSettings().synthScenePolygonRatio : 1.0f);
                loadedThisFrame = asset.loaded;
                if (asset.loaded) ++loadedAssetCount;
            }
            else if (asset.loaded && closestSq > unloadRadius * unloadRadius)
            {
                UnloadAssetModel(asset);
                --loadedAssetCount;
            }
        }
    }

    void NatureObject::Draw(Scene &scene)
    {
        const Vector3 cameraPosition = scene.GetCamera().position;
        const glm::vec3 playerPosition(cameraPosition.x, cameraPosition.y, cameraPosition.z);
        const bool reducedProfile = scene.GetSettings().crtMode || scene.GetSettings().synthMode;
        const float drawRadius = reducedProfile
            ? 29.0f : (scene.GetSettings().retroMode ? 38.0f : 45.0f);
        for (const Instance &instance : mInstances)
        {
            const glm::vec2 delta(instance.position.x - playerPosition.x,
                                  instance.position.z - playerPosition.z);
            if (glm::dot(delta, delta) > drawRadius * drawRadius) continue;
            const Asset &asset = mAssets[instance.assetIndex];
            if (!asset.loaded)
            {
                // Representacao procedural barata enquanto o modelo detalhado
                // esta fora do raio de memoria. Mantem a leitura do cenario e
                // combina com o visual low-poly do SeedSynth.
                if (!reducedProfile) continue;
                const Vector3 base = {instance.position.x, instance.position.y, instance.position.z};
                const float height = asset.desiredHeight * instance.scale;
                if (asset.tree)
                {
                    const float trunkHeight = height * 0.42f;
                    DrawCylinderEx(base, {base.x, base.y + trunkHeight, base.z},
                                   height * 0.035f, height * 0.055f, 5,
                                   Color{91, 66, 45, 255});
                    DrawCylinderEx({base.x, base.y + trunkHeight * 0.72f, base.z},
                                   {base.x, base.y + height, base.z},
                                   height * 0.19f, 0.0f, 5,
                                   Color{55, 112, 69, 255});
                }
                else if (Contains(asset.name, "Rock") || Contains(asset.name, "Pebble"))
                {
                    DrawCube({base.x, base.y + height * 0.45f, base.z},
                             height * 1.15f, height * 0.9f, height,
                             Color{105, 112, 114, 255});
                }
                else
                {
                    DrawCylinderEx(base, {base.x, base.y + height, base.z},
                                   height * 0.32f, height * 0.04f, 4,
                                   Color{66, 132, 72, 255});
                }
                continue;
            }
            const float baseScale = asset.desiredHeight * instance.scale / asset.sourceHeight;
            if (!instance.editorPlaced)
            {
                DrawModelEx(asset.model, {instance.position.x, instance.position.y, instance.position.z},
                            {0.0f, 1.0f, 0.0f}, instance.yaw,
                            {baseScale, baseScale, baseScale}, WHITE);
                continue;
            }
            Matrix transform = MatrixScale(baseScale * instance.editorScale.x,
                                           baseScale * instance.editorScale.y,
                                           baseScale * instance.editorScale.z);
            transform = MatrixMultiply(transform, MatrixRotateXYZ({instance.editorRotation.x * DEG2RAD,
                instance.editorRotation.y * DEG2RAD, instance.editorRotation.z * DEG2RAD}));
            transform = MatrixMultiply(transform, MatrixTranslate(instance.position.x,
                instance.position.y, instance.position.z));
            transform = MatrixMultiply(asset.model.transform, transform);
            for (int mesh = 0; mesh < asset.model.meshCount; ++mesh)
                DrawMesh(asset.model.meshes[mesh],
                         asset.model.materials[asset.model.meshMaterial[mesh]], transform);
        }
    }
}
