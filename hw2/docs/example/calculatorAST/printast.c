#include <stdio.h>
#include <stdlib.h>
#include "calc.h"

void printExp(A_exp e) {
    switch (e->kind) {
        case A_numExp: printf("%d", e->u.num); break;
        case A_idExp: printf("%c", e->u.id+'a'); break;
        case A_opExp:
            printf("(");
            printExp(e->u.op.left);
            switch (e->u.op.oper) {
                case A_plus: printf("+"); break;
                case A_times: printf("*"); break;
                case A_minus: printf("-"); break;
                case A_div: printf("/"); break;
            }
            printExp(e->u.op.right);
            printf(")");
        break;
    }
}

void printStm(A_stm s) {
    printf("%c=", s->id);
    printExp(s->exp);
}
