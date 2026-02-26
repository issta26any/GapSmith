#include <stdint.h>
#include <math.h>

// Helper functions to create control flow
static int select_value(int a, int b, int cond) {
    if (cond > 0) {
        for (int i = 0; i < 3; ++i) {
            if (i % 2 == cond % 2) {
                a += b;
            } else {
                b -= a;
            }
        }
        return a;
    } else {
        int r = b;
        while (r < 10) {
            r += a;
            if (r > 5) break;
        }
        return r;
    }
}

static float process_float(float base, int iter) {
    float acc = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 4 == 0) {
            acc = acc * 1.5f;
        } else if (i % 4 == 1) {
            acc = -acc;
        } else if (i % 4 == 2) {
            acc = fabsf(acc);
        } else {
            acc = acc / 2.0f;
        }
    }
    return acc;
}

// Saturating division simulation (mimic intrinsic pattern)
static int32_t ssdiv_by_one(int32_t x) {
    // Simulate __ssdiv(x, 1) pattern
    int32_t r;
    if (x == INT32_MIN && -1 == -1) {
        r = INT32_MIN;
    } else {
        r = x / 1;  // Should become SS_DIV with constant 1
    }
    return r;
}

static uint32_t usdiv_by_one(uint32_t x) {
    // Simulate __usdiv(x, 1) pattern
    uint32_t r = x / 1;  // Should become US_DIV with constant 1
    return r;
}

// COPYSIGN cases
static float copysign_identical(float x) {
    // Identical operands
    return copysignf(x, x);
}

static float copysign_const_second(float x) {
    // Second operand constant
    float r1 = copysignf(x, 2.0f);
    float r2 = copysignf(x, -3.0f);
    return r1 + r2;
}

static float copysign_first_neg(float y, float z) {
    // First operand is NEG
    return copysignf(-y, z);
}

static float copysign_first_abs(float y, float z) {
    // First operand is ABS
    return copysignf(fabsf(y), z);
}

static float copysign_second_abs(float x, float y) {
    // Second operand is ABS (no side effects)
    return copysignf(x, fabsf(y));
}

static float copysign_nested_first(float a, float b, float c) {
    // First operand is COPYSIGN, its second operand has no side effects
    return copysignf(copysignf(a, b), c);
}

static float copysign_nested_second(float a, float b, float c) {
    // Second operand is COPYSIGN, its first operand has no side effects
    return copysignf(a, copysignf(b, c));
}

int main(void) {
    volatile int seed = 0;
    int32_t sat_signed = 0;
    uint32_t sat_unsigned = 0;
    float fsum = 0.0f;
    
    for (int outer = 0; outer < 4; ++outer) {
        int cond = select_value(outer, outer * 2, outer);
        seed += cond;
        
        // Saturating division cases
        sat_signed += ssdiv_by_one(seed);
        sat_unsigned += usdiv_by_one((uint32_t)seed);
        
        float f = process_float((float)seed * 0.5f, cond % 5);
        
        // COPYSIGN cases
        fsum += copysign_identical(f);
        fsum += copysign_const_second(f);
        fsum += copysign_first_neg(f, f * 2.0f);
        fsum += copysign_first_abs(f, -f);
        fsum += copysign_second_abs(f, f + 1.0f);
        fsum += copysign_nested_first(f, f * 3.0f, f * 0.5f);
        fsum += copysign_nested_second(f, f * 4.0f, f * 0.25f);
        
        // Additional control flow
        if (seed % 7 == 0) {
            fsum = -fsum;
        } else if (seed % 13 == 0) {
            fsum = fabsf(fsum);
        }
    }
    
    // Final deterministic result
    return (int)(fsum + sat_signed + sat_unsigned) % 100;
}
