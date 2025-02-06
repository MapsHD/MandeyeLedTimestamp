#include <unity.h>
#include "gray.cpp"

void test_gray_conversion() {
    for (uint32_t i = 0; i < 16; ++i) {
        uint32_t gray = toGrayCode(i);
        uint32_t original = fromGrayCode(gray);
        TEST_ASSERT_EQUAL_UINT32(i, original);
    }
}

void test_edge_cases() {
    TEST_ASSERT_EQUAL_UINT32(toGrayCode(0), 0);
    TEST_ASSERT_EQUAL_UINT32(toGrayCode(1), 1);
    TEST_ASSERT_EQUAL_UINT32(toGrayCode(2), 3);
    TEST_ASSERT_EQUAL_UINT32(fromGrayCode(0), 0);
    TEST_ASSERT_EQUAL_UINT32(fromGrayCode(1), 1);
    TEST_ASSERT_EQUAL_UINT32(fromGrayCode(3), 2);
}

void RUN_UNITY_TESTS() {
    UNITY_BEGIN();
    RUN_TEST(test_gray_conversion);
    RUN_TEST(test_edge_cases);
    UNITY_END();
}


#include <Arduino.h>
void setup() {
    Serial.begin(115200);
    RUN_UNITY_TESTS();
}

void loop() {
}

