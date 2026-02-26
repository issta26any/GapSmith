#include <stdint.h>
#include <math.h>

// Saturating division intrinsics simulation
static int ssdiv(int a, int b) {
    if (b == 0) return a >= 0 ? INT32_MAX : INT32_MIN;
    int64_t res = (int64_t)a / b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int)res;
}

static unsigned usdiv(unsigned a, unsigned b) {
    if (b == 0) return UINT32_MAX;
    uint64_t res = (uint64_t)a / b;
    if (res > UINT32_MAX) return UINT32_MAX;
    return (unsigned)res;
}

// Helper functions with control flow
static float process_copysign(float base, float sign) {
    float result = copysignf(base, sign);
    for (int i = 0; i < 3; ++i) {
        if (result > 0.0f) {
            result *= 0.5f;
        } else {
            result += 1.0f;
        }
    }
    return result;
}

static double nested_copysign(double a, double b, double c) {
    if (a != 0.0) {
        return copysign(copysign(a, b), c);
    }
    return 0.0;
}

static float abs_neg_chain(float x, float y) {
    float temp = -x;
    if (y > 0.0f) {
        temp = fabsf(temp);
    }
    return copysignf(temp, y);
}

int main() {
    volatile int var_int = 1000;
    volatile unsigned var_uint = 5000;
    
    // Target 1: SS_DIV/US_DIV by constant 1
    int sat_div1 = ssdiv(var_int, 1);
    unsigned usat_div1 = usdiv(var_uint, 1);
    
    // Target 2: COPYSIGN with identical operands
    float f1 = 3.14f;
    float identical_cs = copysignf(f1, f1);
    
    // Target 3: COPYSIGN with constant second operand
    float const_cs1 = copysignf(f1, 2.0f);
    float const_cs2 = copysignf(f1, -3.0f);
    
    // Target 4: COPYSIGN with NEG/ABS as first operand
    float neg_val = -2.5f;
    float abs_val = 4.8f;
    float cs_neg = copysignf(-neg_val, 1.5f);
    float cs_abs = copysignf(fabsf(abs_val), -2.2f);
    
    // Target 5: COPYSIGN with ABS as second operand
    float cs_abs_second = copysignf(1.8f, fabsf(neg_val));
    
    // Target 6: Nested COPYSIGN (first operand is COPYSIGN)
    double d1 = 1.5, d2 = -2.5, d3 = 3.5;
    double nested1 = nested_copysign(d1, d2, d3);
    
    // Target 7: COPYSIGN where second operand is COPYSIGN
    double inner_cs = copysign(d2, d3);
    double cs_second_cs = copysign(d1, inner_cs);
    
    // Complex control flow mixing operations
    float accumulator = 0.0f;
    for (int i = 0; i < 10; ++i) {
        if (i % 3 == 0) {
            accumulator += process_copysign(accumulator, (float)i);
        } else if (i % 3 == 1) {
            accumulator -= abs_neg_chain(accumulator, (float)i);
        } else {
            accumulator *= copysignf(accumulator, -accumulator);
        }
    }
    
    // Conditional use of results to prevent dead code elimination
    int result = 0;
    if (sat_div1 > 0) result += 1;
    if (usat_div1 > 100) result += 2;
    if (identical_cs > 0.0f) result += 4;
    if (const_cs1 != const_cs2) result += 8;
    if (cs_neg < 0.0f) result += 16;
    if (cs_abs > 0.0f) result += 32;
    if (cs_abs_second > 0.0f) result += 64;
    if (nested1 != 0.0) result += 128;
    if (cs_second_cs < 0.0) result += 256;
    if (accumulator != 0.0f) result += 512;
    
    return result % 256;
}
