#include "GameObjects/EnemyObject.h"
#include "GameObjects/PlayerObject.h"
#include "Scene.h"
#include "UI/GameUI.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace game
{
    static int gSynthDetailedEnemyModels = 0;
    static constexpr int MaxSynthDetailedEnemyModels = 3;
    static float RandomUnit() { return (float)GetRandomValue(0, 10000) / 10000.0f; }
    static const char *CategoryName(EnemyCategory category)
    {
        return category == EnemyCategory::Big ? "Big" : category == EnemyCategory::Flying ? "Flying" : "Blob";
    }

    EnemyObject::~EnemyObject()
    {
        ReleaseDetailedModelSlot();
    }

    EnemyObject::EnemyObject(const glm::vec3 &spawnPosition, const EnemyConfig &config, float groundHeight)
        : mConfig(config), mSpawnPosition(spawnPosition)
    {
        mPosition = spawnPosition;
        mPosition.y = groundHeight;
        mSpawnPosition.y = groundHeight;
        mDestination = mPosition;
        mHealth = config.maxHealth;
        mAggressive = RandomUnit() < config.aggressionChance;
        mVisualHeight = config.category == EnemyCategory::Big ? 2.4f :
                        config.category == EnemyCategory::Flying ? 1.7f : 1.35f;
        mStateTimer = 0.5f + RandomUnit() * 2.0f;
    }

    void EnemyObject::OnSpawn(Scene &)
    {
    }

    void EnemyObject::ReleaseDetailedModelSlot()
    {
        if (!mDetailedModelSlot) return;
        mDetailedModelSlot = false;
        gSynthDetailedEnemyModels = std::max(0, gSynthDetailedEnemyModels - 1);
    }

    bool EnemyObject::EnsureModelLoaded(float polygonRatio)
    {
        if (mLoadAttempted) return mModel.IsLoaded();
        mLoadAttempted = true;
        std::string path = std::string("Assets/Enemies/") + CategoryName(mConfig.category) + "/" + mConfig.modelName + ".gltf";
        if (!FileExists(path.c_str()))
            path = std::string("Game/Assets/Kit assets/Ultimate Monsters/") + CategoryName(mConfig.category) + "/glTF/" + mConfig.modelName + ".gltf";
        if (!mModel.Load(path, path))
        {
            TraceLog(LOG_WARNING, "ENEMY: %s", mModel.GetLastError().c_str());
            return false;
        }
        if (polygonRatio < 0.999f) mModel.ReduceTriangles(polygonRatio);
        mModel.PlayFirstAnimationContaining("idle", true);
        return true;
    }

    void EnemyObject::SetState(EnemyState state)
    {
        if (mState == state) return;
        mState = state;
        PlayStateAnimation();
    }

    void EnemyObject::ResetToSpawn()
    {
        mPosition = mSpawnPosition;
        mDestination = mSpawnPosition;
        mHealth = mConfig.maxHealth;
        mProvoked = false;
        mRewardGranted = false;
        mDeathTimer = 0.0f;
        mAttackCooldown = 0.0f;
        mStateTimer = 1.0f;
        mState = EnemyState::Waiting;
        PlayStateAnimation();
    }

    void EnemyObject::PlayStateAnimation()
    {
        if (mState == EnemyState::Dead) mModel.PlayFirstAnimationContaining("death", false);
        else if (mState == EnemyState::Attack) mModel.PlayFirstAnimationContaining("punch", false, 1.15f);
        else if (mState == EnemyState::Chase || mState == EnemyState::Returning) mModel.PlayFirstAnimationContaining("run", true);
        else if (mState == EnemyState::Patrol) mModel.PlayFirstAnimationContaining("walk", true);
        else mModel.PlayFirstAnimationContaining("idle", true);
    }

    void EnemyObject::MoveTowards(const glm::vec3 &target, float speed, float deltaTime)
    {
        glm::vec3 delta(target.x - mPosition.x, 0.0f, target.z - mPosition.z);
        const float distance = glm::length(delta);
        if (distance < 0.001f) return;
        delta /= distance;
        mPosition += delta * std::min(distance, speed * deltaTime);
        // Os monstros glTF sao authored olhando para +Z, entao o yaw (em Y-up)
        // e o angulo do vetor de movimento no plano X-Z.
        mYawDegrees = atan2f(delta.x, delta.z) * RAD2DEG;
    }

    float EnemyObject::TakeDamage(float amount, PlayerObject &player, Scene &scene)
    {
        if (IsDead()) return 0.0f;
        mProvoked = true;
        const float previousHealth = mHealth;
        mHealth = std::max(0.0f, mHealth - std::max(1.0f, amount - mConfig.defense));
        const float damage = previousHealth - mHealth;
        mHitFlash = 0.16f;
        scene.SpawnDamageFeedback(mPosition + glm::vec3(0.0f, mVisualHeight * 0.65f, 0.0f), damage);
        if (mHealth <= 0.0f)
        {
            SetState(EnemyState::Dead);
            mDeathTimer = 4.5f;
            if (!mRewardGranted)
            {
                player.AddExperience(mConfig.experienceReward);
                player.AddMoney(mConfig.moneyReward);
                player.Heal(player.GetMaxHealth() * 0.08f);
                mRewardGranted = true;
            }
        }
        else
        {
            mModel.PlayFirstAnimationContaining("hitreact", false, 1.2f);
            mState = EnemyState::Chase;
        }
        return damage;
    }

    void EnemyObject::Update(Scene &scene, float deltaTime)
    {
        PlayerObject *player = scene.GetPlayer();
        if (!player) return;
        const float loadDistance = glm::distance(glm::vec2(player->GetPosition().x, player->GetPosition().z),
                                                 glm::vec2(mPosition.x, mPosition.z));
        const bool retroMode = scene.GetSettings().retroMode;
        const bool reducedProfile = scene.GetSettings().crtMode || scene.GetSettings().synthMode;
        const float distantTickRadius = reducedProfile ? 38.0f : 48.0f;
        if (retroMode && loadDistance > distantTickRadius)
        {
            mDistantUpdateAccumulator = std::min(
                mDistantUpdateAccumulator + deltaTime, 0.24f);
            if (mDistantUpdateAccumulator < 0.16f) return;
            deltaTime = mDistantUpdateAccumulator;
            mDistantUpdateAccumulator = 0.0f;
        }
        else
        {
            mDistantUpdateAccumulator = 0.0f;
        }
        const float modelDistance = reducedProfile
            ? 22.0f : (retroMode ? 42.0f : 48.0f);
        // Modelos animados glTF mantem malha, esqueleto e animacoes por
        // instancia. Nos perfis leves, descarregue-os fora da vizinhanca para
        // impedir crescimento continuo de RAM durante a exploracao.
        if (reducedProfile && loadDistance > 30.0f && mModel.IsLoaded())
        {
            mModel.Unload();
            mLoadAttempted = false;
            ReleaseDetailedModelSlot();
        }
        if (loadDistance <= modelDistance && !mModel.IsLoaded())
        {
            if (scene.GetSettings().synthMode)
            {
                if (!mLoadAttempted && !mDetailedModelSlot &&
                    gSynthDetailedEnemyModels < MaxSynthDetailedEnemyModels)
                {
                    mDetailedModelSlot = true;
                    ++gSynthDetailedEnemyModels;
                }
                if (!mLoadAttempted && mDetailedModelSlot &&
                    !EnsureModelLoaded(scene.GetSettings().synthEnemyPolygonRatio))
                    ReleaseDetailedModelSlot();
            }
            else EnsureModelLoaded();
        }
        mModel.Update(deltaTime);
        mHitFlash = std::max(0.0f, mHitFlash - deltaTime);
        if (IsDead()) { mDeathTimer = std::max(0.0f, mDeathTimer - deltaTime); return; }

        const glm::vec3 playerPosition = player->GetPosition();
        const float playerDistance = glm::length(glm::vec2(playerPosition.x - mPosition.x, playerPosition.z - mPosition.z));
        const bool playerSwing = player->IsAttacking();
        if (playerSwing && !mPlayerAttackLatched && playerDistance <= 2.25f)
            TakeDamage(player->GetAttackDamage(), *player, scene);
        mPlayerAttackLatched = playerSwing;
        // Um golpe fatal muda o estado durante esta atualizacao. Nao permita
        // que a logica de deteccao abaixo sobrescreva Death com Chase/Attack.
        if (IsDead()) return;

        if (player->IsDead())
        {
            mProvoked = false;
            SetState(EnemyState::Returning);
        }
        const bool levelSafe = player->GetLevel() >= mConfig.level + 5 && !mProvoked;
        if (!player->IsDead() && !levelSafe && playerDistance <= mConfig.detectionRadius)
            SetState(playerDistance <= mConfig.attackRange ? EnemyState::Attack : EnemyState::Chase);
        else if ((mState == EnemyState::Chase || mState == EnemyState::Attack) && !mProvoked)
            SetState(EnemyState::Returning);

        mStateTimer -= deltaTime;
        mAttackCooldown -= deltaTime;
        switch (mState)
        {
        case EnemyState::Waiting:
            if (mStateTimer <= 0.0f)
            {
                const float angle = RandomUnit() * 2.0f * PI;
                const float radius = sqrtf(RandomUnit()) * mConfig.patrolRadius;
                mDestination = mSpawnPosition + glm::vec3(sinf(angle) * radius, 0.0f, cosf(angle) * radius);
                SetState(EnemyState::Patrol);
            }
            break;
        case EnemyState::Patrol:
            MoveTowards(mDestination, 1.25f, deltaTime);
            if (glm::distance(glm::vec2(mPosition.x, mPosition.z), glm::vec2(mDestination.x, mDestination.z)) < 0.15f)
            { mStateTimer = 0.8f + RandomUnit() * 1.4f; SetState(EnemyState::Waiting); }
            break;
        case EnemyState::Chase:
            if (playerDistance > mConfig.detectionRadius * 1.8f) { mProvoked = false; SetState(EnemyState::Returning); }
            else MoveTowards(playerPosition, 3.6f, deltaTime);
            break;
        case EnemyState::Attack:
            if (playerDistance > mConfig.attackRange + 0.35f) SetState(EnemyState::Chase);
            else if (mAttackCooldown <= 0.0f)
            {
                const float damage = player->TakeDamage(mConfig.attack);
                if (damage > 0.0f)
                    scene.SpawnDamageFeedback(player->GetPosition() + glm::vec3(0.0f, 1.15f, 0.0f), damage);
                mAttackCooldown = (mAggressive ? 0.75f : 1.25f) + RandomUnit() * (mAggressive ? 0.55f : 1.15f);
                mModel.PlayFirstAnimationContaining("punch", false, 1.2f);
            }
            break;
        case EnemyState::Returning:
            MoveTowards(mSpawnPosition, 2.2f, deltaTime);
            if (glm::distance(glm::vec2(mPosition.x, mPosition.z), glm::vec2(mSpawnPosition.x, mSpawnPosition.z)) < 0.2f)
            { mStateTimer = 1.0f; SetState(EnemyState::Waiting); }
            break;
        default: break;
        }
    }

    void EnemyObject::Draw(Scene &scene)
    {
        if (IsExpired()) return;
        const glm::vec3 player = scene.GetPlayer() ? scene.GetPlayer()->GetPosition() : glm::vec3(0.0f);
        const bool reducedProfile = scene.GetSettings().crtMode || scene.GetSettings().synthMode;
        const float drawDistance = reducedProfile
            ? 36.0f : (scene.GetSettings().retroMode ? 46.0f : 55.0f);
        if (glm::distance(glm::vec2(player.x, player.z), glm::vec2(mPosition.x, mPosition.z)) > drawDistance) return;
        const Vector3 position = { mPosition.x, mPosition.y + (mConfig.category == EnemyCategory::Flying ? 1.0f : 0.0f), mPosition.z };
        Color tint = WHITE;
        if (mHitFlash > 0.0f)
            tint = {255, 72, 72, 255};
        else if (!IsDead() && mHealth / mConfig.maxHealth <= 0.22f)
        {
            const float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 8.0f);
            tint = ColorLerp(WHITE, Color{255, 58, 58, 255}, pulse * 0.72f);
        }
        if (mModel.IsLoaded()) mModel.Draw(position, mYawDegrees + mEditorRotation.y, mVisualHeight,
            tint, scene.GetCamera().position, {1,1,1}, 1.0f, false, 0.0f, true,
            {mEditorScale.x, mEditorScale.y, mEditorScale.z},
            {mEditorRotation.x, 0.0f, mEditorRotation.z});
        else DrawCube({position.x, position.y + mVisualHeight * 0.5f, position.z}, 0.8f, mVisualHeight, 0.8f, tint);
    }

    void EnemyObject::SetEditorTransform(const glm::vec3 &position, const glm::vec3 &rotation,
                                         const glm::vec3 &scale)
    {
        mPosition = position;
        mSpawnPosition = position;
        mDestination = position;
        mEditorRotation = rotation;
        mEditorScale = scale;
    }

    void EnemyObject::DrawOverlay(const Scene &scene) const
    {
        const PlayerObject *player = scene.GetPlayer();
        if (!player || mDeathTimer <= 0.0f && IsDead()) return;
        const float distance = glm::distance(glm::vec2(player->GetPosition().x, player->GetPosition().z), glm::vec2(mPosition.x, mPosition.z));
        if (distance > 18.0f && mHealth >= mConfig.maxHealth) return;
        const Camera3D &camera = scene.GetCamera();
        const Vector3 worldPos = {mPosition.x, mPosition.y + mVisualHeight + 0.45f, mPosition.z};
        // Nao exibir a barra para inimigos atras da camera: o GetWorldToScreen
        // projeta pontos atras para dentro do viewport, entao a checagem de
        // limites da tela sozinha nao resolve.
        const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        if (Vector3DotProduct(Vector3Subtract(worldPos, camera.position), forward) <= 0.0f) return;
        Vector2 screen = GetWorldToScreen(worldPos, camera);
        if (screen.x < 0 || screen.y < 0 || screen.x > GetScreenWidth() || screen.y > GetScreenHeight()) return;
        char text[96]; std::snprintf(text, sizeof(text), "%s  Nv. %d", mConfig.displayName.c_str(), mConfig.level);
        const int fontSize = 14; const int width = 112;
        DrawText(text, (int)screen.x - MeasureText(text, fontSize) / 2, (int)screen.y - 24, fontSize, WHITE);
        DrawRectangle((int)screen.x - width / 2, (int)screen.y - 7, width, 8, {45,20,24,225});
        DrawRectangle((int)screen.x - width / 2, (int)screen.y - 7, (int)(width * mHealth / mConfig.maxHealth), 8, {205,55,65,255});
    }

    void EnemyObject::DrawDebug(Scene &)
    {
        // Ground ring: horizontal (axis along the world up, +Y).
        DrawCircle3D({mSpawnPosition.x, mSpawnPosition.y + 0.03f, mSpawnPosition.z}, mConfig.patrolRadius, {0,1,0}, 90.0f, Fade(YELLOW, 0.35f));
    }
}
