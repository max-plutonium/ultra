#include <gmock/gmock.h>
#include <ffi.h>
#include <cstdio>

#include "benchmark.h"

class test_libffi : public testing::Test
{
public:
    MOCK_METHOD1(ffi_call_watch, void (void *));
};

using testing::Eq;
static test_libffi *test_libffi_instance;

static int test_libffi_func(void *ptr)
{
    test_libffi_instance->ffi_call_watch(ptr);
    return 1;
}

TEST_F(test_libffi, ffi_call)
{
    test_libffi_instance = this;

    const int argn = 1;
    ffi_cif cif;
    ffi_type *types[argn];
    void *values[argn];
    ffi_type *ret_type = 0;
    void *ret_value = 0;

    int r = 0;
    ret_type = &ffi_type_uint32;
    ret_value = &r;

    void *v = (void *) 0x45;
    types[0] = &ffi_type_pointer;
    values[0] = (void *) &v;

    EXPECT_CALL(*this, ffi_call_watch(Eq((void *) 0x45))).Times(1000000);

    assert(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argn, ret_type, types) == FFI_OK);

    benchmark("test_libffi: 1000000 calls", 1000000) {
        ffi_call(&cif, FFI_FN(test_libffi_func), ret_value, values);
        values[0] = (void *) &v;
    }

    EXPECT_EQ(1, r);

    test_libffi_instance = 0;
}
