#include "armgen.h"

#define ARMGEN_DEBUG
#undef ARMGEN_DEBUG

#define ARCH_SIZE 4
#define TYPELEN 10

static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst)
{
    if (last)
    {
        last = last->tail = AS_InstrList(inst, NULL);
    }
    else
    {
        last = iList = AS_InstrList(inst, NULL);
    }
}

typedef enum
{
    NONE,
    BR,
    RET,
    ADD,
    SUB,
    MUL,
    DIV,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    F2I,
    I2F,
    I2P,
    P2I,
    LOAD,
    STORE,
    CALL,
    EXTCALL,
    ICMP,
    FCMP,
    LABEL,
    CJUMP
} AS_type;

AS_type gettype(AS_instr ins)
{
    AS_type ret = NONE;
    string assem = ins->u.OPER.assem;
    if (ins->kind == I_MOVE)
    {
        assem = ins->u.MOVE.assem;
    }
    else if (ins->kind == I_LABEL)
    {
        ret = LABEL;
        return ret;
    }
    if (!strncmp(assem, "br", TYPELEN))
    {
        ret = BR;
        return ret;
    }
    else if (!strncmp(assem, "ret", TYPELEN))
    {
        ret = RET;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fadd", TYPELEN))
    {
        ret = FADD;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = add", TYPELEN))
    {
        ret = ADD;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fsub", TYPELEN))
    {
        ret = FSUB;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = sub", TYPELEN))
    {
        ret = SUB;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fmul", TYPELEN))
    {
        ret = FMUL;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = mul", TYPELEN))
    {
        ret = MUL;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fdiv", TYPELEN))
    {
        ret = FDIV;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = div", TYPELEN))
    {
        ret = DIV;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fptosi", TYPELEN))
    {
        ret = F2I;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = sitofp", TYPELEN))
    {
        ret = I2F;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = inttoptr", TYPELEN))
    {
        ret = I2P;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = load", TYPELEN))
    {
        ret = LOAD;
        return ret;
    }
    else if (!strncmp(assem, "%`store", TYPELEN))
    {
        ret = STORE;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = ptrtoint", TYPELEN))
    {
        ret = P2I;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = call", TYPELEN))
    {
        ret = CALL;
        return ret;
    }
    else if (!strncmp(assem, "call", TYPELEN))
    {
        ret = EXTCALL;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = icmp", TYPELEN))
    {
        ret = ICMP;
        return ret;
    }
    else if (!strncmp(assem, "%`d0 = fcmp", TYPELEN))
    {
        ret = FCMP;
        return ret;
    }
    else if (!strncmp(assem, "br i1", TYPELEN))
    {
        ret = CJUMP;
        return ret;
    }
    return ret;
}