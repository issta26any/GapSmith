#include <stdint.h>
#include <math.h>

#define SAT_DIV1(x) __builtin_ssub_overflow((x), 0, &(x)) ? (x) : (x)

float test_copysign_identical(float a) {
    float result = copysignf(a, a);
    for (int i = 0; i < 3; ++i) {
        if (a > 0.0f) {
            result += copysignf(a, a);
        } else {
            result -= copysignf(a, a);
        }
    }
    return result;
}

float test_copysign_const_second(float b) {
    float r1 = copysignf(b, 2.0f);
    float r2 = copysignf(b, -3.0f);
    float sum = r1;
    int count = 0;
    while (count < 2) {
        sum += r2;
        ++count;
    }
    return sum;
}

float test_copysign_first_neg_abs(float c, float d) {
    float t1 = copysignf(-c, d);
    float t2 = copysignf(fabsf(c), d);
    if (c != 0.0f) {
        t1 *= 2.0f;
        t2 /= 2.0f;
    }
    return t1 + t2;
}

float test_copysign_second_abs(float e, float f) {
    float res = copysignf(e, fabsf(f));
    for (int j = 0; j < 4; ++j) {
        if (j % 2 == 0) {
            res = copysignf(res, fabsf(f));
        }
    }
    return res;
}

float test_copysign_nested_first(float g, float h, float i) {
    float inner = copysignf(g, h);
    float outer = copysignf(inner, i);
    if (g > h) {
        outer = copysignf(copysignf(g, h), i);
    }
    return outer;
}

float test_copysign_nested_second(float j, float k, float l) {
    float inner = copysignf(k, l);
    float outer = copysignf(j, inner);
    if (k < l) {
        outer = copysignf(j, copysignf(k, l));
    }
    return outer;
}

int test_satdiv_const1(int m) {
    int sat1 = SAT_DIV1(m);
    int acc = sat1;
    for (int n = 0; n < 5; ++n) {
        if (n % 2 == 0) {
            acc += SAT_DIV1(m + n);
        } else {
            acc -= SAT_DIV1(m - n);
        }
    }
    return acc;
}

unsigned test_usatdiv_const1(unsigned p) {
    unsigned sat2 = (p / 1u);
    unsigned total = sat2;
    int q = 0;
    do {
        total += (p + q) / 1u;
        ++q;
    } while (q < 3);
    return total;
}

int main() {
    volatile float a = 3.14f;
    volatile float b = -2.5f;
    volatile float c = 4.0f;
    volatile float d = -1.0f;
    volatile float e = 0.0f;
    volatile float f = 5.0f;
    volatile float g = -3.0f;
    volatile float h = 1.0f;
    volatile float i = -4.0f;
    volatile float j = 2.0f;
    volatile float k = -5.0f;
    volatile float l = 6.0f;
    
    float r1 = test_copysign_identical(a);
    float r2 = test_copysign_const_second(b);
    float r3 = test_copysign_first_neg_abs(c, d);
    float r4 = test_copysign_second_abs(e, f);
    float r5 = test_copysign_nested_first(g, h, i);
    float r6 = test_copysign_nested_second(j, k, l);
    
    volatile int m = 100;
    volatile unsigned p = 200u;
    
    int r7 = test_satdiv_const1(m);
    unsigned r8 = test_usatdiv_const1(p);
    
    return (int)(r1 + r2 + r3 + r4 + r5 + r6) + r7 + (int)r8;
}
