#include "../../src/vm.h"
#include <gmock/gmock.h>

TEST(test_vm, create)
{
    char *argv[] = { "vm", "--help", "--compression=25" };
    uvm_init(2, argv);
}
