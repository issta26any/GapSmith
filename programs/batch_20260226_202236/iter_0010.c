#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SAT_DIV(x, y) __ssdiv(x, y)
#define UNSAT_DIV(x, y) __usdiv(x, y)
#else
static int SAT_DIV(int x, int y) {
    if (x == INT32_MIN && y == -1) return INT32_MAX;
    return x / y;
}
static unsigned UNSAT_DIV(unsigned x, unsigned y) {
    if (y == 0) return UINT32_MAX;
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
    int counter = 0;
    while (counter < 5) {
        if (counter % 2 == 0) {
            acc += copysignf(fabsf(x), z);
        } else {
            acc += copysignf(y, fabsf(z));
        }
        counter++;
    }
    return acc;
}

float helper3(float p, float q, float r) {
    float tmp = 0.0f;
    for (int j = 0; j < 4; ++j) {
        if (j > 1) {
            tmp += copysignf(copysignf(p, q), r);
        } else {
            tmp += copysignf(p, copysignf(q, r));
        }
    }
    return tmp;
}

int main() {
    volatile int var1 = 100;
    volatile int var2 = -200;
    volatile unsigned var3 = 300;
    
    int sat_res = SAT_DIV(var1, 1);
    unsigned unsat_res = UNSAT_DIV(var3, 1);
    
    float a = 3.14f, b = -2.5f, c = 7.0f;
    float sum = 0.0f;
    
    for (int k = 0; k < 3; ++k) {
        if (k == 0) {
            sum += helper1(a, b, 2);
        } else if (k == 1) {
            sum += helper2(a, b, c);
        } else {
            sum += helper3(a, b, c);
        }
    }
    
    float final = copysignf(sum, -4.0f);
    final += copysignf(copysignf(a, b), -c);
    final += copysignf(a, copysignf(b, c));
    
    return (sat_res + unsat_res + (int)final) & 0xFF;
}
