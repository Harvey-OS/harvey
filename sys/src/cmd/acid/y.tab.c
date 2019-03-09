/* A Bison parser, made by GNU Bison 3.0.5.  */

/* Bison implementation for Yacc-like parsers in C

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "dbg.y" /* yacc.c:339  */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

#define YYSIZE_T size_t

#line 77 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
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
#line 13 "dbg.y" /* yacc.c:355  */

	Node		*node;
	Lsym		*sym;
	uint64_t	ival;
	float		fval;
	String		*string;

#line 195 "y.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 212 "y.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   669

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  60
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  18
/* YYNRULES -- Number of rules.  */
#define YYNRULES  88
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  191

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    56,     2,     2,     2,    23,    10,     2,
      29,    51,    21,    19,    54,    20,    27,    22,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    59,     3,
      13,     4,    14,     2,    55,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    28,     2,    58,     9,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    52,     8,    53,    57,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     5,     6,
       7,    11,    12,    15,    16,    17,    18,    24,    25,    26,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    49,    49,    50,    53,    63,    67,    71,    77,    78,
      80,    81,    87,    94,   100,   107,   113,   120,   121,   124,
     125,   131,   132,   136,   140,   144,   148,   152,   156,   160,
     167,   172,   180,   181,   184,   185,   189,   193,   197,   201,
     205,   209,   213,   217,   221,   225,   229,   233,   237,   241,
     245,   249,   253,   257,   261,   267,   268,   275,   276,   280,
     284,   288,   293,   297,   301,   305,   309,   313,   317,   321,
     325,   331,   335,   339,   343,   347,   352,   357,   361,   365,
     370,   371,   375,   382,   389,   396,   401,   408,   409
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "';'", "'='", "Tfmt", "Toror", "Tandand",
  "'|'", "'^'", "'&'", "Teq", "Tneq", "'<'", "'>'", "Tleq", "Tgeq", "Tlsh",
  "Trsh", "'+'", "'-'", "'*'", "'/'", "'%'", "Tdec", "Tinc", "Tindir",
  "'.'", "'['", "'('", "Tid", "Tconst", "Tfconst", "Tstring", "Tif", "Tdo",
  "Tthen", "Telse", "Twhile", "Tloop", "Thead", "Ttail", "Tappend", "Tfn",
  "Tret", "Tlocal", "Tcomplex", "Twhat", "Tdelete", "Teval", "Tbuiltin",
  "')'", "'{'", "'}'", "','", "'@'", "'!'", "'~'", "']'", "':'", "$accept",
  "prog", "bigstmnt", "zsemi", "members", "mname", "member", "zname",
  "slist", "stmnt", "idlist", "zexpr", "expr", "castexpr", "monexpr",
  "term", "name", "args", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,    59,    61,   258,   259,   260,   124,    94,
      38,   261,   262,    60,    62,   263,   264,   265,   266,    43,
      45,    42,    47,    37,   267,   268,   269,    46,    91,    40,
     270,   271,   272,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,    41,   123,   125,    44,    64,    33,   126,    93,    58
};
# endif

