#pragma once

#include <cstdint>

#include "Types.h"

#define PI 3.14159265358979323846f

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
