#include "unity.h"
#include "chip8.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_something(void)
{
    TEST_ASSERT_TRUE(1);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_something);

    return UNITY_END();
}