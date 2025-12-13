#pragma once

#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>

// Forward declaration
struct RenderableObject;

// ============================================================================
// COMPONENTES BÁSICOS - DEFINIDOS UNA SOLA VEZ
// ============================================================================

struct Transform
{
private:
    glm::vec3 position_{0.0f};
    glm::quat rotation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale_{1.0f};

    Transform *parent_ = nullptr;
    std::vector<Transform *> children_;

    glm::mat4 localMatrix_{1.0f};
    glm::mat4 worldMatrix_{1.0f};

    bool dirty_ = true;

    glm::vec3 QuaternionToEuler(const glm::quat &q) const
    {
        glm::vec3 angles;

        float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
        float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
        angles.x = std::atan2(sinr_cosp, cosr_cosp);

        float sinp = 2.0f * (q.w * q.y - q.z * q.x);
        if (std::abs(sinp) >= 1.0f)
            angles.y = std::copysign(glm::pi<float>() / 2.0f, sinp);
        else
            angles.y = std::asin(sinp);

        float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
        float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
        angles.z = std::atan2(siny_cosp, cosy_cosp);

        return angles;
    }

    glm::quat EulerToQuaternion(const glm::vec3 &euler) const
    {
        float cx = std::cos(euler.x * 0.5f);
        float sx = std::sin(euler.x * 0.5f);
        float cy = std::cos(euler.y * 0.5f);
        float sy = std::sin(euler.y * 0.5f);
        float cz = std::cos(euler.z * 0.5f);
        float sz = std::sin(euler.z * 0.5f);

        glm::quat q;
        q.w = cx * cy * cz + sx * sy * sz;
        q.x = sx * cy * cz - cx * sy * sz;
        q.y = cx * sy * cz + sx * cy * sz;
        q.z = cx * cy * sz - sx * sy * cz;

        return glm::normalize(q);
    }

public:
    Transform() = default;
    Transform(float x, float y, float z) : position_(x, y, z) {}
    Transform(const glm::vec3 &pos) : position_(pos) {}

    const glm::vec3 &GetPosition() const { return position_; }
    const glm::quat &GetRotation() const { return rotation_; }
    const glm::vec3 &GetScale() const { return scale_; }
    const glm::mat4 &GetLocalMatrix() const { return localMatrix_; }
    const glm::mat4 &GetWorldMatrix() const { return worldMatrix_; }
    Transform *GetParent() const { return parent_; }
    const std::vector<Transform *> &GetChildren() const { return children_; }
    bool IsDirty() const { return dirty_; }

    glm::vec3 GetEulerAngles() const
    {
        return QuaternionToEuler(rotation_);
    }

    glm::vec3 GetEulerAnglesDegrees() const
    {
        return glm::degrees(QuaternionToEuler(rotation_));
    }

    glm::vec3 GetForward() const
    {
        return glm::normalize(rotation_ * glm::vec3(0, 0, -1));
    }

    glm::vec3 GetRight() const
    {
        return glm::normalize(rotation_ * glm::vec3(1, 0, 0));
    }

    glm::vec3 GetUp() const
    {
        return glm::normalize(rotation_ * glm::vec3(0, 1, 0));
    }

    void SetPosition(const glm::vec3 &pos)
    {
        if (position_ != pos)
        {
            position_ = pos;
            MarkDirty();
        }
    }

    void SetPosition(float x, float y, float z)
    {
        SetPosition(glm::vec3(x, y, z));
    }

    void SetRotation(const glm::quat &rot)
    {
        rotation_ = glm::normalize(rot);
        MarkDirty();
    }

    void SetRotationFromEuler(const glm::vec3 &eulerAngles)
    {
        rotation_ = EulerToQuaternion(eulerAngles);
        MarkDirty();
    }

    void SetRotationFromEuler(float x, float y, float z)
    {
        SetRotationFromEuler(glm::vec3(x, y, z));
    }

    void SetRotationFromEulerDegrees(const glm::vec3 &eulerDegrees)
    {
        SetRotationFromEuler(glm::radians(eulerDegrees));
    }

    void SetRotationFromEulerDegrees(float x, float y, float z)
    {
        SetRotationFromEulerDegrees(glm::vec3(x, y, z));
    }

    void SetRotationFromAxisAngle(const glm::vec3 &axis, float angleRadians)
    {
        rotation_ = glm::angleAxis(angleRadians, glm::normalize(axis));
        MarkDirty();
    }

    void LookAt(const glm::vec3 &target, const glm::vec3 &up = glm::vec3(0, 1, 0))
    {
        glm::vec3 direction = glm::normalize(target - position_);
        rotation_ = glm::quatLookAt(direction, up);
        MarkDirty();
    }

    void SetScale(const glm::vec3 &s)
    {
        if (scale_ != s)
        {
            scale_ = s;
            MarkDirty();
        }
    }

