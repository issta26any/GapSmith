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
        return copysign(x, 2.5);  /* CONST_DOUBLE positive */
    } else {
        return copysign(x, -3.75); /* CONST_DOUBLE negative */
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

/* Saturating division simulation (emulate intrinsics) */
static int32_t ssdiv_by_one(int32_t x) {
    /* Should generate SS_DIV with constant 1 */
    int32_t result;
    /* Simulate saturating division by 1: x/1 = x, but saturation may apply */
    if (x == INT32_MIN && -1 == -1) {
        /* Special case handled by saturation logic */
        result = INT32_MIN;
    } else {
        result = x; /* Division by 1 */
    }
    return result;
}

static uint32_t usdiv_by_one(uint32_t x) {
    /* Should generate US_DIV with constant 1 */
    uint32_t result;
    /* Simulate unsigned saturating division by 1 */
    result = x; /* Division by 1 */
    return result;
}

/* Complex control flow to explore paths */
static float control_flow_copysign(float base, int iterations) {
    float acc = base;
    for (int i = 0; i < iterations; ++i) {
        if (i % 3 == 0) {
            acc = test_copysign_identical(acc);
        } else if (i % 3 == 1) {
            acc = test_copysign_first_neg(acc, base + i);
        } else {
            acc = test_copysign_second_abs(acc, base - i);
        }
        
        if (acc > 100.0f) {
            acc = test_copysign_first_abs((double)acc, (double)(base - i));
        }
    }
    return acc;
}

int main(void) {
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile double d1 = 1.414;
    volatile double d2 = -0.577;
    
    /* Test saturating division patterns */
    int32_t sval = -1000;
    uint32_t uval = 5000;
    int32_t sres = ssdiv_by_one(sval);
    uint32_t ures = usdiv_by_one(uval);
    
    /* Test COPYSIGN patterns */
    float r1 = test_copysign_identical(f1);
    double r2 = test_copysign_const(d1);
    float r3 = test_copysign_first_neg(f1, f2);
    double r4 = test_copysign_first_abs(d1, d2);
    float r5 = test_copysign_second_abs(f1, f2);
    float r6 = test_copysign_nested_first(f1, f2, 1.5f);
    double r7 = test_copysign_nested_second(d1, d2, -3.0);
    
    /* Complex control flow */
    float r8 = control_flow_copysign(f1, 10);
    
    /* Use results to prevent elimination */
    volatile float sink = r1 + r3 + r5 + r6 + r8;
    volatile double dsink = r2 + r4 + r7;
    volatile int32_t isink = sres;
    volatile uint32_t usink = ures;
    
    return (sink > 0.0f) ? 0 : 1;
}
