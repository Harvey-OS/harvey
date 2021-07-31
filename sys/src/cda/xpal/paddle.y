%term _ID _STRING _PART _HEX _OCTAL _DECIMAL _BINARY
%term _ARRAY _INPUTS _OUTPUTS _INTERNAL _EXTERNAL _DECLARE _PACKAGE
%term _COMPLEMENTPLUS _COMPLEMENTMINUS _PINS _FUSES _OFFSET _NOT
%term _SET _RESET _CLOCK _MACROCELL _ENABLED _ENABLES _CLOCKED _INVERTED _ENABLE _INVERT
%term _INPUT _OUTPUT _VCC _GND

%{

#include <stdio.h>
#include "xpal.h"

#define YYMAXDEPTH 5000

#define YYSTYPE yystype
typedef union yystype {
	int integer;
	float real;
	char *string;
	struct PIN pin;
} YYSTYPE;

int array_count = 0;
int input_count = 0;
int output_count = 0;
int cell_count = 0;
int max_pin;
int last_pin;
int match = -1;
int pintypes;
int pinflags;
int side = 0;
int line;
int index;
int macroindex = 0;

struct CELL macrocells[MAXCELLS];
struct ARRAY arrays[MAXARRAY];

extern char yytext[];
extern char *extern_id;
extern char *package;
extern char cbuf[];
extern struct PIN pin_array[MAXPIN];
extern int max_fuse;
extern int found;
extern int missingtype;

int lookup_array(char *ref, struct ARRAY array[], int count);
int lookup_cell(char *ref, struct CELL array[], int count);
int setpin(struct PIN pin);

extern char *strdup(char *);
extern int strcmp(char *, char *);

%}

%left '+' '-'
%left '|' '&'
%left '^'
%left UMINUS

%%
start	:	device start
	|	empty
	;
device	:	{ match = -1; array_count = 0; cell_count = 0; } name { max_pin = MAXPIN; } body
	;
name	:	part nametail
			{ match = $1.integer && $2.integer; }
	;
nametail :	'=' part nametail
			{ $$.integer = $2.integer && $3.integer; }
	|	empty
			{ $$.integer = TRUE; }
	;
part	:	_PART
			{ $$.integer = strcmp(yytext, extern_id); }
	|	_ID
			{ $$.integer = strcmp(yytext, extern_id); }
	;
body	:	'{' defs '}'
	;
defs	:	def defs
	|	empty
	;
def	:	declare
	|	array
	|	fuse
	|	pins
	|	macrocell
	|	package
	;
declare	:	_DECLARE '{' declarations '}'
	;
declarations :	declaration declarations
	|	declaration
	;
declaration :	side '{' decl_list '}'
	;
decl_list :	property_decl decl_list
	|	property_decl
	;
property_decl :	property_list pinsequence
			{ if (!match) {
				declare_pin_stack($1.integer | side);
				if (macroindex > 0) declare_macropins(macroindex);
				clear_pin_stack();
			  }
			  macroindex = 0;
			}
	;
property_list :	property property_list
			{ $$.integer = $1.integer | $2.integer; }
	|	property
			{ $$.integer = $1.integer; }
	;
property :	_SET
			{ $$.integer = SET_TYPE; }
	|	_RESET
			{ $$.integer = OUTPUT_TYPE | RESET_TYPE; }
	|	_CLOCK
			{ $$.integer = OUTPUT_TYPE | CLOCK_TYPE; }
	|	_MACROCELL _ID
			{ if (macroindex > 0)
				fprintf(stderr, "macrocell %s already selected\n", macrocells[macroindex].name);
			  else macroindex = lookup_macrocell(yytext);
			  $$.integer = (macroindex > 0) ? (macrocells[macroindex].type & ~(RESET_TYPE|SET_TYPE)) : 0; }
	|	_ENABLED
			{ $$.integer = ENABLED_TYPE; }
	|	_ENABLES
			{ $$.integer = ENABLE_TYPE; }
	|	_CLOCKED
			{ $$.integer = CLOCKED_TYPE; }
	|	_INVERTED
			{ $$.integer = INVERTED_TYPE; }
	|	_INPUT
			{ $$.integer = INPUT_TYPE | COMB_TYPE; }
	|	_OUTPUT
			{ $$.integer = OUTPUT_TYPE | NONINVERTED_TYPE | INVERTED_TYPE; }
	|	_INPUTS
			{ $$.integer = INPUT_TYPE | COMB_TYPE; }
	|	_OUTPUTS
			{ $$.integer = OUTPUT_TYPE | NONINVERTED_TYPE; }
	|	_VCC
			{ $$.integer = VCC_TYPE; }
	|	_GND
			{ $$.integer = GND_TYPE; }
	;
