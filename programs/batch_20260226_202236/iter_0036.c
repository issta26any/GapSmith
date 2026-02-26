#include <stdint.h>
#include <math.h>

// Saturating arithmetic intrinsics simulation
static int32_t ssdiv_int32(int32_t a, int32_t b) {
    if (b == 0) return a > 0 ? INT32_MAX : (a < 0 ? INT32_MIN : 0);
    if (a == INT32_MIN && b == -1) return INT32_MAX;
    return a / b;
}

static uint32_t usdiv_uint32(uint32_t a, uint32_t b) {
    if (b == 0) return a > 0 ? UINT32_MAX : 0;
    return a / b;
}

// Helper functions with control flow
static float process_copysign_ident(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result = copysignf(result, result);
        } else if (i % 3 == 1) {
            result = copysignf(-result, result);
        } else {
            result = copysignf(fabsf(result), result);
        }
    }
    return result;
}

static double nested_copysign_chain(double a, double b, double c, int depth) {
    double acc = a;
    while (depth > 0) {
        if (depth % 4 == 0) {
            acc = copysign(acc, copysign(b, c));
        } else if (depth % 4 == 1) {
            acc = copysign(copysign(acc, b), c);
        } else if (depth % 4 == 2) {
            acc = copysign(acc, 2.5);
        } else {
            acc = copysign(acc, -3.75);
        }
        --depth;
    }
    return acc;
}

static float abs_second_operand(float x, float y, int selector) {
    float res = x;
    switch (selector) {
        case 0:
            res = copysignf(res, fabsf(y));
            break;
        case 1:
            res = copysignf(-res, fabsf(y));
            break;
        case 2:
            res = copysignf(fabsf(res), fabsf(y));
            break;
        default:
            res = copysignf(res, y);
    }
    return res;
}

int main(void) {
    volatile int32_t sat_div_var = 1000;
    int32_t sat_div_result;
    uint32_t usat_div_result;
    
    // Trigger SS_DIV/US_DIV with constant 1
    for (int i = 0; i < 10; ++i) {
        if (i % 2 == 0) {
            sat_div_result = ssdiv_int32(sat_div_var + i, 1);
        } else {
            usat_div_result = usdiv_uint32((uint32_t)(sat_div_var + i), 1U);
        }
    }
    
    // COPYSIGN with identical operands
    float f1 = 3.14f;
    float f2 = -2.71f;
    f1 = process_copysign_ident(f1, 5);
    f2 = process_copysign_ident(f2, 3);
    
    // COPYSIGN with constant second operand
    double d1 = 1.414;
    for (int j = 0; j < 8; ++j) {
        if (j & 1) {
            d1 = copysign(d1, 2.0);
        } else {
            d1 = copysign(d1, -3.0);
        }
    }
    
    // COPYSIGN with NEG/ABS as first operand
    float f3 = 5.0f;
    float f4 = -7.0f;
    for (int k = 0; k < 6; ++k) {
        if (k % 3 == 0) {
            f3 = copysignf(-f3, f4);
        } else if (k % 3 == 1) {
            f3 = copysignf(fabsf(f3), f4);
        } else {
            f4 = copysignf(-f4, f3);
        }
    }
    
    // COPYSIGN with ABS as second operand
    float f5 = 1.0f;
    float f6 = -4.0f;
    f5 = abs_second_operand(f5, f6, 0);
    f6 = abs_second_operand(f6, f5, 1);
    
    // Nested COPYSIGN patterns
    double d2 = 1.0;
    double d3 = -1.0;
    double d4 = 0.5;
    d2 = nested_copysign_chain(d2, d3, d4, 7);
    d3 = nested_copysign_chain(d3, d4, d2, 5);
    
    // Final deterministic result
    int32_t final_check = (int32_t)(f1 + f2 + f3 + f4 + f5 + f6 + d1 + d2 + d3 + d4)
                          + sat_div_result + (int32_t)usat_div_result;
    
    return final_check != 0 ? 0 : 1;
}
