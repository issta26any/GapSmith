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
static float process_copysign_identical(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(result, result); /* Identical operands */
        } else if (i % 3 == 1) {
            result -= __builtin_copysignf(result, result);
        } else {
            result *= 0.5f;
        }
    }
    return result;
}

static double copysign_const_second(double x, int choice) {
    double temp = x;
    switch (choice) {
        case 0: temp = copysign(temp, 2.0);      /* Positive const */
                break;
        case 1: temp = copysign(temp, -3.0);     /* Negative const */
                break;
        case 2: temp = copysign(temp, 0.0);      /* Zero const */
                break;
        default: temp = copysign(temp, -0.0);    /* Negative zero */
    }
    return temp;
}

static float copysign_first_unary(float a, float b, int mode) {
    float res;
    if (mode & 1) {
        res = copysignf(-a, b);          /* First operand is NEG */
    } else {
        res = copysignf(fabsf(a), b);    /* First operand is ABS */
    }
    return res;
}

static double copysign_second_abs(double x, double y) {
    double r = 0.0;
    int i = 0;
    while (i < 2) {
        r += copysign(x, fabs(y + i));   /* Second operand is ABS */
        i++;
    }
    return r;
}

static float nested_copysign_first(float a, float b, float c) {
    float inner = copysignf(a, b);       /* b has no side effects */
    return copysignf(inner, c);          /* First operand is COPYSIGN */
}

static double nested_copysign_second(double p, double q, double r) {
    double inner = copysign(q, r);
    return copysign(p, inner);           /* Second operand is COPYSIGN */
}

/* Main with non‑trivial flow */
int main(void) {
    volatile int32_t sat_div_one;
    volatile uint32_t usat_div_one;
    float f1 = 3.14f, f2 = -2.5f, f3 = 7.0f;
    double d1 = -1.5e-10, d2 = 9.876e5;
    int counter = 0;

    /* Trigger SS_DIV / US_DIV by constant 1 */
    for (int i = -10; i < 10; ++i) {
        if (i != 0) {
            sat_div_one = ssdiv_int32(i, 1);     /* divisor = 1 */
            usat_div_one = usdiv_uint32(i * i, 1);
        }
        counter += i;
    }

    /* COPYSIGN patterns */
    f1 = process_copysign_identical(f1, 5);
    d1 = copysign_const_second(d1, counter % 4);

    if (counter > 0) {
        f2 = copysign_first_unary(f2, f3, counter);
        d2 = copysign_second_abs(d1, d2);
    } else {
        f3 = nested_copysign_first(f1, f2, f3);
        d1 = nested_copysign_second(d1, d2, -d1);
    }

    /* Loop with nested conditionals */
    float arr[4] = {1.f, -1.f, 0.f, -0.f};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) {
                arr[i] = copysignf(arr[i], arr[j]);
                if (i > j) {
                    arr[i] = copysignf(-arr[i], fabsf(arr[j]));
                }
            }
        }
    }

    /* Deterministic result */
    return (int)(f1 + f2 + f3 + d1 + d2 + arr[0]) % 256;
}
