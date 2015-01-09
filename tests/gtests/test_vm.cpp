#include "../../src/vm.h"
#include <gmock/gmock.h>

class test_vm : public ::testing::Test
{
protected:
};

TEST_F(test_vm, create)
{
    using namespace ultra;

    const char *argv[] = { "vm",
                           "--num-threads=2",
                           "--address=127.0.0.1",
                           "--port=55999",
                           "--cluster=1"};
    ultra::vm vm(5, argv);

    EXPECT_EQ(&vm, ultra::vm::instance());
    return;
}
