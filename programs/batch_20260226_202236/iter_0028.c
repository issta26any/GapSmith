#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SS_DIV(x, y) __ssdiv(x, y)
#define US_DIV(x, y) __usdiv(x, y)
#else
#define SS_DIV(x, y) ((x) / (y))
#define US_DIV(x, y) ((x) / (y))
#endif

float helper1(float a, float b) {
    float result = 0.0f;
    for (int i = 0; i < 3; ++i) {
        if (a > b) {
            result += copysignf(a, a);  // Identical operands
        } else {
            result += copysignf(b, -2.5f);  // Constant second operand
        }
    }
    return result;
}

float helper2(float x, float y, float z) {
    float acc = 0.0f;
    int counter = 0;
    while (counter < 2) {
        if (x != 0.0f) {
            acc += copysignf(-y, z);  // First operand is NEG
            acc += copysignf(fabsf(y), z);  // First operand is ABS
        }
        if (z > 0.0f) {
            acc += copysignf(x, fabsf(y));  // Second operand is ABS
        }
        ++counter;
    }
    return acc;
}

float helper3(float a, float b, float c) {
    float tmp = 0.0f;
    if (a < b && b < c) {
        tmp = copysignf(copysignf(a, b), c);  // First operand is COPYSIGN
    } else if (a > c) {
        tmp = copysignf(a, copysignf(b, c));  // Second operand is COPYSIGN
    }
    return tmp;
}

int32_t sat_div_test(int32_t val) {
    int32_t res = 0;
    for (int i = 0; i < 2; ++i) {
        if (val > 100) {
            res += SS_DIV(val, 1);  // SS_DIV by constant 1
        } else {
            res += SS_DIV(-val, 1);
        }
    }
    return res;
}

uint32_t usat_div_test(uint32_t val) {
    uint32_t res = 0;
    int j = 0;
    do {
        if (val != 0) {
            res += US_DIV(val, 1);  // US_DIV by constant 1
        }
        ++j;
    } while (j < 2);
    return res;
}

int main() {
    float f1 = 3.14f, f2 = -2.71f, f3 = 1.414f;
    int32_t i1 = 500, i2 = -300;
    uint32_t u1 = 400, u2 = 0;

    float r1 = helper1(f1, f2);
    float r2 = helper2(f2, f3, f1);
    float r3 = helper3(f1, f2, f3);

    int32_t r4 = sat_div_test(i1) + sat_div_test(i2);
    uint32_t r5 = usat_div_test(u1) + usat_div_test(u2);

    volatile float fr = r1 + r2 + r3;
    volatile int32_t ir = r4;
    volatile uint32_t ur = r5;

    return (fr != 0.0f && ir != 0 && ur != 0) ? 0 : 1;
}
