#include "core_math.h"

internal m4 identity_m4() {
    m4 result{};
    result._00 = 1.0f;
    result._11 = 1.0f;
    result._22 = 1.0f;
    result._33 = 1.0f;
    return result;
}

internal m4 transpose_m4(m4 m) {
    m4 result = m;
    result._01 = m._10;
    result._02 = m._20;
    result._03 = m._30;
    result._10 = m._01;
    result._12 = m._21;
    result._13 = m._31;
    result._20 = m._02;
    result._21 = m._12;
    result._23 = m._32;
    result._30 = m._03;
    result._31 = m._13;
    result._32 = m._23;
    return result;
}

internal m4 mul_m4(m4 b, m4 a) {
    m4 result{};
    result._00 = a._00 * b._00 + a._01 * b._10 + a._02 * b._20 + a._03 * b._30; 
    result._01 = a._00 * b._01 + a._01 * b._11 + a._02 * b._21 + a._03 * b._31; 
    result._02 = a._00 * b._02 + a._01 * b._12 + a._02 * b._22 + a._03 * b._32; 
    result._03 = a._00 * b._03 + a._01 * b._13 + a._02 * b._23 + a._03 * b._33; 
    result._10 = a._10 * b._00 + a._11 * b._10 + a._12 * b._20 + a._13 * b._30; 
    result._11 = a._10 * b._01 + a._11 * b._11 + a._12 * b._21 + a._13 * b._31; 
    result._12 = a._10 * b._02 + a._11 * b._12 + a._12 * b._22 + a._13 * b._32; 
    result._13 = a._10 * b._03 + a._11 * b._13 + a._12 * b._23 + a._13 * b._33; 
    result._20 = a._20 * b._00 + a._21 * b._10 + a._22 * b._20 + a._23 * b._30; 
    result._21 = a._20 * b._01 + a._21 * b._11 + a._22 * b._21 + a._23 * b._31; 
    result._22 = a._20 * b._02 + a._21 * b._12 + a._22 * b._22 + a._23 * b._32; 
    result._23 = a._20 * b._03 + a._21 * b._13 + a._22 * b._23 + a._23 * b._33; 
    result._30 = a._30 * b._00 + a._31 * b._10 + a._32 * b._20 + a._33 * b._30; 
    result._31 = a._30 * b._01 + a._31 * b._11 + a._32 * b._21 + a._33 * b._31; 
    result._32 = a._30 * b._02 + a._31 * b._12 + a._32 * b._22 + a._33 * b._32; 
    result._33 = a._30 * b._03 + a._31 * b._13 + a._32 * b._23 + a._33 * b._33; 
    return result;
}

internal v4 mul_m4_v4(m4 m, v4 v) {
    v4 result;
    result.e[0] = v.e[0] * m._00 + v.e[1] * m._01 + v.e[2] * m._02 + v.e[3] * m._03;
    result.e[1] = v.e[0] * m._10 + v.e[1] * m._11 + v.e[2] * m._12 + v.e[3] * m._13;
    result.e[2] = v.e[0] * m._20 + v.e[1] * m._21 + v.e[2] * m._22 + v.e[3] * m._23;
    result.e[3] = v.e[0] * m._30 + v.e[1] * m._31 + v.e[2] * m._32 + v.e[3] * m._33;
    return result;
}

internal m4 translate_m4(v3 t) {
    m4 result = identity_m4();
    result._30 = t.x;
    result._31 = t.y;
    result._32 = t.z;
    return result;
}

internal m4 inv_translate_m4(v3 t) {
    t = negate_v3(t);
    m4 result = translate_m4(t);
    return result;
}

internal m4 scale_m4(v3 s) {
    m4 result{};
    result._00 = s.x;
    result._11 = s.y;
    result._22 = s.z;
    result._33 = 1.0f;
    return result;
}

