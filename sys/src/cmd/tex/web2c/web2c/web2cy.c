#define	array_tok	57346
#define	begin_tok	57347
#define	case_tok	57348
#define	const_tok	57349
#define	do_tok	57350
#define	downto_tok	57351
#define	else_tok	57352
#define	end_tok	57353
#define	file_tok	57354
#define	for_tok	57355
#define	function_tok	57356
#define	goto_tok	57357
#define	if_tok	57358
#define	label_tok	57359
#define	of_tok	57360
#define	procedure_tok	57361
#define	program_tok	57362
#define	record_tok	57363
#define	repeat_tok	57364
#define	then_tok	57365
#define	to_tok	57366
#define	type_tok	57367
#define	until_tok	57368
#define	var_tok	57369
#define	while_tok	57370
#define	integer_tok	57371
#define	real_tok	57372
#define	others_tok	57373
#define	r_num_tok	57374
#define	i_num_tok	57375
#define	string_literal_tok	57376
#define	single_char_tok	57377
#define	assign_tok	57378
#define	two_dots_tok	57379
#define	unknown_tok	57380
#define	undef_id_tok	57381
#define	var_id_tok	57382
#define	proc_id_tok	57383
#define	proc_param_tok	57384
#define	fun_id_tok	57385
#define	fun_param_tok	57386
#define	const_id_tok	57387
#define	type_id_tok	57388
#define	hhb0_tok	57389
#define	hhb1_tok	57390
#define	field_id_tok	57391
#define	define_tok	57392
#define	field_tok	57393
#define	break_tok	57394
#define	not_eq_tok	57395
#define	less_eq_tok	57396
#define	great_eq_tok	57397
#define	or_tok	57398
#define	unary_plus_tok	57399
#define	unary_minus_tok	57400
#define	div_tok	57401
#define	mod_tok	57402
#define	and_tok	57403
#define	not_tok	57404

#line	18	"web2c.yacc"
#include "web2c.h"

#define	symbol(x)	sym_table[x].id
#define	MAX_ARGS	50

static char function_return_type[50], for_stack[300], control_var[50],
            relation[3];
static char arg_type[MAX_ARGS][30];
static int last_type = -1, ids_typed;
char my_routine[100];	/* Name of routine being parsed, if any */
static char array_bounds[80], array_offset[80];
static int uses_mem, uses_eqtb, lower_sym, upper_sym;
static FILE *orig_std;
boolean doing_statements = FALSE;
static boolean var_formals = FALSE;
static int param_id_list[MAX_ARGS], ids_paramed=0;

extern char conditional[], temp[], *std_header;
extern int tex, mf, strict_for;
extern boolean ansi;
extern FILE *coerce;
extern char coerce_name[];
extern boolean debug;

/* Forward refs */
#ifdef	ANSI
static long labs(long x);
static void compute_array_bounds(void);
static void fixup_var_list(void);
static void do_proc_args(void);
static void gen_function_head(void);
static boolean doreturn(char *label);
extern int yylex(void);
#else	/* Not ANSI */
static long labs();
static void compute_array_bounds(), fixup_var_list();
static void do_proc_args(), gen_function_head();
static boolean doreturn();
#endif	/* Not ANSI */

extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
#ifndef	YYSTYPE
#define	YYSTYPE	int
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define YYEOFCODE 1
#define YYERRCODE 2

#line	1094	"web2c.yacc"


static void compute_array_bounds()
{
    long lb;
    char tmp[200];

    if (lower_sym == -1) {	/* lower is a constant */
	lb = lower_bound - 1;
	if (lb==0) lb = -1;	/* Treat lower_bound==1 as if lower_bound==0 */
	if (upper_sym == -1)	/* both constants */
	    (void) sprintf(tmp, "[%ld]", upper_bound - lb);
	else {			/* upper a symbol, lower constant */
	    if (lb < 0)
		(void) sprintf(tmp, "[%s + %ld]",
				symbol(upper_sym), (-lb));
	    else
		(void) sprintf(tmp, "[%s - %ld]",
				symbol(upper_sym), lb);
	}
	if (lower_bound < 0 || lower_bound > 1) {
	    if (*array_bounds) {
		fprintf(stderr, "Cannot handle offset in second dimension\n");
		exit(1);
	    }
	    if (lower_bound < 0) {
		(void) sprintf(array_offset, "+%ld", -lower_bound);
	    } else {
		(void) sprintf(array_offset, "-%ld", lower_bound);
	    }
	}
	(void) strcat(array_bounds, tmp);
    }
    else {			/* lower is a symbol */
	if (upper_sym != -1)	/* both are symbols */
	    (void) sprintf(tmp, "[%s - %s + 1]", symbol(upper_sym),
		symbol(lower_sym));
	else {			/* upper constant, lower symbol */
	    (void) sprintf(tmp, "[%ld - %s]", upper_bound + 1,
		symbol(lower_sym));
	}
	if (*array_bounds) {
	    fprintf(stderr, "Cannot handle symbolic offset in second dimension\n");
	    exit(1);
	}
	(void) sprintf(array_offset, "- (int)(%s)", symbol(lower_sym));
	(void) strcat(array_bounds, tmp);
    }
}

static void fixup_var_list()
{
    int i, j;
    char output_string[100], real_symbol[100];

    for (i=0; var_list[i++] == '!'; ) {
	for (j=0; real_symbol[j++] = var_list[i++];);
	if (*array_offset) {
	    (void) fprintf(std, "\n#define %s (%s %s)\n  ",
	        real_symbol, next_temp, array_offset);
	    (void) strcpy(real_symbol, next_temp);
	    find_next_temp();
	}
	(void) sprintf(output_string, "%s%s%c",
	    real_symbol, array_bounds, (var_list[i]=='!' ? ',' : ' '));
	my_output(output_string);
    }
    semicolon();
}


/*
 * If we're not processing TeX, we return 0 (false).  Otherwise,
 * return 1 if the label is "10" and we're not in one of four TeX
 * routines where the line labeled "10" isn't the end of the routine.
 * Otherwise, return 0.
 */
static boolean doreturn(label)
char *label;
{
    if (!tex) return(FALSE);
    if (strcmp(label, "10")) return(FALSE);
    if (strcmp(my_routine, "macrocall") == 0) return(FALSE);
    if (strcmp(my_routine, "hpack") == 0) return(FALSE);
    if (strcmp(my_routine, "vpackage") == 0) return(FALSE);
    if (strcmp(my_routine, "trybreak") == 0) return(FALSE);
    return(TRUE);
}


/* Return the absolute value of a long */
static long labs(x)
long x;
{
    if (x < 0L) return(-x);
    return(x);
}

static void do_proc_args()
{
    int i;

    if (ansi) {
	fprintf(coerce, "%s %s(", function_return_type, z_id);
	if (ids_paramed == 0) fprintf(coerce, "void");
	for (i=0; i<ids_paramed; i++) {
	    if (i > 0) putc(',', coerce);
	    fprintf(coerce, "%s %s",
		arg_type[i],
		symbol(param_id_list[i]));
	}
	fprintf(coerce, ");\n");
    } else
	fprintf(coerce, "%s %s();\n", function_return_type, z_id);
}

