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
    void Update(const Vector4f& v)
    {
        // Element-wise min and max
        min.x = std::min(min.x, v.v[0]);
        min.y = std::min(min.y, v.v[1]);
        min.z = std::min(min.z, v.v[2]);
        max.x = std::max(max.x, v.v[0]);
        max.y = std::max(max.y, v.v[1]);
        max.z = std::max(max.z, v.v[2]);
    }
    Vector3f GetMin() const { return min; }
    Vector3f GetMax() const { return max; }

    // Transform the AABB into a different space, result is non-optimal but probably good enough
    // For an optimal result, would need to recompute via Update() above looping over all transformed vertices
    // https://gamemath.com/book/geomprims.html#transforming_aabbs
    void Transform(const AABB3f& box, const Matrix4f& m)
    {
        min = max = m.GetTranslate();

        // Each matrix row calculates one output component: row 0 contributes only to X, row 1 only to Y, and row 2 only to Z. Mixing these contributions between output components breaks rotated AABBs.
        auto accumulate = [](float matrixElement, float boxMin, float boxMax, float& transformedMin, float& transformedMax)
        {
            // A negative matrix element reverses which endpoint produces the minimum, so test both products instead of assuming boxMin remains the transformed minimum.
            const float a = matrixElement * boxMin;
            const float b = matrixElement * boxMax;
            transformedMin += std::min(a, b);
            transformedMax += std::max(a, b);
        };

        accumulate(m(0, 0), box.min.x, box.max.x, min.x, max.x);
        accumulate(m(0, 1), box.min.y, box.max.y, min.x, max.x);
        accumulate(m(0, 2), box.min.z, box.max.z, min.x, max.x);

        accumulate(m(1, 0), box.min.x, box.max.x, min.y, max.y);
        accumulate(m(1, 1), box.min.y, box.max.y, min.y, max.y);
        accumulate(m(1, 2), box.min.z, box.max.z, min.y, max.y);

        accumulate(m(2, 0), box.min.x, box.max.x, min.z, max.z);
        accumulate(m(2, 1), box.min.y, box.max.y, min.z, max.z);
        accumulate(m(2, 2), box.min.z, box.max.z, min.z, max.z);
    }


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
