#include <math.h>
#include <stdint.h>

#define SAT_DIV1(x) __builtin_ssdiv(x, 1)

float test_copysign_identical(float a) {
    return copysignf(a, a);
}

float test_copysign_const_second(float a) {
    if (a > 0) {
        return copysignf(a, 2.5f);
    } else {
        return copysignf(a, -3.75f);
    }
}

float test_copysign_first_neg(float b, float c) {
    for (int i = 0; i < 3; ++i) {
        if (b > c) {
            return copysignf(-b, c);
        }
        b += 1.0f;
    }
    return copysignf(-b, c);
}

float test_copysign_first_abs(float d, float e) {
    float result = 0.0f;
    while (d < 10.0f) {
        result += copysignf(fabsf(d), e);
        d += 2.0f;
    }
    return result;
}

float test_copysign_second_abs(float f, float g) {
    if (f != g) {
        return copysignf(f, fabsf(g));
    }
    return copysignf(f, fabsf(g + 1.0f));
}

float test_copysign_nested_first(float h, float i, float j) {
    float temp = copysignf(h, i);
    return copysignf(temp, j);
}

float test_copysign_nested_second(float k, float l, float m) {
    if (k > 0 && l > 0) {
        return copysignf(k, copysignf(l, m));
    }
    return copysignf(k, copysignf(l + 1.0f, m));
}

int main() {
    volatile int sat_arg = 100;
    int sat_res = SAT_DIV1(sat_arg);
    
    float arr[4] = {1.0f, -2.0f, 3.5f, -4.25f};
    float sum = 0.0f;
    
    for (int idx = 0; idx < 4; ++idx) {
        sum += test_copysign_identical(arr[idx]);
        sum += test_copysign_const_second(arr[idx]);
        
        if (idx % 2 == 0) {
            sum += test_copysign_first_neg(arr[idx], arr[(idx+1)%4]);
            sum += test_copysign_first_abs(arr[idx], arr[(idx+2)%4]);
        } else {
            sum += test_copysign_second_abs(arr[idx], arr[(idx+3)%4]);
        }
        
        sum += test_copysign_nested_first(arr[idx], arr[(idx+1)%4], arr[(idx+2)%4]);
        sum += test_copysign_nested_second(arr[idx], arr[(idx+2)%4], arr[(idx+3)%4]);
    }
    
    return (int)sum + sat_res;
}
