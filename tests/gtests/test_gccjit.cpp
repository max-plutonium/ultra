#include <gmock/gmock.h>

#include <libgccjit++.h>
#include <cstdlib>
#include <cstdio>

static void
create_code (gccjit::context ctxt)
{
    /* Let's try to inject the equivalent of this C code:
        void
        greet (const char *name)
        {
            printf ("hello %s\n", name);
        }
    */
    gccjit::type void_type = ctxt.get_type (GCC_JIT_TYPE_VOID);
    gccjit::type const_char_ptr_type =
            ctxt.get_type (GCC_JIT_TYPE_CONST_CHAR_PTR);
    gccjit::param param_name =
            ctxt.new_param (const_char_ptr_type, "name");
    std::vector<gccjit::param> func_params;
    func_params.push_back (param_name);
    gccjit::function func =
            ctxt.new_function (GCC_JIT_FUNCTION_EXPORTED,
                               void_type,
                               "greet",
                               func_params, 0);

    gccjit::param param_format =
            ctxt.new_param (const_char_ptr_type, "format");
    std::vector<gccjit::param> printf_params;
    printf_params.push_back (param_format);
    gccjit::function printf_func =
            ctxt.new_function (GCC_JIT_FUNCTION_IMPORTED,
                               ctxt.get_type (GCC_JIT_TYPE_INT),
                               "printf",
                               printf_params, 1);

    gccjit::block block = func.new_block ();
    block.add_eval (ctxt.new_call (printf_func,
                                   ctxt.new_rvalue ("hello %s\n"),
                                   param_name));
    block.end_with_return ();
}

TEST(test_gccjit, helloworld)
{
    gccjit::context ctxt;
    gcc_jit_result *result;

    /* Get a "context" object for working with the library.  */
    ctxt = gccjit::context::acquire ();

    /* Set some options on the context.
       Turn this on to see the code being generated, in assembler form.  */
    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE, 0);

    /* Populate the context.  */
    create_code (ctxt);

    /* Compile the code.  */
    result = ctxt.compile ();
    if (!result)
    {
        fprintf (stderr, "NULL result");
        exit (1);
    }

    ctxt.release ();

    /* Extract the generated code from "result".  */
    typedef void (*fn_type) (const char *);
    fn_type greet =
            (fn_type)gcc_jit_result_get_code (result, "greet");
    if (!greet)
    {
        fprintf (stderr, "NULL greet");
        exit (1);
    }

    /* Now call the generated function: */
    greet ("world");
    fflush (stdout);

    gcc_jit_result_release (result);
}



/* Functions are compiled to this function ptr type.  */
typedef int (*toyvm_compiled_func) (int);

enum opcode {
    /* Ops taking no operand.  */
    DUP,
    ROT,
    BINARY_ADD,
    BINARY_SUBTRACT,
    BINARY_MULT,
    BINARY_COMPARE_LT,
    RECURSE,
    RETURN,

    /* Ops taking an operand.  */
    PUSH_CONST,
    JUMP_ABS_IF_TRUE
};

#define FIRST_UNARY_OPCODE (PUSH_CONST)

const char * const opcode_names[] = {
    "DUP",
    "ROT",
    "BINARY_ADD",
    "BINARY_SUBTRACT",
    "BINARY_MULT",
    "BINARY_COMPARE_LT",
    "RECURSE",
    "RETURN",

    "PUSH_CONST",
    "JUMP_ABS_IF_TRUE",
};

struct toyvm_op
{
    /* Which operation.  */
    enum opcode op_opcode;

    /* Some opcodes take an argument.  */
    int op_operand;

    /* The line number of the operation within the source file.  */
    int op_linenum;
};

#define MAX_OPS  (64)

class toyvm_function
{
public:
    void
    add_op (enum opcode opcode,
            int operand, int linenum);

    void
    add_unary_op (enum opcode opcode,
                  const char *rest_of_line, int linenum);

    static toyvm_function *
    parse (const char *filename, const char *name);

    void
    disassemble_op (toyvm_op *op, int index, FILE *out);

    void
    disassemble (FILE *out);

    int
    interpret (int arg, FILE *trace);

    toyvm_compiled_func
    compile ();

private:
    const char *fn_filename;
    int         fn_num_ops;
    toyvm_op    fn_ops[MAX_OPS];
    friend struct compilation_state;
};

#define MAX_STACK_DEPTH (8)

