#include "../../src/vm.h"
#include <gmock/gmock.h>

#include "../../src/ultra.h"
#include "../../src/message.h"

TEST(test_vm, create)
{
    using namespace ultra;

    const char *argv[] = { "vm", "--num-threads=2", "--address=127.0.0.1", "--port=2200" };
    ultra::vm vm(0, argv);

    return;
}
