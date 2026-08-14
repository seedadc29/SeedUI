#ifndef GAMESCENE_H
#define GAMESCENE_H

#include "Scene.h"
#include "UI/SettingsScreen.h"
#include "Inventory.h"

#include <vector>
#include <array>

namespace game
{
    class GameScene : public Scene
    {
    public:
        GameScene(Settings &settings);
        ~GameScene() override;

        void Init() override;
        void Shutdown() override;
        void Update(float deltaTime) override;
        void Draw() override;
        void DrawWorld() override;
        void DrawOverlay() override;
        void DrawDebug() override;
        void SpawnDamageFeedback(const glm::vec3 &position, float amount) override;

        void ApplySettings();

        bool IsPaused() const { return mPaused; }

    private:
        void DrawPauseMenu();
        void DrawHud();
        void DrawInventory();
        class PickupObject *GetNearestPickup(float maxDistance) const;
        void ToggleInventory();
        Texture2D GetItemIcon(ItemType type) const;
        void UpdateCombatEffects(float deltaTime);
#if !defined(NDEBUG)
        void ToggleLevelEditor();
        void UpdateLevelEditor(float deltaTime);
        void DrawLevelEditor();
        void DrawCrtVisualPanel();
        void DrawSynthVisualPanel();
        void DrawDevTools();
        void SaveLevel();
        void LoadLevel();
        void SwitchEditorLevel(int level);
        void ClearEditorLevelObjects();
        void ApplyEditorTransform();
        int FindEditorEntry(const glm::vec3 &position, float radius = 3.0f) const;
        void DrawEditorGizmo() const;
        Texture2D GetEnemyEditorThumbnail(int archetype);
        void PushEditorUndo();
        void UndoEditor();
        void RedoEditor();
        class EnemyObject *SpawnEditorEnemy(const glm::vec3 &position, int archetype, int level);
        void RemoveEditorObject(const glm::vec3 &position, int tool);
#endif

        bool mPaused = false;
        bool mDevToolsOpen = false;
        bool mPauseSettingsOpen = false;
        bool mInventoryOpen = false;
        int mInventoryDrag = -1;
        float mToastTime = 0.0f;
        char mToast[96] = {};
        Texture2D mItemIcons[4] = {};
        std::vector<class PickupObject *> mPickups;
        std::vector<class EnemyObject *> mEnemies;
        class TerrainObject *mTerrain = nullptr;
        class NatureObject *mNature = nullptr;
        struct BloodParticle
        {
            glm::vec3 position;
            glm::vec3 velocity;
            float life = 0.0f;
        };
        struct DamageNumber
        {
            glm::vec3 position;
            int amount = 0;
            float life = 0.0f;
        };
        std::vector<BloodParticle> mBloodParticles;
        std::vector<DamageNumber> mDamageNumbers;
#if !defined(NDEBUG)
        bool mShowDebugDraw = false;
        bool mEditorActive = false;
        Camera3D mEditorCamera = {};
        float mEditorYaw = 0.0f;
        float mEditorPitch = -0.45f;
        float mEditorSpeed = 14.0f;
        int mEditorTool = 0;
        bool mEditorBrushEnabled = false;
        int mEditorTerrainMode = 0;
        int mEditorPaintLayer = 0;
        bool mEditorStrokeActive = false;
        int mEditorGizmoMode = 1;
        int mEditorGizmoAxis = -1;
        int mEditorGizmoHover = -1;
        bool mEditorGizmoDragging = false;
        int mEditorSelected = -1;
        bool mEditorCollisionEdit = false;
        int mEditorCollisionAxis = -1;
        float mEditorCollisionSide = 1.0f;
        bool mEditorCollisionDragMove = false;
        bool mEditorContextOpen = false;
        int mEditorContextEntry = -1;
        int mEditorLevel = 1;
        char mEditorSearch[64] = {};
        char mEditorImportStatus[160] = {};
        char mSynthPatchStatus[192] = {};
        RenderTexture2D mEnemyEditorThumbnails[12] = {};
        int mEditorProp = 0;
        float mEditorPropPlacementScale = 1.0f;
        float mEditorPropPlacementRotation = 0.0f;
        int mEditorEnemy = 0;
        int mEditorEnemyLevel = 1;
        int mEditorItem = 0;
        float mEditorBrushRadius = 6.0f;
        float mEditorBrushStrength = 0.7f;
        float mTerraformCooldown = 0.0f;
        int mEditorLibraryDock = 1;
        int mEditorTransformDock = 2;
        float mEditorLibraryExtent = 350.0f;
        float mEditorTransformExtent = 300.0f;
        bool mEditorHitValid = false;
        glm::vec3 mEditorHit = glm::vec3(0.0f);
        bool mLevelDirty = false;
        std::vector<float> mEditorBaseHeights;
        std::vector<Color> mEditorBaseStencil;
        std::array<float, 4> mEditorBaseUVScales = {18,18,18,18};
        struct LevelEntry
        {
            unsigned char type = 0;
            int asset = 0;
            int level = 1;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            glm::vec3 rotation = glm::vec3(0.0f);
            bool collision = true;
            glm::vec3 collisionBoxSize = glm::vec3(0.0f);
            glm::vec3 collisionBoxOffset = glm::vec3(0.0f);
            unsigned int natureId = 0;
            GameObject *runtimeObject = nullptr;
        };
        std::vector<LevelEntry> mLevelEntries;
        struct EditorSnapshot
        {
            std::vector<float> heights;
            std::vector<Color> stencil;
            std::array<float, 4> uvScales = {18,18,18,18};
            std::vector<LevelEntry> entries;
        };
        EditorSnapshot CaptureEditorSnapshot() const;
        void RestoreEditorSnapshot(const EditorSnapshot &snapshot);
        std::vector<EditorSnapshot> mEditorUndo;
        std::vector<EditorSnapshot> mEditorRedo;
#endif
        SettingsScreen mSettingsScreen;
    };
}

#endif // GAMESCENE_H
