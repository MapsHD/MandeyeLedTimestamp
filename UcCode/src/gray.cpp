#include "gray.hpp"

uint32_t toGrayCode(uint32_t num) {
    return num ^ (num >> 1);
}

uint32_t fromGrayCode(uint32_t gray) {
    uint32_t num = 0;
    for (; gray; gray >>= 1) {
        num ^= gray;
    }
    return num;
}