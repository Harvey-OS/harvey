/*
 * The machine used here is the same as the one used in fgrep.
 * Logically, it is a trie of all the strings to be recognized.
 * In this case, the strings are the paths of tree patterns.
 * The nodes of the trie are the states of the machine and leaves
 * are accepting states.  The trie is augmented by failure transitions.
 *
 * Each machine state is a linked list of machine_node's and referred to
 * by pointing to the head of the list.  The machine_node represents a
 * transition.  The nst field points to the next state when the current
 * input is MI_EQUAL to the inp field.  If the input is not MI_EQUAL to any
 * of the inp fields in the list,  the next state is the one that fail
 * points to.  The fail field must be the same for all machine_node's in
 * a state.  Accepting states have inp.value equal to -1, and the match
 * field points to a list of match structures.  The match structures record
 * which trees have been partially matched.
 *
 * The index is used by the machine generator to convert the machine into
 * a list of integers.
 *
 * Further details about the data structure and algorithms can be
 * found in:
 *	Knuth, Morris, Pratt in SIAM J Computing 6:3
 *	Aho, Corasick in Comm ACM 18:6
 *	Hoffman, O'Donnell in JACM 29:1
 */
struct machine_input {
	short value;
	struct symbol_entry *sym;
};
#define MI_EQUAL(x,y)	((x).value==(y).value)

#include "machcomm.h"

int path_getsym(struct machine_input *);

struct machine_node {
	struct machine_input inp;
	struct machine_node *nst;
	struct machine_node *link;
	struct machine_node *fail;
	struct match *match;
	short int index;
};

struct match {
	struct match *next;
	short int depth;
	LabelData *tree;
};

extern struct machine_node *machine;

void	machine_build(void);
void	cgotofn(LabelData *);
void	MachineGen(void);
