#include <iostream>
#include <gmock/gmock.h>

int main(int argc, char **argv)
{
    std::printf("ultra gtest\n");
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
