#pragma once
#include "Math/Vector3f.h"
#include "Math/Matrix4f.h"

namespace Magic
{
class AABB3f {
    Vector3f min;
    Vector3f max;
    AABB3f(const Vector3f& _min, const Vector3f& _max) : min(_min), max(_max) {}
public:
    AABB3f() // Init max as min and min as max
        : min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
        , max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest())
    {}

    static AABB3f FromMinMax(const Vector3f& _min, const Vector3f& _max) 
    {
        return AABB3f(_min, _max);
    }

    void Update(const Vector3f& v)
    {
        // Element-wise min and max
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
    }
    Vector3f GetMin() const { return min; }
    Vector3f GetMax() const { return max; }

    // Transform the AABB into a different space, result is non-optimal but probably good enough
    // For an optimal result, would need to recompute via Update() above looping over all transformed vertices
    // https://gamemath.com/book/geomprims.html#transforming_aabbs
    void Transform(const AABB3f& box, const Matrix4f& m)
    {
        min = max = m.GetTranslate();

        // Row major storage
        // m00, m01, m02, m03
        // m10, m11, m12, m13
        // m20, m21, m22, m23
        // m30, m31, m32, m33

        // First row, transforming some corner [x y z]: 
        // x'min = m00 * x + m01 * y + m02 * z
        // Minimize each term, m00*x is smallest for x = xmin if m00 > 0, otherwise for m00 < 0, x should be xmax (most negative)
        if (m.m[0] > 0.0f)
        {
            min.x += m.m[0] * box.min.x;
            max.x += m.m[0] * box.max.x;
        }
        else
        {
            min.x += m.m[0] * box.max.x;
            max.x += m.m[0] * box.min.x;
        }
        if (m.m[1] > 0.0f)
        {
            min.y += m.m[1] * box.min.y;
            max.y += m.m[1] * box.max.y;
        }
        else
        {
            min.y += m.m[1] * box.max.y;
            max.y += m.m[1] * box.min.y;
        }
        if (m.m[2] > 0.0f)
        {
            min.z += m.m[2] * box.min.z;
            max.z += m.m[2] * box.max.z;
        }
        else
        {
            min.z += m.m[2] * box.max.z;
            max.z += m.m[2] * box.min.z;
        }

        // Second row
        if (m.m[4] > 0.0f)
        {
            min.x += m.m[4] * box.min.x;
            max.x += m.m[4] * box.max.x;
        }
        else
        {
            min.x += m.m[4] * box.max.x;
            max.x += m.m[4] * box.min.x;
        }
        if (m.m[5] > 0.0f)
        {
            min.y += m.m[5] * box.min.y;
            max.y += m.m[5] * box.max.y;
        }
        else
        {
            min.y += m.m[5] * box.max.y;
            max.y += m.m[5] * box.min.y;
        }
        if (m.m[6] > 0.0f)
        {
            min.z += m.m[6] * box.min.z;
            max.z += m.m[6] * box.max.z;
        }
        else
        {
            min.z += m.m[6] * box.max.z;
            max.z += m.m[6] * box.min.z;
        }

        // Third row
        if (m.m[8] > 0.0f)
        {
            min.x += m.m[8] * box.min.x;
            max.x += m.m[8] * box.max.x;
        }
        else
        {
            min.x += m.m[8] * box.max.x;
            max.x += m.m[8] * box.min.x;
        }
        if (m.m[9] > 0.0f)
        {
            min.y += m.m[9] * box.min.y;
            max.y += m.m[9] * box.max.y;
        }
        else
        {
            min.y += m.m[9] * box.max.y;
            max.y += m.m[9] * box.min.y;
        }
        if (m.m[10] > 0.0f)
        {
            min.z += m.m[10] * box.min.z;
            max.z += m.m[10] * box.max.z;
        }
        else
        {
            min.z += m.m[10] * box.max.z;
            max.z += m.m[10] * box.min.z;
        }
    }

    // void Transform(const AABB3f& box, const Matrix4f& m)
    // {
    //     // Start from translation (last column)
    //     min = max = { m.m03, m.m13, m.m23 };

    //     auto accum = [](float mij, float bmin, float bmax, float& outMin, float& outMax)
    //     {
    //         float a = mij * bmin;
    //         float b = mij * bmax;
    //         if (a < b) { outMin += a; outMax += b; }
    //         else       { outMin += b; outMax += a; }
    //     };

    //     // Row 0 → x'
    //     accum(m.m00, box.min.x, box.max.x, min.x, max.x);
    //     accum(m.m01, box.min.y, box.max.y, min.x, max.x);
    //     accum(m.m02, box.min.z, box.max.z, min.x, max.x);

    //     // Row 1 → y'
    //     accum(m.m10, box.min.x, box.max.x, min.y, max.y);
    //     accum(m.m11, box.min.y, box.max.y, min.y, max.y);
    //     accum(m.m12, box.min.z, box.max.z, min.y, max.y);

    //     // Row 2 → z'
    //     accum(m.m20, box.min.x, box.max.x, min.z, max.z);
    //     accum(m.m21, box.min.y, box.max.y, min.z, max.z);
    //     accum(m.m22, box.min.z, box.max.z, min.z, max.z);
    // }


    // Non-optimal conservative bounding sphere of the AABB
    Vector3f GetCenter() const
    {
        return (max + min) * 0.5f; // Midpoint formula
    }
    float GetRadius() const
    {
        return ((max - min) * 0.5f).Length();
    }
};

}