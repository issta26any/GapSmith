#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SS_DIV(x, y) __ssdiv(x, y)
#define US_DIV(x, y) __usdiv(x, y)
#else
static int32_t SS_DIV(int32_t x, int32_t y) {
    if (x == INT32_MIN && y == -1) return INT32_MAX;
    return x / y;
}
static uint32_t US_DIV(uint32_t x, uint32_t y) {
    if (y == 0) return UINT32_MAX;
    return x / y;
}
#endif

float helper1(float a, float b, int iter) {
    float result = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(a, a);  // identical operands
        } else if (i % 3 == 1) {
            result += copysignf(b, 2.5f);  // constant second operand
        } else {
            result += copysignf(-a, b);  // first operand is NEG
        }
    }
    return result;
}

float helper2(float x, float y, float z) {
    float acc = 0.0f;
    int count = 0;
    while (count < 5) {
        if (z > 0.0f) {
            acc += copysignf(fabsf(x), y);  // first operand is ABS
        } else {
            acc += copysignf(x, fabsf(y));  // second operand is ABS
        }
        z = -z;
        count++;
    }
    return acc;
}

float helper3(float p, float q, float r) {
    float tmp = 0.0f;
    for (int j = 0; j < 4; ++j) {
        if (j & 1) {
            tmp += copysignf(copysignf(p, q), r);  // first operand is COPYSIGN
        } else {
            tmp += copysignf(p, copysignf(q, r));  // second operand is COPYSIGN
        }
    }
    return tmp;
}

int main() {
    volatile int32_t sval = 100;
    volatile uint32_t uval = 200;
    
    int32_t sres = SS_DIV(sval, 1);  // SS_DIV by constant 1
    uint32_t ures = US_DIV(uval, 1); // US_DIV by constant 1
    
    float f1 = 3.0f, f2 = -4.0f, f3 = 1.5f;
    float r1 = helper1(f1, f2, 6);
    float r2 = helper2(f1, f2, f3);
    float r3 = helper3(f1, f2, f3);
    
    float total = r1 + r2 + r3;
    
    return (int)(total + sres + ures) % 127;
}
