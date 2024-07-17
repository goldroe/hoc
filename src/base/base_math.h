#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <math.h>
#ifndef PI
    #define PI    3.14159265358979323846f
#endif
#ifndef PI2
    #define PI2   6.28318530717958647693f
#endif
#ifndef PIDIV2
    #define PIDIV2 1.5707963267948966192f
#endif
#ifndef EPSILON
    #define EPSILON 0.000001f
#endif

#define DegToRad(deg) (deg*PI/180.f)
#define RadToDeg(rad) (rad*180.f/PI)

inline v2 V2() { v2 result = {0, 0}; return result; }
inline v3 V3() { v3 result = {0, 0, 0}; return result; }
inline v4 V4() { v4 result = {0, 0, 0, 0}; return result; }

inline v2 V2(f32 x, f32 y) { v2 result = {x, y}; return result; }
inline v3 V3(f32 x, f32 y, f32 z) { v3 result = {x, y, z}; return result; }
inline v4 V4(f32 x, f32 y, f32 z, f32 w) { v4 result = {x, y, z, w}; return result; }

internal m4 identity_m4();
internal m4 mul_m4(m4 b, m4 a);
internal v4 mul_m4_v4(m4 m, v4 v);
internal m4 transpose_m4(m4 m);
internal m4 translate_m4(v3 t);
internal m4 inv_translate_m4(v3 t);
internal m4 scale_m4(v3 s);
internal m4 rotate_m4(v3 axis, f32 theta);
internal m4 ortho_rh_zo(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
internal m4 perspective_projection_rh(f32 fov, f32 aspect_ratio, f32 near, f32 far);
internal m4 look_at_rh(v3 eye, v3 target, v3 up);
internal m4 look_at_lh(v3 eye, v3 target, v3 up);

internal v2 add_v2(v2 a, v2 b);
internal v3 add_v3(v3 a, v3 b);
internal v4 add_v4(v4 a, v4 b);
internal v2 sub_v2(v2 a, v2 b);
internal v3 sub_v3(v3 a, v3 b);
internal v4 sub_v4(v4 a, v4 b);
internal v2 negate_v2(v2 v);
internal v3 negate_v3(v3 v);
internal v4 negate_v4(v4 v);

internal v2 mul_v2_s(v2 v, f32 s);
internal v3 mul_v3_s(v3 v, f32 s);
internal v4 mul_v4_s(v4 v, f32 s);
internal v2 div_v2_s(v2 v, f32 s);
internal v3 div_v3_s(v3 v, f32 s);
internal v4 div_v4_s(v4 v, f32 s);

internal f32 length_v2(v2 v);
internal f32 length_v3(v3 v);
internal f32 length_v4(v4 v);
internal f32 length2_v2(v2 v);
internal f32 length2_v3(v3 v);
internal f32 length2_v4(v4 v);
internal v2 normalize_v2(v2 v);
internal v3 normalize_v3(v3 v);
internal v4 normalize_v4(v4 v);

internal f32 lerp(f32 a, f32 b, f32 t);
internal v2 lerp_v2(v2 a, v2 b, f32 t);
internal v3 lerp_v3(v3 a, v3 b, f32 t);
internal v4 lerp_v4(v4 a, v4 b, f32 t);

internal f32 dot_v2(v2 a, v2 b);
internal f32 dot_v3(v3 a, v3 b);
internal f32 dot_v4(v4 a, v4 b);

internal v3 cross_v3(v3 a, v3 b);

//@Note Function overloads
#ifdef __cplusplus
internal v2 normalize(v2 v);
internal v3 normalize(v3 v);
internal v4 normalize(v4 v);
internal f32 dot(v2 a, v2 b);
internal f32 dot(v3 a, v3 b);
internal f32 dot(v4 a, v4 b);
internal v2 lerp(v2 a, v2 b, f32 t);
internal v3 lerp(v3 a, v3 b, f32 t);
internal v4 lerp(v4 a, v4 b, f32 t);
internal f32 length(v2 v);
internal f32 length(v3 v);
internal f32 length(v4 v);
internal f32 length2(v2 v);
internal f32 length2(v3 v);
internal f32 length2(v4 v);

//@Note Operator overloads
inline v2 operator+(v2 a, v2 b) {v2 result = add_v2(a, b); return result;}
inline v3 operator+(v3 a, v3 b) {v3 result = add_v3(a, b); return result;}
inline v4 operator+(v4 a, v4 b) {v4 result = add_v4(a, b); return result;}

inline v2 operator-(v2 v) {v2 result = negate_v2(v); return result;}
inline v3 operator-(v3 v) {v3 result = negate_v3(v); return result;}
inline v4 operator-(v4 v) {v4 result = negate_v4(v); return result;}

inline v2 operator-(v2 a, v2 b) {v2 result = sub_v2(a, b); return result;}
inline v3 operator-(v3 a, v3 b) {v3 result = sub_v3(a, b); return result;}
inline v4 operator-(v4 a, v4 b) {v4 result = sub_v4(a, b); return result;}

inline v2 operator+=(v2 &a, v2 b) {a = a + b; return a;}
inline v3 operator+=(v3 &a, v3 b) {a = a + b; return a;}
inline v4 operator+=(v4 &a, v4 b) {a = a + b; return a;}

inline v2 operator-=(v2 &a, v2 b) {a = a - b; return a;}
inline v3 operator-=(v3 &a, v3 b) {a = a - b; return a;}
inline v4 operator-=(v4 &a, v4 b) {a = a - b; return a;}

inline v2 operator*(v2 v, f32 s) {v2 result = mul_v2_s(v, s); return result;}
inline v3 operator*(v3 v, f32 s) {v3 result = mul_v3_s(v, s); return result;}
inline v4 operator*(v4 v, f32 s) {v4 result = mul_v4_s(v, s); return result;}

inline v2 operator*(f32 s, v2 v) {v2 result = mul_v2_s(v, s); return result;}
inline v3 operator*(f32 s, v3 v) {v3 result = mul_v3_s(v, s); return result;}
inline v4 operator*(f32 s, v4 v) {v4 result = mul_v4_s(v, s); return result;}

inline v2 operator/(v2 v, f32 s) {v2 result = div_v2_s(v, s); return result;}
inline v3 operator/(v3 v, f32 s) {v3 result = div_v3_s(v, s); return result;}
inline v4 operator/(v4 v, f32 s) {v4 result = div_v4_s(v, s); return result;}

inline v2 operator*=(v2 &v, f32 s) {v = v * s; return v;}
inline v3 operator*=(v3 &v, f32 s) {v = v * s; return v;}
inline v4 operator*=(v4 &v, f32 s) {v = v * s; return v;}

inline v2 operator/=(v2 &v, f32 s) {v = v / s; return v;}
inline v3 operator/=(v3 &v, f32 s) {v = v / s; return v;}
inline v4 operator/=(v4 &v, f32 s) {v = v / s; return v;}

inline bool operator==(v2 a, v2 b) {bool result = a.x == b.x && a.y == b.y; return result;}
inline bool operator==(v3 a, v3 b) {bool result = a.x == b.x && a.y == b.y && a.z == b.z; return result;}
inline bool operator==(v4 a, v4 b) {bool result = a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; return result;}

inline bool operator!=(v2 a, v2 b) {return !(a == b);}
inline bool operator!=(v3 a, v3 b) {return !(a == b);}
inline bool operator!=(v4 a, v4 b) {return !(a == b);}

inline m4 operator*(m4 a, m4 b) {m4 result = mul_m4(a, b); return result;}
#endif // __cplusplus

#endif // BASE_MATH_H