#define YYPACT_NINF -54

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-54)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -54,    99,   -54,    21,    21,    21,    21,    21,   360,   -49,
     -54,   -54,   -54,   399,   399,   399,    21,    21,    21,   -10,
     399,     2,    42,    45,    21,    21,    91,   321,    21,    21,
      21,   -54,   -54,    22,   571,   -54,   -54,   133,     0,   399,
     399,   -54,   -54,   -54,   -54,   -54,   -38,   235,    91,   453,
     486,   184,   -54,   -54,    71,    97,   551,   -54,    80,   -24,
      83,   -54,   -54,    82,   -54,   121,   122,   243,   -54,    22,
      -5,   -54,   -54,   -54,   -54,   399,   -54,   399,   399,   399,
     399,   399,   399,   399,   399,   399,   399,   399,   399,   399,
     399,   399,   399,   399,   399,   -54,   -54,   123,   154,   399,
     399,   -54,    21,   -54,   -54,   321,   321,   399,    21,   399,
     -54,   155,   183,     8,    21,   399,    91,   -54,   -54,   -54,
     399,   571,   588,   604,   619,   633,   646,   464,   464,   192,
     192,   192,   192,   208,   208,    34,    34,   -54,   -54,   -54,
     -54,   -54,   160,    13,   -54,   179,   -54,   518,   -54,    59,
     -54,   -54,   -54,    78,     8,   -22,   186,   -54,   -54,    63,
     -54,   -54,   -54,   321,   321,   216,   190,   191,     6,   220,
     -54,   190,   -54,   -54,   -54,   216,   172,   222,   190,   229,
     -54,   231,   -54,   321,   -54,   232,   -54,   -54,   282,   -54,
     -54
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,    32,     1,     0,     0,     0,     0,     0,     0,    85,
      81,    82,    83,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    17,     0,     0,     0,    32,     0,     0,
       0,     3,     4,     0,    33,    34,    55,    57,    80,     0,
      32,    60,    61,    58,    62,    63,    85,     0,     0,     0,
       0,     0,    64,    65,     0,     6,     0,    30,    28,    85,
       0,    18,    84,     0,    70,     0,     0,    32,    19,    87,
       0,    59,    68,    69,    21,     0,    54,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    74,    77,     0,     0,     0,
      32,    87,     0,    71,    86,    32,    32,     0,     0,    32,
      27,     0,     0,     0,     0,    32,     0,    22,    20,    72,
      32,    53,    52,    51,    50,    49,    48,    46,    47,    42,
      43,    44,    45,    41,    40,    38,    39,    35,    36,    37,
      76,    75,     0,     0,    56,    23,    26,     0,    66,     0,
      31,    29,    12,     0,     0,     0,     0,    10,    67,     0,
      88,    73,    78,    32,    32,     8,     0,     0,     0,     0,
      11,     0,    79,    24,    25,     8,     0,     0,     0,     0,
       7,     0,     9,    32,    13,     0,    16,    15,    32,    14,
       5
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -54,   -54,   -54,    61,   105,    55,   -53,   -54,    54,    -1,
     -54,     7,     4,   -54,    -2,   -54,   -15,   -35
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    31,   176,   155,   156,   157,    62,    67,    68,
      58,    33,    34,    35,    36,    37,    38,    70
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      32,    41,    42,    43,    44,    45,     9,    60,   152,   153,
      48,    65,    47,   102,    52,    53,    54,    49,    50,    51,
      55,    48,    63,    64,    56,    74,    71,    72,    73,   100,
     154,   169,    57,   104,    69,    48,   152,   153,   152,   153,
       3,     4,     5,    47,   112,     6,     7,   101,   119,   120,
      39,     9,    10,    11,    12,    92,    93,    94,   154,   179,
     154,    16,    17,    18,   162,   143,   118,   120,    23,    24,
      25,    26,    59,    40,   149,    61,    28,    29,    30,   121,
     159,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,   139,     2,
     144,   112,   170,   142,   145,   146,   148,   101,   152,   166,
     165,   147,   158,   120,   172,   170,   101,   120,     3,     4,
       5,     9,   101,     6,     7,   108,   109,   160,     8,     9,
      10,    11,    12,    13,   111,   113,   114,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
     115,    27,   116,   140,    28,    29,    30,    95,    96,    97,
      98,    99,   173,   174,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,   141,   150,   151,   118,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,   167,    88,
      89,    90,    91,    92,    93,    94,   163,   171,   161,   175,
     152,   177,   178,   180,   183,   184,   181,    90,    91,    92,
      93,    94,   186,   185,   187,   189,   182,   188,   107,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,   168,
       0,     0,     3,     4,     5,     0,     0,     6,     7,     0,
       0,     0,     8,     9,    10,    11,    12,    13,     0,     0,
       0,    14,    15,    16,    17,    18,   103,    20,    21,    66,
      23,    24,    25,    26,     0,    27,   117,     0,    28,    29,
      30,     3,     4,     5,     0,     0,     6,     7,     0,     0,
       0,     8,     9,    10,    11,    12,    13,     0,     0,     0,
      14,    15,    16,    17,    18,     0,    20,    21,    66,    23,
      24,    25,    26,     0,    27,   190,     0,    28,    29,    30,
       3,     4,     5,     0,     0,     6,     7,     0,     0,     0,
       8,     9,    10,    11,    12,    13,     0,     0,     0,    14,
      15,    16,    17,    18,     0,    20,    21,    66,    23,    24,
      25,    26,     0,    27,     0,     0,    28,    29,    30,     3,
       4,     5,     0,     0,     6,     7,     0,     0,     0,     8,
      46,    10,    11,    12,     0,     0,     0,     0,     0,     0,
      16,    17,    18,     0,     0,     0,     0,    23,    24,    25,
      26,     0,    40,     0,     0,    28,    29,    30,     3,     4,
       5,     0,     0,     6,     7,     0,     0,     0,     8,     9,
      10,    11,    12,     0,     0,     0,     0,     0,     0,    16,
      17,    18,     0,     0,     0,     0,    23,    24,    25,    26,
       0,    40,     0,     0,    28,    29,    30,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,     0,   105,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   106,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   164,   110,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    80,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94
};

