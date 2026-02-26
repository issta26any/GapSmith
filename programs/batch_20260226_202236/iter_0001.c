#include <stdint.h>
#include <math.h>

/* Saturating arithmetic intrinsics simulation */
static int32_t ssdiv_int32(int32_t a, int32_t b) {
    if (b == 0) return a > 0 ? INT32_MAX : (a < 0 ? INT32_MIN : 0);
    int64_t res = (int64_t)a / (int64_t)b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}

static uint32_t usdiv_uint32(uint32_t a, uint32_t b) {
    if (b == 0) return a > 0 ? UINT32_MAX : 0;
    uint64_t res = (uint64_t)a / (uint64_t)b;
    if (res > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)res;
}

/* Helper functions with control flow */
static float process_copysign_ident(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(result, result);  /* Identical operands */
        } else if (i % 3 == 1) {
            result -= __builtin_copysignf(result, result);
        } else {
            result *= 0.5f;
        }
    }
    return result;
}

static double process_copysign_const(double val) {
    double acc = val;
    int counter = 0;
    while (counter < 5) {
        if (counter % 2 == 0) {
            acc = copysign(acc, 2.0);      /* CONST_DOUBLE second operand */
        } else {
            acc = copysign(acc, -3.0);     /* Negative CONST_DOUBLE */
        }
        counter++;
    }
    return acc;
}

static float nested_copysign_pattern(float a, float b, float c) {
    float t = 0.0f;
    if (a > b) {
        t = copysignf(-b, c);               /* First operand is NEG */
    } else if (a < c) {
        t = copysignf(fabsf(b), a);         /* First operand is ABS */
    } else {
        t = copysignf(a, fabsf(c));         /* Second operand is ABS */
    }
    return t;
}

static double double_copysign_chain(double x, double y, double z) {
    double r1 = copysign(copysign(x, y), z);   /* First operand is COPYSIGN, second operand y has no side effects */
    double r2 = copysign(x, copysign(y, z));   /* Second operand is COPYSIGN, first operand y has no side effects */
    if (r1 > r2) {
        for (int i = 0; i < 3; ++i) {
            r1 -= 0.1;
        }
    }
    return r1 + r2;
}

int main(void) {
    volatile int32_t sat_div_signed = ssdiv_int32(100, 1);   /* Should trigger SS_DIV by constant 1 */
    volatile uint32_t sat_div_unsigned = usdiv_uint32(200, 1); /* Should trigger US_DIV by constant 1 */

    float f1 = 3.14f;
    float f2 = -2.5f;
    float f3 = 0.0f;

    f3 = process_copysign_ident(f1, 4);
    double d1 = process_copysign_const(-5.67);
    float f4 = nested_copysign_pattern(f1, f2, f3);
    double d2 = double_copysign_chain(d1, f2, f3);

    /* Use results to avoid dead code elimination */
    volatile float vf = f3 + f4;
    volatile double vd = d1 + d2;
    volatile int32_t vs = sat_div_signed;
    volatile uint32_t vu = sat_div_unsigned;

    return (vf > 0.0f) ? (int)vd : (vs + vu);
}