class toyvm_frame
{
public:
    void push (int arg);
    int pop ();
    void dump_stack (FILE *out);

private:
    toyvm_function *frm_function;
    int             frm_pc;
    int             frm_stack[MAX_STACK_DEPTH];
    int             frm_cur_depth;

    friend int toyvm_function::interpret (int arg, FILE *trace);

};

void
toyvm_function::add_op (enum opcode opcode,
                        int operand, int linenum)
{
    toyvm_op *op;
    assert (fn_num_ops < MAX_OPS);
    op = &fn_ops[fn_num_ops++];
    op->op_opcode = opcode;
    op->op_operand = operand;
    op->op_linenum = linenum;
}

void
toyvm_function::add_unary_op (enum opcode opcode,
                              const char *rest_of_line, int linenum)
{
    int operand = atoi (rest_of_line);
    add_op (opcode, operand, linenum);
}

static char *
get_function_name (const char *filename)
{
    /* Skip any path separators.  */
    const char *pathsep = strrchr (filename, '/');
    if (pathsep)
        filename = pathsep + 1;

    /* Copy filename to funcname.  */
    char *funcname = (char *)malloc (strlen (filename) + 1);

    strcpy (funcname, filename);

    /* Convert "." to NIL terminator.  */
    *(strchr (funcname, '.')) = '\0';

    return funcname;
}

toyvm_function *
toyvm_function::parse (const char *filename, const char *name)
{
    FILE *f = NULL;
    toyvm_function *fn = NULL;
    char *line = NULL;
    ssize_t linelen;
    size_t bufsize;
    int linenum = 0;

    assert (filename);
    assert (name);

    f = fopen (filename, "r");
    if (!f)
    {
        fprintf (stderr,
                 "cannot open file %s: %s\n",
                 filename, strerror (errno));
        goto error;
    }

    fn = (toyvm_function *)calloc (1, sizeof (toyvm_function));
    if (!fn)
    {
        fprintf (stderr, "out of memory allocating toyvm_function\n");
        goto error;
    }
    fn->fn_filename = filename;

    /* Read the lines of the file.  */
    while ((linelen = getline (&line, &bufsize, f)) != -1)
    {
        /* Note that this is a terrible parser, but it avoids the need to
     bring in lex/yacc as a dependency.  */
        linenum++;

        if (0)
            fprintf (stdout, "%3d: %s", linenum, line);

        /* Lines beginning with # are comments.  */
        if (line[0] == '#')
            continue;

        /* Skip blank lines.  */
        if (line[0] == '\n')
            continue;

#define LINE_MATCHES(OPCODE) (0 == strncmp ((OPCODE), line, strlen (OPCODE)))
        if (LINE_MATCHES ("DUP\n"))
            fn->add_op (DUP, 0, linenum);
        else if (LINE_MATCHES ("ROT\n"))
            fn->add_op (ROT, 0, linenum);
        else if (LINE_MATCHES ("BINARY_ADD\n"))
            fn->add_op (BINARY_ADD, 0, linenum);
        else if (LINE_MATCHES ("BINARY_SUBTRACT\n"))
            fn->add_op (BINARY_SUBTRACT, 0, linenum);
        else if (LINE_MATCHES ("BINARY_MULT\n"))
            fn->add_op (BINARY_MULT, 0, linenum);
        else if (LINE_MATCHES ("BINARY_COMPARE_LT\n"))
            fn->add_op (BINARY_COMPARE_LT, 0, linenum);
        else if (LINE_MATCHES ("RECURSE\n"))
            fn->add_op (RECURSE, 0, linenum);
        else if (LINE_MATCHES ("RETURN\n"))
            fn->add_op (RETURN, 0, linenum);
        else if (LINE_MATCHES ("PUSH_CONST "))
            fn->add_unary_op (PUSH_CONST,
                              line + strlen ("PUSH_CONST "), linenum);
        else if (LINE_MATCHES ("JUMP_ABS_IF_TRUE "))
            fn->add_unary_op (JUMP_ABS_IF_TRUE,
                              line + strlen("JUMP_ABS_IF_TRUE "), linenum);
        else
        {
            fprintf (stderr, "%s:%d: parse error\n", filename, linenum);
            free (fn);
            fn = NULL;
            goto error;
        }
#undef LINE_MATCHES
    }
    free (line);
    fclose (f);

    return fn;

error:
    free (line);
    if (f)
        fclose (f);
    free (fn);
    return NULL;
}