static const yytype_int16 yycheck[] =
{
       1,     3,     4,     5,     6,     7,    30,    22,    30,    31,
      59,    26,     8,    51,    16,    17,    18,    13,    14,    15,
      30,    59,    24,    25,    20,     3,    28,    29,    30,    29,
      52,    53,    30,    48,    27,    59,    30,    31,    30,    31,
      19,    20,    21,    39,    59,    24,    25,    40,    53,    54,
      29,    30,    31,    32,    33,    21,    22,    23,    52,    53,
      52,    40,    41,    42,    51,   100,    67,    54,    47,    48,
      49,    50,    30,    52,   109,    30,    55,    56,    57,    75,
     115,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,     0,
     102,   116,   155,    99,   105,   106,   108,   100,    30,    31,
      51,   107,   114,    54,    51,   168,   109,    54,    19,    20,
      21,    30,   115,    24,    25,    54,    29,   120,    29,    30,
      31,    32,    33,    34,    54,    52,    54,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      29,    52,    30,    30,    55,    56,    57,    24,    25,    26,
      27,    28,   163,   164,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    30,    30,     3,   188,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,   153,    17,
      18,    19,    20,    21,    22,    23,    37,    31,    58,     3,
      30,   166,    31,     3,    52,     3,   171,    19,    20,    21,
      22,    23,     3,   178,     3,     3,   175,   183,    54,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,   154,
      -1,    -1,    19,    20,    21,    -1,    -1,    24,    25,    -1,
      -1,    -1,    29,    30,    31,    32,    33,    34,    -1,    -1,
      -1,    38,    39,    40,    41,    42,    51,    44,    45,    46,
      47,    48,    49,    50,    -1,    52,    53,    -1,    55,    56,
      57,    19,    20,    21,    -1,    -1,    24,    25,    -1,    -1,
      -1,    29,    30,    31,    32,    33,    34,    -1,    -1,    -1,
      38,    39,    40,    41,    42,    -1,    44,    45,    46,    47,
      48,    49,    50,    -1,    52,    53,    -1,    55,    56,    57,
      19,    20,    21,    -1,    -1,    24,    25,    -1,    -1,    -1,
      29,    30,    31,    32,    33,    34,    -1,    -1,    -1,    38,
      39,    40,    41,    42,    -1,    44,    45,    46,    47,    48,
      49,    50,    -1,    52,    -1,    -1,    55,    56,    57,    19,
      20,    21,    -1,    -1,    24,    25,    -1,    -1,    -1,    29,
      30,    31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,
      40,    41,    42,    -1,    -1,    -1,    -1,    47,    48,    49,
      50,    -1,    52,    -1,    -1,    55,    56,    57,    19,    20,
      21,    -1,    -1,    24,    25,    -1,    -1,    -1,    29,    30,
      31,    32,    33,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      41,    42,    -1,    -1,    -1,    -1,    47,    48,    49,    50,
      -1,    52,    -1,    -1,    55,    56,    57,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    -1,    36,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    35,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    35,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    61,     0,    19,    20,    21,    24,    25,    29,    30,
      31,    32,    33,    34,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    52,    55,    56,
      57,    62,    69,    71,    72,    73,    74,    75,    76,    29,
      52,    74,    74,    74,    74,    74,    30,    72,    59,    72,
      72,    72,    74,    74,    74,    30,    72,    30,    70,    30,
      76,    30,    67,    74,    74,    76,    46,    68,    69,    71,
      77,    74,    74,    74,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    71,    51,    51,    76,    36,    35,    54,    54,    29,
       3,    54,    76,    52,    54,    29,    30,    53,    69,    53,
      54,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      30,    30,    72,    77,    74,    69,    69,    72,    74,    77,
      30,     3,    30,    31,    52,    64,    65,    66,    74,    77,
      71,    58,    51,    37,    35,    51,    31,    65,    64,    53,
      66,    31,    51,    69,    69,     3,    63,    65,    31,    53,
       3,    65,    63,    52,     3,    65,     3,     3,    68,     3,
      53
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    60,    61,    61,    62,    62,    62,    62,    63,    63,
      64,    64,    65,    66,    66,    66,    66,    67,    67,    68,
      68,    69,    69,    69,    69,    69,    69,    69,    69,    69,
      70,    70,    71,    71,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    73,    73,    74,    74,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    74,
      74,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    76,    76,    77,    77
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     9,     2,     6,     0,     2,
       1,     2,     1,     4,     5,     4,     4,     0,     1,     1,
       2,     2,     3,     4,     6,     6,     4,     3,     2,     4,
       1,     3,     0,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     1,     4,     1,     2,     2,
       2,     2,     2,     2,     2,     2,     4,     4,     2,     2,
       2,     3,     3,     4,     2,     3,     3,     2,     4,     5,
       1,     1,     1,     1,     2,     1,     3,     1,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 54 "dbg.y" /* yacc.c:1648  */
    {
			/* make stmnt a root so it isn't collected! */
			mkvar("_thiscmd")->proc = (yyvsp[0].node);
			execute((yyvsp[0].node));
			mkvar("_thiscmd")->proc = nil;
			gc();
			if(interactive)
				Bprint(bout, "acid: ");
		}
#line 1518 "y.tab.c" /* yacc.c:1648  */
    break;

  case 5:
#line 64 "dbg.y" /* yacc.c:1648  */
    {
			(yyvsp[-7].sym)->proc = an(OLIST, (yyvsp[-5].node), (yyvsp[-1].node));
		}
#line 1526 "y.tab.c" /* yacc.c:1648  */
    break;

  case 6:
#line 68 "dbg.y" /* yacc.c:1648  */
    {
			(yyvsp[0].sym)->proc = nil;
		}
#line 1534 "y.tab.c" /* yacc.c:1648  */
    break;

  case 7:
#line 72 "dbg.y" /* yacc.c:1648  */
    {
			defcomplex((yyvsp[-4].node), (yyvsp[-2].node));
		}
#line 1542 "y.tab.c" /* yacc.c:1648  */
    break;

  case 11:
#line 82 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLIST, (yyvsp[-1].node), (yyvsp[0].node));
		}
