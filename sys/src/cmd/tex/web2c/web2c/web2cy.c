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
#define	others_tok	57371
#define	r_num_tok	57372
#define	i_num_tok	57373
#define	string_literal_tok	57374
#define	single_char_tok	57375
#define	assign_tok	57376
#define	two_dots_tok	57377
#define	undef_id_tok	57378
#define	var_id_tok	57379
#define	proc_id_tok	57380
#define	proc_param_tok	57381
#define	fun_id_tok	57382
#define	fun_param_tok	57383
#define	const_id_tok	57384
#define	type_id_tok	57385
#define	hhb0_tok	57386
#define	hhb1_tok	57387
#define	field_id_tok	57388
#define	define_tok	57389
#define	field_tok	57390
#define	break_tok	57391
#define	not_eq_tok	57392
#define	less_eq_tok	57393
#define	great_eq_tok	57394
#define	or_tok	57395
#define	unary_plus_tok	57396
#define	unary_minus_tok	57397
#define	div_tok	57398
#define	mod_tok	57399
#define	and_tok	57400
#define	not_tok	57401

#line	20	"web2c.yacc"
#include "web2c.h"

#define YYDEBUG 1

#define	symbol(x)	sym_table[x].id
#define	MAX_ARGS	50

static char fn_return_type[50], for_stack[300], control_var[50],
            relation[3];
static char arg_type[MAX_ARGS][30];
static int last_type = -1, ids_typed;
char my_routine[100];	/* Name of routine being parsed, if any */
static char array_bounds[80], array_offset[80];
static int uses_mem, uses_eqtb, lower_sym, upper_sym;
static FILE *orig_std;
boolean doing_statements = false;
static boolean var_formals = false;
static int param_id_list[MAX_ARGS], ids_paramed=0;

extern char conditional[], temp[], *std_header;
extern int tex, mf, strict_for;
extern FILE *coerce;
extern char coerce_name[];
extern boolean debug;

static long my_labs();
static void compute_array_bounds(), fixup_var_list();
static void do_proc_args(), gen_function_head();
static boolean doreturn();

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

#line	1106	"web2c.yacc"


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


/* Kludge around negative lower array bounds.  */

static void
fixup_var_list ()
{
  int i, j;
  char output_string[100], real_symbol[100];

  for (i = 0; var_list[i++] == '!'; )
    {
      for (j = 0; real_symbol[j++] = var_list[i++]; )
        ;
      if (*array_offset)
        {
          (void) fprintf (std, "\n#define %s (%s %s)\n  ",
                          real_symbol, next_temp, array_offset);
          (void) strcpy (real_symbol, next_temp);
          /* Add the temp to the symbol table, so that change files can
             use it later on if necessary.  */
          j = add_to_table (next_temp);
          sym_table[j].typ = var_id_tok;
          find_next_temp ();
        }
      (void) sprintf (output_string, "%s%s%c", real_symbol, array_bounds,
                      var_list[i] == '!' ? ',' : ' ');
      my_output (output_string);
  }
  semicolon ();
}


/* If we're not processing TeX, we return false.  Otherwise,
   return true if the label is "10" and we're not in one of four TeX
   routines where the line labeled "10" isn't the end of the routine.
   Otherwise, return 0.  */
   
static boolean
doreturn (label)
    char *label;
{
    return
      tex
      && STREQ (label, "10")
      && !STREQ (my_routine, "macrocall")
      && !STREQ (my_routine, "hpack")
      && !STREQ (my_routine, "vpackage")
      && !STREQ (my_routine, "trybreak");
}


/* Return the absolute value of a long.  */
static long 
my_labs (x)
  long x;
{
    if (x < 0L) return(-x);
    return(x);
}

static void
do_proc_args ()
{
  fprintf (coerce, "%s %s();\n", fn_return_type, z_id);
}

static void
gen_function_head()
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
		fputs(", ", coerce);
	    fprintf(coerce, "(%s) ", arg_type[i]);
	    fprintf(coerce, "%s(%s)",
		    sym_table[param_id_list[i]].var_formal?"&":"",
		    symbol(param_id_list[i]));
	}
	fprintf(coerce, ")\n");
    }
    std = orig_std;
    my_output(z_id);
    my_output("(");
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
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
-1, 38,
	36, 30,
	-2, 27,
-1, 53,
	36, 44,
	-2, 41,
-1, 66,
	36, 86,
	37, 86,
	46, 86,
	-2, 83,
-1, 154,
	72, 151,
	75, 151,
	-2, 153,
-1, 208,
	36, 72,
	46, 72,
	-2, 75,
-1, 299,
	36, 72,
	46, 72,
	-2, 75,
-1, 352,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 176,
-1, 353,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 178,
-1, 355,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 182,
-1, 356,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 184,
-1, 357,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 186,
-1, 358,
	50, 0,
	51, 0,
	52, 0,
	53, 0,
	54, 0,
	55, 0,
	-2, 188,