void
toyvm_function::disassemble_op (toyvm_op *op, int index, FILE *out)
{
    fprintf (out, "%s:%d: index %d: %s",
             fn_filename, op->op_linenum, index,
             opcode_names[op->op_opcode]);
    if (op->op_opcode >= FIRST_UNARY_OPCODE)
        fprintf (out, " %d", op->op_operand);
    fprintf (out, "\n");
}

void
toyvm_function::disassemble (FILE *out)
{
    int i;
    for (i = 0; i < fn_num_ops; i++)
    {
        toyvm_op *op = &fn_ops[i];
        disassemble_op (op, i, out);
    }
}

void
toyvm_frame::push (int arg)
{
    assert (frm_cur_depth < MAX_STACK_DEPTH);
    frm_stack[frm_cur_depth++] = arg;
}

int
toyvm_frame::pop ()
{
    assert (frm_cur_depth > 0);
    return frm_stack[--frm_cur_depth];
}

void
toyvm_frame::dump_stack (FILE *out)
{
    int i;
    fprintf (out, "stack:");
    for (i = 0; i < frm_cur_depth; i++)
    {
        fprintf (out, " %d", frm_stack[i]);
    }
    fprintf (out, "\n");
}

/* Execute the given function.  */

int
toyvm_function::interpret (int arg, FILE *trace)
{
    toyvm_frame frame;
#define PUSH(ARG) (frame.push (ARG))
#define POP(ARG) (frame.pop ())

    frame.frm_function = this;
    frame.frm_pc = 0;
    frame.frm_cur_depth = 0;

    PUSH (arg);

    while (1)
    {
        toyvm_op *op;
        int x, y;
        assert (frame.frm_pc < fn_num_ops);
        op = &fn_ops[frame.frm_pc++];

        if (trace)
        {
            frame.dump_stack (trace);
            disassemble_op (op, frame.frm_pc, trace);
        }

        switch (op->op_opcode)
        {
            /* Ops taking no operand.  */
            case DUP:
                x = POP ();
                PUSH (x);
                PUSH (x);
                break;

            case ROT:
                y = POP ();
                x = POP ();
                PUSH (y);
                PUSH (x);
                break;

            case BINARY_ADD:
                y = POP ();
                x = POP ();
                PUSH (x + y);
                break;

            case BINARY_SUBTRACT:
                y = POP ();
                x = POP ();
                PUSH (x - y);
                break;

            case BINARY_MULT:
                y = POP ();
                x = POP ();
                PUSH (x * y);
                break;

            case BINARY_COMPARE_LT:
                y = POP ();
                x = POP ();
                PUSH (x < y);
                break;

            case RECURSE:
                x = POP ();
                x = interpret (x, trace);
                PUSH (x);
                break;

            case RETURN:
                return POP ();

                /* Ops taking an operand.  */
            case PUSH_CONST:
                PUSH (op->op_operand);
                break;

            case JUMP_ABS_IF_TRUE:
                x = POP ();
                if (x)
                    frame.frm_pc = op->op_operand;
                break;

            default:
                assert (0); /* unknown opcode */

        } /* end of switch on opcode */
    } /* end of while loop */

#undef PUSH
#undef POP
}

/* JIT compilation.  */

class compilation_state
{
public:
    compilation_state (toyvm_function &toyvmfn) :
        toyvmfn (toyvmfn)
    {}

    void create_context ();
    void create_types ();
    void create_locations ();
    void create_function (const char *funcname);
    gcc_jit_result *compile ();

private:
    void
    add_push (gccjit::block block,
              gccjit::rvalue rvalue,
              gccjit::location loc);

    void
    add_pop (gccjit::block block,
             gccjit::lvalue lvalue,
             gccjit::location loc);

private:

    /* State.  */

    toyvm_function &toyvmfn;

    gccjit::context ctxt;

    gccjit::type int_type;
    gccjit::type bool_type;
    gccjit::type stack_type; /* int[MAX_STACK_DEPTH] */

    gccjit::rvalue const_one;

    gccjit::function fn;
    gccjit::param param_arg;
    gccjit::lvalue stack;
    gccjit::lvalue stack_depth;
    gccjit::lvalue x;
    gccjit::lvalue y;

    gccjit::location op_locs[MAX_OPS];
    gccjit::block initial_block;
    gccjit::block op_blocks[MAX_OPS];

};

/* The main compilation hook.  */

