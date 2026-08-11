#ifndef SCENE_H
#define SCENE_H

#include "raylib.h"
#include "raymath.h"
#include "glm/glm.hpp"

#include "Physics/Physics.h"
#include "GameObject.h"
#include "CameraController.h"
#include "UI/Settings.h"

#include <utility>
#include <vector>

namespace game
{
    class GameObject;
    class PlayerObject;

    enum class SceneRequest : unsigned char
    {
        None = 0,
        Play,
        MainMenu,
        Quit,
    };

    class Scene
    {
    public:
        Scene(Settings &settings);
        virtual ~Scene();

        Scene(const Scene &) = delete;
        Scene &operator=(const Scene &) = delete;

        virtual void Init() {}
        virtual void Shutdown() {}
        virtual void Update(float deltaTime) {}
        virtual void Draw() {}
        virtual void DrawDebug() {}
        virtual void SpawnDamageFeedback(const glm::vec3 &, float) {}

        Settings &GetSettings() { return mSettings; }
        const Settings &GetSettings() const { return mSettings; }

        void SetRequest(SceneRequest request) { mRequest = request; }
        SceneRequest ConsumeRequest();

        physics::Physics &GetPhysics() { return mPhysics; }
        Camera &GetCamera() { return mCamera; }
        const Camera &GetCamera() const { return mCamera; }
        CameraController &GetCameraController() { return mCameraController; }

        PlayerObject *GetPlayer() { return mPlayer; }
        const PlayerObject *GetPlayer() const { return mPlayer; }
        void SetPlayer(PlayerObject *player) { mPlayer = player; }

        template <typename T, typename... Args>
        T *SpawnObject(Args &&...args)
        {
            T *obj = new T(std::forward<Args>(args)...);
            obj->OnSpawn(*this);
            mObjects.push_back(obj);
            return obj;
        }

        const std::vector<GameObject *> &GetObjects() const { return mObjects; }

    protected:
        Settings &mSettings;
        physics::Physics mPhysics;
        Camera mCamera = {};
        CameraController mCameraController = {};
        std::vector<GameObject *> mObjects;
        PlayerObject *mPlayer = nullptr;
        SceneRequest mRequest = SceneRequest::None;
    };
}

#endif // SCENE_H
