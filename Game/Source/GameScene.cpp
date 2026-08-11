#include "GameScene.h"
#include "GameObjects/TerrainObject.h"
#include "GameObjects/PlayerObject.h"
#include "GameObjects/PickupObject.h"
#include "GameObjects/EnemyObject.h"
#include "GameObjects/NatureObject.h"
#include "Assets/FbxModel.h"
#include "Assets/TextureLoader.h"
#include "CameraController.h"
#include "UI/GameUI.h"
#include "UI/LoadingScreen.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <array>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <cctype>
#if !defined(NDEBUG)
#include "imgui.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Rename the Win32 symbols that collide with raylib before including
// windows.h, then undefine them so raylib's own names (DrawText, Rectangle,
// CloseWindow, ShowCursor, ...) keep working. Same pattern raylib uses in
// rcore_desktop_win32.c.
#define CloseWindow CloseWindowWin32
#define Rectangle RectangleWin32
#define ShowCursor ShowCursorWin32
#define DrawTextA DrawTextAWin32
#define DrawTextW DrawTextWin32
#define DrawTextExA DrawTextExAWin32
#define DrawTextExW DrawTextExWin32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#undef CloseWindow
#undef Rectangle
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextA
#undef DrawTextW
#undef DrawTextEx
#undef DrawTextExA
#undef DrawTextExW
#endif
#endif

#if !defined(NDEBUG)
    // ---- Campos numericos de dev (F2 e editor) ----
    // Um campo numerico com tres formas de alterar: arrastar (Shift deixa o
    // passo fino), digitar (duplo clique ou Ctrl+clique no valor) e rolagem
    // do mouse. O Shift embutido do ImGui deixa o arrasto 10x mais rapido,
    // entao usamos NoSpeedTweaks e aplicamos a nossa propria suavizacao.
    static bool ImGuiValueEditFloat(const char *label, float *value, float minV, float maxV,
                                    float step, const char *format)
    {
        ImGuiIO &io = ImGui::GetIO();
        const float fine = io.KeyShift ? 0.1f : 1.0f;
        bool changed = false;
        if (ImGui::DragFloat(label, value, step * fine, minV, maxV, format,
                             ImGuiSliderFlags_NoSpeedTweaks | ImGuiSliderFlags_AlwaysClamp))
            changed = true;
        if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        {
            *value = std::clamp(*value + io.MouseWheel * step * fine, minV, maxV);
            changed = true;
        }
        return changed;
    }

    static bool ImGuiValueEditInt(const char *label, int *value, int minV, int maxV,
                                  int step, const char *format)
    {
        ImGuiIO &io = ImGui::GetIO();
        const int fine = io.KeyShift ? 1 : step;
        bool changed = false;
        if (ImGui::DragInt(label, value, (float)fine, minV, maxV, format,
                           ImGuiSliderFlags_NoSpeedTweaks | ImGuiSliderFlags_AlwaysClamp))
            changed = true;
        if (ImGui::IsItemHovered() && io.MouseWheel != 0.0f)
        {
            *value = std::clamp(*value + (int)(io.MouseWheel * (float)fine), minV, maxV);
            changed = true;
        }
        return changed;
    }

    // Linha com 3 campos (X, Y, Z) com os mesmos recursos: arrastar, digitar,
    // rolagem e Shift suavizando.
    static bool ImGuiValueEditVec3(const char *label, float *v, float minV, float maxV,
                                   float step, const char *format)
    {
        bool changed = false;
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::PushID(label);
        const char *axes[3] = { "##x", "##y", "##z" };
        for (int i = 0; i < 3; ++i)
        {
            if (i) ImGui::SameLine();
            ImGui::SetNextItemWidth(66.0f);
            changed |= ImGuiValueEditFloat(axes[i], &v[i], minV, maxV, step, format);
        }
        ImGui::PopID();
        return changed;
    }
#endif

namespace game
{
    static int ItemIconIndex(ItemType type)
    {
        switch (type)
        {
        case ItemType::HealthPotion: return 0;
        case ItemType::IronAxe: return 1;
        case ItemType::LeatherArmor: return 2;
        case ItemType::RangerArmor: return 3;
        default: return -1;
        }
    }

    GameScene::GameScene(Settings &settings) : Scene(settings)
    {
    }

    GameScene::~GameScene() { Shutdown(); }

    void GameScene::Init()
    {
        LoadingScreen loading;
        loading.Draw(0.00f, "Inicializando...");

        mPaused = false;
        mPauseSettingsOpen = false;
        mInventoryOpen = false;
        mInventoryDrag = -1;
        mToastTime = 0.0f;

        mPhysics.Init();
        loading.Draw(0.08f, "Física");

        TerrainObject *terrain = SpawnObject<TerrainObject>(128, 128, 2.0f);
        mTerrain = terrain;
        loading.Draw(0.30f, "Gerando terreno...");

        mNature = SpawnObject<NatureObject>(terrain);
        loading.Draw(0.62f, "Carregando natureza...");
#if !defined(NDEBUG)
        mEditorBaseHeights = terrain->GetHeights();
        mEditorBaseStencil = terrain->GetStencilMap();
        for (int i = 0; i < 4; ++i) mEditorBaseUVScales[i] = terrain->GetUVScales()[i];
#endif
        float h = terrain->GetHeightAt(0.0f, 0.0f);
        PlayerObject *player = SpawnObject<PlayerObject>(glm::vec3(0.0f, h + 5.0f, 0.0f));
        mPlayer = player;
        loading.Draw(0.70f, "Personagem");

        mCameraController.Init(*this);
        mCameraController.Update(*this, 0.0f);
        mCamera = mCameraController.GetCamera();

        ApplySettings();
        loading.Draw(0.76f, "Interface");

        const ItemType iconTypes[] = { ItemType::HealthPotion, ItemType::IronAxe,
                                       ItemType::LeatherArmor, ItemType::RangerArmor };
        for (int i = 0; i < 4; ++i)
        {
            mItemIcons[i] = TextureAcquire(GetItemIconFile(iconTypes[i]));
            if (mItemIcons[i].id == 0)
            {
                const char *file = GetItemIconFile(iconTypes[i]);
                std::string fallback = std::string("Game/Assets/UltimateRPg Items Pack/Icons/") +
                    std::string(file).substr(std::string(file).find_last_of('/') + 1);
                mItemIcons[i] = TextureAcquire(fallback);
            }
        }
        loading.Draw(0.80f, "Itens");

        mPickups.push_back(SpawnObject<PickupObject>(glm::vec3(3.2f, terrain->GetHeightAt(3.2f, 1.2f), 1.2f), ItemType::HealthPotion, 3));
        mPickups.push_back(SpawnObject<PickupObject>(glm::vec3(-3.0f, terrain->GetHeightAt(-3.0f, 2.2f), 2.2f), ItemType::IronAxe));
        mPickups.push_back(SpawnObject<PickupObject>(glm::vec3(1.3f, terrain->GetHeightAt(1.3f, -3.7f), -3.7f), ItemType::LeatherArmor));
        mPickups.push_back(SpawnObject<PickupObject>(glm::vec3(-4.5f, terrain->GetHeightAt(-4.5f, -2.5f), -2.5f), ItemType::RangerArmor));
        loading.Draw(0.86f, "Itens do mundo");

        struct EnemyArchetype { EnemyCategory category; const char *model; const char *name; };
        const std::array<EnemyArchetype, 12> archetypes = {{
            {EnemyCategory::Blob, "GreenBlob", "Blob Verde"}, {EnemyCategory::Blob, "Cactoro", "Cactoro"},
            {EnemyCategory::Blob, "Orc", "Orc Pequeno"}, {EnemyCategory::Blob, "Yeti", "Yeti"},
            {EnemyCategory::Big, "BlueDemon", "Demonio Azul"}, {EnemyCategory::Big, "Dino", "Dinossauro"},
            {EnemyCategory::Big, "MushroomKing", "Rei Cogumelo"}, {EnemyCategory::Big, "Orc_Skull", "Orc Caveira"},
            {EnemyCategory::Flying, "Dragon", "Dragao"}, {EnemyCategory::Flying, "Ghost", "Fantasma"},
            {EnemyCategory::Flying, "Armabee", "Armabelha"}, {EnemyCategory::Flying, "Squidle", "Lula Voadora"}
        }};
        // Duzentas instancias leves; os FBX sao carregados quando entram na area do jogador.
        for (int i = 0; i < 200; ++i)
        {
            const float angle = (float)i * 2.39996323f;
            const float radius = 12.0f + sqrtf((float)i / 199.0f) * 100.0f;
            const float x = cosf(angle) * radius;
            const float z = sinf(angle) * radius;
            const EnemyArchetype &archetype = archetypes[(size_t)GetRandomValue(0, (int)archetypes.size() - 1)];
            EnemyConfig config;
            config.category = archetype.category;
            config.modelName = archetype.model;
            config.displayName = archetype.name;
            config.level = GetRandomValue(1, 12);
            config.maxHealth = 35.0f + config.level * 13.0f;
            config.attack = 5.0f + config.level * 1.8f;
            config.defense = 0.6f + config.level * 0.55f;
            config.patrolRadius = (float)GetRandomValue(5, 13);
            config.detectionRadius = (float)GetRandomValue(10, 17);
            config.aggressionChance = 0.20f + (float)GetRandomValue(0, 55) / 100.0f;
            config.moneyReward = 3 + config.level * 2;
            config.experienceReward = 12.0f + config.level * 7.0f;
            const float ground = terrain->GetHeightAt(x, z);
            mEnemies.push_back(SpawnObject<EnemyObject>(glm::vec3(x, ground, z), config, ground));
        }
        loading.Draw(0.94f, "Inimigos");
#if !defined(NDEBUG)
        LoadLevel();
        loading.Draw(0.99f, "Carregando nível...");
#endif
        DisableCursor();
        loading.Finish();
    }

    void GameScene::Shutdown()
    {
        for (GameObject *obj : mObjects)
            delete obj;
        mObjects.clear();
        mPlayer = nullptr;
        mTerrain = nullptr;
        mNature = nullptr;
        mPickups.clear();
        mEnemies.clear();
        mBloodParticles.clear();
        mDamageNumbers.clear();

        for (Texture2D &icon : mItemIcons)
        {
            if (icon.id != 0) TextureRelease(icon);
            icon = {};
        }
#if !defined(NDEBUG)
        for (RenderTexture2D &thumbnail : mEnemyEditorThumbnails)
        {
            if (thumbnail.id != 0) UnloadRenderTexture(thumbnail);
            thumbnail = {};
        }
#endif

        mPhysics.Shutdown();
    }

    void GameScene::ApplySettings()
    {
        mCameraController.SetMouseSensitivity(mSettings.mouseSensitivity);
        if (mPlayer)
            mPlayer->ApplyCharacterAppearance(mSettings.characterGender, mSettings.skinHue,
                                              mSettings.skinSaturation,
                                              mSettings.characterLightIntensity);
    }

    void GameScene::Update(float deltaTime)
    {
#if !defined(NDEBUG)
        if (IsKeyPressed(KEY_F1))
        {
            ToggleLevelEditor();
            return;
        }
        if (mEditorActive)
        {
            UpdateLevelEditor(deltaTime);
            return;
        }
        if (IsKeyPressed(KEY_F2))
        {
            mDevToolsOpen = !mDevToolsOpen;
            mPaused = mDevToolsOpen;
            mPauseSettingsOpen = false;
            if (mPaused) EnableCursor();
            else DisableCursor();
            return;
        }
#endif
        if (mInventoryOpen && IsKeyPressed(KEY_ESCAPE))
        {
            ToggleInventory();
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (mDevToolsOpen)
            {
                mDevToolsOpen = false;
                mPaused = false;
                DisableCursor();
                return;
            }
            mPaused = !mPaused;
            mPauseSettingsOpen = false;
            if (mPaused) EnableCursor();
            else DisableCursor();
            return;
        }

        if (mPaused) return;

        if (IsKeyPressed(KEY_I))
        {
            ToggleInventory();
            return;
        }

        if (mInventoryOpen)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                const float scale = ui::UiScale();
                const float width = 700.0f * scale;
                const float height = 480.0f * scale;
                const Rectangle panel = { (GetScreenWidth() - width) * 0.5f,
                                          (GetScreenHeight() - height) * 0.5f, width, height };
                if (!CheckCollisionPointRec(GetMousePosition(), panel)) ToggleInventory();
            }
            return;
        }

        if (IsKeyPressed(KEY_Q) && mPlayer && mPlayer->UseEquippedPotion())
        {
            std::snprintf(mToast, sizeof(mToast), "Poção usada: +38 de vida");
            mToastTime = 1.8f;
        }

#if !defined(NDEBUG)
        if (IsKeyPressed(KEY_F3)) mShowDebugDraw = !mShowDebugDraw;
#endif

        // Avoid a large physics jump after loading, resizing or losing focus.
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        mPhysics.Step(deltaTime);
        for (GameObject *obj : mObjects)
            obj->Update(*this, deltaTime);

        UpdateCombatEffects(deltaTime);
        for (auto enemyIt = mEnemies.begin(); enemyIt != mEnemies.end();)
        {
            EnemyObject *enemy = *enemyIt;
            if (!enemy || !enemy->IsExpired()) { ++enemyIt; continue; }
            auto objectIt = std::find(mObjects.begin(), mObjects.end(), enemy);
            if (objectIt != mObjects.end()) mObjects.erase(objectIt);
            delete enemy;
            enemyIt = mEnemies.erase(enemyIt);
        }

        if (mToastTime > 0.0f) mToastTime -= deltaTime;

        PickupObject *nearby = GetNearestPickup(1.7f);
        if (nearby && IsKeyPressed(KEY_E))
        {
            if (mPlayer->GetInventory().Add(nearby->GetItemType(), nearby->GetCount()))
            {
                const int experience = nearby->GetItemType() == ItemType::HealthPotion ? 8 : 20;
                mPlayer->AddExperience((float)experience);
                nearby->Collect();
                std::snprintf(mToast, sizeof(mToast), "%s coletado (+%d XP)",
                              GetItemName(nearby->GetItemType()), experience);
                mToastTime = 2.4f;
                ToggleInventory();
            }
            else
            {
                std::snprintf(mToast, sizeof(mToast), "Inventário cheio");
                mToastTime = 2.0f;
            }
        }

        if (IsKeyPressed(KEY_P) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
        {
            mCameraController.CycleView();
        }

        mCameraController.Update(*this, deltaTime);
        mCamera = mCameraController.GetCamera();
    }

    void GameScene::Draw()
    {
        // Day-sky backdrop. It is drawn in screen space before the 3D world so
        // areas without geometry no longer expose the application's dark clear.
        // Mesmo matiz azul, com luminancia mais alta (exposicao).
        DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                               { 120, 180, 245, 255 }, { 220, 240, 255, 255 });

        const Camera3D &drawCamera =
