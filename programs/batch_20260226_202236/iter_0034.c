#include <stdint.h>
#include <math.h>

// Helper functions to create control flow
static int select_value(int a, int b, int cond) {
    if (cond > 0) {
        for (int i = 0; i < 3; ++i) {
            if (i % 2 == cond % 2) {
                a += b;
            } else {
                b -= a;
            }
        }
        return a;
    } else {
        int j = 5;
        while (j-- > 0) {
            b = b * 2 + 1;
        }
        return b;
    }
}

static float process_float(float x, int mode) {
    float result = x;
    switch (mode) {
        case 0:
            for (int i = 0; i < 4; ++i) {
                result += 1.0f;
                if (result > 10.0f) break;
            }
            break;
        case 1:
            result = -result;
            if (result < 0.0f) {
                result = result * 0.5f;
            }
            break;
        case 2:
            result = fabsf(result);
            for (int k = 0; k < 2; ++k) {
                result = result / 2.0f;
            }
            break;
        default:
            result = 0.0f;
    }
    return result;
}

// Saturating division intrinsics simulation (generic fallback)
static int ssdiv_intrinsic(int a, int b) {
    if (b == 0) return (a >= 0) ? INT32_MAX : INT32_MIN;
    int64_t tmp = (int64_t)a / (int64_t)b;
    if (tmp > INT32_MAX) return INT32_MAX;
    if (tmp < INT32_MIN) return INT32_MIN;
    return (int)tmp;
}

static unsigned usdiv_intrinsic(unsigned a, unsigned b) {
    if (b == 0) return UINT32_MAX;
    return a / b;
}

int main(void) {
    volatile int seed = 42; // prevent constant folding
    int x = seed;
    unsigned ux = (unsigned)seed;
    
    // Target 1: SS_DIV / US_DIV by constant 1
    int y1 = ssdiv_intrinsic(x, 1);
    unsigned y2 = usdiv_intrinsic(ux, 1);
    
    // Control flow to use results
    int cond1 = select_value(x, y1, seed);
    unsigned cond2 = (unsigned)select_value((int)ux, (int)y2, seed + 1);
    
    // Floating point variables
    float f1 = (float)cond1 * 0.1f;
    float f2 = (float)cond2 * 0.2f;
    float f3 = process_float(f1, cond1 % 3);
    float f4 = process_float(f2, cond2 % 3);
    
    // Target 2: COPYSIGN with identical operands
    float c1 = copysignf(f3, f3);
    
    // Target 3: COPYSIGN with constant second operand
    float c2 = copysignf(f4, 2.0f);
    float c3 = copysignf(f3, -3.0f);
    
    // Target 4: COPYSIGN with NEG/ABS as first operand
    float c4 = copysignf(-f2, f1);
    float c5 = copysignf(fabsf(f1), f2);
    
    // Target 5: COPYSIGN with ABS as second operand
    float c6 = copysignf(f3, fabsf(f4));
    
    // Target 6: COPYSIGN with nested COPYSIGN as first operand
    float inner1 = copysignf(f1, f2); // second operand f2 has no side effects
    float c7 = copysignf(inner1, f3);
    
    // Target 7: COPYSIGN with nested COPYSIGN as second operand
    float inner2 = copysignf(f4, f3); // first operand f4 has no side effects
    float c8 = copysignf(f2, inner2);
    
    // Use results in final computation to avoid dead code elimination
    float final = c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8;
    int result = (int)final + y1 + (int)y2;
    
    return result % 256;
}
