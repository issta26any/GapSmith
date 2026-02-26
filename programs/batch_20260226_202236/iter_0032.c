#include <stdint.h>
#include <math.h>

/* Helper functions to generate various patterns */
static float test_copysign_identical(float a) {
    /* COPYSIGN with identical operands */
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    /* COPYSIGN with constant second operand */
    if (x > 0.0) {
        return copysign(x, 2.0);  /* CONST_DOUBLE positive */
    } else {
        return copysign(x, -3.0); /* CONST_DOUBLE negative */
    }
}

static float test_copysign_first_neg(float y, float z) {
    /* COPYSIGN where first operand is NEG */
    return copysignf(-y, z);
}

static double test_copysign_first_abs(double y, double z) {
    /* COPYSIGN where first operand is ABS */
    return copysign(fabs(y), z);
}

static float test_copysign_second_abs(float x, float y) {
    /* COPYSIGN where second operand is ABS */
    return copysignf(x, fabsf(y));
}

static float test_copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    return copysignf(copysignf(a, b), c);
}

static double test_copysign_nested_second(double a, double b, double c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    return copysign(a, copysign(b, c));
}

/* Saturating division simulation */
static int32_t saturating_ssdiv(int32_t x) {
    /* Simulate signed saturating division by 1 */
    if (x == INT32_MIN) {
        return INT32_MIN; /* Saturate case */
    }
    return x / 1; /* Will become SS_DIV by constant 1 */
}

static uint32_t saturating_usdiv(uint32_t x) {
    /* Simulate unsigned saturating division by 1 */
    if (x == 0) {
        return 0; /* Edge case */
    }
    return x / 1; /* Will become US_DIV by constant 1 */
}

/* Complex control flow to exercise multiple paths */
static float control_flow_copysign(float base, int iterations) {
    float result = base;
    for (int i = 0; i < iterations; ++i) {
        if (i % 3 == 0) {
            result = test_copysign_identical(result);
        } else if (i % 3 == 1) {
            result = test_copysign_first_neg(result, base + i);
        } else {
            result = test_copysign_second_abs(result, base - i);
        }
        
        if (result > 100.0f) {
            result = test_copysign_nested_first(result, base, -result);
        }
    }
    return result;
}

static int32_t control_flow_saturating(int32_t seed) {
    int32_t acc = seed;
    for (int i = 0; i < 5; ++i) {
        if (acc > 0) {
            acc = saturating_ssdiv(acc);
        } else {
            acc = saturating_ssdiv(acc + i);
        }
        
        if (i % 2 == 0) {
            uint32_t u = (uint32_t)(acc >= 0 ? acc : -acc);
            u = saturating_usdiv(u);
            acc = (int32_t)u;
        }
    }
    return acc;
}

int main(void) {
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile double d1 = 1.414;
    volatile double d2 = -0.577;
    
    /* Exercise all COPYSIGN patterns */
    float r1 = test_copysign_identical(f1);
    double r2 = test_copysign_const(d1);
    float r3 = test_copysign_first_neg(f1, f2);
    double r4 = test_copysign_first_abs(d1, d2);
    float r5 = test_copysign_second_abs(f1, f2);
    float r6 = test_copysign_nested_first(f1, f2, r1);
    double r7 = test_copysign_nested_second(d1, d2, r2);
    
    /* Complex control flow */
    float r8 = control_flow_copysign(f1, 7);
    
    /* Saturating division patterns */
    int32_t s1 = saturating_ssdiv(100);
    int32_t s2 = saturating_ssdiv(-100);
    uint32_t u1 = saturating_usdiv(200);
    int32_t s3 = control_flow_saturating(42);
    
    /* Use results to prevent elimination */
    volatile float fr = r1 + r3 + r5 + r6 + r8;
    volatile double dr = r2 + r4 + r7;
    volatile int32_t ir = s1 + s2 + s3 + (int32_t)u1;
    
    return (fr > 0.0f && dr < 0.0 && ir != 0) ? 0 : 1;
}
