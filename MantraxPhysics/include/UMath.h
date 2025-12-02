#pragma once
#include <cmath>

// Vector 2D
struct Vec2
{
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2 &other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2 &other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2 operator/(float scalar) const { return Vec2(x / scalar, y / scalar); }

    Vec2 &operator+=(const Vec2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vec2 &operator-=(const Vec2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    Vec2 &operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    Vec2 &operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Producto punto
    float Dot(const Vec2 &other) const { return x * other.x + y * other.y; }

    // Magnitud
    float Length() const { return std::sqrt(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }
    float Magnitude() const { return Length(); } // Alias para Length()

    // Normalizar
    Vec2 Normalized() const
    {
        float len = Length();
        return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
    }

    void Normalize()
    {
        float len = Length();
        if (len > 0)
        {
            x /= len;
            y /= len;
        }
    }

    // Distancia
    static float Distance(const Vec2 &a, const Vec2 &b)
    {
        return (b - a).Length();
    }
};

// Vector 3D
struct Vec3
{
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Operadores básicos
    Vec3 operator+(const Vec3 &other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3 &other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }

    Vec3 &operator+=(const Vec3 &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    Vec3 &operator-=(const Vec3 &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    Vec3 &operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    Vec3 &operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    // Producto punto
    float Dot(const Vec3 &other) const { return x * other.x + y * other.y + z * other.z; }

    // Producto cruz
    Vec3 Cross(const Vec3 &other) const
    {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x);
    }

    // Magnitud
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }
    float Magnitude() const { return Length(); }

    // Normalizar
    Vec3 Normalized() const
    {
        float len = Length();
        return len > 0 ? Vec3(x / len, y / len, z / len) : Vec3(0, 0, 0);
    }

    void Normalize()
    {
        float len = Length();
        if (len > 0)
        {
            x /= len;
            y /= len;
            z /= len;
        }
    }

    // Distancia
    static float Distance(const Vec3 &a, const Vec3 &b)
    {
        return (b - a).Length();
    }
};

// Clase UMath con utilidades de física
class UMath
{
public:
    // Constantes
    static constexpr float PI = 3.14159265359f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;
    static constexpr float GRAVITY = 9.81f;

    // Interpolación lineal
    static float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    static Vec2 Lerp(const Vec2 &a, const Vec2 &b, float t)
    {
        return Vec2(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t));
    }

    static Vec3 Lerp(const Vec3 &a, const Vec3 &b, float t)
    {
        return Vec3(Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t));
    }

    // Clamp
    static float Clamp(float value, float min, float max)
    {
        if (value < min)
            return min;
        if (value > max)
            return max;
        return value;
    }

    // Ángulo entre vectores
    static float Angle(const Vec3 &a, const Vec3 &b)
    {
        float dot = a.Dot(b);
        float lengthProduct = a.Length() * b.Length();
        if (lengthProduct == 0)
            return 0;
        return std::acos(Clamp(dot / lengthProduct, -1.0f, 1.0f)) * RAD_TO_DEG;
    }

    // Reflexión de un vector
    static Vec3 Reflect(const Vec3 &direction, const Vec3 &normal)
    {
        return direction - normal * (2.0f * direction.Dot(normal));
    }

    UMath() {}
    ~UMath() {}
};

// Operadores escalares a la izquierda
inline Vec2 operator*(float scalar, const Vec2 &vec) { return vec * scalar; }
inline Vec3 operator*(float scalar, const Vec3 &vec) { return vec * scalar; }