side	:	_INTERNAL
			{ side = INTERNAL_TYPE; }
	|	_EXTERNAL
			{ side = EXTERNAL_TYPE; }
	;
offset	:	_OFFSET number
			{ $$.integer = $2.integer; }
array	:	_ARRAY arrayname '{' elements '}'
			{ if (!match) {
			  last_pin = input_count*output_count+arrays[array_count].offset;
			  array_count++; found++;
			  }
			}
	;
arrayname :	_ID
	   {  if (!match) {
			if (lookup_array(yytext, arrays, array_count))
				yyerror("array name %s already declared\n", yytext);
			arrays[array_count].offset = 0;
			if (array_count < MAXARRAY) {
				arrays[array_count].name = strdup(yytext);
			        input_count = output_count = 0;
			}
			else
				fprintf(stderr, "too many arrays (MAXARRAY)\n");
		}
	    }
	;
elements :	{ pintypes = 0; pinflags = 0; } element elements
	|	empty
	;
element	:	input buffer linesequence
			{ if (!match) arrays[array_count].max_inputs = input_count;
			  $$.integer = $2.integer;
			}
	|	invert output linesequence
			{ if (!match) arrays[array_count].max_outputs = output_count;
			  $$.integer = $2.integer;
			}
	|	offset
			{ if (!match) {arrays[array_count].offset = $1.integer;  }
			}
	;
input	:	_INPUTS
			{ pintypes |= INPUT_TYPE; line = 0; }
	;
output	:	_OUTPUTS
			{ pintypes |= OUTPUT_TYPE; line = 0; }
	;
invert	:	_INVERTED
			{ pintypes = INVERTED_TYPE; }
	|	_MACROCELL
			{ pintypes = INVERTED_TYPE | NONINVERTED_TYPE; }
	|	empty
			{ pintypes = NONINVERTED_TYPE; }
	;
buffer	:	complemented
			{ pinflags |= DOUBLE_FLAG; }
	|	empty
			{ pinflags &= ~DOUBLE_FLAG; }
complemented :	_COMPLEMENTPLUS
	|	_COMPLEMENTMINUS
			{ pinflags |= COMPLEMENT_FIRST; }
	;
fuse	:	_FUSES { line = 0; } fusesequence
	;
pins	:	_PINS number
			{ max_pin = $2.integer; }
	;
pinsequence :	'{' pinlist '}'
			{ $$.integer = match ? 0 : $2.integer; }
	;
pinlist	:	pin ',' pinlist
	|	pin
	|	empty
	;
linesequence :	'{' linelist '}'
			{ $$.integer = match ? 0 : $2.integer; if (missingtype) exit(1); }
	;
linelist :	line ',' linelist
	|	line
	|	empty
	;
line	:	node
			{ line += setpin($1.pin); }
	|	'[' eqnodes ']'
			{ line++; }
	;
eqnodes	:	node ',' eqnodes
			{ (void) setpin($1.pin); }
	|	node
			{ (void) setpin($1.pin); }
	|	empty
	;
node	:	pin terms
			{ 
			  if (pintypes & INPUT_TYPE)
			  	yyval.pin.input_line = line;
			  else
			  if (pintypes & OUTPUT_TYPE) {
			  	yyval.pin.output_line = line;
			  	yyval.pin.max_term = $2.integer;
			  }
			  yyval.pin.index = array_count;
			  yyval.pin.first = $1.integer;
			  yyval.pin.properties = pintypes;
			  yyval.pin.flags = pinflags;
			}
	|	pin '=' number terms
			{
			  line = $3.integer;
			  if (pintypes & INPUT_TYPE)
			  	yyval.pin.input_line = line;
			  else
			  if (pintypes & OUTPUT_TYPE)
			  	yyval.pin.output_line = line;
			  yyval.pin.index = array_count;
			  yyval.pin.first = $1.integer;
			  yyval.pin.max_term = $4.integer;
			  yyval.pin.properties = pintypes;
			  yyval.pin.flags = pinflags;
			}
	;
terms	:	':' number
			{ $$.integer = $2.integer; }
	|	empty
			{ $$.integer = 1; }
	;
pin	:	expression
			{ if ($1.integer > MAXPIN) yyerror("pin out of range");
			  if (!match) push_pins($1.integer, 0);
			}
	|	expression '.' '.' expression
			{ if ($1.integer > MAXPIN) yyerror("first pin out of range");
			  else
			  if ($4.integer > MAXPIN) yyerror("second pin out of range");
			  else
			  if (!match) push_pins($1.integer, $4.integer);
			}
	;
