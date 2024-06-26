#include "util.h"
#include "temp.h"
#include "tigerirp.h"
#include "semant.h"
#include <stdio.h>
#include <stdlib.h>

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size) {
// transError(out, A_Pos(0,0), "TODO: translate during semant! See Tiger Book for designing details:)");
 // return NULL;
 // The following only for a test. delete when you are done.
return T_FuncDeclList(
T_FuncDecl(String("main"),
  NULL,
  T_Seq(
    T_Seq(
      T_Seq(
        T_Seq(
          T_Seq(
            T_Move(
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
              T_Binop(T_plus,
                T_ExtCall(String("malloc"),
                  T_ExpList(
                    T_IntConst(16)/*T_int*/,
                    NULL)
                ,T_int)/*T_int*/,
                T_IntConst(8)/*T_int*/
              )/*T_int*/
            ),
            T_Move(
              T_Mem(
                T_Binop(T_plus,
                  T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                  T_IntConst(-8)/*T_int*/
                )/*T_int*/
              , T_int)/*T_int*/,
              T_IntConst(1)/*T_int*/
            )
          ),
          T_Move(
            T_Mem(
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
            , T_int)/*T_int*/,
            T_IntConst(0)/*T_int*/
          )
        ),
        T_Move(
          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
        )
      ),
      T_Move(
        T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
        T_IntConst(0)/*T_int*/
      )
    ),
    T_Seq(
      T_Move(
        T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
        T_Eseq(
          T_Seq(
            T_Move(
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
              T_Cast(
                T_ExtCall(String("getfloat"),
                  NULL
                ,T_float)/*T_float*/
              ,T_int)/*T_int*/
            ),
            T_Seq(
              T_Move(
                T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                T_Binop(T_plus,
                  T_ExtCall(String("malloc"),
                    T_ExpList(
                      T_Binop(T_mul,
                        T_Binop(T_plus,
                          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                          T_IntConst(1)/*T_int*/
                        )/*T_int*/,
                        T_IntConst(8)/*T_int*/
                      )/*T_int*/,
                      NULL)
                  ,T_int)/*T_int*/,
                  T_IntConst(8)/*T_int*/
                )/*T_int*/
              ),
              T_Move(
                T_Mem(
                  T_Binop(T_plus,
                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                    T_IntConst(-8)/*T_int*/
                  )/*T_int*/
                , T_int)/*T_int*/,
                T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
              )
            )
          ),
          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
        )/*T_int*/
      ),
      T_Seq(
        T_Move(
          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
          T_Mem(
            T_Binop(T_plus,
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
              T_IntConst(-8)/*T_int*/
            )/*T_int*/
          , T_int)/*T_int*/
        ),
        T_Seq(
          T_Move(
            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
            T_Eseq(
              T_Seq(
                T_Seq(
                  T_Move(
                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                    T_ExtCall(String("malloc"),
                      T_ExpList(
                        T_IntConst(16)/*T_int*/,
                        NULL)
                    ,T_int)/*T_int*/
                  ),
                  T_Move(
                    T_Mem(
                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
                    , T_int)/*T_int*/,
                    T_IntConst(2)/*T_int*/
                  )
                ),
                T_Move(
                  T_Mem(
                    T_Binop(T_plus,
                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                      T_IntConst(8)/*T_int*/
                    )/*T_int*/
                  , T_int)/*T_int*/,
                  T_Name(Temp_namedlabel(String("c1$m1")))/*T_int*/
                )
              ),
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
            )/*T_int*/
          ),
          T_Seq(
            T_Move(
              T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
              T_Eseq(
                T_Seq(
                  T_Seq(
                    T_Move(
                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                      T_ExtCall(String("malloc"),
                        T_ExpList(
                          T_IntConst(16)/*T_int*/,
                          NULL)
                      ,T_int)/*T_int*/
                    ),
                    T_Move(
                      T_Mem(
                        T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
                      , T_int)/*T_int*/,
                      T_IntConst(2)/*T_int*/
                    )
                  ),
                  T_Move(
                    T_Mem(
                      T_Binop(T_plus,
                        T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                        T_IntConst(8)/*T_int*/
                      )/*T_int*/
                    , T_int)/*T_int*/,
                    T_Name(Temp_namedlabel(String("c2$m1")))/*T_int*/
                  )
                ),
                T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
              )/*T_int*/
            ),
            T_Seq(
              T_Seq(
                T_Label(Temp_namedlabel(String("L0"))),
                T_Seq(
                  T_Cjump(T_lt,
                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                    Temp_namedlabel(String("L5")),
                    Temp_namedlabel(String("L1"))
                  ),
                  T_Seq(
                    T_Label(Temp_namedlabel(String("L5"))),
                    T_Seq(
                      T_Seq(
                        T_Seq(
                          T_Cjump(T_eq,
                            T_Binop(T_mul,
                              T_Binop(T_div,
                                T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                T_IntConst(2)/*T_int*/
                              )/*T_int*/,
                              T_IntConst(2)/*T_int*/
                            )/*T_int*/,
                            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                            Temp_namedlabel(String("L2")),
                            Temp_namedlabel(String("L3"))
                          ),
                          T_Seq(
                            T_Label(Temp_namedlabel(String("L2"))),
                            T_Seq(
                              T_Move(
                                T_Mem(
                                  T_Binop(T_plus,
                                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                    T_Binop(T_mul,
                                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                      T_IntConst(8)/*T_int*/
                                    )/*T_int*/
                                  )/*T_int*/
                                , T_int)/*T_int*/,
                                T_Call(String("m1"),
                                  T_Mem(
                                    T_Binop(T_plus,
                                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                      T_IntConst(8)/*T_int*/
                                    )/*T_int*/
                                  , T_int)/*T_int*/,
                                  T_ExpList(
                                    T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                    T_ExpList(
                                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                      NULL))
                                , T_int)/*T_int*/
                              ),
                              T_Seq(
                                T_Jump(Temp_namedlabel(String("L4"))),
                                T_Seq(
                                  T_Label(Temp_namedlabel(String("L3"))),
                                  T_Seq(
                                    T_Move(
                                      T_Mem(
                                        T_Binop(T_plus,
                                          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                          T_Binop(T_mul,
                                            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                            T_IntConst(8)/*T_int*/
                                          )/*T_int*/
                                        )/*T_int*/
                                      , T_int)/*T_int*/,
                                      T_Call(String("m1"),
                                        T_Mem(
                                          T_Binop(T_plus,
                                            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                            T_IntConst(8)/*T_int*/
                                          )/*T_int*/
                                        , T_int)/*T_int*/,
                                        T_ExpList(
                                          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                          T_ExpList(
                                            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                                            NULL))
                                      , T_int)/*T_int*/
                                    ),
                                    T_Label(Temp_namedlabel(String("L4")))
                                  )
                                )
                              )
                            )
                          )
                        ),
                        T_Move(
                          T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                          T_Binop(T_plus,
                            T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                            T_IntConst(1)/*T_int*/
                          )/*T_int*/
                        )
                      ),
                      T_Seq(
                        T_Jump(Temp_namedlabel(String("L0"))),
                        T_Label(Temp_namedlabel(String("L1")))
                      )
                    )
                  )
                )
              ),
              T_Seq(
                T_Exp(
                  T_ExtCall(String("putarray"),
                    T_ExpList(
                      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                      T_ExpList(
                        T_Temp(Temp_namedtemp(100,T_int))/*T_int*/,
                        NULL))
                  ,T_int)/*T_int*/
                ),
                T_Return(
                  T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
                )
              )
            )
          )
        )
      )
    )
  )
), 
T_FuncDeclList(
T_FuncDecl(String("c1$m1"),
  Temp_TempList(Temp_namedtemp(99,T_int),
    Temp_TempList(Temp_namedtemp(100,T_int),
      NULL)),
  T_Return(
    T_Mem(
      T_Temp(Temp_namedtemp(99,T_int))/*T_int*/
    , T_int)/*T_int*/
  )
),
T_FuncDeclList(
T_FuncDecl(String("c2$m1"),
  Temp_TempList(Temp_namedtemp(99,T_int),
    Temp_TempList(Temp_namedtemp(100,T_int),
      NULL)),
  T_Return(
    T_Binop(T_plus,
      T_Mem(
        T_Temp(Temp_namedtemp(99,T_int))/*T_int*/
      , T_int)/*T_int*/,
      T_Temp(Temp_namedtemp(100,T_int))/*T_int*/
    )/*T_int*/
  )
),
NULL
)));
}

