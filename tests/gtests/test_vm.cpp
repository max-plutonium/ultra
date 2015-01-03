#include "../../src/vm.h"
#include <gmock/gmock.h>

#include "../../src/ultra.h"
#include "../../src/message.h"

TEST(test_vm, create)
{
    using namespace ultra;

    const char *argv[] = { "vm", "--num-threads=2", "--address=127.0.0.1", "--port=2200" };
    ultra::vm vm(0, argv);

    auto p1 = std::make_shared<port>(address(1, 2, 3), openmode::inout);
    auto p2 = std::make_shared<port>(address(2, 3, 4), openmode::inout);
    vm.register_node(p1);
    vm.register_node(p2);
    *p1 << "1" << "2" << "3";
    p1->connect(p2);
    p1->disconnect(p2);
    p1->post_message(scalar_message::unknown, address(2, 3, 4), "123");

//    sleep(3);
    std::string ss;
    *p2 >> ss;
while(true);
    return;
}
