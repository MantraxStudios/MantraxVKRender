#pragma once
#include <iostream>
#include "UMath.h"
#include "UCollisionBehaviour.h"

class UBody
{
private:
    // Propiedades físicas
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;

    Vec3 force; // Fuerza acumulada
    float mass; // Masa del cuerpo
    float drag; // Resistencia al aire (0-1)

    bool useGravity; // Si aplica gravedad
    bool isStatic;   // Si el objeto es estático (no se mueve)
    bool isSleep = false;
    float sleepTimer = 0.0f;

    // Componente de colisión
    UCollisionBehaviour *collisionBehaviour;

public:
    UBody();
    UBody(Vec3 pos, float m = 1.0f);
    ~UBody(); // Destructor para limpiar el collision behaviour si es necesario

    // Actualizar física (llamar cada frame)
    void Update(float deltaTime);

    void SetAsSleep(bool val);
    void WakeUp();
    bool SleepState() { return isSleep; }

    // Aplicar una fuerza
    void ApplyForce(const Vec3 &f);

    // Aplicar impulso (cambio instantáneo de velocidad)
    void ApplyImpulse(const Vec3 &impulse);

    // Getters
    Vec3 GetPosition() const;
    Vec3 GetVelocity() const;
    Vec3 GetAcceleration() const;
    float GetMass() const;
    bool IsStatic() const;

    // Setters
    void SetPosition(const Vec3 &pos);
    void SetVelocity(const Vec3 &vel);
    void SetMass(float m);
    void SetDrag(float d);
    void SetGravity(bool enabled);
    void SetStatic(bool s);

    // Manejo de colisión
    void SetCollisionBehaviour(UCollisionBehaviour *collision);
    UCollisionBehaviour *GetCollisionBehaviour() const;
    bool CheckCollision(const UBody *other) const;

    // Energía cinética
    float GetKineticEnergy() const;

    // Momentum
    Vec3 GetMomentum() const;

    // Detener el cuerpo
    void Stop();

    // Debug info
    void PrintInfo() const;
};