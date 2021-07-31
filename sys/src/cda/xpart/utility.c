#include "part.h"
#include <stdio.h>

extern int yylineno;
extern int match;
extern int input_count;
extern int output_count;
extern int cell_count;

int pin_count = 0;

extern struct PIN pin_array[MAXPIN];
extern struct CELL macrocells[MAXCELLS];

struct PIN pin_stack[MAXPIN];

void bug(char *msg);
void yyerror(char *msg, char *arg);
void output(char c);

void
yyerror(char *msg, char *arg)
{
	fprintf(stderr, "xpal: line %d: ", yylineno);
	fprintf(stderr, msg, arg);
	fprintf(stderr, "\n");
}

void
output(char c)
{
	if (c != '\n') fprintf(stderr, "output called with %o\n", c);
}

int
lookup_array(char *ref, struct ARRAY array[], int count)
{
	int i;
	for (i=0; i<count; i++)
		if (strcmp(array[i].name, ref) == 0) return(TRUE);
	return(FALSE);
}

int
lookup_macrocell(char *name)
{
	int i, type;
	for (i=1; i<=cell_count; i++)
		if (strcmp(name, macrocells[i].name) == 0) return(i);
	fprintf(stderr, "macrocell %s not found\n", name);
	return(0);
} /* end lookup_cell_types */

int
setpin(struct PIN pin)
{
	int pinnumber, flags;
	pinnumber = pin.first;
	if (pinnumber > MAXPIN) {
		fprintf(stderr, "pin %d > MAXPIN! Time to recompile!\n", pinnumber);
		BADEND;
	}
	flags = pin_array[pinnumber].flags;
	if (match) return(0); /* only if strcmp != 0... */
	if (pin.flags & INPUT_TYPE) {
		pin_array[pinnumber].input_line = pin.input_line;
		input_count = maximum(input_count, pin.input_line);
		if (pin.flags & DOUBLE_FLAG) input_count++;
	}
	else
	if (pin.flags & OUTPUT_TYPE) {
		pin_array[pinnumber].output_line = pin.output_line;
		pin_array[pinnumber].max_term = pin.max_term;
		output_count = maximum(output_count, pin.output_line + pin.max_term - 1);
		if (pin.flags & DOUBLE_FLAG) output_count++;
	}
	else
	if (pin.flags & FUSE_FLAG) {
		pin_array[pinnumber].output_line = pin.output_line;
		pin_array[pinnumber].max_term = pin.max_term;
	}
	pin_array[pinnumber].flags |= (OCCUPIED_FLAG | pin.flags);
	pin_array[pinnumber].index = pin.index;
	return((pin.flags & INPUT_TYPE) ? ((pin.flags & DOUBLE_FLAG) ? 2 : 1) :
		pin.max_term);
}

void
bug(char *msg)
{
	fprintf(stderr, msg);
	fflush(stderr);
	abort();
}

void
clear_pin_stack(void)
{
	pin_count = 0;
} /* end clear_pin_stack */

void
push_pins(int first, int last)
{
	pin_stack[pin_count].first = first;
	pin_stack[pin_count++].last = last ? last : first;
} /* end push_pins */

void
declare_pins(struct PIN pins, int offset, int properties)
{
	int pin;
	for(pin=pins.first+offset; pin<=pins.last+offset; pin++)
		pin_array[pin].properties |= properties;
} /* end declare_pins */

void
declare_pin_stack(int properties)
{
	int count;
	for (count = pin_count-1; count >= 0; count--) declare_pins(pin_stack[count], 0, properties);
} /* end declare_pin_stack */

void
declare_macropins(int index)
{
	int pin;
	struct PIN pins;
	while (pin_count) {
		pins = pin_stack[--pin_count];
		for (pin = pins.first; pin <= pins.last; pin++)
			pin_array[pin].macrocell = index;
		if (macrocells[index].reset)
			declare_pins(pins, macrocells[index].reset, INTERNAL_TYPE|OUTPUT_TYPE|RESET_TYPE);
		if (macrocells[index].set)
			declare_pins(pins, macrocells[index].set, INTERNAL_TYPE|OUTPUT_TYPE|SET_TYPE);
		if (macrocells[index].clocked)
			declare_pins(pins, macrocells[index].clocked, INTERNAL_TYPE|OUTPUT_TYPE|CLOCKED_TYPE);
		if (macrocells[index].enable)
			declare_pins(pins, macrocells[index].enable, INTERNAL_TYPE|OUTPUT_TYPE|ENABLE_TYPE);
		if (macrocells[index].invert)
			declare_pins(pins, macrocells[index].invert, INTERNAL_TYPE|OUTPUT_TYPE|INVERTED_TYPE);
	} /* end while */
} /* end declare_pins */

void
print_pins(void)
{
	int pin, property, cell;
	for (pin=0; pin<MAXPIN; pin++)
		if (property = pin_array[pin].properties) {
			fprintf(stderr, "%d: ", pin);
			if (property & INTERNAL_TYPE) fprintf(stderr, "INT ");
			if (property & EXTERNAL_TYPE) fprintf(stderr, "EXT ");
			if (property & INPUT_TYPE) fprintf(stderr, "IN ");
			if (property & OUTPUT_TYPE) fprintf(stderr, "OUT ");
			if (property & BURIED_TYPE) fprintf(stderr, "BURIED ");
			if (property & SET_TYPE) fprintf(stderr, "SET ");
			if (property & RESET_TYPE) fprintf(stderr, "RESET ");
			if (property & ENABLE_TYPE) fprintf(stderr, "ENABLE ");
			if (property & ENABLED_TYPE) fprintf(stderr, "ENABLED ");
			if (property & CLOCK_TYPE) fprintf(stderr, "CLOCK ");
			if (property & COMB_TYPE) fprintf(stderr, "COMB ");
			if (property & CLOCKED_TYPE) fprintf(stderr, "CLOCKED ");
			if (property & INVERTED_TYPE) fprintf(stderr, "INVERTED ");
			if (property & NONINVERTED_TYPE) fprintf(stderr, "NONINVERTED ");
			if (property & VCC_TYPE) fprintf(stderr, "VCC ");
			if (property & GND_TYPE) fprintf(stderr, "GND ");
			if (cell = pin_array[pin].macrocell) fprintf(stderr, ": %s", macrocells[cell]);
			fprintf(stderr, "\n");
		} /* end if */
} /* end print_pins */
