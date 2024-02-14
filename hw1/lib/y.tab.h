/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public 
   







   
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     PUBLIC = 258,
     INT = 259,
     MAIN = 260,
     EXTENDS = 261,
     CLASS = 262,
     IF = 263,
     ELSE = 264,
     WHILE = 265,
     CONTINUE = 266,
     BREAK = 267,
     RETURN = 268,
     PUTINT = 269,
     PUTCH = 270,
     PUTARRAY = 271,
     STARTTIME = 272,
     STOPTIME = 273,
     T = 274,
     F = 275,
     LENGTH = 276,
     THIS = 277,
     NEW = 278,
     GETINT = 279,
     GETCH = 280,
     GETARRAY = 281,
     ADD = 282,
     MINUS = 283,
     TIMES = 284,
     DIV = 285,
     OR = 286,
     AND = 287,
     LT = 288,
     LE = 289,
     GT = 290,
     GE = 291,
     EQ = 292,
     NE = 293,
     ID = 294,
     NUM = 295,
     UMINUS = 296
   };
#endif
/* Tokens.  */
#define PUBLIC 258
#define INT 259
#define MAIN 260
#define EXTENDS 261
#define CLASS 262
#define IF 263
#define ELSE 264
#define WHILE 265
#define CONTINUE 266
#define BREAK 267
#define RETURN 268
#define PUTINT 269
#define PUTCH 270
#define PUTARRAY 271
#define STARTTIME 272
#define STOPTIME 273
#define T 274
#define F 275
#define LENGTH 276
#define THIS 277
#define NEW 278
#define GETINT 279
#define GETCH 280
#define GETARRAY 281
#define ADD 282
#define MINUS 283
#define TIMES 284
#define DIV 285
#define OR 286
#define AND 287
#define LT 288
#define LE 289
#define GT 290
#define GE 291
#define EQ 292
#define NE 293
#define ID 294
#define NUM 295
#define UMINUS 296




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 18 "parser.yacc"
{
  A_pos pos;
  A_prog prog;
  A_mainMethod mainMethod;
  A_stmList stmList;
  A_stm stm;
  A_exp exp;
}
/* Line 1529 of yacc.c.  */
#line 140 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

