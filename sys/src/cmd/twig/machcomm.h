/*
 * The machine is laid out as a sequence of integers in the walker.
 * The form described above is only used inside the preprocessor.
 * Each machine state is one of the following sequence:
 *
 * TABLE <value_1><index_1>...<value_n><index_n> -1 [FAIL <fail_index>] -1
 * or
 * TABLE2 <value_1><index_1>...<value_n><index_n> -1 [FAIL <fail_index>] -1
 * or
 * ACCEPT <accept table index> -1
 *
 * The accept table is in the form:
 *
 * <tree index_1> ...<tree_index_m> -1
 *
 */

enum{
	FASTLIM,
	TABLE,
	FAIL,
	ACCEPT,
	TABLE2,
	EOT	= -1,

	/* special machine state */
	HANG	= -1,
};

/*
 * In order for the walker to access the labelled leaves of a pattern,
 * a table (mistakenly) called the path table is generated (see sym.c).
 * This table contains the following possible values:
 *
 * ePush	follow the children link in the walker skeleton.
 * ePop		Pop up to parent.
 * eNext	follow  the siblings link.
 * eEval <arg>	The current node is a labelled leaf whose label id
 *		is the integer <arg>.
 * eStop	All done. stop!
 *
 * The table is interpreted by the _getleaves routine in the walker.
 */
enum{
	eSTOP		= 0,
	ePOP		= -1,
	eEVAL		= -2,
	eNEXT		= -3,
	ePUSH		= -4,

	/*
	 * Tags that indicate the type of a value
	 * max values for shorts
	 * CHANGE CAREFULLY!!!
	 */
	M_BRANCH	= 0x4000,
	M_NODE		= 0,
	M_LABEL		= 0x2000,
	MAX_NODE_VALUE	= 0x1fff,
	MTAG_SHIFT	= 9,
};

#define M_DETAG(x)	((x)&~(M_BRANCH|M_LABEL|M_NODE))

/* predicates to tell whether a value x is of type NODE, BRANCH, or LABEL */
#define MI_NODE(x)	(((x)&(M_BRANCH|M_LABEL))==0)
#define MI_DEFINED(x)	((x)!=-1)
#define MI_LABEL(x)	(((x)&M_LABEL)!=0)
#define MI_BRANCH(x)	(((x)&M_BRANCH)!=0)

/* build a tagged value */
#define MV_NODE(x)	(x)
#define MV_BRANCH(x)	((x)|M_BRANCH)
#define MV_LABEL(x)	((x)|M_LABEL)