#line 1550 "y.tab.c" /* yacc.c:1648  */
    break;

  case 12:
#line 88 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONAME, ZN, ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 1559 "y.tab.c" /* yacc.c:1648  */
    break;

  case 13:
#line 95 "dbg.y" /* yacc.c:1648  */
    {
			(yyvsp[-1].node)->store.ival = (yyvsp[-2].ival);
			(yyvsp[-1].node)->store.fmt = (yyvsp[-3].ival);
			(yyval.node) = (yyvsp[-1].node);
		}
#line 1569 "y.tab.c" /* yacc.c:1648  */
    break;

  case 14:
#line 101 "dbg.y" /* yacc.c:1648  */
    {
			(yyvsp[-1].node)->store.ival = (yyvsp[-2].ival);
			(yyvsp[-1].node)->store.fmt = (yyvsp[-4].ival);
			(yyvsp[-1].node)->right = (yyvsp[-3].node);
			(yyval.node) = (yyvsp[-1].node);
		}
#line 1580 "y.tab.c" /* yacc.c:1648  */
    break;

  case 15:
#line 108 "dbg.y" /* yacc.c:1648  */
    {
			(yyvsp[-1].node)->store.ival = (yyvsp[-2].ival);
			(yyvsp[-1].node)->left = (yyvsp[-3].node);
			(yyval.node) = (yyvsp[-1].node);
		}
