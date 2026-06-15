#ifndef ASCII_FUN_H_
#define ASCII_FUN_H_

#include <stdint.h>


void hexToASCII(const uint8_t *data, uint8_t size, uint8_t *str) {
    uint8_t chr;
    for(int i = 0; i < size; i ++) {
        chr = data[i] >> 4;
        if(chr < 10)
            str[i * 2] = '0' + chr;  // 0 - 9
        else
            str[i * 2] = 'A' + chr - 10;  // A - F
        chr = data[i] & 0x0F;
        if(chr < 10)
            str[i * 2 + 1] = '0' + chr;
        else
            str[i * 2 + 1] = 'A' + chr - 10;
    }
    return;
}
void intToASCII(int val, uint8_t *str, uint8_t len) {
    int sign = 0;
    int i = len - 1;
    if(val == 0) {
        str[len - 1] = '0';
        i --;
    }
    if(val < 0) {
        sign = 1;
        val *= -1;
    }
    for(; i >= 0; i --) {
        if(val + sign > 0) {
            if(val == 0) {
                str[i] = '-';
                sign = 0;
            }
            else {
                str[i] = '0' + (val % 10);
            }
            val /= 10;
        }
        else {
            str[i] = ' ';
        }
    }
    return;
}
int ASCIIToInt(const uint8_t *str, uint8_t len) {
    int res = 0, sign = 1, tmp;
    for(int i = 0; i < len; i ++) {
        if(str[i] == '-') {
            sign = -1;
            tmp = 0;
        }
        else if(str[i] == ' ') {
            tmp = 0;
        }
        else if(str[i] < '0' || str[i] > '9') {
            return res * sign;
        }
        else {
            tmp = str[i] - '0';
        }
        res = res * 10 + tmp;
    }
    return res * sign;
}

bool str_eq(const uint8_t *str1, const uint8_t *str2, uint8_t len) {
    for(uint8_t i = 0; i < len; i ++) {
        if(str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}
#endif