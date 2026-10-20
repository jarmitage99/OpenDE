#pragma once

#include <cmath>
#include <cstdint>

namespace dx {
    struct Vector {
        float x = 0;
        float y = 0;
        float z = 0;

        Vector operator+(const Vector& o) const
        {
            return { x + o.x, y + o.y, z + o.z };
        }

        Vector operator-(const Vector& o) const
        {
            return { x - o.x, y - o.y, z - o.z };
        }

        Vector operator*(float s) const
        {
            return { x * s, y * s, z * s };
        }

        Vector operator*(const Vector& o) const
        {
            return { x * o.x, y * o.y, z * o.z };
        }

        Vector operator/(float s) const
        {
            return { x / s, y / s, z / s };
        }

        Vector operator-() const
        {
            return { -x, -y, -z };
        }

        Vector& operator+=(const Vector& o)
        {
            x += o.x;
            y += o.y;
            z += o.z;
            return *this;
        }

        Vector& operator-=(const Vector& o)
        {
            x -= o.x;
            y -= o.y;
            z -= o.z;
            return *this;
        }

        Vector& operator*=(float s)
        {
            x *= s;
            y *= s;
            z *= s;
            return *this;
        }

        bool operator==(const Vector& o) const
        {
            return x == o.x && y == o.y && z == o.z;
        }

        float dot(const Vector& o) const
        {
            return x * o.x + y * o.y + z * o.z;
        }

        Vector cross(const Vector& o) const
        {
            return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
        }

        float sizeSquared() const
        {
            return x * x + y * y + z * z;
        }

        float size() const
        {
            return std::sqrt(sizeSquared());
        }

        float size2D() const
        {
            return std::sqrt(x * x + y * y);
        }

        Vector normal() const
        {
            double squared = static_cast<double>(sizeSquared());
            if (squared < 1e-8) {
                return {};
            }
            double scale = 1.0 / std::sqrt(squared);
            return { static_cast<float>(static_cast<double>(x) * scale), static_cast<float>(static_cast<double>(y) * scale), static_cast<float>(static_cast<double>(z) * scale) };
        }

        bool zero() const
        {
            return x == 0 && y == 0 && z == 0;
        }
    };

    struct Rotator {
        int32_t pitch = 0;
        int32_t yaw = 0;
        int32_t roll = 0;

        bool operator==(const Rotator& o) const
        {
            return pitch == o.pitch && yaw == o.yaw && roll == o.roll;
        }
    };

    struct Plane {
        float x = 0;
        float y = 0;
        float z = 0;
        float w = 0;
    };

    struct Color {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        uint8_t a = 0;
    };

    struct Coords {
        Vector origin;
        Vector xAxis;
        Vector yAxis;
        Vector zAxis;
    };

    constexpr float Pi = 3.14159265358979f;

    inline float unrToRad(int32_t value)
    {
        return static_cast<float>(value & 65535) * (2.0f * Pi / 65536.0f);
    }

    inline Coords rotationAxes(const Rotator& rotation)
    {
        float sp = std::sin(unrToRad(rotation.pitch));
        float cp = std::cos(unrToRad(rotation.pitch));
        float sy = std::sin(unrToRad(rotation.yaw));
        float cy = std::cos(unrToRad(rotation.yaw));
        float sr = std::sin(unrToRad(rotation.roll));
        float cr = std::cos(unrToRad(rotation.roll));
        Coords c;
        c.xAxis = { cp * cy, cp * sy, sp };
        c.yAxis = { sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp };
        c.zAxis = { -(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp };
        return c;
    }

    inline Vector rotationVector(const Rotator& rotation)
    {
        return rotationAxes(rotation).xAxis;
    }

    inline Rotator vectorRotation(const Vector& v)
    {
        Rotator r;
        r.yaw = static_cast<int32_t>(std::atan2(v.y, v.x) * 32768.0f / Pi) & 65535;
        r.pitch = static_cast<int32_t>(std::atan2(v.z, std::sqrt(v.x * v.x + v.y * v.y)) * 32768.0f / Pi) & 65535;
        r.roll = 0;
        return r;
    }

    inline Vector transformVector(const Coords& c, const Vector& v)
    {
        return { v.dot(c.xAxis), v.dot(c.yAxis), v.dot(c.zAxis) };
    }

    inline Vector untransformVector(const Coords& c, const Vector& v)
    {
        return c.xAxis * v.x + c.yAxis * v.y + c.zAxis * v.z;
    }
}
