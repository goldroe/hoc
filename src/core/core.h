#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Abs(x) ((x) >= 0 ? (x) : -(x))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Clamp(v, min, max) ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))
#define ClampBot(v, bottom) (Max(v, bottom))
#define ClampTop(v, top) (Min(v, top))

#define KB(n) (1024 * (n))
#define MB(n) (1024 * (KB(n)))
#define GB(n) (1024 * (MB(n)))

#define IsPow2(x)          ((x)!=0 && ((x)&((x)-1))==0)
#define AlignForward(x, a) ((x)+(a)-((x)&((a)-1)))

#ifdef __cplusplus
#define EnumDefineFlagOperators(ENUMTYPE)       \
extern "C++" { \
inline ENUMTYPE operator  | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline ENUMTYPE &operator |=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline ENUMTYPE operator  & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline ENUMTYPE &operator &=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline ENUMTYPE operator  ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline ENUMTYPE operator  ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline ENUMTYPE &operator ^=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}
#else
#define EnumDefineFlagOperators(ENUMTYPE) // NOP, C allows these operators.
#endif 

#define Introspect
#define ArrayCount(array) (sizeof((array)) / sizeof((array)[0]))
#define DeferLoop(begin, end) for(int _i_ = ((begin), 0); !_i_; _i_ += 1, (end))

#define MemoryCopy(dest, src, bytes) (memmove(dest, src, bytes)) 
#define MemoryZero(dest, bytes)      (memset(dest, 0, bytes))

#ifdef _WIN32
#define DebugTrap() __debugbreak()
#elif __linux__
#define DebugTrap() __builtin_trap()
#endif

void AssertMessage(const char *message, const char *file, int line) {
    printf("Assert failed: %s, file %s, line %d\n", message, file, line);
}

#define Assert(cond) if (!(cond)) { \
    AssertMessage(#cond, __FILE__, __LINE__); \
    DebugTrap(); \
} \

#define internal static
#define global static
#define local_persist static

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s8  b8;
typedef s16 b16;
typedef s32 b32;
typedef s64 b64;
typedef float  f32;
typedef double f64;
typedef uintptr_t uintptr;

enum Axis2 {
    AXIS_X,
    AXIS_Y,
    AXIS_COUNT
};

union v2i {
    struct {
        int x;
        int y;
    };
    int e[2];

    int& operator[](int index) {
        return e[index];
    }
};

union v2 {
    f32 e[2];
    struct {
        f32 x, y;
    };

    f32& operator[](int index) {
        return e[index];
    }
};

union v3 {
    struct {
        f32 x, y, z;
    };
    f32 e[3];

    f32& operator[](int index) {
        return e[index];
    }
};

union v4 {
    struct {
        f32 x, y, z, w;
    };
    f32 e[4];

    f32& operator[](int index) {
        return e[index];
    }
};

union m4 {
    struct {
        f32 _00, _01, _02, _03;
        f32 _10, _11, _12, _13;
        f32 _20, _21, _22, _23;
        f32 _30, _31, _32, _33;
    };
    f32 e[16];
    v4 columns[4];
};

union Rect {
    struct {
        f32 x0, y0, x1, y1;
    };
    struct {
        v2 p0;
        v2 p1;
    };
};