#if !defined(NDEBUG)
            mEditorActive ? mEditorCamera :
#endif
            mCamera;
        BeginMode3D(drawCamera);
            for (GameObject *obj : mObjects)
                obj->Draw(*this);

            for (const BloodParticle &particle : mBloodParticles)
                DrawSphere({particle.position.x, particle.position.y, particle.position.z},
                           0.025f + particle.life * 0.018f, {178, 18, 28, 235});
#if !defined(NDEBUG)
            if (mEditorActive && mEditorHitValid)
            {
                if (mEditorBrushEnabled)
                    DrawCircle3D({mEditorHit.x, mEditorHit.y + 0.04f, mEditorHit.z},
                                 mEditorBrushRadius, {0,1,0}, 90.0f, ColorAlpha(SKYBLUE, 0.8f));
                else if (mEditorTool == 1 && mNature)
                    mNature->DrawPreview(mEditorProp, mEditorHit,
                        glm::vec3(mEditorPropPlacementScale),
                        glm::vec3(0.0f, mEditorPropPlacementRotation, 0.0f));
                else
                    DrawSphere({mEditorHit.x, mEditorHit.y + 0.25f, mEditorHit.z}, 0.25f,
                               mEditorTool == 2 ? RED : GOLD);
            }
            if (mEditorActive)
            {
                DrawEditorGizmo();
                for (EnemyObject *enemy : mEnemies)
                    if (enemy)
                    {
                        const glm::vec3 &spawn = enemy->GetSpawnPosition();
                        DrawCylinderEx({spawn.x, spawn.y + 0.35f, spawn.z},
                                       {spawn.x, spawn.y + 1.05f, spawn.z},
                                       0.16f, 0.0f, 8, ColorAlpha(RED, 0.72f));
                    }
            }
#endif

#if !defined(NDEBUG)
            if (!mPaused && mShowDebugDraw) DrawDebug();
#endif
        EndMode3D();

#if !defined(NDEBUG)
        if (!mEditorActive)
#endif
        {
            for (EnemyObject *enemy : mEnemies)
                if (enemy) enemy->DrawOverlay(*this);

            for (const DamageNumber &number : mDamageNumbers)
            {
                const Vector3 numberPos = {number.position.x, number.position.y, number.position.z};
                // Mesmo cuidado da barra de vida: descartar numeros atras da camera.
                const Vector3 forward = Vector3Normalize(Vector3Subtract(mCamera.target, mCamera.position));
                if (Vector3DotProduct(Vector3Subtract(numberPos, mCamera.position), forward) <= 0.0f) continue;
                const Vector2 screen = GetWorldToScreen(numberPos, mCamera);
                if (screen.x < -80.0f || screen.y < -80.0f ||
                    screen.x > GetScreenWidth() + 80.0f || screen.y > GetScreenHeight() + 80.0f) continue;
                char damageText[24];
                std::snprintf(damageText, sizeof(damageText), "-%d", number.amount);
                const int fontSize = 22;
                const Color color = ColorAlpha(Color{255, 72, 72, 255}, Clamp(number.life, 0.0f, 1.0f));
                DrawText(damageText, (int)screen.x - MeasureText(damageText, fontSize) / 2,
                         (int)screen.y, fontSize, color);
            }
        }

        if (mPaused)
        {
#if !defined(NDEBUG)
            if (mDevToolsOpen) DrawDevTools();
            else
#endif
            DrawPauseMenu();
        }
        else
        {
#if !defined(NDEBUG)
            if (!mEditorActive)
#endif
            DrawHud();
            if (mInventoryOpen) DrawInventory();
        }
#if !defined(NDEBUG)
        if (mEditorActive) DrawLevelEditor();
#endif
    }

    void GameScene::SpawnDamageFeedback(const glm::vec3 &position, float amount)
    {
        if (amount <= 0.0f) return;
        DamageNumber number;
        number.position = position + glm::vec3(0.0f, 0.35f, 0.0f);
        number.amount = std::max(1, (int)roundf(amount));
        number.life = 1.0f;
        mDamageNumbers.push_back(number);

        for (int i = 0; i < 14; ++i)
        {
            BloodParticle particle;
            particle.position = position;
            particle.velocity = glm::vec3(
                (float)GetRandomValue(-100, 100) / 180.0f,
                (float)GetRandomValue(45, 145) / 100.0f,
                (float)GetRandomValue(-100, 100) / 180.0f);
            particle.life = (float)GetRandomValue(45, 85) / 100.0f;
            mBloodParticles.push_back(particle);
        }
    }

    void GameScene::UpdateCombatEffects(float deltaTime)
    {
        for (BloodParticle &particle : mBloodParticles)
        {
            particle.life -= deltaTime;
            particle.velocity.z -= 3.8f * deltaTime;
            particle.position += particle.velocity * deltaTime;
        }
        mBloodParticles.erase(std::remove_if(mBloodParticles.begin(), mBloodParticles.end(),
            [](const BloodParticle &particle) { return particle.life <= 0.0f; }), mBloodParticles.end());

        for (DamageNumber &number : mDamageNumbers)
        {
            number.life -= deltaTime;
            number.position.y += 0.65f * deltaTime;
        }
        mDamageNumbers.erase(std::remove_if(mDamageNumbers.begin(), mDamageNumbers.end(),
            [](const DamageNumber &number) { return number.life <= 0.0f; }), mDamageNumbers.end());
    }

    PickupObject *GameScene::GetNearestPickup(float maxDistance) const
    {
        if (!mPlayer) return nullptr;
        const glm::vec3 playerPosition = mPlayer->GetPosition();
        PickupObject *closest = nullptr;
        float closestSq = maxDistance * maxDistance;
        for (PickupObject *pickup : mPickups)
        {
            if (!pickup || pickup->IsCollected()) continue;
            const glm::vec3 p = pickup->GetPosition();
            const float dx = p.x - playerPosition.x;
            const float dy = p.y - playerPosition.y;
            const float distanceSq = dx * dx + dy * dy;
            if (distanceSq <= closestSq)
            {
                closest = pickup;
                closestSq = distanceSq;
            }
        }
        return closest;
    }

    void GameScene::ToggleInventory()
    {
        mInventoryOpen = !mInventoryOpen;
        mInventoryDrag = -1;
        if (mInventoryOpen) EnableCursor();
        else DisableCursor();
    }

    Texture2D GameScene::GetItemIcon(ItemType type) const
    {
        const int index = ItemIconIndex(type);
        return index >= 0 ? mItemIcons[index] : Texture2D{};
    }

    void GameScene::DrawHud()
    {
        if (!mPlayer) return;
        const float scale = ui::UiScale();
        const float margin = 24.0f * scale;
        const Rectangle panel = { margin, margin, 330.0f * scale, 108.0f * scale };
        DrawRectangleRounded(panel, 0.08f, 0, { 15, 18, 25, 205 });
        DrawRectangleRoundedLinesEx(panel, 0.08f, 0, 1.0f, ui::PanelBorder());

        char label[64];
        std::snprintf(label, sizeof(label), "Nível %d", mPlayer->GetLevel());
        ui::DrawText(label, { panel.x + 15.0f * scale, panel.y + 13.0f * scale },
                     (int)(20 * scale), ui::TextPrimary());
        char money[48];
        std::snprintf(money, sizeof(money), "$ %d", mPlayer->GetMoney());
        ui::DrawText(money, { panel.x + 118.0f * scale, panel.y + 14.0f * scale },
                     (int)(17 * scale), {244, 205, 100, 255});
        std::snprintf(label, sizeof(label), "%.0f / %.0f", mPlayer->GetHealth(), mPlayer->GetMaxHealth());
        ui::DrawText(label, { panel.x + panel.width - 85.0f * scale, panel.y + 13.0f * scale },
                     (int)(16 * scale), ui::TextDim());

        const Rectangle health = { panel.x + 15.0f * scale, panel.y + 43.0f * scale,
                                   panel.width - 30.0f * scale, 17.0f * scale };
        const float healthRatio = mPlayer->GetHealth() / mPlayer->GetMaxHealth();
        DrawRectangleRounded(health, 0.5f, 0, { 62, 34, 42, 255 });
        DrawRectangleRounded({ health.x, health.y, health.width * healthRatio, health.height },
                             0.5f, 0, { 202, 65, 76, 255 });
        const Rectangle experience = { health.x, health.y + 30.0f * scale, health.width, 10.0f * scale };
        DrawRectangleRounded(experience, 0.5f, 0, { 30, 47, 72, 255 });
        DrawRectangleRounded({ experience.x, experience.y,
                               experience.width * (mPlayer->GetExperience() / mPlayer->GetExperienceToNextLevel()),
                               experience.height }, 0.5f, 0, { 74, 149, 230, 255 });
        ui::DrawText("Experiência", { experience.x, experience.y + 14.0f * scale },
                     (int)(13 * scale), ui::TextDim());

        PickupObject *nearby = GetNearestPickup(1.7f);
        if (nearby && !mInventoryOpen)
        {
            char prompt[128];
            std::snprintf(prompt, sizeof(prompt), "[E] Coletar: %s", GetItemName(nearby->GetItemType()));
            const float w = (float)MeasureText(prompt, (int)(18 * scale));
            DrawRectangleRounded({ (GetScreenWidth() - w - 34.0f * scale) * 0.5f,
                                   GetScreenHeight() - 86.0f * scale, w + 34.0f * scale,
                                   38.0f * scale }, 0.25f, 0, { 17, 20, 28, 220 });
            ui::DrawTextCentered(prompt, GetScreenWidth() * 0.5f, GetScreenHeight() - 78.0f * scale,
                                 (int)(18 * scale), ui::TextPrimary());
        }
        if (mToastTime > 0.0f)
            ui::DrawTextCentered(mToast, GetScreenWidth() * 0.5f, 30.0f * scale,
                                 (int)(20 * scale), { 244, 218, 151, 255 });

        ui::DrawText("[I] Inventário   [Mouse esquerdo] Atacar", { margin, GetScreenHeight() - 29.0f * scale },
                     (int)(15 * scale), ui::TextDim());
    }

    void GameScene::DrawInventory()
    {
        if (!mPlayer) return;
        ui::DrawOverlay(0.50f);
        const float scale = ui::UiScale();
        const float width = 700.0f * scale;
        const float height = 480.0f * scale;
        Rectangle panel = { (GetScreenWidth() - width) * 0.5f, (GetScreenHeight() - height) * 0.5f, width, height };
        ui::DrawPanel(panel, nullptr);
        ui::DrawTextCentered("INVENTARIO", panel.x + panel.width * 0.5f,
                             panel.y + 18.0f * scale, (int)(24 * scale), ui::TextPrimary());
        DrawRectangleV({ panel.x + 24.0f * scale, panel.y + 53.0f * scale },
                       { panel.width - 48.0f * scale, 1.0f }, ui::PanelBorder());

        constexpr int WeaponDragId = Inventory::SlotCount;
        constexpr int ArmorDragId = Inventory::SlotCount + 1;
        constexpr int PotionDragId = Inventory::SlotCount + 2;
        const float slotSize = 76.0f * scale;
        const float gap = 8.0f * scale;
        const float gridX = panel.x + 24.0f * scale;
        const float gridY = panel.y + 82.0f * scale;
        Vector2 mouse = GetMousePosition();
        int releaseTarget = -1;
        bool releaseEquipment = false;
        EquipmentSlot releaseEquipmentSlot = EquipmentSlot::Weapon;
        for (int index = 0; index < Inventory::SlotCount; ++index)
        {
            const int column = index % 4;
            const int row = index / 4;
            Rectangle slotRect = { gridX + column * (slotSize + gap), gridY + row * (slotSize + gap), slotSize, slotSize };
            InventorySlot &slot = mPlayer->GetInventory().GetSlot(index);
            const bool hovered = CheckCollisionPointRec(mouse, slotRect);
            DrawRectangleRounded(slotRect, 0.025f, 0,
                                 hovered ? Color{ 65, 71, 87, 255 } : Color{ 35, 39, 50, 245 });
            DrawRectangleRoundedLinesEx(slotRect, 0.025f, 0, hovered ? 2.0f : 1.0f,
                                        hovered ? ui::Accent() : ui::PanelBorder());
            if (!slot.IsEmpty())
            {
                Texture2D icon = GetItemIcon(slot.type);
                if (icon.id != 0)
                    DrawTexturePro(icon, { 0, 0, (float)icon.width, (float)icon.height },
                                   { slotRect.x + 7.0f * scale, slotRect.y + 6.0f * scale,
                                     slotRect.width - 14.0f * scale, slotRect.height - 14.0f * scale },
                                   { 0, 0 }, 0.0f, WHITE);
                else
                    DrawCircleV({ slotRect.x + slotRect.width * 0.5f, slotRect.y + slotRect.height * 0.5f },
                                19.0f * scale, GetItemColor(slot.type));
                if (slot.count > 1)
                {
                    char count[16]; std::snprintf(count, sizeof(count), "%d", slot.count);
                    ui::DrawText(count, { slotRect.x + slotRect.width - 17.0f * scale,
                                           slotRect.y + slotRect.height - 23.0f * scale },
                                 (int)(16 * scale), WHITE);
                }
            }

            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (!slot.IsEmpty()) mInventoryDrag = index;
            }
            if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) releaseTarget = index;
        }

        const float equipmentX = gridX + 4.0f * (slotSize + gap) + 18.0f * scale;
        const float equipmentY = gridY - 2.0f * scale;
        const float equipmentW = panel.x + panel.width - equipmentX - 24.0f * scale;
        const float equipmentH = 66.0f * scale;
        ui::DrawText("EQUIPAMENTOS", { equipmentX, panel.y + 61.0f * scale },
                     (int)(16 * scale), ui::TextDim());

        auto drawEquipmentSlot = [&](EquipmentSlot equipmentSlot, int dragId, const char *label,
                                     const char *hint, float y)
        {
            Rectangle bounds = { equipmentX, y, equipmentW, equipmentH };
            const InventorySlot &slot = mPlayer->GetEquipment(equipmentSlot);
            const bool hovered = CheckCollisionPointRec(mouse, bounds);
            DrawRectangleRounded(bounds, 0.025f, 0, hovered ? Color{ 57, 65, 81, 250 } : Color{ 29, 33, 43, 245 });
            DrawRectangleRoundedLinesEx(bounds, 0.025f, 0, hovered ? 2.0f : 1.0f,
                                        hovered ? ui::Accent() : ui::PanelBorder());
            ui::DrawText(label, { bounds.x + 10.0f * scale, bounds.y + 8.0f * scale },
                         (int)(14 * scale), ui::TextPrimary());
            ui::DrawText(slot.IsEmpty() ? hint : GetItemName(slot.type),
                         { bounds.x + 10.0f * scale, bounds.y + 33.0f * scale },
                         (int)(12 * scale), slot.IsEmpty() ? ui::TextDim() : Color{ 177, 221, 190, 255 });
            if (!slot.IsEmpty())
            {
                Texture2D icon = GetItemIcon(slot.type);
                Rectangle iconBounds = { bounds.x + bounds.width - 54.0f * scale, bounds.y + 7.0f * scale,
                                         46.0f * scale, 52.0f * scale };
                if (icon.id != 0)
                    DrawTexturePro(icon, { 0, 0, (float)icon.width, (float)icon.height }, iconBounds,
                                   { 0, 0 }, 0.0f, WHITE);
                if (slot.count > 1)
                {
                    char count[16]; std::snprintf(count, sizeof(count), "%d", slot.count);
                    ui::DrawText(count, { iconBounds.x + iconBounds.width - 15.0f * scale,
                                           iconBounds.y + iconBounds.height - 21.0f * scale },
                                 (int)(15 * scale), WHITE);
                }
            }
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !slot.IsEmpty()) mInventoryDrag = dragId;
            if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                releaseEquipment = true;
                releaseEquipmentSlot = equipmentSlot;
            }
        };
        drawEquipmentSlot(EquipmentSlot::Weapon, WeaponDragId, "ARMA", "Arraste uma arma aqui", equipmentY);
        drawEquipmentSlot(EquipmentSlot::Armor, ArmorDragId, "ARMADURA", "Arraste uma armadura aqui",
                          equipmentY + equipmentH + 9.0f * scale);
        drawEquipmentSlot(EquipmentSlot::Potion, PotionDragId, "POCAO RAPIDA", "Arraste uma pocao aqui",
                          equipmentY + 2.0f * (equipmentH + 9.0f * scale));

        auto spawnDroppedItem = [&](ItemType type, int count)
        {
            const glm::vec3 p = mPlayer->GetPosition();
            mPickups.push_back(SpawnObject<PickupObject>(
                glm::vec3(p.x + 0.9f, p.y - 0.85f, p.z + 0.9f), type, count));
        };

        if (mInventoryDrag >= 0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            const bool droppedOutside = !CheckCollisionPointRec(mouse, panel);
            if (mInventoryDrag < Inventory::SlotCount)
            {
                if (releaseTarget >= 0 && releaseTarget != mInventoryDrag)
                    mPlayer->GetInventory().Swap(mInventoryDrag, releaseTarget);
                else if (releaseEquipment)
                    mPlayer->MoveInventoryToEquipment(mInventoryDrag, releaseEquipmentSlot);
                else if (droppedOutside)
                {
                    ItemType type = ItemType::None;
                    int count = 0;
                    if (mPlayer->DropInventorySlot(mInventoryDrag, type, count))
                        spawnDroppedItem(type, count);
                }
            }
            else if (releaseTarget >= 0)
            {
                const EquipmentSlot source = mInventoryDrag == WeaponDragId ? EquipmentSlot::Weapon :
                                             mInventoryDrag == ArmorDragId ? EquipmentSlot::Armor : EquipmentSlot::Potion;
                mPlayer->MoveEquipmentToInventory(source, releaseTarget);
            }
            else if (droppedOutside)
            {
                const EquipmentSlot source = mInventoryDrag == WeaponDragId ? EquipmentSlot::Weapon :
                                             mInventoryDrag == ArmorDragId ? EquipmentSlot::Armor : EquipmentSlot::Potion;
                ItemType type = ItemType::None;
                int count = 0;
                if (mPlayer->DropEquipmentSlot(source, type, count))
                    spawnDroppedItem(type, count);
            }
            mInventoryDrag = -1;
        }
        if (mInventoryDrag >= 0)
        {
            const InventorySlot &dragged = mInventoryDrag < Inventory::SlotCount
                ? mPlayer->GetInventory().GetSlot(mInventoryDrag)
                : mPlayer->GetEquipment(mInventoryDrag == WeaponDragId ? EquipmentSlot::Weapon :
                                         mInventoryDrag == ArmorDragId ? EquipmentSlot::Armor : EquipmentSlot::Potion);
            Texture2D icon = GetItemIcon(dragged.type);
            if (icon.id != 0)
                DrawTexturePro(icon, { 0, 0, (float)icon.width, (float)icon.height },
                               { mouse.x - 24.0f * scale, mouse.y - 24.0f * scale, 48.0f * scale, 48.0f * scale },
                               { 0, 0 }, 0.0f, ColorAlpha(WHITE, 0.82f));
        }

