#include "../include/UPhysics.h"
#include "../include/UCollisionBehaviour.h"
#include <algorithm>
#include <cmath>
#include <iostream>

UPhysics::UPhysics()
    : globalGravity(0, -9.81f, 0), timeAccumulator(0.0f), fixedTimeStep(0.016f)
{
}

UPhysics::~UPhysics()
{
    bodies.clear();
}

void UPhysics::Update(float deltaTime)
{
    timeAccumulator += deltaTime;

    while (timeAccumulator >= fixedTimeStep)
    {
        for (UBody *body : bodies)
        {
            if (body && !body->IsStatic() && !body->SleepState())
            {
                body->ApplyForce(globalGravity * body->GetMass());
            }
        }

        for (UBody *body : bodies)
        {
            if (body)
            {
                body->Update(fixedTimeStep);
            }
        }

        ResolveCollisions();

        timeAccumulator -= fixedTimeStep;
    }
}

void UPhysics::AddBody(UBody *body)
{
    if (body)
    {
        bodies.push_back(body);
    }
}

void UPhysics::RemoveBody(UBody *body)
{
    for (size_t i = 0; i < bodies.size(); i++)
    {
        if (bodies[i] == body)
        {
            bodies.erase(bodies.begin() + i);
            break;
        }
    }
}

void UPhysics::ClearBodies()
{
    bodies.clear();
}

int UPhysics::GetBodyCount() const
{
    return static_cast<int>(bodies.size());
}

UBody *UPhysics::GetBody(int index) const
{
    if (index >= 0 && index < static_cast<int>(bodies.size()))
    {
        return bodies[index];
    }
    return nullptr;
}

void UPhysics::SetGlobalGravity(const Vec3 &gravity)
{
    globalGravity = gravity;
}

Vec3 UPhysics::GetGlobalGravity() const
{
    return globalGravity;
}

void UPhysics::SetFixedTimeStep(float timeStep)
{
    fixedTimeStep = timeStep;
}

bool UPhysics::CheckCollision(int indexA, int indexB) const
{
    if (indexA < 0 || indexA >= static_cast<int>(bodies.size()) ||
        indexB < 0 || indexB >= static_cast<int>(bodies.size()))
    {
        return false;
    }

    UBody *bodyA = bodies[indexA];
    UBody *bodyB = bodies[indexB];

    if (!bodyA || !bodyB)
        return false;

    return bodyA->CheckCollision(bodyB);
}

