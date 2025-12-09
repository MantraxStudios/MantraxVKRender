#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Forward declaration
struct RenderableObject;

// ============================================================================
// COMPONENTES BÁSICOS - DEFINIDOS UNA SOLA VEZ
// ============================================================================

struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    Transform() = default;
    Transform(float x, float y, float z = 0.0f)
        : position(x, y, z) {}
    Transform(const glm::vec3 &pos)
        : position(pos) {}
};

struct Velocity
{
    glm::vec3 linear = glm::vec3(0.0f);
    glm::vec3 angular = glm::vec3(0.0f);

    Velocity() = default;
    Velocity(const glm::vec3 &lin, const glm::vec3 &ang = glm::vec3(0.0f))
        : linear(lin), angular(ang) {}
};

struct Rotator
{
    glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
    float speed = 1.0f;

    Rotator() = default;
    Rotator(const glm::vec3 &axis, float speed)
        : axis(axis), speed(speed) {}
};

struct RenderComponent
{
    RenderableObject *renderObject = nullptr;
    bool isVisible = true;

    RenderComponent() = default;
    RenderComponent(RenderableObject *obj)
        : renderObject(obj) {}
};

struct Sprite
{
    std::string texturePath;
    int width = 32;
    int height = 32;
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

class Rigidbody
{
private:
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float mass = 1.0f;
    float drag = 0.99f;
    bool useGravity = true;

public:
    void setVelocity(float vx, float vy)
    {
        velocityX = vx;
        velocityY = vy;
    }

    void addForce(float fx, float fy)
    {
        velocityX += fx / mass;
        velocityY += fy / mass;
    }

    void setMass(float m)
    {
        mass = m > 0.0f ? m : 0.1f;
    }

    void setDrag(float d)
    {
        drag = d;
    }

    void setUseGravity(bool enabled)
    {
        useGravity = enabled;
    }

    float getVelocityX() const { return velocityX; }
    float getVelocityY() const { return velocityY; }
    float getMass() const { return mass; }
    bool isUsingGravity() const { return useGravity; }

    void update(float dt, float gravity = 9.8f)
    {
        if (useGravity)
            velocityY += gravity * dt;

        velocityX *= drag;
        velocityY *= drag;
    }
};

class Health
{
private:
    int current = 100;
    int max = 100;
    bool isDead = false;

public:
    Health() = default;
    Health(int maxHealth) : current(maxHealth), max(maxHealth) {}

    void damage(int amount)
    {
        if (isDead)
            return;

        current -= amount;
        if (current <= 0)
        {
            current = 0;
            isDead = true;
        }
    }

    void heal(int amount)
    {
        if (isDead)
            return;
        current = std::min(max, current + amount);
    }

    void setMax(int value)
    {
        max = value;
        if (current > max)
            current = max;
    }

    void revive()
    {
        isDead = false;
        current = max;
    }

    int getCurrent() const { return current; }
    int getMax() const { return max; }
    bool getIsDead() const { return isDead; }
    float getPercentage() const { return (float)current / (float)max; }
};

class BoxCollider
{
private:
    float width = 1.0f;
    float height = 1.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    bool isTrigger = false;

public:
    BoxCollider() = default;
    BoxCollider(float w, float h) : width(w), height(h) {}

    void setSize(float w, float h)
    {
        width = w;
        height = h;
    }

    void setOffset(float x, float y)
    {
        offsetX = x;
        offsetY = y;
    }

    void setTrigger(bool trigger)
    {
        isTrigger = trigger;
    }

    float getWidth() const { return width; }
    float getHeight() const { return height; }
    float getOffsetX() const { return offsetX; }
    float getOffsetY() const { return offsetY; }
    bool getIsTrigger() const { return isTrigger; }

    bool intersects(const BoxCollider &other,
                    float myX, float myY,
                    float otherX, float otherY) const
    {
        float left1 = myX + offsetX - width * 0.5f;
        float right1 = myX + offsetX + width * 0.5f;
        float top1 = myY + offsetY - height * 0.5f;
        float bottom1 = myY + offsetY + height * 0.5f;

        float left2 = otherX + other.offsetX - other.width * 0.5f;
        float right2 = otherX + other.offsetX + other.width * 0.5f;
        float top2 = otherY + other.offsetY - other.height * 0.5f;
        float bottom2 = otherY + other.offsetY + other.height * 0.5f;

        return !(right1 < left2 || left1 > right2 ||
                 bottom1 < top2 || top1 > bottom2);
    }
};

// AQUÍ TERMINA EL ARCHIVO - NO PONGAS MÁS CÓDIGO