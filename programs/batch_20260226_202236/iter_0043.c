#include <stdint.h>
#include <math.h>

// Saturating division intrinsics simulation
static int32_t ssdiv(int32_t a, int32_t b) {
    if (b == 0) return a > 0 ? INT32_MAX : (a < 0 ? INT32_MIN : 0);
    int64_t res = (int64_t)a / (int64_t)b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}

static uint32_t usdiv(uint32_t a, uint32_t b) {
    if (b == 0) return a > 0 ? UINT32_MAX : 0;
    uint64_t res = (uint64_t)a / (uint64_t)b;
    if (res > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)res;
}

// Helper functions with control flow
static float process_copysign_identical(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result = copysignf(result, result); // Identical operands
        } else if (i % 3 == 1) {
            result = copysignf(-result, result * 2.0f); // First operand NEG
        } else {
            result = copysignf(result, -3.5f); // Constant second operand
        }
    }
    return result;
}

static double nested_copysign_pattern(double a, double b, double c, int mode) {
    double temp = 0.0;
    switch (mode & 3) {
        case 0:
            temp = copysign(copysign(a, b), c); // First operand COPYSIGN
            break;
        case 1:
            temp = copysign(a, copysign(b, c)); // Second operand COPYSIGN
            break;
        case 2:
            temp = copysign(fabs(a), b); // First operand ABS
            break;
        case 3:
            temp = copysign(a, fabs(b)); // Second operand ABS
            break;
    }
    return temp;
}

static int32_t satdiv_variants(int32_t val, unsigned count) {
    int32_t acc = val;
    unsigned i = 0;
    while (i < count) {
        if (acc > 1000) {
            acc = ssdiv(acc, 1); // SS_DIV by constant 1
        } else if (acc < -1000) {
            acc = ssdiv(acc, 1); // Another SS_DIV by 1
        } else {
            uint32_t u = (uint32_t)(acc + 2000);
            u = usdiv(u, 1); // US_DIV by constant 1
            acc = (int32_t)u - 2000;
        }
        i += (acc & 1) ? 1 : 2;
    }
    return acc;
}

float conditional_copysign(float x, float y, int selector) {
    float res;
    if (selector == 0) {
        res = copysignf(x, 2.0f); // Constant second operand (float)
    } else if (selector == 1) {
        res = copysignf(-x, y); // First operand NEG
    } else if (selector == 2) {
        res = copysignf(copysignf(x, y), -y); // Nested, second inner has no side effects
    } else {
        res = copysignf(x, copysignf(y, x)); // Nested, first inner has no side effects
    }
    return res;
}

int main() {
    volatile int32_t s1 = 5000, s2 = -5000;
    volatile uint32_t u1 = 4000000000u;
    
    // Trigger saturating division by 1
    int32_t r1 = satdiv_variants(s1, 5);
    int32_t r2 = satdiv_variants(s2, 5);
    uint32_t r3 = usdiv(u1, 1);
    
    // COPYSIGN patterns
    float f1 = process_copysign_identical(3.14f, 4);
    double d1 = nested_copysign_pattern(1.5, -2.5, 3.5, 0);
    double d2 = nested_copysign_pattern(-1.5, 2.5, -3.5, 1);
    float f2 = conditional_copysign(7.0f, -8.0f, 0);
    float f3 = conditional_copysign(7.0f, -8.0f, 1);
    float f4 = conditional_copysign(7.0f, -8.0f, 2);
    float f5 = conditional_copysign(7.0f, -8.0f, 3);
    
    // Use results to prevent elimination
    return (int)(r1 + r2 + r3 + (int)f1 + (int)d1 + (int)d2 + (int)f2 + (int)f3 + (int)f4 + (int)f5) & 255;
}
