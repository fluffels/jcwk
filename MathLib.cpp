#pragma once

#include <math.h>

#include "MathLib.h"

#ifndef max
#define max(a, b) a > b? a: b
#endif

#ifndef min
#define min(a, b) a < b? a: b
#endif

static inline float toRadians(float d) {
    return (d / 180.f) * PI;
}

static inline bool vectorEquals(Vec3i& lhs, Vec3i& rhs) {
    bool result = true;
    if (lhs.x != rhs.x) result = false;
    if (lhs.y != rhs.y) result = false;
    if (lhs.z != rhs.z) result = false;
    return result;
}

static inline void vectorCross(Vec3& a, Vec3& b, Vec3& r) {
    r.x = a.y*b.z - a.z*b.y;
    r.y = a.z*b.x - a.x*b.z;
    r.z = a.x*b.y - a.y*b.x;
}

static inline float vectorMagnitude(Vec3& v) {
    float result = 0;

    result += powf(v.x, 2);
    result += powf(v.y, 2);
    result += powf(v.z, 2);
    result /= sqrtf(result);

    return result;
}

static inline void vectorNormalize(Vec3& v) {
    float magnitude = vectorMagnitude(v);
    v.x /= magnitude;
    v.y /= magnitude;
    v.z /= magnitude;
}

static inline void vectorScale(float d, Vec3& v) {
    v.x *= d;
    v.y *= d;
    v.z *= d;
}

static inline void vectorAdd(Vec3& a, Vec3& b, Vec3& r) {
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
}

static inline void vectorSub(Vec3& a, Vec3& b, Vec3& r) {
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
}

static inline void matrixInit(float* m) {
    *m++ = 1;
    *m++ = 0;
    *m++ = 0;
    *m++ = 0;

    *m++ = 0;
    *m++ = 1;
    *m++ = 0;
    *m++ = 0;

    *m++ = 0;
    *m++ = 0;
    *m++ = 1;
    *m++ = 0;

    *m++ = 0;
    *m++ = 0;
    *m++ = 0;
    *m++ = 1;
}

static inline void matrixCopy(float* s, float* d) {
    for (int i = 0; i < 16; i++) {
        *d++ = *s++;
    }
}

static inline void matrixScale(float x, float y, float z, float* m) {
    m[0] *= x;
    m[5] *= y;
    m[10] *= z;
}

static inline void matrixScale(float s, float* m) {
    matrixScale(s, s, s, m);
}

static inline void matrixTranslate(float x, float y, float z, float* m) {
    m[12] += x;
    m[13] += y;
    m[14] += z;
}

static inline void matrixTranslate(Vec3 v, float* m) {
    matrixTranslate(v.x, v.y, v.z, m);
}

static inline void matrixMultiply(float* m, float* n, float* r) {
    for (int nIdx = 0; nIdx <= 12; nIdx += 4) {
        for (int mIdx = 0; mIdx <= 3; mIdx++) {
            *r = 0;
            for (int i = 0; i <= 3; i++) {
                *r += m[mIdx + i*4] * n[nIdx + i];
            }
            r++;
        }
    }
}

static inline void matrixMultiplyPoint(const float* m, Vec3& v, Vec3& r) {
    r.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12];
    r.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13];
    r.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14];
}

static inline void matrixMultiplyPoint(const float* m, Vec3& v, Vec4& r) {
    r.x = m[0]*v.x + m[4]*v.y + m[8]*v.z + m[12];
    r.y = m[1]*v.x + m[5]*v.y + m[9]*v.z + m[13];
    r.z = m[2]*v.x + m[6]*v.y + m[10]*v.z + m[14];
    r.w = m[3]*v.x + m[7]*v.y + m[11]*v.z + m[15];
}

static inline void matrixOrtho(
    uint32_t screenWidth,
    uint32_t screenHeight,
    float* m
) {
    matrixInit(m);

    const float ar = screenWidth / (float)screenHeight;

    m[0] = 2.f / screenWidth;
    m[12] = -1;
    m[5] = 2.f / screenHeight;
    m[13] = -1;
    m[10] = 0;
}

static inline void matrixProjection(
    uint32_t screenWidth,
    uint32_t screenHeight,
    float fov,
    float farz,
    float nearz,
    float* m
) {
    matrixInit(m);

    const float ar = screenWidth / (float)screenHeight;
    const float halfFOV = fov / 2.f;
    const float halfTanFOV = tanf(halfFOV);

    m[0] = 1 / (ar * halfTanFOV);
    m[5] = 1 / halfTanFOV;
    m[10] = 1 / (farz - nearz);
    m[11] = 1;
    m[14] = -nearz / (farz - nearz);
}

static inline void matrixView(Vec3 pos, Vec3 view, Vec3 down, float* m) {
    matrixInit(m);

    Vec3 z = view;
    vectorNormalize(z);

    Vec3 x;
    vectorCross(down, z, x);
    vectorNormalize(x);

    Vec3 y;
    vectorCross(z, x, y);
    vectorNormalize(y);

    m[0] = x.x;
    m[1] = x.y;
    m[2] = x.z;
    m[4] = y.x;
    m[5] = y.y;
    m[6] = y.z;
    m[8] = z.x;
    m[9] = z.y;
    m[10] = z.z;

    matrixTranslate(-pos.x, -pos.y, -pos.z, m);
}

