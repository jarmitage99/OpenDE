#pragma once

#include "core/math.h"

#include <array>
#include <cmath>

namespace dx {
    using Mat4 = std::array<float, 16>;

    inline Mat4 mat4Multiply(const Mat4& a, const Mat4& b)
    {
        Mat4 result {};
        for (size_t col = 0; col < 4; col++) {
            for (size_t row = 0; row < 4; row++) {
                float sum = 0;
                for (size_t k = 0; k < 4; k++) {
                    sum += a[k * 4 + row] * b[col * 4 + k];
                }
                result[col * 4 + row] = sum;
            }
        }
        return result;
    }

    inline Mat4 viewMatrix(const Vector& location, const Coords& axes)
    {
        const Vector& forward = axes.xAxis;
        const Vector& right = axes.yAxis;
        const Vector& up = axes.zAxis;
        return {
            right.x, up.x, -forward.x, 0,
            right.y, up.y, -forward.y, 0,
            right.z, up.z, -forward.z, 0,
            -right.dot(location), -up.dot(location), forward.dot(location), 1
        };
    }

    inline Mat4 viewMatrix(const Vector& location, const Rotator& rotation)
    {
        return viewMatrix(location, rotationAxes(rotation));
    }

    inline Mat4 projectionMatrix(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        float halfWidth = std::tan(fovDegrees * Pi / 360.0f);
        float sx = 1.0f / halfWidth;
        float sy = sx * aspect;
        return {
            sx, 0, 0, 0,
            0, sy, 0, 0,
            0, 0, -(farPlane + nearPlane) / (farPlane - nearPlane), -1,
            0, 0, -2.0f * farPlane * nearPlane / (farPlane - nearPlane), 0
        };
    }

    inline Mat4 orthoMatrix(float width, float height)
    {
        return {
            2.0f / width, 0, 0, 0,
            0, -2.0f / height, 0, 0,
            0, 0, -1, 0,
            -1, 1, 0, 1
        };
    }
}
