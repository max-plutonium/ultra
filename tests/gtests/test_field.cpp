#include "../../src/field.h"
#include <gmock/gmock.h>

#include "../../src/vm.h"

class test_field : public ::testing::Test
{
protected:
};

#include "../../src/message.h"

#include <boost/coroutine/asymmetric_coroutine.hpp>

TEST_F(test_field, create)
{
    using namespace ultra;

    const char *argv[] = { "vm",
                           "--num-threads=2",
                           "--address=127.0.0.1",
                           "--port=55888",
                           "--cluster=1"};
    ultra::vm vm(5, argv);


    ultra::field fld(1);
}
