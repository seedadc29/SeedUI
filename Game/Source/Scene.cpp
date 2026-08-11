#include "Scene.h"

namespace game
{
    Scene::Scene(Settings &settings) : mSettings(settings)
    {
    }

    Scene::~Scene() = default;

    SceneRequest Scene::ConsumeRequest()
    {
        SceneRequest request = mRequest;
        mRequest = SceneRequest::None;
        return request;
    }
}
