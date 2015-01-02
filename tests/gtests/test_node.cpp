#include "../../src/vm.h"
#include <gmock/gmock.h>
#include "benchmark.h"

class mock_node : public ultra::node
{
public:
    mock_node(ultra::address a) : ultra::node(a) { }
    MOCK_METHOD1(message, void (ultra::scalar_message_ptr));
};

using testing::InSequence;
using testing::_;
using testing::Eq;
using testing::AtLeast;

MATCHER_P5(ScalarMessage, type, sender, receiver, time, data, "Message") {
    return ultra::scalar_message(type, sender, receiver, time, data) == *arg;
}

TEST(test_node, test)
{
    using namespace ultra;

    ultra::vm vm(0, nullptr);
    node_ptr n1 = std::make_shared<mock_node>(address(1, 2, 3));
    node_ptr n2 = std::make_shared<mock_node>(address(2, 3, 4));
    vm.register_node(n1);
    vm.register_node(n2);

    EXPECT_EQ(address(1, 2, 3), n1->get_address());
    EXPECT_EQ(address(2, 3, 4), n2->get_address());
    EXPECT_EQ(std::size_t(0), n1->time().time());
    EXPECT_EQ(std::size_t(0), n2->time().time());

    InSequence seq;
    EXPECT_CALL(*std::static_pointer_cast<mock_node>(n2),
                message(_)).Times(1);

    EXPECT_CALL(*std::static_pointer_cast<mock_node>(n2),
                message(ScalarMessage(
                            scalar_message::unknown,
                            address(1, 2, 3),
                            address(2, 3, 4),
                            ultra::scalar_time(2),
                            std::string(""))
                        )).Times(1);

    EXPECT_CALL(*std::static_pointer_cast<mock_node>(n2),
                message(_)).Times(1);

    EXPECT_TRUE(n1->connect(n2));

    n1->post_message(scalar_message::unknown, address(2, 3, 4));
}
