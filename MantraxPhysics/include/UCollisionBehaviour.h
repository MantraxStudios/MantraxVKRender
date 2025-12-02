
#pragma once
#include "UMath.h"

// Forward declaration
class UBody;

// Clase base para comportamientos de colisión
class UCollisionBehaviour
{
protected:
    UBody *attachedBody; // Referencia al cuerpo al que está adjunto

public:
    UCollisionBehaviour();
    virtual ~UCollisionBehaviour();

    // Adjuntar a un cuerpo
    void AttachToBody(UBody *body);

    // Obtener el cuerpo adjunto
    UBody *GetAttachedBody() const;

    // Método virtual para verificar colisión con otro comportamiento
    virtual bool CheckCollision(const UCollisionBehaviour *other) const = 0;

    // Obtener la posición desde el cuerpo adjunto
    Vec3 GetPosition() const;

    // Métodos virtuales puros para diferentes tipos de colisión
    virtual void DrawDebug() const = 0;
};

// Colisión esférica
class USphereCollision : public UCollisionBehaviour
{
private:
    float radius;
    Vec3 offset; // Offset relativo al cuerpo

public:
    USphereCollision(float r = 1.0f, Vec3 off = Vec3(0, 0, 0));

    bool CheckCollision(const UCollisionBehaviour *other) const override;
    bool CheckSphereCollision(const USphereCollision *other) const;

    void DrawDebug() const override;

    float GetRadius() const { return radius; }
    void SetRadius(float r) { radius = r; }
    Vec3 GetOffset() const { return offset; }
    Vec3 GetWorldPosition() const; // Posición en el mundo (body + offset)
};

// Colisión de caja (AABB)
class UBoxCollision : public UCollisionBehaviour
{
private:
    Vec3 halfExtents; // Mitad del tamaño en cada eje
    Vec3 offset;

public:
    UBoxCollision(Vec3 extents = Vec3(1, 1, 1), Vec3 off = Vec3(0, 0, 0));

    bool CheckCollision(const UCollisionBehaviour *other) const override;
    bool CheckBoxCollision(const UBoxCollision *other) const;
    bool CheckSphereCollision(const USphereCollision *other) const;

    void DrawDebug() const override;

    Vec3 GetHalfExtents() const { return halfExtents; }
    Vec3 GetWorldPosition() const;
    Vec3 GetMin() const; // Esquina mínima del AABB
    Vec3 GetMax() const; // Esquina máxima del AABB
};