#if 0 // Superseded by the direct equipment slots above.
        const float sideX = gridX + 4.0f * (slotSize + gap) + 37.0f * scale;
        Rectangle detail = { sideX, gridY, panel.x + panel.width - sideX - 30.0f * scale, 248.0f * scale };
        DrawRectangleRounded(detail, 0.06f, 0, { 29, 33, 43, 245 });
        DrawRectangleRoundedLinesEx(detail, 0.06f, 0, 1.0f, ui::PanelBorder());
        const InventorySlot *selected = mInventorySelected >= 0 ? &mPlayer->GetInventory().GetSlot(mInventorySelected) : nullptr;
        ui::DrawText(selected && !selected->IsEmpty() ? GetItemName(selected->type) : "Selecione um item",
                     { detail.x + 18.0f * scale, detail.y + 22.0f * scale }, (int)(20 * scale), ui::TextPrimary());
        if (selected && !selected->IsEmpty())
        {
            const char *kind = IsConsumable(selected->type) ? "Consumível: restaura 38 de vida" :
                               IsWeapon(selected->type) ? "Arma: libera ataque com machado" :
                               "Equipamento: altera o visual do personagem";
            ui::DrawText(kind, { detail.x + 18.0f * scale, detail.y + 61.0f * scale },
                         (int)(15 * scale), ui::TextDim());
            if (mInventorySelected == mPlayer->GetEquippedWeaponSlot() ||
                mInventorySelected == mPlayer->GetEquippedArmorSlot())
                ui::DrawText("EQUIPADO", { detail.x + 18.0f * scale, detail.y + 91.0f * scale },
                             (int)(16 * scale), { 116, 211, 142, 255 });
        }

        const float buttonW = detail.width - 36.0f * scale;
        float buttonY = detail.y + 128.0f * scale;
        if (selected && !selected->IsEmpty() && IsConsumable(selected->type) &&
            ui::DrawButton("Usar", { detail.x + 18.0f * scale, buttonY, buttonW, 38.0f * scale }))
            mPlayer->UseInventorySlot(mInventorySelected);
        buttonY += 47.0f * scale;
        if (selected && !selected->IsEmpty() && (IsWeapon(selected->type) || IsArmor(selected->type)) &&
            ui::DrawButton("Equipar / remover", { detail.x + 18.0f * scale, buttonY, buttonW, 38.0f * scale }))
            mPlayer->EquipInventorySlot(mInventorySelected);
        buttonY += 47.0f * scale;
        if (selected && !selected->IsEmpty() &&
            ui::DrawButton("Soltar", { detail.x + 18.0f * scale, buttonY, buttonW, 38.0f * scale }))
        {
            ItemType type; int count;
            if (mPlayer->DropInventorySlot(mInventorySelected, type, count))
            {
                const glm::vec3 p = mPlayer->GetPosition();
                mPickups.push_back(SpawnObject<PickupObject>(glm::vec3(p.x + 1.0f, p.y - 0.85f, p.z + 1.0f), type, count));
                if (mPlayer->GetInventory().GetSlot(mInventorySelected).IsEmpty()) mInventorySelected = -1;
            }
        }
        ui::DrawText("Arraste itens entre slots. [I] ou [Esc] fecha", { panel.x + 30.0f * scale,
                     panel.y + panel.height - 37.0f * scale }, (int)(16 * scale), ui::TextDim());
#endif
        ui::DrawText("Arraste da mochila para os equipamentos. [Q] usa a pocao rapida.",
                     { panel.x + 30.0f * scale, panel.y + panel.height - 42.0f * scale },
                     (int)(15 * scale), ui::TextDim());
        ui::DrawText("Clique fora da janela, [I] ou [Esc] para fechar.",
                     { panel.x + 30.0f * scale, panel.y + panel.height - 21.0f * scale },
                     (int)(14 * scale), ui::TextDim());
    }

    void GameScene::DrawDebug()
    {
        for (GameObject *obj : mObjects)
            obj->DrawDebug(*this);
    }

#if !defined(NDEBUG)
    void GameScene::DrawDevTools()
    {
        PlayerObject *player = mPlayer;
        if (!player) return;
        // Sem o overlay escuro de tela cheia: a cena fica visivel em claridade
        // total atras do painel (igual ao editor F1). O painel ImGui tem fundo
        // proprio para manter o texto legivel.
        ImGui::SetNextWindowPos(ImVec2(24, 24), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 440), ImGuiCond_FirstUseEver);
        const bool wasDevOpen = mDevToolsOpen;
        if (ImGui::Begin("Ferramentas de desenvolvimento (F2)", &mDevToolsOpen))
        {
            const float maxHealth = player->GetMaxHealth();
            ImGui::Text("Nivel %d  |  Dano: %.0f", player->GetLevel(), player->GetAttackDamage());
            ImGui::Text("XP: %.0f / %.0f", player->GetExperience(), player->GetExperienceToNextLevel());
            ImGui::Separator();

            float health = player->GetHealth();
            if (ImGuiValueEditFloat("Vida", &health, 0.0f, maxHealth, 1.0f, "%.0f"))
                player->DevSetHealth(health);
            float maxHp = maxHealth;
            if (ImGuiValueEditFloat("Vida maxima", &maxHp, 1.0f, 9999.0f, 10.0f, "%.0f"))
                player->DevSetMaxHealth(maxHp);
            int level = player->GetLevel();
            if (ImGuiValueEditInt("Nivel", &level, 1, 100, 1, "%d"))
                player->DevSetLevel(level);
            float experience = player->GetExperience();
            if (ImGuiValueEditFloat("Experiencia", &experience, 0.0f, 10000.0f, 10.0f, "%.0f"))
                player->DevSetExperience(experience);
            int money = player->GetMoney();
            if (ImGuiValueEditInt("Dinheiro", &money, 0, 999999, 100, "%d"))
                player->DevSetMoney(money);
            ImGui::Separator();
            ImGui::TextDisabled("Arraste, role a roda ou digite (2x clique/Ctrl+clique no valor). Shift suaviza.");
            ImGui::Separator();
            if (ImGui::Button("Curar totalmente"))
                player->DevSetHealth(player->GetMaxHealth());
            ImGui::SameLine();
            if (ImGui::Button("Subir nivel"))
                player->AddExperience(player->GetExperienceToNextLevel() - player->GetExperience() + 1.0f);
            if (ImGui::Button("+1000 de XP"))
                player->AddExperience(1000.0f);
            ImGui::SameLine();
            if (ImGui::Button("+500 de ouro"))
                player->AddMoney(500);
            ImGui::SameLine();
            if (ImGui::Button("Morte instantanea"))
                player->DevSetHealth(0.0f);
        }
        ImGui::End();
        // Fechar pelo X do ImGui desfaz a pausa, como a tecla F2.
        if (wasDevOpen && !mDevToolsOpen)
        {
            mPaused = false;
            DisableCursor();
        }
    }
#endif

    void GameScene::DrawPauseMenu()
    {
        ui::DrawOverlay();

        if (mPauseSettingsOpen)
        {
            if (mSettingsScreen.Draw(mSettings))
            {
                SaveSettings(mSettings);
                mPauseSettingsOpen = false;
            }
            return;
        }

        float scale = ui::UiScale();
        float panelW = 360 * scale;
        float panelH = 360 * scale;
        Rectangle panel = { (GetScreenWidth() - panelW) * 0.5f,
                            (GetScreenHeight() - panelH) * 0.5f, panelW, panelH };
        ui::DrawPanel(panel, "Pausa");

        float btnW = 260 * scale;
        float btnH = 48 * scale;
        float gap = 14 * scale;
        float bx = panel.x + (panel.width - btnW) * 0.5f;
        float by = panel.y + 84 * scale;

        if (ui::DrawButton("Continuar", { bx, by, btnW, btnH }))
        {
            mPaused = false;
            mPauseSettingsOpen = false;
            DisableCursor();
        }
        by += btnH + gap;

        if (ui::DrawButton("Configurações", { bx, by, btnW, btnH }))
            mPauseSettingsOpen = true;
        by += btnH + gap;

        if (ui::DrawButton("Menu principal", { bx, by, btnW, btnH }))
            SetRequest(SceneRequest::MainMenu);
        by += btnH + gap;

        if (ui::DrawButton("Sair", { bx, by, btnW, btnH }))
            SetRequest(SceneRequest::Quit);
    }

