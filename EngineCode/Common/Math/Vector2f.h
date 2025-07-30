#pragma once
#include <cmath>
namespace Magic
{
// Column vector
struct Vector2f {
    float x;
    float y;
    Vector2f() : x(0.0f), y(0.0f) {}
    Vector2f(float _x, float _y) : x(_x), y(_y) {}
    Vector2f(float _v) : x(_v), y(_v) {}
    Vector2f& operator*=(float c) {
        x *= c;
        y *= c;
        return *this; 
    }
    Vector2f& operator/=(float c) {
        x /= c;
        y /= c;
        return *this; 
    }
    Vector2f& operator+=(float c) {
        x += c;
        y += c;
        return *this; 
    }
    Vector2f& operator-=(float c) {
        x -= c;
        y -= c;
        return *this; 
    }
    Vector2f& operator+=(const Vector2f& right) {
        x += right.x;
        y += right.y;
        return *this; 
    }
    Vector2f& operator-=(const Vector2f& right) {
        x -= right.x;
        y -= right.y;
        return *this; 
    }
    float Length() {
        return std::sqrt(x * x + y * y);
    }
    Vector2f AsNormalized();
};

inline Vector2f operator+ (const Vector2f& left, const Vector2f& right) {
    Vector2f result;
    result.x = left.x + right.x;
    result.y = left.y + right.y;
    return result;
}

inline Vector2f operator- (const Vector2f& left, const Vector2f& right) {
    Vector2f result;
    result.x = left.x - right.x;
    result.y = left.y - right.y;
    return result;
}

inline Vector2f operator* (float scale, const Vector2f& right) {
    Vector2f result;
    result.x = scale * right.x;
    result.y = scale * right.y;
    return result;
}

inline Vector2f operator* (const Vector2f& left, float scale) {
    Vector2f result;
    result.x = scale * left.x;
    result.y = scale * left.y;
    return result;
}

inline Vector2f operator/ (const Vector2f& left, float div) {
    Vector2f result;
    result.x = left.x / div;
    result.y = left.y / div;
    return result;
}

inline Vector2f Vector2f::AsNormalized() {
    float mag = Length();
    Vector2f ret = *this / mag;
    return ret;
}

inline float Dot(const Vector2f& left, const Vector2f& right) {
    return left.x * right.x + left.y * right.y;
}
} // namespace Magic