#line 1590 "y.tab.c" /* yacc.c:1648  */
    break;

  case 16:
#line 114 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCTRUCT, (yyvsp[-2].node), ZN);
		}
#line 1598 "y.tab.c" /* yacc.c:1648  */
    break;

  case 17:
#line 120 "dbg.y" /* yacc.c:1648  */
    { (yyval.sym) = 0; }
#line 1604 "y.tab.c" /* yacc.c:1648  */
    break;

  case 20:
#line 126 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLIST, (yyvsp[-1].node), (yyvsp[0].node));
		}
#line 1612 "y.tab.c" /* yacc.c:1648  */
    break;

  case 22:
#line 133 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = (yyvsp[-1].node);
		}
#line 1620 "y.tab.c" /* yacc.c:1648  */
    break;

  case 23:
#line 137 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OIF, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1628 "y.tab.c" /* yacc.c:1648  */
    break;

  case 24:
#line 141 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OIF, (yyvsp[-4].node), an(OELSE, (yyvsp[-2].node), (yyvsp[0].node)));
		}
#line 1636 "y.tab.c" /* yacc.c:1648  */
    break;

  case 25:
#line 145 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ODO, an(OLIST, (yyvsp[-4].node), (yyvsp[-2].node)), (yyvsp[0].node));
		}
#line 1644 "y.tab.c" /* yacc.c:1648  */
    break;

  case 26:
#line 149 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OWHILE, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1652 "y.tab.c" /* yacc.c:1648  */
    break;

  case 27:
#line 153 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ORET, (yyvsp[-1].node), ZN);
		}
#line 1660 "y.tab.c" /* yacc.c:1648  */
    break;

  case 28:
#line 157 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLOCAL, (yyvsp[0].node), ZN);
		}
#line 1668 "y.tab.c" /* yacc.c:1648  */
    break;

  case 29:
#line 161 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCOMPLEX, (yyvsp[-1].node), ZN);
			(yyval.node)->sym = (yyvsp[-2].sym);
		}
#line 1677 "y.tab.c" /* yacc.c:1648  */
    break;

  case 30:
#line 168 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONAME, ZN, ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 1686 "y.tab.c" /* yacc.c:1648  */
    break;

  case 31:
#line 173 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONAME, (yyvsp[-2].node), ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 1695 "y.tab.c" /* yacc.c:1648  */
    break;

  case 32:
#line 180 "dbg.y" /* yacc.c:1648  */
    { (yyval.node) = 0; }
#line 1701 "y.tab.c" /* yacc.c:1648  */
    break;

  case 35:
#line 186 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OMUL, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1709 "y.tab.c" /* yacc.c:1648  */
    break;

  case 36:
#line 190 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ODIV, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1717 "y.tab.c" /* yacc.c:1648  */
    break;

  case 37:
