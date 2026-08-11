#include "Physics/Physics.h"

#include <Jolt/Core/Factory.h>

#include <glm/gtc/type_ptr.hpp>

namespace physics
{
    Physics::Physics() = default;
    Physics::~Physics() { Shutdown(); }

    void Physics::Init()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        mTempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        mJobSystem      = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2);
        mPhysicsSystem  = new JPH::PhysicsSystem();

        const JPH::uint cMaxBodies = 1024;
        const JPH::uint cNumBodyMutexes = 0;
        const JPH::uint cMaxBodyPairs = 4096;
        const JPH::uint cMaxContactConstraints = 2048;

        mPhysicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs,
                             cMaxContactConstraints,
                             mBpLayerInterface, mObjVsBpFilter, mObjPairFilter);
        mPhysicsSystem->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    }

    void Physics::Shutdown()
    {
        delete mPhysicsSystem; mPhysicsSystem = nullptr;
        delete mJobSystem;       mJobSystem = nullptr;
        delete mTempAllocator;   mTempAllocator = nullptr;

        if (JPH::Factory::sInstance)
        {
            delete JPH::Factory::sInstance;
            JPH::Factory::sInstance = nullptr;
        }
    }

    void Physics::Step(float deltaTime)
    {
        if (!mPhysicsSystem) return;
        mPhysicsSystem->Update(deltaTime, 1, mTempAllocator, mJobSystem);
    }

    JPH::BodyID Physics::AddStaticMesh(const std::vector<JPH::Vec3> &vertices,
                                       const std::vector<JPH::IndexedTriangle> &triangles,
                                       const glm::vec3 &position)
    {
        JPH::VertexList verts;
        verts.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            verts[i] = JPH::Float3(vertices[i].GetX(), vertices[i].GetY(), vertices[i].GetZ());
        }

        JPH::IndexedTriangleList tris;
        tris.resize(triangles.size());
        for (size_t i = 0; i < triangles.size(); ++i)
        {
            tris[i] = triangles[i];
        }

        JPH::MeshShapeSettings shapeSettings(verts, tris);
        shapeSettings.SetEmbedded();
        auto result = shapeSettings.Create();
        if (result.HasError())
            return JPH::BodyID();

        JPH::BodyCreationSettings creation(result.Get(), JPH::RVec3(position.x, position.y, position.z),
                                           JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

        auto &bodyInterface = mPhysicsSystem->GetBodyInterface();
        return bodyInterface.CreateAndAddBody(creation, JPH::EActivation::DontActivate);
    }

    JPH::BodyID Physics::AddStaticBox(const glm::vec3 &halfExtents, const glm::vec3 &position)
    {
        JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
        shapeSettings.SetEmbedded();
        auto result = shapeSettings.Create();
        if (result.HasError())
            return JPH::BodyID();

        JPH::BodyCreationSettings creation(result.Get(), JPH::RVec3(position.x, position.y, position.z),
                                           JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

        auto &bodyInterface = mPhysicsSystem->GetBodyInterface();
        return bodyInterface.CreateAndAddBody(creation, JPH::EActivation::DontActivate);
    }

    JPH::BodyID Physics::AddStaticBoxRotated(const glm::vec3 &halfExtents, const glm::vec3 &position,
                                             const glm::vec3 &rotationRadians)
    {
        JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z));
        shapeSettings.SetEmbedded();
        auto result = shapeSettings.Create();
        if (result.HasError())
            return JPH::BodyID();

        // Same convention as raylib MatrixRotateXYZ: apply Z, then Y, then X.
        const JPH::Quat rotation =
            JPH::Quat::sRotation(JPH::Vec3::sAxisX(), rotationRadians.x) *
            JPH::Quat::sRotation(JPH::Vec3::sAxisY(), rotationRadians.y) *
            JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), rotationRadians.z);

        JPH::BodyCreationSettings creation(result.Get(), JPH::RVec3(position.x, position.y, position.z),
                                           rotation, JPH::EMotionType::Static, Layers::NON_MOVING);

        auto &bodyInterface = mPhysicsSystem->GetBodyInterface();
        return bodyInterface.CreateAndAddBody(creation, JPH::EActivation::DontActivate);
    }

    void Physics::RemoveBody(JPH::BodyID id)
    {
        if (!mPhysicsSystem || id.IsInvalid()) return;
        JPH::BodyInterface &bodyInterface = mPhysicsSystem->GetBodyInterface();
        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
    }

    bool Physics::RayCast(const glm::vec3 &from, const glm::vec3 &to, HitResult &outResult)
    {
        if (!mPhysicsSystem) return false;

        JPH::RRayCast ray;
        ray.mOrigin = JPH::RVec3(from.x, from.y, from.z);
        ray.mDirection = JPH::Vec3(to.x - from.x, to.y - from.y, to.z - from.z);

        JPH::RayCastResult hit;
        bool result = mPhysicsSystem->GetNarrowPhaseQuery().CastRay(ray, hit);

        if (!result) return false;

        outResult.hit = true;
        outResult.fraction = hit.mFraction;
        JPH::RVec3 hitPoint = ray.GetPointOnRay(hit.mFraction);
        outResult.position = glm::vec3(float(hitPoint.GetX()), float(hitPoint.GetY()), float(hitPoint.GetZ()));

        JPH::TransformedShape transformed = mPhysicsSystem->GetBodyInterface().GetTransformedShape(hit.mBodyID);
        JPH::Vec3 normal = transformed.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, hitPoint);
        outResult.normal = glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
        return true;
    }

    bool Physics::SphereCast(float radius, const glm::vec3 &from, const glm::vec3 &to, HitResult &outResult)
    {
        if (!mPhysicsSystem) return false;

        JPH::SphereShape sphere(radius);
        sphere.SetEmbedded();

        JPH::RShapeCast shapeCast = JPH::RShapeCast::sFromWorldTransform(
            &sphere, JPH::Vec3::sReplicate(1.0f),
            JPH::RMat44::sTranslation(JPH::RVec3(from.x, from.y, from.z)),
            JPH::Vec3(to.x - from.x, to.y - from.y, to.z - from.z));

        JPH::ShapeCastSettings settings;
        settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces;
        settings.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;

        JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
        mPhysicsSystem->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

        if (!collector.HadHit()) return false;

        const JPH::ShapeCastResult &hit = collector.mHit;
        outResult.hit = true;
        outResult.fraction = hit.mFraction;
        outResult.position = glm::vec3(float(hit.mContactPointOn2.GetX()), float(hit.mContactPointOn2.GetY()), float(hit.mContactPointOn2.GetZ()));

        JPH::Vec3 normal = -hit.mPenetrationAxis.Normalized();
        outResult.normal = glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
        return true;
    }
}
