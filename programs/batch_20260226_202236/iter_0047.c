#include <stdint.h>
#include <math.h>

/* Helper functions to generate various patterns */
static float test_copysign_identical(float a) {
    if (a > 0.0f) {
        return copysignf(a, a);  /* Identical operands */
    }
    return 0.0f;
}

static double test_copysign_const(double x) {
    for (int i = 0; i < 3; ++i) {
        if (x < 0.0) {
            x = copysign(x, 2.0);  /* Second operand constant positive */
        } else {
            x = copysign(x, -3.0); /* Second operand constant negative */
        }
    }
    return x;
}

static float test_copysign_first_neg(float y, float z) {
    float result = 0.0f;
    if (y != 0.0f) {
        result = copysignf(-y, z);  /* First operand is NEG */
    }
    return result;
}

static float test_copysign_first_abs(float y, float z) {
    float temp = y;
    while (temp > -10.0f && temp < 10.0f) {
        temp = copysignf(fabsf(temp), z);  /* First operand is ABS */
        z += 1.0f;
    }
    return temp;
}

static double test_copysign_second_abs(double x, double y) {
    if (y != 0.0) {
        return copysign(x, fabs(y));  /* Second operand is ABS */
    }
    return x;
}

static float test_copysign_nested_first(float a, float b, float c) {
    float res = a;
    for (int i = 0; i < 2; ++i) {
        res = copysignf(copysignf(res, b), c);  /* First operand is COPYSIGN */
    }
    return res;
}

static double test_copysign_nested_second(double a, double b, double c) {
    double val = a;
    if (b > 0.0) {
        val = copysign(val, copysign(b, c));  /* Second operand is COPYSIGN */
    }
    return val;
}

/* Saturating division simulation */
static int32_t ssdiv_by_one(int32_t x) {
    /* Simulate signed saturating division by 1 */
    int32_t result;
    if (x == INT32_MIN) {
        result = INT32_MIN;  /* Saturate case */
    } else {
        result = x / 1;      /* Division by 1 */
    }
    return result;
}

static uint32_t usdiv_by_one(uint32_t x) {
    /* Simulate unsigned saturating division by 1 */
    return x / 1;  /* Always x */
}

int main(void) {
    volatile float f1 = 5.0f, f2 = -3.0f, f3 = 7.0f;
    volatile double d1 = 9.0, d2 = -4.0, d3 = 12.0;
    volatile int32_t i1 = 100, i2 = -200, i3 = INT32_MIN;
    volatile uint32_t u1 = 300, u2 = 0, u3 = UINT32_MAX;

    /* Call all helpers to ensure code generation */
    float r1 = test_copysign_identical(f1);
    double r2 = test_copysign_const(d1);
    float r3 = test_copysign_first_neg(f2, f3);
    float r4 = test_copysign_first_abs(f1, f2);
    double r5 = test_copysign_second_abs(d1, d2);
    float r6 = test_copysign_nested_first(f1, f2, f3);
    double r7 = test_copysign_nested_second(d1, d2, d3);

    int32_t r8 = ssdiv_by_one(i1);
    int32_t r9 = ssdiv_by_one(i2);
    int32_t r10 = ssdiv_by_one(i3);
    uint32_t r11 = usdiv_by_one(u1);
    uint32_t r12 = usdiv_by_one(u2);
    uint32_t r13 = usdiv_by_one(u3);

    /* Use results to avoid dead code elimination */
    volatile float sinkf = r1 + r3 + r4 + r6;
    volatile double sinkd = r2 + r5 + r7;
    volatile int32_t sinki = r8 + r9 + r10;
    volatile uint32_t sinku = r11 + r12 + r13;

    return (sinkf > 0.0f) || (sinkd > 0.0) || (sinki != 0) || (sinku != 0);
}