static inline void quaternionInit(Quaternion& q) {
    q.x = 0;
    q.y = 0;
    q.z = 0;
    q.w = 1;
}

static inline float quaternionMagnitude(Quaternion& q) {
    float result = 0;

    result += powf(q.w, 2);
    result += powf(q.x, 2);
    result += powf(q.y, 2);
    result += powf(q.z, 2);
    result /= sqrtf(result);

    return result;
}

static inline void quaternionLog(Quaternion& q) {
    INFO("%f %f %f %f %f", quaternionMagnitude(q), q.w, q.x, q.y, q.z);
}

static inline void quaternionNormalize(Quaternion& q) {
    float magnitude = quaternionMagnitude(q);
    q.w /= magnitude;
    q.x /= magnitude;
    q.y /= magnitude;
    q.z /= magnitude;
}

static inline Quaternion quaternionMultiply(Quaternion& q1, Quaternion& q2) {
    Quaternion r;

    r.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    r.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    r.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
    r.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;

    return r;
}

static inline Quaternion quaternionFromAngleAxis(float x, float y, float z, float angle) {
    Quaternion r;

    r.w = cosf(angle/2);
    r.x = x * sinf(angle/2);
    r.y = y * sinf(angle/2);
    r.z = z * sinf(angle/2);

    return r;
}

static inline Quaternion quaternionInverse(Quaternion q) {
    Quaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

    return result;
}

static inline void quaternionRotate(Quaternion q, Quaternion& p) {
    Quaternion qi = quaternionInverse(q);
    p = quaternionMultiply(q, p);
    p = quaternionMultiply(p, qi);
}

static inline void quaternionUnrotate(Quaternion q, Quaternion& p) {
    Quaternion qi = quaternionInverse(q);
    p = quaternionMultiply(qi, p);
    p = quaternionMultiply(p, q);
}

static void moveAlongQuaternion(float d, Quaternion q, Vec4& p) {
    Quaternion dir = {};
    dir.z = 1;
    quaternionUnrotate(q, dir);
    p.x += d * dir.x;
    p.y += d * dir.y;
    p.z += d * dir.z;
}

static void movePerpendicularToQuaternion(float d, Quaternion q, Vec4& p) {
    Quaternion dir = {};
    dir.x = 1;
    quaternionUnrotate(q, dir);
    p.x += d * dir.x;
    p.y += d * dir.y;
    p.z += d * dir.z;
}

static void rotateQuaternionY(float d, Quaternion& q) {
    auto rad = toRadians(d);
    auto delta = quaternionFromAngleAxis(0, 1, 0, rad);
    q = quaternionMultiply(delta, q);
}

static void rotateQuaternionX(float d, Quaternion& q) {
    auto rad = toRadians(d);
    auto delta = quaternionFromAngleAxis(1, 0, 0, rad);
    q = quaternionMultiply(delta, q);
}

static inline void quaternionToMatrix(Quaternion& q, float* m) {
    *m++ = powf(q.w, 2) + powf(q.x, 2) - powf(q.y, 2) - powf(q.z, 2);
    *m++ = 2*q.x*q.y + 2*q.w*q.z;
    *m++ = 2*q.x*q.z - 2*q.w*q.y;
    *m++ = 0;

    *m++ = 2*q.x*q.y - 2*q.w*q.z;
    *m++ = powf(q.w, 2) - powf(q.x, 2) + powf(q.y, 2) - powf(q.z, 2);
    *m++ = 2*q.y*q.z - 2*q.w*q.x;
    *m++ = 0;

    *m++ = 2*q.x*q.z + 2*q.w*q.y;
    *m++ = 2*q.y*q.z + 2*q.w*q.x;
    *m++ = powf(q.w, 2) - powf(q.x, 2) - powf(q.y, 2) + powf(q.z, 2);
    *m++ = 0;

    *m++ = 0;
    *m++ = 0;
    *m++ = 0;
    *m++ = 1;
}

static inline void getXAxis(Quaternion& q, float* v) {
    float m[16];
    quaternionToMatrix(q, m);
    v[0] = m[0];
    v[1] = m[4];
    v[2] = m[8];
}

static inline void getZAxis(Quaternion& q, float* v) {
    float m[16];
    quaternionToMatrix(q, m);
    v[0] = m[2];
    v[1] = m[6];
    v[2] = m[10];
}

static inline void rotatePoint(Quaternion& q, Vec3& p, Vec3& result) {
    Quaternion qConj = quaternionInverse(q);
    Quaternion qPos = {p.x, p.y, p.z, 0 };

    Quaternion qTmp = quaternionMultiply(q, qPos);
    Quaternion r = quaternionMultiply(qTmp, qConj);

    result = { r.x, r.y, r.z };
}