-1, 372,
	24, 258,
	-2, 261,
};
#define	YYNPROD	264
#define	YYPRIVATE 57344
#define	YYLAST	542
short	yyact[] =
{
 307, 294, 379, 232, 365, 128, 129, 306, 301, 363,
 127, 172, 290, 243,  83, 217, 293,  51, 221,  36,
 164,  24,  84,  15, 252, 233,  95, 248, 269, 270,
 272, 273, 274, 275, 265, 266, 277, 212,  75, 267,
 278, 268, 271, 276, 222, 391,  46, 223, 347, 390,
 205, 346, 269, 270, 272, 273, 274, 275, 265, 266,
 277, 204, 186, 267, 278, 268, 271, 276, 107, 340,
 107, 378, 376, 339, 106, 207, 344, 269, 270, 272,
 273, 274, 275, 265, 266, 277, 381,  45, 267, 278,
 268, 271, 276, 297, 341, 342, 296,  44,  58, 179,
 410,  59, 218, 265, 266, 277,  97, 181, 267, 278,
 268, 271, 276,  91, 165, 109, 180, 289, 408, 288,
 118, 142,  31,  32, 284, 169, 389, 183,  28,  29,
 124, 267, 278, 268, 271, 276, 298,  49, 178, 377,
 163, 166, 167, 168, 269, 270, 272, 273, 274, 275,
 265, 266, 277, 258,  50, 267, 278, 268, 271, 276,
  34,  47, 184, 368, 211, 185, 182, 206, 203, 111,
 110, 114, 115, 201, 185, 126, 154,  33,  94, 234,
 235, 116, 388, 200,  93,  62,  61,  60,  35, 142,
 142, 213, 299, 214,  30, 142, 225, 236, 228, 229,
 240, 142, 219,  27,  17, 230,  49, 231, 237, 185,
 108,  89,  49, 250,  67,  42, 255, 256, 185, 249,
  54,  85,  86,  50, 302,  73, 396, 280, 142,  50,
 264,  87, 210, 279, 303, 262, 263, 261, 239, 259,
 269, 270, 272, 273, 274, 275, 265, 266, 277,  70,
  57, 267, 278, 268, 271, 276, 304,   5, 102, 287,
  39, 308, 104, 105, 295, 269, 270, 272, 273, 274,
 275, 265, 266, 277,  69,  72, 267, 278, 268, 271,
 276,  82, 324,  23,   6, 111, 110, 114, 115,  98,
  22, 100, 101,  21,  20,  19,  18, 116, 325,  56,
   8,  63, 285, 331, 188, 333, 332, 187, 385, 249,
 348, 349, 350, 351, 352, 353, 354, 355, 356, 357,
 358, 359, 360, 361, 338, 337, 190,  43, 370, 372,
 142,  64, 369, 142, 402, 373, 148, 158, 380, 367,
 245, 366,  52,  37, 161, 406, 147, 162, 329, 383,
  81,  92, 367, 160, 366,  80, 397, 336, 209, 159,
  81,  16, 133, 194, 407,  80, 392, 145, 154, 144,
 146, 155, 156, 326,  25, 375, 413, 394, 412, 142,
 138, 393, 405, 371, 398, 328, 395, 400, 238, 198,
 327, 197, 399, 283, 142, 196, 404, 403, 401, 380,
 409, 153, 152, 151, 364, 387, 362, 148, 158, 142,
 323, 411, 195, 414, 415, 161,  11, 147, 162, 224,
 286, 199, 193,  10, 160, 157, 150, 149,  12, 141,
 159, 140, 189, 343,  13, 384,  14, 305, 145, 154,
 144, 146, 155, 156, 257, 282, 281, 227, 322, 321,
 320, 138, 319, 318, 317, 316, 315,   9, 314, 313,
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
-1000,-1000, 237,-1000,-1000, 264, 409, 344, 137, 260,
 259, 258, 257, 254, 247, 367,-1000,-1000, 136,  61,
 127,  55, 110, 121, 318,-1000, 296,-1000,-1000,  28,
-1000,-1000,  18,-1000, 164,-1000, 315,-1000,-1000,-1000,
 214,  31,-1000,-1000, 120, 119, 118, 266, 300,-1000,
-1000,-1000,-1000,-1000,-1000, 213,-1000,-1000,-1000, 296,
-1000,-1000,-1000, 164,-1000, 336,-1000,-1000, 185,-1000,
-1000, 161,-1000,-1000, 346,-1000, 117, 111,-1000,-1000,
 253, 222,-1000,   0,-1000,-1000,-1000,-1000, 160, 255,
-1000,-1000,-1000,-1000,-1000,-1000, 344,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 185,-1000, 108,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 331, 367,  46,
  46,  46,  46,  95,-1000,  95,-1000, 151,-1000,-1000,
 -12,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000, 273, 270,-1000,-1000,-1000, 295,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 353,-1000,-1000,
-1000,-1000,-1000, 318, 106,-1000, 101, -13, -24, 100,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   3,
-1000, 340, 189,  97, -38, 331, 402,-1000,-1000,  34,
-1000, 331, -28,-1000,-1000, 139, 139, 331, 201, 139,
-1000,-1000, 313,-1000,-1000,-1000,-1000, 170,-1000,-1000,
-1000,-1000,-1000,-1000,-1000, 139, 139,-1000,-1000, 142,
 -28,-1000,-1000, 191, 331, 215, 139,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000, 190,  98, 268,-1000,
 190, 315,  50,-1000,-1000,-1000,  95,  95,  23,-1000,
-1000, 125,-1000, 188,  95, 190, 190, 139,-1000,-1000,
 139,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000, 139,  34, 365,-1000,-1000, 325,-1000,-1000, 313,
-1000, 185,-1000,-1000,-1000,-1000, 339, 170,-1000,-1000,
  -1,-1000,-1000,-1000,-1000,  25,-1000,   2, -22, 139,
 139, 139, 139, 139, 139, 139, 139, 139, 139, 139,
 139, 139, 139, 310,  94,-1000, 331, 139, 139, 331,
 370,-1000,  -2,-1000,  72,   4,  95,  13,-1000,-1000,
 188,-1000,-1000,-1000, 277,-1000,-1000,-1000,  70,  70,
-1000,-1000,  47,  47,-1000,  47,  47,  47,  47,-1000,
  70,-1000, 115,-1000, -25,-1000,-1000,-1000,-1000,-1000,
 190, 358, 190,-1000,-1000, 331, 183,-1000,-1000,-1000,
-1000, 338,  95,-1000, 139,-1000, 139,-1000, 323,-1000,
 402, 310,-1000, 321, 355, 107,-1000,  95,-1000,-1000,
  27,-1000,-1000,-1000,-1000, 331,-1000,-1000,-1000,-1000,
-1000,-1000, 139, 139, 190, 190
};
short	yypgo[] =
{
   0, 541, 540, 539, 538,  23,  21,  19,  17, 537,
 536, 535, 534,  11,  26, 533, 532, 531, 530, 529,
 528, 215, 527, 260, 526, 525,  25, 524, 523, 522,
 220, 521, 520, 519,   1, 518, 517, 516, 161, 515,
 514, 513, 512, 511,  27,   2, 510, 509,  24, 508,
 507, 506,   8, 505, 504, 214, 503,  14, 502,  22,
 501,  10,  38, 500, 499, 498, 497,  20, 496, 495,
 494, 493,  13,  12, 492, 491, 490, 489, 488, 487,
  16, 486, 485, 484, 483, 482, 481, 480,   5,   6,
 479, 478, 477, 476, 475, 474, 473,   3, 472,   0,
 471, 470, 469, 468,  18, 467, 466, 465, 464, 463,
 462, 461, 460, 459, 458, 456, 455, 454, 453, 452,
 450, 449, 448, 447, 446, 445,  15, 444, 437,   7,
 435, 433, 432, 431, 429, 427, 426, 425, 422, 421,
 420, 419, 412, 410, 406, 405,   9, 404,   4, 403,
 402, 401, 395, 393, 391, 390, 389, 388, 385, 383,
 382, 381, 378, 377, 376
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
  57,  59,  59,  59,  11,  60,  11,  10,  10,  10,
  62,  62,  63,  66,  65,  69,  65,  67,  70,  67,
  71,  71,  74,  73,  75,  72,  76,  72,  68,  68,
  64,  78,  79,  81,  77,  83,  84,  85,  77,  82,
  82,  80,  18,  87,  86,  61,  61,  88,  88,  90,
  89,  89,  91,  91,  91,  91,  91,  98,  93, 101,
  93, 102,  97,  97, 100, 100, 103, 103, 105, 104,
 104, 104, 104, 106, 107, 106,  99, 109,  99, 110,
  99, 111,  99, 112,  99, 113,  99, 114,  99, 115,
  99, 116,  99, 117,  99, 118,  99, 119,  99, 120,
  99, 121,  99, 122,  99,  99, 108, 108, 108, 124,
 123, 123, 123, 123, 125, 123, 127, 126, 128, 130,
 128, 129, 131, 131,  94,  94, 132,  94,  95,  96,
  92,  92,  92, 133, 133, 135, 135, 139, 140, 137,
 141, 138, 142, 143, 136, 144, 144, 146, 147, 147,
 148, 148, 145, 145, 134, 134, 134, 152, 153, 149,
 154, 155, 150, 156, 158, 160, 151, 157, 161, 162,
 159, 163, 164, 159
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
   3,   1,   1,   1,   0,   0,   5,   0,   1,   2,
   2,   2,   2,   0,   5,   0,   5,   0,   0,   4,
   1,   3,   0,   4,   0,   2,   0,   3,   1,   1,
   2,   0,   0,   0,   9,   0,   0,   0,   9,   1,
   1,   1,   3,   0,   4,   1,   3,   1,   3,   1,
   1,   1,   1,   1,   1,   1,   1,   0,   4,   0,
   4,   0,   3,   1,   1,   1,   1,   2,   0,   4,
   2,   2,   2,   1,   0,   4,   2,   0,   4,   0,
   4,   0,   4,   0,   4,   0,   4,   0,   4,   0,
   4,   0,   4,   0,   4,   0,   4,   0,   4,   0,
   4,   0,   4,   0,   4,   1,   1,   1,   1,   0,
   4,   1,   1,   1,   0,   3,   0,   4,   1,   0,
   4,   2,   2,   0,   1,   1,   0,   3,   2,   0,
   1,   1,   1,   1,   1,   1,   2,   0,   0,   6,
   0,   3,   0,   0,   7,   1,   3,   3,   1,   3,
   1,   1,   1,   2,   1,   1,   1,   0,   0,   6,
   0,   0,   6,   0,   0,   0,   9,   1,   0,   0,
   5,   0,   0,   5
};
short	yychk[] =
{
-1000,  -1,  -2,  -3, -12,  20,  47,  -4,  36,  48,
  14,   7,  19,  25,  27,  -5,  17,  67,  36,  36,
  36,  36,  36,  36,  -6,   7, -19,  67,  67,  68,
  67,  67,  68,  67,  50,  67,  -7,  25, -22, -23,
 -24, -20, -21,  31,  69,  69, -13, -38, -39,  42,
  59,  -8,  27, -29, -30, -31, -23,  36,  67,  70,
  67,  67,  67,  35,  31,  -9, -54, -55, -56, -30,
  36, -25, -21, -38, -10, -62, -63, -64, -65, -77,
  19,  14, -55, -57, -59,  36,  37,  46, -32,  50,
 -11, -62,   5,  67,  67, -14, -15, -14,  36, -68,
  38,  39,  36, -82,  40,  41,  74,  70,  50, -26,
  31,  30, -27, -28,  32,  33,  42, -60,  -5, -66,
 -69, -78, -83, -58, -59, -33,  67, -61, -88, -89,
 -90, -91, -92,  31, -93, -94, -95, -96,  49, -86,
-133,-134, -97,-100,  38,  36,  39,  15,   5,-135,
-136,-149,-150,-151,  37,  40,  41,-137,   6,  28,
  22,  13,  16,  -6, -67,  68, -67, -67, -67, -34,
 -35, -36, -13, -37, -40, -41, -42, -43,  43,   4,
  21,  12,  71, -34,  11,  67,  74,  34,  34,-132,
  31, -87,-102,-138,  10,-142,-152,-154,-156,-139,
  -7,  67, -70,  67,  74,  74,  67,  72, -46,  18,
  43,  67,  75, -88, -89, -98,-101,-126,  68, -61,
-103,-104,  72,  75,-141, -99,-108,-123,  59,  60,
  66,  68, -97, -26,  40,  41, -99, -61,-157,  37,
 -99, -16, -71, -72, -75,  27, -79, -84, -44, -13,
  43, -47, -48, -49, -53, -99, -99,-127,  11,-104,
-105,  46,  44,  45, -88,  56,  57,  61,  63,  50,
  51,  64,  52,  53,  54,  55,  65,  58,  62,  18,
 -99,-124,-125,-153,  26,  34,-140,  -8,  69,  67,
 -73, -74, -76, -80, -34, -80,  73,  70,  11,  67,
 -50, -52,  36,  46, -34,-128,-129, -99, -99,-109,
-110,-111,-112,-113,-114,-115,-116,-117,-118,-119,
-120,-121,-122,-143, -99,-126,   8,-155,-158,  23,
 -17, -72, -57, -73, -81, -85,  18, -44, -48,  74,
  70,  69,  70,-131,  74,-106,  73,  70, -99, -99,
 -99, -99, -99, -99, -99, -99, -99, -99, -99, -99,
 -99, -99,-144,-146,-147,-148,  31,  29,  69, -88,
 -99,-159, -99, -88, -18,   5,  74,  67,  67, -45,
 -34,  73, -51, -52,-130,  31,-107,-145,  67,  11,
  74,  70,   8,-161,-163, -61,  43,  18, -34,-129,
 -99,-146,  11, -89,-148,-160,  24,   9,  11, -45,
  73, -88,-162,-164, -99, -99
};
short	yydef[] =
{
   4,  -2,   0,   1,   5,   0,   0,  20,   0,   0,
   0,   0,   0,   0,   0,  26,  21,  15,   0,   0,
   0,   0,   0,   0,  40,  30,   0,   6,   7,   0,
   8,  10,   0,  12,  53,  14,  82,  44,  -2,  28,
   0,   0,  23,  25,   0,   0,   0,   0,   0,  56,
  54,   2,  86,  -2,  42,   0,  29,  31,  22,   0,
   9,  11,  13,  53,  55,  97,  -2,  84,   0,  43,
  45,   0,  24,  52,  94,  98,   0,   0,  16,  16,
   0,   0,  85,   0,  89,  91,  92,  93,   0,   0,
   3,  99,  95, 100, 101, 102,  20, 120, 103, 105,
 118, 119, 121, 125, 129, 130,  87,   0,  46,   0,
  33,  34,  35,  36,  37,  38,  39, 219,  26, 107,
 107, 107, 107,  53,  90,  53,  32,   0, 135, 137,
   0, 140, 141, 139, 142, 143, 144, 145, 146, 220,
 221, 222,   0,   0, 214, 215, 216,   0, 133, 223,
 224, 244, 245, 246,  -2, 154, 155, 225, 232, 247,
 250, 253, 227,  40,   0, 108,   0,   0,   0,   0,
  48,  49,  50,  51,  58,  59,  60,  61,  57,   0,
  68,   0,   0,   0,   0, 219, 219, 147, 149,   0,
 218, 219,   0, 226, 230,   0,   0, 219,   0,   0,
  17, 104, 114, 106, 122, 126,  88,  53,  -2,  80,
  62,  47,  96, 136, 138,   0,   0, 217, 206,   0,
 152, 156, 158,   0, 219,   0,   0, 195, 196, 197,
 198, 199, 201, 202, 203, 204, 248,   0,   0, 257,
 228,  82,   0, 110, 112, 116,  53,  53,   0,  65,
  66,   0,  70,   0,  53, 148, 150,   0, 134, 157,
   0, 160, 161, 162, 231, 167, 169, 171, 173, 175,
 177, 179, 181, 183, 185, 187, 189, 191, 193, 233,
 166,   0,   0,   0, 251, 254,   0,  18, 109, 114,
 115,   0, 112, 123, 131, 127,   0,  53,  69,  -2,
   0,  76,  78,  79,  81,   0, 208, 213,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0, 205, 219,   0,   0, 219,
   0, 111,   0, 117,   0,   0,  53,   0,  71,  73,
   0, 207, 209, 211,   0, 159, 163, 164, 168, 170,
 172, 174,  -2,  -2, 180,  -2,  -2,  -2,  -2, 190,
 192, 194,   0, 235,   0, 238, 240, 241, 200, 249,
 252,   0,  -2, 229,  19, 219,   0, 124, 128,  63,
  67,   0,  53,  77,   0, 212,   0, 234,   0, 242,
 219,   0, 255,   0,   0,   0, 113,  53,  74, 210,
   0, 236, 243, 237, 239, 219, 259, 262, 132,  64,
 165, 256,   0,   0, 260, 263
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  68,  69,  61,  56,  70,  57,  75,  62,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  74,  67,
  52,  50,  53,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  72,   0,  73,  71
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,
  42,  43,  44,  45,  46,  47,  48,  49,  51,  54,
  55,  58,  59,  60,  63,  64,  65,  66
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
long	yychar;				/* for debugging */

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
	if(yychar <= 0) {
		c = yytok1[0];
		goto out;
	}
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
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
	YYSTYPE save1, save2;
	int save3, save4;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];
	goto yystack;

