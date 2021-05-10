#pragma once

#include <cstdint>

#include "Types.h"

#define PI 3.14159265358979323846f

struct Vec2i {
    i32 x;
    i32 y;
};

struct Vec2 {
    f32 x;
    f32 y;
};

struct Vec3i {
    i32 x;
    i32 y;
    i32 z;
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
    i32 x;
    i32 y;
    i32 z;
    i32 w;
};

struct Quaternion {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
};
