#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "raylib.h"
#include "glm/glm.hpp"

namespace game
{
    class Scene;
    class PlayerObject;

    enum class CameraView : unsigned char
    {
        FirstPerson = 0,
        SecondPerson = 1,
        ThirdPerson = 2,
        TopDown = 3,
    };

    class CameraController
    {
    public:
        CameraController();

        void Init(Scene &scene);
        void Update(Scene &scene, float deltaTime);
        Camera &GetCamera() { return mCamera; }
        const Camera &GetCamera() const { return mCamera; }

        CameraView GetView() const { return mView; }
        void SetView(CameraView v) { mView = v; }
        void CycleView();

        float GetYaw() const { return mYaw; }
        float GetPitch() const { return mPitch; }

        void SetMouseSensitivity(float sensitivity) { mMouseSensitivity = sensitivity; }

    private:
        void UpdateFirstPerson(Scene &scene, PlayerObject &player, const Vector3 &target);
        void UpdateThirdPerson(Scene &scene, PlayerObject &player, const Vector3 &target);
        void UpdateSecondPerson(Scene &scene, PlayerObject &player, const Vector3 &target);
        void UpdateTopDown(Scene &scene, PlayerObject &player, const Vector3 &target);

        void ApplyMouseLook();

        Camera mCamera = {};
        CameraView mView = CameraView::ThirdPerson;

        float mYaw = 0.0f;
        float mPitch = -0.35f;
        float mDistance = 6.0f;
        float mMouseSensitivity = 0.0025f;

        // Collision radius for camera sphere cast.
        float mCameraRadius = 0.2f;
    };
}

#endif // CAMERACONTROLLER_H
