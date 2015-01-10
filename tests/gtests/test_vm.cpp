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
                           "--num-network-threads=2",
                           "--num-reactors=1",
                           "--address=127.0.0.1",
                           "--port=55777",
                           "--cluster=0"};
    ultra::vm vm(7, argv);

    EXPECT_EQ(&vm, ultra::vm::instance());
    return;
}
