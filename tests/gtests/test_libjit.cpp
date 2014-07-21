#include <gmock/gmock.h>
#include <jit/jit.h>
#include <jit/jit-dump.h>

#include "benchmark.h"

static int gcd_native(int u, int v)
{
    int t;
    while (v) {
        t = u;
        u = v;
        v = t % v;
    }
    return u < 0 ? -u : u; /* abs(u) */
}

// Returns an uncompiled jit_function_t.
static jit_function_t build_gcd_func(jit_context_t context)
{
    jit_context_build_start(context);

    // Create function signature and object. int (*)(int, int)
    jit_type_t params[2] = {jit_type_int, jit_type_int};
    jit_type_t signature = jit_type_create_signature(
                               jit_abi_cdecl, jit_type_int, params, 2, 1);
    jit_function_t F = jit_function_create(context, signature);

    // u, v are function parameters; t is a temporary value.
    jit_value_t u, v, t;
    u = jit_value_get_param(F, 0);
    v = jit_value_get_param(F, 1);
    t = jit_value_create(F, jit_type_int);

    // Create the while (v) condition with a label that allows to loop back.
    //
    // label_while:
    //   if (v == 0) goto label_after_while
    //   .. contents of while loop
    //
    // label_after_while is created as undefined at this point, so that
    // instructions can have forward references to it. It will be placed later.
    jit_label_t label_while = jit_label_undefined;
    jit_label_t label_after_while = jit_label_undefined;
    jit_value_t const0 = jit_value_create_nint_constant(F, jit_type_int, 0);

    jit_insn_label(F, &label_while);
    jit_value_t cmp_v_0 = jit_insn_eq(F, v, const0);
    jit_insn_branch_if(F, cmp_v_0, &label_after_while);

    // t = u
    jit_insn_store(F, t, u);
    // u = v
    jit_insn_store(F, u, v);

    // v = t % v
    jit_value_t rem = jit_insn_rem(F, t, v);
    jit_insn_store(F, v, rem);

    //   goto label_while
    // label_after_while:
    //   ...
    jit_insn_branch(F, &label_while);
    jit_insn_label(F, &label_after_while);

    //   if (u >= 0) goto label_positive
    //   return -u
    // label_pos:
    //   return u
    jit_label_t label_positive = jit_label_undefined;
    jit_value_t cmp_u_0 = jit_insn_ge(F, u, const0);
    jit_insn_branch_if(F, cmp_u_0, &label_positive);

    jit_value_t minus_u = jit_insn_neg(F, u);
    jit_insn_return(F, minus_u);
    jit_insn_label(F, &label_positive);
    jit_insn_return(F, u);

    jit_context_build_end(context);
    return F;
}

TEST(test_libjit, gcd)
{
    jit_init();
    jit_context_t context = jit_context_create();
    jit_function_t gcd = build_gcd_func(context);

    // This will dump the uncompiled function, showing libjit opcodes
    jit_dump_function(stdout, gcd, "gcd [uncompiled]");

    printf("Optimization level: %u\n", jit_function_get_optimization_level(gcd));

    // Compile (JIT) the function to machine code
    jit_context_build_start(context);
    jit_function_compile(gcd);
    jit_context_build_end(context);

    // This will dump the disassembly of the machine code for the function
    jit_dump_function(stdout, gcd, "gcd [compiled]");

    // Run the function on argv input
    int u = 257458;
    int v = 127199;
    void* args[2] = { &u, &v };

    jit_int jit_result;

    benchmark("test_libjit: 1000000 jit_function_apply's", 1000000) {
        jit_function_apply(gcd, args, &jit_result);
    }

    benchmark("test_libjit: 1000000 native calls", 1000000) {
        jit_result = gcd_native(u, v);
    }

    //    typedef int (*FF)(int, int);
    //    FF gcd_f = (FF) jit_function_to_closure(gcd);
    //    int result = 0;

    //    benchmark("test_libjit: 1000000 direct calls", 1000000) {
    //        result = gcd_f(u, v);
    //    }

    jit_context_destroy(context);
}

