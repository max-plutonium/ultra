#include "../../src/vm.h"
#include <gmock/gmock.h>

class test_vm : public ::testing::Test
{
protected:
};

TEST_F(test_vm, create)
{
    ultra::vm &vm = *ultra::vm::instance();
    vm.create_field(10, 10);

    for(auto &i : vm._inputs)
        *i << "1.5";

    auto pair = vm.create_interp(1, 1);
    auto pair2 = vm.create_child_vertex(pair, 2, 0.67);
    auto pair3 = vm.create_child_vertex(pair2, 3, 0.77);
    auto pair4 = vm.create_child_vertex(pair3, 4, 0.88);
    auto pair5 = vm.create_child_vertex(pair, 5);
    vm.dump(pair);
    vm.interpret();
    return;
}
