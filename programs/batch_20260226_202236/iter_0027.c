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

/* Saturating division helpers (using GCC vector extensions as proxy) */
typedef int v2si __attribute__((vector_size(8)));
typedef unsigned v2usi __attribute__((vector_size(8)));

static v2si sat_div_const1(v2si x) {
    /* Should generate SS_DIV with constant 1 after lowering */
    v2si one = {1, 1};
    v2si mask = x < one;
    v2si result = x / one;
    return result | mask;  /* Simulate saturation effect */
}

static v2usi usat_div_const1(v2usi x) {
    /* Should generate US_DIV with constant 1 after lowering */
    v2usi one = {1, 1};
    v2usi mask = x < one;
    v2usi result = x / one;
    return result | mask;
}

/* Control flow with loops and conditionals */
static void process_array(float* arr, int n) {
    for (int i = 0; i < n; ++i) {
        float val = arr[i];
        if (val != 0.0f) {
            if (i % 3 == 0) {
                arr[i] = test_copysign_identical(val);
            } else if (i % 3 == 1) {
                arr[i] = test_copysign_first_neg(val, arr[(i+1)%n]);
            } else {
                arr[i] = test_copysign_second_abs(val, arr[(i+2)%n]);
            }
        }
    }
}

static double compute_sequence(int iterations) {
    double acc = 1.0;
    for (int i = 1; i <= iterations; ++i) {
        switch (i % 5) {
            case 0:
                acc += test_copysign_const(acc);
                break;
            case 1:
                acc += test_copysign_first_abs(acc, (double)i);
                break;
            case 2:
                acc += test_copysign_nested_first(acc, (double)i, -acc);
                break;
            case 3:
                acc += test_copysign_nested_second(acc, (double)i, -acc);
                break;
            default:
                acc *= 0.9;
        }
        if (acc > 100.0) acc = 1.0;
    }
    return acc;
}

int main(void) {
    /* Initialize test data */
    float farr[12];
    for (int i = 0; i < 12; ++i) {
        farr[i] = (i % 2 == 0) ? (float)i : -(float)i;
    }
    
    /* Exercise copysign patterns */
    process_array(farr, 12);
    
    /* Exercise saturating division patterns */
    v2si vec1 = {5, -3};
    v2si vec2 = {0, 2};
    v2si sres1 = sat_div_const1(vec1);
    v2si sres2 = sat_div_const1(vec2);
    
    v2usi uvec1 = {5, 3};
    v2usi uvec2 = {0, 1};
    v2usi ures1 = usat_div_const1(uvec1);
    v2usi ures2 = usat_div_const1(uvec2);
    
    /* Exercise floating-point patterns with control flow */
    double dres = compute_sequence(25);
    
    /* Use results to avoid dead code elimination */
    volatile float check1 = farr[0] + farr[11];
    volatile double check2 = dres;
    volatile int check3 = sres1[0] + sres2[1];
    volatile unsigned check4 = ures1[0] + ures2[1];
    
    return (check1 > 0.0f) ? 0 : 1;
}