static void gen_function_head()
{
    int i;

    if (strcmp(my_routine, z_id)) {
	fprintf(coerce, "#define %s(", my_routine);
	for (i=0; i<ids_paramed; i++) {
	    if (i > 0)
		fprintf(coerce, ", %s", symbol(param_id_list[i]));
	    else
		fprintf(coerce, "%s", symbol(param_id_list[i]));
	}
	fprintf(coerce, ") %s(", z_id);
	for (i=0; i<ids_paramed; i++) {
	    if (i > 0)
		fprintf(coerce, ", (%s) %s(%s)",
		    arg_type[i],
		    sym_table[param_id_list[i]].var_formal?"&":"",
		    symbol(param_id_list[i]));
	    else
		fprintf(coerce, "(%s) %s(%s)",
		    arg_type[i],
		    sym_table[param_id_list[i]].var_formal?"&":"",
		    symbol(param_id_list[i]));
	}
	fprintf(coerce, ")\n");
    }
    std = orig_std;
    my_output(z_id);
    my_output("(");
    if (ansi) {
	for (i=0; i<ids_paramed; i++) {
	    if (i > 0) my_output(",");
	    my_output(arg_type[i]);
	    my_output(symbol(param_id_list[i]));
	}
	my_output(")");
	indent_line();
    } else {	/* Not ansi */
	for (i=0; i<ids_paramed; i++) {
	    if (i > 0) my_output(",");
	    my_output(symbol(param_id_list[i]));
	}
	my_output(")");
	indent_line();
	for (i=0; i<ids_paramed; i++) {
	    my_output(arg_type[i]);
	    my_output(symbol(param_id_list[i]));
	    semicolon();
	}
    }
}
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
-1, 38,
	39, 30,
	-2, 27,
-1, 53,
	39, 44,
	-2, 41,
-1, 66,
	39, 86,
	40, 86,
	49, 86,
	-2, 83,
-1, 154,
	75, 150,
	78, 150,
	-2, 152,
-1, 208,
	39, 72,
	49, 72,
	-2, 75,
-1, 299,
	39, 72,
	49, 72,
	-2, 75,
-1, 352,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 175,
-1, 353,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 177,
-1, 355,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 181,
-1, 356,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 183,
-1, 357,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 185,
-1, 358,
	53, 0,
	54, 0,
	55, 0,
	56, 0,
	57, 0,
	58, 0,
	-2, 187,
