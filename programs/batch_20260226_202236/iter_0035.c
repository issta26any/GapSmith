#include <math.h>
#include <stdint.h>

/* Helper functions to generate various copysign patterns */
float copysign_identical(float x) {
    /* COPYSIGN with identical operands */
    return copysignf(x, x);
}

float copysign_const_second(float x) {
    /* COPYSIGN with constant second operand */
    if (x > 0) {
        return copysignf(x, 2.0f);
    } else {
        return copysignf(x, -3.0f);
    }
}

float copysign_first_neg(float y, float z) {
    /* COPYSIGN where first operand is NEG */
    for (int i = 0; i < 3; ++i) {
        if (z > 0) break;
        z += 0.5f;
    }
    return copysignf(-y, z);
}

float copysign_first_abs(float y, float z) {
    /* COPYSIGN where first operand is ABS */
    float t = fabsf(y);
    if (t < 10.0f) {
        return copysignf(t, z);
    }
    return 0.0f;
}

float copysign_second_abs(float x, float y) {
    /* COPYSIGN where second operand is ABS */
    float a = fabsf(y);
    return copysignf(x, a);
}

float copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    float inner = copysignf(a, b);
    return copysignf(inner, c);
}

float copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    float inner = copysignf(b, c);
    return copysignf(a, inner);
}

/* Saturating division helpers (simulate with inline asm for common targets) */
#ifdef __ARM_FEATURE_SAT
int32_t ssdiv_by_one(int32_t x) {
    /* SS_DIV by constant 1 */
    int32_t result;
    __asm__ ("ssat %0, #32, %1" : "=r" (result) : "r" (x));
    return result;
}

uint32_t usdiv_by_one(uint32_t x) {
    /* US_DIV by constant 1 */
    uint32_t result;
    __asm__ ("usat %0, #32, %1" : "=r" (result) : "r" (x));
    return result;
}
#else
/* Fallback: use generic saturation simulation */
int32_t ssdiv_by_one(int32_t x) {
    if (x > 0) {
        for (int i = 0; i < 2; ++i) {
            if (x < 100) break;
            x /= 2;
        }
    }
    return x;
}

uint32_t usdiv_by_one(uint32_t x) {
    while (x > 1000) {
        x /= 3;
    }
    return x;
}
#endif

/* Main function with non-trivial control flow */
int main() {
    volatile float f1 = 5.0f;
    volatile float f2 = -7.0f;
    volatile float f3 = 12.0f;
    float res[8];
    int idx = 0;

    /* Loop with nested conditionals */
    for (int i = 0; i < 10; ++i) {
        if (i % 3 == 0) {
            res[idx++] = copysign_identical(f1 + i);
        } else if (i % 3 == 1) {
            res[idx++] = copysign_const_second(f2 - i);
        } else {
            res[idx++] = copysign_first_neg(f3, f1);
        }

        if (i > 5) {
            res[idx++] = copysign_first_abs(f2, f3);
            res[idx++] = copysign_second_abs(f1, f2);
        }

        if (i == 4) {
            res[idx++] = copysign_nested_first(f1, f2, f3);
            res[idx++] = copysign_nested_second(f1, f2, f3);
        }
    }

    /* Saturating division paths */
    volatile int32_t sval = -100;
    volatile uint32_t uval = 200;
    int32_t sres[4];
    uint32_t ures[4];
    int sidx = 0, uidx = 0;

    for (int j = 0; j < 8; ++j) {
        if (j % 2 == 0) {
            sres[sidx++] = ssdiv_by_one(sval + j);
        } else {
            ures[uidx++] = usdiv_by_one(uval + j);
        }

        switch (j % 3) {
            case 0: sval += 50; break;
            case 1: uval *= 2; break;
            default: sval -= 30; break;
        }
    }

    /* Final deterministic result */
    float sum = 0.0f;
    for (int k = 0; k < idx; ++k) sum += res[k];
    int32_t ssum = 0;
    for (int k = 0; k < sidx; ++k) ssum += sres[k];
    uint32_t usum = 0;
    for (int k = 0; k < uidx; ++k) usum += ures[k];

    return ((sum != 0.0f) + (ssum != 0) + (usum != 0)) == 3 ? 0 : 1;
}
