#include "../include/UCollisionBehaviour.h"
#include "../include/UBody.h"
#include <iostream>
#include <cmath>

// ============== UCollisionBehaviour ==============
UCollisionBehaviour::UCollisionBehaviour() : attachedBody(nullptr) {}

UCollisionBehaviour::~UCollisionBehaviour() {}

void UCollisionBehaviour::AttachToBody(UBody *body)
{
    attachedBody = body;
}

UBody *UCollisionBehaviour::GetAttachedBody() const
{
    return attachedBody;
}

Vec3 UCollisionBehaviour::GetPosition() const
{
    if (attachedBody)
        return attachedBody->GetPosition();
    return Vec3(0, 0, 0);
}

// ============== USphereCollision ==============
USphereCollision::USphereCollision(float r, Vec3 off)
    : radius(r), offset(off) {}

Vec3 USphereCollision::GetWorldPosition() const
{
    return GetPosition() + offset;
}

bool USphereCollision::CheckCollision(const UCollisionBehaviour *other) const
{
    if (!attachedBody || !other->GetAttachedBody())
        return false;

    // Intentar cast a esfera
    const USphereCollision *otherSphere = dynamic_cast<const USphereCollision *>(other);
    if (otherSphere)
        return CheckSphereCollision(otherSphere);

    // Intentar cast a caja
    const UBoxCollision *otherBox = dynamic_cast<const UBoxCollision *>(other);
    if (otherBox)
        return otherBox->CheckSphereCollision(this);

    return false;
}

bool USphereCollision::CheckSphereCollision(const USphereCollision *other) const
{
    Vec3 pos1 = GetWorldPosition();
    Vec3 pos2 = other->GetWorldPosition();

    float distance = (pos2 - pos1).Magnitude();
    float minDistance = radius + other->radius;

    return distance <= minDistance;
}

void USphereCollision::DrawDebug() const
{
    Vec3 pos = GetWorldPosition();
    std::cout << "Sphere Collision at (" << pos.x << ", " << pos.y << ", " << pos.z
              << ") - Radius: " << radius << std::endl;
}

// ============== UBoxCollision ==============
UBoxCollision::UBoxCollision(Vec3 extents, Vec3 off)
    : halfExtents(extents), offset(off) {}

Vec3 UBoxCollision::GetWorldPosition() const
{
    return GetPosition() + offset;
}

Vec3 UBoxCollision::GetMin() const
{
    Vec3 center = GetWorldPosition();
    return center - halfExtents;
}

Vec3 UBoxCollision::GetMax() const
{
    Vec3 center = GetWorldPosition();
    return center + halfExtents;
}

bool UBoxCollision::CheckCollision(const UCollisionBehaviour *other) const
{
    if (!attachedBody || !other->GetAttachedBody())
        return false;

    const UBoxCollision *otherBox = dynamic_cast<const UBoxCollision *>(other);
    if (otherBox)
        return CheckBoxCollision(otherBox);

    const USphereCollision *otherSphere = dynamic_cast<const USphereCollision *>(other);
    if (otherSphere)
        return CheckSphereCollision(otherSphere);

    return false;
}

bool UBoxCollision::CheckBoxCollision(const UBoxCollision *other) const
{
    Vec3 min1 = GetMin();
    Vec3 max1 = GetMax();
    Vec3 min2 = other->GetMin();
    Vec3 max2 = other->GetMax();

    // AABB vs AABB collision
    return (min1.x <= max2.x && max1.x >= min2.x) &&
           (min1.y <= max2.y && max1.y >= min2.y) &&
           (min1.z <= max2.z && max1.z >= min2.z);
}

bool UBoxCollision::CheckSphereCollision(const USphereCollision *other) const
{
    Vec3 spherePos = other->GetWorldPosition();
    float radius = other->GetRadius();

    Vec3 min = GetMin();
    Vec3 max = GetMax();

    // Encontrar el punto más cercano en la caja a la esfera
    Vec3 closestPoint;
    closestPoint.x = std::max(min.x, std::min(spherePos.x, max.x));
    closestPoint.y = std::max(min.y, std::min(spherePos.y, max.y));
    closestPoint.z = std::max(min.z, std::min(spherePos.z, max.z));

    // Calcular distancia al punto más cercano
    float distance = (closestPoint - spherePos).Magnitude();

    return distance <= radius;
}

void UBoxCollision::DrawDebug() const
{
    Vec3 pos = GetWorldPosition();
    Vec3 min = GetMin();
    Vec3 max = GetMax();
    std::cout << "Box Collision at (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n"
              << "  Min: (" << min.x << ", " << min.y << ", " << min.z << ")\n"
              << "  Max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
}