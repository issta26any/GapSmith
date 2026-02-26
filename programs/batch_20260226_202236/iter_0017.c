#include <stdint.h>
#include <math.h>

/* Helper functions to generate various patterns */
static float test_copysign_identical(float a) {
    /* COPYSIGN with identical operands */
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    /* COPYSIGN with constant second operand */
    if (x > 0) {
        return copysign(x, 2.0);
    } else {
        return copysign(x, -3.0);
    }
}

static float test_copysign_first_neg(float y, float z) {
    /* COPYSIGN where first operand is NEG */
    float neg_y = -y;
    for (int i = 0; i < 3; ++i) {
        if (z > 0) neg_y += 0.1f;
    }
    return copysignf(neg_y, z);
}

static double test_copysign_first_abs(double y, double z) {
    /* COPYSIGN where first operand is ABS */
    double abs_y = fabs(y);
    if (z < 0) abs_y *= 1.5;
    return copysign(abs_y, z);
}

static float test_copysign_second_abs(float x, float y) {
    /* COPYSIGN where second operand is ABS */
    float abs_y = fabsf(y);
    int i = 0;
    while (i < 2) {
        abs_y += 0.5f;
        ++i;
    }
    return copysignf(x, abs_y);
}

static double test_copysign_nested_first(double a, double b, double c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    double inner = copysign(a, b);
    if (c > 0) {
        inner *= 2.0;
    }
    return copysign(inner, c);
}

static float test_copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    float inner = copysignf(b, c);
    for (int i = 0; i < 4; ++i) {
        if (a < 0) inner += 1.0f;
    }
    return copysignf(a, inner);
}

/* Saturating division helpers (using GCC builtins) */
#ifdef __ARM_FEATURE_SAT
static int32_t test_ssdiv_const1(int32_t x) {
    /* SS_DIV by constant 1 */
    int32_t result = __ssdiv(x, 1);
    if (x > 1000) result += 5;
    return result;
}

static uint32_t test_usdiv_const1(uint32_t x) {
    /* US_DIV by constant 1 */
    uint32_t result = __usdiv(x, 1);
    for (int i = 0; i < 2; ++i) {
        if (x < 500) result += i;
    }
    return result;
}
#else
/* Fallback implementations if builtins not available */
static int32_t test_ssdiv_const1(int32_t x) {
    int32_t result = x; /* division by 1 is identity */
    if (x > 1000) result += 5;
    return result;
}

static uint32_t test_usdiv_const1(uint32_t x) {
    uint32_t result = x;
    for (int i = 0; i < 2; ++i) {
        if (x < 500) result += i;
    }
    return result;
}
#endif

/* Main function with non-trivial control flow */
int main(void) {
    volatile float f1 = 3.14f, f2 = -2.5f, f3 = 7.0f;
    volatile double d1 = -6.28, d2 = 4.0, d3 = -9.5;
    volatile int32_t i1 = 100, i2 = -2000;
    volatile uint32_t u1 = 300, u2 = 600;
    
    float fres = 0.0f;
    double dres = 0.0;
    int32_t ires = 0;
    uint32_t ures = 0;
    
    /* Complex conditional structure */
    for (int outer = 0; outer < 3; ++outer) {
        if (outer % 2 == 0) {
            fres += test_copysign_identical(f1 + outer);
            dres += test_copysign_const(d1 * outer);
        } else {
            fres += test_copysign_first_neg(f2, f3);
            dres += test_copysign_first_abs(d2, d3);
        }
        
        switch (outer) {
            case 0:
                fres += test_copysign_second_abs(f1, f2);
                break;
            case 1:
                dres += test_copysign_nested_first(d1, d2, d3);
                break;
            case 2:
                fres += test_copysign_nested_second(f1, f2, f3);
                break;
        }
        
        /* Nested loop */
        for (int inner = 0; inner < 2; ++inner) {
            if (inner == 0) {
                ires += test_ssdiv_const1(i1 + outer * 10);
            } else {
                ures += test_usdiv_const1(u1 + outer * 20);
            }
        }
    }
    
    /* Final conditional to use all results */
    if (fres > 0 && dres < 0 && ires != 0 && ures > 0) {
        return 0;
    } else {
        return 1;
    }
}