internal m4 rotate_m4(v3 axis, f32 theta) {
    m4 result = identity_m4();
    axis = normalize_v3(axis);

    f32 sin_t = sinf(theta);
    f32 cos_t = cosf(theta);
    f32 cos_inv = 1.0f - cos_t;

    result._00 = (cos_inv * axis.x * axis.x) + cos_t;
    result._01 = (cos_inv * axis.x * axis.y) + (axis.z * sin_t);
    result._02 = (cos_inv * axis.x * axis.z) - (axis.y * sin_t);

    result._10 = (cos_inv * axis.y * axis.x) - (axis.z * sin_t);
    result._11 = (cos_inv * axis.y * axis.y) - cos_t;
    result._12 = (cos_inv * axis.y * axis.z) + (axis.x * sin_t);

    result._20 = (cos_inv * axis.z * axis.x) + (axis.y * sin_t);
    result._21 = (cos_inv * axis.z * axis.y) - (axis.x * sin_t);
    result._22 = (cos_inv * axis.z * axis.z) + cos_t;

    return result; 
}

internal m4 ortho_rh_zo(f32 left, f32 right, f32 bottom, f32 top, f32 _near, f32 _far) {
    m4 result = identity_m4();
    result._00 = 2.0f / (right - left);
    result._11 = 2.0f / (top - bottom);
    result._22 = 1.0f / (_far - _near);
    result._30 = - (right + left) / (right - left);
    result._31 = - (top + bottom) / (top - bottom);
    // result._32 = - near / (far - near);
    result._32 = -(_near + _far) / (_far - _near);
    return result;
}

internal m4 perspective_projection_rh(f32 fov, f32 aspect_ratio, f32 _near, f32 _far) {
    m4 result = identity_m4();
    f32 c = 1.0f / tanf(fov / 2.0f);
    result._00 = c / aspect_ratio;
    result._11 = c;
    result._23 = -1.0f;

    result._22 = (_far) / (_near - _far);
    result._32 = (_near * _far) / (_near - _far);
    return result;
}

internal m4 look_at_rh(v3 eye, v3 target, v3 up) {
    v3 F = normalize_v3(sub_v3(target, eye));
    v3 R = normalize_v3(cross_v3(F, up));
    v3 U = cross_v3(R, F);

    m4 result;
    result._00 = R.x;
    result._01 = U.x;
    result._02 = -F.x;
    result._03 = 0.f;

    result._10 = R.y;
    result._11 = U.y;
    result._12 = -F.y;
    result._13 = 0.f;

    result._20 = R.z;
    result._21 = U.z;
    result._22 = -F.z;
    result._23 = 0.f;

    result._30 = -dot_v3(R, eye);
    result._31 = -dot_v3(U, eye);
    result._32 =  dot_v3(F, eye);
    result._33 = 1.f;
    return result;
}

internal m4 look_at_lh(v3 eye, v3 target, v3 up) {
    v3 F = normalize_v3(sub_v3(target, eye));
    v3 R = normalize_v3(cross_v3(up, F));
    v3 U = cross_v3(F, R);

    m4 result = identity_m4();
    result._00 = R.x;
    result._10 = R.y;
    result._20 = R.z;
    result._01 = U.x;
    result._11 = U.y;
    result._21 = U.z;
    result._02 = F.x;
    result._12 = F.y;
    result._22 = F.z;
    result._30 = -dot_v3(R, eye);
    result._31 = -dot_v3(U, eye);
    result._32 = -dot_v3(F, eye);
    return result;
}

internal f32 length_v2(v2 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y);
    return result;
}