expression :	expression '+' expression
			{ $$.integer = $1.integer + $3.integer; }
	|	expression '-' expression
			{ $$.integer = $1.integer - $3.integer; }
	|	expression '|' expression
			{ $$.integer = $1.integer | $3.integer; }
	|	expression '&' expression
			{ $$.integer = $1.integer & $3.integer; }
	|	expression '^' expression
			{ $$.integer = $1.integer ^ $3.integer; }
	|	'-' expression %prec UMINUS
			{ $$.integer = $2.integer; }
	|	'(' expression ')'
			{ $$.integer = $2.integer; }
	|	number
			{ $$.integer = $1.integer; }
	|	'.'
			{ $$.integer = last_pin; }
	;
fusesequence :	'{' fuses '}'
	;
fuses	:	fuse ',' fuses
			{ (void) setpin($1.pin); }
	|	fuse
			{ (void) setpin($1.pin); }
	;
fuse	:	expression '=' number
			{ if ($1.integer > MAXPIN) yyerror("fuse pin out of range");
			  yyval.pin.index = -1;
			  yyval.pin.first = $1.integer;
			  yyval.pin.max_term = 1;
			  yyval.pin.flags = FUSE_FLAG;
			  yyval.pin.properties = OUTPUT_TYPE;
			  line = $3.integer;
			  yyval.pin.output_line = line;
			  if (array_count && (line > max_fuse)) max_fuse = line;
			  line++;
			}
	|	expression
			{
			  yyval.pin.index = -1;
			  yyval.pin.first = $1.integer;
			  yyval.pin.max_term = 1;
			  yyval.pin.flags = FUSE_FLAG;
			  yyval.pin.properties = OUTPUT_TYPE;
			  yyval.pin.output_line = line;
			  if (array_count && (line > max_fuse)) max_fuse = line;
			  line++;
			}
	;
macrocell :	macrotype _MACROCELL macro_id '{' defines '}'
			{ 
				macrocells[cell_count].name = $3.string;
			  if (!match) {
				macrocells[cell_count].type |= INPUT_TYPE|OUTPUT_TYPE|COMB_TYPE|NONINVERTED_TYPE | $1.integer;
				macrocells[cell_count].mask = $5.integer;
				macrocells[cell_count].mask |= ($5.integer & INVERTED_TYPE) ? 0 : NONINVERTED_TYPE;
				macrocells[cell_count].mask |= ($5.integer & CLOCKED_TYPE) ? 0 : COMB_TYPE;
			  }
			}
	;
macrotype :	_CLOCKED
			{ $$.integer = CLOCKED_TYPE; }
	|	empty
			{ $$.integer = 0; }
defines	:	define ',' defines
			{ $$.integer = $1.integer | $3.integer; }
	|	define
			{ $$.integer = $1.integer; }
	|	empty
			{ $$.integer = 0; }
	;
define :	_NOT subfield
			{ $$.integer = 0; }
	|	subfield
			{ $$.integer = $1.integer; }
	;
subfield :	_ENABLE ':' expression
			{ if (!match) {
				macrocells[cell_count].enable = $3.integer;
				macrocells[cell_count].type |= ENABLED_TYPE;
				$$.integer = ENABLED_TYPE;
			  }
			}
	|	_CLOCK ':' expression
			{ if (!match) {
				macrocells[cell_count].clock = $3.integer;
				macrocells[cell_count].type |= CLOCK_TYPE;
				$$.integer = CLOCK_TYPE;
			  }
			}
	|	_CLOCKED ':' expression
			{ if (!match) {
				macrocells[cell_count].clocked = $3.integer;
				macrocells[cell_count].type |= CLOCKED_TYPE;
				$$.integer = CLOCKED_TYPE;
			  }
			}
	|	_RESET ':' expression
			{ if (!match) {
				macrocells[cell_count].reset = $3.integer;
				macrocells[cell_count].type |= RESET_TYPE;
				$$.integer = RESET_TYPE;
			  }
			}
	|	_SET ':' expression
			{ if (!match) {
				macrocells[cell_count].set = $3.integer;
				macrocells[cell_count].type |= SET_TYPE;
				$$.integer = SET_TYPE;
			  }
			}
	|	_INVERT ':' expression
			{ if (!match) {
				macrocells[cell_count].invert = $3.integer;
				macrocells[cell_count].type |= INVERTED_TYPE;
				$$.integer = INVERTED_TYPE;
			  }
			}
	;
macro_id :	_ID
			{ $$.string = strdup(yytext); cell_count++; }
package	:	_PACKAGE _STRING
			{ if (!match) package = strdup(cbuf); }
	;
number	:	_DECIMAL
			{ sscanf(yytext, "%d", &$$.integer); }
	|	_HEX
			{ sscanf(yytext, "%x", &$$.integer); }
	|	_OCTAL
			{ sscanf(yytext, "%o", &$$.integer); }
	;
empty	:
	;