toyvm_compiled_func
toyvm_function::compile ()
{
    compilation_state state (*this);
    char *funcname;

    funcname = get_function_name (fn_filename);

    state.create_context ();
    state.create_types ();
    state.create_locations ();
    state.create_function (funcname);

    /* We've now finished populating the context.  Compile it.  */
    gcc_jit_result *result = state.compile ();

    return (toyvm_compiled_func)gcc_jit_result_get_code (result, funcname);
    /* (this leaks "result" and "funcname") */
}

/* Stack manipulation.  */

void
compilation_state::add_push (gccjit::block block,
                             gccjit::rvalue rvalue,
                             gccjit::location loc)
{
    /* stack[stack_depth] = RVALUE */
    block.add_assignment (
                /* stack[stack_depth] */
                ctxt.new_array_access (
                    stack,
                    stack_depth,
                    loc),
                rvalue,
                loc);

    /* "stack_depth++;".  */
    block.add_assignment_op (
                stack_depth,
                GCC_JIT_BINARY_OP_PLUS,
                const_one,
                loc);
}

void
compilation_state::add_pop (gccjit::block block,
                            gccjit::lvalue lvalue,
                            gccjit::location loc)
{
    /* "--stack_depth;".  */
    block.add_assignment_op (
                stack_depth,
                GCC_JIT_BINARY_OP_MINUS,
                const_one,
                loc);

    /* "LVALUE = stack[stack_depth];".  */
    block.add_assignment (
                lvalue,
                /* stack[stack_depth] */
                ctxt.new_array_access (stack,
                                       stack_depth,
                                       loc),
                loc);
}

/* Create the context. */

void
compilation_state::create_context ()
{
    ctxt = gccjit::context::acquire ();

    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE,
                          0);
    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE,
                          0);
    ctxt.set_int_option (GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
                         3);
    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_KEEP_INTERMEDIATES,
                          0);
    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_DUMP_EVERYTHING,
                          0);
    ctxt.set_bool_option (GCC_JIT_BOOL_OPTION_DEBUGINFO,
                          1);
}

/* Create types.  */

void
compilation_state::create_types ()
{
    /* Create types.  */
    int_type = ctxt.get_type (GCC_JIT_TYPE_INT);
    bool_type = ctxt.get_type (GCC_JIT_TYPE_BOOL);
    stack_type = ctxt.new_array_type (int_type, MAX_STACK_DEPTH);

    /* The constant value 1.  */
    const_one = ctxt.one (int_type);

}

/* Create locations.  */

void
compilation_state::create_locations ()
{
    for (int pc = 0; pc < toyvmfn.fn_num_ops; pc++)
    {
        toyvm_op *op = &toyvmfn.fn_ops[pc];

        op_locs[pc] = ctxt.new_location (toyvmfn.fn_filename,
                                         op->op_linenum,
                                         0); /* column */
    }
}

/* Creating the function.  */

