#include <stdint.h>
#include <math.h>

/* Helper functions to create control flow complexity */
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
            r += (a > 0) ? 1 : -1;
        }
        return r;
    }
}

static float process_float(float x, int iter) {
    float acc = x;
    for (int i = 0; i < iter; ++i) {
        if (i % 4 == 0) {
            acc = acc * 0.5f;
        } else if (i % 4 == 1) {
            acc = -acc;
        } else if (i % 4 == 2) {
            acc = fabsf(acc);
        } else {
            acc = acc + 1.0f;
        }
    }
    return acc;
}

/* Simulate saturating division intrinsics via builtins if available */
#ifdef __ARM_FEATURE_SAT
    #define SAT_DIV_1(x) __ssdiv(x, 1)
    #define USAT_DIV_1(x) __usdiv(x, 1)
#else
    /* Portable emulation for testing */
    static int sat_div_1(int x) {
        if (x == INT_MIN) return INT_MIN;
        return x / 1;
    }
    static unsigned usat_div_1(unsigned x) {
        return x / 1;
    }
    #define SAT_DIV_1(x) sat_div_1(x)
    #define USAT_DIV_1(x) usat_div_1(x)
#endif

/* Functions to generate the required COPYSIGN patterns */
static float copysign_identical(float a) {
    /* COPYSIGN with identical operands */
    return copysignf(a, a);
}

static float copysign_const_second(float a) {
    /* COPYSIGN with constant second operand */
    return copysignf(a, 2.0f) + copysignf(a, -3.0f);
}

static float copysign_first_neg_abs(float a, float b) {
    /* COPYSIGN where first operand is NEG or ABS */
    float t1 = copysignf(-a, b);
    float t2 = copysignf(fabsf(a), b);
    return t1 + t2;
}

static float copysign_second_abs(float a, float b) {
    /* COPYSIGN where second operand is ABS */
    return copysignf(a, fabsf(b));
}

static float copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    return copysignf(copysignf(a, b), c);
}

static float copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    return copysignf(a, copysignf(b, c));
}

int main(void) {
    volatile int seed = 0; /* prevent constant folding */
    int x = select_value(seed, 5, seed);
    unsigned u = (unsigned)select_value(seed, 7, -seed);

    /* Trigger SS_DIV/US_DIV by constant 1 */
    int sat_res = SAT_DIV_1(x);
    unsigned usat_res = USAT_DIV_1(u);

    float f1 = process_float(seed * 1.5f, 3);
    float f2 = process_float(seed * -2.0f, 2);
    float f3 = process_float(seed * 0.75f, 4);

    /* Execute all COPYSIGN variants */
    float r1 = copysign_identical(f1);
    float r2 = copysign_const_second(f2);
    float r3 = copysign_first_neg_abs(f1, f2);
    float r4 = copysign_second_abs(f2, f3);
    float r5 = copysign_nested_first(f1, f2, f3);
    float r6 = copysign_nested_second(f1, f2, f3);

    /* Use results to affect control flow */
    int cond = (int)(r1 + r2 + r3 + r4 + r5 + r6) + sat_res + (int)usat_res;
    int final = select_value(cond, seed, cond % 3);

    return final != 0 ? 0 : 1;
}
