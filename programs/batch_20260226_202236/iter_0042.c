#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SS_DIV(x, y) __builtin_arm_ssdiv(x, y)
#define US_DIV(x, y) __builtin_arm_usdiv(x, y)
#else
#define SS_DIV(x, y) ((x) / (y))
#define US_DIV(x, y) ((x) / (y))
#endif

static float helper1(float a, float b, int cond) {
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

static double helper2(double x, double y, int iter) {
    double acc = x;
    while (iter-- > 0) {
        if (acc > 0.0) {
            acc = copysign(acc, y);
        } else {
            acc = copysign(y, acc);
        }
    }
    return acc;
}

static float helper3(float p, float q) {
    if (p == q) {
        return copysignf(p, p);
    }
    if (p > 0.0f) {
        return copysignf(-p, q);
    } else {
        return copysignf(fabsf(p), q);
    }
}

static double helper4(double a, double b, double c) {
    double t1 = copysign(copysign(a, b), c);
    double t2 = copysign(a, copysign(b, c));
    if (t1 == t2) {
        return copysign(a, 2.5);
    } else {
        return copysign(b, -3.75);
    }
}

static int32_t sat_div_test(int32_t val, unsigned count) {
    int32_t sum = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (i % 3 == 0) {
            sum += SS_DIV(val + i, 1);
        } else if (i % 3 == 1) {
            sum += SS_DIV(val - i, 1);
        } else {
            sum += SS_DIV(val * i, 1);
        }
    }
    return sum;
}

static uint32_t usat_div_test(uint32_t val, unsigned count) {
    uint32_t sum = 0;
    for (unsigned i = 0; i < count; ++i) {
        if (i % 2 == 0) {
            sum += US_DIV(val + i, 1);
        } else {
            sum += US_DIV(val * i, 1);
        }
    }
    return sum;
}

int main(void) {
    float f1 = 3.14f, f2 = -2.71f, f3 = 0.0f;
    double d1 = 1.414, d2 = -1.732, d3 = 0.0;

    float r1 = helper1(f1, f2, 5);
    double r2 = helper2(d1, d2, 3);
    float r3 = helper3(f2, f1);
    double r4 = helper4(d1, d2, d3);

    int32_t s1 = sat_div_test(100, 10);
    uint32_t s2 = usat_div_test(200, 10);

    volatile float vf = r1 + r3;
    volatile double vd = r2 + r4;
    volatile int32_t vi = s1;
    volatile uint32_t vu = s2;

    return (vf > 0.0f && vd < 0.0 && vi != 0 && vu != 0) ? 0 : 1;
}