void
compilation_state::create_function (const char *funcname)
{
    std::vector <gccjit::param> params;
    param_arg = ctxt.new_param (int_type, "arg", op_locs[0]);
    params.push_back (param_arg);
    fn = ctxt.new_function (GCC_JIT_FUNCTION_EXPORTED,
                            int_type,
                            funcname,
                            params, 0,
                            op_locs[0]);

    /* Create stack lvalues.  */
    stack = fn.new_local (stack_type, "stack");
    stack_depth = fn.new_local (int_type, "stack_depth");
    x = fn.new_local (int_type, "x");
    y = fn.new_local (int_type, "y");

    /* 1st pass: create blocks, one per opcode. */

    /* We need an entry block to do one-time initialization, so create that
     first.  */
    initial_block = fn.new_block ("initial");

    /* Create a block per operation.  */
    for (int pc = 0; pc < toyvmfn.fn_num_ops; pc++)
    {
        char buf[16];
        sprintf (buf, "instr%i", pc);
        op_blocks[pc] = fn.new_block (buf);
    }

    /* Populate the initial block.  */

    /* "stack_depth = 0;".  */
    initial_block.add_assignment (stack_depth,
                                  ctxt.zero (int_type),
                                  op_locs[0]);

    /* "PUSH (arg);".  */
    add_push (initial_block,
              param_arg,
              op_locs[0]);

    /* ...and jump to insn 0.  */
    initial_block.end_with_jump (op_blocks[0],
            op_locs[0]);

    /* 2nd pass: fill in instructions.  */
    for (int pc = 0; pc < toyvmfn.fn_num_ops; pc++)
    {
        gccjit::location loc = op_locs[pc];

        gccjit::block block = op_blocks[pc];
        gccjit::block next_block = (pc < toyvmfn.fn_num_ops
                                    ? op_blocks[pc + 1]
                                    : NULL);

        toyvm_op *op;
        op = &toyvmfn.fn_ops[pc];

        /* Helper macros.  */

#define X_EQUALS_POP()\
    add_pop (block, x, loc)
#define Y_EQUALS_POP()\
    add_pop (block, y, loc)
#define PUSH_RVALUE(RVALUE)\
    add_push (block, (RVALUE), loc)
#define PUSH_X()\
    PUSH_RVALUE (x)
#define PUSH_Y() \
    PUSH_RVALUE (y)

        block.add_comment (opcode_names[op->op_opcode], loc);

        /* Handle the individual opcodes.  */

        switch (op->op_opcode)
        {
            case DUP:
                X_EQUALS_POP ();
                PUSH_X ();
                PUSH_X ();
                break;

            case ROT:
                Y_EQUALS_POP ();
                X_EQUALS_POP ();
                PUSH_Y ();
                PUSH_X ();
                break;

            case BINARY_ADD:
                Y_EQUALS_POP ();
                X_EQUALS_POP ();
                PUSH_RVALUE (
                            ctxt.new_binary_op (
                                GCC_JIT_BINARY_OP_PLUS,
                                int_type,
                                x, y,
                                loc));
                break;

            case BINARY_SUBTRACT:
                Y_EQUALS_POP ();
                X_EQUALS_POP ();
                PUSH_RVALUE (
                            ctxt.new_binary_op (
                                GCC_JIT_BINARY_OP_MINUS,
                                int_type,
                                x, y,
                                loc));
                break;

            case BINARY_MULT:
                Y_EQUALS_POP ();
                X_EQUALS_POP ();
                PUSH_RVALUE (
                            ctxt.new_binary_op (
                                GCC_JIT_BINARY_OP_MULT,
                                int_type,
                                x, y,
                                loc));
                break;

            case BINARY_COMPARE_LT:
                Y_EQUALS_POP ();
                X_EQUALS_POP ();
                PUSH_RVALUE (
                            /* cast of bool to int */
                            ctxt.new_cast (
                                /* (x < y) as a bool */
                                ctxt.new_comparison (
                                    GCC_JIT_COMPARISON_LT,
                                    x, y,
                                    loc),
                                int_type,
                                loc));
                break;

            case RECURSE:
            {
                X_EQUALS_POP ();
                PUSH_RVALUE (
                            ctxt.new_call (
                                fn,
                                x,
                                loc));
                break;
            }

            case RETURN:
                X_EQUALS_POP ();
                block.end_with_return (x, loc);
                break;

                /* Ops taking an operand.  */
            case PUSH_CONST:
                PUSH_RVALUE (
                            ctxt.new_rvalue (int_type, op->op_operand));
                break;

            case JUMP_ABS_IF_TRUE:
                X_EQUALS_POP ();
                block.end_with_conditional (
                            /* "(bool)x".  */
                            ctxt.new_cast (x, bool_type, loc),
                            op_blocks[op->op_operand], /* on_true */
                        next_block, /* on_false */
                        loc);
                break;

            default:
                assert(0);
        } /* end of switch on opcode */

        /* Go to the next block.  */
        if (op->op_opcode != JUMP_ABS_IF_TRUE
            && op->op_opcode != RETURN)
            block.end_with_jump (next_block, loc);

    } /* end of loop on PC locations.  */
}

gcc_jit_result *
compilation_state::compile ()
{
    return ctxt.compile ();
}

TEST(test_gccjit, toyvm)
{
    const int argc = 3;
    const char *argv[3] = { "toyvm", "factorial.toy", "10" };

    const char *filename = NULL;
    toyvm_function *fn = NULL;

    if (argc != 3)
    {
        fprintf (stdout,
                 "%s FILENAME INPUT: Parse and run a .toy file\n",
                 argv[0]);
        exit (1);
    }

    filename = argv[1];
    fn = toyvm_function::parse (filename, filename);
    if (!fn)
        exit (1);

    if (0)
        fn->disassemble (stdout);

    printf ("interpreter result: %d\n",
            fn->interpret (atoi (argv[2]), NULL));

    /* JIT-compilation.  */
    toyvm_compiled_func code = fn->compile ();
    printf ("compiler result: %d\n",
            code (atoi (argv[2])));
}


