#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
    #define __ssdiv(x, y) __builtin_arm_ssdiv((x), (y))
    #define __usdiv(x, y) __builtin_arm_usdiv((x), (y))
#else
    static int __ssdiv(int x, int y) {
        long long result = (long long)x / y;
        if (result > 2147483647) return 2147483647;
        if (result < -2147483648) return -2147483648;
        return (int)result;
    }
    static unsigned __usdiv(unsigned x, unsigned y) {
        unsigned long long result = (unsigned long long)x / y;
        if (result > 4294967295U) return 4294967295U;
        return (unsigned)result;
    }
#endif

float helper_copysign_identical(float a, int iter) {
    float res = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            res += copysignf(a, a);
        } else if (i % 3 == 1) {
            res -= __builtin_copysignf(a, a);
        } else {
            res *= copysignf(a, a);
        }
    }
    return res;
}

double helper_copysign_const(double x, int n) {
    double acc = 0.0;
    while (n-- > 0) {
        if (n % 2 == 0) {
            acc += copysign(x, 2.0);
        } else {
            acc += copysign(x, -3.0);
        }
    }
    return acc;
}

float helper_copysign_first_neg_abs(float base, float sgn, int flag) {
    float t = 0.0f;
    for (int j = 0; j < 4; ++j) {
        if (flag & (1 << j)) {
            t += copysignf(-base, sgn);
        } else {
            t += copysignf(fabsf(base), sgn);
        }
    }
    return t;
}

double helper_copysign_second_abs(double val, double mag) {
    double sum = 0.0;
    int k = 0;
    do {
        sum += copysign(val, fabs(mag + (double)k));
        k++;
    } while (k < 3);
    return sum;
}

float helper_copysign_nested_first(float a, float b, float c) {
    float tmp = copysignf(copysignf(a, b), c);
    if (tmp > 0.0f) {
        return tmp + copysignf(copysignf(b, a), c);
    }
    return tmp;
}

double helper_copysign_nested_second(double p, double q, double r) {
    double out = 0.0;
    for (int i = 0; i < 2; ++i) {
        out += copysign(p, copysign(q, r));
        p += 1.0;
    }
    return out;
}

int main() {
    volatile int sat_arg = 1000;
    int sat1 = __ssdiv(sat_arg, 1);
    unsigned sat2 = __usdiv(sat_arg, 1);

    float f1 = helper_copysign_identical(5.0f, 4);
    double d1 = helper_copysign_const(7.0, 3);

    float f2 = helper_copysign_first_neg_abs(9.0f, -2.0f, 5);
    double d2 = helper_copysign_second_abs(4.0, -6.0);

    float f3 = helper_copysign_nested_first(1.5f, -2.5f, 3.5f);
    double d3 = helper_copysign_nested_second(1.1, 2.2, 3.3);

    int result = (int)(f1 + d1 + f2 + d2 + f3 + d3) + sat1 + (int)sat2;
    return result % 256;
}
