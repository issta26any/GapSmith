#include <stdint.h>
#include <math.h>

// Helper functions to generate various patterns
static float test_copysign_identical(float a) {
    // COPYSIGN with identical operands
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    // COPYSIGN with constant second operand
    if (x > 0) return copysign(x, 2.0);
    else return copysign(x, -3.0);
}

static float test_copysign_first_neg(float y, float z) {
    // COPYSIGN where first operand is NEG
    return copysignf(-y, z);
}

static double test_copysign_first_abs(double y, double z) {
    // COPYSIGN where first operand is ABS
    return copysign(fabs(y), z);
}

static float test_copysign_second_abs(float x, float y) {
    // COPYSIGN where second operand is ABS
    return copysignf(x, fabsf(y));
}

static double test_copysign_nested_first(double a, double b, double c) {
    // COPYSIGN where first operand is another COPYSIGN
    // with side-effect-free second operand
    return copysign(copysign(a, b), c);
}

static float test_copysign_nested_second(float a, float b, float c) {
    // COPYSIGN where second operand is another COPYSIGN
    // with side-effect-free first operand
    return copysignf(a, copysignf(b, c));
}

// Saturating division simulation (emulating intrinsics)
static int32_t ssdiv_by_one(int32_t x) {
    // Should generate SS_DIV with constant 1
    // Using inline assembly to simulate saturating division
    int32_t result;
    #ifdef __ARM_FEATURE_SAT
    __asm__("ssdiv %0, %1, #1" : "=r"(result) : "r"(x));
    #else
    // Fallback: emulate saturation
    if (x == INT32_MIN && -1 == -1) {
        result = INT32_MAX;
    } else {
        result = x / 1;
    }
    #endif
    return result;
}

static uint32_t usdiv_by_one(uint32_t x) {
    // Should generate US_DIV with constant 1
    uint32_t result;
    #ifdef __ARM_FEATURE_SAT
    __asm__("usdiv %0, %1, #1" : "=r"(result) : "r"(x));
    #else
    result = x / 1U;
    #endif
    return result;
}

// Complex control flow with loops and conditionals
static float process_float_values(float arr[], int n) {
    float acc = 1.0f;
    for (int i = 0; i < n; i++) {
        if (arr[i] != 0.0f) {
            acc *= test_copysign_identical(arr[i]);
            if (i % 2 == 0) {
                acc += test_copysign_first_neg(arr[i], acc);
            } else {
                acc -= test_copysign_second_abs(acc, arr[i]);
            }
        }
    }
    return acc;
}

static double process_double_values(double a, double b, int iterations) {
    double result = 0.0;
    while (iterations-- > 0) {
        result += test_copysign_const(a);
        result -= test_copysign_first_abs(b, result);
        
        if (iterations % 3 == 0) {
            result *= test_copysign_nested_first(a, b, result);
        } else if (iterations % 3 == 1) {
            result /= test_copysign_nested_second((float)a, (float)b, (float)result) + 1.0;
        }
        
        a = b * 0.9;
        b = result * 1.1;
    }
    return result;
}

int main(void) {
    // Test data
    float farr[] = {1.5f, -2.3f, 0.0f, 4.7f, -0.0f};
    double d1 = 3.14159, d2 = -2.71828;
    
    // Process floats with complex control flow
    float fresult = process_float_values(farr, 5);
    
    // Process doubles with loops
    double dresult = process_double_values(d1, d2, 10);
    
    // Test saturating division by 1
    int32_t sat_int = 0;
    uint32_t sat_uint = 0;
    for (int i = -5; i <= 5; i++) {
        sat_int += ssdiv_by_one(i * 1000000);
        sat_uint += usdiv_by_one(i * 1000000 + 1000000000U);
    }
    
    // Final computation mixing everything
    float final = (float)dresult + fresult + (float)sat_int * 0.001f + (float)sat_uint * 0.000001f;
    
    // Use final result to avoid dead code elimination
    volatile float sink = final;
    (void)sink;
    
    return (final > 0.0f) ? 0 : 1;
}