-1, 372,
	24, 257,
	-2, 260,
};
#define	YYNPROD	263
#define	YYPRIVATE 57344
#define	YYLAST	542
short	yyact[] =
{
 307, 294, 379, 232, 365, 128, 129, 306, 301, 363,
 127, 172, 290, 243,  83, 217, 293,  51, 221,  36,
 164,  24,  84,  15, 252, 233,  95, 248, 269, 270,
 272, 273, 274, 275, 265, 266, 277, 212, 205, 267,
 278, 268, 271, 276, 222, 391,  46, 223, 347, 390,
 204, 346, 269, 270, 272, 273, 274, 275, 265, 266,
 277, 186, 381, 267, 278, 268, 271, 276, 107, 340,
 107, 378, 376, 339, 106, 207, 344, 269, 270, 272,
 273, 274, 275, 265, 266, 277,  75,  45, 267, 278,
 268, 271, 276, 289, 297, 288, 179, 296, 341, 342,
 410,  44,  58, 377, 181,  59,  97, 267, 278, 268,
 271, 276, 218, 180, 408, 109,  31,  32,  28,  29,
 118, 142, 165, 389, 211, 169, 298, 183, 284, 258,
 124, 206, 203, 184, 201, 126,  34,  49, 178,  94,
 163, 166, 167, 168, 269, 270, 272, 273, 274, 275,
 265, 266, 277,  33,  50, 267, 278, 268, 271, 276,
  93,  91,  67, 368,  62,  61, 182,  60, 111, 110,
 114, 115, 185, 185,  35,  30, 154,  27,  17, 234,
 235, 116, 388, 200,  54, 299,  49, 250, 185, 142,
 142, 213, 185, 214,  47, 142, 225, 236, 228, 229,
 240, 142, 219,  50,  49, 230,  11, 231, 237, 108,
  89, 396, 210,  10, 239,  42, 255, 256,  12, 249,
  70,  50,  85,  86,  13,  63,  14, 280, 142,  82,
 264, 279,  87, 262, 263, 261,  57,  23,  69, 259,
 269, 270, 272, 273, 274, 275, 265, 266, 277,  22,
   9, 267, 278, 268, 271, 276, 304,   5,  73, 287,
  98, 308, 100, 101, 295,  21, 269, 270, 272, 273,
 274, 275, 265, 266, 277,  72, 285, 267, 278, 268,
 271, 276, 324, 265, 266, 277, 302,   6, 267, 278,
 268, 271, 276,  20,  19, 102, 303,  18, 325, 104,
 105,   8, 188, 331,  39, 333, 332, 187, 385, 249,
 348, 349, 350, 351, 352, 353, 354, 355, 356, 357,
 358, 359, 360, 361, 338, 337, 190,  43, 370, 372,
 142,  64, 369, 142, 402, 373, 148, 158, 380, 111,
 110, 114, 115,  56, 161, 245, 147, 162, 367, 383,
 366,  52, 116, 160, 367,  37, 366, 406, 329, 159,
  81, 397, 336, 209, 133,  80,  16,  92, 194, 407,
 145, 154, 144, 146, 155, 156,  81, 392, 326, 142,
  25,  80, 375, 138, 398, 413, 395, 400, 394, 412,
 393, 405, 399, 371, 142, 328, 404, 403, 401, 380,
 409, 238, 198, 327, 197, 283, 196, 148, 158, 142,
 153, 411, 152, 414, 415, 161, 151, 147, 162, 364,
 387, 362, 323, 195, 160, 224, 286, 199, 193, 157,
 159, 150, 149, 141, 140, 189, 343, 384, 305, 257,
 282, 145, 154, 144, 146, 155, 156, 281, 227, 322,
 321, 320, 319, 318, 138, 317, 316, 315, 314, 313,
 312, 311, 310, 309, 226, 386, 345, 260, 220, 192,
 216, 143, 215, 137, 136, 135, 134, 132, 131, 130,
 191, 139, 335, 247, 122, 103, 334, 246, 121,  79,
 292, 244, 291, 242, 202, 120,  99, 119,  78,  77,
  76, 117, 123,  68,  66, 254, 382, 300, 253, 251,
 208, 177, 176, 175, 174,  48, 173, 171, 170, 125,
  88,  55,  53, 113, 112,  71,  40,  38,  41,  26,
 374, 330, 241,  96,   4,  90,  74,  65,   7,   3,
   2,   1
};
short	yypact[] =
{
-1000,-1000, 237,-1000,-1000, 262, 199, 349, 108, 258,
 255, 254, 226, 210, 198, 373,-1000,-1000, 107,  48,
 105,  46,  83, 104, 330,-1000, 294,-1000,-1000,  29,
-1000,-1000,  15,-1000, 159,-1000, 324,-1000,-1000,-1000,
 197,  32,-1000,-1000,  97,  95,  94, 188, 298,-1000,
-1000,-1000,-1000,-1000,-1000, 181,-1000,-1000,-1000, 294,
-1000,-1000,-1000, 159,-1000, 346,-1000,-1000, 183,-1000,
-1000, 157,-1000,-1000, 362,-1000,  90,  69,-1000,-1000,
 221, 256,-1000,  -3,-1000,-1000,-1000,-1000, 156, 307,
-1000,-1000,-1000,-1000,-1000,-1000, 349,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 183,-1000,  65,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 331, 373,  51,
  51,  51,  51,  92,-1000,  92,-1000, 122,-1000,-1000,
 -16,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000, 271, 266,-1000,-1000,-1000, 293,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 358,-1000,-1000,
-1000,-1000,-1000, 330,  64,-1000,  62, -27, -39,  61,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   0,
-1000, 345, 166,  54, -41, 331, 402,-1000,-1000,  41,
-1000, 331, -31,-1000,-1000, 136, 136, 331, 174, 136,
-1000,-1000, 318,-1000,-1000,-1000,-1000, 141,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, 136, 136,-1000,-1000, 118,
 -31,-1000,-1000, 186, 331, 213, 136,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, 187, 102, 240,-1000,
 187, 324,  23,-1000,-1000,-1000,  92,  92,  21,-1000,
-1000, 115,-1000, 247,  92, 187, 187, 136,-1000,-1000,
 136,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 136,  41, 370,-1000,-1000, 335,-1000,-1000, 318,
-1000, 183,-1000,-1000,-1000,-1000, 344, 141,-1000,-1000,
  -4,-1000,-1000,-1000,-1000,  26,-1000,  -1, -25, 136,
 136, 136, 136, 136, 136, 136, 136, 136, 136, 136,
 136, 136, 136, 317,  91,-1000, 331, 136, 136, 331,
 377,-1000,  -5,-1000,  33,   1,  92, -14,-1000,-1000,
 247,-1000,-1000,-1000, 275,-1000,-1000,-1000,  43,  43,
-1000,-1000, 224, 224,-1000, 224, 224, 224, 224,-1000,
  43,-1000, 112,-1000, -28,-1000,-1000,-1000,-1000,-1000,
 187, 369, 187,-1000,-1000, 331, 165,-1000,-1000,-1000,
-1000, 343,  92,-1000, 136,-1000, 136,-1000, 323,-1000,
 402, 317,-1000, 333, 360, 103,-1000,  92,-1000,-1000,
  24,-1000,-1000,-1000,-1000, 331,-1000,-1000,-1000,-1000,
-1000,-1000, 136, 136, 187, 187
};
short	yypgo[] =
{
   0, 541, 540, 539, 538,  23,  21,  19,  17, 537,
 536, 535, 534,  11,  26, 533, 532, 531, 530, 529,
 528, 215, 527, 304, 526, 525,  25, 524, 523, 522,
 184, 521, 520, 519,   1, 518, 517, 516, 194, 515,
 514, 513, 512, 511,  27,   2, 510, 509,  24, 508,
 507, 506,   8, 505, 504, 162, 503,  14, 502,  22,
 501,  10,  86, 500, 499, 498, 497,  20, 496, 495,
 494, 493,  13,  12, 492, 491, 490, 489, 488, 487,
  16, 486, 485, 484, 483, 482, 481, 480,   5,   6,
 479, 478, 477, 476, 475, 474, 473,   3, 472,   0,
 471, 470, 469, 468,  18, 467, 466, 465, 464, 463,
 462, 461, 460, 459, 458, 457, 456, 455, 453, 452,
 451, 450, 449, 448, 447, 440,  15, 439, 438,   7,
 437, 436, 435, 434, 433, 432, 431, 429, 428, 427,
 426, 425, 423, 422, 421, 420,   9, 419,   4, 416,
 412, 410, 406, 405, 404, 403, 402, 401, 395, 393,
 391, 390, 389, 388, 385
};
short	yyr1[] =
{
   0,   4,   9,   1,   2,   2,  12,  12,  12,  12,
  12,  12,  12,  12,  12,   3,  15,  16,  17,  14,
   5,  19,   5,  20,  20,  21,   6,   6,  22,  22,
  24,  25,  23,  26,  26,  26,  26,  27,  27,  28,
   7,   7,  29,  29,  31,  32,  33,  30,  34,  34,
  35,  35,  13,  39,  39,  38,  38,  37,  36,  36,
  36,  36,  43,  40,  40,  44,  44,  45,  46,  41,
  47,  47,  49,  51,  48,  48,  50,  50,  52,  52,
  53,  42,   8,   8,  54,  54,  56,  58,  55,  57,
  57,  59,  59,  59,  11,  60,  11,  10,  10,  62,
  62,  63,  66,  65,  69,  65,  67,  70,  67,  71,
  71,  74,  73,  75,  72,  76,  72,  68,  68,  64,
  78,  79,  81,  77,  83,  84,  85,  77,  82,  82,
  80,  18,  87,  86,  61,  61,  88,  88,  90,  89,
  89,  91,  91,  91,  91,  91,  98,  93, 101,  93,
 102,  97,  97, 100, 100, 103, 103, 105, 104, 104,
 104, 104, 106, 107, 106,  99, 109,  99, 110,  99,
 111,  99, 112,  99, 113,  99, 114,  99, 115,  99,
 116,  99, 117,  99, 118,  99, 119,  99, 120,  99,
 121,  99, 122,  99,  99, 108, 108, 108, 124, 123,
 123, 123, 123, 125, 123, 127, 126, 128, 130, 128,
 129, 131, 131,  94,  94, 132,  94,  95,  96,  92,
  92,  92, 133, 133, 135, 135, 139, 140, 137, 141,
 138, 142, 143, 136, 144, 144, 146, 147, 147, 148,
 148, 145, 145, 134, 134, 134, 152, 153, 149, 154,
 155, 150, 156, 158, 160, 151, 157, 161, 162, 159,
 163, 164, 159
};
short	yyr2[] =
{
   0,   0,   0,  10,   0,   2,   4,   4,   4,   6,
   4,   6,   4,   6,   4,   3,   0,   0,   0,   8,
   0,   0,   4,   1,   3,   1,   0,   2,   1,   2,
   0,   0,   6,   1,   1,   1,   1,   1,   1,   1,
   0,   2,   1,   2,   0,   0,   0,   7,   1,   1,
   1,   1,   3,   0,   1,   2,   1,   1,   1,   1,
   1,   1,   2,   6,   8,   1,   1,   1,   0,   4,
   1,   3,   0,   0,   5,   0,   1,   3,   1,   1,
   0,   4,   0,   2,   1,   2,   0,   0,   6,   1,
   3,   1,   1,   1,   0,   0,   5,   1,   2,   2,
   2,   2,   0,   5,   0,   5,   0,   0,   4,   1,
   3,   0,   4,   0,   2,   0,   3,   1,   1,   2,
   0,   0,   0,   9,   0,   0,   0,   9,   1,   1,
   1,   3,   0,   4,   1,   3,   1,   3,   1,   1,
   1,   1,   1,   1,   1,   1,   0,   4,   0,   4,
   0,   3,   1,   1,   1,   1,   2,   0,   4,   2,
   2,   2,   1,   0,   4,   2,   0,   4,   0,   4,
   0,   4,   0,   4,   0,   4,   0,   4,   0,   4,
   0,   4,   0,   4,   0,   4,   0,   4,   0,   4,
   0,   4,   0,   4,   1,   1,   1,   1,   0,   4,
   1,   1,   1,   0,   3,   0,   4,   1,   0,   4,
   2,   2,   0,   1,   1,   0,   3,   2,   0,   1,
   1,   1,   1,   1,   1,   2,   0,   0,   6,   0,
   3,   0,   0,   7,   1,   3,   3,   1,   3,   1,
   1,   1,   2,   1,   1,   1,   0,   0,   6,   0,
   0,   6,   0,   0,   0,   9,   1,   0,   0,   5,
   0,   0,   5
};
short	yychk[] =
{
-1000,  -1,  -2,  -3, -12,  20,  50,  -4,  39,  51,
  14,   7,  19,  25,  27,  -5,  17,  70,  39,  39,
  39,  39,  39,  39,  -6,   7, -19,  70,  70,  71,
  70,  70,  71,  70,  53,  70,  -7,  25, -22, -23,
 -24, -20, -21,  33,  72,  72, -13, -38, -39,  45,
  62,  -8,  27, -29, -30, -31, -23,  39,  70,  73,
  70,  70,  70,  37,  33,  -9, -54, -55, -56, -30,
  39, -25, -21, -38, -10, -62, -63, -64, -65, -77,
  19,  14, -55, -57, -59,  39,  40,  49, -32,  53,
 -11, -62,   5,  70,  70, -14, -15, -14,  39, -68,
  41,  42,  39, -82,  43,  44,  77,  73,  53, -26,
  33,  32, -27, -28,  34,  35,  45, -60,  -5, -66,
 -69, -78, -83, -58, -59, -33,  70, -61, -88, -89,
 -90, -91, -92,  33, -93, -94, -95, -96,  52, -86,
-133,-134, -97,-100,  41,  39,  42,  15,   5,-135,
-136,-149,-150,-151,  40,  43,  44,-137,   6,  28,
  22,  13,  16,  -6, -67,  71, -67, -67, -67, -34,
 -35, -36, -13, -37, -40, -41, -42, -43,  46,   4,
  21,  12,  74, -34,  11,  70,  77,  36,  36,-132,
  33, -87,-102,-138,  10,-142,-152,-154,-156,-139,
  -7,  70, -70,  70,  77,  77,  70,  75, -46,  18,
  46,  70,  78, -88, -89, -98,-101,-126,  71, -61,
-103,-104,  75,  78,-141, -99,-108,-123,  62,  63,
  69,  71, -97, -26,  43,  44, -99, -61,-157,  40,
 -99, -16, -71, -72, -75,  27, -79, -84, -44, -13,
  46, -47, -48, -49, -53, -99, -99,-127,  11,-104,
-105,  49,  47,  48, -88,  59,  60,  64,  66,  53,
  54,  67,  55,  56,  57,  58,  68,  61,  65,  18,
 -99,-124,-125,-153,  26,  36,-140,  -8,  72,  70,
 -73, -74, -76, -80, -34, -80,  76,  73,  11,  70,
 -50, -52,  39,  49, -34,-128,-129, -99, -99,-109,
-110,-111,-112,-113,-114,-115,-116,-117,-118,-119,
-120,-121,-122,-143, -99,-126,   8,-155,-158,  23,
 -17, -72, -57, -73, -81, -85,  18, -44, -48,  77,
  73,  72,  73,-131,  77,-106,  76,  73, -99, -99,
 -99, -99, -99, -99, -99, -99, -99, -99, -99, -99,
 -99, -99,-144,-146,-147,-148,  33,  31,  72, -88,
 -99,-159, -99, -88, -18,   5,  77,  70,  70, -45,
 -34,  76, -51, -52,-130,  33,-107,-145,  70,  11,
  77,  73,   8,-161,-163, -61,  46,  18, -34,-129,
 -99,-146,  11, -89,-148,-160,  24,   9,  11, -45,
  76, -88,-162,-164, -99, -99
};
short	yydef[] =
{
   4,  -2,   0,   1,   5,   0,   0,  20,   0,   0,
   0,   0,   0,   0,   0,  26,  21,  15,   0,   0,
   0,   0,   0,   0,  40,  30,   0,   6,   7,   0,
   8,  10,   0,  12,  53,  14,  82,  44,  -2,  28,
   0,   0,  23,  25,   0,   0,   0,   0,   0,  56,
  54,   2,  86,  -2,  42,   0,  29,  31,  22,   0,
   9,  11,  13,  53,  55,   0,  -2,  84,   0,  43,
  45,   0,  24,  52,  94,  97,   0,   0,  16,  16,
   0,   0,  85,   0,  89,  91,  92,  93,   0,   0,
   3,  98,  95,  99, 100, 101,  20, 119, 102, 104,
 117, 118, 120, 124, 128, 129,  87,   0,  46,   0,
  33,  34,  35,  36,  37,  38,  39, 218,  26, 106,
 106, 106, 106,  53,  90,  53,  32,   0, 134, 136,
   0, 139, 140, 138, 141, 142, 143, 144, 145, 219,
 220, 221,   0,   0, 213, 214, 215,   0, 132, 222,
 223, 243, 244, 245,  -2, 153, 154, 224, 231, 246,
 249, 252, 226,  40,   0, 107,   0,   0,   0,   0,
  48,  49,  50,  51,  58,  59,  60,  61,  57,   0,
  68,   0,   0,   0,   0, 218, 218, 146, 148,   0,
 217, 218,   0, 225, 229,   0,   0, 218,   0,   0,
  17, 103, 113, 105, 121, 125,  88,  53,  -2,  80,
  62,  47,  96, 135, 137,   0,   0, 216, 205,   0,
 151, 155, 157,   0, 218,   0,   0, 194, 195, 196,
 197, 198, 200, 201, 202, 203, 247,   0,   0, 256,
 227,  82,   0, 109, 111, 115,  53,  53,   0,  65,
  66,   0,  70,   0,  53, 147, 149,   0, 133, 156,
   0, 159, 160, 161, 230, 166, 168, 170, 172, 174,
 176, 178, 180, 182, 184, 186, 188, 190, 192, 232,
 165,   0,   0,   0, 250, 253,   0,  18, 108, 113,
 114,   0, 111, 122, 130, 126,   0,  53,  69,  -2,
   0,  76,  78,  79,  81,   0, 207, 212,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 204, 218,   0,   0, 218,
   0, 110,   0, 116,   0,   0,  53,   0,  71,  73,
   0, 206, 208, 210,   0, 158, 162, 163, 167, 169,
 171, 173,  -2,  -2, 179,  -2,  -2,  -2,  -2, 189,
 191, 193,   0, 234,   0, 237, 239, 240, 199, 248,
 251,   0,  -2, 228,  19, 218,   0, 123, 127,  63,
  67,   0,  53,  77,   0, 211,   0, 233,   0, 241,
 218,   0, 254,   0,   0,   0, 112,  53,  74, 209,
   0, 235, 242, 236, 238, 218, 258, 261, 131,  64,
 164, 255,   0,   0, 259, 262
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  71,  72,  64,  59,  73,  60,  78,  65,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  77,  70,
  55,  53,  56,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  75,   0,  76,  74
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,
  52,  54,  57,  58,  61,  62,  63,  66,  67,  68,
  69
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
#endif

/*	parser for yacc output	*/

int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */

char*	yytoknames[1];		/* for debugging */
char*	yystates[1];		/* for debugging */

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprintf(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprintf(x, "<%d>\n", yys);
	return x;
}

long
yylex1(void)
{
	long yychar;
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		if(yychar <= 0) {
			c = yytok1[0];
			goto out;
		}
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
	if(yydebug >= 3)
		printf("lex %.4X %s\n", yychar, yytokname(c));
	return c;
}

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		printf("char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) { 
		yyerror("yacc stack overflow"); 
		return 1; 
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
		yychar = yylex1();
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
			yychar = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			return 0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 1) {
				printf("%s", yystatname(yystate));
				printf("saw %s\n", yytokname(yychar));
			}
yyerrlab:
			yynerrs++;

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					printf("error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			return 1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= YYEOFCODE)
				printf("error recovery discards %s\n", yytokname(yychar));
			if(yychar == 0)
				return 1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		printf("reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 1:
#line	66	"web2c.yacc"
{block_level++;
			 printf("#include \"%s\"\n", std_header);} break;
case 2:
#line	70	"web2c.yacc"
{printf("\n#include \"%s\"\n", coerce_name); } break;
case 3:
#line	73	"web2c.yacc"
{YYACCEPT;} break;
case 6:
#line	81	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = field_id_tok;
			} break;
case 7:
#line	86	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = fun_id_tok;
			} break;
case 8:
#line	91	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = const_id_tok;
			} break;
