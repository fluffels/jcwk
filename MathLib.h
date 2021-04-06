#pragma once

#include <cstdint>

#define PI 3.14159265358979323846f

struct Vec2i {
    int32_t x;
    int32_t y;
};

struct Vec2 {
    float x;
    float y;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vec4 {
    float x;
    float y;
    float z;
    float w;
};

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct BiVec3 {
    float b01;
    float b02;
    float b12;
};

struct Rotor3 {
    float a;
    float b01;
    float b02;
    float b12;
};
