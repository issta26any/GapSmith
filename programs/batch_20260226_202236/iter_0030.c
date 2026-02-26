#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SAT_DIV(x, y) __ssdiv(x, y)
#define USAT_DIV(x, y) __usdiv(x, y)
#else
#define SAT_DIV(x, y) ((x) / (y))
#define USAT_DIV(x, y) ((x) / (y))
#endif

float helper1(float a, float b, int cond) {
    float result = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (cond & (1 << i)) {
            result += copysignf(a, b);
        } else {
            result -= copysignf(b, a);
        }
    }
    return result;
}

float helper2(float x, float y, int iter) {
    float acc = x;
    while (iter-- > 0) {
        if (acc > 0.0f) {
            acc = copysignf(acc, y);
        } else {
            acc = copysignf(y, acc);
        }
    }
    return acc;
}

float helper3(float a, float b, float c) {
    if (a == b) {
        return copysignf(copysignf(a, b), c);
    } else if (b == c) {
        return copysignf(a, copysignf(b, c));
    } else {
        return copysignf(-a, fabsf(b));
    }
}

float helper4(float base, int count) {
    float val = base;
    for (int i = 0; i < count; ++i) {
        if (i % 3 == 0) {
            val = copysignf(val, 2.0f);
        } else if (i % 3 == 1) {
            val = copysignf(fabsf(val), -3.0f);
        } else {
            val = copysignf(-val, val);
        }
    }
    return val;
}

int sat_ops(int x, unsigned y) {
    int s1 = SAT_DIV(x, 1);
    unsigned u1 = USAT_DIV(y, 1U);
    int s2 = SAT_DIV(s1, 1);
    unsigned u2 = USAT_DIV(u1, 1U);
    return (s1 + s2) * (int)(u1 + u2);
}

int main(void) {
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile float f3 = 0.0f;
    volatile int n = 5;
    volatile unsigned m = 10;

    float r1 = helper1(f1, f2, n);
    float r2 = helper2(f2, f3, 3);
    float r3 = helper3(f1, f2, f3);
    float r4 = helper4(f1, 4);
    int r5 = sat_ops(n, m);

    float final = r1 + r2 + r3 + r4 + (float)r5;
    if (final > 100.0f) {
        return 1;
    }
    return 0;
}