ret0:
	yyn = 0;
	goto ret;

ret1:
	yyn = 1;
	goto ret;

ret:
	yylval = save1;
	yyval = save2;
	yynerrs = save3;
	yyerrflag = save4;
	return yyn;

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		printf("char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) { 
		yyerror("yacc stack overflow"); 
		goto ret1; 
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
			goto ret0;
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
			goto ret1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= YYEOFCODE)
				printf("error recovery discards %s\n", yytokname(yychar));
			if(yychar == 0)
				goto ret1;
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
#line	58	"web2c.yacc"
{ block_level++;
 	    printf ("#include \"%s\"\n", std_header);
	  } break;
case 2:
#line	63	"web2c.yacc"
{ printf ("\n#include \"%s\"\n", coerce_name); } break;
case 3:
#line	66	"web2c.yacc"
{ YYACCEPT; } break;
case 6:
#line	76	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = field_id_tok;
	    } break;
case 7:
#line	81	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = fun_id_tok;
	    } break;
case 8:
#line	86	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = const_id_tok;
	    } break;
case 9:
#line	91	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = fun_param_tok;
	    } break;
case 10:
#line	96	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = proc_id_tok;
	    } break;
case 11:
#line	101	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = proc_param_tok;
	    } break;
case 12:
#line	106	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = type_id_tok;
	    } break;
