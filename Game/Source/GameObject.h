#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "raylib.h"
#include "glm/glm.hpp"

namespace game
{
    class Scene;

    class GameObject
    {
    public:
        GameObject() = default;
        virtual ~GameObject() = default;

        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject &) = delete;

        virtual void OnSpawn(Scene &scene) {}
        virtual void Update(Scene &scene, float deltaTime) {}
        virtual void Draw(Scene &scene) {}
        virtual void DrawDebug(Scene &scene) {}

        const glm::vec3 &GetPosition() const { return mPosition; }
        const glm::vec3 &GetRotation() const { return mRotation; }
        void SetPosition(const glm::vec3 &p) { mPosition = p; }
        void SetRotation(const glm::vec3 &r) { mRotation = r; }

    protected:
        glm::vec3 mPosition = glm::vec3(0.0f);
        glm::vec3 mRotation = glm::vec3(0.0f);
    };
}

#endif // GAMEOBJECT_H
