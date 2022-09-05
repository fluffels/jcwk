#pragma once

#include <cstdint>

#include "Types.h"

#define PI 3.14159265358979323846f

#define KIBIBYTE 1024
#define MEBIBYTE 1024 * KIBIBYTE

struct Vec2i {
    s32 x;
    s32 y;
};

struct Vec2 {
    f32 x;
    f32 y;
};

struct Vec3i {
    s32 x;
    s32 y;
    s32 z;
};

struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
};

struct Vec4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct Vec4i {
    s32 x;
    s32 y;
    s32 z;
    s32 w;
};

struct Quaternion {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};

struct AABox {
    f32 x0;
    f32 x1;
    f32 y0;
    f32 y1;
};

struct Triangle {
    Vec3 p0;
    Vec3 p1;
    Vec3 p2;
};