case 9:
#line	96	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = fun_param_tok;
			} break;
case 10:
#line	101	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = proc_id_tok;
			} break;
case 11:
#line	106	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = proc_param_tok;
			} break;
case 12:
#line	111	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = type_id_tok;
			} break;
case 13:
#line	117	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = type_id_tok;
			    sym_table[ii].val = lower_bound;
			    sym_table[ii].val_sym = lower_sym;
			    sym_table[ii].upper = upper_bound;
			    sym_table[ii].upper_sym = upper_sym;
			} break;
case 14:
#line	126	"web2c.yacc"
{
			    ii = add_to_table(last_id); 
			    sym_table[ii].typ = var_id_tok;
			} break;
case 16:
#line	136	"web2c.yacc"
{	if (block_level > 0) my_output("{");
			indent++; block_level++;
		      } break;
case 17:
#line	141	"web2c.yacc"
{if (block_level == 2) {
			    if (strcmp(function_return_type, "void")) {
			      my_output("register");
			      my_output(function_return_type);
			      my_output("Result;");
			    }
			    if (tex) {
			      (void) sprintf(safe_string, "%s_regmem",
						my_routine);
			      my_output(safe_string);
			      indent_line();
			    }
			 }
			} break;
case 18:
#line	156	"web2c.yacc"
{if (block_level == 1)
				puts("\n#include \"coerce.h\"");
			 doing_statements = TRUE;
			} break;
