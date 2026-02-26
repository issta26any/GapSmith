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
            result += copysignf(a, 2.5f);  // constant second operand
        } else {
            result += copysignf(a, -4.0f);  // constant second operand (negative)
        }
    }
    return result;
}

float helper2(float x, float y, float z) {
    float t = 0.0f;
    if (x > y) {
        t = copysignf(-y, z);  // first operand is NEG
    } else if (x < y) {
        t = copysignf(fabsf(y), z);  // first operand is ABS
    } else {
        t = copysignf(x, fabsf(z));  // second operand is ABS
    }
    return t;
}

float helper3(float a, float b, float c, int sel) {
    float r = a;
    for (int i = 0; i < sel; ++i) {
        if (i % 2 == 0) {
            r = copysignf(copysignf(r, b), c);  // first operand is COPYSIGN
        } else {
            r = copysignf(r, copysignf(b, c));  // second operand is COPYSIGN
        }
    }
    return r;
}

void process_saturating(int32_t sval, uint32_t uval, int cond) {
    int32_t sres = 0;
    uint32_t ures = 0;
    for (int i = 0; i < cond; ++i) {
        if (i & 1) {
            sres += SS_DIV(sval, 1);  // signed saturating division by 1
        } else {
            ures += US_DIV(uval, 1);  // unsigned saturating division by 1
        }
    }
    volatile int32_t dummy1 = sres;
    volatile uint32_t dummy2 = ures;
}

int main() {
    float arr[8];
    for (int i = 0; i < 8; ++i) {
        arr[i] = (i - 4) * 0.7f;
    }

    float sum = 0.0f;
    for (int j = 0; j < 100; ++j) {
        float a = arr[j % 8];
        float b = arr[(j + 2) % 8];
        float c = arr[(j + 4) % 8];
        if (j % 10 == 0) {
            sum += helper1(a, b, 5);
        } else if (j % 10 == 5) {
            sum += helper2(a, b, c);
        } else {
            sum += helper3(a, b, c, 3);
        }
    }

    int32_t s = -1000;
    uint32_t u = 2000;
    for (int k = 0; k < 50; ++k) {
        process_saturating(s + k, u + k, k % 4 + 1);
    }

    volatile float dummy = sum;
    return (int)(sum * 0.0f);
}