#line 194 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OMOD, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1725 "y.tab.c" /* yacc.c:1648  */
    break;

  case 38:
#line 198 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OADD, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1733 "y.tab.c" /* yacc.c:1648  */
    break;

  case 39:
#line 202 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OSUB, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1741 "y.tab.c" /* yacc.c:1648  */
    break;

  case 40:
#line 206 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ORSH, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1749 "y.tab.c" /* yacc.c:1648  */
    break;

  case 41:
#line 210 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLSH, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1757 "y.tab.c" /* yacc.c:1648  */
    break;

  case 42:
#line 214 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLT, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1765 "y.tab.c" /* yacc.c:1648  */
    break;

  case 43:
#line 218 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OGT, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1773 "y.tab.c" /* yacc.c:1648  */
    break;

  case 44:
#line 222 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLEQ, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1781 "y.tab.c" /* yacc.c:1648  */
    break;

  case 45:
#line 226 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OGEQ, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1789 "y.tab.c" /* yacc.c:1648  */
    break;

  case 46:
#line 230 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OEQ, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1797 "y.tab.c" /* yacc.c:1648  */
    break;

  case 47:
#line 234 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONEQ, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1805 "y.tab.c" /* yacc.c:1648  */
    break;

  case 48:
#line 238 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLAND, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1813 "y.tab.c" /* yacc.c:1648  */
    break;

  case 49:
#line 242 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OXOR, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1821 "y.tab.c" /* yacc.c:1648  */
    break;

  case 50:
#line 246 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLOR, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1829 "y.tab.c" /* yacc.c:1648  */
    break;

  case 51:
#line 250 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCAND, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1837 "y.tab.c" /* yacc.c:1648  */
    break;

  case 52:
#line 254 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCOR, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1845 "y.tab.c" /* yacc.c:1648  */
    break;

  case 53:
#line 258 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OASGN, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1853 "y.tab.c" /* yacc.c:1648  */
    break;

  case 54:
#line 262 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OFMT, (yyvsp[-1].node), con((yyvsp[0].ival)));
		}
#line 1861 "y.tab.c" /* yacc.c:1648  */
    break;

  case 56:
#line 269 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCAST, (yyvsp[0].node), ZN);
			(yyval.node)->sym = (yyvsp[-2].sym);
		}
#line 1870 "y.tab.c" /* yacc.c:1648  */
    break;

  case 58:
#line 277 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OINDM, (yyvsp[0].node), ZN);
		}
#line 1878 "y.tab.c" /* yacc.c:1648  */
    break;

  case 59:
#line 281 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OINDC, (yyvsp[0].node), ZN);
		}
#line 1886 "y.tab.c" /* yacc.c:1648  */
    break;

  case 60:
#line 285 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OADD, (yyvsp[0].node), ZN);
		}
#line 1894 "y.tab.c" /* yacc.c:1648  */
    break;

  case 61:
#line 289 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = con(0);
			(yyval.node) = an(OSUB, (yyval.node), (yyvsp[0].node));
		}
#line 1903 "y.tab.c" /* yacc.c:1648  */
    break;

  case 62:
#line 294 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OEDEC, (yyvsp[0].node), ZN);
		}
#line 1911 "y.tab.c" /* yacc.c:1648  */
    break;

  case 63:
#line 298 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OEINC, (yyvsp[0].node), ZN);
		}
#line 1919 "y.tab.c" /* yacc.c:1648  */
    break;

  case 64:
#line 302 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OHEAD, (yyvsp[0].node), ZN);
		}
#line 1927 "y.tab.c" /* yacc.c:1648  */
    break;

  case 65:
#line 306 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OTAIL, (yyvsp[0].node), ZN);
		}
#line 1935 "y.tab.c" /* yacc.c:1648  */
    break;

  case 66:
#line 310 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OAPPEND, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1943 "y.tab.c" /* yacc.c:1648  */
    break;

  case 67:
