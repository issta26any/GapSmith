#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
    #define __ssdiv(x, y) __builtin_arm_ssdiv(x, y)
    #define __usdiv(x, y) __builtin_arm_usdiv(x, y)
#else
    #define __ssdiv(x, y) ((x) / (y))
    #define __usdiv(x, y) ((x) / (y))
#endif

static float helper_copysign_identical(float a) {
    float res = copysignf(a, a);
    if (a > 0.0f) {
        for (int i = 0; i < 3; ++i) {
            res += copysignf(a, a);
        }
    }
    return res;
}

static double helper_copysign_const(double x) {
    double r1 = copysign(x, 2.0);
    double r2 = copysign(x, -3.0);
    double r3 = copysign(x, 0.0);
    return r1 + r2 + r3;
}

static float helper_copysign_first_neg(float y, float z) {
    float t = -y;
    float r = copysignf(t, z);
    if (z < 0.0f) {
        r += copysignf(-y, z);
    }
    return r;
}

static float helper_copysign_first_abs(float y, float z) {
    float a = fabsf(y);
    float r = copysignf(a, z);
    for (int i = 0; i < 2; ++i) {
        r += copysignf(fabsf(y), z);
    }
    return r;
}

static float helper_copysign_second_abs(float x, float y) {
    float a = fabsf(y);
    float r = copysignf(x, a);
    if (y != 0.0f) {
        r += copysignf(x, fabsf(y));
    }
    return r;
}

static float helper_copysign_nested_first(float a, float b, float c) {
    float inner = copysignf(a, b);
    float outer = copysignf(inner, c);
    if (c > 0.0f) {
        outer += copysignf(copysignf(a, b), c);
    }
    return outer;
}

static float helper_copysign_nested_second(float a, float b, float c) {
    float inner = copysignf(b, c);
    float outer = copysignf(a, inner);
    for (int i = 0; i < 2; ++i) {
        outer += copysignf(a, copysignf(b, c));
    }
    return outer;
}

static int32_t satdiv_signed(int32_t x) {
    int32_t r = __ssdiv(x, 1);
    if (x < 0) {
        for (int i = 0; i < 4; ++i) {
            r += __ssdiv(x, 1);
        }
    }
    return r;
}

static uint32_t satdiv_unsigned(uint32_t x) {
    uint32_t r = __usdiv(x, 1);
    int i = 0;
    while (i < 3) {
        r += __usdiv(x, 1);
        ++i;
    }
    return r;
}

int main() {
    volatile float f1 = 5.0f, f2 = -2.0f, f3 = 0.0f;
    volatile double d1 = 7.0, d2 = -4.0;
    volatile int32_t i1 = 100, i2 = -200;
    volatile uint32_t u1 = 300;

    float r1 = helper_copysign_identical(f1);
    float r2 = helper_copysign_identical(f2);
    double r3 = helper_copysign_const(d1);
    float r4 = helper_copysign_first_neg(f1, f2);
    float r5 = helper_copysign_first_abs(f2, f1);
    float r6 = helper_copysign_second_abs(f1, f2);
    float r7 = helper_copysign_nested_first(f1, f2, f3);
    float r8 = helper_copysign_nested_second(f1, f2, f3);
    int32_t r9 = satdiv_signed(i1);
    int32_t r10 = satdiv_signed(i2);
    uint32_t r11 = satdiv_unsigned(u1);

    volatile float sum = r1 + r2 + r4 + r5 + r6 + r7 + r8;
    volatile int32_t isum = r9 + r10;
    volatile uint32_t usum = r11;

    return (sum != 0.0f) || (isum != 0) || (usum != 0) ? 0 : 1;
}
