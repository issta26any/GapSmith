#include <stdint.h>
#include <math.h>

#define SAT_DIV1(x) __builtin_ssub_overflow((x), 0, &(x)) ? (x) : (x)

static float test_copysign_identical(float a) {
    float res = copysignf(a, a);
    for (int i = 0; i < 3; ++i) {
        if (a > 0.0f) {
            res += copysignf(a, a);
        } else {
            res -= copysignf(a, a);
        }
    }
    return res;
}

static double test_copysign_const(double x) {
    double r1 = copysign(x, 2.0);
    double r2 = copysign(x, -3.0);
    double r3 = copysign(x, 0.0);
    if (x > 0.0) {
        return r1 + r2 + r3;
    } else {
        return r1 - r2 - r3;
    }
}

static float test_copysign_first_neg_abs(float y, float z) {
    float t1 = copysignf(-y, z);
    float t2 = copysignf(fabsf(y), z);
    for (int i = 0; i < 2; ++i) {
        if (z < 0.0f) {
            t1 = copysignf(-y, z);
        } else {
            t2 = copysignf(fabsf(y), z);
        }
    }
    return t1 + t2;
}

static double test_copysign_second_abs(double x, double y) {
    double r = copysign(x, fabs(y));
    if (y != 0.0) {
        for (int j = 0; j < 2; ++j) {
            r += copysign(x, fabs(y));
        }
    }
    return r;
}

static float test_copysign_nested_first(float a, float b, float c) {
    float inner = copysignf(a, b);
    float outer = copysignf(inner, c);
    if (a > b) {
        outer += copysignf(copysignf(a, b), c);
    }
    return outer;
}

static double test_copysign_nested_second(double a, double b, double c) {
    double inner = copysign(b, c);
    double outer = copysign(a, inner);
    for (int k = 0; k < 2; ++k) {
        if (b > c) {
            outer += copysign(a, copysign(b, c));
        }
    }
    return outer;
}

static int32_t test_satdiv_const1(int32_t v) {
    int32_t r = SAT_DIV1(v);
    int32_t sum = 0;
    for (int i = 0; i < 4; ++i) {
        if (v & 1) {
            sum += SAT_DIV1(v);
        } else {
            sum -= SAT_DIV1(v);
        }
        v += 1;
    }
    return r + sum;
}

static uint32_t test_usatdiv_const1(uint32_t v) {
    uint32_t r = v / 1;
    uint32_t acc = 0;
    while (v > 10) {
        acc += v / 1;
        v >>= 1;
    }
    return r + acc;
}

int main(void) {
    volatile float f1 = 5.0f, f2 = -2.0f, f3 = 3.0f;
    volatile double d1 = 7.0, d2 = -4.0, d3 = 1.5;
    volatile int32_t i1 = 100, i2 = -200;
    volatile uint32_t u1 = 300;

    float fr1 = test_copysign_identical(f1);
    double dr1 = test_copysign_const(d1);
    float fr2 = test_copysign_first_neg_abs(f2, f3);
    double dr2 = test_copysign_second_abs(d1, d2);
    float fr3 = test_copysign_nested_first(f1, f2, f3);
    double dr3 = test_copysign_nested_second(d1, d2, d3);
    int32_t ir1 = test_satdiv_const1(i1);
    int32_t ir2 = test_satdiv_const1(i2);
    uint32_t ur1 = test_usatdiv_const1(u1);

    return (fr1 + fr2 + fr3 + dr1 + dr2 + dr3 + ir1 + ir2 + ur1) != 0;
}
