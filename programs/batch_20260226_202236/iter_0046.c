#include <stdint.h>
#include <math.h>

#define SAT_DIV1(x) __builtin_ssub_overflow((x), 0, &(int){0}) ? (x) : (x)  // Placeholder for saturating division by 1

float helper1(float a, float b, int cond) {
    float result = 0.0f;
    for (int i = 0; i < 3; ++i) {
        if (cond & (1 << i)) {
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
        if (z > 0.0f) {
            acc += copysignf(-x, y);  // First operand is NEG
        } else {
            acc += copysignf(fabsf(x), y);  // First operand is ABS
        }
        z = -z;
        ++counter;
    }
    return acc;
}

float helper3(float p, float q, float r) {
    float tmp = 0.0f;
    for (int j = 0; j < 2; ++j) {
        if (p != q) {
            tmp += copysignf(p, fabsf(q));  // Second operand is ABS
        } else {
            tmp += copysignf(copysignf(p, r), q);  // First operand is COPYSIGN, second operand r has no side effects
        }
        p += 1.0f;
    }
    return tmp;
}

float helper4(float u, float v, float w) {
    float val = 0.0f;
    if (u < v) {
        val = copysignf(u, copysignf(v, w));  // Second operand is COPYSIGN, first operand v has no side effects
    } else if (u > v) {
        val = copysignf(-u, copysignf(v, w));
    }
    return val;
}

int main() {
    volatile int sat_arg = 1000;
    int sat_result = SAT_DIV1(sat_arg);  // Should trigger saturating division by 1 simplification

    float a = 3.14f, b = -2.71f, c = 1.414f;
    float res1 = helper1(a, b, 5);
    float res2 = helper2(a, b, c);
    float res3 = helper3(b, c, a);
    float res4 = helper4(c, a, b);

    // Use results to avoid dead code elimination
    volatile float sink = res1 + res2 + res3 + res4 + (float)sat_result;
    return sink > 0.0f ? 0 : 1;
}