case 19:
#line	161	"web2c.yacc"
{if (block_level == 2) {
			    if (strcmp(function_return_type,"void")) {
			      my_output("return(Result)");
			      semicolon();
			     }
			     if (tex) {
			       if (uses_mem && uses_eqtb)
				(void) fprintf(coerce,
	"#define %s_regmem register memoryword *mem=zmem, *eqtb=zeqtb;\n",
				   my_routine);
			       else if (uses_mem)
				(void) fprintf(coerce,
			"#define %s_regmem register memoryword *mem=zmem;\n",
				   my_routine);
			       else if (uses_eqtb)
				(void) fprintf(coerce,
			"#define %s_regmem register memoryword *eqtb=zeqtb;\n",
				   my_routine);
			       else
				(void) fprintf(coerce,
				   "#define %s_regmem\n",
				   my_routine);
			    }
			    my_routine[0] = '\0';
			 }
			 indent--; block_level--;
			 my_output("}"); new_line();
			 doing_statements = FALSE;
			} break;
case 21:
#line	194	"web2c.yacc"
{ my_output("/*"); } break;
case 22:
#line	196	"web2c.yacc"
{ my_output("*/"); } break;
case 25:
#line	204	"web2c.yacc"
{ my_output(temp); } break;
case 27:
#line	209	"web2c.yacc"
{ indent_line(); } break;
case 30:
#line	217	"web2c.yacc"
{ new_line(); my_output("#define"); } break;
case 31:
#line	219	"web2c.yacc"
{ ii=add_to_table(last_id);
				  sym_table[ii].typ = const_id_tok;
				  my_output(last_id);
				} break;
case 32:
#line	224	"web2c.yacc"
{ sym_table[ii].val=last_i_num;
				  new_line(); } break;
case 33:
#line	229	"web2c.yacc"
{
				  (void) sscanf(temp, "%ld", &last_i_num);
				  if (labs((long) last_i_num) > 32767)
				      (void) strcat(temp, "L");
				  my_output(temp);
				  yyval = ex_32;
				} break;
case 34:
#line	237	"web2c.yacc"
{ my_output(temp);
				  yyval = ex_real;
				} break;
case 35:
#line	241	"web2c.yacc"
{ yyval = 0; } break;
case 36:
#line	243	"web2c.yacc"
{ yyval = ex_32; } break;
case 37:
#line	247	"web2c.yacc"
{ int i, j; char s[132];
	  			  j = 1;
				  s[0] = '"';
	  			  for (i=1; yytext[i-1]!=0; i++) {
	  			    if (yytext[i] == '\\' || yytext[i] == '"')
					s[j++]='\\';
	    			    else if (yytext[i] == '\'') i++;
	    			    s[j++] = yytext[i];
				  }
	    			  s[j-1] = '"';
				  s[j] = 0;
				  my_output(s);
				} break;
case 38:
#line	261	"web2c.yacc"
{ char s[5];
				  s[0]='\'';
	    			  if (yytext[1] == '\\' || yytext[1] == '\'') {
	  				s[2] = yytext[1];
					s[1] = '\\';
					s[3] = '\'';
					s[4] = '\0';
				  }
	  			  else {
					s[1] = yytext[1];
					s[2]='\'';
					s[3]='\0';
				  }
	  			  my_output(s);
				} break;
case 39:
#line	279	"web2c.yacc"
{ my_output(last_id); } break;
case 44:
#line	291	"web2c.yacc"
{ my_output("typedef"); } break;
case 45:
#line	293	"web2c.yacc"
{ ii = add_to_table(last_id);
				  sym_table[ii].typ = type_id_tok;
				  (void) strcpy(safe_string, last_id);
				  last_type = ii;
				} break;
