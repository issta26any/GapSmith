#include <stdint.h>
#include <math.h>

// Helper functions to generate various patterns
static float test_copysign_identical(float a) {
    // COPYSIGN with identical operands
    return copysignf(a, a);
}

static double test_copysign_const(double x) {
    // COPYSIGN with constant second operand
    if (x > 0) {
        return copysign(x, 2.0);
    } else {
        return copysign(x, -3.0);
    }
}

static float test_copysign_first_neg(float y, float z) {
    // COPYSIGN where first operand is NEG
    for (int i = 0; i < 3; ++i) {
        if (z > 0) {
            return copysignf(-y, z);
        }
        z -= 0.5f;
    }
    return 0.0f;
}

static float test_copysign_first_abs(float y, float z) {
    // COPYSIGN where first operand is ABS
    float result = copysignf(fabsf(y), z);
    if (y < 0) {
        result += 1.0f;
    }
    return result;
}

static double test_copysign_second_abs(double x, double y) {
    // COPYSIGN where second operand is ABS
    double temp = copysign(x, fabs(y));
    for (int j = 0; j < 2; ++j) {
        if (y < 0) {
            temp *= 1.1;
        }
    }
    return temp;
}

static float test_copysign_nested_first(float a, float b, float c) {
    // COPYSIGN where first operand is another COPYSIGN
    // b has no side effects
    float inner = copysignf(a, b);
    return copysignf(inner, c);
}

static double test_copysign_nested_second(double a, double b, double c) {
    // COPYSIGN where second operand is another COPYSIGN
    // b has no side effects
    double inner = copysign(b, c);
    return copysign(a, inner);
}

// Saturating division simulation (generic implementation)
static int32_t ssdiv_int32(int32_t x, int32_t y) {
    if (y == 0) return 0;
    int64_t result = (int64_t)x / (int64_t)y;
    if (result > INT32_MAX) return INT32_MAX;
    if (result < INT32_MIN) return INT32_MIN;
    return (int32_t)result;
}

static uint32_t usdiv_uint32(uint32_t x, uint32_t y) {
    if (y == 0) return 0;
    uint64_t result = (uint64_t)x / (uint64_t)y;
    if (result > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)result;
}

int main() {
    volatile int32_t sat_signed;
    volatile uint32_t sat_unsigned;
    
    // Trigger SS_DIV/US_DIV by constant 1 patterns
    for (int i = -5; i <= 5; ++i) {
        // These should simplify to just the numerator
        sat_signed = ssdiv_int32(i, 1);
        sat_unsigned = usdiv_uint32(i > 0 ? i : -i, 1);
    }
    
    float f1 = 3.14f, f2 = -2.71f, f3 = 1.618f;
    double d1 = 0.577, d2 = -1.414, d3 = 2.718;
    
    // Control flow with nested conditionals
    if (f1 > 0) {
        float r1 = test_copysign_identical(f1);
        double r2 = test_copysign_const(d1);
        
        if (r1 > r2) {
            float r3 = test_copysign_first_neg(f2, f3);
            float r4 = test_copysign_first_abs(f2, f3);
            
            for (int k = 0; k < 2; ++k) {
                double r5 = test_copysign_second_abs(d1, d2);
                if (r5 < 0) {
                    float r6 = test_copysign_nested_first(f1, f2, f3);
                    double r7 = test_copysign_nested_second(d1, d2, d3);
                    sat_signed = ssdiv_int32((int)r6, 1);
                }
            }
        } else {
            double r8 = test_copysign_nested_second(d2, d3, d1);
            sat_unsigned = usdiv_uint32((unsigned)r8, 1);
        }
    }
    
    // Loop with multiple iterations
    float accum = 0.0f;
    for (int n = 0; n < 4; ++n) {
        switch (n % 3) {
            case 0:
                accum += test_copysign_identical(f1 + n);
                sat_signed = ssdiv_int32(n, 1);
                break;
            case 1:
                accum += test_copysign_first_neg(f2, f3 + n);
                sat_unsigned = usdiv_uint32(n * 10, 1);
                break;
            case 2:
                accum += test_copysign_nested_first(f1, f2 + n, f3);
                break;
        }
    }
    
    // Final deterministic result
    return (int)(accum + sat_signed + sat_unsigned) & 0xFF;
}
