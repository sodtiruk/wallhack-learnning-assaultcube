// Vectors.h
#pragma once
#include <cstddef>

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

struct Vec4 {
    float x, y, z, w;
};

// เมทริกซ์ 4x4 เช่น view matrix สำหรับ world-to-screen
struct Matrix4 {
    float m[16];
};

struct Matrix_4x4 {
    float m[4][4];
};

// กัน compiler เติม padding จนขนาดไม่ตรงกับในเกม
static_assert(sizeof(Vec2) == 8,  "Vec2 size mismatch");
static_assert(sizeof(Vec3) == 12, "Vec3 size mismatch");
static_assert(sizeof(Vec4) == 16, "Vec4 size mismatch");
static_assert(sizeof(Matrix4) == 64, "Matrix4 size mismatch");