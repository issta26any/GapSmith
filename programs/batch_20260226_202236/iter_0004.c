#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SS_DIV(x, y) __builtin_arm_ssdiv(x, y)
#define US_DIV(x, y) __builtin_arm_usdiv(x, y)
#else
#define SS_DIV(x, y) ((x) / (y))
#define US_DIV(x, y) ((x) / (y))
#endif

static float test_copysign_identical(float a) {
    float r1 = copysignf(a, a);
    float r2 = __builtin_copysignf(a, a);
    return r1 + r2;
}

static double test_copysign_const(double x) {
    double r1 = copysign(x, 2.0);
    double r2 = copysign(x, -3.0);
    double r3 = copysign(x, 0.0);
    return r1 + r2 + r3;
}

static float test_copysign_first_neg_abs(float y, float z) {
    float r1 = copysignf(-y, z);
    float r2 = copysignf(fabsf(y), z);
    float r3 = copysignf(-fabsf(y), z);
    return r1 + r2 + r3;
}

static double test_copysign_second_abs(double x, double y) {
    double r1 = copysign(x, fabs(y));
    double r2 = copysign(x, -fabs(y));
    return r1 + r2;
}

static float test_copysign_nested_first(float a, float b, float c) {
    float inner = copysignf(a, b);
    float r = copysignf(inner, c);
    return r;
}

static double test_copysign_nested_second(double a, double b, double c) {
    double inner = copysign(b, c);
    double r = copysign(a, inner);
    return r;
}

static int32_t test_satdiv_const1(int32_t v) {
    int32_t r1 = SS_DIV(v, 1);
    uint32_t r2 = US_DIV((uint32_t)v, 1);
    return (int32_t)(r1 + r2);
}

static int loop_satdiv(int start, int end) {
    int sum = 0;
    for (int i = start; i < end; ++i) {
        if (i % 3 == 0) {
            sum += test_satdiv_const1(i);
        } else if (i % 3 == 1) {
            sum -= test_satdiv_const1(-i);
        } else {
            sum ^= test_satdiv_const1(i * 2);
        }
    }
    return sum;
}

static float loop_copysign(float base, int count) {
    float acc = base;
    for (int i = 0; i < count; ++i) {
        if (i % 4 == 0) {
            acc += test_copysign_identical(acc);
        } else if (i % 4 == 1) {
            acc += test_copysign_const(acc);
        } else if (i % 4 == 2) {
            acc += test_copysign_first_neg_abs(acc, base);
        } else {
            acc += test_copysign_second_abs(acc, base);
        }
    }
    return acc;
}

int main(void) {
    volatile int32_t sat_sum = 0;
    volatile float cs_sum = 0.0f;

    for (int outer = 0; outer < 5; ++outer) {
        if (outer % 2 == 0) {
            sat_sum += loop_satdiv(outer * 10, outer * 10 + 8);
        } else {
            cs_sum += loop_copysign(outer * 1.5f, 6);
        }

        float a = outer * 0.7f;
        float b = outer * 1.3f;
        float c = outer * 2.1f;
        cs_sum += test_copysign_nested_first(a, b, c);
        cs_sum += test_copysign_nested_second(a, b, c);
    }

    return (int)(sat_sum + cs_sum);
}