#line 314 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ODELETE, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 1951 "y.tab.c" /* yacc.c:1648  */
    break;

  case 68:
#line 318 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONOT, (yyvsp[0].node), ZN);
		}
#line 1959 "y.tab.c" /* yacc.c:1648  */
    break;

  case 69:
#line 322 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OXOR, (yyvsp[0].node), con(-1));
		}
#line 1967 "y.tab.c" /* yacc.c:1648  */
    break;

  case 70:
#line 326 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OEVAL, (yyvsp[0].node), ZN);
		}
#line 1975 "y.tab.c" /* yacc.c:1648  */
    break;

  case 71:
#line 332 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = (yyvsp[-1].node);
		}
#line 1983 "y.tab.c" /* yacc.c:1648  */
    break;

  case 72:
#line 336 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCTRUCT, (yyvsp[-1].node), ZN);
		}
#line 1991 "y.tab.c" /* yacc.c:1648  */
    break;

  case 73:
#line 340 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OINDEX, (yyvsp[-3].node), (yyvsp[-1].node));
		}
#line 1999 "y.tab.c" /* yacc.c:1648  */
    break;

  case 74:
#line 344 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OPDEC, (yyvsp[-1].node), ZN);
		}
#line 2007 "y.tab.c" /* yacc.c:1648  */
    break;

  case 75:
#line 348 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ODOT, (yyvsp[-2].node), ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 2016 "y.tab.c" /* yacc.c:1648  */
    break;

  case 76:
#line 353 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ODOT, an(OINDM, (yyvsp[-2].node), ZN), ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 2025 "y.tab.c" /* yacc.c:1648  */
    break;

  case 77:
#line 358 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OPINC, (yyvsp[-1].node), ZN);
		}
#line 2033 "y.tab.c" /* yacc.c:1648  */
    break;

  case 78:
#line 362 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCALL, (yyvsp[-3].node), (yyvsp[-1].node));
		}
#line 2041 "y.tab.c" /* yacc.c:1648  */
    break;

  case 79:
#line 366 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCALL, (yyvsp[-3].node), (yyvsp[-1].node));
			(yyval.node)->builtin = 1;
		}
#line 2050 "y.tab.c" /* yacc.c:1648  */
    break;

  case 81:
#line 372 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = con((yyvsp[0].ival));
		}
#line 2058 "y.tab.c" /* yacc.c:1648  */
    break;

  case 82:
#line 376 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCONST, ZN, ZN);
			(yyval.node)->type = TFLOAT;
			(yyval.node)->store.fmt = 'f';
			(yyval.node)->store.fval = (yyvsp[0].fval);
		}
#line 2069 "y.tab.c" /* yacc.c:1648  */
    break;

  case 83:
#line 383 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OCONST, ZN, ZN);
			(yyval.node)->type = TSTRING;
			(yyval.node)->store.string = (yyvsp[0].string);
			(yyval.node)->store.fmt = 's';
		}
#line 2080 "y.tab.c" /* yacc.c:1648  */
    break;

  case 84:
#line 390 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OWHAT, ZN, ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 2089 "y.tab.c" /* yacc.c:1648  */
    break;

  case 85:
#line 397 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(ONAME, ZN, ZN);
			(yyval.node)->sym = (yyvsp[0].sym);
		}
#line 2098 "y.tab.c" /* yacc.c:1648  */
    break;

  case 86:
#line 402 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OFRAME, (yyvsp[0].node), ZN);
			(yyval.node)->sym = (yyvsp[-2].sym);
		}
#line 2107 "y.tab.c" /* yacc.c:1648  */
    break;

  case 88:
#line 410 "dbg.y" /* yacc.c:1648  */
    {
			(yyval.node) = an(OLIST, (yyvsp[-2].node), (yyvsp[0].node));
		}
#line 2115 "y.tab.c" /* yacc.c:1648  */
    break;


#line 2119 "y.tab.c" /* yacc.c:1648  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