#define MAX_OPEN_PARENS 200

typedef struct bf_compiler
{
    const char *filename;
    int line;
    int column;

    gcc_jit_context *ctxt;

    gcc_jit_type *void_type;
    gcc_jit_type *int_type;
    gcc_jit_type *byte_type;
    gcc_jit_type *array_type;

    gcc_jit_function *func_getchar;
    gcc_jit_function *func_putchar;

    gcc_jit_function *func;
    gcc_jit_block *curblock;

    gcc_jit_rvalue *int_zero;
    gcc_jit_rvalue *int_one;
    gcc_jit_rvalue *byte_zero;
    gcc_jit_rvalue *byte_one;
    gcc_jit_lvalue *data_cells;
    gcc_jit_lvalue *idx;

    int num_open_parens;
    gcc_jit_block *paren_test[MAX_OPEN_PARENS];
    gcc_jit_block *paren_body[MAX_OPEN_PARENS];
    gcc_jit_block *paren_after[MAX_OPEN_PARENS];

} bf_compiler;

/* Bail out, with a message on stderr.  */

static void
fatal_error (bf_compiler *bfc, const char *msg)
{
    fprintf (stderr,
             "%s:%i:%i: %s",
             bfc->filename, bfc->line, bfc->column, msg);
    abort ();
}

/* Get "data_cells[idx]" as an lvalue.  */

static gcc_jit_lvalue *
bf_get_current_data (bf_compiler *bfc, gcc_jit_location *loc)
{
    return gcc_jit_context_new_array_access (
                bfc->ctxt,
                loc,
                gcc_jit_lvalue_as_rvalue (bfc->data_cells),
                gcc_jit_lvalue_as_rvalue (bfc->idx));
}

/* Get "data_cells[idx] == 0" as a boolean rvalue.  */

static gcc_jit_rvalue *
bf_current_data_is_zero (bf_compiler *bfc, gcc_jit_location *loc)
{
    return gcc_jit_context_new_comparison (
                bfc->ctxt,
                loc,
                GCC_JIT_COMPARISON_EQ,
                gcc_jit_lvalue_as_rvalue (bf_get_current_data (bfc, loc)),
                bfc->byte_zero);
}

/* Compile one bf character.  */

