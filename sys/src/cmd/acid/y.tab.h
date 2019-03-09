/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    Tfmt = 258,
    Toror = 259,
    Tandand = 260,
    Teq = 261,
    Tneq = 262,
    Tleq = 263,
    Tgeq = 264,
    Tlsh = 265,
    Trsh = 266,
    Tdec = 267,
    Tinc = 268,
    Tindir = 269,
    Tid = 270,
    Tconst = 271,
    Tfconst = 272,
    Tstring = 273,
    Tif = 274,
    Tdo = 275,
    Tthen = 276,
    Telse = 277,
    Twhile = 278,
    Tloop = 279,
    Thead = 280,
    Ttail = 281,
    Tappend = 282,
    Tfn = 283,
    Tret = 284,
    Tlocal = 285,
    Tcomplex = 286,
    Twhat = 287,
    Tdelete = 288,
    Teval = 289,
    Tbuiltin = 290
  };
#endif
/* Tokens.  */
#define Tfmt 258
#define Toror 259
#define Tandand 260
#define Teq 261
#define Tneq 262
#define Tleq 263
#define Tgeq 264
#define Tlsh 265
#define Trsh 266
#define Tdec 267
#define Tinc 268
#define Tindir 269
#define Tid 270
#define Tconst 271
#define Tfconst 272
#define Tstring 273
#define Tif 274
#define Tdo 275
#define Tthen 276
#define Telse 277
#define Twhile 278
#define Tloop 279
#define Thead 280
#define Ttail 281
#define Tappend 282
#define Tfn 283
#define Tret 284
#define Tlocal 285
#define Tcomplex 286
#define Twhat 287
#define Tdelete 288
#define Teval 289
#define Tbuiltin 290

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 13 "dbg.y" /* yacc.c:1910  */

	Node		*node;
	Lsym		*sym;
	uint64_t	ival;
	float		fval;
	String		*string;

#line 132 "y.tab.h" /* yacc.c:1910  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