#if !defined(NDEBUG)
    namespace
    {
        // The editing gizmo uses the Blender axis convention: X (red) = right,
        // Y (green) = forward, Z (blue) = up. The world itself is Y-up, so the
        // green gizmo axis drives world Z and the blue gizmo axis drives world
        // Y (height). Indexes are gizmo axes; values are world component indexes.
        static constexpr int kGizmoAxisToWorld[3] = { 0, 2, 1 };
        static const Vector3 kWorldAxisDir[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
        // Cada anel tem um raio proprio (azul/yaw e o maior) para os tres eixos
        // ficarem claramente distinguiveis e o clique nao cair no anel errado.
        static const float kGizmoRingRadius[3] = { 1.25f, 1.38f, 1.52f };

        // ---- Gizmo grafico estilo Blender ----
        // Cores dos eixos (tema padrao do Blender) e estado ativo.
        static const Color kGizmoColors[3] = {
            { 255, 90, 90, 255 },    // X vermelho
            { 130, 230, 130, 255 },  // Y verde
            { 120, 150, 255, 255 }   // Z azul
        };
        static const Color kGizmoActiveColor = { 255, 255, 255, 255 };

        // Escala do gizmo para que ele mantenha tamanho constante na tela
        // (~90 px), independente da distancia da camera (como no Blender).
        static float GizmoScreenScale(const Camera3D &camera, const Vector3 &origin)
        {
            const float dist = std::max(0.001f, Vector3Distance(camera.position, origin));
            const float fovScale = 2.0f * tanf(camera.fovy * 0.5f * DEG2RAD);
            const float pixelsPerUnit = (float)GetScreenHeight() / (dist * fovScale);
            return 90.0f / pixelsPerUnit;
        }

        // Linha 3D grossa: um ribbon (quad) virado para a camera, so com a cor.
        static void DrawGizmoLine(const Vector3 &start, const Vector3 &end,
                                  float thickness, Color color, const Vector3 &cameraPos)
        {
            Vector3 dir = Vector3Subtract(end, start);
            const float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
            if (len < 0.0001f) return;
            dir = Vector3Scale(dir, 1.0f / len);
            const Vector3 mid = Vector3Scale(Vector3Add(start, end), 0.5f);
            Vector3 viewDir = Vector3Subtract(cameraPos, mid);
            Vector3 normal = Vector3CrossProduct(dir, viewDir);
            if (Vector3LengthSqr(normal) < 0.0001f) normal = { 1.0f, 0.0f, 0.0f };
            normal = Vector3Normalize(normal);
            const Vector3 off = Vector3Scale(normal, thickness * 0.5f);
            const Vector3 a = Vector3Add(start, off);
            const Vector3 b = Vector3Subtract(start, off);
            const Vector3 c = Vector3Add(end, off);
            const Vector3 d = Vector3Subtract(end, off);
            DrawTriangle3D(a, c, b, color);
            DrawTriangle3D(b, c, d, color);
        }

        // Anel grosso de rotacao em cor unica solida (sem contorno e sem
        // escurecer a metade de tras).
        static void DrawGizmoRing(const Vector3 &center, float radius, const Vector3 &axis,
                                  float thickness, Color color, const Vector3 &cameraPos)
        {
            Vector3 u = { 0.0f, 1.0f, 0.0f };
            if (fabsf(axis.y) > 0.9f) u = { 1.0f, 0.0f, 0.0f };
            Vector3 v = Vector3Normalize(Vector3CrossProduct(axis, u));
            u = Vector3Normalize(Vector3CrossProduct(v, axis));
            const int segs = 96;
            Vector3 prev = Vector3Add(center, Vector3Scale(u, radius));
            for (int s = 1; s <= segs; ++s)
            {
                const float ang = (float)s * 2.0f * PI / segs;
                const Vector3 cur = Vector3Add(center, Vector3Add(
                    Vector3Scale(u, cosf(ang) * radius),
                    Vector3Scale(v, sinf(ang) * radius)));
                DrawGizmoLine(prev, cur, thickness, color, cameraPos);
                prev = cur;
            }
        }

        // Ponta de cone (seta) ao longo de um eixo.
        static void DrawGizmoCone(const Vector3 &base, const Vector3 &tip,
                                  float baseRadius, Color color)
        {
            DrawCylinderEx(base, tip, baseRadius, 0.0f, 8, color);
        }

        // Picking compartilhado (clique e hover): retorna o eixo mais proximo do
        // cursor, amostrando os aneis/linhas na mesma geometria desenhada.
        static int PickGizmoAxis(const Vector3 &origin, const Vector2 &mouse, bool rotate,
                                 const Camera3D &camera)
        {
            int axis = -1; float closest = 21.0f * 21.0f;
            const float gizmoScale = GizmoScreenScale(camera, origin);
            const float lineLen = 1.15f * gizmoScale;
            const float ringScale = 0.5f * gizmoScale;
            if (rotate)
            {
                const Vector3 ringNormals[3] = { {1,0,0}, {0,0,1}, {0,1,0} };
                for (int i = 0; i < 3; ++i)
                {
                    Vector3 u = { 0.0f, 1.0f, 0.0f };
                    if (fabsf(ringNormals[i].y) > 0.9f) u = { 1.0f, 0.0f, 0.0f };
                    Vector3 v = Vector3Normalize(Vector3CrossProduct(ringNormals[i], u));
                    u = Vector3Normalize(Vector3CrossProduct(v, ringNormals[i]));
                    for (int s = 0; s < 64; ++s)
                    {
                        const float angle = (float)s * 2.0f * PI / 64.0f;
                        const Vector3 point = Vector3Add(origin, Vector3Add(
                            Vector3Scale(u, cosf(angle) * kGizmoRingRadius[i] * ringScale),
                            Vector3Scale(v, sinf(angle) * kGizmoRingRadius[i] * ringScale)));
                        const Vector2 screen = GetWorldToScreen(point, camera);
                        const float dx = screen.x - mouse.x, dy = screen.y - mouse.y;
                        const float d2 = dx*dx + dy*dy;
                        if (d2 < closest) { closest = d2; axis = i; }
                    }
                }
            }
            else
            {
                const Vector3 ends[3] = {
                    Vector3Add(origin, Vector3{ lineLen, 0.0f, 0.0f }),
                    Vector3Add(origin, Vector3{ 0.0f, 0.0f, lineLen }),
                    Vector3Add(origin, Vector3{ 0.0f, lineLen, 0.0f })
                };
                for (int i = 0; i < 3; ++i)
                {
                    const Vector3 dir = Vector3Subtract(ends[i], origin);
                    for (float t = 0.0f; t <= 1.0f; t += 0.08f)
                    {
                        const Vector3 point = Vector3Add(origin, Vector3Scale(dir, t));
                        const Vector2 screen = GetWorldToScreen(point, camera);
                        const float dx = screen.x - mouse.x, dy = screen.y - mouse.y;
                        const float d2 = dx*dx + dy*dy;
                        if (d2 < closest) { closest = d2; axis = i; }
                    }
                }
            }
            return axis;
        }

        // Rotaciona um vetor direcao pela parte 3x3 de uma matriz (w = 0).
        static Vector3 TransformDir(const Matrix &m, const Vector3 &v)
        {
            return { m.m0*v.x + m.m4*v.y + m.m8*v.z,
                     m.m1*v.x + m.m5*v.y + m.m9*v.z,
                     m.m2*v.x + m.m6*v.y + m.m10*v.z };
        }

        // Intersecao raio vs caixa (slab) no espaco local. Devolve o eixo da face
        // atingida e o lado (+1 face positiva, -1 face negativa).
        static bool RayBoxFace(const Ray &ray, const Vector3 &center, const Vector3 &rotRad,
                               const Vector3 &half, int &axis, float &side)
        {
            const Matrix world = MatrixMultiply(MatrixRotateXYZ(rotRad),
                                                MatrixTranslate(center.x, center.y, center.z));
            const Matrix inv = MatrixInvert(world);
            const Vector3 origin = Vector3Transform(ray.position, inv);
            const Vector3 dir = TransformDir(inv, ray.direction);
            const float o[3] = {origin.x, origin.y, origin.z};
            const float d[3] = {dir.x, dir.y, dir.z};
            const float h[3] = {half.x, half.y, half.z};
            float tmin = -1e30f, tmax = 1e30f;
            int hitAxis = 0; float hitSide = 1.0f;
            for (int i = 0; i < 3; ++i)
            {
                if (fabsf(d[i]) < 1e-6f) continue;
                const float t1 = (-h[i] - o[i]) / d[i];
                const float t2 = ( h[i] - o[i]) / d[i];
                const float tn = fminf(t1, t2), tf = fmaxf(t1, t2);
                if (tn > tmin) { tmin = tn; hitAxis = i; hitSide = (t1 < t2) ? -1.0f : 1.0f; }
                if (tf < tmax) tmax = tf;
                if (tmax < tmin) return false;
            }
            if (tmin <= 0.0f) return false;
            axis = hitAxis; side = hitSide;
            return true;
        }

        // Caixa de colisao editavel: cubo em grade (grid) envolvendo o asset,
        // com a face sob o mouse clareada para situar o usuario.
        static void DrawBoxGrid(const Vector3 &center, const Vector3 &size, const Vector3 &rotRad,
                                float thickness, Color edgeColor, Color gridColor,
                                const Vector3 &cameraPos, int hoverAxis, float hoverSide)
        {
            const Matrix rot = MatrixRotateXYZ(rotRad);
            const float h[3] = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
            Vector3 corners[8];
            for (int i = 0; i < 8; ++i)
            {
                const Vector3 local = { (i & 1) ? h[0] : -h[0],
                                        (i & 2) ? h[1] : -h[1],
                                        (i & 4) ? h[2] : -h[2] };
                corners[i] = Vector3Add(center, Vector3Transform(local, rot));
            }
            static const int edges[12][2] = {
                {0,1},{2,3},{4,5},{6,7},
                {0,2},{1,3},{4,6},{5,7},
                {0,4},{1,5},{2,6},{3,7}
            };
            for (int e = 0; e < 12; ++e)
                DrawGizmoLine(corners[edges[e][0]], corners[edges[e][1]], thickness, edgeColor, cameraPos);
            // Linhas internas do grid em cada face (2 divisoes por face).
            for (int f = 0; f < 6; ++f)
            {
                const int a = f >> 1;
                const float sign = (f & 1) ? 1.0f : -1.0f;
                const int s1 = (a + 1) % 3, s2 = (a + 2) % 3;
                const bool hovered = hoverAxis == a && ((hoverSide > 0.0f) == (sign > 0.0f));
                Color color = hovered ? ColorLerp(gridColor, WHITE, 0.55f) : gridColor;
                const float t = 0.5f;
                for (int k = -1; k <= 1; k += 2)
                {
                    float p0[3] = {0,0,0}, p1[3] = {0,0,0};
                    p0[a] = p1[a] = sign * h[a];
                    p0[s2] = p1[s2] = (float)k * h[s2] * t;
                    p0[s1] = -h[s1]; p1[s1] = h[s1];
                    DrawGizmoLine(Vector3Add(center, Vector3Transform({p0[0], p0[1], p0[2]}, rot)),
                                  Vector3Add(center, Vector3Transform({p1[0], p1[1], p1[2]}, rot)),
                                  thickness * 0.45f, color, cameraPos);
                    p0[s1] = p1[s1] = (float)k * h[s1] * t;
                    p0[s2] = -h[s2]; p1[s2] = h[s2];
                    DrawGizmoLine(Vector3Add(center, Vector3Transform({p0[0], p0[1], p0[2]}, rot)),
                                  Vector3Add(center, Vector3Transform({p1[0], p1[1], p1[2]}, rot)),
                                  thickness * 0.45f, color, cameraPos);
                }
                // Face sob o mouse: contorno mais grosso e mais claro.
                if (hovered)
                {
                    const int bit = 1 << a;
                    for (int e = 0; e < 12; ++e)
                    {
                        const int c0 = edges[e][0], c1 = edges[e][1];
                        const bool on0 = ((c0 & bit) != 0) == (sign > 0.0f);
                        const bool on1 = ((c1 & bit) != 0) == (sign > 0.0f);
                        if (on0 && on1)
                            DrawGizmoLine(corners[c0], corners[c1], thickness * 1.7f,
                                          ColorLerp(edgeColor, WHITE, 0.5f), cameraPos);
                    }
                }
            }
        }

        std::filesystem::path LevelFilePath(int level)
        {
            const std::filesystem::path candidates[] = {
                "Game/Assets/Levels/Gameplay.level",
                "../../Game/Assets/Levels/Gameplay.level",
                "Assets/Levels/Gameplay.level"
            };
            std::filesystem::path base = candidates[0];
            for (const auto &candidate : candidates)
                if (std::filesystem::exists(candidate.parent_path().parent_path())) { base = candidate; break; }
            char name[32];
            std::snprintf(name, sizeof(name), "Fase%02d.level", std::max(1, level));
            const std::filesystem::path phase = base.parent_path() / name;
            // Compatibilidade com o primeiro arquivo criado pela milestone 08.
            if (level == 1 && !std::filesystem::exists(phase) && std::filesystem::exists(base)) return base;
            return phase;
        }

        std::filesystem::path SelectFBXFile()
        {
#if defined(_WIN32)
            wchar_t fileName[32768] = {};
            const wchar_t filter[] = L"Modelos FBX (*.fbx)\0*.fbx\0Todos os arquivos (*.*)\0*.*\0";
            OPENFILENAMEW dialog = {};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = (HWND)GetWindowHandle();
            dialog.lpstrFile = fileName;
            dialog.nMaxFile = (DWORD)std::size(fileName);
            dialog.lpstrFilter = filter;
            dialog.nFilterIndex = 1;
            dialog.lpstrTitle = L"Importar prop FBX";
            dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR |
                           OFN_EXPLORER | OFN_HIDEREADONLY;
            return GetOpenFileNameW(&dialog) ? std::filesystem::path(fileName) : std::filesystem::path{};
#else
            return {};
#endif
        }
    }

    void GameScene::ToggleLevelEditor()
    {
        mEditorActive = !mEditorActive;
        if (mEditorActive)
        {
            mEditorCamera = mCamera;
            const Vector3 direction = Vector3Normalize(Vector3Subtract(mEditorCamera.target, mEditorCamera.position));
            mEditorYaw = atan2f(direction.x, direction.z);
            mEditorPitch = asinf(Clamp(direction.y, -1.0f, 1.0f));
            EnableCursor();
        }
        else
        {
            SaveLevel();
            DisableCursor();
        }
    }

    void GameScene::UpdateLevelEditor(float deltaTime)
    {
        mTerraformCooldown = std::max(0.0f, mTerraformCooldown - deltaTime);
        const Vector2 mouseDelta = GetMouseDelta();
        const bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
        const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        const bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        ImGuiIO &io = ImGui::GetIO();
        if (control && IsKeyPressed(KEY_Z))
        {
            if (shift) RedoEditor(); else UndoEditor();
            return;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) mEditorStrokeActive = false;
        // O botao do meio controla a visao; o direito fica exclusivamente para remover.
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) DisableCursor();
        if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) EnableCursor();
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            mEditorYaw -= mouseDelta.x * 0.003f;
            mEditorPitch -= mouseDelta.y * 0.003f;
            mEditorPitch = Clamp(mEditorPitch, -1.48f, 1.48f);
        }
        else if (alt && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            if (shift)
            {
                const Vector3 right = {cosf(mEditorYaw), 0.0f, -sinf(mEditorYaw)};
                const Vector3 up = {0.0f, 1.0f, 0.0f};
                mEditorCamera.position = Vector3Add(mEditorCamera.position,
                    Vector3Add(Vector3Scale(right, -mouseDelta.x * 0.025f),
                               Vector3Scale(up, mouseDelta.y * 0.025f)));
            }
            else if (control)
            {
                const Vector3 forward = Vector3Normalize(Vector3Subtract(mEditorCamera.target, mEditorCamera.position));
                mEditorCamera.position = Vector3Add(mEditorCamera.position,
                                                    Vector3Scale(forward, mouseDelta.y * 0.08f));
            }
            else
            {
                mEditorYaw -= mouseDelta.x * 0.003f;
                mEditorPitch -= mouseDelta.y * 0.003f;
            }
        }

        const float cp = cosf(mEditorPitch);
        const Vector3 forward = {sinf(mEditorYaw) * cp, sinf(mEditorPitch), cosf(mEditorYaw) * cp};
        const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0,1,0}));
        if (!io.WantCaptureKeyboard)
        {
            float speed = mEditorSpeed * deltaTime * (shift ? 3.0f : 1.0f);
            if (IsKeyDown(KEY_W)) mEditorCamera.position = Vector3Add(mEditorCamera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_S)) mEditorCamera.position = Vector3Subtract(mEditorCamera.position, Vector3Scale(forward, speed));
            if (IsKeyDown(KEY_D)) mEditorCamera.position = Vector3Add(mEditorCamera.position, Vector3Scale(right, speed));
            if (IsKeyDown(KEY_A)) mEditorCamera.position = Vector3Subtract(mEditorCamera.position, Vector3Scale(right, speed));
            if (IsKeyDown(KEY_E)) mEditorCamera.position.y += speed;
            if (IsKeyDown(KEY_Q)) mEditorCamera.position.y -= speed;
        }
        mEditorCamera.target = Vector3Add(mEditorCamera.position, forward);
        mEditorCamera.up = {0,1,0};
        mEditorCamera.fovy = 60.0f;
        mEditorCamera.projection = CAMERA_PERSPECTIVE;
        mCamera = mEditorCamera;
        if (mNature) mNature->Update(*this, 0.0f);

        mEditorHitValid = false;
        if (mTerrain && !IsCursorHidden())
        {
            const Ray ray = GetScreenToWorldRay(GetMousePosition(), mEditorCamera);
            const RayCollision collision = GetRayCollisionMesh(ray, mTerrain->GetMesh(), MatrixIdentity());
            if (collision.hit)
            {
                mEditorHitValid = true;
                mEditorHit = {collision.point.x, collision.point.y, collision.point.z};
            }
        }

        const float wheel = io.WantCaptureMouse ? 0.0f : GetMouseWheelMove();
        bool placementWheelHandled = false;
        if (!mEditorBrushEnabled && mEditorTool == 1 && wheel != 0.0f)
        {
            if (IsKeyDown(KEY_Z))
            {
                mEditorPropPlacementScale = std::max(0.0001f,
                    mEditorPropPlacementScale * powf(1.12f, wheel));
                placementWheelHandled = true;
            }
            else if (IsKeyDown(KEY_C))
            {
                mEditorPropPlacementRotation += wheel * 5.0f;
                placementWheelHandled = true;
            }
        }

        // --- Edicao da caixa de colisao (primeira ferramenta do menu de contexto) ---
        if (mEditorCollisionEdit && mEditorSelected >= 0 &&
            mEditorSelected < (int)mLevelEntries.size() && !mEditorBrushEnabled)
        {
            LevelEntry &entry = mLevelEntries[mEditorSelected];
            if (entry.type != 1 || !mNature || entry.natureId == 0)
                mEditorCollisionEdit = false;
            else
            {
                glm::vec3 boxSize, boxOffset;
                mNature->GetEditorCollisionBox(entry.natureId, boxSize, boxOffset);
                const Vector3 rotRad = {entry.rotation.x * DEG2RAD, entry.rotation.y * DEG2RAD,
                                        entry.rotation.z * DEG2RAD};
                const Vector3 offsetWorld = Vector3Transform({boxOffset.x, boxOffset.y, boxOffset.z},
                                                             MatrixRotateXYZ(rotRad));
                const Vector3 boxCenter = {entry.position.x + offsetWorld.x,
                                           entry.position.y + offsetWorld.y,
                                           entry.position.z + offsetWorld.z};
                const Vector3 boxHalf = {boxSize.x * 0.5f, boxSize.y * 0.5f, boxSize.z * 0.5f};

                if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    mEditorCollisionEdit = false;

                if (mEditorCollisionEdit && !io.WantCaptureMouse)
                {
                    const Ray ray = GetScreenToWorldRay(GetMousePosition(), mEditorCamera);
                    int hitAxis = -1; float hitSide = 0.0f;
                    const bool overBox = RayBoxFace(ray, boxCenter, rotRad, boxHalf, hitAxis, hitSide);
                    // Hover para clarear a face (codificado: eixo*2 + lado).
                    if (mEditorCollisionAxis < 0 && !mEditorCollisionDragMove)
                        mEditorGizmoHover = overBox ? hitAxis * 2 + (hitSide > 0.0f ? 1 : 0) : -1;

                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && overBox && !mEditorGizmoDragging)
                    {
                        PushEditorUndo();
                        mEditorGizmoDragging = false;
                        if (shift) { mEditorCollisionDragMove = true;  mEditorCollisionAxis = -1; }
                        else       { mEditorCollisionDragMove = false; mEditorCollisionAxis = hitAxis; mEditorCollisionSide = hitSide; }
                    }

                    const bool dragging = mEditorCollisionDragMove || mEditorCollisionAxis >= 0;
                    if (dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                    {
                        const float fine = shift ? 0.2f : 1.0f;
                        if (mEditorCollisionDragMove)
                        {
                            // Move o offset no plano da camera (Shift+arrastar).
                            const Vector3 camForward = Vector3Normalize(Vector3Subtract(mEditorCamera.target, mEditorCamera.position));
                            const Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, {0,1,0}));
                            const Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, camForward));
                            const Vector3 worldDelta = Vector3Add(Vector3Scale(camRight, mouseDelta.x * 0.02f * fine),
                                                                  Vector3Scale(camUp, -mouseDelta.y * 0.02f * fine));
                            const Vector3 localDelta = TransformDir(MatrixInvert(MatrixRotateXYZ(rotRad)), worldDelta);
                            boxOffset.x += localDelta.x; boxOffset.y += localDelta.y; boxOffset.z += localDelta.z;
                        }
                        else
                        {
                            // Redimensiona na direcao da face arrastada.
                            const Vector3 axisWorld[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
                            const int a = mEditorCollisionAxis;
                            const Vector3 outward = Vector3Scale(axisWorld[a], mEditorCollisionSide);
                            const Vector2 screenA = GetWorldToScreen(boxCenter, mEditorCamera);
                            const Vector2 screenB = GetWorldToScreen(Vector3Add(boxCenter, outward), mEditorCamera);
                            Vector2 screenOut = Vector2Subtract(screenB, screenA);
                            const float len = sqrtf(screenOut.x*screenOut.x + screenOut.y*screenOut.y);
                            if (len > 0.0001f) screenOut = Vector2Scale(screenOut, 1.0f / len);
                            const float amount = (mouseDelta.x * screenOut.x + mouseDelta.y * screenOut.y) * 0.03f * fine;
                            float &dim = (a == 0) ? boxSize.x : (a == 1) ? boxSize.y : boxSize.z;
                            dim = std::max(0.05f, dim + amount);
                        }
                        mNature->SetEditorCollisionBox(entry.natureId, boxSize, boxOffset);
                        entry.collisionBoxSize = boxSize; entry.collisionBoxOffset = boxOffset;
                        mLevelDirty = true;
                    }

                    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                    { mEditorCollisionDragMove = false; mEditorCollisionAxis = -1; }
                    if (wheel != 0.0f)
                    {
                        // Roda do mouse: escala a caixa inteira (Shift suaviza).
                        float factor = powf(1.1f, wheel);
                        if (shift) factor = 1.0f + (factor - 1.0f) * 0.2f;
                        boxSize.x = std::max(0.05f, boxSize.x * factor);
                        boxSize.y = std::max(0.05f, boxSize.y * factor);
                        boxSize.z = std::max(0.05f, boxSize.z * factor);
                        mNature->SetEditorCollisionBox(entry.natureId, boxSize, boxOffset);
                        entry.collisionBoxSize = boxSize; entry.collisionBoxOffset = boxOffset;
                        mLevelDirty = true;
                    }
                }
                if (mEditorCollisionEdit) return;
            }
        }

        // Manipuladores de viewport: as pontas coloridas controlam o eixo ativo.
        if (!io.WantCaptureMouse && mEditorSelected >= 0 &&
            mEditorSelected < (int)mLevelEntries.size() && !mEditorBrushEnabled)
        {
            LevelEntry &entry = mLevelEntries[mEditorSelected];
            const Vector3 origin = {entry.position.x, entry.position.y + 0.08f, entry.position.z};
            const bool rotateGizmo = mEditorGizmoMode == 2 || mEditorGizmoMode == 4;
            if (!mEditorGizmoDragging)
                mEditorGizmoHover = PickGizmoAxis(origin, GetMousePosition(), rotateGizmo, mEditorCamera);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                const int axis = PickGizmoAxis(origin, GetMousePosition(), rotateGizmo, mEditorCamera);
                if (axis >= 0)
                { PushEditorUndo(); mEditorGizmoAxis = axis; mEditorGizmoDragging = true; }
            }
            if (mEditorGizmoDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            {
                const int worldAxis = kGizmoAxisToWorld[mEditorGizmoAxis];
                if (mEditorGizmoMode == 2 || (mEditorGizmoMode == 4 && alt))
                {
                    // Rotacao estilo Blender (arrasto angular): o objeto gira em
                    // torno do eixo do anel agarrado, seguindo a tangente do anel
                    // no ponto mais proximo do cursor. Cada anel (azul = altura/
                    // yaw, verde = frente, vermelho = lateral) tem o seu proprio
                    // gesto e o seu proprio resultado, em qualquer camera.
                    const Vector3 axisDir = kWorldAxisDir[worldAxis];
                    Vector3 u = { 0.0f, 1.0f, 0.0f };
                    if (fabsf(axisDir.y) > 0.9f) u = { 1.0f, 0.0f, 0.0f };
                    Vector3 v = Vector3Normalize(Vector3CrossProduct(axisDir, u));
                    u = Vector3Normalize(Vector3CrossProduct(v, axisDir));
                    const float ringRadius = kGizmoRingRadius[mEditorGizmoAxis];
                    const Vector2 mouse = GetMousePosition();
                    float bestAngle = 0.0f, bestDist = 1e30f;
                    for (int s = 0; s < 64; ++s)
                    {
                        const float angle = (float)s * 2.0f * PI / 64.0f;
                        const Vector3 point = Vector3Add(origin, Vector3Add(
                            Vector3Scale(u, cosf(angle) * ringRadius),
                            Vector3Scale(v, sinf(angle) * ringRadius)));
                        const Vector2 screen = GetWorldToScreen(point, mEditorCamera);
                        const float dx = screen.x - mouse.x, dy = screen.y - mouse.y;
                        const float d2 = dx*dx + dy*dy;
                        if (d2 < bestDist) { bestDist = d2; bestAngle = angle; }
                    }
                    const Vector3 tangent = Vector3Add(
                        Vector3Scale(u, -sinf(bestAngle) * ringRadius),
                        Vector3Scale(v, cosf(bestAngle) * ringRadius));
                    const Vector2 screenA = GetWorldToScreen(origin, mEditorCamera);
                    const Vector2 screenB = GetWorldToScreen(Vector3Add(origin, tangent), mEditorCamera);
                    Vector2 screenTangent = Vector2Subtract(screenB, screenA);
                    const float tangentLen = sqrtf(screenTangent.x*screenTangent.x +
                                                   screenTangent.y*screenTangent.y);
                    if (tangentLen > 0.0001f) screenTangent = Vector2Scale(screenTangent, 1.0f / tangentLen);
                    const float rotationAmount = (mouseDelta.x * screenTangent.x +
                                                  mouseDelta.y * screenTangent.y) * 0.022f;
                    entry.rotation[worldAxis] += rotationAmount * 24.0f;
                }
                else if (mEditorGizmoMode == 3 || (mEditorGizmoMode == 4 && control))
                    entry.scale[worldAxis] = std::max(0.0001f,
                        entry.scale[worldAxis] * expf((mouseDelta.x - mouseDelta.y) * 0.018f));
                else entry.position[worldAxis] += (mouseDelta.x - mouseDelta.y) * 0.018f;
                ApplyEditorTransform(); mLevelDirty = true;
            }
            if (mEditorGizmoDragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            { mEditorGizmoDragging = false; mEditorGizmoAxis = -1; }
            if (mEditorGizmoDragging) return;
        }

        if (!placementWheelHandled && mEditorSelected >= 0 &&
            mEditorSelected < (int)mLevelEntries.size() && wheel != 0.0f)
        {
            LevelEntry &entry = mLevelEntries[mEditorSelected];
            bool transformed = false;
            if (control)
            {
                PushEditorUndo(); transformed = true;
                // Exponencial: permite qualquer dimensao positiva sem um teto artificial.
                entry.scale *= powf(1.12f, wheel);
                entry.scale = glm::max(entry.scale, glm::vec3(0.0001f));
            }
            else if (alt && shift) { PushEditorUndo(); transformed = true; entry.rotation.z += wheel * 5.0f; }
            else if (alt) { PushEditorUndo(); transformed = true; entry.rotation.y += wheel * 5.0f; }
            else if (shift) { PushEditorUndo(); transformed = true; entry.rotation.x += wheel * 5.0f; }
            else if (mEditorBrushEnabled)
                mEditorBrushRadius = std::max(0.01f, mEditorBrushRadius * powf(1.15f, wheel));
            if (transformed) { ApplyEditorTransform(); mLevelDirty = true; }
        }
        else if (!placementWheelHandled && mEditorBrushEnabled && wheel != 0.0f)
            mEditorBrushRadius = std::max(0.01f, mEditorBrushRadius * powf(1.15f, wheel));

        // Fluxo G+X/Y/Z: o movimento acompanha o mouse ate outro clique ou Escape.
        if (!io.WantCaptureKeyboard)
        {
            if (IsKeyPressed(KEY_G))
            { if (mEditorSelected >= 0) PushEditorUndo(); mEditorGizmoAxis = -2; }
            if (mEditorGizmoAxis == -2)
            {
                if (IsKeyPressed(KEY_X)) mEditorGizmoAxis = 0;
                if (IsKeyPressed(KEY_Y)) mEditorGizmoAxis = 1;
                if (IsKeyPressed(KEY_Z)) mEditorGizmoAxis = 2;
            }
            if (mEditorGizmoAxis >= 0 && mEditorSelected >= 0)
            {
                const float amount = (mouseDelta.x - mouseDelta.y) * 0.025f * (shift ? 0.2f : 1.0f);
                if (amount != 0.0f)
                {
                    const int worldAxis = kGizmoAxisToWorld[mEditorGizmoAxis];
                    mLevelEntries[mEditorSelected].position[worldAxis] += amount;
                    ApplyEditorTransform(); mLevelDirty = true;
                }
                if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) mEditorGizmoAxis = -1;
            }
        }
        if (!mEditorHitValid || io.WantCaptureMouse || alt) return;

        if (mEditorBrushEnabled && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mTerraformCooldown <= 0.0f)
        {
            if (!mEditorStrokeActive) { PushEditorUndo(); mEditorStrokeActive = true; }
            if (mEditorTerrainMode == 0)
                mTerrain->Terraform(*this, mEditorHit.x, mEditorHit.z, mEditorBrushRadius,
                                    (control ? -1.0f : 1.0f) * mEditorBrushStrength);
            else if (mEditorTerrainMode == 2)
                mTerrain->EraseTexture(mEditorHit.x, mEditorHit.z, mEditorBrushRadius,
                                       mEditorBrushStrength * 0.22f);
            else
                mTerrain->PaintTexture(mEditorHit.x, mEditorHit.z, mEditorBrushRadius,
                                       mEditorBrushStrength * 0.22f, mEditorPaintLayer);
            mTerraformCooldown = 0.09f;
            mLevelDirty = true;
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            const int found = FindEditorEntry(mEditorHit);
            if (found >= 0)
            {
                if (control)
                {
                    // Ctrl + clique direito: remove o objeto (comportamento original).
                    PushEditorUndo();
                    mEditorSelected = found;
                    RemoveEditorObject(mLevelEntries[found].position, mLevelEntries[found].type);
                }
                else
                {
                    // Clique direito isolado: seleciona e abre o menu de contexto
                    // com as ferramentas do objeto (caixa de colisao, etc).
                    mEditorSelected = found;
                    mEditorGizmoAxis = -1;
                    mEditorGizmoDragging = false;
                    mEditorCollisionEdit = false;
                    mEditorCollisionAxis = -1;
                    mEditorCollisionDragMove = false;
                    mEditorContextOpen = true;
                    mEditorContextEntry = found;
                }
            }
        }
        else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            if (mEditorTool == 0)
            {
                mEditorSelected = FindEditorEntry(mEditorHit);
                mEditorGizmoAxis = -1;
            }
            else if (mEditorTool == 1 && mNature)
            {
                PushEditorUndo();
                LevelEntry entry; entry.type = 1; entry.asset = mEditorProp; entry.position = mEditorHit;
                entry.scale = glm::vec3(mEditorPropPlacementScale);
                entry.rotation.y = mEditorPropPlacementRotation;
                entry.natureId = mNature->AddEditorInstance(*this, mEditorProp, mEditorHit,
                    entry.scale, entry.rotation);
                mLevelEntries.push_back(entry);
                mEditorSelected = (int)mLevelEntries.size() - 1;
                mLevelDirty = true;
            }
            else if (mEditorTool == 2)
            {
                PushEditorUndo();
                LevelEntry entry; entry.type = 2; entry.asset = mEditorEnemy;
                entry.level = mEditorEnemyLevel; entry.position = mEditorHit;
                entry.runtimeObject = SpawnEditorEnemy(mEditorHit, mEditorEnemy, mEditorEnemyLevel);
                mLevelEntries.push_back(entry); mEditorSelected = (int)mLevelEntries.size() - 1;
                mLevelDirty = true;
            }
            else if (mEditorTool == 3)
            {
                PushEditorUndo();
                const ItemType items[] = {ItemType::HealthPotion, ItemType::IronAxe,
                                          ItemType::LeatherArmor, ItemType::RangerArmor};
                PickupObject *pickup = SpawnObject<PickupObject>(mEditorHit, items[mEditorItem]);
                mPickups.push_back(pickup);
                LevelEntry entry; entry.type = 3; entry.asset = mEditorItem;
                entry.position = mEditorHit; entry.runtimeObject = pickup;
                mLevelEntries.push_back(entry); mEditorSelected = (int)mLevelEntries.size() - 1;
                mLevelDirty = true;
            }
        }
    }

    void GameScene::DrawLevelEditor()
    {
        const ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)GetScreenWidth(), 54));
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::Begin("##EditorToolbar", nullptr, toolbarFlags | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "EDITOR DE FASES");
        ImGui::SameLine(155);
        const char *tools[] = {"Selecionar", "Props", "Inimigos", "Itens"};
        for (int i = 0; i < 4; ++i)
        {
            if (i) ImGui::SameLine();
            if (ImGui::Selectable(tools[i], mEditorTool == i, 0, ImVec2(82, 28)))
            { mEditorTool = i; mEditorBrushEnabled = false; }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Brush", &mEditorBrushEnabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        int phase = mEditorLevel;
        if (ImGui::SliderInt("Fase", &phase, 1, 20)) SwitchEditorLevel(phase);
        ImGui::SameLine();
        if (ImGui::Button("Salvar")) SaveLevel();
        ImGui::SameLine();
        if (ImGui::Button("Desfazer")) UndoEditor();
        ImGui::SameLine();
        if (ImGui::Button("Refazer")) RedoEditor();
        ImGui::SameLine(); ImGui::TextDisabled("F1 fecha");
        ImGui::End();

        auto beginEditorPanel = [&](const char *title, int &dock, float &extent,
                                    const ImVec2 &floatingPosition, const ImVec2 &floatingSize)
        {
            const float width = (float)GetScreenWidth();
            const float height = (float)GetScreenHeight();
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
            extent = std::max(170.0f, extent);
            if (dock == 0)
            {
                ImGui::SetNextWindowPos(floatingPosition, ImGuiCond_FirstUseEver);
                ImGui::SetNextWindowSize(floatingSize, ImGuiCond_FirstUseEver);
            }
            else
            {
                flags |= ImGuiWindowFlags_NoMove;
                if (dock == 1)
                {
                    extent = std::min(extent, width - 80.0f);
                    ImGui::SetNextWindowPos(ImVec2(0, 54), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(extent, height - 54), ImGuiCond_Always);
                }
                else if (dock == 2)
                {
                    extent = std::min(extent, width - 80.0f);
                    ImGui::SetNextWindowPos(ImVec2(width - extent, 54), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(extent, height - 54), ImGuiCond_Always);
                }
                else if (dock == 3)
                {
                    extent = std::min(extent, height - 110.0f);
                    ImGui::SetNextWindowPos(ImVec2(0, 54), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(width, extent), ImGuiCond_Always);
                }
                else
                {
                    extent = std::min(extent, height - 110.0f);
                    ImGui::SetNextWindowPos(ImVec2(0, height - extent), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(width, extent), ImGuiCond_Always);
                }
            }
            ImGui::SetNextWindowBgAlpha(0.98f);
            ImGui::Begin(title, nullptr, flags);
            if (dock != 0)
            {
                const ImVec2 actual = ImGui::GetWindowSize();
                extent = (dock <= 2) ? actual.x : actual.y;
            }
            const char *dockNames[] = {"Flutuante", "Esquerda", "Direita", "Superior", "Inferior"};
            ImGui::SetNextItemWidth(130.0f);
            ImGui::Combo("Acoplamento", &dock, dockNames, 5);
            ImGui::Separator();
        };

        beginEditorPanel("Biblioteca", mEditorLibraryDock, mEditorLibraryExtent,
                         ImVec2(10, 64), ImVec2(350, 620));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Pesquisar por nome...", mEditorSearch, sizeof(mEditorSearch));
        if (mEditorBrushEnabled)
        {
            ImGui::TextColored(ImVec4(0.35f,0.75f,1,1), "Brush de terreno ativo");
            if (ImGui::Selectable("Esculpir", mEditorTerrainMode == 0, 0, ImVec2(145,30))) mEditorTerrainMode = 0;
            ImGui::SameLine();
            if (ImGui::Selectable("Pintar texturas", mEditorTerrainMode == 1, 0, ImVec2(145,30))) mEditorTerrainMode = 1;
            ImGui::SameLine();
            if (ImGui::Selectable("Borracha", mEditorTerrainMode == 2, 0, ImVec2(145,30))) mEditorTerrainMode = 2;
            if (mEditorTerrainMode == 0)
                ImGui::TextUnformatted("Clique: elevar | Ctrl+clique: abaixar");
            else if (mEditorTerrainMode == 2)
                ImGui::TextUnformatted("Clique: apagar a textura pintada");
            else if (mTerrain)
            {
                const char *names[] = {"Grama", "Terra", "Areia", "Pavimento"};
                ImGui::TextUnformatted("Camada do stencil map");
                for (int i = 0; i < 4; ++i)
                {
                    ImGui::PushID(i);
                    const Texture2D texture = mTerrain->GetLayerTexture(i);
                    if (texture.id && ImGui::ImageButton("##terrainLayer",
                        ImTextureRef((ImTextureID)texture.id), ImVec2(62,62), ImVec2(0,1), ImVec2(1,0)))
                        mEditorPaintLayer = i;
                    if (i < 3) ImGui::SameLine();
                    ImGui::PopID();
                }
                for (int i = 0; i < 4; ++i)
                {
                    ImGui::PushID(100 + i);
                    float scale = mTerrain->GetUVScales()[i];
                    if (ImGuiValueEditFloat(names[i], &scale, 0.001f, 1.0e6f, 0.1f, "UV %.2f"))
                    {
                        PushEditorUndo();
                        float scales[4]; for (int c = 0; c < 4; ++c) scales[c] = mTerrain->GetUVScales()[c];
                        scales[i] = std::max(0.001f, scale); mTerrain->SetUVScales(scales);
                        mLevelDirty = true;
                    }
                    ImGui::PopID();
                }
                ImGui::TextColored(ImVec4(0.5f,0.8f,1,1), "Selecionada: %s", names[mEditorPaintLayer]);
            }
            if (ImGuiValueEditFloat("Raio", &mEditorBrushRadius, 0.01f, 1.0e6f, 0.1f, "%.3f m"))
                mEditorBrushRadius = std::max(0.01f, mEditorBrushRadius);
            ImGuiValueEditFloat("Forca", &mEditorBrushStrength, 0.1f, 2.0f, 0.05f, "%.2f");
            ImGui::TextDisabled("Scroll ajusta o raio sem limite maximo.");
        }
        else if (mEditorTool == 1 && mNature)
        {
            if (ImGui::Button("Importar arquivo FBX...", ImVec2(-1, 34)))
            {
                const std::filesystem::path selectedFile = SelectFBXFile();
                if (!selectedFile.empty())
                {
                    const int imported = mNature->ImportFBX(selectedFile);
                    if (imported >= 0)
                    {
                        mEditorProp = imported;
                        mEditorPropPlacementScale = 1.0f;
                        mEditorPropPlacementRotation = 0.0f;
                        std::snprintf(mEditorImportStatus, sizeof(mEditorImportStatus),
                            "Importado: %s", selectedFile.filename().string().c_str());
                    }
                    else std::snprintf(mEditorImportStatus, sizeof(mEditorImportStatus),
                        "Falha ao importar o FBX selecionado.");
                }
            }
            if (mEditorImportStatus[0])
                ImGui::TextWrapped("%s", mEditorImportStatus);
            ImGui::TextColored(ImVec4(0.5f,0.8f,1,1), "Pre-colocacao: Z+scroll escala | C+scroll rotacao Y");
            if (ImGuiValueEditFloat("Escala da previa", &mEditorPropPlacementScale,
                                    0.0001f, 1.0e6f, 0.01f, "%.4f"))
                mEditorPropPlacementScale = std::max(0.0001f, mEditorPropPlacementScale);
            ImGuiValueEditFloat("Rotacao Y da previa", &mEditorPropPlacementRotation,
                                -360.0f, 360.0f, 0.5f, "%.1f");
            if (ImGui::Button("Restaurar previa", ImVec2(-1, 28)))
            { mEditorPropPlacementScale = 1.0f; mEditorPropPlacementRotation = 0.0f; }
            std::string filter = mEditorSearch;
            std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
            ImGui::BeginChild("PropsCatalog", ImVec2(0, -1), true);
            for (int i = 0; i < mNature->GetAssetCount(); ++i)
            {
                std::string name = mNature->GetAssetName(i), lowered = name;
                std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
                if (!filter.empty() && lowered.find(filter) == std::string::npos) continue;
                ImGui::PushID(i); const Texture2D thumbnail = mNature->GetThumbnail(i);
                if (thumbnail.id && ImGui::ImageButton("##prop", ImTextureRef((ImTextureID)thumbnail.id),
                    ImVec2(60,60), ImVec2(0,1), ImVec2(1,0)))
                { if (mEditorProp != i) { mEditorPropPlacementScale = 1.0f; mEditorPropPlacementRotation = 0.0f; } mEditorProp = i; }
                ImGui::SameLine();
                if (ImGui::Selectable(name.c_str(), mEditorProp == i, 0, ImVec2(0,60)))
                { if (mEditorProp != i) { mEditorPropPlacementScale = 1.0f; mEditorPropPlacementRotation = 0.0f; } mEditorProp = i; }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        else if (mEditorTool == 2)
        {
            const char *names[] = {"Blob Verde", "Cactoro", "Orc Pequeno", "Yeti",
                "Demonio Azul", "Dinossauro", "Rei Cogumelo", "Orc Caveira",
                "Dragao", "Fantasma", "Armabelha", "Lula Voadora"};
            ImGuiValueEditInt("Nivel", &mEditorEnemyLevel, 1, 50, 1, "%d");
            std::string filter = mEditorSearch;
            std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
            ImGui::BeginChild("EnemyCatalog", ImVec2(0, -42), true);
            for (int i = 0; i < 12; ++i)
            {
                std::string lowered = names[i];
                std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
                if (!filter.empty() && lowered.find(filter) == std::string::npos) continue;
                ImGui::PushID(i);
                const Texture2D thumbnail = GetEnemyEditorThumbnail(i);
                if (thumbnail.id && ImGui::ImageButton("##enemy", ImTextureRef((ImTextureID)thumbnail.id),
                    ImVec2(60,60), ImVec2(0,1), ImVec2(1,0))) mEditorEnemy = i;
                ImGui::SameLine();
                char label[96]; std::snprintf(label, sizeof(label), "%s  | Nivel %d", names[i], mEditorEnemyLevel);
                if (ImGui::Selectable(label, mEditorEnemy == i, 0, ImVec2(0,60))) mEditorEnemy = i;
                ImGui::PopID();
            }
            ImGui::EndChild();
            if (ImGui::Button("Resetar todos os inimigos", ImVec2(-1, 34)))
                for (EnemyObject *enemy : mEnemies) if (enemy) enemy->ResetToSpawn();
        }
        else if (mEditorTool == 3)
        {
            const char *items[] = {"Pocao de vida", "Machado", "Armadura de couro", "Armadura ranger"};
            for (int i = 0; i < 4; ++i)
            {
                ImGui::PushID(i);
                if (mItemIcons[i].id && ImGui::ImageButton("##item", ImTextureRef((ImTextureID)mItemIcons[i].id),
                    ImVec2(60,60), ImVec2(0,1), ImVec2(1,0))) mEditorItem = i;
                ImGui::SameLine();
                if (ImGui::Selectable(items[i], mEditorItem == i, 0, ImVec2(0,60))) mEditorItem = i;
                ImGui::PopID();
            }
        }
        ImGui::Separator();
        ImGui::TextDisabled("Ctrl+RMB remove | RMB ferramentas | WASD/QE voa | MMB olha");
        ImGui::End();

        beginEditorPanel("Transformacao", mEditorTransformDock, mEditorTransformExtent,
                         ImVec2((float)GetScreenWidth() - 350, 64), ImVec2(340, 350));
        const char *gizmos[] = {"Mover", "Rotacionar", "Escala", "Completo"};
        for (int i = 0; i < 4; ++i) { if (i) ImGui::SameLine(); if (ImGui::Selectable(gizmos[i], mEditorGizmoMode == i + 1, 0, ImVec2(62,24))) mEditorGizmoMode = i + 1; }
        if (mEditorSelected >= 0 && mEditorSelected < (int)mLevelEntries.size())
        {
            LevelEntry &entry = mLevelEntries[mEditorSelected];
            EditorSnapshot beforeTransform = CaptureEditorSnapshot();
            bool changed = false;
            changed |= ImGuiValueEditVec3("Posicao", &entry.position.x, -1.0e6f, 1.0e6f, 0.05f, "%.2f");
            changed |= ImGuiValueEditVec3("Rotacao", &entry.rotation.x, -360.0f, 360.0f, 0.5f, "%.1f");
            changed |= ImGuiValueEditVec3("Escala", &entry.scale.x, 0.0001f, 1.0e6f, 0.02f, "%.2f");
            if (changed)
            {
                mEditorUndo.push_back(std::move(beforeTransform));
                if (mEditorUndo.size() > 64) mEditorUndo.erase(mEditorUndo.begin());
                mEditorRedo.clear();
                entry.scale = glm::max(entry.scale, glm::vec3(0.0001f));
                ApplyEditorTransform(); mLevelDirty = true;
            }
            ImGui::Separator();
            if (entry.type == 1 && mNature && entry.natureId != 0)
            {
                bool collision = mNature->GetEditorCollision(entry.natureId);
                if (ImGui::Checkbox("Colisao", &collision))
                {
                    PushEditorUndo();
                    mNature->SetEditorCollision(entry.natureId, collision);
                    entry.collision = collision;
                    mLevelDirty = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Liga/desliga a colisao fisica deste prop. Desligar permite que o jogador passe atraves dele.");
                if (mEditorCollisionEdit)
                {
                    ImGui::TextColored(ImVec4(0.45f, 0.9f, 0.55f, 1.0f), "Editando caixa de colisao");
                    ImGui::TextWrapped("Arraste uma face para redimensionar; Shift+arraste para mover; roda do mouse para escalar (Shift suaviza). Esc sai.");
                    if (ImGui::Button("Restaurar caixa automatica"))
                    {
                        PushEditorUndo();
                        mNature->SetEditorCollisionBox(entry.natureId, glm::vec3(0.0f), glm::vec3(0.0f));
                        entry.collisionBoxSize = glm::vec3(0.0f);
                        entry.collisionBoxOffset = glm::vec3(0.0f);
                        mLevelDirty = true;
                    }
                }
                else if (ImGui::Button("Caixa de colisao"))
                {
                    mEditorCollisionEdit = true;
                    mEditorGizmoAxis = -1; mEditorGizmoDragging = false;
                }
            }
            ImGui::Separator();
            ImGui::TextWrapped("Ctrl+scroll escala proporcional; Alt+scroll gira Y; Shift+scroll gira X; Shift+Alt gira Z.");
            ImGui::TextWrapped("G e depois X, Y ou Z move no eixo (convencao Blender: Z = altura).");
        }
        else ImGui::TextWrapped("Selecione um objeto na cena para editar sua transformacao.");
        ImGui::End();

        // Menu de contexto do clique direito (ferramentas do objeto selecionado).
        if (mEditorContextOpen)
        {
            mEditorContextOpen = false;
            ImGui::OpenPopup("##EditorContext");
        }
        if (ImGui::BeginPopup("##EditorContext"))
        {
            const int idx = mEditorContextEntry;
            const bool valid = idx >= 0 && idx < (int)mLevelEntries.size();
            ImGui::TextColored(ImVec4(0.3f, 0.75f, 1.0f, 1.0f), "Ferramentas");
            ImGui::Separator();
            if (valid)
            {
                LevelEntry &entry = mLevelEntries[idx];
                if (entry.type == 1 && mNature && entry.natureId != 0)
                {
                    const bool editing = mEditorCollisionEdit && mEditorSelected == idx;
                    if (ImGui::MenuItem(editing ? "Caixa de colisao (editando)" : "Caixa de colisao",
                                        editing ? "Esc p/ sair" : nullptr))
                    {
                        mEditorSelected = idx;
                        mEditorCollisionEdit = !editing;
                        if (mEditorCollisionEdit)
                        { mEditorGizmoAxis = -1; mEditorGizmoDragging = false; }
                    }
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Mostra a caixa de colisao editavel ao redor do prop: arraste as faces para dimensionar, Shift+arraste para mover, roda para escalar.");
                    ImGui::Separator();
                }
                if (ImGui::MenuItem("Remover objeto", "Ctrl+clique direito"))
                {
                    PushEditorUndo();
                    RemoveEditorObject(entry.position, entry.type);
                }
            }
            ImGui::EndPopup();
        }
    }

    int GameScene::FindEditorEntry(const glm::vec3 &position, float radius) const
    {
        int found = -1; float closest = radius * radius;
        for (int i = 0; i < (int)mLevelEntries.size(); ++i)
        {
            const glm::vec2 delta(mLevelEntries[i].position.x - position.x,
                                  mLevelEntries[i].position.z - position.z);
            const float distance = glm::dot(delta, delta);
            if (distance < closest) { closest = distance; found = i; }
        }
        return found;
    }

    void GameScene::ApplyEditorTransform()
    {
        if (mEditorSelected < 0 || mEditorSelected >= (int)mLevelEntries.size()) return;
        LevelEntry &entry = mLevelEntries[mEditorSelected];
        if (entry.type == 1 && mNature)
            mNature->TransformEditorInstance(entry.natureId, entry.position, entry.scale, entry.rotation);
        else if (entry.runtimeObject)
        {
            if (entry.type == 2)
                static_cast<EnemyObject *>(entry.runtimeObject)->SetEditorTransform(
                    entry.position, entry.rotation, entry.scale);
            else if (entry.type == 3)
                static_cast<PickupObject *>(entry.runtimeObject)->SetEditorTransform(
                    entry.position, entry.rotation, entry.scale);
        }
    }

    GameScene::EditorSnapshot GameScene::CaptureEditorSnapshot() const
    {
        EditorSnapshot snapshot;
        if (!mTerrain) return snapshot;
        snapshot.heights = mTerrain->GetHeights();
        snapshot.stencil = mTerrain->GetStencilMap();
        for (int i = 0; i < 4; ++i) snapshot.uvScales[i] = mTerrain->GetUVScales()[i];
        snapshot.entries = mLevelEntries;
        for (LevelEntry &entry : snapshot.entries)
        { entry.natureId = 0; entry.runtimeObject = nullptr; }
        return snapshot;
    }

    void GameScene::PushEditorUndo()
    {
        mEditorUndo.push_back(CaptureEditorSnapshot());
        if (mEditorUndo.size() > 64) mEditorUndo.erase(mEditorUndo.begin());
        mEditorRedo.clear();
    }

    void GameScene::RestoreEditorSnapshot(const EditorSnapshot &snapshot)
    {
        ClearEditorLevelObjects();
        if (!mTerrain) return;
        mTerrain->SetHeights(*this, snapshot.heights);
        mTerrain->SetStencilMap(snapshot.stencil);
        mTerrain->SetUVScales(snapshot.uvScales.data());
        mLevelEntries = snapshot.entries;
        const ItemType items[] = {ItemType::HealthPotion, ItemType::IronAxe,
                                  ItemType::LeatherArmor, ItemType::RangerArmor};
        for (LevelEntry &entry : mLevelEntries)
        {
            entry.natureId = 0; entry.runtimeObject = nullptr;
            if (entry.type == 1 && mNature)
            {
                entry.natureId = mNature->AddEditorInstance(*this, entry.asset, entry.position,
                                                            entry.scale, entry.rotation,
                                                            entry.collision);
                if (entry.natureId != 0)
                    mNature->SetEditorCollisionBox(entry.natureId, entry.collisionBoxSize, entry.collisionBoxOffset);
            }
            else if (entry.type == 2)
            {
                EnemyObject *enemy = SpawnEditorEnemy(entry.position, entry.asset, entry.level);
                enemy->SetEditorTransform(entry.position, entry.rotation, entry.scale);
                entry.runtimeObject = enemy;
            }
            else if (entry.type == 3 && entry.asset >= 0 && entry.asset < 4)
            {
                PickupObject *pickup = SpawnObject<PickupObject>(entry.position, items[entry.asset]);
                pickup->SetEditorTransform(entry.position, entry.rotation, entry.scale);
                mPickups.push_back(pickup); entry.runtimeObject = pickup;
            }
        }
        mEditorSelected = -1; mLevelDirty = true;
    }

    void GameScene::UndoEditor()
    {
        if (mEditorUndo.empty()) return;
        mEditorRedo.push_back(CaptureEditorSnapshot());
        EditorSnapshot snapshot = std::move(mEditorUndo.back()); mEditorUndo.pop_back();
        RestoreEditorSnapshot(snapshot);
    }

    void GameScene::RedoEditor()
    {
        if (mEditorRedo.empty()) return;
        mEditorUndo.push_back(CaptureEditorSnapshot());
        EditorSnapshot snapshot = std::move(mEditorRedo.back()); mEditorRedo.pop_back();
        RestoreEditorSnapshot(snapshot);
    }

    void GameScene::DrawEditorGizmo() const
    {
        if (mEditorSelected < 0 || mEditorSelected >= (int)mLevelEntries.size()) return;
        const LevelEntry &selEntry = mLevelEntries[mEditorSelected];
        if (mEditorCollisionEdit)
        {
            // Caixa de colisao editavel no lugar do gizmo de transformacao.
            if (selEntry.type == 1 && mNature && selEntry.natureId != 0)
            {
                glm::vec3 boxSize, boxOffset;
                if (mNature->GetEditorCollisionBox(selEntry.natureId, boxSize, boxOffset))
                {
                    const Vector3 rotRad = {selEntry.rotation.x * DEG2RAD,
                                            selEntry.rotation.y * DEG2RAD,
                                            selEntry.rotation.z * DEG2RAD};
                    const Vector3 offsetWorld = Vector3Transform({boxOffset.x, boxOffset.y, boxOffset.z},
                                                                 MatrixRotateXYZ(rotRad));
                    const Vector3 center = {selEntry.position.x + offsetWorld.x,
                                            selEntry.position.y + offsetWorld.y,
                                            selEntry.position.z + offsetWorld.z};
                    const int hover = mEditorGizmoHover;
                    DrawBoxGrid(center, {boxSize.x, boxSize.y, boxSize.z}, rotRad,
                                0.028f * GizmoScreenScale(mEditorCamera, center),
                                Color{ 40, 220, 120, 255 }, Color{ 40, 220, 120, 90 },
                                mEditorCamera.position,
                                hover >= 0 ? hover / 2 : -1,
                                hover >= 0 && (hover % 2) ? 1.0f : -1.0f);
                }
            }
            return;
        }
        const glm::vec3 p = selEntry.position;
        const Vector3 origin = {p.x, p.y + 0.08f, p.z};
        // Blender convention: red X = right, green Y = forward (world Z),
        // blue Z = up (world Y).
        const Vector3 axes[3] = { {1.0f,0,0}, {0,0,1.0f}, {0,1.0f,0} };
        const bool move = mEditorGizmoMode == 1 || mEditorGizmoMode == 4;
        const bool rotate = mEditorGizmoMode == 2 || mEditorGizmoMode == 4;
        const bool scale = mEditorGizmoMode == 3 || mEditorGizmoMode == 4;
        const int hover = mEditorGizmoHover;
        const int active = mEditorGizmoDragging ? mEditorGizmoAxis : -1;

        // Tamanho constante na tela (como no Blender) e linhas grossas.
        const float gizmoScale = GizmoScreenScale(mEditorCamera, origin);
        const float lineLen = 1.15f * gizmoScale;
        const float thickness = 0.030f * gizmoScale;
        const float ringScale = 0.5f * gizmoScale;
        const float coneLen = 0.34f * gizmoScale;
        const float coneRadius = 0.10f * gizmoScale;
        const Vector3 cameraPos = mEditorCamera.position;

        rlDisableBackfaceCulling();
        if (rotate)
        {
            // Aneis grossos em cor unica solida. Azul (yaw) e o maior.
            const Vector3 ringNormals[3] = { {1,0,0}, {0,0,1}, {0,1,0} };
            for (int i = 0; i < 3; ++i)
            {
                Color color = kGizmoColors[i];
                if (active == i) color = kGizmoActiveColor;
                else if (hover == i) color = ColorLerp(color, WHITE, 0.45f);
                DrawGizmoRing(origin, kGizmoRingRadius[i] * ringScale, ringNormals[i],
                              thickness, color, cameraPos);
            }
        }
        if (move || scale)
        {
            for (int i = 0; i < 3; ++i)
            {
                Color color = kGizmoColors[i];
                if (active == i) color = kGizmoActiveColor;
                else if (hover == i) color = ColorLerp(color, WHITE, 0.45f);
                const Vector3 end = Vector3Add(origin, Vector3Scale(axes[i], lineLen));
                DrawGizmoLine(origin, end, thickness, color, cameraPos);
                if (scale)
                {
                    // Cubo na ponta (escala).
                    const float half = 0.17f * gizmoScale;
                    DrawCube(end, half * 2.0f, half * 2.0f, half * 2.0f, color);
                }
                else
                {
                    // Seta com ponta de cone.
                    const Vector3 tip = Vector3Add(end, Vector3Scale(axes[i], coneLen));
                    DrawGizmoCone(end, tip, coneRadius, color);
                }
            }
            // Ponto central discreto, como no Blender.
            DrawSphere(origin, 0.07f * gizmoScale, { 190, 195, 205, 255 });
        }
        rlEnableBackfaceCulling();
    }

    Texture2D GameScene::GetEnemyEditorThumbnail(int archetype)
    {
        if (archetype < 0 || archetype >= 12) return {};
        if (mEnemyEditorThumbnails[archetype].id) return mEnemyEditorThumbnails[archetype].texture;
        const char *categories[] = {"Blob","Blob","Blob","Blob","Big","Big","Big","Big",
                                    "Flying","Flying","Flying","Flying"};
        const char *models[] = {"GreenBlob","Cactoro","Orc","Yeti","BlueDemon","Dino",
                                "MushroomKing","Orc_Skull","Dragon","Ghost","Armabee","Squidle"};
        std::string path = std::string("Assets/Enemies/") + categories[archetype] + "/" + models[archetype] + ".gltf";
        if (!FileExists(path.c_str())) path = std::string("Game/Assets/Kit assets/Ultimate Monsters/") +
            categories[archetype] + "/glTF/" + models[archetype] + ".gltf";
        Model model = LoadModel(path.c_str());
        if (model.meshCount <= 0) return {};
        const BoundingBox bounds = GetModelBoundingBox(model);
        const float height = std::max(0.001f, bounds.max.y - bounds.min.y);
        const float scale = 1.7f / height;
        Camera3D camera = {}; camera.position = {2.4f, 1.5f, 2.4f};
        camera.target = {0,0.85f,0}; camera.up = {0,1,0}; camera.fovy = 38; camera.projection = CAMERA_PERSPECTIVE;
        mEnemyEditorThumbnails[archetype] = LoadRenderTexture(96, 96);
        BeginTextureMode(mEnemyEditorThumbnails[archetype]); ClearBackground({25,30,39,255});
        BeginMode3D(camera); DrawModelEx(model, {0,0,0}, {0,1,0}, 25, {scale,scale,scale}, WHITE); EndMode3D();
        EndTextureMode(); UnloadModel(model);
        return mEnemyEditorThumbnails[archetype].texture;
    }

    EnemyObject *GameScene::SpawnEditorEnemy(const glm::vec3 &position, int archetype, int level)
    {
        struct Entry { EnemyCategory category; const char *model; const char *name; };
        static const Entry entries[] = {
            {EnemyCategory::Blob,"GreenBlob","Blob Verde"},{EnemyCategory::Blob,"Cactoro","Cactoro"},
            {EnemyCategory::Blob,"Orc","Orc Pequeno"},{EnemyCategory::Blob,"Yeti","Yeti"},
            {EnemyCategory::Big,"BlueDemon","Demonio Azul"},{EnemyCategory::Big,"Dino","Dinossauro"},
            {EnemyCategory::Big,"MushroomKing","Rei Cogumelo"},{EnemyCategory::Big,"Orc_Skull","Orc Caveira"},
            {EnemyCategory::Flying,"Dragon","Dragao"},{EnemyCategory::Flying,"Ghost","Fantasma"},
            {EnemyCategory::Flying,"Armabee","Armabelha"},{EnemyCategory::Flying,"Squidle","Lula Voadora"}
        };
        archetype = std::max(0, std::min(11, archetype));
        EnemyConfig config;
        config.category = entries[archetype].category;
        config.modelName = entries[archetype].model;
        config.displayName = entries[archetype].name;
        config.level = level;
        config.maxHealth = 35.0f + level * 13.0f;
        config.attack = 5.0f + level * 1.8f;
        config.defense = 0.6f + level * 0.55f;
        config.moneyReward = 3 + level * 2;
        config.experienceReward = 12.0f + level * 7.0f;
        EnemyObject *enemy = SpawnObject<EnemyObject>(position, config, position.y);
        mEnemies.push_back(enemy);
        return enemy;
    }

    void GameScene::RemoveEditorObject(const glm::vec3 &position, int tool)
    {
        int index = -1; float closestSq = 3.0f * 3.0f;
        for (int i = 0; i < (int)mLevelEntries.size(); ++i)
        {
            if (tool > 0 && mLevelEntries[i].type != (unsigned char)tool) continue;
            const glm::vec2 delta(mLevelEntries[i].position.x - position.x,
                                  mLevelEntries[i].position.z - position.z);
            const float distanceSq = glm::dot(delta, delta);
            if (distanceSq < closestSq) { closestSq = distanceSq; index = i; }
        }
        if (index < 0) return;
        LevelEntry &entry = mLevelEntries[index];
        if (entry.type == 1 && mNature) mNature->RemoveEditorInstance(entry.natureId);
        else if (entry.runtimeObject)
        {
            if (entry.type == 2)
                mEnemies.erase(std::remove(mEnemies.begin(), mEnemies.end(),
                    static_cast<EnemyObject *>(entry.runtimeObject)), mEnemies.end());
            else if (entry.type == 3)
                mPickups.erase(std::remove(mPickups.begin(), mPickups.end(),
                    static_cast<PickupObject *>(entry.runtimeObject)), mPickups.end());
            mObjects.erase(std::remove(mObjects.begin(), mObjects.end(), entry.runtimeObject), mObjects.end());
            delete entry.runtimeObject;
        }
        mLevelEntries.erase(mLevelEntries.begin() + index);
        mEditorSelected = -1;
        mLevelDirty = true;
    }

    void GameScene::ClearEditorLevelObjects()
    {
        if (mNature) mNature->ClearEditorInstances();
        for (LevelEntry &entry : mLevelEntries)
        {
            if (!entry.runtimeObject) continue;
            if (entry.type == 2) mEnemies.erase(std::remove(mEnemies.begin(), mEnemies.end(),
                static_cast<EnemyObject *>(entry.runtimeObject)), mEnemies.end());
            if (entry.type == 3) mPickups.erase(std::remove(mPickups.begin(), mPickups.end(),
                static_cast<PickupObject *>(entry.runtimeObject)), mPickups.end());
            mObjects.erase(std::remove(mObjects.begin(), mObjects.end(), entry.runtimeObject), mObjects.end());
            delete entry.runtimeObject;
        }
        mLevelEntries.clear(); mEditorSelected = -1;
        if (mTerrain && !mEditorBaseHeights.empty()) mTerrain->SetHeights(*this, mEditorBaseHeights);
        if (mTerrain && !mEditorBaseStencil.empty()) mTerrain->SetStencilMap(mEditorBaseStencil);
        if (mTerrain) mTerrain->SetUVScales(mEditorBaseUVScales.data());
    }

    void GameScene::SwitchEditorLevel(int level)
    {
        level = std::max(1, std::min(20, level));
        if (level == mEditorLevel) return;
        if (mLevelDirty) SaveLevel();
        ClearEditorLevelObjects();
        mEditorLevel = level;
        LoadLevel();
        mEditorUndo.clear(); mEditorRedo.clear();
    }

    void GameScene::SaveLevel()
    {
        if (!mTerrain) return;
        const std::filesystem::path path = LevelFilePath(mEditorLevel);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) return;
        struct DiskEntry { unsigned char type; int asset; int level; glm::vec3 position; glm::vec3 scale; glm::vec3 rotation; unsigned char collision; glm::vec3 collisionBoxSize; glm::vec3 collisionBoxOffset; };
        const unsigned int magic = 0x364C4445;
        const unsigned int heightCount = (unsigned int)mTerrain->GetHeights().size();
        const unsigned int stencilCount = (unsigned int)mTerrain->GetStencilMap().size();
        const unsigned int entryCount = (unsigned int)mLevelEntries.size();
        file.write((const char *)&magic, sizeof(magic));
        file.write((const char *)&heightCount, sizeof(heightCount));
        file.write((const char *)mTerrain->GetHeights().data(), heightCount * sizeof(float));
        file.write((const char *)&stencilCount, sizeof(stencilCount));
        file.write((const char *)mTerrain->GetStencilMap().data(), stencilCount * sizeof(Color));
        file.write((const char *)mTerrain->GetUVScales(), 4 * sizeof(float));
        file.write((const char *)&entryCount, sizeof(entryCount));
        for (const LevelEntry &entry : mLevelEntries)
        {
            const DiskEntry disk = {entry.type, entry.asset, entry.level, entry.position, entry.scale, entry.rotation,
                                    (unsigned char)(entry.collision ? 1 : 0),
                                    entry.collisionBoxSize, entry.collisionBoxOffset};
            file.write((const char *)&disk, sizeof(disk));
        }
        mLevelDirty = false;
    }

    void GameScene::LoadLevel()
    {
        const std::filesystem::path path = LevelFilePath(mEditorLevel);
        std::ifstream file(path, std::ios::binary);
        if (!file || !mTerrain) return;
        unsigned int magic = 0, heightCount = 0, entryCount = 0;
        file.read((char *)&magic, sizeof(magic));
        file.read((char *)&heightCount, sizeof(heightCount));
        if ((magic != 0x314C4445 && magic != 0x324C4445 && magic != 0x334C4445 &&
             magic != 0x344C4445 && magic != 0x354C4445 && magic != 0x364C4445) || heightCount > 1000000) return;
        std::vector<float> heights(heightCount);
        file.read((char *)heights.data(), heightCount * sizeof(float));
        mTerrain->SetHeights(*this, heights);
        if (magic == 0x334C4445 || magic == 0x344C4445)
        {
            unsigned int stencilCount = 0; file.read((char *)&stencilCount, sizeof(stencilCount));
            if (stencilCount > 4000000) return;
            std::vector<Color> stencil(stencilCount);
            file.read((char *)stencil.data(), stencilCount * sizeof(Color));
            // A primeira versao do sistema gravava o mapa inteiro como grama pintada.
            // Converte somente esse estado inicial conhecido para stencil vazio.
            if (magic == 0x334C4445 && !stencil.empty() &&
                std::all_of(stencil.begin(), stencil.end(), [](Color c)
                    { return c.r == 255 && c.g == 0 && c.b == 0 && c.a == 0; }))
                std::fill(stencil.begin(), stencil.end(), Color{0,0,0,0});
            float scales[4] = {}; file.read((char *)scales, sizeof(scales));
            mTerrain->SetStencilMap(stencil); mTerrain->SetUVScales(scales);
        }
        file.read((char *)&entryCount, sizeof(entryCount));
        if (entryCount > 100000) return;
        struct OldEntry { unsigned char type; int asset; int level; glm::vec3 position; float scale; float yaw; };
        struct DiskEntry { unsigned char type; int asset; int level; glm::vec3 position; glm::vec3 scale; glm::vec3 rotation; unsigned char collision; glm::vec3 collisionBoxSize; glm::vec3 collisionBoxOffset; };
        mLevelEntries.clear(); mLevelEntries.reserve(entryCount);
        for (unsigned int i = 0; i < entryCount; ++i)
        {
            LevelEntry entry;
            if (magic == 0x314C4445)
            {
                OldEntry old = {}; file.read((char *)&old, sizeof(old));
                entry.type = old.type; entry.asset = old.asset; entry.level = old.level;
                entry.scale = glm::vec3(old.scale);
                entry.position = old.position;
                entry.rotation.z = old.yaw;
            }
            else
            {
                DiskEntry disk = {}; file.read((char *)&disk, sizeof(disk));
                entry.type = disk.type; entry.asset = disk.asset; entry.level = disk.level;
                entry.scale = disk.scale;
                entry.position = disk.position;
                entry.rotation = disk.rotation;
                entry.collision = (magic >= 0x354C4445) ? (disk.collision != 0) : true;
                if (magic == 0x364C4445)
                {
                    entry.collisionBoxSize = disk.collisionBoxSize;
                    entry.collisionBoxOffset = disk.collisionBoxOffset;
                }
            }
            mLevelEntries.push_back(entry);
        }
        const ItemType items[] = {ItemType::HealthPotion, ItemType::IronAxe,
                                  ItemType::LeatherArmor, ItemType::RangerArmor};
        for (LevelEntry &entry : mLevelEntries)
        {
            if (entry.type == 1 && mNature)
            {
                entry.natureId = mNature->AddEditorInstance(*this, entry.asset, entry.position, entry.scale, entry.rotation,
                                                           entry.collision);
                if (entry.natureId != 0)
                    mNature->SetEditorCollisionBox(entry.natureId, entry.collisionBoxSize, entry.collisionBoxOffset);
            }
            else if (entry.type == 2)
            {
                EnemyObject *enemy = SpawnEditorEnemy(entry.position, entry.asset, entry.level);
                enemy->SetEditorTransform(entry.position, entry.rotation, entry.scale);
                entry.runtimeObject = enemy;
            }
            else if (entry.type == 3 && entry.asset >= 0 && entry.asset < 4)
            {
                PickupObject *pickup = SpawnObject<PickupObject>(entry.position, items[entry.asset]);
                pickup->SetEditorTransform(entry.position, entry.rotation, entry.scale);
                mPickups.push_back(pickup); entry.runtimeObject = pickup;
            }
        }
        mLevelDirty = false;
    }
#endif
}
