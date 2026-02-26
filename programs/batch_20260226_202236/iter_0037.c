#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SAT_DIV(x, y) __ssdiv(x, y)
#define UNSAT_DIV(x, y) __usdiv(x, y)
#else
static int SAT_DIV(int x, int y) {
    if (y == 0) return 0;
    if (x == -2147483648 && y == -1) return 2147483647;
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
            result += copysignf(a, a);
        } else if (i % 3 == 1) {
            result += copysignf(b, 2.0f);
        } else {
            result += copysignf(-a, b);
        }
    }
    return result;
}

float helper2(float x, float y, float z) {
    float acc = 0.0f;
    for (int j = 0; j < 5; ++j) {
        if (j % 2 == 0) {
            acc += copysignf(fabsf(x), z);
        } else {
            acc += copysignf(y, fabsf(z));
        }
    }
    return acc;
}

float helper3(float p, float q, float r) {
    float tmp = 0.0f;
    for (int k = 0; k < 3; ++k) {
        if (k == 0) {
            tmp += copysignf(copysignf(p, q), r);
        } else if (k == 1) {
            tmp += copysignf(p, copysignf(q, r));
        } else {
            tmp += copysignf(-p, -q);
        }
    }
    return tmp;
}

int main() {
    volatile int sat_var = 100;
    volatile unsigned unsat_var = 200;
    
    int sat_res = SAT_DIV(sat_var, 1);
    unsigned unsat_res = UNSAT_DIV(unsat_var, 1);
    
    float a = 3.14f, b = -2.71f, c = 0.0f;
    float sum = 0.0f;
    
    for (int outer = 0; outer < 4; ++outer) {
        if (outer % 2 == 0) {
            sum += helper1(a, b, outer + 1);
        } else {
            sum += helper2(a, b, c);
        }
        if (outer > 1) {
            sum += helper3(a, b, c);
        }
    }
    
    float final = copysignf(sum, -3.0);
    final += copysignf(copysignf(a, b), copysignf(b, a));
    
    return (int)final + sat_res + (int)unsat_res;
}
