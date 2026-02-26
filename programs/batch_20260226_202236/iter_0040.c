#include <stdint.h>
#include <math.h>

/* Helper functions to generate various patterns */
static float test_copysign_identical(float a) {
    /* COPYSIGN with identical operands */
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    /* COPYSIGN with constant second operand */
    if (x > 0.0) {
        return copysign(x, 2.5);  /* CONST_DOUBLE positive */
    } else {
        return copysign(x, -3.75); /* CONST_DOUBLE negative */
    }
}

static float test_copysign_first_neg(float y, float z) {
    /* COPYSIGN where first operand is NEG */
    return copysignf(-y, z);
}

static double test_copysign_first_abs(double y, double z) {
    /* COPYSIGN where first operand is ABS */
    return copysign(fabs(y), z);
}

static float test_copysign_second_abs(float x, float y) {
    /* COPYSIGN where second operand is ABS */
    return copysignf(x, fabsf(y));
}

static float test_copysign_nested_first(float a, float b, float c) {
    /* COPYSIGN where first operand is another COPYSIGN */
    return copysignf(copysignf(a, b), c);
}

static double test_copysign_nested_second(double a, double b, double c) {
    /* COPYSIGN where second operand is another COPYSIGN */
    return copysign(a, copysign(b, c));
}

/* Saturating division helpers (using GCC vector extensions for saturation) */
typedef int v2si __attribute__((vector_size(8)));
typedef unsigned v2usi __attribute__((vector_size(8)));

static int sat_div_signed(int x) {
    /* Generate SS_DIV by constant 1 via vector intrinsic pattern */
    v2si v = {x, x};
    v2si one = {1, 1};
    v2si res = __builtin_ssdiv_round(v, one, 0); /* SS_DIV by CONST1_RTX */
    return res[0];
}

static unsigned sat_div_unsigned(unsigned x) {
    /* Generate US_DIV by constant 1 via vector intrinsic pattern */
    v2usi v = {x, x};
    v2usi one = {1, 1};
    v2usi res = __builtin_usdiv_round(v, one, 0); /* US_DIV by CONST1_RTX */
    return res[0];
}

/* Control flow with loops and conditionals */
static void process_values(float *arr, int n) {
    for (int i = 0; i < n; i++) {
        float val = arr[i];
        if (val != 0.0f) {
            arr[i] = test_copysign_identical(val);
            if (i % 3 == 0) {
                arr[i] += test_copysign_first_neg(val, arr[i-1 < 0 ? 0 : i-1]);
            } else if (i % 3 == 1) {
                arr[i] += test_copysign_second_abs(val, arr[i+1 < n ? i+1 : 0]);
            }
        }
    }
}

int main(void) {
    float farr[10];
    double darr[10];
    int iarr[10];
    unsigned uarr[10];
    
    /* Initialize arrays */
    for (int i = 0; i < 10; i++) {
        farr[i] = (i - 5) * 0.7f;
        darr[i] = (i - 3) * 1.3;
        iarr[i] = i - 7;
        uarr[i] = i * 3u;
    }
    
    /* Complex control flow */
    process_values(farr, 10);
    
    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            darr[i] = test_copysign_const(darr[i]);
            if (i > 2 && i < 8) {
                darr[i] += test_copysign_first_abs(darr[i-1], darr[i+1]);
            }
        } else {
            darr[i] = test_copysign_nested_second(darr[i], darr[i-1], darr[9-i]);
        }
        
        /* Nested copysign patterns */
        if (i % 4 == 0) {
            farr[i] = test_copysign_nested_first(farr[i], farr[9-i], farr[i/2]);
        }
    }
    
    /* Saturating division patterns */
    int sum_s = 0;
    unsigned sum_u = 0;
    for (int i = 0; i < 10; i++) {
        if (iarr[i] != 0) {
            sum_s += sat_div_signed(iarr[i]);  /* Should simplify SS_DIV by 1 */
        }
        if (uarr[i] != 0) {
            sum_u += sat_div_unsigned(uarr[i]); /* Should simplify US_DIV by 1 */
        }
    }
    
    /* Final deterministic result */
    float fsum = 0.0f;
    double dsum = 0.0;
    for (int i = 0; i < 10; i++) {
        fsum += farr[i];
        dsum += darr[i];
    }
    
    /* Prevent dead code elimination */
    return (int)(fsum + dsum + sum_s + sum_u) % 256;
}
