

#pragma once

#include <cmath>

struct Vec2
{
    float x{0.0f};
    float y{0.0f};

    Vec2() = default;

    Vec2(float x_, float y_)
        : x(x_), y(y_)
    {};
    
    Vec2 operator+(const Vec2& other) const
    {
        return {x+other.x, y+other.y};
    }
    
    Vec2 operator-(const Vec2& other) const
    {
        return {x-other.x, y-other.y};
    }
    
    Vec2 operator*(const Vec2& other) const
    {
        return {x*other.x, y*other.y};
    }
    
    float length() const
    {
        return std::sqrt(x*x+y*y);
    }
    
    Vec2 normalized() const
    {
        float len = length();
        return {x/len, y/len};
    }

    static float dot(const Vec2& a, const Vec2& b)
    {
        return a.x*b.x + a.y*b.y;
    }

    Vec2 operator*(float scalar) const
    {
        return {x*scalar, y*scalar};
    }

    Vec2 operator/(float scalar) const
    {
        return {x/scalar, y/scalar};
    }

    float lengthSquared() const
    {
        return x*x + y*y;
    }

    Vec2& operator+=(const Vec2& o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }

    Vec2& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vec2 operator-() const
    {
        return {-x, -y};
    }

    Vec2 projectOnto(const Vec2& other) const
    {
        float scale = dot(*this, other) / other.lengthSquared();
        return other * scale;
    }

};