static void
bf_compile_char (bf_compiler *bfc,
                 unsigned char ch)
{
    gcc_jit_location *loc =
            gcc_jit_context_new_location (bfc->ctxt,
                                          bfc->filename,
                                          bfc->line,
                                          bfc->column);

    /* Turn this on to trace execution, by injecting putchar ()
     of each source char. */
    if (0)
    {
        gcc_jit_rvalue *arg =
                gcc_jit_context_new_rvalue_from_int (
                    bfc->ctxt,
                    bfc->int_type,
                    ch);
        gcc_jit_rvalue *call =
                gcc_jit_context_new_call (bfc->ctxt,
                                          loc,
                                          bfc->func_putchar,
                                          1, &arg);
        gcc_jit_block_add_eval (bfc->curblock,
                                loc,
                                call);
    }

    switch (ch)
    {
        case '>':
            gcc_jit_block_add_comment (bfc->curblock,
                                       loc,
                                       "'>': idx += 1;");
            gcc_jit_block_add_assignment_op (bfc->curblock,
                                             loc,
                                             bfc->idx,
                                             GCC_JIT_BINARY_OP_PLUS,
                                             bfc->int_one);
            break;

        case '<':
            gcc_jit_block_add_comment (bfc->curblock,
                                       loc,
                                       "'<': idx -= 1;");
            gcc_jit_block_add_assignment_op (bfc->curblock,
                                             loc,
                                             bfc->idx,
                                             GCC_JIT_BINARY_OP_MINUS,
                                             bfc->int_one);
            break;

        case '+':
            gcc_jit_block_add_comment (bfc->curblock,
                                       loc,
                                       "'+': data[idx] += 1;");
            gcc_jit_block_add_assignment_op (bfc->curblock,
                                             loc,
                                             bf_get_current_data (bfc, loc),
                                             GCC_JIT_BINARY_OP_PLUS,
                                             bfc->byte_one);
            break;

        case '-':
            gcc_jit_block_add_comment (bfc->curblock,
                                       loc,
                                       "'-': data[idx] -= 1;");
            gcc_jit_block_add_assignment_op (bfc->curblock,
                                             loc,
                                             bf_get_current_data (bfc, loc),
                                             GCC_JIT_BINARY_OP_MINUS,
                                             bfc->byte_one);
            break;

        case '.':
        {
            gcc_jit_rvalue *arg =
                    gcc_jit_context_new_cast (
                        bfc->ctxt,
                        loc,
                        gcc_jit_lvalue_as_rvalue (bf_get_current_data (bfc, loc)),
                        bfc->int_type);
            gcc_jit_rvalue *call =
                    gcc_jit_context_new_call (bfc->ctxt,
                                              loc,
                                              bfc->func_putchar,
                                              1, &arg);
            gcc_jit_block_add_comment (bfc->curblock,
                                       loc,
                                       "'.': putchar ((int)data[idx]);");
            gcc_jit_block_add_eval (bfc->curblock,
                                    loc,
                                    call);
        }
            break;

        case ',':
        {
            gcc_jit_rvalue *call =
                    gcc_jit_context_new_call (bfc->ctxt,
                                              loc,
                                              bfc->func_getchar,
                                              0, NULL);
            gcc_jit_block_add_comment (
                        bfc->curblock,
                        loc,
                        "',': data[idx] = (unsigned char)getchar ();");
            gcc_jit_block_add_assignment (bfc->curblock,
                                          loc,
                                          bf_get_current_data (bfc, loc),
                                          gcc_jit_context_new_cast (
                                              bfc->ctxt,
                                              loc,
                                              call,
                                              bfc->byte_type));
        }
            break;

        case '[':
        {
            gcc_jit_block *loop_test =
                    gcc_jit_function_new_block (bfc->func, NULL);
            gcc_jit_block *on_zero =
                    gcc_jit_function_new_block (bfc->func, NULL);
            gcc_jit_block *on_non_zero =
                    gcc_jit_function_new_block (bfc->func, NULL);

            if (bfc->num_open_parens == MAX_OPEN_PARENS)
                fatal_error (bfc, "too many open parens");

            gcc_jit_block_end_with_jump (
                        bfc->curblock,
                        loc,
                        loop_test);

            gcc_jit_block_add_comment (
                        loop_test,
                        loc,
                        "'['");
            gcc_jit_block_end_with_conditional (
                        loop_test,
                        loc,
                        bf_current_data_is_zero (bfc, loc),
                        on_zero,
                        on_non_zero);
            bfc->paren_test[bfc->num_open_parens] = loop_test;
            bfc->paren_body[bfc->num_open_parens] = on_non_zero;
            bfc->paren_after[bfc->num_open_parens] = on_zero;
            bfc->num_open_parens += 1;
            bfc->curblock = on_non_zero;
        }
            break;

        case ']':
        {
            gcc_jit_block_add_comment (
                        bfc->curblock,
                        loc,
                        "']'");

            if (bfc->num_open_parens == 0)
                fatal_error (bfc, "mismatching parens");
            bfc->num_open_parens -= 1;
            gcc_jit_block_end_with_jump (
                        bfc->curblock,
                        loc,
                        bfc->paren_test[bfc->num_open_parens]);
            bfc->curblock = bfc->paren_after[bfc->num_open_parens];
        }
            break;

        case '\n':
            bfc->line +=1;
            bfc->column = 0;
            break;
    }

    if (ch != '\n')
        bfc->column += 1;
}

/* Make "main" function:
     int
     main (int argc, char **argv)
     {
       ...
     }
*/
static gcc_jit_function *
make_main (gcc_jit_context *ctxt)
{
    gcc_jit_type *int_type =
            gcc_jit_context_get_type (ctxt, GCC_JIT_TYPE_INT);
    gcc_jit_param *param_argc =
            gcc_jit_context_new_param (ctxt, NULL, int_type, "argc");
    gcc_jit_type *char_ptr_ptr_type =
            gcc_jit_type_get_pointer (
                gcc_jit_type_get_pointer (
                    gcc_jit_context_get_type (ctxt, GCC_JIT_TYPE_CHAR)));
    gcc_jit_param *param_argv =
            gcc_jit_context_new_param (ctxt, NULL, char_ptr_ptr_type, "argv");
    gcc_jit_param *params[2] = {param_argc, param_argv};
    gcc_jit_function *func_main =
            gcc_jit_context_new_function (ctxt, NULL,
                                          GCC_JIT_FUNCTION_EXPORTED,
                                          int_type,
                                          "main",
                                          2, params,
                                          0);
    return func_main;
}

