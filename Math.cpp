#include "Math.hpp"
#include <cmath>

namespace Math
{
    Mat4 identity()
    {
        Mat4 res;
        res.m[0] = res.m[5] = res.m[10] = res.m[15] = 1.0f;
        return res;
    }

    Mat4 multiply(const Mat4& a, const Mat4& b)
    {
        Mat4 res;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                float sum = 0.0f;
                for (int k = 0; k < 4; k++)
                {
                    sum += a.m[k * 4 + j] * b.m[i * 4 + k];
                }
                res.m[i * 4 + j] = sum;
            }
        }
        return res;
    }

    Mat4 perspective(float fov_rad, float aspect, float z_near, float z_far)
    {
        Mat4 res;
        float tan_half_fov = tanf(fov_rad / 2.0f);
        res.m[0] = 1.0f / (aspect * tan_half_fov);
        res.m[5] = 1.0f / tan_half_fov;
        res.m[10] = -(z_far + z_near) / (z_far - z_near);
        res.m[11] = -1.0f;
        res.m[14] = -2.0f * z_far * z_near / (z_far - z_near);
        return res;
    }

    Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
    {
        Vec3 f = { center.x - eye.x, center.y - eye.y, center.z - eye.z };
        float f_len = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
        if (f_len > 1e-6)
        {
            f.x /= f_len; f.y /= f_len; f.z /= f_len;
        }

        Vec3 s = { f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z, f.x * up.y - f.y * up.x };
        float s_len = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
        if (s_len > 1e-6)
        {
            s.x /= s_len; s.y /= s_len; s.z /= s_len;
        }

        Vec3 u = { s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z, s.x * f.y - s.y * f.x };

        Mat4 res = identity();
        res.m[0] = s.x; res.m[4] = s.y; res.m[8] = s.z;
        res.m[1] = u.x; res.m[5] = u.y; res.m[9] = u.z;
        res.m[2] = -f.x; res.m[6] = -f.y; res.m[10] = -f.z;
        res.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
        res.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
        res.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
        return res;
    }

    Mat4 translate(const Mat4& m, const Vec3& v)
    {
        Mat4 res = m;
        res.m[12] = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12];
        res.m[13] = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13];
        res.m[14] = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14];
        return res;
    }

    Mat4 scale(const Mat4& m, const Vec3& v)
    {
        Mat4 res = m;
        res.m[0] *= v.x; res.m[5] *= v.y; res.m[10] *= v.z;
        return res;
    }

    bool project(const Vec3& obj, const Mat4& view, const Mat4& proj, int viewport_width, int viewport_height, Vec3& screen)
    {
        float in[4] = { obj.x, obj.y, obj.z, 1.0f };
        float out[4];
        Mat4 view_proj = multiply(proj, view);

        out[0] = view_proj.m[0] * in[0] + view_proj.m[4] * in[1] + view_proj.m[8] * in[2] + view_proj.m[12] * in[3];
        out[1] = view_proj.m[1] * in[0] + view_proj.m[5] * in[1] + view_proj.m[9] * in[2] + view_proj.m[13] * in[3];
        out[2] = view_proj.m[2] * in[0] + view_proj.m[6] * in[1] + view_proj.m[10] * in[2] + view_proj.m[14] * in[3];
        out[3] = view_proj.m[3] * in[0] + view_proj.m[7] * in[1] + view_proj.m[11] * in[2] + view_proj.m[15] * in[3];

        if (out[3] < 0.001f) return false;

        out[0] /= out[3]; out[1] /= out[3]; out[2] /= out[3];
        screen.x = (out[0] * 0.5f + 0.5f) * viewport_width;
        screen.y = (-out[1] * 0.5f + 0.5f) * viewport_height;
        screen.z = out[2];
        return true;
    }
}