case 13:
#line	111	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = type_id_tok;
	      sym_table[ii].val = lower_bound;
	      sym_table[ii].val_sym = lower_sym;
	      sym_table[ii].upper = upper_bound;
	      sym_table[ii].upper_sym = upper_sym;
	    } break;
case 14:
#line	120	"web2c.yacc"
{
	      ii = add_to_table (last_id); 
	      sym_table[ii].typ = var_id_tok;
	    } break;
case 16:
#line	131	"web2c.yacc"
{	if (block_level > 0) my_output("{");
			indent++; block_level++;
		      } break;
case 17:
#line	136	"web2c.yacc"
{if (block_level == 2) {
			    if (strcmp(fn_return_type, "void")) {
			      my_output("register");
			      my_output(fn_return_type);
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
#line	151	"web2c.yacc"
{if (block_level == 1)
				puts("\n#include \"coerce.h\"");
			 doing_statements = true;
			} break;
case 19:
#line	156	"web2c.yacc"
{if (block_level == 2) {
			    if (strcmp(fn_return_type,"void")) {
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
			 doing_statements = false;
			} break;
case 21:
#line	189	"web2c.yacc"
{ my_output("/*"); } break;
case 22:
#line	191	"web2c.yacc"
{ my_output("*/"); } break;
case 25:
#line	199	"web2c.yacc"
{ my_output(temp); } break;
case 27:
#line	204	"web2c.yacc"
{ indent_line(); } break;
case 30:
#line	212	"web2c.yacc"
{ new_line(); my_output("#define"); } break;
case 31:
#line	214	"web2c.yacc"
{ ii=add_to_table(last_id);
				  sym_table[ii].typ = const_id_tok;
				  my_output(last_id);
				} break;
case 32:
#line	219	"web2c.yacc"
{ sym_table[ii].val=last_i_num;
				  new_line(); } break;
case 33:
#line	224	"web2c.yacc"
{
				  (void) sscanf(temp, "%ld", &last_i_num);
				  if (my_labs((long) last_i_num) > 32767)
				      (void) strcat(temp, "L");
				  my_output(temp);
				  yyval = ex_32;
				} break;
case 34:
#line	232	"web2c.yacc"
{ my_output(temp);
				  yyval = ex_real;
				} break;
case 35:
#line	236	"web2c.yacc"
{ yyval = 0; } break;
case 36:
#line	238	"web2c.yacc"
{ yyval = ex_32; } break;
case 37:
#line	242	"web2c.yacc"
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
#line	256	"web2c.yacc"
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
#line	274	"web2c.yacc"
{ my_output(last_id); } break;
case 44:
#line	286	"web2c.yacc"
{ my_output("typedef"); } break;
case 45:
#line	288	"web2c.yacc"
{ ii = add_to_table(last_id);
				  sym_table[ii].typ = type_id_tok;
				  (void) strcpy(safe_string, last_id);
				  last_type = ii;
				} break;
case 46:
#line	294	"web2c.yacc"
{
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 47:
#line	299	"web2c.yacc"
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
#line	315	"web2c.yacc"
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
					my_output(UNSIGNED_SHORT_STRING);
	  			    else my_output("integer");
				  }
				  else my_output("integer");
				} break;
case 55:
#line	353	"web2c.yacc"
{lower_bound = upper_bound;
				 lower_sym = upper_sym;
				 (void) sscanf(temp, "%ld", &upper_bound);
				 upper_sym = -1; /* no sym table entry */
				} break;
case 56:
#line	359	"web2c.yacc"
{ lower_bound = upper_bound;
				  lower_sym = upper_sym;
				  upper_bound = sym_table[l_s].val;
				  upper_sym = l_s;
				} break;
case 57:
#line	367	"web2c.yacc"
{if (last_type >= 0) {
	    sym_table[last_type].var_not_needed = sym_table[l_s].var_not_needed;
	    sym_table[last_type].upper = sym_table[l_s].upper;
	    sym_table[last_type].upper_sym = sym_table[l_s].upper_sym;
	    sym_table[last_type].val = sym_table[l_s].val;
	    sym_table[last_type].val_sym = sym_table[l_s].val_sym;
	 }
	 my_output(last_id); } break;
case 58:
#line	379	"web2c.yacc"
{ if (last_type >= 0)
	        sym_table[last_type].var_not_needed = true;
            } break;
case 60:
#line	384	"web2c.yacc"
{ if (last_type >= 0)
	        sym_table[last_type].var_not_needed = true;
            } break;
case 61:
#line	388	"web2c.yacc"
{ if (last_type >= 0)
	        sym_table[last_type].var_not_needed = true;
            } break;
case 62:
#line	394	"web2c.yacc"
{if (last_type >= 0) {
	    sym_table[last_type].var_not_needed = sym_table[l_s].var_not_needed;
	    sym_table[last_type].upper = sym_table[l_s].upper;
	    sym_table[last_type].upper_sym = sym_table[l_s].upper_sym;
	    sym_table[last_type].val = sym_table[l_s].val;
	    sym_table[last_type].val_sym = sym_table[l_s].val_sym;
	 }
	 my_output(last_id); my_output("*"); } break;
case 65:
#line	410	"web2c.yacc"
{ compute_array_bounds(); } break;
case 66:
#line	412	"web2c.yacc"
{ lower_bound = sym_table[l_s].val;
				  lower_sym = sym_table[l_s].val_sym;
				  upper_bound = sym_table[l_s].upper;
				  upper_sym = sym_table[l_s].upper_sym;
				  compute_array_bounds();
				} break;
case 68:
#line	425	"web2c.yacc"
{ my_output("struct"); my_output("{"); indent++;} break;
case 69:
#line	427	"web2c.yacc"
{ indent--; my_output("}"); semicolon(); } break;
case 72:
#line	435	"web2c.yacc"
{ field_list[0] = 0; } break;
case 73:
#line	437	"web2c.yacc"
{
				  /*array_bounds[0] = 0;
				  array_offset[0] = 0;*/
				} break;
case 74:
#line	442	"web2c.yacc"
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
#line	463	"web2c.yacc"
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
#line	475	"web2c.yacc"
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
#line	487	"web2c.yacc"
{ my_output("file_ptr /* of "); } break;
case 81:
#line	489	"web2c.yacc"
{ my_output("*/"); } break;
case 86:
#line	501	"web2c.yacc"
{ var_list[0] = 0;
	      array_bounds[0] = 0;
	      array_offset[0] = 0;
	      var_formals = false;
	      ids_paramed = 0;
	    } break;
case 87:
#line	508	"web2c.yacc"
{
	      array_bounds[0] = 0;	
	      array_offset[0] = 0;
	    } break;
case 88:
#line	513	"web2c.yacc"
{ fixup_var_list(); } break;
case 91:
#line	521	"web2c.yacc"
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
#line	535	"web2c.yacc"
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
#line	549	"web2c.yacc"
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
{ my_output ("void main_body() {");
		  indent++;
		  new_line ();
		} break;
case 96:
#line	572	"web2c.yacc"
{ indent--;
                  my_output ("}");
                  new_line ();
                } break;
case 100:
#line	584	"web2c.yacc"
{ indent_line(); remove_locals(); } break;
case 101:
#line	586	"web2c.yacc"
{ indent_line(); remove_locals(); } break;
case 103:
#line	594	"web2c.yacc"
{ ii = add_to_table(last_id);
	      if (debug)
	        (void) fprintf(stderr, "%3d Procedure %s\n",
                  pf_count++, last_id);
	      sym_table[ii].typ = proc_id_tok;
	      (void) strcpy(my_routine, last_id);
	      uses_eqtb = uses_mem = false;
	      my_output("void");
	      orig_std = std;
	      std = 0;
	    } break;
case 104:
#line	606	"web2c.yacc"
{ (void) strcpy(fn_return_type, "void");
	      do_proc_args();
	      gen_function_head();} break;
case 105:
#line	610	"web2c.yacc"
{ ii = l_s; 
	      if (debug)
	        (void) fprintf(stderr, "%3d Procedure %s\n",
                               pf_count++, last_id);
	      (void) strcpy(my_routine, last_id);
	      my_output("void");
	    } break;
case 106:
#line	618	"web2c.yacc"
{ (void) strcpy(fn_return_type, "void");
	      do_proc_args();
	      gen_function_head();
            } break;
case 107:
#line	626	"web2c.yacc"
{
              (void) strcpy (z_id, last_id);
	      mark ();
	      ids_paramed = 0;
	    } break;
case 108:
#line	632	"web2c.yacc"
{ sprintf (z_id, "z%s", last_id);
	      ids_paramed = 0;
	      if (sym_table[ii].typ == proc_id_tok)
	        sym_table[ii].typ = proc_param_tok;
	      else if (sym_table[ii].typ == fun_id_tok)
	        sym_table[ii].typ = fun_param_tok;
	      mark();
	    } break;
case 112:
#line	648	"web2c.yacc"
{ ids_typed = ids_paramed;} break;
case 113:
#line	650	"web2c.yacc"
{ int i, need_var;
	      i = search_table(last_id);
	      need_var = !sym_table[i].var_not_needed;
	      for (i=ids_typed; i<ids_paramed; i++)
                {
	          (void) strcpy(arg_type[i], last_id);
		  if (need_var && sym_table[param_id_list[i]].var_formal)
	            (void) strcat(arg_type[i], " *");
		  else
                    sym_table[param_id_list[i]].var_formal = false;
	        }
	    } break;
case 114:
#line	664	"web2c.yacc"
{var_formals = 0;} break;
case 116:
#line	665	"web2c.yacc"
{var_formals = 1;} break;
case 121:
#line	676	"web2c.yacc"
{ orig_std = std;
				  std = 0;
				  ii = add_to_table(last_id);
				  if (debug)
				  (void) fprintf(stderr, "%3d Function %s\n",
					pf_count++, last_id);
	  			  sym_table[ii].typ = fun_id_tok;
				  (void) strcpy(my_routine, last_id);
				  uses_eqtb = uses_mem = false;
				} break;
case 122:
#line	687	"web2c.yacc"
{ normal();
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 123:
#line	692	"web2c.yacc"
{(void) strcpy(fn_return_type, yytext);
			     do_proc_args();
			     gen_function_head();
			    } break;
case 125:
#line	699	"web2c.yacc"
{ orig_std = std;
				  std = 0;
				  ii = l_s;
				  if (debug)
				  (void) fprintf(stderr, "%3d Function %s\n",
					pf_count++, last_id);
				  (void) strcpy(my_routine, last_id);
				  uses_eqtb = uses_mem = false;
				} break;
case 126:
#line	709	"web2c.yacc"
{ normal();
				  array_bounds[0] = 0;
				  array_offset[0] = 0;
				} break;
case 127:
#line	714	"web2c.yacc"
{(void) strcpy(fn_return_type, yytext);
			     do_proc_args();
			     gen_function_head();
			    } break;
case 133:
#line	732	"web2c.yacc"
{my_output("{"); indent++; new_line();} break;
case 134:
#line	734	"web2c.yacc"
{ indent--; my_output("}"); new_line(); } break;
case 139:
#line	747	"web2c.yacc"
{if (!doreturn(temp)) {
				    (void) sprintf(safe_string, "lab%s:",
					temp);
				    my_output(safe_string);
				 }
				} break;
case 140:
#line	756	"web2c.yacc"
{ semicolon(); } break;
case 141:
#line	758	"web2c.yacc"
{ semicolon(); } break;
case 146:
#line	766	"web2c.yacc"
{my_output("break");} break;
case 147:
#line	770	"web2c.yacc"
{ my_output("="); } break;
case 149:
#line	773	"web2c.yacc"
{ my_output("Result ="); } break;
case 151:
#line	778	"web2c.yacc"
{ if (strcmp(last_id, "mem") == 0)
					uses_mem = 1;
				  else if (strcmp(last_id, "eqtb") == 0)
					uses_eqtb = 1;
				  if (sym_table[l_s].var_formal)
					(void) putchar('*');
				  my_output(last_id);
				  yyval = ex_32;
				} break;
case 153:
#line	789	"web2c.yacc"
{ if (sym_table[l_s].var_formal)
					(void) putchar('*');
				  my_output(last_id); yyval = ex_32; } break;
case 154:
#line	795	"web2c.yacc"
{ yyval = ex_32; } break;
case 155:
#line	797	"web2c.yacc"
{ yyval = ex_32; } break;
case 158:
#line	805	"web2c.yacc"
{ my_output("["); } break;
case 159:
#line	807	"web2c.yacc"
{ my_output("]"); } break;
case 160:
#line	809	"web2c.yacc"
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
case 161:
#line	827	"web2c.yacc"
{ my_output(".hh.b0");} break;
case 162:
#line	829	"web2c.yacc"
{ my_output(".hh.b1");} break;
case 164:
#line	834	"web2c.yacc"
{ my_output("][");} break;
case 166:
#line	839	"web2c.yacc"
{ yyval = yypt[-0].yyv; } break;
case 167:
#line	840	"web2c.yacc"
{my_output("+");} break;
case 168:
#line	841	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 169:
#line	842	"web2c.yacc"
{my_output("-");} break;
case 170:
#line	843	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 171:
#line	844	"web2c.yacc"
{my_output("*");} break;
case 172:
#line	845	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 173:
#line	846	"web2c.yacc"
{my_output("/");} break;
case 174:
#line	847	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 175:
#line	848	"web2c.yacc"
{my_output("==");} break;
case 176:
#line	849	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 177:
#line	850	"web2c.yacc"
{my_output("!=");} break;
case 178:
#line	851	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 179:
#line	852	"web2c.yacc"
{my_output("%");} break;
case 180:
#line	853	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 181:
#line	854	"web2c.yacc"
{my_output("<");} break;
case 182:
#line	855	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 183:
#line	856	"web2c.yacc"
{my_output(">");} break;
case 184:
#line	857	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 185:
#line	858	"web2c.yacc"
{my_output("<=");} break;
case 186:
#line	859	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 187:
#line	860	"web2c.yacc"
{my_output(">=");} break;
case 188:
#line	861	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 189:
#line	862	"web2c.yacc"
{my_output("&&");} break;
case 190:
#line	863	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 191:
#line	864	"web2c.yacc"
{my_output("||");} break;
case 192:
#line	865	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv);} break;
case 193:
#line	867	"web2c.yacc"
{ my_output("/ ((double)"); } break;
case 194:
#line	869	"web2c.yacc"
{yyval = max(yypt[-3].yyv, yypt[-0].yyv); my_output(")"); } break;
case 195:
#line	871	"web2c.yacc"
{ yyval = yypt[-0].yyv; } break;
case 197:
#line	876	"web2c.yacc"
{ my_output("- (integer)"); } break;
case 198:
#line	878	"web2c.yacc"
{ my_output("!"); } break;
case 199:
#line	882	"web2c.yacc"
{ my_output("("); } break;
case 200:
#line	884	"web2c.yacc"
{ my_output(")"); yyval = yypt[-3].yyv; } break;
case 203:
#line	888	"web2c.yacc"
{ my_output(last_id); my_output("()"); } break;
case 204:
#line	890	"web2c.yacc"
{ my_output(last_id); } break;
case 206:
#line	895	"web2c.yacc"
{ my_output("("); } break;
case 207:
#line	897	"web2c.yacc"
{ my_output(")"); } break;
case 209:
#line	902	"web2c.yacc"
{ my_output(","); } break;
case 214:
#line	915	"web2c.yacc"
{ my_output(last_id); my_output("()"); } break;
case 215:
#line	917	"web2c.yacc"
{ my_output(last_id);
				  ii = add_to_table(last_id);
	  			  sym_table[ii].typ = proc_id_tok;
				  my_output("()");
				} break;
case 216:
#line	923	"web2c.yacc"
{ my_output(last_id); } break;
case 218:
#line	928	"web2c.yacc"
{if (doreturn(temp)) {
				    if (strcmp(fn_return_type,"void"))
					my_output("return(Result)");
				    else
					my_output("return");
				 } else {
				     (void) sprintf(safe_string, "goto lab%s",
					temp);
				     my_output(safe_string);
				 }
				} break;
case 227:
#line	958	"web2c.yacc"
{ my_output("if"); my_output("("); } break;
case 228:
#line	960	"web2c.yacc"
{ my_output(")"); new_line();} break;
case 230:
#line	965	"web2c.yacc"
{ my_output("else"); } break;
case 232:
#line	970	"web2c.yacc"
{ my_output("switch"); my_output("("); } break;
case 233:
#line	972	"web2c.yacc"
{ my_output(")"); indent_line();
				  my_output("{"); indent++;
				} break;
case 234:
#line	976	"web2c.yacc"
{ indent--; my_output("}"); new_line(); } break;
case 237:
#line	984	"web2c.yacc"
{ my_output("break"); semicolon(); } break;
case 240:
#line	992	"web2c.yacc"
{ my_output("case"); 
				  my_output(temp);
				  my_output(":"); indent_line();
				} break;
case 241:
#line	997	"web2c.yacc"
{ my_output("default:"); indent_line(); } break;
case 247:
#line	1010	"web2c.yacc"
{ my_output("while");
				  my_output("(");
				} break;
case 248:
#line	1014	"web2c.yacc"
{ my_output(")"); } break;
case 250:
#line	1019	"web2c.yacc"
{ my_output("do"); my_output("{"); indent++; } break;
case 251:
#line	1021	"web2c.yacc"
{ indent--; my_output("}"); 
				  my_output("while"); my_output("( ! (");
				} break;
case 252:
#line	1025	"web2c.yacc"
{ my_output(") )"); } break;
case 253:
#line	1029	"web2c.yacc"
{
				  my_output("{");
				  my_output("register");
				  my_output("integer");
				  if (strict_for)
					my_output("for_begin,");
				  my_output("for_end;");
				 } break;
case 254:
#line	1038	"web2c.yacc"
{ if (strict_for)
					my_output("for_begin");
				  else
					my_output(control_var);
				  my_output("="); } break;
case 255:
#line	1044	"web2c.yacc"
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
case 256:
#line	1060	"web2c.yacc"
{
				  char *top = strrchr (for_stack, '#');
				  indent--;
                                  new_line();
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
case 257:
#line	1078	"web2c.yacc"
{(void) strcpy(control_var, last_id); } break;
case 258:
#line	1082	"web2c.yacc"
{ my_output(";"); } break;
case 259:
#line	1084	"web2c.yacc"
{ 
				  (void) strcpy(relation, "<=");
				  my_output("for_end");
				  my_output("="); } break;
case 260:
#line	1089	"web2c.yacc"
{ 
				  (void) sprintf(for_stack + strlen(for_stack),
				    "#%s++ < for_end", control_var);
				} break;
case 261:
#line	1094	"web2c.yacc"
{ my_output(";"); } break;
case 262:
#line	1096	"web2c.yacc"
{
				  (void) strcpy(relation, ">=");
				  my_output("for_end");
				  my_output("="); } break;
case 263:
#line	1101	"web2c.yacc"
{ 
				  (void) sprintf(for_stack + strlen(for_stack),
				    "#%s-- > for_end", control_var);
				} break;
	}
	goto yystack;  /* stack new state and value */
}
