#include <gmock/gmock.h>
#include <jit/jit.h>

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

    // Compile (JIT) the function to machine code
    jit_context_build_start(context);
    jit_function_compile(gcd);
    jit_context_build_end(context);

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
