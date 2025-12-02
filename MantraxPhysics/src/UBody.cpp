#include "../include/UBody.h"
#include "../include/UCollisionBehaviour.h"

UBody::UBody()
    : position(0, 0, 0), velocity(0, 0, 0), acceleration(0, 0, 0),
      force(0, 0, 0), mass(1.0f), drag(0.01f),
      useGravity(true), isStatic(false), collisionBehaviour(nullptr)
{
}

UBody::UBody(Vec3 pos, float m)
    : position(pos), velocity(0, 0, 0), acceleration(0, 0, 0),
      force(0, 0, 0), mass(m), drag(0.01f),
      useGravity(true), isStatic(false), collisionBehaviour(nullptr)
{
}

UBody::~UBody()
{
}

void UBody::Update(float deltaTime)
{
    if (isStatic)
        return;

    acceleration = force / mass;

    velocity += acceleration * deltaTime;

    velocity *= (1.0f - drag);

    position += velocity * deltaTime;

    // --- LÓGICA DE SLEEP ---
    const float sleepVelocityThreshold = 0.01f;
    const float sleepDelay = 0.5f;

    if (velocity.Length() < sleepVelocityThreshold)
    {
        sleepTimer += deltaTime;

        if (sleepTimer >= sleepDelay)
        {
            isSleep = true;
            velocity = Vec3(0, 0, 0);
            acceleration = Vec3(0, 0, 0);
            force = Vec3(0, 0, 0);
        }
    }
    else
    {
        sleepTimer = 0.0f; // aún se está moviendo
    }

    // Limpiar fuerza
    force = Vec3(0, 0, 0);
}

void UBody::SetAsSleep(bool val)
{
    isSleep = val;
}

void UBody::WakeUp()
{
    isSleep = false;
}

void UBody::ApplyForce(const Vec3 &f)
{
    force += f;
}

void UBody::ApplyImpulse(const Vec3 &impulse)
{
    if (!isStatic)
    {
        velocity += impulse / mass;
    }
}

Vec3 UBody::GetPosition() const { return position; }
Vec3 UBody::GetVelocity() const { return velocity; }
Vec3 UBody::GetAcceleration() const { return acceleration; }
float UBody::GetMass() const { return mass; }
bool UBody::IsStatic() const { return isStatic; }

void UBody::SetPosition(const Vec3 &pos) { position = pos; }
void UBody::SetVelocity(const Vec3 &vel) { velocity = vel; }
void UBody::SetMass(float m) { mass = m > 0 ? m : 1.0f; }
void UBody::SetDrag(float d) { drag = UMath::Clamp(d, 0.0f, 1.0f); }
void UBody::SetGravity(bool enabled) { useGravity = enabled; }
void UBody::SetStatic(bool s) { isStatic = s; }

void UBody::SetCollisionBehaviour(UCollisionBehaviour *collision)
{
    collisionBehaviour = collision;
    if (collisionBehaviour)
    {
        collisionBehaviour->AttachToBody(this);
    }
}

UCollisionBehaviour *UBody::GetCollisionBehaviour() const
{
    return collisionBehaviour;
}

bool UBody::CheckCollision(const UBody *other) const
{
    if (!collisionBehaviour || !other || !other->collisionBehaviour)
        return false;

    return collisionBehaviour->CheckCollision(other->collisionBehaviour);
}

float UBody::GetKineticEnergy() const
{
    return 0.5f * mass * velocity.LengthSquared();
}

Vec3 UBody::GetMomentum() const
{
    return velocity * mass;
}

void UBody::Stop()
{
    velocity = Vec3(0, 0, 0);
    acceleration = Vec3(0, 0, 0);
    force = Vec3(0, 0, 0);
}

void UBody::PrintInfo() const
{
    std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")\n";
    std::cout << "Velocity: (" << velocity.x << ", " << velocity.y << ", " << velocity.z << ")\n";
    std::cout << "Mass: " << mass << " | Speed: " << velocity.Length() << "\n";

    if (collisionBehaviour)
    {
        std::cout << "Has collision behaviour attached\n";
    }
}