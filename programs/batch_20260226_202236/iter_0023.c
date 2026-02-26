#include <stdint.h>
#include <math.h>

// Helper functions to create control flow complexity
static int select_value(int a, int b, int cond) {
    if (cond > 0) {
        for (int i = 0; i < 3; ++i) {
            if (i % 2 == cond % 2) {
                return a;
            }
        }
        return b;
    } else {
        int j = 0;
        while (j < 2) {
            if (j == cond) break;
            ++j;
        }
        return (j == 1) ? a : b;
    }
}

static float process_float(float x, int mode) {
    float result = x;
    switch (mode) {
        case 0:
            result = x * 2.0f;
            break;
        case 1:
            for (int k = 0; k < 2; ++k) {
                result += 1.5f;
                if (k == 1) result = -result;
            }
            break;
        case 2:
            if (x > 0.0f) {
                result = x / 2.0f;
            } else {
                result = x * 3.0f;
            }
            break;
        default:
            result = x;
    }
    return result;
}

// Saturating division intrinsics simulation (generic portable implementation)
static int32_t ssdiv_int32(int32_t a, int32_t b) {
    if (b == 0) return (a >= 0) ? INT32_MAX : INT32_MIN;
    if (a == INT32_MIN && b == -1) return INT32_MAX;
    return a / b;
}

static uint32_t usdiv_uint32(uint32_t a, uint32_t b) {
    if (b == 0) return UINT32_MAX;
    return a / b;
}

// Main test function
int main(void) {
    volatile int32_t sat_var = 1000;
    volatile uint32_t usat_var = 2000;
    
    // Target 1: SS_DIV / US_DIV by constant 1
    int32_t ssdiv_result = ssdiv_int32(sat_var, 1);
    uint32_t usdiv_result = usdiv_uint32(usat_var, 1);
    
    // Complex control flow to affect optimization paths
    int cond = select_value(ssdiv_result, usdiv_result, 2);
    
    // Floating point variables for COPYSIGN tests
    float f1 = 3.14f;
    float f2 = -2.71f;
    float f3 = 0.0f;
    float f4 = -0.0f;
    
    // Process floats through non-trivial control flow
    f1 = process_float(f1, cond % 3);
    f2 = process_float(f2, (cond + 1) % 3);
    f3 = process_float(f3, (cond + 2) % 3);
    f4 = process_float(f4, cond % 2);
    
    // Target 2: COPYSIGN with identical operands
    float cs_same = copysignf(f1, f1);
    
    // Target 3: COPYSIGN with constant second operand
    float cs_const1 = copysignf(f2, 2.0f);
    float cs_const2 = copysignf(f3, -3.0f);
    
    // Target 4: COPYSIGN with NEG/ABS as first operand
    float cs_neg = copysignf(-f4, f1);
    float cs_abs = copysignf(fabsf(f2), f3);
    
    // Target 5: COPYSIGN with ABS as second operand
    float cs_abs_second = copysignf(f1, fabsf(f2));
    
    // Target 6: COPYSIGN with nested COPYSIGN as first operand
    float inner_cs = copysignf(f2, f3);  // No side effects in f3
    float cs_nested_first = copysignf(inner_cs, f4);
    
    // Target 7: COPYSIGN with nested COPYSIGN as second operand
    float cs_nested_second = copysignf(f1, inner_cs);  // No side effects in f1
    
    // Use results in conditional to prevent dead code elimination
    float final_result = cs_same + cs_const1 + cs_const2 + cs_neg + cs_abs +
                        cs_abs_second + cs_nested_first + cs_nested_second;
    
    if (final_result > 100.0f) {
        return (int)(final_result + ssdiv_result + usdiv_result) % 256;
    } else {
        return (int)(final_result - ssdiv_result - usdiv_result) % 256;
    }
}
