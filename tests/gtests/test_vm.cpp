#include "../../src/vm.h"
#include <gmock/gmock.h>

TEST(test_vm, create)
{
    char *argv[] = { "vm", "--help", "--compression=25" };
    auto vm = ultra::vm::create_vm(2, argv);
}
