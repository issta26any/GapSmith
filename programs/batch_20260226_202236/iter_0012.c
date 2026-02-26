#include <stdint.h>
#include <math.h>

// Helper functions to generate various patterns
static float test_copysign_identical(float a) {
    // COPYSIGN with identical operands
    float r1 = copysignf(a, a);
    // Use in control flow
    if (a > 0) {
        return r1 * 2.0f;
    }
    return r1;
}

static double test_copysign_const(double x) {
    // COPYSIGN with constant second operand
    double r1 = copysign(x, 2.0);
    double r2 = copysign(x, -3.0);
    // Nested conditionals
    for (int i = 0; i < 3; ++i) {
        if (x < 0) {
            r1 += copysign(x, 5.0);
        }
    }
    return r1 + r2;
}

static float test_copysign_first_neg_abs(float y, float z) {
    // COPYSIGN where first operand is NEG or ABS
    float r1 = copysignf(-y, z);
    float r2 = copysignf(fabsf(y), z);
    // Loop with multiple paths
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (z > 0) {
            sum += r1;
        } else {
            sum += r2;
        }
    }
    return sum;
}

static double test_copysign_second_abs(double x, double y) {
    // COPYSIGN where second operand is ABS
    double r = copysign(x, fabs(y));
    // Conditional call
    if (x != y) {
        return r * 2.0;
    }
    return r;
}

static float test_copysign_nested_first(float a, float b, float c) {
    // COPYSIGN where first operand is another COPYSIGN
    // b has no side effects
    float inner = copysignf(a, b);
    float outer = copysignf(inner, c);
    // Use in loop
    for (int i = 0; i < 2; ++i) {
        if (c < 0) {
            outer += 1.0f;
        }
    }
    return outer;
}

static double test_copysign_nested_second(double a, double b, double c) {
    // COPYSIGN where second operand is another COPYSIGN
    // b has no side effects
    double inner = copysign(b, c);
    double outer = copysign(a, inner);
    // Nested conditional
    if (a > 0) {
        if (b < 0) {
            return outer * 3.0;
        }
    }
    return outer;
}

// Saturating division helpers (simulate with inline asm for x86/ARM-like behavior)
static int32_t ssdiv_int32(int32_t x, int32_t y) {
    // Simulate signed saturating division intrinsic
    if (y == 0) return (x >= 0) ? INT32_MAX : INT32_MIN;
    int64_t tmp = (int64_t)x / (int64_t)y;
    if (tmp > INT32_MAX) return INT32_MAX;
    if (tmp < INT32_MIN) return INT32_MIN;
    return (int32_t)tmp;
}

static uint32_t usdiv_uint32(uint32_t x, uint32_t y) {
    // Simulate unsigned saturating division intrinsic
    if (y == 0) return UINT32_MAX;
    uint64_t tmp = (uint64_t)x / (uint64_t)y;
    if (tmp > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)tmp;
}

static int test_satdiv_const1(int32_t v1, uint32_t v2) {
    // Signed saturating division by constant 1
    int32_t r1 = ssdiv_int32(v1, 1);
    // Unsigned saturating division by constant 1
    uint32_t r2 = usdiv_uint32(v2, 1);
    // Complex control flow
    int sum = 0;
    for (int i = 0; i < 5; ++i) {
        if (r1 > 0) {
            sum += r2;
        } else {
            sum -= r1;
        }
    }
    return sum;
}

int main() {
    volatile float fa = 3.14f;
    volatile float fb = -2.5f;
    volatile float fc = 0.0f;
    volatile double dx = 1.618;
    volatile double dy = -0.707;
    volatile double dz = 9.8;
    
    // Force compiler to generate RTL for all patterns
    float res1 = test_copysign_identical(fa);
    double res2 = test_copysign_const(dx);
    float res3 = test_copysign_first_neg_abs(fb, fc);
    double res4 = test_copysign_second_abs(dx, dy);
    float res5 = test_copysign_nested_first(fa, fb, fc);
    double res6 = test_copysign_nested_second(dx, dy, dz);
    
    volatile int32_t sv = 1000;
    volatile uint32_t uv = 2000;
    int res7 = test_satdiv_const1(sv, uv);
    
    // Use results to prevent elimination
    return (int)(res1 + res2 + res3 + res4 + res5 + res6) + res7;
}
