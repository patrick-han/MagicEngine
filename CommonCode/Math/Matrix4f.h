#pragma once
#include "Vector4f.h"

namespace Magic
{
struct Vector3f;

class Matrix4f {
public:
    // Row major storage
    float m[16];
    void Identity();
    Matrix4f();
    Matrix4f(
          float _m00, float _m01, float _m02, float _m03
        , float _m10, float _m11, float _m12, float _m13
        , float _m20, float _m21, float _m22, float _m23
        , float _m30, float _m31, float _m32, float _m33);

    static Matrix4f FromColumns(const Vector4f& c1, const Vector4f& c2, const Vector4f& c3, const Vector4f& c4);

    static Matrix4f FromRows(const Vector4f& r1, const Vector4f& r2, const Vector4f& r3, const Vector4f& r4);

    static Matrix4f MakeScale(float s);
    static Matrix4f MakeScale(float sx, float sy, float sz);
    static Matrix4f MakeScale(const Vector3f& scale);

    static Matrix4f MakeTranslate(float tx, float ty, float tz);
    static Matrix4f MakeTranslate(const Vector3f& translate);
    [[nodiscard]] Vector3f GetTranslate() const;

    // Right handed, CCW is a positive rotation
    static Matrix4f MakeRotateX(float rads);
    static Matrix4f MakeRotateY(float rads);
    static Matrix4f MakeRotateZ(float rads);

    Matrix4f Transposed();

    // Inverse of a transform matrix assuming no deformation
    // Mainly this can be used to compute the View matrix (world-to-camera) from the camera's world matrix (camera-to-world)
    Matrix4f InvertedRigid();

