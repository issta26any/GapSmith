#include <math.h>
#include <stdint.h>

/* Helper functions to generate various COPYSIGN patterns */
float copysign_identical(float x) {
    /* COPYSIGN with identical operands */
    return copysignf(x, x);
}

float copysign_const_second(float x) {
    /* COPYSIGN with constant second operand */
    return copysignf(x, 2.5f);
}

float copysign_neg_first(float y, float z) {
    /* COPYSIGN with NEG as first operand */
    return copysignf(-y, z);
}

float copysign_abs_first(float y, float z) {
    /* COPYSIGN with ABS as first operand */
    return copysignf(fabsf(y), z);
}

float copysign_abs_second(float x, float y) {
    /* COPYSIGN with ABS as second operand */
    return copysignf(x, fabsf(y));
}

float copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN with COPYSIGN as first operand */
    return copysignf(copysignf(a, b), c);
}

float copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN with COPYSIGN as second operand */
    return copysignf(a, copysignf(b, c));
}

/* Saturating division helpers (using GCC builtins) */
#ifdef __ARM_FEATURE_SAT
int sat_div_signed(int x) {
    /* SS_DIV by constant 1 */
    return __ssdiv(x, 1);
}

unsigned sat_div_unsigned(unsigned x) {
    /* US_DIV by constant 1 */
    return __usdiv(x, 1);
}
#else
/* Fallback definitions for non-ARM targets */
int sat_div_signed(int x) {
    volatile int r = x / 1;
    return r;
}

unsigned sat_div_unsigned(unsigned x) {
    volatile unsigned r = x / 1;
    return r;
}
#endif

/* Control flow with loops and conditionals */
void process_values(float *arr, int n) {
    for (int i = 0; i < n; ++i) {
        float val = arr[i];
        float result = 0.0f;
        
        if (i % 7 == 0) {
            result = copysign_identical(val);
        } else if (i % 7 == 1) {
            result = copysign_const_second(val);
        } else if (i % 7 == 2) {
            result = copysign_neg_first(val, -val);
        } else if (i % 7 == 3) {
            result = copysign_abs_first(val, val * 0.5f);
        } else if (i % 7 == 4) {
            result = copysign_abs_second(val, -val);
        } else if (i % 7 == 5) {
            result = copysign_nested_first(val, val + 1.0f, val - 1.0f);
        } else {
            result = copysign_nested_second(val, val + 2.0f, val - 2.0f);
        }
        
        arr[i] = result;
    }
}

int main(void) {
    float data[32];
    for (int i = 0; i < 32; ++i) {
        data[i] = (i % 2) ? -i * 0.7f : i * 0.7f;
    }
    
    process_values(data, 32);
    
    int sum_s = 0;
    unsigned sum_u = 0;
    for (int j = 0; j < 16; ++j) {
        if (j % 3 == 0) {
            sum_s += sat_div_signed(j);
        } else if (j % 3 == 1) {
            sum_u += sat_div_unsigned(j);
        } else {
            sum_s -= sat_div_signed(-j);
        }
    }
    
    volatile float check = data[0] + data[31];
    volatile int check_s = sum_s;
    volatile unsigned check_u = sum_u;
    
    return (check > 0.0f) ? 0 : 1;
}
