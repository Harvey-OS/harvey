/* From sys/sys/queue.h */
#define	TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
}

#define	TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}

#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}


/* From sys/sys/cdefs.h */
#define	__BEGIN_DECLS
#define	__END_DECLS


/* From sys/ddb/ddb.h */

/*
 * Like _DB_SET but also create the function declaration which
 * must be followed immediately by the body; e.g.
 *   _DB_FUNC(_cmd, panic, db_panic, db_cmd_table, 0, NULL)
 *   {
 *	...panic implementation...
 *   }
 *
 * This macro is mostly used to define commands placed in one of
 * the ddb command tables; see DB_COMMAND, etc. below.
 */
#define	_DB_FUNC(_suffix, _name, _func, list, _flag, _more)	\
static db_cmdfcn_t _func;					\
_DB_SET(_suffix, _name, _func, list, _flag, _more);		\
static void							\
_func(db_expr_t addr, bool have_addr, db_expr_t count, char *modif)


#define	DB_SHOW_COMMAND(cmd_name, func_name) \
	_DB_FUNC(_show, cmd_name, func_name, db_show_table, 0, NULL)
