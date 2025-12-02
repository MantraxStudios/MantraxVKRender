#pragma once
#include <vector>
#include "UBody.h"

class UBody;
class UPhysics
{
private:
    std::vector<UBody *> bodies; // Lista de todos los cuerpos físicos
    Vec3 globalGravity;          // Gravedad global
    float timeAccumulator;       // Para fixed timestep
    float fixedTimeStep;         // Timestep fijo para física estable

public:
    UPhysics();
    ~UPhysics();

    // Actualizar física
    void Update(float deltaTime);

    // Gestión de bodies
    void AddBody(UBody *body);
    void RemoveBody(UBody *body);
    void ClearBodies();

    // Getters
    int GetBodyCount() const;
    UBody *GetBody(int index) const;

    // Configuración
    void SetGlobalGravity(const Vec3 &gravity);
    Vec3 GetGlobalGravity() const;
    void SetFixedTimeStep(float timeStep);

    // Detección de colisiones
    bool CheckCollision(int indexA, int indexB) const;
    void ResolveCollisions();

    // Aplicar fuerza a todos los cuerpos
    void ApplyForceToAll(const Vec3 &force);

    // Debug
    void PrintAllBodies() const;
};