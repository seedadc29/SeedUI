#ifndef PHYSICS_H
#define PHYSICS_H

#include <Jolt/Jolt.h>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollector.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <glm/glm.hpp>
#include <vector>

namespace physics
{
    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }

    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr JPH::uint NUM_LAYERS(2);
    }

    struct BroadPhaseLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface
    {
        BroadPhaseLayerInterfaceImpl()
        {
            mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
        }

        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];

        virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }
    };

    struct ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
        {
            switch (inObject1)
            {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
            case Layers::MOVING:      return true;
            default:                  JPH_ASSERT(false); return false;
            }
        }
    };

    struct ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
        {
            switch (inLayer1)
            {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:      return true;
            default:                  JPH_ASSERT(false); return false;
            }
        }
    };

    struct HitResult
    {
        glm::vec3 position;
        glm::vec3 normal;
        float fraction = 1.0f;
        bool  hit = false;
    };

    class Physics
    {
    public:
        Physics();
        ~Physics();

        Physics(const Physics &) = delete;
        Physics &operator=(const Physics &) = delete;

        void Init();
        void Shutdown();
        void Step(float deltaTime);

        // Rigid body creation helpers (non-owning; user stores returned id).
        JPH::BodyID AddStaticMesh(const std::vector<JPH::Vec3> &vertices,
                                  const std::vector<JPH::IndexedTriangle> &triangles,
                                  const glm::vec3 &position = glm::vec3(0.0f));

        JPH::BodyID AddStaticBox(const glm::vec3 &halfExtents, const glm::vec3 &position);

        // Box rotated by euler angles in radians (XYZ order, applied Z->Y->X,
        // matching raylib MatrixRotateXYZ used by the editor transforms).
        JPH::BodyID AddStaticBoxRotated(const glm::vec3 &halfExtents, const glm::vec3 &position,
                                        const glm::vec3 &rotationRadians);

        void RemoveBody(JPH::BodyID id);

        // Ray cast against everything. Returns closest hit.
        bool RayCast(const glm::vec3 &from, const glm::vec3 &to, HitResult &outResult);

        // Sweep a sphere along a segment (used for camera collision).
        bool SphereCast(float radius, const glm::vec3 &from, const glm::vec3 &to, HitResult &outResult);

        JPH::PhysicsSystem &GetSystem() { return *mPhysicsSystem; }
        JPH::BodyInterface &GetBodyInterface() { return mPhysicsSystem->GetBodyInterface(); }
        JPH::TempAllocator &GetTempAllocator() { return *mTempAllocator; }

    private:
        BroadPhaseLayerInterfaceImpl          mBpLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl       mObjVsBpFilter;
        ObjectLayerPairFilterImpl              mObjPairFilter;

        JPH::PhysicsSystem *mPhysicsSystem = nullptr;
        JPH::TempAllocator *mTempAllocator = nullptr;
        JPH::JobSystemThreadPool *mJobSystem = nullptr;
    };
}

#endif // PHYSICS_H
