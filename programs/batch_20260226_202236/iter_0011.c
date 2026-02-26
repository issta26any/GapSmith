#include <stdint.h>
#include <math.h>

/* Helper functions to generate various patterns */
float copysign_identical(float a) {
    /* COPYSIGN with identical operands */
    return copysignf(a, a);
}

float copysign_const_second(float a) {
    /* COPYSIGN with constant second operand */
    return copysignf(a, 2.0f) + copysignf(a, -3.0f);
}

float copysign_first_neg(float a, float b) {
    /* COPYSIGN where first operand is NEG */
    return copysignf(-a, b);
}

float copysign_first_abs(float a, float b) {
    /* COPYSIGN where first operand is ABS */
    return copysignf(fabsf(a), b);
}

float copysign_second_abs(float a, float b) {
    /* COPYSIGN where second operand is ABS */
    return copysignf(a, fabsf(b));
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
int32_t ssdiv_by_one(int32_t x) {
    /* Should generate SS_DIV with constant 1 */
    return __ssdiv(x, 1);
}

uint32_t usdiv_by_one(uint32_t x) {
    /* Should generate US_DIV with constant 1 */
    return __usdiv(x, 1);
}
#else
/* Fallback implementations if intrinsics not available */
int32_t ssdiv_by_one(int32_t x) {
    volatile int32_t y = 1;
    return x / y; /* May not generate saturating RTL but keeps code valid */
}

uint32_t usdiv_by_one(uint32_t x) {
    volatile uint32_t y = 1;
    return x / y;
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
        }
    }
    
    /* Nested conditionals */
    if (iter > 5) {
        result = copysign_nested_first(result, 2.0f, -result);
    }
    if (iter < 10) {
        result = copysign_nested_second(-result, result, 3.0f);
    }
    
    return result;
}

int main(void) {
    float f1 = 3.14f;
    float f2 = -2.71f;
    float f3 = 0.0f;
    
    /* Loop with multiple copysign patterns */
    for (int i = 0; i < 8; ++i) {
        f1 = process_value(f1, i);
        f2 = process_value(f2, i + 1);
        f3 = copysign_first_abs(f1, f2) + copysign_second_abs(f2, f1);
        
        if (i % 2 == 0) {
            f3 = copysign_nested_first(f3, f1, f2);
        } else {
            f3 = copysign_nested_second(f3, f2, f1);
        }
    }
    
    /* Integer saturating division paths */
    int32_t int_val = 100;
    uint32_t uint_val = 200;
    
    for (int j = 0; j < 4; ++j) {
        if (j % 2 == 0) {
            int_val = ssdiv_by_one(int_val + j);
        } else {
            uint_val = usdiv_by_one(uint_val + j);
        }
    }
    
    /* Final deterministic result */
    volatile float final_float = f1 + f2 + f3;
    volatile int32_t final_int = int_val;
    volatile uint32_t final_uint = uint_val;
    
    return (final_int + final_uint) != 0 ? 0 : 1;
}
