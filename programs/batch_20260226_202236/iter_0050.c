#include <stdint.h>
#include <math.h>

// Helper functions to create control flow
static int cond_select(int a, int b, int flag) {
    int result = a;
    for (int i = 0; i < 3; ++i) {
        if (flag & (1 << i)) {
            result += b;
        } else {
            result -= b;
        }
    }
    return result;
}

static float loop_copysign(float base, int iterations) {
    float acc = base;
    for (int i = 0; i < iterations; ++i) {
        if (i % 2 == 0) {
            acc = copysignf(acc, -base);
        } else {
            acc = copysignf(acc, base);
        }
    }
    return acc;
}

// ARM-like saturating intrinsics (emulated for portability)
static int32_t ssdiv(int32_t a, int32_t b) {
    if (b == 0) return a >= 0 ? INT32_MAX : INT32_MIN;
    int64_t res = (int64_t)a / b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}

static uint32_t usdiv(uint32_t a, uint32_t b) {
    if (b == 0) return UINT32_MAX;
    return a / b;
}

// Main test function
int main(void) {
    volatile int32_t sval = 1000;
    volatile uint32_t uval = 2000;
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile float f3 = 0.0f;
    volatile float f4 = -0.0f;
    int flag = 7;
    
    // 1. Saturating division by constant 1 (should simplify)
    int32_t sres1 = ssdiv(sval, 1);
    int32_t sres2 = ssdiv(-sval, 1);
    uint32_t ures1 = usdiv(uval, 1);
    uint32_t ures2 = usdiv(uval + 100, 1);
    
    // 2. COPYSIGN with identical operands
    float c1 = copysignf(f1, f1);
    float c2 = copysignf(f2, f2);
    float c3 = __builtin_copysign(f3, f3);
    
    // 3. COPYSIGN with constant second operand
    float c4 = copysignf(f1, 2.0f);
    float c5 = copysignf(f2, -3.0f);
    float c6 = copysignf(f3, 0.0);
    float c7 = copysignf(f4, -5.5);
    
    // 4. COPYSIGN with NEG/ABS as first operand
    float c8 = copysignf(-f1, f2);
    float c9 = copysignf(fabsf(f2), f1);
    float c10 = copysignf(-fabsf(f1), f3);
    
    // 5. COPYSIGN with ABS as second operand
    float c11 = copysignf(f1, fabsf(f2));
    float c12 = copysignf(f2, fabsf(f1));
    
    // 6. Nested COPYSIGN (first operand is COPYSIGN)
    float inner1 = copysignf(f1, f2);
    float c13 = copysignf(inner1, f3);
    float c14 = copysignf(copysignf(f2, f3), f1);
    
    // 7. Nested COPYSIGN (second operand is COPYSIGN)
    float inner2 = copysignf(f2, f3);
    float c15 = copysignf(f1, inner2);
    float c16 = copysignf(f3, copysignf(f1, f2));
    
    // Mix with control flow
    int cond = cond_select(sres1, sres2, flag);
    float loop_res = loop_copysign(c1, cond % 5);
    
    // Use results to prevent elimination
    float sum = c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9 + c10 +
                c11 + c12 + c13 + c14 + c15 + c16 + loop_res;
    int isum = sres1 + sres2 + ures1 + ures2 + cond;
    
    return ((sum != 0.0f) && (isum != 0)) ? 0 : 1;
}