    void SetScale(float x, float y, float z)
    {
        SetScale(glm::vec3(x, y, z));
    }

    void SetScale(float uniformScale)
    {
        SetScale(glm::vec3(uniformScale));
    }

    void Translate(const glm::vec3 &offset)
    {
        position_ += offset;
        MarkDirty();
    }

    void Rotate(const glm::quat &rotation)
    {
        rotation_ = glm::normalize(rotation * rotation_);
        MarkDirty();
    }

    void RotateEuler(const glm::vec3 &eulerAngles)
    {
        Rotate(EulerToQuaternion(eulerAngles));
    }

    void RotateEuler(float x, float y, float z)
    {
        RotateEuler(glm::vec3(x, y, z));
    }

    void RotateAxisAngle(const glm::vec3 &axis, float angleRadians)
    {
        Rotate(glm::angleAxis(angleRadians, glm::normalize(axis)));
    }

    void RotateAroundAxis(const glm::vec3 &axis, float angleRadians)
    {
        glm::vec3 worldAxis = rotation_ * glm::normalize(axis);
        RotateAxisAngle(worldAxis, angleRadians);
    }

    void RotateX(float angleRadians)
    {
        glm::quat rotX = glm::angleAxis(angleRadians, glm::vec3(1, 0, 0));
        rotation_ = rotX * rotation_;
        MarkDirty();
    }

    void RotateY(float angleRadians)
    {
        glm::quat rotY = glm::angleAxis(angleRadians, glm::vec3(0, 1, 0));
        rotation_ = rotY * rotation_;
        MarkDirty();
    }

    void RotateZ(float angleRadians)
    {
        glm::quat rotZ = glm::angleAxis(angleRadians, glm::vec3(0, 0, 1));
        rotation_ = rotZ * rotation_;
        MarkDirty();
    }

    void RotateXDegrees(float angleDegrees)
    {
        RotateX(glm::radians(angleDegrees));
    }

    void RotateYDegrees(float angleDegrees)
    {
        RotateY(glm::radians(angleDegrees));
    }

    void RotateZDegrees(float angleDegrees)
    {
        RotateZ(glm::radians(angleDegrees));
    }

    void RotateLocal(const glm::vec3 &axis, float angleRadians)
    {
        glm::vec3 localAxis = glm::normalize(axis);
        glm::quat localRot = glm::angleAxis(angleRadians, localAxis);
        rotation_ = rotation_ * localRot;
        MarkDirty();
    }

    void RotateWorld(const glm::vec3 &axis, float angleRadians)
    {
        glm::vec3 worldAxis = glm::normalize(axis);
        glm::quat worldRot = glm::angleAxis(angleRadians, worldAxis);
        rotation_ = worldRot * rotation_;
        MarkDirty();
    }

    void ScaleBy(const glm::vec3 &factor)
    {
        scale_ *= factor;
        MarkDirty();
    }

    void ScaleBy(float uniformFactor)
    {
        scale_ *= uniformFactor;
        MarkDirty();
    }

    void UpdateLocalMatrix()
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position_);
        glm::mat4 rotation = glm::mat4_cast(rotation_);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale_);

        localMatrix_ = translation * rotation * scaleMatrix;
    }

    void UpdateWorldMatrix()
    {
        if (dirty_)
        {
            UpdateLocalMatrix();

            if (parent_)
            {
                worldMatrix_ = parent_->worldMatrix_ * localMatrix_;
            }
            else
            {
                worldMatrix_ = localMatrix_;
            }

            dirty_ = false;
        }

        for (Transform *child : children_)
        {
            child->UpdateWorldMatrix();
        }
    }

    glm::vec3 GetWorldPosition() const
    {
        return glm::vec3(worldMatrix_[3]);
    }

    glm::quat GetWorldRotation() const
    {
        if (!parent_)
            return rotation_;

        return parent_->GetWorldRotation() * rotation_;
    }

    glm::vec3 GetWorldScale() const
    {
        glm::vec3 worldScale = scale_;
        Transform *p = parent_;
        while (p)
        {
            worldScale *= p->scale_;
            p = p->parent_;
        }
        return worldScale;
    }

    void AttachChild(Transform *child)
    {
        if (!child || child == this)
            return;

        if (child->parent_)
            child->parent_->DetachChild(child);

        child->parent_ = this;
        children_.push_back(child);
        child->MarkDirty();
    }

    void DetachChild(Transform *child)
    {
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end())
        {
            (*it)->parent_ = nullptr;
            children_.erase(it);
            (*it)->MarkDirty();
        }
    }

    void DetachFromParent()
    {
        if (parent_)
            parent_->DetachChild(this);
    }

    void MarkDirty()
    {
        dirty_ = true;
        for (Transform *child : children_)
        {
            child->MarkDirty();
        }
    }
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