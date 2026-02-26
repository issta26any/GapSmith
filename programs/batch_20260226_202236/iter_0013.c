#include <stdint.h>
#include <math.h>

#define SAT_DIV_1(x) __builtin_ssub_overflow((x), 0, &(int){0}) ? (x) : (x)  // Placeholder for saturating division by 1

float helper_copysign_identical(float a, int iter) {
    float result = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(a, a);
        } else if (i % 3 == 1) {
            result -= __builtin_copysignf(a, a);
        } else {
            result *= copysignf(a, a);
        }
    }
    return result;
}

float helper_copysign_const(float a, int iter) {
    float acc = a;
    for (int i = 0; i < iter; ++i) {
        if (i % 4 == 0) {
            acc = copysignf(acc, 2.0f);
        } else if (i % 4 == 1) {
            acc = __builtin_copysignf(acc, -3.0);
        } else if (i % 4 == 2) {
            acc = copysignf(acc, 0.0f);
        } else {
            acc = __builtin_copysignf(acc, -0.0f);
        }
    }
    return acc;
}

float helper_copysign_first_neg_abs(float x, float y, int iter) {
    float t = x;
    for (int i = 0; i < iter; ++i) {
        if (i % 2 == 0) {
            t = copysignf(-y, t);
        } else {
            t = __builtin_copysignf(fabsf(y), t);
        }
    }
    return t;
}

float helper_copysign_second_abs(float a, float b, int iter) {
    float r = a;
    int i = 0;
    while (i < iter) {
        if (i % 3 == 0) {
            r = copysignf(r, fabsf(b));
        } else if (i % 3 == 1) {
            r = __builtin_copysignf(r, fabsf(b + 1.0f));
        } else {
            r = copysignf(r, fabsf(b - 1.0f));
        }
        ++i;
    }
    return r;
}

float helper_copysign_nested_first(float a, float b, float c, int iter) {
    float val = a;
    for (int i = 0; i < iter; ++i) {
        if (i % 2 == 0) {
            val = copysignf(copysignf(val, b), c);
        } else {
            val = __builtin_copysignf(__builtin_copysignf(val, b + 1.0f), c);
        }
    }
    return val;
}

float helper_copysign_nested_second(float a, float b, float c, int iter) {
    float val = a;
    int i = 0;
    do {
        if (i % 2 == 0) {
            val = copysignf(val, copysignf(b, c));
        } else {
            val = __builtin_copysignf(val, __builtin_copysignf(b, c + 1.0f));
        }
        ++i;
    } while (i < iter);
    return val;
}

int helper_satdiv_const1(int x, unsigned u, int iter) {
    int sacc = x;
    unsigned uacc = u;
    for (int i = 0; i < iter; ++i) {
        if (i % 5 == 0) {
            sacc += SAT_DIV_1(sacc);
        } else if (i % 5 == 1) {
            sacc -= SAT_DIV_1(sacc);
        } else if (i % 5 == 2) {
            uacc += (unsigned)SAT_DIV_1((int)uacc);
        } else if (i % 5 == 3) {
            sacc ^= SAT_DIV_1(sacc);
        } else {
            sacc = SAT_DIV_1(sacc) | 1;
        }
    }
    return sacc + (int)uacc;
}

int main(void) {
    volatile float f1 = 3.14f;
    volatile float f2 = -2.5f;
    volatile float f3 = 0.0f;
    volatile int n = 7;
    volatile unsigned un = 42;

    float r1 = helper_copysign_identical(f1, n);
    float r2 = helper_copysign_const(f2, n);
    float r3 = helper_copysign_first_neg_abs(f1, f2, n);
    float r4 = helper_copysign_second_abs(f1, f2, n);
    float r5 = helper_copysign_nested_first(f1, f2, f3, n);
    float r6 = helper_copysign_nested_second(f1, f2, f3, n);
    int r7 = helper_satdiv_const1((int)f1, un, n);

    return ((r1 + r2 + r3 + r4 + r5 + r6) > 0.0f) ? r7 : 0;
}