case 46:
#line	299	"web2c.yacc"
{
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 47:
#line	304	"web2c.yacc"
{ if (*array_offset) {
			fprintf(stderr, "Cannot typedef arrays with offsets\n");
					exit(1);
				  }
				  my_output(safe_string);
				  my_output(array_bounds);
				  semicolon();
				  last_type = -1;
				} break;
case 50:
#line	320	"web2c.yacc"
{ if (last_type >= 0) {
					sym_table[ii].val = lower_bound;
					sym_table[ii].val_sym = lower_sym;
			 		sym_table[ii].upper = upper_bound;
					sym_table[ii].upper_sym = upper_sym;
					ii= -1;
				  }
/* The following code says: if the bounds are known at translation time
 * on an integral type, then we select the smallest type of data which
 * can represent it in ANSI C.  We only use unsigned types when necessary.
 */
				  if (lower_sym == -1 && upper_sym == -1) {
				    if (lower_bound>= -127 && upper_bound<=127)
					my_output("schar");
				    else if (lower_bound >= 0
				      && upper_bound <= 255)
					my_output("unsigned char");
	  			    else if (lower_bound >= -32767
				      && upper_bound <= 32767)
					my_output("short");
	  			    else if (lower_bound >= 0
				      && upper_bound <= 65535)
					my_output("unsigned short");
	  			    else my_output("integer");
				  }
				  else my_output("integer");
				} break;
case 55:
#line	358	"web2c.yacc"
{lower_bound = upper_bound;
				 lower_sym = upper_sym;
				 (void) sscanf(temp, "%ld", &upper_bound);
				 upper_sym = -1; /* no sym table entry */
				} break;
case 56:
#line	364	"web2c.yacc"
{ lower_bound = upper_bound;
				  lower_sym = upper_sym;
				  upper_bound = sym_table[l_s].val;
				  upper_sym = l_s;
				} break;
case 57:
#line	372	"web2c.yacc"
{if (last_type >= 0) {
	    sym_table[last_type].var_not_needed = sym_table[l_s].var_not_needed;
	    sym_table[last_type].upper = sym_table[l_s].upper;
	    sym_table[last_type].upper_sym = sym_table[l_s].upper_sym;
	    sym_table[last_type].val = sym_table[l_s].val;
	    sym_table[last_type].val_sym = sym_table[l_s].val_sym;
	 }
	 my_output(last_id); } break;
case 58:
#line	383	"web2c.yacc"
{if (last_type >= 0)
				    sym_table[last_type].var_not_needed = TRUE;} break;
case 60:
#line	387	"web2c.yacc"
{if (last_type >= 0)
				    sym_table[last_type].var_not_needed = TRUE;} break;
case 61:
#line	390	"web2c.yacc"
{if (last_type >= 0)
				    sym_table[last_type].var_not_needed = TRUE;} break;
case 62:
#line	395	"web2c.yacc"
{if (last_type >= 0) {
	    sym_table[last_type].var_not_needed = sym_table[l_s].var_not_needed;
	    sym_table[last_type].upper = sym_table[l_s].upper;
	    sym_table[last_type].upper_sym = sym_table[l_s].upper_sym;
	    sym_table[last_type].val = sym_table[l_s].val;
	    sym_table[last_type].val_sym = sym_table[l_s].val_sym;
	 }
	 my_output(last_id); my_output("*"); } break;
case 65:
#line	411	"web2c.yacc"
{ compute_array_bounds(); } break;
case 66:
#line	413	"web2c.yacc"
{ lower_bound = sym_table[l_s].val;
				  lower_sym = sym_table[l_s].val_sym;
				  upper_bound = sym_table[l_s].upper;
				  upper_sym = sym_table[l_s].upper_sym;
				  compute_array_bounds();
				} break;
case 68:
#line	426	"web2c.yacc"
{ my_output("struct"); my_output("{"); indent++;} break;
case 69:
#line	428	"web2c.yacc"
{ indent--; my_output("}"); semicolon(); } break;
case 72:
#line	436	"web2c.yacc"
{ field_list[0] = 0; } break;
case 73:
#line	438	"web2c.yacc"
{
				  /*array_bounds[0] = 0;
				  array_offset[0] = 0;*/
				} break;
case 74:
#line	443	"web2c.yacc"
{ int i=0, j; char ltemp[80];
				  while(field_list[i++] == '!') {
					j = 0;
					while (field_list[i])
					    ltemp[j++] = field_list[i++];
					i++;
					if (field_list[i] == '!')
						ltemp[j++] = ',';
					ltemp[j] = 0;
					my_output(ltemp);
				  }
				  semicolon();
				} break;
case 78:
#line	464	"web2c.yacc"
{ int i=0, j=0;
				  while (field_list[i] == '!')
					while(field_list[i++]);
				  ii = add_to_table(last_id);
				  sym_table[ii].typ = field_id_tok;
				  field_list[i++] = '!';
				  while (last_id[j])
					field_list[i++] = last_id[j++];
				  field_list[i++] = 0;
				  field_list[i++] = 0;
				} break;
case 79:
#line	476	"web2c.yacc"
{ int i=0, j=0;
				  while (field_list[i] == '!')
					while(field_list[i++]);
				  field_list[i++] = '!';
				  while (last_id[j])
					field_list[i++] = last_id[j++];
				  field_list[i++] = 0;
				  field_list[i++] = 0;
				} break;
case 80:
#line	488	"web2c.yacc"
{ my_output("file_ptr /* of "); } break;
case 81:
#line	490	"web2c.yacc"
{ my_output("*/"); } break;
case 86:
#line	502	"web2c.yacc"
{ var_list[0] = 0;
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				  var_formals = FALSE;
				  ids_paramed = 0;
				} break;
case 87:
#line	509	"web2c.yacc"
{
				  array_bounds[0] = 0;	
				  array_offset[0] = 0;
				} break;
case 88:
#line	514	"web2c.yacc"
{ fixup_var_list(); } break;
case 91:
#line	522	"web2c.yacc"
{ int i=0, j=0;
				  ii = add_to_table(last_id);
				  sym_table[ii].typ = var_id_tok;
				  sym_table[ii].var_formal = var_formals;
				  param_id_list[ids_paramed++] = ii;
	  			  while (var_list[i] == '!')
					while(var_list[i++]);
				  var_list[i++] = '!';
				  while (last_id[j])
					var_list[i++] = last_id[j++];
	  			  var_list[i++] = 0;
				  var_list[i++] = 0;
				} break;
case 92:
#line	536	"web2c.yacc"
{ int i=0, j=0;
				  ii = add_to_table(last_id);
	  			  sym_table[ii].typ = var_id_tok;
				  sym_table[ii].var_formal = var_formals;
				  param_id_list[ids_paramed++] = ii;
	  			  while (var_list[i] == '!')
					while (var_list[i++]);
	  			  var_list[i++] = '!';
				  while (last_id[j])
					var_list[i++] = last_id[j++];
	  			  var_list[i++] = 0;
				  var_list[i++] = 0;
				} break;
case 93:
#line	550	"web2c.yacc"
{ int i=0, j=0;
				  ii = add_to_table(last_id);
	  			  sym_table[ii].typ = var_id_tok;
				  sym_table[ii].var_formal = var_formals;
				  param_id_list[ids_paramed++] = ii;
	  			  while (var_list[i] == '!')
					while(var_list[i++]);
	  			  var_list[i++] = '!';
				  while (last_id[j])
					var_list[i++] = last_id[j++];
				  var_list[i++] = 0;
				  var_list[i++] = 0;
				} break;
case 95:
#line	567	"web2c.yacc"
{ my_output("void main_body() {");
				  indent++;
			      	  new_line();
                                } break;
case 96:
#line	572	"web2c.yacc"
{ indent--; my_output("}"); new_line(); } break;
case 99:
#line	580	"web2c.yacc"
{ indent_line(); remove_locals(); } break;
case 100:
#line	582	"web2c.yacc"
{ indent_line(); remove_locals(); } break;
case 102:
#line	589	"web2c.yacc"
{ ii = add_to_table(last_id);
				  if (debug)
				  (void) fprintf(stderr, "%3d Procedure %s\n",
					pf_count++, last_id);
				  sym_table[ii].typ = proc_id_tok;
				  (void) strcpy(my_routine, last_id);
				  uses_eqtb = uses_mem = FALSE;
				  my_output("void");
				  orig_std = std;
				  std = 0;
				} break;
case 103:
#line	601	"web2c.yacc"
{(void) strcpy(function_return_type, "void");
				 do_proc_args();
				 gen_function_head();} break;
case 104:
#line	605	"web2c.yacc"
{ ii = l_s; 
				  if (debug)
				  (void) fprintf(stderr, "%3d Procedure %s\n",
					pf_count++, last_id);
				  (void) strcpy(my_routine, last_id);
				  my_output("void");
				} break;
case 105:
#line	613	"web2c.yacc"
{(void) strcpy(function_return_type, "void");
				 do_proc_args();
				 gen_function_head();} break;
case 106:
#line	619	"web2c.yacc"
{ (void) strcpy(z_id, last_id);
				  mark();
				  ids_paramed = 0;
				} break;
case 107:
#line	624	"web2c.yacc"
{ if (ansi) (void) strcpy(z_id, last_id);
				  else (void) sprintf(z_id, "z%s", last_id);
				  ids_paramed = 0;
 				  if (sym_table[ii].typ == proc_id_tok)
				  	sym_table[ii].typ = proc_param_tok;
				  else if (sym_table[ii].typ == fun_id_tok)
					sym_table[ii].typ = fun_param_tok;
				  mark();
				} break;
case 111:
#line	640	"web2c.yacc"
{ ids_typed = ids_paramed;} break;
case 112:
#line	642	"web2c.yacc"
{int i, need_var;
				 i = search_table(last_id);
				 need_var = !sym_table[i].var_not_needed;
				 for (i=ids_typed; i<ids_paramed; i++) {
					(void) strcpy(arg_type[i], last_id);
		if (need_var && sym_table[param_id_list[i]].var_formal)
					(void) strcat(arg_type[i], " *");
		else sym_table[param_id_list[i]].var_formal = FALSE;
				 }
				} break;
case 113:
#line	654	"web2c.yacc"
{var_formals = 0;} break;
case 115:
#line	655	"web2c.yacc"
{var_formals = 1;} break;
case 120:
#line	666	"web2c.yacc"
{ orig_std = std;
				  std = 0;
				  ii = add_to_table(last_id);
				  if (debug)
				  (void) fprintf(stderr, "%3d Function %s\n",
					pf_count++, last_id);
	  			  sym_table[ii].typ = fun_id_tok;
				  (void) strcpy(my_routine, last_id);
				  uses_eqtb = uses_mem = FALSE;
				} break;
case 121:
#line	677	"web2c.yacc"
{ normal();
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 122:
#line	682	"web2c.yacc"
{(void) strcpy(function_return_type, yytext);
			     do_proc_args();
			     gen_function_head();
			    } break;
case 124:
#line	689	"web2c.yacc"
{ orig_std = std;
				  std = 0;
				  ii = l_s;
				  (void) fprintf(stderr, "%3d Function %s\n",
					pf_count++, last_id);
				  (void) strcpy(my_routine, last_id);
				  uses_eqtb = uses_mem = FALSE;
				} break;
case 125:
#line	698	"web2c.yacc"
{ normal();
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 126:
#line	703	"web2c.yacc"
{(void) strcpy(function_return_type, yytext);
			     do_proc_args();
			     gen_function_head();
			    } break;
case 132:
#line	721	"web2c.yacc"
{my_output("{"); indent++; new_line();} break;
case 133:
#line	723	"web2c.yacc"
{ indent--; my_output("}"); new_line(); } break;
case 138:
#line	736	"web2c.yacc"
{if (!doreturn(temp)) {
				    (void) sprintf(safe_string, "lab%s:",
					temp);
				    my_output(safe_string);
				 }
				} break;
case 139:
#line	745	"web2c.yacc"
{ semicolon(); } break;
case 140:
#line	747	"web2c.yacc"
{ semicolon(); } break;
case 145:
#line	755	"web2c.yacc"
{my_output("break");} break;
case 146:
#line	759	"web2c.yacc"
{ my_output("="); } break;
case 148:
#line	762	"web2c.yacc"
{ my_output("Result ="); } break;
case 150:
#line	767	"web2c.yacc"
{ if (strcmp(last_id, "mem") == 0)
					uses_mem = 1;
				  else if (strcmp(last_id, "eqtb") == 0)
					uses_eqtb = 1;
				  if (sym_table[l_s].var_formal)
					(void) putchar('*');
				  my_output(last_id);
				  yyval = ex_32;
				} break;
case 152:
#line	778	"web2c.yacc"
{ if (sym_table[l_s].var_formal)
					(void) putchar('*');
				  my_output(last_id); yyval = ex_32; } break;
case 153:
#line	784	"web2c.yacc"
{ yyval = ex_32; } break;
case 154:
#line	786	"web2c.yacc"
{ yyval = ex_32; } break;
case 157:
#line	794	"web2c.yacc"
{ my_output("["); } break;
case 158:
#line	796	"web2c.yacc"
{ my_output("]"); } break;
case 159:
#line	798	"web2c.yacc"
{if (tex || mf) {
				   if (strcmp(last_id, "int")==0)
					my_output(".cint");
				   else if (strcmp(last_id, "lh")==0)
					my_output(".v.LH");
				   else if (strcmp(last_id, "rh")==0)
					my_output(".v.RH");
				   else {
				     (void)sprintf(safe_string, ".%s", last_id);
				     my_output(safe_string);
				   }
				 }
				 else {
				    (void) sprintf(safe_string, ".%s", last_id);
				    my_output(safe_string);
				 }
				} break;
case 160:
#line	816	"web2c.yacc"
{ my_output(".hh.b0");} break;
case 161:
#line	818	"web2c.yacc"
{ my_output(".hh.b1");} break;
case 163:
#line	823	"web2c.yacc"
{ my_output("][");} break;
case 165:
#line	828	"web2c.yacc"
{ yyval = yypt[-0].yyv; } break;
case 166:
#line	829	"web2c.yacc"
{my_output("+");} break;
case 167:
#line	830	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 168:
#line	831	"web2c.yacc"
{my_output("-");} break;
case 169:
#line	832	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 170:
#line	833	"web2c.yacc"
{my_output("*");} break;
case 171:
#line	834	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 172:
#line	835	"web2c.yacc"
{my_output("/");} break;
case 173:
#line	836	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 174:
#line	837	"web2c.yacc"
{my_output("==");} break;
case 175:
#line	838	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 176:
#line	839	"web2c.yacc"
{my_output("!=");} break;
case 177:
#line	840	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 178:
#line	841	"web2c.yacc"
{my_output("%");} break;
case 179:
#line	842	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 180:
#line	843	"web2c.yacc"
{my_output("<");} break;
case 181:
#line	844	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 182:
#line	845	"web2c.yacc"
{my_output(">");} break;
case 183:
#line	846	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 184:
#line	847	"web2c.yacc"
{my_output("<=");} break;
case 185:
#line	848	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 186:
#line	849	"web2c.yacc"
{my_output(">=");} break;
case 187:
#line	850	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 188:
#line	851	"web2c.yacc"
{my_output("&&");} break;
case 189:
#line	852	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 190:
#line	853	"web2c.yacc"
{my_output("||");} break;
case 191:
#line	854	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 192:
#line	856	"web2c.yacc"
{ my_output("/ ((double)"); } break;
case 193:
#line	858	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv); my_output(")"); } break;
case 194:
#line	860	"web2c.yacc"
{ yyval = yypt[-0].yyv; } break;
case 196:
#line	865	"web2c.yacc"
{ my_output("- (integer)"); } break;
case 197:
#line	867	"web2c.yacc"
{ my_output("!"); } break;
case 198:
#line	871	"web2c.yacc"
{ my_output("("); } break;
case 199:
#line	873	"web2c.yacc"
{ my_output(")"); yyval = yypt[-3].yyv; } break;
case 202:
#line	877	"web2c.yacc"
{ my_output(last_id); my_output("()"); } break;
case 203:
#line	879	"web2c.yacc"
{ my_output(last_id); } break;
case 205:
#line	884	"web2c.yacc"
{ my_output("("); } break;
case 206:
#line	886	"web2c.yacc"
{ my_output(")"); } break;
case 208:
#line	891	"web2c.yacc"
{ my_output(","); } break;
case 213:
#line	904	"web2c.yacc"
{ my_output(last_id); my_output("()"); } break;
case 214:
#line	906	"web2c.yacc"
{ my_output(last_id);
				  ii = add_to_table(last_id);
	  			  sym_table[ii].typ = proc_id_tok;
				  my_output("()");
				} break;
case 215:
#line	912	"web2c.yacc"
{ my_output(last_id); } break;
case 217:
#line	917	"web2c.yacc"
{if (doreturn(temp)) {
				    if (strcmp(function_return_type,"void"))
					my_output("return(Result)");
				    else
					my_output("return");
				 } else {
				     (void) sprintf(safe_string, "goto lab%s",
					temp);
				     my_output(safe_string);
				 }
				} break;
case 226:
#line	947	"web2c.yacc"
{ my_output("if"); my_output("("); } break;
case 227:
#line	949	"web2c.yacc"
{ my_output(")"); new_line();} break;
case 229:
#line	954	"web2c.yacc"
{ my_output("else"); } break;
case 231:
#line	959	"web2c.yacc"
{ my_output("switch"); my_output("("); } break;
case 232:
#line	961	"web2c.yacc"
{ my_output(")"); indent_line();
				  my_output("{"); indent++;
				} break;
case 233:
#line	965	"web2c.yacc"
{ indent--; my_output("}"); new_line(); } break;
case 236:
#line	973	"web2c.yacc"
{ my_output("break"); semicolon(); } break;
case 239:
#line	981	"web2c.yacc"
{ my_output("case"); 
				  my_output(temp);
				  my_output(":"); indent_line();
				} break;
case 240:
#line	986	"web2c.yacc"
{ my_output("default:"); indent_line(); } break;
case 246:
#line	999	"web2c.yacc"
{ my_output("while");
				  my_output("(");
				} break;
case 247:
#line	1003	"web2c.yacc"
{ my_output(")"); } break;
case 249:
#line	1008	"web2c.yacc"
{ my_output("do"); my_output("{"); indent++; } break;
case 250:
#line	1010	"web2c.yacc"
{ indent--; my_output("}"); 
				  my_output("while"); my_output("( ! (");
				} break;
case 251:
#line	1014	"web2c.yacc"
{ my_output(") )"); } break;
case 252:
#line	1018	"web2c.yacc"
{
				  my_output("{");
				  my_output("register");
				  my_output("integer");
				  if (strict_for)
					my_output("for_begin,");
				  my_output("for_end;");
				 } break;
case 253:
#line	1027	"web2c.yacc"
{ if (strict_for)
					my_output("for_begin");
				  else
					my_output(control_var);
				  my_output("="); } break;
case 254:
#line	1033	"web2c.yacc"
{ my_output("; if (");
				  if (strict_for) my_output("for_begin");
				  else my_output(control_var);
				  my_output(relation);
				  my_output("for_end)");
				  if (strict_for) {
					my_output("{");
					my_output(control_var);
					my_output("=");
					my_output("for_begin");
					semicolon();
				  }
				  my_output("do"); 
				  indent++; 
				  new_line();} break;
case 255:
#line	1049	"web2c.yacc"
{
				  char *top = rindex(for_stack, '#');
				  indent--; new_line();
				  my_output("while"); 
				  my_output("("); 
				  my_output(top+1); 
				  my_output(")"); 
				  my_output(";");
				  my_output("}");
				  if (strict_for)
					my_output("}");
				  *top=0;
				  new_line();
				} break;
case 256:
#line	1066	"web2c.yacc"
{(void) strcpy(control_var, last_id); } break;
case 257:
#line	1070	"web2c.yacc"
{ my_output(";"); } break;
case 258:
#line	1072	"web2c.yacc"
{ 
				  (void) strcpy(relation, "<=");
				  my_output("for_end");
				  my_output("="); } break;
case 259:
#line	1077	"web2c.yacc"
{ 
				  (void) sprintf(for_stack + strlen(for_stack),
				    "#%s++ < for_end", control_var);
				} break;
case 260:
#line	1082	"web2c.yacc"
{ my_output(";"); } break;
case 261:
#line	1084	"web2c.yacc"
{
				  (void) strcpy(relation, ">=");
				  my_output("for_end");
				  my_output("="); } break;
case 262:
#line	1089	"web2c.yacc"
{ 
				  (void) sprintf(for_stack + strlen(for_stack),
				    "#%s-- > for_end", control_var);
				} break;
	}
	goto yystack;  /* stack new state and value */
}
