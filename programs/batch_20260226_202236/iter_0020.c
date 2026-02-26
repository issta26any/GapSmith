#include <math.h>
#include <stdint.h>

/* Helper functions to generate various COPYSIGN patterns */
float copysign_identical(float x) {
    /* COPYSIGN with identical operands */
    return copysignf(x, x);
}

float copysign_const_second(float x) {
    /* COPYSIGN with constant second operand */
    return copysignf(x, 2.5f) + copysignf(x, -3.75f);
}

float copysign_first_neg(float y, float z) {
    /* COPYSIGN where first operand is NEG */
    return copysignf(-y, z);
}

float copysign_first_abs(float y, float z) {
    /* COPYSIGN where first operand is ABS */
    return copysignf(fabsf(y), z);
}

float copysign_second_abs(float x, float y) {
    /* COPYSIGN where second operand is ABS */
    return copysignf(x, fabsf(y));
}

float copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    return copysignf(copysignf(a, b), c);
}

float copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    return copysignf(a, copysignf(b, c));
}

/* Saturating division helpers (using compiler builtins where available) */
#ifdef __ARM_FEATURE_SAT
int32_t sat_div_const1(int32_t x) {
    /* Should generate SS_DIV with constant 1 */
    return __ssdiv(x, 1);
}

uint32_t usat_div_const1(uint32_t x) {
    /* Should generate US_DIV with constant 1 */
    return __usdiv(x, 1);
}
#else
/* Fallback implementations if intrinsics not available */
int32_t sat_div_const1(int32_t x) {
    /* Simple implementation that compiler may still optimize */
    if (x == INT32_MIN) return INT32_MIN;
    return x / 1;
}

uint32_t usat_div_const1(uint32_t x) {
    return x / 1;
}
#endif

/* Complex control flow to exercise multiple paths */
float process_value(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysign_identical(result);
        } else if (i % 3 == 1) {
            result = copysign_first_neg(result, 1.5f);
        } else {
            result = copysign_second_abs(result, -result);
        }
        
        if (result > 100.0f) {
            result = copysign_const_second(result);
        } else if (result < -50.0f) {
            result = copysign_first_abs(result, 4.0f);
        }
    }
    return result;
}

int main(void) {
    volatile float test_vals[] = {1.0f, -2.0f, 0.0f, -0.0f, 15.5f, -30.25f};
    volatile int32_t int_vals[] = {0, 100, -100, INT32_MAX, INT32_MIN};
    volatile uint32_t uint_vals[] = {0, 100, UINT32_MAX};
    
    float accum = 0.0f;
    int32_t int_accum = 0;
    uint32_t uint_accum = 0;
    
    /* Exercise COPYSIGN patterns */
    for (int i = 0; i < 6; ++i) {
        accum += copysign_identical(test_vals[i]);
        accum += copysign_const_second(test_vals[i]);
        
        for (int j = 0; j < 6; ++j) {
            if (i != j) {
                accum += copysign_first_neg(test_vals[i], test_vals[j]);
                accum += copysign_first_abs(test_vals[i], test_vals[j]);
                accum += copysign_second_abs(test_vals[i], test_vals[j]);
                accum += copysign_nested_first(test_vals[i], test_vals[j], 2.0f);
                accum += copysign_nested_second(test_vals[i], test_vals[j], -2.0f);
            }
        }
        
        accum = process_value(accum, 5);
    }
    
    /* Exercise saturating division patterns */
    for (int i = 0; i < 5; ++i) {
        int_accum += sat_div_const1(int_vals[i]);
    }
    
    for (int i = 0; i < 3; ++i) {
        uint_accum += usat_div_const1(uint_vals[i]);
    }
    
    /* Final conditional to prevent dead code elimination */
    if (accum > 1000.0f || int_accum > 1000 || uint_accum > 1000) {
        return 1;
    }
    
    return 0;
}
