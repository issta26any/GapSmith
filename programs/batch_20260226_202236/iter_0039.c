#include <stdint.h>
#include <math.h>

// Helper functions to generate various patterns
static float test_copysign_identical(float a) {
    // COPYSIGN with identical operands
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    // COPYSIGN with constant second operand
    return copysign(x, -3.14159);
}

static float test_copysign_neg_first(float y, float z) {
    // COPYSIGN with NEG as first operand
    return copysignf(-y, z);
}

static double test_copysign_abs_first(double y, double z) {
    // COPYSIGN with ABS as first operand
    return copysign(fabs(y), z);
}

static float test_copysign_abs_second(float x, float y) {
    // COPYSIGN with ABS as second operand
    return copysignf(x, fabsf(y));
}

static double test_copysign_nested_first(double a, double b, double c) {
    // COPYSIGN with first operand being another COPYSIGN
    return copysign(copysign(a, b), c);
}

static float test_copysign_nested_second(float a, float b, float c) {
    // COPYSIGN with second operand being another COPYSIGN
    return copysignf(a, copysignf(b, c));
}

// Saturating division helpers (using compiler intrinsics where available)
#ifdef __ARM_ARCH
static int32_t sat_div_int1(int32_t x) {
    // Should generate SS_DIV with constant 1
    return __ssdiv(x, 1);
}

static uint32_t sat_div_uint1(uint32_t x) {
    // Should generate US_DIV with constant 1
    return __usdiv(x, 1);
}
#else
// Fallback implementations for non-ARM targets
static int32_t sat_div_int1(int32_t x) {
    if (x == INT32_MIN) return INT32_MIN;
    return x / 1;
}

static uint32_t sat_div_uint1(uint32_t x) {
    return x / 1;
}
#endif

// Complex control flow with nested conditionals and loops
static float process_sequence(int iterations, float base) {
    float result = base;
    for (int i = 0; i < iterations; ++i) {
        if (i % 3 == 0) {
            result += test_copysign_identical(result);
        } else if (i % 3 == 1) {
            result = test_copysign_neg_first(result, -result);
        } else {
            result = test_copysign_abs_second(result, result * 2.0f);
        }
        
        int j = 0;
        while (j < 2) {
            if (result > 0) {
                result = test_copysign_const((double)result);
            }
            ++j;
        }
    }
    return result;
}

static int validate_sat_division(int32_t val) {
    int32_t sat_result = sat_div_int1(val);
    uint32_t usat_result = sat_div_uint1((uint32_t)val);
    
    if (val > 100) {
        return sat_result + usat_result;
    } else if (val < -50) {
        return sat_result - usat_result;
    } else {
        return sat_result * 2;
    }
}

int main(void) {
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile double d1 = 1.618;
    volatile double d2 = -0.577;
    
    // Test all COPYSIGN patterns
    float r1 = test_copysign_identical(f1);
    double r2 = test_copysign_const(d1);
    float r3 = test_copysign_neg_first(f1, f2);
    double r4 = test_copysign_abs_first(d1, d2);
    float r5 = test_copysign_abs_second(f1, f2);
    double r6 = test_copysign_nested_first(d1, d2, 1.0);
    float r7 = test_copysign_nested_second(f1, f2, 0.0f);
    
    // Complex processing
    float complex_result = process_sequence(10, r1);
    
    // Test saturating division patterns
    int32_t test_vals[] = {0, 100, -100, 1000, -1000};
    int sat_sum = 0;
    for (int i = 0; i < 5; ++i) {
        sat_sum += validate_sat_division(test_vals[i]);
        
        // Nested conditional with function calls
        if (sat_sum > 0) {
            sat_sum += (int)test_copysign_identical((float)sat_sum);
        } else {
            sat_sum -= (int)test_copysign_const((double)sat_sum);
        }
    }
    
    // Final computation using all results
    float final_float = r1 + r3 + r5 + r7 + complex_result;
    double final_double = r2 + r4 + r6;
    
    // Return deterministic value based on all computations
    return (int)(final_float + final_double) + sat_sum;
}
