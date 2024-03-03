typedef struct A_exp_ *A_exp;
typedef struct A_stm_ *A_stm;
typedef enum {A_plus,A_minus,A_times,A_div} A_binop;

struct A_stm_ {int id; A_exp exp;};
struct A_exp_ {
    enum {A_idExp, A_numExp, A_opExp} kind;
    union {int id;
           int num;
           struct {A_exp left; A_binop oper; A_exp right;} op;
          } u;
};

/* function prototypes below */
A_exp A_IdExp(int id);
A_exp A_NumExp(int num);
A_exp A_OpExp(A_exp left, A_binop oper, A_exp right);
A_stm A_AssignStm(int id, A_exp exp);

void printExp(A_exp);
void printStm(A_stm);
