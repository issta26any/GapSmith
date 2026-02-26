#include <math.h>
#include <stdint.h>

/* Helper functions to create various patterns */
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
    /* COPYSIGN with first operand being another COPYSIGN */
    return copysignf(copysignf(a, b), c);
}

float copysign_nested_second(float a, float b, float c) {
    /* COPYSIGN with second operand being another COPYSIGN */
    return copysignf(a, copysignf(b, c));
}

/* Saturating division helpers (using GCC vector extensions) */
typedef int32_t v2si __attribute__((vector_size(8)));
typedef uint32_t v2ui __attribute__((vector_size(8)));

v2si ssdiv_by_one(v2si x) {
    /* Should generate SS_DIV with constant 1 */
    return x / (v2si){1, 1};
}

v2ui usdiv_by_one(v2ui x) {
    /* Should generate US_DIV with constant 1 */
    return x / (v2ui){1, 1};
}

/* Control flow with loops and conditionals */
void process_array(float* arr, int n) {
    for (int i = 0; i < n; ++i) {
        float val = arr[i];
        if (val > 0) {
            arr[i] = copysign_identical(val);
        } else if (val < -10.0f) {
            arr[i] = copysign_neg_first(val, 3.0f);
        } else {
            arr[i] = copysign_const_second(val);
        }
        
        /* Nested conditional */
        if (i % 3 == 0) {
            arr[i] = copysign_abs_first(arr[i], -1.0f);
        } else if (i % 3 == 1) {
            arr[i] = copysign_abs_second(arr[i], arr[i-1 < 0 ? 0 : i-1]);
        }
    }
}

/* Function with switch statement */
float select_operation(int op, float a, float b, float c) {
    switch (op) {
        case 0:
            return copysign_nested_first(a, b, c);
        case 1:
            return copysign_nested_second(a, b, c);
        case 2:
            return copysign_neg_first(a, b);
        case 3:
            return copysign_abs_first(a, b);
        default:
            return copysign_identical(a);
    }
}

/* Main function with deterministic behavior */
int main() {
    /* Test data */
    float arr[10];
    for (int i = 0; i < 10; ++i) {
        arr[i] = (i % 2 == 0) ? i * 1.5f : -i * 2.0f;
    }
    
    /* Process array with complex control flow */
    process_array(arr, 10);
    
    /* Test various copysign patterns */
    float x = 5.0f, y = -3.0f, z = 7.0f;
    float r1 = copysign_identical(x);
    float r2 = copysign_const_second(y);
    float r3 = copysign_neg_first(z, x);
    float r4 = copysign_abs_first(y, z);
    float r5 = copysign_abs_second(x, y);
    float r6 = copysign_nested_first(x, y, z);
    float r7 = copysign_nested_second(x, y, z);
    
    /* Test saturating division patterns */
    v2si vec_si = {100, -200};
    v2ui vec_ui = {300, 400};
    v2si res_si = ssdiv_by_one(vec_si);
    v2ui res_ui = usdiv_by_one(vec_ui);
    
    /* Use results to prevent dead code elimination */
    float sum = r1 + r2 + r3 + r4 + r5 + r6 + r7;
    for (int i = 0; i < 10; ++i) {
        sum += arr[i];
    }
    
    int32_t si_sum = res_si[0] + res_si[1];
    uint32_t ui_sum = res_ui[0] + res_ui[1];
    
    /* Deterministic return value */
    return ((int)sum + si_sum + (int)ui_sum) % 256;
}
