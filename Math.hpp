#pragma once

namespace Math
{
    struct Vec3 { float x, y, z; };
    struct Mat4 { float m[16] = { 0 }; };

    Mat4 identity();
    Mat4 multiply(const Mat4& a, const Mat4& b);
    Mat4 perspective(float fov_rad, float aspect, float z_near, float z_far);
    Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    Mat4 translate(const Mat4& m, const Vec3& v);
    Mat4 scale(const Mat4& m, const Vec3& v);
    bool project(const Vec3& obj, const Mat4& view, const Mat4& proj, int viewport_width, int viewport_height, Vec3& screen);
}
