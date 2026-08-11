#ifndef ENEMYOBJECT_H
#define ENEMYOBJECT_H

#include "GameObject.h"
#include "Assets/FbxModel.h"
#include <string>

namespace game
{
    enum class EnemyCategory { Blob, Big, Flying };
    enum class EnemyState { Waiting, Patrol, Chase, Attack, Returning, Dead };

    struct EnemyConfig
    {
        EnemyCategory category = EnemyCategory::Blob;
        std::string modelName = "GreenBlob";
        std::string displayName = "Blob Verde";
        int level = 1;
        float maxHealth = 45.0f;
        float attack = 8.0f;
        float defense = 1.0f;
        float patrolRadius = 9.0f;
        float detectionRadius = 12.0f;
        float attackRange = 1.7f;
        float aggressionChance = 0.35f;
        int moneyReward = 5;
        float experienceReward = 20.0f;
    };

    class EnemyObject : public GameObject
    {
    public:
        EnemyObject(const glm::vec3 &spawnPosition, const EnemyConfig &config, float groundHeight);
        void OnSpawn(Scene &scene) override;
        void Update(Scene &scene, float deltaTime) override;
        void Draw(Scene &scene) override;
        void DrawDebug(Scene &scene) override;
        void DrawOverlay(const Scene &scene) const;
        bool IsDead() const { return mHealth <= 0.0f; }
        bool IsExpired() const { return IsDead() && mDeathTimer <= 0.0f; }
        const glm::vec3 &GetSpawnPosition() const { return mSpawnPosition; }
        void ResetToSpawn();
        void SetEditorTransform(const glm::vec3 &position, const glm::vec3 &rotation,
                                const glm::vec3 &scale);

    private:
        void SetState(EnemyState state);
        void MoveTowards(const glm::vec3 &target, float speed, float deltaTime);
        float TakeDamage(float amount, class PlayerObject &player, Scene &scene);
        void PlayStateAnimation();
        void EnsureModelLoaded();

        EnemyConfig mConfig;
        glm::vec3 mSpawnPosition;
        glm::vec3 mDestination;
        FbxModel mModel;
        EnemyState mState = EnemyState::Waiting;
        float mHealth = 1.0f;
        float mStateTimer = 0.0f;
        float mAttackCooldown = 0.0f;
        float mHitFlash = 0.0f;
        float mDeathTimer = 0.0f;
        float mYawDegrees = 0.0f;
        float mVisualHeight = 1.5f;
        bool mAggressive = false;
        bool mPlayerAttackLatched = false;
        bool mProvoked = false;
        bool mRewardGranted = false;
        bool mLoadAttempted = false;
        glm::vec3 mEditorRotation = glm::vec3(0.0f);
        glm::vec3 mEditorScale = glm::vec3(1.0f);
    };
}

#endif