    float Trace();
};

inline Vector4f operator* (const Matrix4f& mat, const Vector4f& v) {
    Vector4f res;
    res.x = v.x * mat.m[0] + v.y * mat.m[1] + v.z * mat.m[2] + v.w * mat.m[3];
    res.y = v.x * mat.m[4] + v.y * mat.m[5] + v.z * mat.m[6] + v.w * mat.m[7];
    res.z = v.x * mat.m[8] + v.y * mat.m[9] + v.z * mat.m[10] + v.w * mat.m[11];
    res.w = v.x * mat.m[12] + v.y * mat.m[13] + v.z * mat.m[14] + v.w * mat.m[15];
    return res;
}

inline Matrix4f operator* (const Matrix4f& mat1, const Matrix4f& mat2) {
#ifdef USE_ARM_NEON
    float32x4_t vm1_row0 = vld1q_f32(&mat1.m[0]);
    float32x4_t vm1_row1 = vld1q_f32(&mat1.m[4]);
    float32x4_t vm1_row2 = vld1q_f32(&mat1.m[8]);
    float32x4_t vm1_row3 = vld1q_f32(&mat1.m[12]);

    float32x4_t vm2_row0 = vld1q_f32(&mat2.m[0]);
    float32x4_t vm2_row1 = vld1q_f32(&mat2.m[4]);
    float32x4_t vm2_row2 = vld1q_f32(&mat2.m[8]);
    float32x4_t vm2_row3 = vld1q_f32(&mat2.m[12]);

    Matrix4f result;

    float32x4_t row0 = vmulq_laneq_f32(vm2_row0, vm1_row0, 0);
    row0 = vfmaq_laneq_f32(row0, vm2_row1, vm1_row0, 1);
    row0 = vfmaq_laneq_f32(row0, vm2_row2, vm1_row0, 2);
    row0 = vfmaq_laneq_f32(row0, vm2_row3, vm1_row0, 3);
    vst1q_f32(&result.m[0], row0);

    float32x4_t row1 = vmulq_laneq_f32(vm2_row0, vm1_row1, 0);
    row1 = vfmaq_laneq_f32(row1, vm2_row1, vm1_row1, 1);
    row1 = vfmaq_laneq_f32(row1, vm2_row2, vm1_row1, 2);
    row1 = vfmaq_laneq_f32(row1, vm2_row3, vm1_row1, 3);
    vst1q_f32(&result.m[4], row1);

    float32x4_t row2 = vmulq_laneq_f32(vm2_row0, vm1_row2, 0);
    row2 = vfmaq_laneq_f32(row2, vm2_row1, vm1_row2, 1);
    row2 = vfmaq_laneq_f32(row2, vm2_row2, vm1_row2, 2);
    row2 = vfmaq_laneq_f32(row2, vm2_row3, vm1_row2, 3);
    vst1q_f32(&result.m[8], row2);

    float32x4_t row3 = vmulq_laneq_f32(vm2_row0, vm1_row3, 0);
    row3 = vfmaq_laneq_f32(row3, vm2_row1, vm1_row3, 1);
    row3 = vfmaq_laneq_f32(row3, vm2_row2, vm1_row3, 2);
    row3 = vfmaq_laneq_f32(row3, vm2_row3, vm1_row3, 3);
    vst1q_f32(&result.m[12], row3);

    return result;
#else
    return Matrix4f(
        // Row 0
        mat1.m[0] * mat2.m[0] + mat1.m[1] * mat2.m[4] + mat1.m[2] * mat2.m[8] + mat1.m[3] * mat2.m[12],
        mat1.m[0] * mat2.m[1] + mat1.m[1] * mat2.m[5] + mat1.m[2] * mat2.m[9] + mat1.m[3] * mat2.m[13],
        mat1.m[0] * mat2.m[2] + mat1.m[1] * mat2.m[6] + mat1.m[2] * mat2.m[10] + mat1.m[3] * mat2.m[14],
        mat1.m[0] * mat2.m[3] + mat1.m[1] * mat2.m[7] + mat1.m[2] * mat2.m[11] + mat1.m[3] * mat2.m[15],
        // Row 1
        mat1.m[4] * mat2.m[0] + mat1.m[5] * mat2.m[4] + mat1.m[6] * mat2.m[8]  + mat1.m[7] * mat2.m[12],
        mat1.m[4] * mat2.m[1] + mat1.m[5] * mat2.m[5] + mat1.m[6] * mat2.m[9]  + mat1.m[7] * mat2.m[13],
        mat1.m[4] * mat2.m[2] + mat1.m[5] * mat2.m[6] + mat1.m[6] * mat2.m[10] + mat1.m[7] * mat2.m[14],
        mat1.m[4] * mat2.m[3] + mat1.m[5] * mat2.m[7] + mat1.m[6] * mat2.m[11] + mat1.m[7] * mat2.m[15],
        // Row 2
        mat1.m[8] * mat2.m[0] + mat1.m[9] * mat2.m[4] + mat1.m[10] * mat2.m[8]  + mat1.m[11] * mat2.m[12],
        mat1.m[8] * mat2.m[1] + mat1.m[9] * mat2.m[5] + mat1.m[10] * mat2.m[9]  + mat1.m[11] * mat2.m[13],
        mat1.m[8] * mat2.m[2] + mat1.m[9] * mat2.m[6] + mat1.m[10] * mat2.m[10] + mat1.m[11] * mat2.m[14],
        mat1.m[8] * mat2.m[3] + mat1.m[9] * mat2.m[7] + mat1.m[10] * mat2.m[11] + mat1.m[11] * mat2.m[15],
        // Row 3
        mat1.m[12] * mat2.m[0] + mat1.m[13] * mat2.m[4] + mat1.m[14] * mat2.m[8]  + mat1.m[15] * mat2.m[12],
        mat1.m[12] * mat2.m[1] + mat1.m[13] * mat2.m[5] + mat1.m[14] * mat2.m[9]  + mat1.m[15] * mat2.m[13],
        mat1.m[12] * mat2.m[2] + mat1.m[13] * mat2.m[6] + mat1.m[14] * mat2.m[10] + mat1.m[15] * mat2.m[14],
        mat1.m[12] * mat2.m[3] + mat1.m[13] * mat2.m[7] + mat1.m[14] * mat2.m[11] + mat1.m[15] * mat2.m[15]
    );
#endif
}
}