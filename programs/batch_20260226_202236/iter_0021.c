#include <stdint.h>
#include <math.h>

// Helper functions to generate various patterns
static float copysign_identical(float x) {
    // COPYSIGN with identical operands
    return copysignf(x, x);
}

static float copysign_const_second(float x) {
    // COPYSIGN with constant second operand
    return copysignf(x, 2.0f) + copysignf(x, -3.0f);
}

static float copysign_first_neg(float y, float z) {
    // COPYSIGN where first operand is NEG
    return copysignf(-y, z);
}

static float copysign_first_abs(float y, float z) {
    // COPYSIGN where first operand is ABS
    return copysignf(fabsf(y), z);
}

static float copysign_second_abs(float x, float y) {
    // COPYSIGN where second operand is ABS
    return copysignf(x, fabsf(y));
}

static float copysign_nested_first(float a, float b, float c) {
    // COPYSIGN where first operand is another COPYSIGN
    return copysignf(copysignf(a, b), c);
}

static float copysign_nested_second(float a, float b, float c) {
    // COPYSIGN where second operand is another COPYSIGN
    return copysignf(a, copysignf(b, c));
}

// Saturating division helpers (simulated with builtins if available)
#ifdef __ARM_FEATURE_SAT
    #define SAT_DIV_1(x) __ssdiv(x, 1)
    #define UNSAT_DIV_1(x) __usdiv(x, 1)
#else
    // Fallback implementations that may still generate relevant RTL
    static int sat_div_1(int x) {
        if (x == INT32_MIN) return INT32_MIN;
        return x / 1;
    }
    static unsigned unsat_div_1(unsigned x) {
        return x / 1;
    }
#endif

// Complex control flow to explore compilation paths
static float process_value(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysign_identical(result);
        } else if (i % 3 == 1) {
            result += copysign_const_second(result);
        } else {
            result += copysign_first_neg(result, 1.5f);
        }
        
        // Nested conditionals
        if (result > 0) {
            result = copysign_first_abs(result, -result);
            if (i % 2 == 0) {
                result = copysign_second_abs(result, result * 0.5f);
            }
        } else {
            result = copysign_nested_first(result, result + 1.0f, -result);
        }
    }
    return result;
}

static int saturating_operations(int val, unsigned uval) {
    int sat_result = 0;
    unsigned unsat_result = 0;
    
    for (int i = 0; i < 5; ++i) {
        if (val > 1000) {
            #ifdef __ARM_FEATURE_SAT
                sat_result += SAT_DIV_1(val);
            #else
                sat_result += sat_div_1(val);
            #endif
            val -= 500;
        } else {
            #ifdef __ARM_FEATURE_SAT
                unsat_result += UNSAT_DIV_1(uval);
            #else
                unsat_result += unsat_div_1(uval);
            #endif
            uval += 100;
        }
        
        // Additional branching
        switch (i % 4) {
            case 0: sat_result += 1; break;
            case 1: sat_result -= 1; break;
            case 2: sat_result *= 2; break;
            default: sat_result /= 2; break;
        }
    }
    
    return sat_result + (int)unsat_result;
}

int main(void) {
    float f1 = 1.0f, f2 = -2.5f, f3 = 3.75f;
    int i1 = 10000, i2 = -20000;
    unsigned u1 = 50000;
    
    // Process floating values with copysign patterns
    float res1 = process_value(f1, 4);
    float res2 = process_value(f2, 3);
    float res3 = copysign_nested_second(f1, f2, f3);
    float res4 = copysign_nested_first(res1, res2, res3);
    
    // Process integer values with saturating division patterns
    int int_res = saturating_operations(i1, u1);
    int_res += saturating_operations(i2, u1 * 2);
    
    // Final mixing
    float final_float = res1 + res2 + res3 + res4;
    int final_int = int_res + (int)final_float;
    
    // Deterministic return
    return (final_int > 0) ? 0 : 1;
}
