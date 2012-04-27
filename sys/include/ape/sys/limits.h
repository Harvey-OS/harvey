/*
	local limits
*/

#undef	ARG_MAX
#define	ARG_MAX		16384
#undef	CHILD_MAX
#define	CHILD_MAX	75
#undef	OPEN_MAX
#define	OPEN_MAX	96
#undef	LINK_MAX
#define	LINK_MAX	1
#undef	NAME_MAX
#define	NAME_MAX	27
#undef	PATH_MAX
#define	PATH_MAX	1023
#undef	NGROUPS_MAX
#define	NGROUPS_MAX	32
#undef	MAX_CANON
#define	MAX_CANON	1023
#undef	MAX_INPUT
#define	MAX_INPUT	1023
#undef	PIPE_BUF
#define	PIPE_BUF	8192

#define	_POSIX_SAVED_IDS		1
#define	_POSIX_CHOWN_RESTRICTED		1
#define	_POSIX_NO_TRUNC			1
