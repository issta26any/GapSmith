#include <math.h>
#include <stdint.h>

/* Helper functions to generate various copysign patterns */
float copysign_identical(float x) {
    /* COPYSIGN with identical operands */
    return copysignf(x, x);
}

float copysign_const_second(float x) {
    /* COPYSIGN with constant second operand */
    if (x > 0) return copysignf(x, 2.0f);
    else return copysignf(x, -3.0f);
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
int sat_div_signed(int x) {
    /* Should generate SS_DIV with constant 1 */
    return __ssdiv(x, 1);
}

unsigned sat_div_unsigned(unsigned x) {
    /* Should generate US_DIV with constant 1 */
    return __usdiv(x, 1);
}
#else
/* Fallback implementations if intrinsics not available */
int sat_div_signed(int x) {
    if (x == INT32_MIN) return INT32_MIN;
    return x / 1;
}

unsigned sat_div_unsigned(unsigned x) {
    return x / 1;
}
#endif

/* Complex control flow to exercise multiple paths */
void process_values(float* arr, int n, int mode) {
    for (int i = 0; i < n; i++) {
        float val = arr[i];
        float result = 0.0f;
        
        if (mode == 0) {
            result = copysign_identical(val);
        } else if (mode == 1) {
            result = copysign_const_second(val);
        } else if (mode == 2) {
            result = copysign_first_neg(val, val * 0.5f);
        } else if (mode == 3) {
            result = copysign_first_abs(val, -val);
        } else if (mode == 4) {
            result = copysign_second_abs(val, val + 1.0f);
        } else if (mode == 5) {
            result = copysign_nested_first(val, val * 2.0f, val * 3.0f);
        } else {
            result = copysign_nested_second(val, val * 0.5f, val * 1.5f);
        }
        
        /* Use result to affect control flow */
        if (result > 0.0f) {
            for (int j = 0; j < 3; j++) {
                arr[i] += 0.1f * j;
            }
        } else {
            arr[i] = result;
        }
    }
}

int main(void) {
    float data[10];
    int int_data[10];
    unsigned uint_data[10];
    
    /* Initialize arrays */
    for (int i = 0; i < 10; i++) {
        data[i] = (i % 2 == 0) ? (float)i : -(float)i;
        int_data[i] = i * 100 - 500;
        uint_data[i] = i * 100;
    }
    
    /* Exercise all copysign patterns */
    for (int mode = 0; mode < 7; mode++) {
        process_values(data, 10, mode);
    }
    
    /* Exercise saturating division patterns */
    int sum_signed = 0;
    unsigned sum_unsigned = 0;
    
    for (int i = 0; i < 10; i++) {
        sum_signed += sat_div_signed(int_data[i]);
        sum_unsigned += sat_div_unsigned(uint_data[i]);
        
        /* Nested conditional */
        if (sum_signed > 0) {
            if (sum_unsigned < 1000) {
                sum_signed -= sat_div_signed(i);
            } else {
                sum_unsigned += sat_div_unsigned(i);
            }
        }
    }
    
    /* Final computation using results */
    float final = 0.0f;
    for (int i = 0; i < 10; i++) {
        final += copysign_identical(data[i]) + 
                 copysign_first_abs(data[i], data[(i+1)%10]) +
                 copysign_nested_second(data[i], data[(i+2)%10], data[(i+3)%10]);
    }
    
    /* Prevent dead code elimination */
    volatile float sink = final;
    volatile int sink_int = sum_signed;
    volatile unsigned sink_uint = sum_unsigned;
    
    return (sink > 0.0f) ? 0 : 1;
}
