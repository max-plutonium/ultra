#include <iostream>
#include <gmock/gmock.h>

#include "../../src/vm.h"

int main(int argc, const char **argv)
{
    std::printf("ultra gtest\n");
    testing::InitGoogleMock(&argc, const_cast<char **>(argv));
    ultra::vm vm(argc, argv);
    return RUN_ALL_TESTS();
}
