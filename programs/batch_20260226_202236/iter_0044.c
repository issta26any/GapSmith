#include <stdint.h>
#include <math.h>

/* Saturating arithmetic intrinsics simulation */
static int32_t ssdiv(int32_t a, int32_t b) {
    if (b == 0) return a > 0 ? INT32_MAX : (a < 0 ? INT32_MIN : 0);
    int64_t res = (int64_t)a / (int64_t)b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}

static uint32_t usdiv(uint32_t a, uint32_t b) {
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

static double handle_const_second(double val, int mode) {
    double acc = val;
    while (mode-- > 0) {
        if (mode & 1) {
            acc = copysign(acc, 2.0);      /* CONST_DOUBLE second operand */
        } else {
            acc = copysign(acc, -3.0);     /* Negative CONST_DOUBLE */
        }
        if (acc < 0.0) acc = -acc;
    }
    return acc;
}

static float nested_copysign_pattern(float a, float b, float c, int depth) {
    if (depth <= 0) return a;
    float t;
    if (depth % 4 == 0) {
        t = copysignf(-b, c);                     /* First operand is NEG */
    } else if (depth % 4 == 1) {
        t = copysignf(fabsf(b), c);               /* First operand is ABS */
    } else if (depth % 4 == 2) {
        t = copysignf(a, fabsf(c));               /* Second operand is ABS */
    } else {
        t = copysignf(copysignf(a, b), c);        /* Nested copysign first */
    }
    return t + nested_copysign_pattern(a, b, c, depth - 1);
}

static double double_nested_copysign(double x, double y, double z) {
    double r = x;
    for (int i = 0; i < 5; ++i) {
        if (i == 0) {
            r = copysign(r, copysign(y, z));      /* Second operand is COPYSIGN */
        } else if (i == 2) {
            r = copysign(copysign(r, y), z);      /* First operand is COPYSIGN */
        } else {
            r = copysign(r, y);
        }
        if (r > 100.0) r = 100.0;
    }
    return r;
}

int main(void) {
    volatile int32_t sat_signed = 0;
    volatile uint32_t sat_unsigned = 0;
    
    /* Trigger SS_DIV/US_DIV with constant divisor 1 */
    for (int i = -10; i < 10; ++i) {
        if (i != 0) {
            sat_signed += ssdiv(i, 1);            /* divisor = 1 */
            sat_unsigned += usdiv(i * i, 1);      /* divisor = 1 */
        }
    }
    
    float f1 = 3.14f;
    float f2 = -2.5f;
    float f3 = 0.0f;
    
    f3 += process_copysign_ident(f1, 7);
    f3 += handle_const_second(f2, 4);
    f3 += nested_copysign_pattern(f1, f2, -f1, 6);
    f3 += double_nested_copysign(f1, f2, -f2);
    
    /* Additional control flow to prevent dead code elimination */
    double sum = (double)sat_signed + (double)sat_unsigned + (double)f3;
    if (sum > 1000.0) {
        return 1;
    }
    return 0;
}