/* Compile the given .bf file into a gcc_jit_context, containing a
   single "main" function suitable for compiling into an executable.  */
gcc_jit_context *
bf_compile (const char *filename)
{
    bf_compiler bfc;
    FILE *f_in;
    int ch;

    memset (&bfc, 0, sizeof (bfc));

    bfc.filename = filename;
    f_in = fopen (filename, "r");
    if (!f_in)
        fatal_error (&bfc, "unable to open file");
    bfc.line = 1;

    bfc.ctxt = gcc_jit_context_acquire ();

    gcc_jit_context_set_int_option (
                bfc.ctxt,
                GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
                3);

    gcc_jit_context_set_bool_option (
                bfc.ctxt,
                GCC_JIT_BOOL_OPTION_DEBUGINFO,
                1);

    bfc.void_type =
            gcc_jit_context_get_type (bfc.ctxt, GCC_JIT_TYPE_VOID);
    bfc.int_type =
            gcc_jit_context_get_type (bfc.ctxt, GCC_JIT_TYPE_INT);
    bfc.byte_type =
            gcc_jit_context_get_type (bfc.ctxt, GCC_JIT_TYPE_UNSIGNED_CHAR);
    bfc.array_type =
            gcc_jit_context_new_array_type (bfc.ctxt,
                                            NULL,
                                            bfc.byte_type,
                                            30000);

    bfc.func_getchar =
            gcc_jit_context_new_function (bfc.ctxt, NULL,
                                          GCC_JIT_FUNCTION_IMPORTED,
                                          bfc.int_type,
                                          "getchar",
                                          0, NULL,
                                          0);

    gcc_jit_param *param_c =
            gcc_jit_context_new_param (bfc.ctxt, NULL, bfc.int_type, "c");
    bfc.func_putchar =
            gcc_jit_context_new_function (bfc.ctxt, NULL,
                                          GCC_JIT_FUNCTION_IMPORTED,
                                          bfc.void_type,
                                          "putchar",
                                          1, &param_c,
                                          0);

    bfc.func = make_main (bfc.ctxt);
    bfc.curblock =
            gcc_jit_function_new_block (bfc.func, "initial");
    bfc.int_zero = gcc_jit_context_zero (bfc.ctxt, bfc.int_type);
    bfc.int_one = gcc_jit_context_one (bfc.ctxt, bfc.int_type);
    bfc.byte_zero = gcc_jit_context_zero (bfc.ctxt, bfc.byte_type);
    bfc.byte_one = gcc_jit_context_one (bfc.ctxt, bfc.byte_type);

    bfc.data_cells =
            gcc_jit_context_new_global (bfc.ctxt, NULL,
                                        GCC_JIT_GLOBAL_INTERNAL,
                                        bfc.array_type,
                                        "data_cells");
    bfc.idx =
            gcc_jit_function_new_local (bfc.func, NULL,
                                        bfc.int_type,
                                        "idx");

    gcc_jit_block_add_comment (bfc.curblock,
                               NULL,
                               "idx = 0;");
    gcc_jit_block_add_assignment (bfc.curblock,
                                  NULL,
                                  bfc.idx,
                                  bfc.int_zero);

    bfc.num_open_parens = 0;

    while ( EOF != (ch = fgetc (f_in)))
        bf_compile_char (&bfc, (unsigned char)ch);

    gcc_jit_block_end_with_return (bfc.curblock, NULL, bfc.int_zero);

    fclose (f_in);

    return bfc.ctxt;
}

TEST(test_gccjit, bf)
{
    const int argc = 3;
    const char *argv[3] = { "bf", "src.bf", "bf-prog" };
    const char *input_file;
    const char *output_file;
    gcc_jit_context *ctxt;
    const char *err;

    if (argc != 3)
    {
        fprintf (stderr, "%s: INPUT_FILE OUTPUT_FILE\n", argv[0]);
        return;
    }

    input_file = argv[1];
    output_file = argv[2];
    ctxt = bf_compile (input_file);

    gcc_jit_context_compile_to_file (ctxt,
                                     GCC_JIT_OUTPUT_KIND_EXECUTABLE,
                                     output_file);

    err = gcc_jit_context_get_first_error (ctxt);

    if (err)
    {
        gcc_jit_context_release (ctxt);
        return;
    }

    gcc_jit_context_release (ctxt);
    return;
}