void UPhysics::ResolveCollisions()
{
    const float restitution = 0.5f;
    const float minVelocityThreshold = 0.01f;
    const float restingVelocityThreshold = 0.5f;
    const float penetrationSlop = 0.001f;

    for (size_t i = 0; i < bodies.size(); i++)
    {
        for (size_t j = i + 1; j < bodies.size(); j++)
        {
            UBody *bodyA = bodies[i];
            UBody *bodyB = bodies[j];

            if (!bodyA || !bodyB)
                continue;

            if (!bodyA->CheckCollision(bodyB))
                continue;

            if (bodyA->IsStatic() && bodyB->IsStatic())
                continue;

            Vec3 posA = bodyA->GetPosition();
            Vec3 posB = bodyB->GetPosition();

            UCollisionBehaviour *collA = bodyA->GetCollisionBehaviour();
            UCollisionBehaviour *collB = bodyB->GetCollisionBehaviour();

            USphereCollision *sphereA = dynamic_cast<USphereCollision *>(collA);
            USphereCollision *sphereB = dynamic_cast<USphereCollision *>(collB);
            UBoxCollision *boxA = dynamic_cast<UBoxCollision *>(collA);
            UBoxCollision *boxB = dynamic_cast<UBoxCollision *>(collB);

            Vec3 normal;
            float overlap = 0.0f;

            // ===== ESFERA vs ESFERA =====
            if (sphereA && sphereB)
            {
                Vec3 diff = posB - posA;
                float distance = diff.Length();

                if (distance < 0.0001f)
                {
                    diff = Vec3(0.01f, 1.0f, 0.0f);
                    distance = diff.Length();
                }

                normal = diff / distance;
                float totalRadius = sphereA->GetRadius() + sphereB->GetRadius();
                overlap = totalRadius - distance;
            }
            // ===== ESFERA vs CAJA =====
            else if ((sphereA && boxB) || (boxA && sphereB))
            {
                USphereCollision *sphere = sphereA ? sphereA : sphereB;
                UBoxCollision *box = boxA ? boxA : boxB;
                UBody *sphereBody = sphereA ? bodyA : bodyB;
                UBody *boxBody = sphereA ? bodyB : bodyA;

                Vec3 spherePos = sphereBody->GetPosition();
                Vec3 boxPos = boxBody->GetPosition();
                Vec3 boxHalfExtents = box->GetHalfExtents();

                // Punto más cercano en la caja
                Vec3 closestPoint;
                closestPoint.x = std::max(boxPos.x - boxHalfExtents.x,
                                          std::min(spherePos.x, boxPos.x + boxHalfExtents.x));
                closestPoint.y = std::max(boxPos.y - boxHalfExtents.y,
                                          std::min(spherePos.y, boxPos.y + boxHalfExtents.y));
                closestPoint.z = std::max(boxPos.z - boxHalfExtents.z,
                                          std::min(spherePos.z, boxPos.z + boxHalfExtents.z));

                Vec3 distVec = spherePos - closestPoint;
                float dist = distVec.Length();

                overlap = sphere->GetRadius() - dist;

                if (dist > 0.0001f)
                {
                    normal = distVec / dist;
                }
                else
                {
                    Vec3 dirFromBox = spherePos - boxPos;
                    if (dirFromBox.Length() > 0.0001f)
                    {
                        normal = dirFromBox / dirFromBox.Length();
                    }
                    else
                    {
                        normal = Vec3(0, 1, 0);
                    }
                }

                // Mantener consistencia: normal debe apuntar de A hacia B
                // Si la esfera es bodyA, invertir porque el normal naturalmente apunta hacia la esfera
                if (sphereBody == bodyA)
                {
                    normal = normal * -1.0f;
                }
            }
            // ===== CAJA vs CAJA (AABB) =====
            else if (boxA && boxB)
            {
                Vec3 halfExtentsA = boxA->GetHalfExtents();
                Vec3 halfExtentsB = boxB->GetHalfExtents();

                // Calcular penetración en cada eje
                Vec3 delta = posB - posA;
                Vec3 penetration;
                penetration.x = (halfExtentsA.x + halfExtentsB.x) - std::abs(delta.x);
                penetration.y = (halfExtentsA.y + halfExtentsB.y) - std::abs(delta.y);
                penetration.z = (halfExtentsA.z + halfExtentsB.z) - std::abs(delta.z);

                // Si alguna penetración es negativa, no hay colisión
                if (penetration.x <= 0 || penetration.y <= 0 || penetration.z <= 0)
                    continue;

                // Encontrar el eje de menor penetración (Minimum Translation Vector)
                if (penetration.x < penetration.y && penetration.x < penetration.z)
                {
                    // Eje X tiene menor penetración
                    overlap = penetration.x;
                    normal = Vec3(delta.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
                }
                else if (penetration.y < penetration.z)
                {
                    // Eje Y tiene menor penetración
                    overlap = penetration.y;
                    normal = Vec3(0.0f, delta.y > 0 ? 1.0f : -1.0f, 0.0f);
                }
                else
                {
                    // Eje Z tiene menor penetración
                    overlap = penetration.z;
                    normal = Vec3(0.0f, 0.0f, delta.z > 0 ? 1.0f : -1.0f);
                }
            }

            if (overlap <= 0)
                continue;

            float massA = bodyA->IsStatic() ? 0.0f : bodyA->GetMass();
            float massB = bodyB->IsStatic() ? 0.0f : bodyB->GetMass();
            float totalMass = massA + massB;

            if (totalMass < 0.0001f)
                continue;

            // ===== SEPARACIÓN POSICIONAL =====
            float correctionPercent = 0.8f;
            float correctionSlop = penetrationSlop;

            if (overlap > correctionSlop)
            {
                float correction = (overlap - correctionSlop) * correctionPercent;

                if (!bodyA->IsStatic() && !bodyB->IsStatic())
                {
                    float ratioA = massB / totalMass;
                    float ratioB = massA / totalMass;

                    bodyA->SetPosition(posA - normal * (correction * ratioA));
                    bodyB->SetPosition(posB + normal * (correction * ratioB));
                }
                else if (bodyA->IsStatic())
                {
                    bodyB->SetPosition(posB + normal * correction);
                }
                else
                {
                    bodyA->SetPosition(posA - normal * correction);
                }
            }

            // ===== RESOLUCIÓN DE VELOCIDADES =====
            Vec3 velA = bodyA->GetVelocity();
            Vec3 velB = bodyB->GetVelocity();
            Vec3 relativeVel = velB - velA;
            float velAlongNormal = relativeVel.Dot(normal);

            bool isResting = std::abs(velAlongNormal) < restingVelocityThreshold && overlap < 0.01f;

            if (isResting)
            {
                // CONTACTO ESTÁTICO
                if (!bodyA->IsStatic())
                {
                    float velANormal = velA.Dot(normal);
                    if (velANormal > 0)
                    {
                        Vec3 tangentVel = velA - normal * velANormal;
                        bodyA->SetVelocity(tangentVel);
                    }

                    if (bodyA->GetVelocity().Length() < minVelocityThreshold)
                    {
                        bodyA->SetVelocity(Vec3(0, 0, 0));
                    }
                }

                if (!bodyB->IsStatic())
                {
                    float velBNormal = velB.Dot(normal);
                    if (velBNormal < 0)
                    {
                        Vec3 tangentVel = velB - normal * velBNormal;
                        bodyB->SetVelocity(tangentVel);
                    }

                    if (bodyB->GetVelocity().Length() < minVelocityThreshold)
                    {
                        bodyB->SetVelocity(Vec3(0, 0, 0));
                    }
                }
            }
            else if (velAlongNormal < 0)
            {
                // COLISIÓN DINÁMICA
                float invMassA = bodyA->IsStatic() ? 0.0f : 1.0f / massA;
                float invMassB = bodyB->IsStatic() ? 0.0f : 1.0f / massB;

                float impulseMagnitude = -(1.0f + restitution) * velAlongNormal;
                impulseMagnitude /= (invMassA + invMassB);

                Vec3 impulse = normal * impulseMagnitude;

                if (!bodyA->IsStatic())
                {
                    Vec3 newVelA = velA - impulse * invMassA;

                    if (newVelA.Length() < minVelocityThreshold)
                    {
                        bodyA->SetVelocity(Vec3(0, 0, 0));
                    }
                    else
                    {
                        bodyA->SetVelocity(newVelA);
                    }
                }

                if (!bodyB->IsStatic())
                {
                    Vec3 newVelB = velB + impulse * invMassB;

                    if (newVelB.Length() < minVelocityThreshold)
                    {
                        bodyB->SetVelocity(Vec3(0, 0, 0));
                    }
                    else
                    {
                        bodyB->SetVelocity(newVelB);
                    }
                }
            }
        }
    }
}

void UPhysics::ApplyForceToAll(const Vec3 &force)
{
    for (UBody *body : bodies)
    {
        if (body && !body->IsStatic())
        {
            body->ApplyForce(force);
        }
    }
}

void UPhysics::PrintAllBodies() const
{
    std::cout << "=== UPhysics - Total Bodies: " << bodies.size() << " ===\n";
    for (size_t i = 0; i < bodies.size(); i++)
    {
        std::cout << "Body " << i << ":\n";
        if (bodies[i])
        {
            bodies[i]->PrintInfo();
        }
        std::cout << "---\n";
    }
}