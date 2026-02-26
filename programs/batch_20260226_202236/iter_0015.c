#include <stdint.h>
#include <math.h>

// Helper functions to create control flow
static int cond_select(int a, int b, int flag) {
    if (flag > 0) {
        for (int i = 0; i < 3; ++i) {
            if (i % 2 == 0) {
                a += b;
            } else {
                a -= b;
            }
        }
        return a;
    } else {
        int t = b;
        while (t > 0) {
            t /= 2;
            a++;
        }
        return a;
    }
}

static float loop_copysign(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 2 == 0) {
            result = copysignf(result, -base);
        } else {
            result = copysignf(result, base);
        }
    }
    return result;
}

// Saturating division intrinsics simulation
// Using inline assembly to generate SS_DIV/US_DIV RTL for constant divisor 1
#ifdef __ARM_ARCH
static int32_t ssdiv_by_one(int32_t x) {
    int32_t res;
    __asm__ ("ssdiv %0, %1, #1" : "=r"(res) : "r"(x));
    return res;
}

static uint32_t usdiv_by_one(uint32_t x) {
    uint32_t res;
    __asm__ ("usdiv %0, %1, #1" : "=r"(res) : "r"(x));
    return res;
}
#else
// Fallback for non-ARM: use regular division (won't generate saturating RTL)
static int32_t ssdiv_by_one(int32_t x) { return x / 1; }
static uint32_t usdiv_by_one(uint32_t x) { return x / 1; }
#endif

// Test COPYSIGN patterns
static float test_copysign_identical(float x) {
    // Identical operands
    return copysignf(x, x);
}

static float test_copysign_const_second(float x) {
    // Second operand constant
    float r1 = copysignf(x, 2.0f);
    float r2 = copysignf(x, -3.0f);
    return r1 + r2;
}

static float test_copysign_first_neg_abs(float y, float z) {
    // First operand is NEG or ABS
    float r1 = copysignf(-y, z);
    float r2 = copysignf(fabsf(y), z);
    return r1 + r2;
}

static float test_copysign_second_abs(float x, float y) {
    // Second operand is ABS
    return copysignf(x, fabsf(y));
}

static float test_copysign_nested_first(float a, float b, float c) {
    // First operand is COPYSIGN, its second operand has no side effects
    return copysignf(copysignf(a, b), c);
}

static float test_copysign_nested_second(float a, float b, float c) {
    // Second operand is COPYSIGN, its first operand has no side effects
    return copysignf(a, copysignf(b, c));
}

int main(void) {
    volatile int flag = 1;
    int32_t sval = cond_select(100, -50, flag);
    uint32_t uval = cond_select(200, 30, flag - 1);

    // Saturating division by 1
    int32_t sres = ssdiv_by_one(sval);
    uint32_t ures = usdiv_by_one(uval);

    float fbase = 5.0f;
    float fres = loop_copysign(fbase, 4);

    float x = 7.0f, y = -8.0f, z = 9.0f;
    float a = 1.5f, b = -2.5f, c = 3.5f;

    float r1 = test_copysign_identical(x);
    float r2 = test_copysign_const_second(y);
    float r3 = test_copysign_first_neg_abs(y, z);
    float r4 = test_copysign_second_abs(x, y);
    float r5 = test_copysign_nested_first(a, b, c);
    float r6 = test_copysign_nested_second(a, b, c);

    // Use results to avoid dead code elimination
    float sum = r1 + r2 + r3 + r4 + r5 + r6 + fres + sres + ures;
    return (sum > 0.0f) ? 0 : 1;
}
