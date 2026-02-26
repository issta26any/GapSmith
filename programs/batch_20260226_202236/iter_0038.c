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
    // Simulate __ssdiv(x, 1) behavior
    if (x == INT32_MIN && -1 == 1) {
        return INT32_MIN;
    }
    return x / 1;
}

static uint32_t usdiv_by_one(uint32_t x) {
    // Simulate __usdiv(x, 1) behavior
    return x / 1;
}

// Functions to trigger COPYSIGN patterns
static float copysign_identical(float x) {
    // Identical operands
    return copysignf(x, x);
}

static float copysign_const_second(float x) {
    // Second operand constant
    return copysignf(x, 2.0f) + copysignf(x, -3.0f);
}

static float copysign_first_neg(float x, float y) {
    // First operand is NEG
    return copysignf(-y, x);
}

static float copysign_first_abs(float x, float y) {
    // First operand is ABS
    return copysignf(fabsf(y), x);
}

static float copysign_second_abs(float x, float y) {
    // Second operand is ABS (no side effects)
    return copysignf(x, fabsf(y));
}

static float copysign_nested_first(float a, float b, float c) {
    // First operand is another COPYSIGN, its second operand has no side effects
    return copysignf(copysignf(a, b), c);
}

static float copysign_nested_second(float a, float b, float c) {
    // Second operand is another COPYSIGN, its first operand has no side effects
    return copysignf(a, copysignf(b, c));
}

int main(void) {
    volatile int seed = 0;
    int32_t sval = select_value(100, -200, seed);
    uint32_t uval = (uint32_t)select_value(300, 400, seed + 1);
    
    // Trigger saturating division by 1
    int32_t sres = ssdiv_by_one(sval);
    uint32_t ures = usdiv_by_one(uval);
    
    float f1 = process_float(1.25f, 5);
    float f2 = process_float(-2.75f, 3);
    float f3 = process_float(0.0f, 2);
    
    // Trigger all COPYSIGN patterns
    float r1 = copysign_identical(f1);
    float r2 = copysign_const_second(f2);
    float r3 = copysign_first_neg(f1, f2);
    float r4 = copysign_first_abs(f2, f3);
    float r5 = copysign_second_abs(f3, f1);
    float r6 = copysign_nested_first(f1, f2, f3);
    float r7 = copysign_nested_second(f1, f2, f3);
    
    // Use results to avoid dead code elimination
    volatile float sink = r1 + r2 + r3 + r4 + r5 + r6 + r7;
    volatile int32_t vsink = sres;
    volatile uint32_t vusink = ures;
    
    return (sink != 0.0f) ? 0 : 1;
}
