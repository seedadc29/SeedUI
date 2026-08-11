#include "CameraController.h"
#include "Scene.h"
#include "GameObjects/PlayerObject.h"
#include "Physics/Physics.h"

#include "raylib.h"
#include "raymath.h"

#include <glm/glm.hpp>
#include <cmath>

namespace game
{
    static Vector3 ToVec3(const glm::vec3 &v) { return { v.x, v.y, v.z }; }

    CameraController::CameraController()
    {
        mCamera.up = { 0,1,0 };
        mCamera.fovy = 60.0f;
        mCamera.projection = CAMERA_PERSPECTIVE;
        mCamera.position = { 0, 10, 5 };
        mCamera.target = { 0, 0, 0 };
    }

    void CameraController::Init(Scene &scene)
    {
        DisableCursor();
    }

    void CameraController::CycleView()
    {
        unsigned char v = (unsigned char)mView;
        v = (v + 1) & 0x03;
        mView = (CameraView)v;
    }

    void CameraController::ApplyMouseLook()
    {
        Vector2 delta = GetMouseDelta();
        mYaw   -= delta.x * mMouseSensitivity;
        // Mantem as cameras de gameplay voltadas para a frente: somente yaw.
        mPitch = 0.0f;
    }

    void CameraController::Update(Scene &scene, float deltaTime)
    {
        PlayerObject *player = scene.GetPlayer();
        if (!player) return;

        Vector3 target = ToVec3(player->GetCameraTarget());

        ApplyMouseLook();

        switch (mView)
        {
        case CameraView::FirstPerson:  UpdateFirstPerson(scene, *player, target); break;
        case CameraView::SecondPerson: UpdateSecondPerson(scene, *player, target); break;
        case CameraView::ThirdPerson:  UpdateThirdPerson(scene, *player, target); break;
        case CameraView::TopDown:      UpdateTopDown(scene, *player, target); break;
        }
    }

    void CameraController::UpdateFirstPerson(Scene &scene, PlayerObject &player, const Vector3 &target)
    {
        // Camera sits at player eye. Look forward based on yaw/pitch.
        Vector3 forward = {
            sinf(mYaw),
            0.0f,
            cosf(mYaw)
        };
        mCamera.up = { 0,1,0 };
        mCamera.position = target;
        mCamera.target = Vector3Add(target, forward);
    }

    void CameraController::UpdateThirdPerson(Scene &scene, PlayerObject &player, const Vector3 &target)
    {
        // Orbit behind player at mDistance with collision against terrain.
        Vector3 back = {
            -sinf(mYaw),
            0.0f,
            -cosf(mYaw)
        };
        Vector3 desired = Vector3Add(target, Vector3Scale(back, mDistance));
        desired.y += 1.15f;

        // Sphere-cast from target to desired; clamp on hit.
        glm::vec3 from(target.x, target.y, target.z);
        glm::vec3 to(desired.x, desired.y, desired.z);
        physics::HitResult hit;
        if (scene.GetPhysics().SphereCast(mCameraRadius, from, to, hit))
        {
            float t = hit.fraction;
            if (t < 1.0f) t -= 0.05f;
            if (t < 0.0f) t = 0.0f;
            Vector3 shortDir = Vector3Scale(Vector3Subtract(desired, target), t);
            mCamera.position = Vector3Add(target, shortDir);
        }
        else
        {
            mCamera.position = desired;
        }
        mCamera.up = { 0,1,0 };
        mCamera.target = target;
    }

    void CameraController::UpdateSecondPerson(Scene &scene, PlayerObject &player, const Vector3 &target)
    {
        // "Second person" — frente do player, vendo o player de frente.
        const Vector3 back = { -sinf(mYaw), 0.0f, -cosf(mYaw) };
        const Vector3 right = { cosf(mYaw), 0.0f, -sinf(mYaw) };
        Vector3 desired = Vector3Add(target, Vector3Scale(back, 2.65f));
        desired = Vector3Add(desired, Vector3Scale(right, 0.58f));
        desired.y += 0.55f;

        glm::vec3 from(target.x, target.y, target.z);
        glm::vec3 to(desired.x, desired.y, desired.z);
        physics::HitResult hit;
        if (scene.GetPhysics().SphereCast(mCameraRadius, from, to, hit))
        {
            float t = hit.fraction;
            if (t < 1.0f) t -= 0.05f;
            if (t < 0.0f) t = 0.0f;
            Vector3 shortDir = Vector3Scale(Vector3Subtract(desired, target), t);
            mCamera.position = Vector3Add(target, shortDir);
        }
        else
        {
            mCamera.position = desired;
        }
        mCamera.up = { 0,1,0 };
        mCamera.target = target;
    }

    void CameraController::UpdateTopDown(Scene &scene, PlayerObject &player, const Vector3 &target)
    {
        // Câmera olhando de cima (estilo MOBA / LoL). Mantém um pouco de inclinação pro.
        float topHeight = 18.0f;
        float back = 4.0f;
        Vector3 desired = {
            target.x - sinf(mYaw) * back,
            target.y + topHeight,
            target.z - cosf(mYaw) * back
        };

        glm::vec3 from(target.x, target.y + 2.0f, target.z);
        glm::vec3 to(desired.x, desired.y, desired.z);
        physics::HitResult hit;
        if (scene.GetPhysics().SphereCast(mCameraRadius, from, to, hit))
        {
            float t = hit.fraction;
            if (t < 1.0f) t -= 0.05f;
            if (t < 0.0f) t = 0.0f;
            Vector3 shortDir = Vector3Scale(Vector3Subtract(desired, Vector3{from.x,from.y,from.z}), t);
            mCamera.position = Vector3Add({from.x,from.y,from.z}, shortDir);
        }
        else
        {
            mCamera.position = desired;
        }
        mCamera.target = target;
        mCamera.up = { 0,1,0 }; // up vector for top-down view (X right, Y up on screen)
    }
}