internal f32 length_v3(v3 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

internal f32 length_v4(v4 v) {
    f32 result;
    result = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    return result;
}

internal f32 length2_v2(v2 v) {
    f32 result;
    result = v.x * v.x + v.y * v.y;
    return result;
}

internal f32 length2_v3(v3 v) {
    f32 result = v.x * v.x + v.y * v.y + v.z * v.z;
    return result;
}

internal f32 length2_v4(v4 v) {
    f32 result = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    return result;
}

internal v2 normalize_v2(v2 v) {
    f32 length = length_v2(v);
    v2 result = div_v2_s(v, length);
    return result;
}

internal v3 normalize_v3(v3 v) {
    f32 length = length_v3(v);
    v3 result = div_v3_s(v, length);
    return result;
}

internal v4 normalize_v4(v4 v) {
    f32 length = length_v4(v);
    v4 result = mul_v4_s(v, length);
    return result;
}

internal f32 dot_v2(v2 a, v2 b) {
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

internal f32 dot_v3(v3 a, v3 b) {
    f32 result;
    result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result; 
}

internal f32 dot_v4(v4 a, v4 b) {
    f32 result;
    result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result; 
}

internal v3 cross_v3(v3 a, v3 b) {
    v3 result;
    result.x = (a.y * b.z) - (a.z * b.y);
    result.y = (a.z * b.x) - (a.x * b.z);
    result.z = (a.x * b.y) - (a.y * b.x);
    return result;
}

internal f32 lerp(f32 a, f32 b, f32 t) {
    f32 result = (1.0f - t) * a + b * t;
    return result;
}

internal v2 lerp_v2(v2 a, v2 b, f32 t) {
    v2 result;
    result = add_v2(mul_v2_s(a, (1.0f - t)), mul_v2_s(b, t));
    return result;
}

internal v3 lerp_v3(v3 a, v3 b, f32 t) {
    v3 result;
    result = add_v3(mul_v3_s(a, (1.0f - t)), mul_v3_s(b, t));
    return result;
}

internal v4 lerp_v4(v4 a, v4 b, f32 t) {
    v4 result;
    result = add_v4(mul_v4_s(a, (1.0f - t)), mul_v4_s(b, t));
    return result;
}

internal v2 add_v2(v2 a, v2 b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

internal v3 add_v3(v3 a, v3 b) {
    v3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result; 
}

internal v4 add_v4(v4 a, v4 b) {
    v4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

internal v2 sub_v2(v2 a, v2 b) {
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

internal v3 sub_v3(v3 a, v3 b) {
    v3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result; 
}

internal v4 sub_v4(v4 a, v4 b) {
    v4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;
    return result;
}

internal v2 negate_v2(v2 v) {
    v2 result;
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

internal v3 negate_v3(v3 v) {
    v3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

internal v4 negate_v4(v4 v) {
    v4 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    result.w = -v.w;
    return result;
}

internal v2 mul_v2_s(v2 v, f32 s) {
    v2 result;
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

internal v3 mul_v3_s(v3 v, f32 s) {
    v3 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

internal v4 mul_v4_s(v4 v, f32 s) {
    v4 result;
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;
    return result;
}

internal v2 div_v2_s(v2 v, f32 s) {
    v2 result;
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

internal v3 div_v3_s(v3 v, f32 s) {
    v3 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    return result;
}

internal v4 div_v4_s(v4 v, f32 s) {
    v4 result;
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    result.w = v.w / s;
    return result;
}

//@Note Function overloads
#ifdef __cplusplus
internal m4 translate(v3 v) {m4 result = translate_m4(v); return result;}
internal m4 scale(v3 v) {m4 result = scale_m4(v); return result;}
internal m4 rotate(v3 v, f32 theta) {m4 result = rotate_m4(v, theta); return result;}
internal m4 inv_translate(v3 v) {m4 result = inv_translate_m4(v); return result;}

internal v4 mul(m4 m, v4 v) {v4 result = mul_m4_v4(m, v); return result;}
internal m4 mul(m4 a, m4 b) {m4 result = mul_m4(a, b); return result;}

internal v2 normalize(v2 v) {v2 result = normalize_v2(v); return result;}
internal v3 normalize(v3 v) {v3 result = normalize_v3(v); return result;}
internal v4 normalize(v4 v) {v4 result = normalize_v4(v); return result;}

internal f32 dot(v2 a, v2 b) {f32 result = dot_v2(a, b); return result;}
internal f32 dot(v3 a, v3 b) {f32 result = dot_v3(a, b); return result;}
internal f32 dot(v4 a, v4 b) {f32 result = dot_v4(a, b); return result;}

internal f32 length(v2 v) {f32 result = length_v2(v); return result;}
internal f32 length(v3 v) {f32 result = length_v3(v); return result;}
internal f32 length(v4 v) {f32 result = length_v4(v); return result;}

internal f32 length2(v2 v) {f32 result = length2_v2(v); return result;}
internal f32 length2(v3 v) {f32 result = length2_v3(v); return result;}
internal f32 length2(v4 v) {f32 result = length2_v4(v); return result;}

internal v2 lerp(v2 a, v2 b, f32 t) {v2 result = lerp_v2(a, b, t); return result;}
internal v3 lerp(v3 a, v3 b, f32 t) {v3 result = lerp_v3(a, b, t); return result;}
internal v4 lerp(v4 a, v4 b, f32 t) {v4 result = lerp_v4(a, b, t); return result;}

internal v3 cross(v3 a, v3 b) {v3 result = cross_v3(a, b); return result;}
#endif // __cplusplus
