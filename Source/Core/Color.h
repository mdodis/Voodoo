#pragma once
#include "MathTypes.h"

#pragma pack(push, 1)
struct Color {
    float r, g, b, a;

    static Color red() { return Color{1.0f, 0.0f, 0.0f, 1.0f}; }
    static Color green() { return Color{0.0f, 1.0f, 0.0f, 1.0f}; }
    static Color blue() { return Color{0.0f, 0.0f, 1.0f, 1.0f}; }
    static Color white() { return Color{1.0f, 1.0f, 1.0f, 1.0f}; }

    operator Vec4() const { return Vec4(r, g, b, a); }
};

#pragma pack(pop)
