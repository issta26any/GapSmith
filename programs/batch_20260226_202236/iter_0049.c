#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SAT_DIV(x, y) __ssdiv(x, y)
#define UNSAT_DIV(x, y) __usdiv(x, y)
#else
static int SAT_DIV(int x, int y) {
    if (y == 0) return 0;
    if (x == INT32_MIN && y == -1) return INT32_MAX;
    return x / y;
}
static unsigned UNSAT_DIV(unsigned x, unsigned y) {
    if (y == 0) return 0;
    return x / y;
}
#endif

float helper1(float a, float b, int iter) {
    float result = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(a, a);  // COPYSIGN identical operands
        } else if (i % 3 == 1) {
            result += copysignf(a, 2.5f);  // COPYSIGN second operand constant
        } else {
            result += copysignf(a, -4.0f);
        }
    }
    return result;
}

float helper2(float x, float y, float z) {
    float t = 0.0f;
    if (x != y) {
        t += copysignf(-y, z);  // COPYSIGN first operand NEG
        t += copysignf(fabsf(y), z);  // COPYSIGN first operand ABS
    } else {
        t += copysignf(x, fabsf(z));  // COPYSIGN second operand ABS
    }
    return t;
}

float helper3(float a, float b, float c) {
    float r1 = copysignf(copysignf(a, b), c);  // COPYSIGN first operand COPYSIGN
    float r2 = copysignf(a, copysignf(b, c));  // COPYSIGN second operand COPYSIGN
    return r1 + r2;
}

int helper4(int val, unsigned uval) {
    int sdiv = SAT_DIV(val, 1);  // SS_DIV by constant 1
    unsigned udiv = UNSAT_DIV(uval, 1);  // US_DIV by constant 1
    return sdiv + (int)udiv;
}

int main() {
    volatile int seed = 0;
    float fsum = 0.0f;
    int isum = 0;
    
    for (int i = 1; i < 10; ++i) {
        float f1 = (i * 0.7f) - 3.0f;
        float f2 = (i * 0.3f) + 1.0f;
        float f3 = (i * 0.5f) - 2.0f;
        
        if (i % 2 == 0) {
            fsum += helper1(f1, f2, i);
            fsum += helper2(f1, f2, f3);
        } else {
            fsum += helper3(f1, f2, f3);
        }
        
        int ival = (i * 100) - 500;
        unsigned uval = i * 200;
        isum += helper4(ival, uval);
        
        if (fsum > 100.0f) {
            fsum *= 0.9f;
        }
    }
    
    return (int)fsum + isum;
}