// Builds this function, and returns an uncompiled jit_function_t:
//
// int jit_adder(int x, y) {
//    return x + y;
// }
static jit_function_t build_jit_adder(jit_context_t context)
{
    jit_context_build_start(context);

    // Create function signature and object. int (*)(int, int)
    jit_type_t params[2] = {jit_type_int, jit_type_int};
    jit_type_t signature = jit_type_create_signature(
                               jit_abi_cdecl, jit_type_int, params, 2, 1);
    jit_function_t F = jit_function_create(context, signature);

    // x, y are the parameters; sum is a temporary
    jit_value_t x = jit_value_get_param(F, 0);
    jit_value_t y = jit_value_get_param(F, 1);
    jit_value_t sum = jit_value_create(F, jit_type_int);

    // sum = x + y
    jit_value_t temp_sum = jit_insn_add(F, x, y);
    jit_insn_store(F, sum, temp_sum);

    // return sum
    jit_insn_return(F, sum);
    jit_context_build_end(context);
    return F;
}

static int native_mult(int a, int b)
{
    return a * b;
}

static jit_function_t build_native_caller(jit_context_t context)
{
    jit_context_build_start(context);

    jit_type_t params[] = {jit_type_int, jit_type_int};
    jit_type_t signature = jit_type_create_signature(
                               jit_abi_cdecl, jit_type_int, params, 2, 1);
    jit_function_t F = jit_function_create(context, signature);

    jit_value_t x = jit_value_get_param(F, 0);
    jit_value_t y = jit_value_get_param(F, 1);

    jit_type_t mult_params[] = {jit_type_int, jit_type_int};
    jit_type_t mult_signature = jit_type_create_signature(
                        jit_abi_cdecl, jit_type_int, mult_params, 2, 1);

    jit_value_t mult_args[] = {x, y};
    jit_value_t res = jit_insn_call_native(
                  F, "native_mult", (void *) native_mult, mult_signature,
                  mult_args, sizeof(mult_args) / sizeof(jit_value_t),
                  JIT_CALL_NOTHROW);

    jit_insn_return(F, res);
    jit_context_build_end(context);
    return F;
}

static jit_function_t build_interp_caller(
        jit_context_t context, jit_function_t interp_fun)
{
    jit_context_build_start(context);

    jit_type_t params[] = {jit_type_int, jit_type_int};
    jit_type_t signature = jit_type_create_signature(
                               jit_abi_cdecl, jit_type_int, params, 2, 1);
    jit_function_t F = jit_function_create(context, signature);

    jit_value_t x = jit_value_get_param(F, 0);
    jit_value_t y = jit_value_get_param(F, 1);

    jit_value_t adder_args[] = {x, y};
    jit_value_t res = jit_insn_call(F, "interp_function",
                            interp_fun, 0, adder_args, 2, 0);

    jit_insn_return(F, res);
    jit_context_build_end(context);
    return F;
}

TEST(test_libjit, invokations)
{
    jit_init();
    jit_context_t context = jit_context_create();
    jit_function_t jit_adder = build_jit_adder(context);
    jit_function_t native_caller = build_native_caller(context);
    jit_function_t interp_caller = build_interp_caller(context, jit_adder);

    // This will dump the uncompiled function, showing libjit opcodes
    jit_dump_function(stdout, jit_adder, "jit_adder [uncompiled]");
    jit_dump_function(stdout, native_caller, "native_caller [uncompiled]");
    jit_dump_function(stdout, interp_caller, "interp_caller [uncompiled]");

    // Compile (JIT) the functions to machine code
    jit_context_build_start(context);
    jit_function_compile(jit_adder);
    jit_function_compile(native_caller);
    jit_function_compile(interp_caller);
    jit_context_build_end(context);

    // This will dump the disassembly of the machine code for the function
    jit_dump_function(stdout, jit_adder, "jit_adder [compiled]");
    jit_dump_function(stdout, native_caller, "native_caller [compiled]");
    jit_dump_function(stdout, interp_caller, "interp_caller [compiled]");

    // Run the function on argv input
    int u = 2;
    int v = 3;
    void *args[] = {&u, &v};
    int res = 0;
    jit_function_apply(jit_adder, args, &res);
    jit_function_apply(native_caller, args, &res);
    jit_function_apply(interp_caller, args, &res);

    jit_context_destroy(context);
}
