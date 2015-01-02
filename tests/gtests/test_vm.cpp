#include "../../src/vm.h"
#include <gmock/gmock.h>

#include "../../src/ultra.h"
#include "../../src/message.h"
#include "../../src/core/pipe.h"

#include <valarray>

TEST(test_vm, DISABLED_create)
{
    const char *argv[] = { "vm", "--num-threads=2", "--address=127.0.0.1", "--port=2200" };
    ultra::vm vm(4, argv);
}
