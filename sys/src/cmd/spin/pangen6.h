/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/***** spin: pangen6.h *****/

/* Copyright (c) 2006-2007 by the California Institute of Technology.     */
/* ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged */
/* Supporting routines for a multi-core extension of the SPIN software    */
/* Developed as part of Reliable Software Engineering Project ESAS/6G     */
/* Like all SPIN Software this software is for educational purposes only. */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Any commercial use must be negotiated with the Office of Technology    */
/* Transfer at the California Institute of Technology.                    */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Bug-reports and/or questions can be send to: bugs@spinroot.com         */

static char* Code2c[] = {
    /* multi-core option - Spin 5.0 and later */
    "#if NCORE>1", "#if defined(WIN32) || defined(WIN64)",
    "	#ifndef _CONSOLE", "		#define _CONSOLE", "	#endif",
    "	#ifdef WIN64", "		#undef long", "	#endif",
    "	#include <windows.h>", "	#ifdef WIN64",
    "		#define long	long long", "	#endif", "#else",
    "	#include <sys/ipc.h>", "	#include <sys/sem.h>",
    "	#include <sys/shm.h>", "#endif", "",
    "/* code common to cygwin/linux and win32/win64: */", "", "#ifdef VERBOSE",
    "	#define VVERBOSE	(1)", "#else",
    "	#define VVERBOSE	(0)", "#endif", "",
    "/* the following values must be larger than 256 and must fit in an int */",
    "#define QUIT		1024	/* terminate now command */",
    "#define QUERY		 512	/* termination status query message */",
    "#define QUERY_F	 513	/* query failed, cannot quit */", "",
    "#define GN_FRAMES	(int) (GWQ_SIZE / (double) sizeof(SM_frame))",
    "#define LN_FRAMES	(int) (LWQ_SIZE / (double) sizeof(SM_frame))", "",
    "#ifndef VMAX", "	#define VMAX	VECTORSZ", "#endif", "#ifndef PMAX",
    "	#define PMAX	64", "#endif", "#ifndef QMAX",
    "	#define QMAX	64", "#endif", "", "#if VECTORSZ>32000",
    "	#define OFFT	int", "#else", "	#define OFFT	short",
    "#endif", "", "#ifdef SET_SEG_SIZE",
    "	/* no longer usefule -- being recomputed for local heap size anyway */",
    "	double SEG_SIZE = (((double) SET_SEG_SIZE) * 1048576.);", "#else",
    "	double SEG_SIZE = (1048576.*1024.);	/* 1GB default shared memory "
    "pool segments */",
    "#endif", "", "double LWQ_SIZE = 0.; /* initialized in main */", "",
    "#ifdef SET_WQ_SIZE", "	#ifdef NGQ",
    "	#warning SET_WQ_SIZE applies to global queue -- ignored",
    "	double GWQ_SIZE = 0.;", "	#else",
    "	double GWQ_SIZE = (((double) SET_WQ_SIZE) * 1048576.);",
    "	/* must match the value in pan_proxy.c, if used */", "	#endif",
    "#else", "	#ifdef NGQ", "	double GWQ_SIZE = 0.;", "	#else",
    "	double GWQ_SIZE = (128.*1048576.);	/* 128 MB default queue sizes "
    "*/",
    "	#endif", "#endif", "", "/* Crash Detection Parameters */",
    "#ifndef ONESECOND",
    "	#define ONESECOND	(1<<25)", /* name is somewhat of a misnomer */
    "#endif", "#ifndef SHORT_T", "	#define SHORT_T	(0.1)", "#endif",
    "#ifndef LONG_T", "	#define LONG_T	(600)", "#endif", "",
    "double OneSecond   = (double) (ONESECOND); /* waiting for a free slot -- "
    "checks crash */",
    "double TenSeconds  = 10. * (ONESECOND);    /* waiting for a lock -- check "
    "for a crash */",
    "", "/* Termination Detection Params -- waiting for new state input in "
        "Get_Full_Frame */",
    "double Delay       = ((double) SHORT_T) * (ONESECOND);	/* termination "
    "detection trigger */",
    "double OneHour     = ((double) LONG_T) * (ONESECOND);	/* timeout "
    "termination detection */",
    "", "typedef struct SM_frame     SM_frame;",
    "typedef struct SM_results   SM_results;",
    "typedef struct sh_Allocater sh_Allocater;", "",
    "struct SM_frame {			/* about 6K per slot */",
    "	volatile int	m_vsize;	/* 0 means free slot */",
    "	volatile int	m_boq;		/* >500 is a control message */",
    "#ifdef FULL_TRAIL",
    "	volatile struct Stack_Tree *m_stack;	/* ptr to previous state */",
    "#endif", "	volatile uchar	m_tau;", "	volatile uchar	m_o_pm;",
    "	volatile int	nr_handoffs;	/* to compute real_depth */",
    "	volatile char	m_now     [VMAX];",
    "	volatile char	m_Mask    [(VMAX + 7)/8];",
    "	volatile OFFT	m_p_offset[PMAX];",
    "	volatile OFFT	m_q_offset[QMAX];",
    "	volatile uchar	m_p_skip  [PMAX];",
    "	volatile uchar	m_q_skip  [QMAX];",
    "#if defined(C_States) && (HAS_TRACK==1) && (HAS_STACK==1)",
    "	volatile uchar	m_c_stack [StackSize];",
    /* captures contents of c_stack[] for unmatched objects */
    "#endif", "};", "", "int	proxy_pid;		/* id of proxy if "
                        "nonzero -- receive half */",
    "int	store_proxy_pid;", "short	remote_party;",
    "int	proxy_pid_snd;		/* id of proxy if nonzero -- send half "
    "*/",
    "char	o_cmdline[512];		/* to pass options to children */", "",
    "int	iamin[CS_NR+NCORE];		/* non-shared */", "",
    "#if defined(WIN32) || defined(WIN64)", "int tas(volatile LONG *);", "",
    "HANDLE		proxy_handle_snd;	/* for Windows Create and "
    "Terminate */",
    "", "struct sh_Allocater {			/* shared memory for states */",
    "	volatile char	*dc_arena;	/* to allocate states from */",
    "	volatile long	 pattern;	/* to detect overruns */",
    "	volatile long	 dc_size;	/* nr of bytes left */",
    "	volatile void	*dc_start;	/* where memory segment starts */",
    "	volatile void	*dc_id;		/* to attach, detach, remove shared "
    "memory segments */",
    "	volatile sh_Allocater *nxt;	/* linked list of pools */", "};",
    "DWORD		worker_pids[NCORE];	/* root mem of pids of all "
    "workers created */",
    "HANDLE		worker_handles[NCORE];	/* for windows Create and "
    "Terminate */",
    "void *		shmid      [NR_QS];	/* return value from "
    "CreateFileMapping */",
    "void *		shmid_M;		/* shared mem for state "
    "allocation in hashtable */",
    "", "#ifdef SEP_STATE", "	void *shmid_X;", "#else",
    "	void *shmid_S;			/* shared bitstate arena or hashtable "
    "*/",
    "#endif", "#else", "int tas(volatile int *);", "",
    "struct sh_Allocater {			/* shared memory for states */",
    "	volatile char	*dc_arena;	/* to allocate states from */",
    "	volatile long	 pattern;	/* to detect overruns */",
    "	volatile long	 dc_size;	/* nr of bytes left */",
    "	volatile char	*dc_start;	/* where memory segment starts */",
    "	volatile int	dc_id;		/* to attach, detach, remove shared "
    "memory segments */",
    "	volatile sh_Allocater *nxt;	/* linked list of pools */", "};", "",
    "int	worker_pids[NCORE];	/* root mem of pids of all workers "
    "created */",
    "int	shmid      [NR_QS];	/* return value from shmget */",
    "int	nibis = 0;		/* set after shared mem has been "
    "released */",
    "int	shmid_M;		/* shared mem for state allocation in "
    "hashtable */",
    "#ifdef SEP_STATE", "	long	shmid_X;", "#else",
    "	int	shmid_S;	/* shared bitstate arena or hashtable */",
    "	volatile sh_Allocater	*first_pool;	/* of shared state memory */",
    "	volatile sh_Allocater	*last_pool;", "#endif", /* SEP_STATE */
    "#endif",                                           /* WIN32 || WIN64 */
    "", "struct SM_results {			/* for shuttling back final "
        "stats */",
    "	volatile int	m_vsize;	/* avoid conflicts with frames */",
    "	volatile int	m_boq;		/* these 2 fields are not written in "
    "record_info */",
    "	/* probably not all fields really need to be volatile */",
    "	volatile double	m_memcnt;", "	volatile double	m_nstates;",
    "	volatile double	m_truncs;", "	volatile double	m_truncs2;",
    "	volatile double	m_nShadow;", "	volatile double	m_nlinks;",
    "	volatile double	m_ngrabs;", "	volatile double	m_nlost;",
    "	volatile double	m_hcmp;", "	volatile double	m_frame_wait;",
    "	volatile int	m_hmax;", "	volatile int	m_svmax;",
    "	volatile int	m_smax;", "	volatile int	m_mreached;",
    "	volatile int	m_errors;", "	volatile int	m_VMAX;",
    "	volatile short	m_PMAX;", "	volatile short	m_QMAX;",
    "	volatile uchar	m_R;		/* reached info for all proctypes */",
    "};", "", "int		core_id = 0;		/* internal process nr, "
              "to know which q to use */",
    "unsigned long	nstates_put = 0;	/* statistics */",
    "unsigned long	nstates_get = 0;",
    "int		query_in_progress = 0;	/* termination detection */",
    "", "double		free_wait  = 0.;	/* waiting for a free frame */",
    "double		frame_wait = 0.;	/* waiting for a full frame */",
    "double		lock_wait  = 0.;	/* waiting for access to cs */",
    "double		glock_wait[3];	/* waiting for access to global lock "
    "*/",
    "", "char		*sprefix = \"rst\";",
    "uchar		was_interrupted, issued_kill, writing_trail;", "",
    "static SM_frame cur_Root;		/* current root, to be safe with error "
    "trails */",
    "",
    "SM_frame	*m_workq   [NR_QS];	/* per cpu work queues + global q */",
    "char		*shared_mem[NR_QS];	/* return value from shmat */",
    "#ifdef SEP_HEAP", "char		*my_heap;",
    "long		 my_size;", "#endif", "volatile sh_Allocater	"
                                              "*dc_shared;	/* assigned at "
                                              "initialization */",
    "", "static int	vmax_seen, pmax_seen, qmax_seen;",
    "static double	gq_tries, gq_hasroom, gq_hasnoroom;", "",
    "volatile int *prfree;", /* [NCORE] */
    "volatile int *prfull;", /* [NCORE] */
    "volatile int *prcnt;",  /* [NCORE] */
    "volatile int *prmax;",  /* [NCORE] */
    "", "volatile int	*sh_lock;	/* mutual exclusion locks - in shared "
        "memory */",
    "volatile double *is_alive;	/* to detect when processes crash */",
    "volatile int    *grfree, *grfull, *grcnt, *grmax;	/* access to shared "
    "global q */",
    "volatile double *gr_readmiss, *gr_writemiss;", "static   int	lrfree;	"
                                                    "	/* used for temporary "
                                                    "recording of slot */",
    "static   int dfs_phase2;", "",
    "void mem_put(int);		/* handoff state to other cpu */",
    "void mem_put_acc(void);	/* liveness mode */",
    "void mem_get(void);		/* get state from work queue  */",
    "void sudden_stop(char *);", "#if 0", "void enter_critical(int);",
    "void leave_critical(int);", "#endif", "", "void",
    "record_info(SM_results *r)", "{	int i;", "	uchar *ptr;", "",
    "#ifdef SEP_STATE", "	if (0)", "	{	cpu_printf(\"nstates "
                                         "%%g nshadow %%g -- memory %%-6.3f "
                                         "Mb\\n\",",
    "			nstates, nShadow, memcnt/(1048576.));", "	}",
    "	r->m_memcnt = 0;", "#else", "	#ifdef BITSTATE",
    "	r->m_memcnt = 0; /* it's shared */", "	#endif",
    "	r->m_memcnt = memcnt;", "#endif", "	if (a_cycles && core_id == 1)",
    "	{	r->m_nstates  = nstates;",
    "		r->m_nShadow  = nstates;", "	} else",
    "	{	r->m_nstates  = nstates;",
    "		r->m_nShadow  = nShadow;", "	}",
    "	r->m_truncs   = truncs;", "	r->m_truncs2  = truncs2;",
    "	r->m_nlinks   = nlinks;", "	r->m_ngrabs   = ngrabs;",
    "	r->m_nlost    = nlost;", "	r->m_hcmp     = hcmp;",
    "	r->m_frame_wait = frame_wait;", "	r->m_hmax     = hmax;",
    "	r->m_svmax    = svmax;", "	r->m_smax     = smax;",
    "	r->m_mreached = mreached;", "	r->m_errors   = errors;",
    "	r->m_VMAX     = vmax_seen;", "	r->m_PMAX     = (short) pmax_seen;",
    "	r->m_QMAX     = (short) qmax_seen;", "	ptr = (uchar *) &(r->m_R);",
    "	for (i = 0; i <= _NP_; i++)	/* all proctypes */",
    "	{	memcpy(ptr, reached[i], NrStates[i]*sizeof(uchar));",
    "		ptr += NrStates[i]*sizeof(uchar);", "	}", "	if (verbose>1)",
    "	{	cpu_printf(\"Put Results nstates %%g (sz %%d)\\n\", nstates, "
    "ptr - &(r->m_R));",
    "	}", "}", "", "void snapshot(void);", "", "void",
    "retrieve_info(SM_results *r)", "{	int i, j;", "	volatile uchar *ptr;",
    "", "	snapshot();	/* for a final report */", "",
    "	enter_critical(GLOBAL_LOCK);", "#ifdef SEP_HEAP", "	if (verbose)",
    "	{	printf(\"cpu%%d: local heap-left %%ld KB (%%d MB)\\n\",",
    "			core_id, (long) (my_size/1024), (int) "
    "(my_size/1048576));",
    "	}", "#endif", "	if (verbose && core_id == 0)",
    "	{	printf(\"qmax: \");", "		for (i = 0; i < NCORE; i++)",
    "		{	printf(\"%%d \", prmax[i]);", "		}",
    "#ifndef NGQ", "		printf(\"G: %%d\", *grmax);", "#endif",
    "		printf(\"\\n\");", "	}", "	leave_critical(GLOBAL_LOCK);",
    "", "	memcnt  += r->m_memcnt;", "	nstates += r->m_nstates;",
    "	nShadow += r->m_nShadow;", "	truncs  += r->m_truncs;",
    "	truncs2 += r->m_truncs2;", "	nlinks  += r->m_nlinks;",
    "	ngrabs  += r->m_ngrabs;", "	nlost   += r->m_nlost;",
    "	hcmp    += r->m_hcmp;", "	/* frame_wait += r->m_frame_wait; */",
    "	errors  += r->m_errors;", "",
    "	if (hmax  < r->m_hmax)  hmax  = r->m_hmax;",
    "	if (svmax < r->m_svmax) svmax = r->m_svmax;",
    "	if (smax  < r->m_smax)  smax  = r->m_smax;",
    "	if (mreached < r->m_mreached) mreached = r->m_mreached;", "",
    "	if (vmax_seen < r->m_VMAX) vmax_seen = r->m_VMAX;",
    "	if (pmax_seen < (int) r->m_PMAX) pmax_seen = (int) r->m_PMAX;",
    "	if (qmax_seen < (int) r->m_QMAX) qmax_seen = (int) r->m_QMAX;", "",
    "	ptr = &(r->m_R);",
    "	for (i = 0; i <= _NP_; i++)	/* all proctypes */",
    "	{	for (j = 0; j < NrStates[i]; j++)",
    "		{	if (*(ptr + j) != 0)",
    "			{	reached[i][j] = 1;", "		}	}",
    "		ptr += NrStates[i]*sizeof(uchar);", "	}", "	if (verbose>1)",
    "	{	cpu_printf(\"Got Results (%%d)\\n\", (int) (ptr - &(r->m_R)));",
    "		snapshot();", "	}", "}", "",
    "#if !defined(WIN32) && !defined(WIN64)", "static void",
    "rm_shared_segments(void)", "{	int m;",
    "	volatile sh_Allocater *nxt_pool;", "	/*",
    "	 * mark all shared memory segments for removal ",
    "	 * the actual removes wont happen intil last process dies or detaches",
    "	 * the shmctl calls can return -1 if not all procs have detached yet",
    "	 */", "	for (m = 0; m < NR_QS; m++)	/* +1 for global q */",
    "	{	if (shmid[m] != -1)",
    "		{	(void) shmctl(shmid[m], IPC_RMID, NULL);",
    "	}	}", "#ifdef SEP_STATE", "	if (shmid_M != -1)",
    "	{	(void) shmctl(shmid_M, IPC_RMID, NULL);", "	}", "#else",
    "	if (shmid_S != -1)",
    "	{	(void) shmctl(shmid_S, IPC_RMID, NULL);", "	}",
    "	for (last_pool = first_pool; last_pool != NULL; last_pool = nxt_pool)",
    "	{	shmid_M = (int) (last_pool->dc_id);",
    "		nxt_pool = last_pool->nxt;	/* as a pre-caution only */",
    "		if (shmid_M != -1)",
    "		{	(void) shmctl(shmid_M, IPC_RMID, NULL);",
    "	}	}", "#endif", "}", "#endif", "", "void", "sudden_stop(char *s)",
    "{	char b[64];", "	int i;", "",
    "	printf(\"cpu%%d: stop - %%s\\n\", core_id, s);",
    "#if !defined(WIN32) && !defined(WIN64)", "	if (proxy_pid != 0)",
    "	{	rm_shared_segments();", "	}", "#endif",
    "	if (search_terminated != NULL)",
    "	{	if (*search_terminated != 0)",
    "		{	if (verbose)", "			{	"
                                       "printf(\"cpu%%d: termination initiated "
                                       "(%%d)\\n\",",
    "					core_id, *search_terminated);",
    "			}", "		} else",
    "		{	if (verbose)", "			{	"
                                       "printf(\"cpu%%d: initiated "
                                       "termination\\n\", core_id);",
    "			}",
    "			*search_terminated |= 8;	/* sudden_stop */",
    "		}", "		if (core_id == 0)", "		{	if "
                                                    "(((*search_terminated) & "
                                                    "4)	/* uerror in "
                                                    "one of the cpus */",
    "			&& !((*search_terminated) & (8|32|128|256))) /* "
    "abnormal stop */",
    "			{	if (errors == 0) errors++; /* we know there is "
    "at least 1 */",
    "			}", "			wrapup(); /* incomplete stats, "
                            "but at least something */",
    "		}", "		return;",
    "	} /* else: should rarely happen, take more drastic measures */", "",
    "	if (core_id == 0)	/* local root process */",
    "	{	for (i = 1; i < NCORE; i++)	/* not for 0 of course */",
    "		{", "#if defined(WIN32) || defined(WIN64)",
    "				DWORD dwExitCode = 0;",
    "				GetExitCodeProcess(worker_handles[i], "
    "&dwExitCode);",
    "				if (dwExitCode == STILL_ACTIVE)",
    "				{	TerminateProcess(worker_handles[i], "
    "0);",
    "				}",
    "				printf(\"cpu0: terminate %%d %%d\\n\",",
    "					worker_pids[i], (dwExitCode == "
    "STILL_ACTIVE));",
    "#else", "				sprintf(b, \"kill -%%d %%d\", SIGKILL, "
             "worker_pids[i]);",
    "				system(b);	/* if this is a proxy: receive "
    "half */",
    "				printf(\"cpu0: %%s\\n\", b);", "#endif",
    "		}", "		issued_kill++;", "	} else",
    "	{	/* on WIN32/WIN64 -- these merely kills the root process... */",
    "		if (was_interrupted == 0)", /* 2=SIGINT to root to trigger stop
                                               */
    "		{	sprintf(b, \"kill -%%d %%d\", SIGINT, worker_pids[0]);",
    "			system(b);	/* warn the root process */",
    "			printf(\"cpu%%d: %%s\\n\", core_id, b);",
    "			issued_kill++;", "	}	}", "}", "",
    "#define iam_alive()	is_alive[core_id]++", /* for crash detection */
    "", "extern int crash_test(double);", "extern void crash_reset(void);", "",
    "int", "someone_crashed(int wait_type)",
    "{	static double last_value = 0.0;", "	static int count = 0;", "",
    "	if (search_terminated == NULL", "	|| *search_terminated != 0)",
    "	{", "		if (!(*search_terminated & (8|32|128|256)))",
    "		{	if (count++ < 100*NCORE)",
    "			{	return 0;", "		}	}",
    "		return 1;", "	}", "	/* check left neighbor only */",
    "	if (last_value == is_alive[(core_id + NCORE - 1) %% NCORE])",
    "	{	if (count++ >= 100)	/* to avoid unnecessary checks */",
    "		{	return 1;", "		}", "		return 0;",
    "	}", "	last_value = is_alive[(core_id + NCORE - 1) %% NCORE];",
    "	count = 0;", "	crash_reset();", "	return 0;", "}", "", "void",
    "sleep_report(void)", "{", "	enter_critical(GLOBAL_LOCK);",
    "	if (verbose)", "	{", "#ifdef NGQ",
    "		printf(\"cpu%%d: locks: global %%g\\tother %%g\\t\",",
    "			core_id, glock_wait[0], lock_wait - glock_wait[0]);",
    "#else",
    "		printf(\"cpu%%d: locks: GL %%g, RQ %%g, WQ %%g, HT %%g\\t\",",
    "			core_id, glock_wait[0], glock_wait[1], glock_wait[2],",
    "			lock_wait - glock_wait[0] - glock_wait[1] - "
    "glock_wait[2]);",
    "#endif", "		printf(\"waits: states %%g slots %%g\\n\", "
              "frame_wait, free_wait);",
    "#ifndef NGQ", "		printf(\"cpu%%d: gq [tries %%g, room %%g, "
                   "noroom %%g]\\n\", core_id, gq_tries, gq_hasroom, "
                   "gq_hasnoroom);",
    "		if (core_id == 0 && (*gr_readmiss >= 1.0 || *gr_readmiss >= 1.0 "
    "|| *grcnt != 0))",
    "		printf(\"cpu0: gq [readmiss: %%g, writemiss: %%g cnt %%d]\\n\", "
    "*gr_readmiss, *gr_writemiss, *grcnt);",
    "#endif", "	}", "	if (free_wait > 1000000.)", "	#ifndef NGQ",
    "	if (!a_cycles)", "	{	printf(\"hint: this search may be "
                         "faster with a larger work-queue\\n\");",
    "		printf(\"	(-DSET_WQ_SIZE=N with N>%%g), and/or with "
    "-DUSE_DISK\\n\",",
    "			GWQ_SIZE/sizeof(SM_frame));",
    "		printf(\"      or with a larger value for -zN (N>%%ld)\\n\", "
    "z_handoff);",
    "	#else", "	{	printf(\"hint: this search may be faster if "
                "compiled without -DNGQ, with -DUSE_DISK, \");",
    "		printf(\"or with a larger -zN (N>%%d)\\n\", z_handoff);",
    "	#endif", "	}", "	leave_critical(GLOBAL_LOCK);", "}", "",
    "#ifndef MAX_DSK_FILE", "	#define MAX_DSK_FILE	1000000	/* default is "
                            "max 1M states per file */",
    "#endif", "", "void", "multi_usage(FILE *fd)", "{	static int warned = 0;",
    "	if (warned > 0) { return; } else { warned++; }",
    "	fprintf(fd, \"\\n\");",
    "	fprintf(fd, \"Defining multi-core mode:\\n\\n\");",
    "	fprintf(fd, \"        -DDUAL_CORE --> same as -DNCORE=2\\n\");",
    "	fprintf(fd, \"        -DQUAD_CORE --> same as -DNCORE=4\\n\");",
    "	fprintf(fd, \"        -DNCORE=N   --> enables multi_core verification "
    "if N>1\\n\");",
    "	fprintf(fd, \"\\n\");", "	fprintf(fd, \"Additional directives "
                                "supported in multi-core mode:\\n\\n\");",
    "	fprintf(fd, \"        -DSEP_STATE --> forces separate statespaces "
    "instead of a single shared state space\\n\");",
    "	fprintf(fd, \"        -DNUSE_DISK --> use disk for storing states when "
    "a work queue overflows\\n\");",
    "	fprintf(fd, \"        -DMAX_DSK_FILE --> max nr of states per diskfile "
    "(%%d)\\n\", MAX_DSK_FILE);",
    "	fprintf(fd, \"        -DFULL_TRAIL --> support full error trails "
    "(increases memory use)\\n\");",
    "	fprintf(fd, \"\\n\");",
    "	fprintf(fd, \"More advanced use (should rarely need "
    "changing):\\n\\n\");",
    "	fprintf(fd, \"     To change the nr of states that can be stored in the "
    "global queue\\n\");",
    "	fprintf(fd, \"     (lower numbers allow for more states to be stored, "
    "prefer multiples of 8):\\n\");",
    "	fprintf(fd, \"        -DVMAX=N    --> upperbound on statevector for "
    "handoffs (N=%%d)\\n\", VMAX);",
    "	fprintf(fd, \"        -DPMAX=N    --> upperbound on nr of procs "
    "(default: N=%%d)\\n\", PMAX);",
    "	fprintf(fd, \"        -DQMAX=N    --> upperbound on nr of channels "
    "(default: N=%%d)\\n\", QMAX);",
    "	fprintf(fd, \"\\n\");",
#if 0
	"#if !defined(WIN32) && !defined(WIN64)",
	"	fprintf(fd, \"     To change the size of spin's individual shared memory segments for cygwin/linux:\\n\");",
	"	fprintf(fd, \"        -DSET_SEG_SIZE=N --> default %%g (Mbytes)\\n\", SEG_SIZE/(1048576.));",
	"	fprintf(fd, \"\\n\");",
	"#endif",
#endif
    "	fprintf(fd, \"     To set the total amount of memory reserved for the "
    "global workqueue:\\n\");",
    "	fprintf(fd, \"        -DSET_WQ_SIZE=N --> default: N=128 (defined in "
    "MBytes)\\n\\n\");",
#if 0
	"	fprintf(fd, \"     To omit the global workqueue completely (bad idea):\\n\");",
	"	fprintf(fd, \"        -DNGQ\\n\\n\");",
#endif
    "	fprintf(fd, \"     To force the use of a single global heap, instead of "
    "separate heaps:\\n\");",
    "	fprintf(fd, \"        -DGLOB_HEAP\\n\");", "	fprintf(fd, \"\\n\");",
    "	fprintf(fd, \"     To define a fct to initialize data before spawning "
    "processes (use quotes):\\n\");",
    "	fprintf(fd, \"        \\\"-DC_INIT=fct()\\\"\\n\");",
    "	fprintf(fd, \"\\n\");", "	fprintf(fd, \"     Timer settings for "
                                "termination and crash detection:\\n\");",
    "	fprintf(fd, \"        -DSHORT_T=N --> timeout for termination detection "
    "trigger (N=%%g)\\n\", (double) SHORT_T);",
    "	fprintf(fd, \"        -DLONG_T=N  --> timeout for giving up on "
    "termination detection (N=%%g)\\n\", (double) LONG_T);",
    "	fprintf(fd, \"        -DONESECOND --> (1<<29) --> timeout waiting for a "
    "free slot -- to check for crash\\n\");",
    "	fprintf(fd, \"        -DT_ALERT   --> collect stats on crash alert "
    "timeouts\\n\\n\");",
    "	fprintf(fd, \"Help with Linux/Windows/Cygwin configuration for "
    "multi-core:\\n\");",
    "	fprintf(fd, \"	"
    "http://spinroot.com/spin/multicore/V5_Readme.html\\n\");",
    "	fprintf(fd, \"\\n\");", "}", "#if NCORE>1 && defined(FULL_TRAIL)",
    "typedef struct Stack_Tree {",
    "	uchar	      pr;	/* process that made transition */",
    "	T_ID	    t_id;	/* id of transition */",
    "	volatile struct Stack_Tree *prv; /* backward link towards root */",
    "} Stack_Tree;", "", "struct H_el *grab_shared(int);",
    "volatile Stack_Tree **stack_last; /* in shared memory */",
    "char *stack_cache = NULL;	/* local */",
    "int  nr_cached = 0;		/* local */", "", "#ifndef CACHE_NR",
    "	#define CACHE_NR	1024", "#endif", "", "volatile Stack_Tree *",
    "stack_prefetch(void)", "{	volatile Stack_Tree *st;", "",
    "	if (nr_cached == 0)", "	{	stack_cache = (char *) "
                              "grab_shared(CACHE_NR * sizeof(Stack_Tree));",
    "		nr_cached = CACHE_NR;", "	}",
    "	st = (volatile Stack_Tree *) stack_cache;",
    "	stack_cache += sizeof(Stack_Tree);", "	nr_cached--;",
    "	return st;", "}", "", "void", "Push_Stack_Tree(short II, T_ID t_id)",
    "{	volatile Stack_Tree *st;", "",
    "	st = (volatile Stack_Tree *) stack_prefetch();", "	st->pr = II;",
    "	st->t_id = t_id;", "	st->prv = (Stack_Tree *) stack_last[core_id];",
    "	stack_last[core_id] = st;", "}", "", "void", "Pop_Stack_Tree(void)",
    "{	volatile Stack_Tree *cf = stack_last[core_id];", "", "	if (cf)",
    "	{	stack_last[core_id] = cf->prv;",
    "	} else if (nr_handoffs * z_handoff + depth > 0)",
    "	{	printf(\"cpu%%d: error pop_stack_tree (depth %%d)\\n\",",
    "			core_id, depth);", "	}", "}",
    "#endif", /* NCORE>1 && FULL_TRAIL */
    "", "void", "e_critical(int which)", "{	double cnt_start;", "",
    "	if (readtrail || iamin[which] > 0)",
    "	{	if (!readtrail && verbose)",
    "		{	printf(\"cpu%%d: Double Lock on %%d (now %%d)\\n\",",
    "				core_id, which, iamin[which]+1);",
    "			fflush(stdout);", "		}",
    "		iamin[which]++;	/* local variable */",
    "		return;", "	}", "", "	cnt_start = lock_wait;", "",
    "	while (sh_lock != NULL)	/* as long as we have shared memory */",
    "	{	int r = tas(&sh_lock[which]);", "		if (r == 0)",
    "		{	iamin[which] = 1;",
    "			return;		/* locked */", "		}", "",
    "		lock_wait++;", "#ifndef NGQ",
    "		if (which < 3) { glock_wait[which]++; }", "#else",
    "		if (which == 0) { glock_wait[which]++; }", "#endif",
    "		iam_alive();", "",
    "		if (lock_wait - cnt_start > TenSeconds)",
    "		{	printf(\"cpu%%d: lock timeout on %%d\\n\", core_id, "
    "which);",
    "			cnt_start = lock_wait;",
    "			if (someone_crashed(1))",
    "			{	sudden_stop(\"lock timeout\");",
    "				pan_exit(1);", "	}	}	}", "}",
    "", "void", "x_critical(int which)", "{", "	if (iamin[which] != 1)",
    "	{	if (iamin[which] > 1)",
    "		{	iamin[which]--;	/* this is thread-local - no races on "
    "this one */",
    "			if (!readtrail && verbose)",
    "			{	printf(\"cpu%%d: Partial Unlock on %%d (%%d "
    "more needed)\\n\",",
    "					core_id, which, iamin[which]);",
    "				fflush(stdout);", "			}",
    "			return;", "		} else /* iamin[which] <= 0 */",
    "		{	if (!readtrail)", "			{	"
                                          "printf(\"cpu%%d: Invalid Unlock "
                                          "iamin[%%d] = %%d\\n\",",
    "					core_id, which, iamin[which]);",
    "				fflush(stdout);", "			}",
    "			return;", "	}	}", "",
    "	if (sh_lock != NULL)", "	{	iamin[which]   = 0;",
    "		sh_lock[which] = 0;	/* unlock */", "	}", "}", "",
    "void", "#if defined(WIN32) || defined(WIN64)",
    "start_proxy(char *s, DWORD r_pid)", "#else",
    "start_proxy(char *s, int r_pid)", "#endif",
    "{	char  Q_arg[16], Z_arg[16], Y_arg[16];", "	char *args[32], *ptr;",
    "	int   argcnt = 0;", "", "	sprintf(Q_arg, \"-Q%%d\", getpid());",
    "	sprintf(Y_arg, \"-Y%%d\", r_pid);",
    "	sprintf(Z_arg, \"-Z%%d\", proxy_pid /* core_id */);", "",
    "	args[argcnt++] = \"proxy\";", "	args[argcnt++] = s; /* -r or -s */",
    "	args[argcnt++] = Q_arg;", "	args[argcnt++] = Z_arg;",
    "	args[argcnt++] = Y_arg;", "", "	if (strlen(o_cmdline) > 0)",
    "	{	ptr = o_cmdline; /* assume args separated by spaces */",
    "		do {	args[argcnt++] = ptr++;",
    "			if ((ptr = strchr(ptr, ' ')) != NULL)",
    "			{	while (*ptr == ' ')",
    "				{	*ptr++ = '\\0';",
    "				}", "			} else",
    "			{	break;", "			}",
    "		} while (argcnt < 31);", "	}", "	args[argcnt] = NULL;",
    "#if defined(WIN32) || defined(WIN64)",
    "	execvp(\"pan_proxy\", args); /* no return */", "#else",
    "	execvp(\"./pan_proxy\", args); /* no return */", "#endif",
    "	Uerror(\"pan_proxy exec failed\");", "}",
    "/*** end of common code fragment ***/", "",
    "#if !defined(WIN32) && !defined(WIN64)",
    "void", "init_shm(void)		/* initialize shared work-queues - "
            "linux/cygwin */",
    "{	key_t	key[NR_QS];", "	int	n, m;", "	int	must_exit = 0;",
    "", "	if (core_id == 0 && verbose)",
    "	{	printf(\"cpu0: step 3: allocate shared workqueues %%g MB\\n\",",
    "			((double) NCORE * LWQ_SIZE + GWQ_SIZE) / (1048576.) );",
    "	}",
    "	for (m = 0; m < NR_QS; m++)		/* last q is the global q */",
    "	{	double qsize = (m == NCORE) ? GWQ_SIZE : LWQ_SIZE;",
    "		key[m] = ftok(PanSource, m+1);", /* m must be nonzero, 1..NCORE
                                                    */
    "		if (key[m] == -1)",
    "		{	perror(\"ftok shared queues\"); must_exit = 1; break;",
    "		}", "",
    "		if (core_id == 0)	/* root creates */",
    "		{	/* check for stale copy */",
    "			shmid[m] = shmget(key[m], (size_t) qsize, 0600);",
    "			if (shmid[m] != -1)	/* yes there is one; remove it "
    "*/",
    "			{	printf(\"cpu0: removing stale q%%d, status: "
    "%%d\\n\",",
    "					m, shmctl(shmid[m], IPC_RMID, NULL));",
    "			}", "			shmid[m] = shmget(key[m], "
                            "(size_t) qsize, 0600|IPC_CREAT|IPC_EXCL);",
    "			memcnt += qsize;",
    "		} else			/* workers attach */",
    "		{	shmid[m] = shmget(key[m], (size_t) qsize, 0600);",
    "			/* never called, since we create shm *before* we fork "
    "*/",
    "		}", "		if (shmid[m] == -1)",
    "		{	perror(\"shmget shared queues\"); must_exit = 1; "
    "break;",
    "		}", "", "		shared_mem[m] = (char *) "
                        "shmat(shmid[m], (void *) 0, 0);	/* attach */",
    "		if (shared_mem[m] == (char *) -1)",
    "		{ fprintf(stderr, \"error: cannot attach shared wq %%d (%%d "
    "Mb)\\n\",",
    "				m+1, (int) (qsize/(1048576.)));",
    "		  perror(\"shmat shared queues\"); must_exit = 1; break;",
    "		}", "",
    "		m_workq[m] = (SM_frame *) shared_mem[m];",
    "		if (core_id == 0)",
    "		{	int nframes = (m == NCORE) ? GN_FRAMES : LN_FRAMES;",
    "			for (n = 0; n < nframes; n++)",
    "			{	m_workq[m][n].m_vsize = 0;",
    "				m_workq[m][n].m_boq = 0;",
    "	}	}	}", "", "	if (must_exit)",
    "	{	rm_shared_segments();", "		fprintf(stderr, \"pan: "
                                        "check './pan --' for usage "
                                        "details\\n\");",
    "		pan_exit(1);	/* calls cleanup_shm */", "	}", "}", "",
    "static uchar *", "prep_shmid_S(size_t n)		/* either sets SS or "
                      "H_tab, linux/cygwin */",
    "{	char	*rval;", "#ifndef SEP_STATE", "	key_t	key;", "",
    "	if (verbose && core_id == 0)", "	{", "	#ifdef BITSTATE",
    "		printf(\"cpu0: step 1: allocate shared bitstate %%g Mb\\n\",",
    "			(double) n / (1048576.));", "	#else",
    "		printf(\"cpu0: step 1: allocate shared hastable %%g Mb\\n\",",
    "			(double) n / (1048576.));", "	#endif", "	}",
    "	#ifdef MEMLIM", /* memlim has a value */
    "	if (memcnt + (double) n > memlim)",
    "	{	printf(\"cpu0: S %%8g + %%d Kb exceeds memory limit of %%8g "
    "Mb\\n\",",
    "			memcnt/1024., (int) (n/1024), memlim/(1048576.));",
    "		printf(\"cpu0: insufficient memory -- aborting\\n\");",
    "		exit(1);", "	}", "	#endif", "",
    "	key = ftok(PanSource, NCORE+2);	/* different from queues */",
    "	if (key == -1)",
    "	{	perror(\"ftok shared bitstate or hashtable\");",
    "		fprintf(stderr, \"pan: check './pan --' for usage "
    "details\\n\");",
    "		pan_exit(1);", "	}", "",
    "	if (core_id == 0)	/* root */",
    "	{	shmid_S = shmget(key, n, 0600);",
    "		if (shmid_S != -1)",
    "		{	printf(\"cpu0: removing stale segment, status: "
    "%%d\\n\",",
    "				(int) shmctl(shmid_S, IPC_RMID, NULL));",
    "		}",
    "		shmid_S = shmget(key, n, 0600 | IPC_CREAT | IPC_EXCL);",
    "		memcnt += (double) n;",
    "	} else			/* worker */",
    "	{	shmid_S = shmget(key, n, 0600);", "	}",
    "	if (shmid_S == -1)",
    "	{	perror(\"shmget shared bitstate or hashtable too large?\");",
    "		fprintf(stderr, \"pan: check './pan --' for usage "
    "details\\n\");",
    "		pan_exit(1);", "	}", "",
    "	rval = (char *) shmat(shmid_S, (void *) 0, 0);	/* attach */",
    "	if ((char *) rval == (char *) -1)",
    "	{	perror(\"shmat shared bitstate or hashtable\");",
    "		fprintf(stderr, \"pan: check './pan --' for usage "
    "details\\n\");",
    "		pan_exit(1);", "	}", "#else",
    "	rval = (char *) emalloc(n);", "#endif", "	return (uchar *) rval;",
    "}", "", "#define TRY_AGAIN	1", "#define NOT_AGAIN	0", "",
    "static char shm_prep_result;", "", "static uchar *",
    "prep_state_mem(size_t n)		/* sets memory arena for states "
    "linux/cygwin */",
    "{	char	*rval;", "	key_t	key;", "	static int cnt = 3;	"
                                               "	/* start larger than "
                                               "earlier ftok calls */",
    "", "	shm_prep_result = NOT_AGAIN;	/* default */",
    "	if (verbose && core_id == 0)", "	{	printf(\"cpu0: step 2+: "
                                       "pre-allocate memory arena %%d of "
                                       "%%6.2g Mb\\n\",",
    "			cnt-3, (double) n / (1048576.));", "	}",
    "	#ifdef MEMLIM", "	if (memcnt + (double) n > memlim)",
    "	{	printf(\"cpu0: error: M %%.0f + %%.0f Kb exceeds memory limit "
    "of %%.0f Mb\\n\",",
    "			memcnt/1024.0, (double) n/1024.0, memlim/(1048576.));",
    "		return NULL;", "	}", "	#endif", "",
    "	key = ftok(PanSource, NCORE+cnt); cnt++;", /* starts at NCORE+3 */
    "	if (key == -1)", "	{	perror(\"ftok T\");",
    "		printf(\"pan: check './pan --' for usage details\\n\");",
    "		pan_exit(1);", "	}", "", "	if (core_id == 0)",
    "	{	shmid_M = shmget(key, n, 0600);",
    "		if (shmid_M != -1)",
    "		{	printf(\"cpu0: removing stale memory segment %%d, "
    "status: %%d\\n\",",
    "				cnt-3, shmctl(shmid_M, IPC_RMID, NULL));",
    "		}",
    "		shmid_M = shmget(key, n, 0600 | IPC_CREAT | IPC_EXCL);",
    "		/* memcnt += (double) n; -- only amount actually used is "
    "counted */",
    "	} else", "	{	shmid_M = shmget(key, n, 0600);", "	",
    "	}", "	if (shmid_M == -1)",
    "	{	if (verbose)", "		{	printf(\"error: failed "
                               "to get pool of shared memory %%d of %%.0f "
                               "Mb\\n\",",
    "				cnt-3, ((double)n)/(1048576.));",
    "			perror(\"state mem\");", "			"
                                                 "printf(\"pan: check './pan "
                                                 "--' for usage details\\n\");",
    "		}", "		shm_prep_result = TRY_AGAIN;",
    "		return NULL;", "	}",
    "	rval = (char *) shmat(shmid_M, (void *) 0, 0);	/* attach */", "",
    "	if ((char *) rval == (char *) -1)", "	{	printf(\"cpu%%d error: "
                                            "failed to attach pool of shared "
                                            "memory %%d of %%.0f Mb\\n\",",
    "			 core_id, cnt-3, ((double)n)/(1048576.));",
    "		perror(\"state mem\");", "		return NULL;",
    "	}", "	return (uchar *) rval;", "}", "", "void",
    "init_HT(unsigned long n)	/* cygwin/linux version */",
    "{	volatile char	*x;", "	double  get_mem;", "#ifndef SEP_STATE",
    "	volatile char	*dc_mem_start;",
    "	double  need_mem, got_mem = 0.;", "#endif", "", "#ifdef SEP_STATE",
    " #ifndef MEMLIM", "	if (verbose)",
    "	{	printf(\"cpu0: steps 0,1: no -DMEMLIM set\\n\");", /* cannot
                                                                      happen */
    "	}",
    " #else", "	if (verbose)", "	{	printf(\"cpu0: steps 0,1: "
                               "-DMEMLIM=%%d Mb - (hashtable %%g Mb + "
                               "workqueues %%g Mb)\\n\",",
    "		MEMLIM, ((double)n/(1048576.)), (((double) NCORE * LWQ_SIZE) + "
    "GWQ_SIZE) /(1048576.) );",
    "	}", " #endif", "	get_mem = NCORE * sizeof(double) + (1 + CS_NR) "
                       "* sizeof(void *) + 4*sizeof(void *) + "
                       "2*sizeof(double);",
    "	/* NCORE * is_alive + search_terminated + CS_NR * sh_lock + 6 gr vars "
    "*/",
    "	get_mem += 4 * NCORE * sizeof(void *); /* prfree, prfull, prcnt, prmax "
    "*/",
    " #ifdef FULL_TRAIL",
    "	get_mem += (NCORE) * sizeof(Stack_Tree *); /* NCORE * stack_last */",
    " #endif",
    "	x = (volatile char *) prep_state_mem((size_t) get_mem); /* work queues "
    "and basic structs */",
    "	shmid_X = (long) x;",
    "	if (x == NULL)", /* do not repeat for smaller sizes */
    "	{	printf(\"cpu0: could not allocate shared memory, see ./pan "
    "--\\n\");",
    "		exit(1);", "	}",
    "	search_terminated = (volatile unsigned int *) x; /* comes first */",
    "	x += sizeof(void *); /* maintain alignment */", "",
    "	is_alive   = (volatile double *) x;", "	x += NCORE * sizeof(double);",
    "", "	sh_lock   = (volatile int *) x;",
    "	x += CS_NR * sizeof(void *);", /* allow 1 word per entry */
    "", "	grfree    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grfull    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grcnt    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grmax    = (volatile int *) x;", "	x += sizeof(void *);",
    "	prfree = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prfull = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prcnt = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prmax = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	gr_readmiss    = (volatile double *) x;", "	x += sizeof(double);",
    "	gr_writemiss    = (volatile double *) x;", "	x += sizeof(double);",
    "", "	#ifdef FULL_TRAIL",
    "		stack_last = (volatile Stack_Tree **) x;",
    "		x += NCORE * sizeof(Stack_Tree *);", "	#endif", "",
    "	#ifndef BITSTATE", "		H_tab = (struct H_el **) emalloc(n);",
    "	#endif", "#else", "	#ifndef MEMLIM",
    "		#warning MEMLIM not set", /* cannot happen */
    "		#define MEMLIM	(2048)", "	#endif", "",
    "	if (core_id == 0 && verbose)", "	{	printf(\"cpu0: step 0: "
                                       "-DMEMLIM=%%d Mb minus hashtable+workqs "
                                       "(%%g + %%g Mb) leaves %%g Mb\\n\",",
    "			MEMLIM, ((double)n/(1048576.)), (NCORE * LWQ_SIZE + "
    "GWQ_SIZE)/(1048576.),",
    "			(memlim - memcnt - (double) n - (NCORE * LWQ_SIZE + "
    "GWQ_SIZE))/(1048576.));",
    "	}", "	#ifndef BITSTATE", "		H_tab = (struct H_el **) "
                                   "prep_shmid_S((size_t) n);	/* hash_table "
                                   "*/",
    "	#endif",
    "	need_mem = memlim - memcnt - ((double) NCORE * LWQ_SIZE) - GWQ_SIZE;",
    "	if (need_mem <= 0.)",
    "	{	Uerror(\"internal error -- shared state memory\");", "	}", "",
    "	if (core_id == 0 && verbose)", "	{	printf(\"cpu0: step 2: "
                                       "pre-allocate shared state memory %%g "
                                       "Mb\\n\",",
    "			need_mem/(1048576.));", "	}", "#ifdef SEP_HEAP",
    "	SEG_SIZE = need_mem / NCORE;", "	if (verbose && core_id == 0)",
    "	{	printf(\"cpu0: setting segsize to %%6g MB\\n\",",
    "			SEG_SIZE/(1048576.));", "	}",
    "	#if defined(CYGWIN) || defined(__CYGWIN__)",
    "	if (SEG_SIZE > 512.*1024.*1024.)", "	{	printf(\"warning: "
                                           "reducing SEG_SIZE of %%g MB to "
                                           "512MB (exceeds max for "
                                           "Cygwin)\\n\",",
    "			SEG_SIZE/(1024.*1024.));",
    "		SEG_SIZE = 512.*1024.*1024.;", "	}", "	#endif",
    "#endif", "	mem_reserved = need_mem;", "	while (need_mem > 1024.)",
    "	{	get_mem = need_mem;", "shm_more:",
    "		if (get_mem > (double) SEG_SIZE)",
    "		{	get_mem = (double) SEG_SIZE;", "		}",
    "		if (get_mem <= 0.0) break;", "",
    "		/* for allocating states: */",
    "		x = dc_mem_start = (volatile char *) prep_state_mem((size_t) "
    "get_mem);",
    "		if (x == NULL)",
    "		{	if (shm_prep_result == NOT_AGAIN",
    "			||  first_pool != NULL",
    "			||  SEG_SIZE < (16. * 1048576.))",
    "			{	break;", "			}",
    "			SEG_SIZE /= 2.;", "			if (verbose)",
    "			{	printf(\"pan: lowered segsize to %f\\n\", "
    "SEG_SIZE);",
    "			}", "			if (SEG_SIZE >= 1024.)",
    "			{	goto shm_more;", /* always terminates */
    "			}", "			break;", "		}", "",
    "		need_mem -= get_mem;", "		got_mem  += get_mem;",
    "		if (first_pool == NULL)",
    "		{	search_terminated = (volatile unsigned int *) x; /* "
    "comes first */",
    "			x += sizeof(void *); /* maintain alignment */", "",
    "			is_alive   = (volatile double *) x;",
    "			x += NCORE * sizeof(double);", "",
    "			sh_lock   = (volatile int *) x;",
    "			x += CS_NR * sizeof(void *);", /* allow 1 word per entry
                                                          */
    "",
    "			grfree    = (volatile int *) x;",
    "			x += sizeof(void *);",
    "			grfull    = (volatile int *) x;",
    "			x += sizeof(void *);",
    "			grcnt    = (volatile int *) x;",
    "			x += sizeof(void *);",
    "			grmax    = (volatile int *) x;",
    "			x += sizeof(void *);",
    "			prfree = (volatile int *) x;",
    "			x += NCORE * sizeof(void *);",
    "			prfull = (volatile int *) x;",
    "			x += NCORE * sizeof(void *);",
    "			prcnt = (volatile int *) x;",
    "			x += NCORE * sizeof(void *);",
    "			prmax = (volatile int *) x;",
    "			x += NCORE * sizeof(void *);",
    "			gr_readmiss  = (volatile double *) x;",
    "			x += sizeof(double);",
    "			gr_writemiss = (volatile double *) x;",
    "			x += sizeof(double);", " #ifdef FULL_TRAIL",
    "			stack_last = (volatile Stack_Tree **) x;",
    "			x += NCORE * sizeof(Stack_Tree *);", " #endif",
    "			if (((long)x)&(sizeof(void *)-1)) /* 64-bit word "
    "alignment */",
    "			{	x += sizeof(void *)-(((long)x)&(sizeof(void "
    "*)-1));",
    "			}", "", "			#ifdef COLLAPSE",
    "			ncomps = (unsigned long *) x;",
    "			x += (256+2) * sizeof(unsigned long);",
    "			#endif", "		}", "",
    "		dc_shared = (sh_Allocater *) x; /* must be in shared memory */",
    "		x += sizeof(sh_Allocater);", "",
    "		if (core_id == 0)	/* root only */",
    "		{	dc_shared->dc_id     = shmid_M;",
    "			dc_shared->dc_start  = dc_mem_start;",
    "			dc_shared->dc_arena  = x;",
    "			dc_shared->pattern   = 1234567; /* protection */",
    "			dc_shared->dc_size   = (long) get_mem - (long) (x - "
    "dc_mem_start);",
    "			dc_shared->nxt       = (long) 0;", "",
    "			if (last_pool == NULL)",
    "			{	first_pool = last_pool = dc_shared;",
    "			} else",
    "			{	last_pool->nxt = dc_shared;",
    "				last_pool = dc_shared;",
    "			}", "		} else if (first_pool == NULL)",
    "		{	first_pool = dc_shared;", "	}	}", "",
    "	if (need_mem > 1024.)", "	{	printf(\"cpu0: could allocate "
                                "only %%g Mb of shared memory (wanted %%g "
                                "more)\\n\",",
    "			got_mem/(1048576.), need_mem/(1048576.));", "	}", "",
    "	if (!first_pool)",
    "	{	printf(\"cpu0: insufficient memory -- aborting.\\n\");",
    "		exit(1);", "	}",
    "	/* we are still single-threaded at this point, with core_id 0 */",
    "	dc_shared = first_pool;", "", "#endif", /* !SEP_STATE */
    "}", "", "	/* Test and Set assembly code */", "",
    "	#if defined(i386) || defined(__i386__) || defined(__x86_64__)",
    "		int", "		tas(volatile int *s)	/* tested */",
    "		{	int r;", "			__asm__ __volatile__(",
    "				\"xchgl %%0, %%1 \\n\\t\"",
    "		       		: \"=r\"(r), \"=m\"(*s)",
    "				: \"0\"(1), \"m\"(*s)",
    "				: \"memory\");", "		",
    "			return r;", "		}", "	#elif defined(__arm__)",
    "		int", "		tas(volatile int *s)	/* not tested */",
    "		{	int r = 1;", "			__asm__ __volatile__(",
    "				\"swpb %%0, %%0, [%%3] \\n\"",
    "				: \"=r\"(r), \"=m\"(*s)",
    "				: \"0\"(r), \"r\"(s));", "",
    "			return r;", "		}",
    "	#elif defined(sparc) || defined(__sparc__)", "		int",
    "		tas(volatile int *s)	/* not tested */",
    "		{	int r = 1;", "			__asm__ __volatile__(",
    "				\" ldstub [%%2], %%0 \\n\"",
    "				: \"=r\"(r), \"=m\"(*s)",
    "				: \"r\"(s));", "",
    "			return r;", "		}",
    "	#elif defined(ia64) || defined(__ia64__)",
    "		/* Intel Itanium */", "		int",
    "		tas(volatile int *s)	/* tested */",
    "		{	long int r;", "			__asm__ __volatile__(",
    "				\"	xchg4 	%%0=%%1,%%2	\\n\"",
    "		:		\"=r\"(r), \"+m\"(*s)",
    "		:		\"r\"(1)",
    "		:		\"memory\");",
    "			return (int) r;", "		}", "	#else",
    "		#error missing definition of test and set operation for this "
    "platform",
    "	#endif", "", "void", "cleanup_shm(int val)",
    "{	volatile sh_Allocater *nxt_pool;", "	unsigned long cnt = 0;",
    "	int m;", "", "	if (nibis != 0)",
    "	{	printf(\"cpu%%d: Redundant call to cleanup_shm(%%d)\\n\", "
    "core_id, val);",
    "		return;", "	} else", "	{	nibis = 1;", "	}",
    "	if (search_terminated != NULL)",
    "	{	*search_terminated |= 16; /* cleanup_shm */", "	}", "",
    "	for (m = 0; m < NR_QS; m++)",
    "	{	if (shmdt((void *) shared_mem[m]) > 0)",
    "		{	perror(\"shmdt detaching from shared queues\");",
    "	}	}", "", "#ifdef SEP_STATE",
    "	if (shmdt((void *) shmid_X) != 0)",
    "	{	perror(\"shmdt detaching from shared state memory\");",
    "	}", "#else", "	#ifdef BITSTATE",
    "		if (SS > 0 && shmdt((void *) SS) != 0)",
    "		{	if (verbose)", "			{	"
                                       "perror(\"shmdt detaching from shared "
                                       "bitstate arena\");",
    "		}	}", "	#else", "		if (core_id == 0)",
    "		{	/* before detaching: */", "			for "
                                                  "(nxt_pool = dc_shared; "
                                                  "nxt_pool != NULL; nxt_pool "
                                                  "= nxt_pool->nxt)",
    "			{	cnt += nxt_pool->dc_size;",
    "			}", "			if (verbose)",
    "			{	printf(\"cpu0: done, %%ld Mb of shared state "
    "memory left\\n\",",
    "					cnt / (long)(1048576));",
    "		}	}", "",
    "		if (shmdt((void *) H_tab) != 0)",
    "		{	perror(\"shmdt detaching from shared hashtable\");",
    "		}", "", "		for (last_pool = first_pool; last_pool "
                        "!= NULL; last_pool = nxt_pool)",
    "		{	nxt_pool = last_pool->nxt;",
    "			if (shmdt((void *) last_pool->dc_start) != 0)",
    "			{	perror(\"shmdt detaching from shared state "
    "memory\");",
    "		}	}",
    "		first_pool = last_pool = NULL;	/* precaution */", "	#endif",
    "#endif", "	/* detached from shared memory - so cannot use cpu_printf */",
    "	if (verbose)",
    "	{	printf(\"cpu%%d: done -- got %%ld states from queue\\n\",",
    "			core_id, nstates_get);", "	}", "}", "",
    "extern void give_up(int);", "extern void Read_Queue(int);", "", "void",
    "mem_get(void)", "{	SM_frame   *f;", "	int is_parent;", "",
    "#if defined(MA) && !defined(SEP_STATE)",
    "	#error MA without SEP_STATE is not supported with multi-core", "#endif",
    "#ifdef BFS", "	#error BFS is not supported with multi-core", "#endif",
    "#ifdef SC", "	#error SC is not supported with multi-core", "#endif",
    "	init_shm();	/* we are single threaded when this starts */", "",
    "	if (core_id == 0 && verbose)",
    "	{	printf(\"cpu0: step 4: calling fork()\\n\");", "	}",
    "	fflush(stdout);", "",
    "/*	if NCORE > 1 the child or the parent should fork N-1 more times",
    " *	the parent is the only process with core_id == 0 and is_parent > 0",
    " *	the workers have is_parent = 0 and core_id = 1..NCORE-1", " */",
    "	if (core_id == 0)",
    "	{	worker_pids[0] = getpid();	/* for completeness */",
    "		while (++core_id < NCORE)	/* first worker sees core_id = "
    "1 */",
    "		{	is_parent = fork();",
    "			if (is_parent == -1)",
    "			{	Uerror(\"fork failed\");",
    "			}",
    "			if (is_parent == 0)	/* this is a worker process */",
    "			{	if (proxy_pid == core_id)	/* always "
    "non-zero */",
    "				{	start_proxy(\"-r\", 0);	/* no return "
    "*/",
    "				}",
    "				goto adapt;	/* root process continues "
    "spawning */",
    "			}",
    "			worker_pids[core_id] = is_parent;", "		}",
    "		/* note that core_id is now NCORE */",
    "		if (proxy_pid > 0 && proxy_pid < NCORE)", /* add send-half of
                                                             proxy */
    "		{	proxy_pid_snd = fork();",
    "			if (proxy_pid_snd == -1)",
    "			{	Uerror(\"proxy fork failed\");",
    "			}", "			if (proxy_pid_snd == 0)",
    "			{	start_proxy(\"-s\", worker_pids[proxy_pid]); /* "
    "no return */",
    "		}	} /* else continue */",

    "		if (is_parent > 0)",
    "		{	core_id = 0;	/* reset core_id for root process */",
    "		}", "	} else	/* worker */",
    "	{	static char db0[16];	/* good for up to 10^6 cores */",
    "		static char db1[16];",
    "adapt:		tprefix = db0; sprefix = db1;",
    "		sprintf(tprefix, \"cpu%%d_trail\", core_id);",
    "		sprintf(sprefix, \"cpu%%d_rst\", core_id);",
    "		memcnt = 0;	/* count only additionally allocated memory */",
    "	}", "	signal(SIGINT, give_up);", "",
    "	if (proxy_pid == 0)		/* not in a cluster setup, pan_proxy "
    "must attach */",
    "	{	rm_shared_segments();	/* mark all shared segments for removal "
    "on exit */",
    "	}", /* doing it early means less chance of being unable to do this */
    "	if (verbose)", "	{	cpu_printf(\"starting core_id %%d -- "
                       "pid %%d\\n\", core_id, getpid());",
    "	}",

    "#if defined(SEP_HEAP) && !defined(SEP_STATE)", /* set my_heap and adjust
                                                       dc_shared */
    "	{	int i;",
    "		volatile sh_Allocater *ptr;", "		ptr = first_pool;",
    "		for (i = 0; i < NCORE  && ptr != NULL; i++)",
    "		{	if (i == core_id)",
    "			{	my_heap = (char *) ptr->dc_arena;",
    "				my_size = (long) ptr->dc_size;",
    "				if (verbose)",
    "				cpu_printf(\"local heap %%ld MB\\n\", "
    "my_size/(1048576));",
    "				break;", "			}",
    "			ptr = ptr->nxt; /* local */", "		}",
    "		if (my_heap == NULL)",
    "		{	printf(\"cpu%%d: no local heap\\n\", core_id);",
    "			pan_exit(1);", "		} /* else */",
    "	#if defined(CYGWIN) || defined(__CYGWIN__)",
    "		ptr = first_pool;",
    "		for (i = 0; i < NCORE  && ptr != NULL; i++)",
    "		{	ptr = ptr->nxt; /* local */", "		}",
    "		dc_shared = ptr; /* any remainder */", "	#else",
    "		dc_shared = NULL; /* used all mem for local heaps */",
    "	#endif", "	}", "#endif",

    "	if (core_id == 0 && !remote_party)",
    "	{	new_state();		/* cpu0 explores root */",
    "		if (verbose)",
    "		cpu_printf(\"done with 1st dfs, nstates %%g (put %%d states), "
    "read q\\n\",",
    "			nstates, nstates_put);",
    "		dfs_phase2 = 1;", "	}",
    "	Read_Queue(core_id);	/* all cores */", "", "	if (verbose)",
    "	{	cpu_printf(\"put %%6d states into queue -- got %%6d\\n\",",
    "			nstates_put, nstates_get);", "	}",
    "	if (proxy_pid != 0)", "	{	rm_shared_segments();", "	}",
    "	done = 1;", "	wrapup();", "	exit(0);", "}", "", "#else",
    "int unpack_state(SM_frame *, int);", "#endif", "", "struct H_el *",
    "grab_shared(int n)", "{", "#ifndef SEP_STATE",
    "	char *rval = (char *) 0;", "", "	if (n == 0)",
    "	{	printf(\"cpu%%d: grab shared zero\\n\", core_id); "
    "fflush(stdout);",
    "		return (struct H_el *) rval;",
    "	} else if (n&(sizeof(void *)-1))",
    "	{	n += sizeof(void *)-(n&(sizeof(void *)-1)); /* alignment */",
    "	}", "", "#ifdef SEP_HEAP", "	/* no locking */",
    "	if (my_heap != NULL && my_size > n)", "	{	rval = my_heap;",
    "		my_heap += n;", "		my_size -= n;",
    "		goto done;", "	}", "#endif", "", "	if (!dc_shared)",
    "	{	sudden_stop(\"pan: out of memory\");", "	}", "",
    "	/* another lock is always already in effect when this is called */",
    "	/* but not always the same lock -- i.e., on different parts of the "
    "hashtable */",
    "	enter_critical(GLOBAL_LOCK);	/* this must be independently mutex */",
    "#if defined(SEP_HEAP) && !defined(WIN32) && !defined(WIN64)",
    "	{	static int noted = 0;", "		if (!noted)",
    "		{	noted = 1;",
    "			printf(\"cpu%%d: global heap has %%ld bytes left, "
    "needed %%d\\n\",",
    "				core_id, dc_shared?dc_shared->dc_size:0, n);",
    "	}	}", "#endif", "#if 0", /* for debugging */
    "		if (dc_shared->pattern != 1234567)",
    "		{	leave_critical(GLOBAL_LOCK);",
    "			Uerror(\"overrun -- memory corruption\");",
    "		}", "#endif", "		if (dc_shared->dc_size < n)",
    "		{	if (verbose)",
    "			{ printf(\"Next Pool %%g Mb + %%d\\n\", "
    "memcnt/(1048576.), n);",
    "			}", "			if (dc_shared->nxt == NULL",
    "			||  dc_shared->nxt->dc_arena == NULL",
    "			||  dc_shared->nxt->dc_size < n)",
    "			{	printf(\"cpu%%d: memcnt %%g Mb + wanted %%d "
    "bytes more\\n\",",
    "					core_id, memcnt / (1048576.), n);",
    "				leave_critical(GLOBAL_LOCK);",
    "				sudden_stop(\"out of memory -- aborting\");",
    "				wrapup();	/* exits */",
    "			} else",
    "			{	dc_shared = (sh_Allocater *) dc_shared->nxt;",
    "		}	}", "",
    "		rval = (char *) dc_shared->dc_arena;",
    "		dc_shared->dc_arena += n;",
    "		dc_shared->dc_size  -= (long) n;", "#if 0",
    "		if (VVERBOSE)",
    "		printf(\"cpu%%d grab shared (%%d bytes) -- %%ld left\\n\",",
    "			core_id, n, dc_shared->dc_size);", "#endif",
    "	leave_critical(GLOBAL_LOCK);", "done:", "	memset(rval, 0, n);",
    "	memcnt += (double) n;", "", "	return (struct H_el *) rval;", "#else",
    "	return (struct H_el *) emalloc(n);", "#endif", "}", "", "SM_frame *",
    "Get_Full_Frame(int n)", "{	SM_frame *f;",
    "	double cnt_start = frame_wait;", "", "	f = &m_workq[n][prfull[n]];",
    "	while (f->m_vsize == 0)	/* await full slot LOCK : full frame */",
    "	{	iam_alive();", "#ifndef NGQ", "	#ifndef SAFETY",
    "		if (!a_cycles || core_id != 0)", "	#endif",
    "		if (*grcnt > 0)	/* accessed outside lock, but safe even if "
    "wrong */",
    "		{	enter_critical(GQ_RD);	/* gq - read access */",
    "			if (*grcnt > 0)		/* could have changed */",
    "			{	f = &m_workq[NCORE][*grfull];	/* global q */",
    "				if (f->m_vsize == 0)",
    "				{	/* writer is still filling the slot */",
    "					*gr_writemiss++;",
    "					f = &m_workq[n][prfull[n]]; /* reset "
    "*/",
    "				} else",
    "				{	*grfull = (*grfull+1) %% (GN_FRAMES);",
    "						enter_critical(GQ_WR);",
    "						*grcnt = *grcnt - 1;",
    "						leave_critical(GQ_WR);",
    "					leave_critical(GQ_RD);",
    "					return f;",
    "			}	}", "			leave_critical(GQ_RD);",
    "		}", "#endif", "		if (frame_wait++ - cnt_start > Delay)",
    "		{	if (0)", /* too frequent to enable this one */
    "			{	cpu_printf(\"timeout on q%%d -- %%u -- query "
    "%%d\\n\",",
    "					n, f, query_in_progress);",
    "			}",
    "			return (SM_frame *) 0;	/* timeout */",
    "	}	}", "	iam_alive();",
    "	if (VVERBOSE) cpu_printf(\"got frame from q%%d\\n\", n);",
    "	prfull[n] = (prfull[n] + 1) %% (LN_FRAMES);",
    "	enter_critical(QLOCK(n));",
    "		prcnt[n]--; /* lock out increments */",
    "	leave_critical(QLOCK(n));", "	return f;", "}", "", "SM_frame *",
    "Get_Free_Frame(int n)", "{	SM_frame *f;",
    "	double cnt_start = free_wait;", "",
    "	if (VVERBOSE) { cpu_printf(\"get free frame from q%%d\\n\", n); }", "",
    "	if (n == NCORE)	/* global q */",
    "	{	f = &(m_workq[n][lrfree]);", "	} else",
    "	{	f = &(m_workq[n][prfree[n]]);", "	}",
    "	while (f->m_vsize != 0)	/* await free slot LOCK : free slot */",
    "	{	iam_alive();",
    "		if (free_wait++ - cnt_start > OneSecond)",
    "		{	if (verbose)", "			{	"
                                       "cpu_printf(\"timeout waiting for free "
                                       "slot q%%d\\n\", n);",
    "			}", "			cnt_start = free_wait;",
    "			if (someone_crashed(1))",
    "			{	printf(\"cpu%%d: search terminated\\n\", "
    "core_id);",
    "				sudden_stop(\"get free frame\");",
    "				pan_exit(1);", "	}	}	}",
    "	if (n != NCORE)",
    "	{	prfree[n] = (prfree[n] + 1) %% (LN_FRAMES);",
    "		enter_critical(QLOCK(n));",
    "			prcnt[n]++; /* lock out decrements */",
    "			if (prmax[n] < prcnt[n])",
    "			{	prmax[n] = prcnt[n];",
    "			}", "		leave_critical(QLOCK(n));", "	}",
    "	return f;", "}", "", "#ifndef NGQ", "int", "GlobalQ_HasRoom(void)",
    "{	int rval = 0;", "", "	gq_tries++;",
    "	if (*grcnt < GN_FRAMES) /* there seems to be room */",
    "	{	enter_critical(GQ_WR);	/* gq write access */",
    "		if (*grcnt < GN_FRAMES)",
    "		{	if (m_workq[NCORE][*grfree].m_vsize != 0)",
    "			{	/* can happen if reader is slow emptying slot "
    "*/",
    "				*gr_readmiss++;", "				"
                                                  "goto out; /* dont wait: "
                                                  "release lock and return */",
    "			}", "			lrfree = *grfree;	/* "
                            "Get_Free_Frame use lrfree in this mode */",
    "			*grfree = (*grfree + 1) %% GN_FRAMES;", /* next process
                                                                   looks at next
                                                                   slot */
    "			*grcnt = *grcnt + 1;	/* count nr of slots filled -- "
    "no additional lock needed */",
    "			if (*grmax < *grcnt) *grmax = *grcnt;",
    "			leave_critical(GQ_WR);	/* for short lock duration */",
    "			gq_hasroom++;",
    "			mem_put(NCORE);		/* copy state into reserved "
    "slot */",
    "			rval = 1;		/* successfull handoff */",
    "		} else", "		{	gq_hasnoroom++;",
    "out:			leave_critical(GQ_WR);", /* should be rare */
    "	}	}", "	return rval;", "}", "#endif", "", "int",
    "unpack_state(SM_frame *f, int from_q)", "{	int i, j;",
    "	static struct H_el D_State;", "", "	if (f->m_vsize > 0)",
    "	{	boq   = f->m_boq;", "		if (boq > 256)",
    "		{	cpu_printf(\"saw control %%d, expected state\\n\", "
    "boq);",
    "			return 0;", "		}",
    "		vsize = f->m_vsize;", "correct:",
    "		memcpy((uchar *) &now, (uchar *) f->m_now, vsize);",
    "		for (i = j = 0; i < VMAX; i++, j = (j+1)%%8)",
    "		{	Mask[i] = (f->m_Mask[i/8] & (1<<j)) ? 1 : 0;",
    "		}", "		if (now._nr_pr > 0)",
    "		{	memcpy((uchar *) proc_offset, (uchar *) f->m_p_offset, "
    "now._nr_pr * sizeof(OFFT));",
    "			memcpy((uchar *) proc_skip,   (uchar *) f->m_p_skip,   "
    "now._nr_pr * sizeof(uchar));",
    "		}", "		if (now._nr_qs > 0)",
    "		{	memcpy((uchar *) q_offset,    (uchar *) f->m_q_offset, "
    "now._nr_qs * sizeof(OFFT));",
    "			memcpy((uchar *) q_skip,      (uchar *) f->m_q_skip,   "
    "now._nr_qs * sizeof(uchar));",
    "		}", "#ifndef NOVSZ", "		if (vsize != now._vsz)",
    "		{	cpu_printf(\"vsize %%d != now._vsz %%d (type %%d) "
    "%%d\\n\",",
    "				vsize, now._vsz, f->m_boq, f->m_vsize);",
    "			vsize = now._vsz;",
    "			goto correct;	/* rare event: a race */",
    "		}", "#endif", "		hmax = max(hmax, vsize);", "",
    "		if (f != &cur_Root)", "		{	memcpy((uchar "
                                      "*) &cur_Root, (uchar *) f, "
                                      "sizeof(SM_frame));",
    "		}", "", "		if (((now._a_t) & 1) == 1)	/* "
                        "i.e., when starting nested DFS */",
    "		{	A_depth = depthfound = 0;",
    "			memcpy((uchar *)&A_Root, (uchar *)&now, vsize);",
    "		}", "		nr_handoffs = f->nr_handoffs;", "	} else",
    "	{	cpu_printf(\"pan: state empty\\n\");", "	}", "",
    "	depth = 0;", "	trpt = &trail[1];", "	trpt->tau    = f->m_tau;",
    "	trpt->o_pm   = f->m_o_pm;", "",
    "	(trpt-1)->ostate = &D_State; /* stub */",
    "	trpt->ostate = &D_State;", "", "#ifdef FULL_TRAIL", "	if (upto > 0)",
    "	{	stack_last[core_id] = (Stack_Tree *) f->m_stack;", "	}",
    "	#if defined(VERBOSE)", "	if (stack_last[core_id])",
    "	{	cpu_printf(\"%%d: UNPACK -- SET m_stack %%u (%%d,%%d)\\n\",",
    "			depth, stack_last[core_id], stack_last[core_id]->pr,",
    "			stack_last[core_id]->t_id);", "	}", "	#endif",
    "#endif", "", "	if (!trpt->o_t)", "	{	static Trans D_Trans;",
    "		trpt->o_t = &D_Trans;", "	}", "", "	#ifdef VERI",
    "	if ((trpt->tau & 4) != 4)",
    "	{	trpt->tau |= 4;	/* the claim moves first */",
    "		cpu_printf(\"warning: trpt was not up to date\\n\");",
    "	}", "	#endif", "", "	for (i = 0; i < (int) now._nr_pr; i++)",
    "	{	P0 *ptr = (P0 *) pptr(i);", "	#ifndef NP",
    "		if (accpstate[ptr->_t][ptr->_p])",
    "		{	trpt->o_pm |= 2;", "		}", "	#else",
    "		if (progstate[ptr->_t][ptr->_p])",
    "		{	trpt->o_pm |= 4;", "		}", "	#endif",
    "	}", "", "	#ifdef EVENT_TRACE", "		#ifndef NP",
    "			if (accpstate[EVENT_TRACE][now._event])",
    "			{	trpt->o_pm |= 2;", "			}",
    "		#else",
    "			if (progstate[EVENT_TRACE][now._event])",
    "			{	trpt->o_pm |= 4;", "			}",
    "		#endif", "	#endif", "",
    "	#if defined(C_States) && (HAS_TRACK==1)",
    "		/* restore state of tracked C objects */",
    "		c_revert((uchar *) &(now.c_state[0]));",
    "		#if (HAS_STACK==1)", "		c_unstack((uchar *) "
                                     "f->m_c_stack); /* unmatched tracked data "
                                     "*/",
    "		#endif", "	#endif", "	return 1;", "}", "", "void",
    "write_root(void)	/* for trail file */", "{	int fd;", "",
    "	if (iterative == 0 && Nr_Trails > 1)",
    "		sprintf(fnm, \"%%s%%d.%%s\", TrailFile, Nr_Trails-1, sprefix);",
    "	else", "		sprintf(fnm, \"%%s.%%s\", TrailFile, sprefix);",
    "", "	if (cur_Root.m_vsize == 0)",
    "	{	(void) unlink(fnm); /* remove possible old copy */",
    "		return;	/* its the default initial state */", "	}", "",
    "	if ((fd = creat(fnm, TMODE)) < 0)", "	{	char *q;",
    "		if ((q = strchr(TrailFile, \'.\')))",
    "		{	*q = \'\\0\';		/* strip .pml */",
    "			if (iterative == 0 && Nr_Trails-1 > 0)",
    "				sprintf(fnm, \"%%s%%d.%%s\", TrailFile, "
    "Nr_Trails-1, sprefix);",
    "			else",
    "				sprintf(fnm, \"%%s.%%s\", TrailFile, sprefix);",
    "			*q = \'.\';",
    "			fd = creat(fnm, TMODE);", "		}",
    "		if (fd < 0)",
    "		{	cpu_printf(\"pan: cannot create %%s\\n\", fnm);",
    "			perror(\"cause\");", "			return;",
    "	}	}", "",
    "	if (write(fd, &cur_Root, sizeof(SM_frame)) != sizeof(SM_frame))",
    "	{	cpu_printf(\"pan: error writing %%s\\n\", fnm);", "	} else",
    "	{	cpu_printf(\"pan: wrote %%s\\n\", fnm);", "	}",
    "	close(fd);", "}", "", "void", "set_root(void)", "{	int fd;",
    "	char *q;",
    "	char MyFile[512];", /* enough to hold a reasonable pathname */
    "	char MySuffix[16];", "	char *ssuffix = \"rst\";",
    "	int  try_core = 1;", "", "	strcpy(MyFile, TrailFile);",
    "try_again:", "	if (whichtrail > 0)",
    "	{	sprintf(fnm, \"%%s%%d.%%s\", MyFile, whichtrail, ssuffix);",
    "		fd = open(fnm, O_RDONLY, 0);",
    "		if (fd < 0 && (q = strchr(MyFile, \'.\')))",
    "		{	*q = \'\\0\';	/* strip .pml */",
    "			sprintf(fnm, \"%%s%%d.%%s\", MyFile, whichtrail, "
    "ssuffix);",
    "			*q = \'.\';",
    "			fd = open(fnm, O_RDONLY, 0);", "		}",
    "	} else", "	{	sprintf(fnm, \"%%s.%%s\", MyFile, ssuffix);",
    "		fd = open(fnm, O_RDONLY, 0);",
    "		if (fd < 0 && (q = strchr(MyFile, \'.\')))",
    "		{	*q = \'\\0\';	/* strip .pml */",
    "			sprintf(fnm, \"%%s.%%s\", MyFile, ssuffix);",
    "			*q = \'.\';",
    "			fd = open(fnm, O_RDONLY, 0);", "	}	}", "",
    "	if (fd < 0)", "	{	if (try_core < NCORE)",
    "		{	ssuffix = MySuffix;",
    "			sprintf(ssuffix, \"cpu%%d_rst\", try_core++);",
    "			goto try_again;", "		}",
    "		cpu_printf(\"no file '%%s.rst' or '%%s' (not an error)\\n\", "
    "MyFile, fnm);",
    "	} else",
    "	{	if (read(fd, &cur_Root, sizeof(SM_frame)) != sizeof(SM_frame))",
    "		{	cpu_printf(\"read error %%s\\n\", fnm);",
    "			close(fd);", "			pan_exit(1);",
    "		}", "		close(fd);",
    "		(void) unpack_state(&cur_Root, -2);", "#ifdef SEP_STATE",
    "		cpu_printf(\"partial trail -- last few steps only\\n\");",
    "#endif", "		cpu_printf(\"restored root from '%%s'\\n\", fnm);",
    "		printf(\"=====State:=====\\n\");",
    "		{	int i, j; P0 *z;",
    "			for (i = 0; i < now._nr_pr; i++)",
    "			{	z = (P0 *)pptr(i);",
    "				printf(\"proc %%2d (%%s) \", i, "
    "procname[z->_t]);",

    "				for (j = 0; src_all[j].src; j++)",
    "				if (src_all[j].tp == (int) z->_t)",
    "				{	printf(\" %%s:%%d \",",
    "						PanSource, "
    "src_all[j].src[z->_p]);",
    "					break;",
    "				}",
    "				printf(\"(state %%d)\\n\", z->_p);",
    "				c_locals(i, z->_t);", "			}",
    "			c_globals();", "		}",
    "		printf(\"================\\n\");", "	}", "}", "",
    "#ifdef USE_DISK", "unsigned long dsk_written, dsk_drained;",
    "void mem_drain(void);", "#endif", "", "void",
    "m_clear_frame(SM_frame *f)", /* clear room for stats */
    "{	int i, clr_sz = sizeof(SM_results);", "",
    "	for (i = 0; i <= _NP_; i++)	/* all proctypes */",
    "	{	clr_sz += NrStates[i]*sizeof(uchar);", "	}",
    "	memset(f, 0, clr_sz);",
    "	/* caution if sizeof(SM_results) > sizeof(SM_frame) */", "}", "",
    "#define TargetQ_Full(n)	(m_workq[n][prfree[n]].m_vsize != 0)", /* no
                                                                          free
                                                                          slot
                                                                          */
    "#define TargetQ_NotFull(n)	(m_workq[n][prfree[n]].m_vsize == 0)", /* avoiding
                                                                          prcnt
                                                                          */
    "",
    "int", "AllQueuesEmpty(void)", "{	int q;", "#ifndef NGQ",
    "	if (*grcnt != 0)", "	{	return 0;", "	}", "#endif",
    "	for (q = 0; q < NCORE; q++)",
    "	{	if (prcnt[q] != 0)", /* not locked, ok if race */
    "		{	return 0;", "	}	}", "	return 1;", "}", "",
    "void", "Read_Queue(int q)", "{	SM_frame   *f, *of;",
    "	int	remember, target_q;", "	SM_results *r;",
    "	double patience = 0.0;", "", "	target_q = (q + 1) %% NCORE;", "",
    "	for (;;)", "	{	f = Get_Full_Frame(q);",
    "		if (!f)	/* 1 second timeout -- and trigger for Query */",
    "		{	if (someone_crashed(2))",
    "			{	printf(\"cpu%%d: search terminated [code "
    "%%d]\\n\",",
    "					core_id, "
    "search_terminated?*search_terminated:-1);",
    "				sudden_stop(\"\");",
    "				pan_exit(1);", "			}",
    "#ifdef TESTING", "	/* to profile with cc -pg and gprof pan.exe -- "
                      "set handoff depth beyond maxdepth */",
    "			exit(0);", "#endif",
    "			remember = *grfree;",
    "			if (core_id == 0		/* root can initiate "
    "termination */",
    "			&& remote_party == 0		/* and only the "
    "original root */",
    "			&& query_in_progress == 0	/* unless its already "
    "in progress */",
    "			&& AllQueuesEmpty())",
    "			{	f = Get_Free_Frame(target_q);",
    "				query_in_progress = 1;	/* only root process "
    "can do this */",
    "				if (!f) { Uerror(\"Fatal1: no free slot\"); }",
    "				f->m_boq = QUERY;	/* initiate Query */",
    "				if (verbose)",
    "				{  cpu_printf(\"snd QUERY to q%%d (%%d) into "
    "slot %%d\\n\",",
    "					target_q, nstates_get + 1, "
    "prfree[target_q]-1);",
    "				}",
    "				f->m_vsize = remember + 1;",
    "				/* number will not change unless we receive "
    "more states */",
    "			} else if (patience++ > OneHour) /* one hour watchdog "
    "timer */",
    "			{	cpu_printf(\"timeout -- giving up\\n\");",
    "				sudden_stop(\"queue timeout\");",
    "				pan_exit(1);", "			}",
    "			if (0) cpu_printf(\"timed out -- try again\\n\");",
    "			continue;	", "		}",
    "		patience = 0.0; /* reset watchdog */", "",
    "		if (f->m_boq == QUERY)", "		{	if (verbose)",
    "			{	cpu_printf(\"got QUERY on q%%d (%%d <> %%d) "
    "from slot %%d\\n\",",
    "					q, f->m_vsize, nstates_put + 1, "
    "prfull[q]-1);",
    "				snapshot();", "			}",
    "			remember = f->m_vsize;",
    "			f->m_vsize = 0;	/* release slot */", "",
    "			if (core_id == 0 && remote_party == 0)	/* original "
    "root cpu0 */",
    "			{	if (query_in_progress == 1	/* didn't send "
    "more states in the interim */",
    "				&&  *grfree + 1 == remember)	/* no action on "
    "global queue meanwhile */",
    "				{	if (verbose) cpu_printf(\"Termination "
    "detected\\n\");",
    "					if (TargetQ_Full(target_q))",
    "					{	if (verbose)",
    "						cpu_printf(\"warning: target q "
    "is full\\n\");",
    "					}",
    "					f = Get_Free_Frame(target_q);",
    "					if (!f) { Uerror(\"Fatal2: no free "
    "slot\"); }",
    "					m_clear_frame(f);",
    "					f->m_boq = QUIT; /* send final Quit, "
    "collect stats */",
    "					f->m_vsize = 111; /* anything non-zero "
    "will do */",
    "					if (verbose)",
    "					cpu_printf(\"put QUIT on q%%d\\n\", "
    "target_q);",
    "				} else",
    "				{	if (verbose) cpu_printf(\"Stale "
    "Query\\n\");",
    "#ifdef USE_DISK", "					mem_drain();",
    "#endif", "				}",
    "				query_in_progress = 0;",
    "			} else",
    "			{	if (TargetQ_Full(target_q))",
    "				{	if (verbose)",
    "					cpu_printf(\"warning: forward query - "
    "target q full\\n\");",
    "				}",
    "				f = Get_Free_Frame(target_q);",
    "				if (verbose)",
    "				cpu_printf(\"snd QUERY response to q%%d (%%d <> "
    "%%d) in slot %%d\\n\",",
    "					target_q, remember, *grfree + 1, "
    "prfree[target_q]-1);",
    "				if (!f) { Uerror(\"Fatal4: no free slot\"); }",
    "", "				if (*grfree + 1 == remember)	/* no "
        "action on global queue */",
    "				{	f->m_boq = QUERY;	/* forward "
    "query, to root */",
    "					f->m_vsize = remember;",
    "				} else",
    "				{	f->m_boq = QUERY_F;	/* no match -- "
    "busy */",
    "					f->m_vsize = 112;	/* anything "
    "non-zero */",
    "#ifdef USE_DISK",
    "					if (dsk_written != dsk_drained)",
    "					{	mem_drain();",
    "					}", "#endif",
    "			}	}", "			continue;",
    "		}", "", "		if (f->m_boq == QUERY_F)",
    "		{	if (verbose)", "			{	"
                                       "cpu_printf(\"got QUERY_F on q%%d from "
                                       "slot %%d\\n\", q, prfull[q]-1);",
    "			}",
    "			f->m_vsize = 0;	/* release slot */", "",
    "			if (core_id == 0 && remote_party == 0)		/* "
    "original root cpu0 */",
    "			{	if (verbose) cpu_printf(\"No Match on "
    "Query\\n\");",
    "				query_in_progress = 0;",
    "			} else",
    "			{	if (TargetQ_Full(target_q))",
    "				{	if (verbose) cpu_printf(\"warning: "
    "forwarding query_f, target queue full\\n\");",
    "				}",
    "				f = Get_Free_Frame(target_q);",
    "				if (verbose) cpu_printf(\"forward QUERY_F to "
    "q%%d into slot %%d\\n\",",
    "						target_q, prfree[target_q]-1);",
    "				if (!f) { Uerror(\"Fatal5: no free slot\"); }",
    "				f->m_boq = QUERY_F;		/* cannot "
    "terminate yet */",
    "				f->m_vsize = 113;		/* anything "
    "non-zero */",
    "			}", "#ifdef USE_DISK",
    "			if (dsk_written != dsk_drained)",
    "			{	mem_drain();", "			}",
    "#endif", "			continue;", "		}", "",
    "		if (f->m_boq == QUIT)", "		{	if (0) "
                                        "cpu_printf(\"done -- local memcnt %%g "
                                        "Mb\\n\", memcnt/(1048576.));",
    "			retrieve_info((SM_results *) f); /* collect and combine "
    "stats */",
    "			if (verbose)",
    "			{	cpu_printf(\"received Quit\\n\");",
    "				snapshot();", "			}",
    "			f->m_vsize = 0;	/* release incoming slot */",
    "			if (core_id != 0)", "			{	f = "
                                            "Get_Free_Frame(target_q); /* new "
                                            "outgoing slot */",
    "				if (!f) { Uerror(\"Fatal6: no free slot\"); }",
    "				m_clear_frame(f);	/* start with zeroed "
    "stats */",
    "				record_info((SM_results *) f);",
    "				f->m_boq = QUIT;	/* forward combined "
    "results */",
    "				f->m_vsize = 114;	/* anything non-zero "
    "*/",
    "				if (verbose>1)",
    "				cpu_printf(\"fwd Results to q%%d\\n\", "
    "target_q);",
    "			}",
    "			break;			/* successful termination */",
    "		}", "",
    "		/* else: 0<= boq <= 255, means STATE transfer */",
    "		if (unpack_state(f, q) != 0)",
    "		{	nstates_get++;",
    "			f->m_vsize = 0;	/* release slot */",
    "			if (VVERBOSE) cpu_printf(\"Got state\\n\");", "",
    "			if (search_terminated != NULL",
    "			&&  *search_terminated == 0)",
    "			{	new_state();	/* explore successors */",
    "				memset((uchar *) &cur_Root, 0, "
    "sizeof(SM_frame));	/* avoid confusion */",
    "			} else", "			{	pan_exit(0);",
    "			}", "		} else",
    "		{	pan_exit(0);", "	}	}",
    "	if (verbose) cpu_printf(\"done got %%d put %%d\\n\", nstates_get, "
    "nstates_put);",
    "	sleep_report();", "}", "", "void", "give_up(int unused_x)", "{",
    "	if (search_terminated != NULL)",
    "	{	*search_terminated |= 32;	/* give_up */", "	}",
    "	if (!writing_trail)", "	{	was_interrupted = 1;",
    "		snapshot();", "		cpu_printf(\"Give Up\\n\");",
    "		sleep_report();", "		pan_exit(1);",
    "	} else /* we are already terminating */",
    "	{	cpu_printf(\"SIGINT\\n\");", "	}", "}", "", "void",
    "check_overkill(void)", "{", "	vmax_seen = (vmax_seen + 7)/ 8;",
    "	vmax_seen *= 8;	/* round up to a multiple of 8 */", "",
    "	if (core_id == 0", "	&&  !remote_party", "	&&  nstates_put > 0",
    "	&&  VMAX - vmax_seen > 8)", "	{", "#ifdef BITSTATE",
    "		printf(\"cpu0: max VMAX value seen in this run: \");", "#else",
    "		printf(\"cpu0: recommend recompiling with \");", "#endif",
    "		printf(\"-DVMAX=%%d\\n\", vmax_seen);", "	}", "}", "",
    "void", "mem_put(int q)	/* handoff state to other cpu, workq q */",
    "{	SM_frame *f;", "	int i, j;", "", "	if (vsize > VMAX)",
    "	{	vsize = (vsize + 7)/8; vsize *= 8; /* round up */",
    "		printf(\"pan: recompile with -DVMAX=N with N >= %%d\\n\", (int) "
    "vsize);",
    "		Uerror(\"aborting\");", "	}", "	if (now._nr_pr > PMAX)",
    "	{	printf(\"pan: recompile with -DPMAX=N with N >= %%d\\n\", "
    "now._nr_pr);",
    "		Uerror(\"aborting\");", "	}", "	if (now._nr_qs > QMAX)",
    "	{	printf(\"pan: recompile with -DQMAX=N with N >= %%d\\n\", "
    "now._nr_qs);",
    "		Uerror(\"aborting\");", "	}",
    "	if (vsize > vmax_seen) vmax_seen = vsize;",
    "	if (now._nr_pr > pmax_seen) pmax_seen = now._nr_pr;",
    "	if (now._nr_qs > qmax_seen) qmax_seen = now._nr_qs;", "",
    "	f = Get_Free_Frame(q);	/* not called in likely deadlock states */",
    "	if (!f) { Uerror(\"Fatal3: no free slot\"); }", "",
    "	if (VVERBOSE) cpu_printf(\"putting state into q%%d\\n\", q);", "",
    "	memcpy((uchar *) f->m_now,  (uchar *) &now, vsize);",
    "	memset((uchar *) f->m_Mask, 0, (VMAX+7)/8 * sizeof(char));",
    "	for (i = j = 0; i < VMAX; i++, j = (j+1)%%8)",
    "	{	if (Mask[i])",
    "		{	f->m_Mask[i/8] |= (1<<j);", "	}	}", "",
    "	if (now._nr_pr > 0)", "	{ memcpy((uchar *) f->m_p_offset, "
                              "(uchar *) proc_offset, now._nr_pr * "
                              "sizeof(OFFT));",
    "	  memcpy((uchar *) f->m_p_skip,   (uchar *) proc_skip,   now._nr_pr * "
    "sizeof(uchar));",
    "	}", "	if (now._nr_qs > 0)", "	{ memcpy((uchar *) "
                                      "f->m_q_offset, (uchar *) q_offset, "
                                      "now._nr_qs * sizeof(OFFT));",
    "	  memcpy((uchar *) f->m_q_skip,   (uchar *) q_skip,   now._nr_qs * "
    "sizeof(uchar));",
    "	}", "#if defined(C_States) && (HAS_TRACK==1) && (HAS_STACK==1)",
    "	c_stack((uchar *) f->m_c_stack); /* save unmatched tracked data */",
    "#endif", "#ifdef FULL_TRAIL", "	f->m_stack = stack_last[core_id];",
    "#endif", "	f->nr_handoffs = nr_handoffs+1;",
    "	f->m_tau    = trpt->tau;", "	f->m_o_pm   = trpt->o_pm;",
    "	f->m_boq    = boq;",
    "	f->m_vsize  = vsize;	/* must come last - now the other cpu can see "
    "it */",
    "", "	if (query_in_progress == 1)",
    "		query_in_progress = 2;	/* make sure we know, if a query makes "
    "the rounds */",
    "	nstates_put++;", "}", "", "#ifdef USE_DISK", "int Dsk_W_Nr, Dsk_R_Nr;",
    "int dsk_file = -1, dsk_read = -1;",
    "unsigned long dsk_written, dsk_drained;", "char dsk_name[512];", "",
    "#ifndef BFS_DISK", "#if defined(WIN32) || defined(WIN64)",
    "	#define RFLAGS	(O_RDONLY|O_BINARY)",
    "	#define WFLAGS	(O_CREAT|O_WRONLY|O_TRUNC|O_BINARY)", "#else",
    "	#define RFLAGS	(O_RDONLY)",
    "	#define WFLAGS	(O_CREAT|O_WRONLY|O_TRUNC)", "#endif", "#endif", "",
    "void", "dsk_stats(void)", "{	int i;", "", "	if (dsk_written > 0)",
    "	{	cpu_printf(\"dsk_written %%d states in %%d files\\ncpu%%d: "
    "dsk_drained %%6d states\\n\",",
    "			dsk_written, Dsk_W_Nr, core_id, dsk_drained);",
    "		close(dsk_read);", "		close(dsk_file);",
    "		for (i = 0; i < Dsk_W_Nr; i++)",
    "		{	sprintf(dsk_name, \"Q%%.3d_%%.3d.tmp\", i, core_id);",
    "			unlink(dsk_name);", "	}	}", "}", "", "void",
    "mem_drain(void)", "{	SM_frame *f, g;",
    "	int q = (core_id + 1) %% NCORE;	/* target q */", "	int sz;", "",
    "	if (dsk_read < 0", "	||  dsk_written <= dsk_drained)",
    "	{	return;", "	}", "",
    "	while (dsk_written > dsk_drained", "	&& TargetQ_NotFull(q))",
    "	{	f = Get_Free_Frame(q);",
    "		if (!f) { Uerror(\"Fatal: unhandled condition\"); }", "",
    "		if ((dsk_drained+1)%%MAX_DSK_FILE == 0)	/* 100K states max per "
    "file */",
    "		{	(void) close(dsk_read); 	/* close current read "
    "handle */",
    "			sprintf(dsk_name, \"Q%%.3d_%%.3d.tmp\", Dsk_R_Nr++, "
    "core_id);",
    "			(void) unlink(dsk_name);	/* remove current file "
    "*/",
    "			sprintf(dsk_name, \"Q%%.3d_%%.3d.tmp\", Dsk_R_Nr, "
    "core_id);",
    "			cpu_printf(\"reading %%s\\n\", dsk_name);",
    "			dsk_read = open(dsk_name, RFLAGS); /* open next file "
    "*/",
    "			if (dsk_read < 0)",
    "			{	Uerror(\"could not open dsk file\");",
    "		}	}",
    "		if (read(dsk_read, &g, sizeof(SM_frame)) != sizeof(SM_frame))",
    "		{	Uerror(\"bad dsk file read\");", "		}",
    "		sz = g.m_vsize;", "		g.m_vsize = 0;",
    "		memcpy(f, &g, sizeof(SM_frame));",
    "		f->m_vsize = sz;	/* last */", "",
    "		dsk_drained++;", "	}", "}", "", "void", "mem_file(void)",
    "{	SM_frame f;", "	int i, j, q = (core_id + 1) %% NCORE;	/* target q */",
    "", "	if (vsize > VMAX)", "	{	printf(\"pan: recompile with "
                                    "-DVMAX=N with N >= %%d\\n\", vsize);",
    "		Uerror(\"aborting\");", "	}", "	if (now._nr_pr > PMAX)",
    "	{	printf(\"pan: recompile with -DPMAX=N with N >= %%d\\n\", "
    "now._nr_pr);",
    "		Uerror(\"aborting\");", "	}", "	if (now._nr_qs > QMAX)",
    "	{	printf(\"pan: recompile with -DQMAX=N with N >= %%d\\n\", "
    "now._nr_qs);",
    "		Uerror(\"aborting\");", "	}", "",
    "	if (VVERBOSE) cpu_printf(\"filing state for q%%d\\n\", q);", "",
    "	memcpy((uchar *) f.m_now,  (uchar *) &now, vsize);",
    "	memset((uchar *) f.m_Mask, 0, (VMAX+7)/8 * sizeof(char));",
    "	for (i = j = 0; i < VMAX; i++, j = (j+1)%%8)",
    "	{	if (Mask[i])",
    "		{	f.m_Mask[i/8] |= (1<<j);", "	}	}", "",
    "	if (now._nr_pr > 0)", "	{	memcpy((uchar *)f.m_p_offset, "
                              "(uchar *)proc_offset, now._nr_pr*sizeof(OFFT));",
    "		memcpy((uchar *)f.m_p_skip,   (uchar *)proc_skip,   "
    "now._nr_pr*sizeof(uchar));",
    "	}", "	if (now._nr_qs > 0)", "	{	memcpy((uchar *) "
                                      "f.m_q_offset, (uchar *) q_offset, "
                                      "now._nr_qs*sizeof(OFFT));",
    "		memcpy((uchar *) f.m_q_skip,   (uchar *) q_skip,   "
    "now._nr_qs*sizeof(uchar));",
    "	}", "#if defined(C_States) && (HAS_TRACK==1) && (HAS_STACK==1)",
    "	c_stack((uchar *) f.m_c_stack); /* save unmatched tracked data */",
    "#endif", "#ifdef FULL_TRAIL", "	f.m_stack  = stack_last[core_id];",
    "#endif", "	f.nr_handoffs = nr_handoffs+1;",
    "	f.m_tau    = trpt->tau;", "	f.m_o_pm   = trpt->o_pm;",
    "	f.m_boq    = boq;", "	f.m_vsize  = vsize;", "",
    "	if (query_in_progress == 1)", "	{	query_in_progress = 2;",
    "	}", "	if (dsk_file < 0)",
    "	{	sprintf(dsk_name, \"Q%%.3d_%%.3d.tmp\", Dsk_W_Nr, core_id);",
    "		dsk_file = open(dsk_name, WFLAGS, 0644);",
    "		dsk_read = open(dsk_name, RFLAGS);",
    "		if (dsk_file < 0 || dsk_read < 0)",
    "		{	cpu_printf(\"File: <%%s>\\n\", dsk_name);",
    "			Uerror(\"cannot open diskfile\");", "		}",
    "		Dsk_W_Nr++; /* nr of next file to open */",
    "		cpu_printf(\"created temporary diskfile %%s\\n\", dsk_name);",
    "	} else if ((dsk_written+1)%%MAX_DSK_FILE == 0)",
    "	{	close(dsk_file); /* close write handle */",
    "		sprintf(dsk_name, \"Q%%.3d_%%.3d.tmp\", Dsk_W_Nr++, core_id);",
    "		dsk_file = open(dsk_name, WFLAGS, 0644);",
    "		if (dsk_file < 0)",
    "		{	cpu_printf(\"File: <%%s>\\n\", dsk_name);",
    "			Uerror(\"aborting: cannot open new diskfile\");",
    "		}",
    "		cpu_printf(\"created temporary diskfile %%s\\n\", dsk_name);",
    "	}", "	if (write(dsk_file, &f, sizeof(SM_frame)) != sizeof(SM_frame))",
    "	{	Uerror(\"aborting -- disk write failed (disk full?)\");",
    "	}", "	nstates_put++;", "	dsk_written++;", "}", "#endif", "",
    "int", "mem_hand_off(void)", "{", "	if (search_terminated == NULL",
    "	||  *search_terminated != 0)	/* not a full crash check */",
    "	{	pan_exit(0);", "	}",
    "	iam_alive();		/* on every transition of Down */",
    "#ifdef USE_DISK",
    "	mem_drain();		/* maybe call this also on every Up */",
    "#endif", "	if (depth > z_handoff	/* above handoff limit */",
    "#ifndef SAFETY", "	&&  !a_cycles		/* not in liveness mode */",
    "#endif", "#if SYNC", "	&&  boq == -1		/* not mid-rv */",
    "#endif", "#ifdef VERI",
    "	&&  (trpt->tau&4)	 /* claim moves first  */",
    "	&&  !((trpt-1)->tau&128) /* not a stutter move */", "#endif",
    "	&&  !(trpt->tau&8))	/* not an atomic move */",
    "	{	int q = (core_id + 1) %% NCORE;	/* circular handoff */",
    "	#ifdef GENEROUS",
    "		if (prcnt[q] < LN_FRAMES)", /* not the best strategy */
    "	#else", "		if (TargetQ_NotFull(q)",
    "		&& (dfs_phase2 == 0 || prcnt[core_id] > 0))", /* not locked, ok
                                                                 if race */
    "	#endif",
    "		{	mem_put(q);", /* only 1 writer: lock-free */
    "			return 1;", "		}",
    "		{	int rval;", "	#ifndef NGQ",
    "			rval = GlobalQ_HasRoom();", "	#else",
    "			rval = 0;", "	#endif", "	#ifdef USE_DISK",
    "			if (rval == 0)",
    "			{	void mem_file(void);",
    "				mem_file();",
    "				rval = 1;", "			}", "	#endif",
    "			return rval;", "		}", "	}",
    "	return 0; /* i.e., no handoff */", "}", "", "void",
    "mem_put_acc(void)	/* liveness mode */",
    "{	int q = (core_id + 1) %% NCORE;", "", "	if (search_terminated == NULL",
    "	||  *search_terminated != 0)", "	{	pan_exit(0);",
    "	}", "#ifdef USE_DISK", "	mem_drain();", "#endif",
    "	/* some tortured use of preprocessing: */",
    "#if !defined(NGQ) || defined(USE_DISK)", "	if (TargetQ_Full(q))",
    "	{", "#endif", "#ifndef NGQ", "		if (GlobalQ_HasRoom())",
    "		{	return;", "		}", "#endif", "#ifdef USE_DISK",
    "		mem_file();", "	} else", "#else",
    "	#if !defined(NGQ) || defined(USE_DISK)", "	}", "	#endif",
    "#endif", "	{	mem_put(q);", "	}", "}", "",
    "#if defined(WIN32) || defined(WIN64)", /* visual studio */
    "void", "init_shm(void)		/* initialize shared work-queues */",
    "{	char	key[512];", "	int	n, m;", "	int	must_exit = 0;",
    "", "	if (core_id == 0 && verbose)", "	{	printf(\"cpu0: "
                                               "step 3: allocate shared "
                                               "work-queues %%g Mb\\n\",",
    "			((double) NCORE * LWQ_SIZE + GWQ_SIZE) / (1048576.));",
    "	}", "	for (m = 0; m < NR_QS; m++)	/* last q is global 1 */",
    "	{	double qsize = (m == NCORE) ? GWQ_SIZE : LWQ_SIZE;",
    "		sprintf(key, \"Global\\\\pan_%%s_%%.3d\", PanSource, m);",
    "		if (core_id == 0)", /* root process creates shared memory
                                       segments */
    "		{	shmid[m] = CreateFileMapping(",
    "				INVALID_HANDLE_VALUE,	/* use paging file */",
    "				NULL,			/* default security */",
    "				PAGE_READWRITE,		/* access permissions "
    "*/",
    "				0,			/* high-order 4 bytes "
    "*/",
    "				qsize,			/* low-order bytes, "
    "size in bytes */",
    "				key);			/* name */",
    "		} else			/* worker nodes just open these "
    "segments */",
    "		{	shmid[m] = OpenFileMapping(",
    "				FILE_MAP_ALL_ACCESS,	/* read/write access "
    "*/",
    "				FALSE,			/* children do not "
    "inherit handle */",
    "				key);", "		}",
    "		if (shmid[m] == NULL)",
    "		{	fprintf(stderr, \"cpu%%d: could not create or open "
    "shared queues\\n\",",
    "				core_id);", "			must_exit = 1;",
    "			break;", "		}", "		/* attach: */",
    "		shared_mem[m] = (char *) MapViewOfFile(shmid[m], "
    "FILE_MAP_ALL_ACCESS, 0, 0, 0);",
    "		if (shared_mem[m] == NULL)", "		{ fprintf(stderr, "
                                             "\"cpu%%d: cannot attach shared "
                                             "q%%d (%%d Mb)\\n\",",
    "			core_id, m+1, (int) (qsize/(1048576.)));",
    "		  must_exit = 1;", "		  break;", "		}", "",
    "		memcnt += qsize;", "",
    "		m_workq[m] = (SM_frame *) shared_mem[m];",
    "		if (core_id == 0)",
    "		{	int nframes = (m == NCORE) ? GN_FRAMES : LN_FRAMES;",
    "			for (n = 0; n < nframes; n++)",
    "			{	m_workq[m][n].m_vsize = 0;",
    "				m_workq[m][n].m_boq = 0;",
    "	}	}	}", "", "	if (must_exit)",
    "	{	fprintf(stderr, \"pan: check './pan --' for usage "
    "details\\n\");",
    "		pan_exit(1);	/* calls cleanup_shm */", "	}", "}", "",
    "static uchar *", "prep_shmid_S(size_t n)		/* either sets SS or "
                      "H_tab, WIN32/WIN64 */",
    "{	char	*rval;", "#ifndef SEP_STATE", "	char	key[512];", "",
    "	if (verbose && core_id == 0)", "	{", "	#ifdef BITSTATE",
    "		printf(\"cpu0: step 1: allocate shared bitstate %%g Mb\\n\",",
    "			(double) n / (1048576.));", "	#else",
    "		printf(\"cpu0: step 1: allocate shared hastable %%g Mb\\n\",",
    "			(double) n / (1048576.));", "	#endif", "	}",
    "	#ifdef MEMLIM", "	if (memcnt + (double) n > memlim)",
    "	{	printf(\"cpu%%d: S %%8g + %%d Kb exceeds memory limit of %%8g "
    "Mb\\n\",",
    "			core_id, memcnt/1024., n/1024, memlim/(1048576.));",
    "		printf(\"cpu%%d: insufficient memory -- aborting\\n\", "
    "core_id);",
    "		exit(1);", "	}", "	#endif", "",
    "	/* make key different from queues: */",
    "	sprintf(key, \"Global\\\\pan_%%s_%%.3d\", PanSource, NCORE+2); /* "
    "different from qs */",
    "", "	if (core_id == 0)	/* root */",
    "	{	shmid_S = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,",
    "#ifdef WIN64",
    "			PAGE_READWRITE, (n>>32), (n & 0xffffffff), key);",
    "#else", "			PAGE_READWRITE, 0, n, key);", "#endif",
    "		memcnt += (double) n;",
    "	} else			/* worker */",
    "	{	shmid_S = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, key);",
    "	}",

    "	if (shmid_S == NULL)", "	{", "	#ifdef BITSTATE",
    "		fprintf(stderr, \"cpu%%d: cannot %%s shared bitstate\",",
    "			core_id, core_id?\"open\":\"create\");", "	#else",
    "		fprintf(stderr, \"cpu%%d: cannot %%s shared hashtable\",",
    "			core_id, core_id?\"open\":\"create\");", "	#endif",
    "		fprintf(stderr, \"pan: check './pan --' for usage "
    "details\\n\");",
    "		pan_exit(1);", "	}",
    "", "	rval = (char *) MapViewOfFile(shmid_S, FILE_MAP_ALL_ACCESS, 0, "
        "0, 0);	/* attach */",
    "	if ((char *) rval == NULL)", "	{ fprintf(stderr, \"cpu%%d: cannot "
                                     "attach shared bitstate or "
                                     "hashtable\\n\", core_id);",
    "	  fprintf(stderr, \"pan: check './pan --' for usage details\\n\");",
    "	  pan_exit(1);", "	}", "#else", "	rval = (char *) emalloc(n);",
    "#endif", "	return (uchar *) rval;", "}", "", "static uchar *",
    "prep_state_mem(size_t n)		/* WIN32/WIN64 sets memory arena for "
    "states */",
    "{	char	*rval;", "	char	key[512];", "	static int cnt = 3;	"
                                                    "	/* start larger than "
                                                    "earlier ftok calls */",
    "", "	if (verbose && core_id == 0)", "	{	printf(\"cpu0: "
                                               "step 2+: pre-allocate memory "
                                               "arena %%d of %%g Mb\\n\",",
    "			cnt-3, (double) n / (1048576.));", "	}",
    "	#ifdef MEMLIM", "	if (memcnt + (double) n > memlim)",
    "	{	printf(\"cpu%%d: error: M %%.0f + %%.0f exceeds memory limit of "
    "%%.0f Kb\\n\",",
    "			core_id, memcnt/1024.0, (double) n/1024.0, "
    "memlim/1024.0);",
    "		return NULL;", "	}", "	#endif", "",
    "	sprintf(key, \"Global\\\\pan_%%s_%%.3d\", PanSource, NCORE+cnt); "
    "cnt++;",
    "", "	if (core_id == 0)",
    "	{	shmid_M = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,",
    "#ifdef WIN64",
    "			PAGE_READWRITE, (n>>32), (n & 0xffffffff), key);",
    "#else", "			PAGE_READWRITE, 0, n, key);", "#endif",
    "	} else",
    "	{	shmid_M = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, key);",
    "	}", "	if (shmid_M == NULL)",
    "	{	printf(\"cpu%%d: failed to get pool of shared memory nr %%d of "
    "size %%d\\n\",",
    "			core_id, cnt-3, n);",
    "		printf(\"pan: check './pan --' for usage details\\n\");",
    "		return NULL;",
    "	}", "	rval = (char *) MapViewOfFile(shmid_M, FILE_MAP_ALL_ACCESS, 0, "
            "0, 0);	/* attach */",
    "", "	if (rval == NULL)", "	{ printf(\"cpu%%d: failed to attach "
                                    "pool of shared memory nr %%d of size "
                                    "%%d\\n\",",
    "		core_id, cnt-3, n);", "	  return NULL;", "	}",
    "	return (uchar *) rval;", "}", "", "void",
    "init_HT(unsigned long n)	/* WIN32/WIN64 version */",
    "{	volatile char	*x;", "	double  get_mem;", "#ifndef SEP_STATE",
    "	char	*dc_mem_start;", "#endif", "	if (verbose) printf(\"cpu%%d: "
                                           "initialization for Windows\\n\", "
                                           "core_id);",
    "", "#ifdef SEP_STATE", " #ifndef MEMLIM", "	if (verbose)",
    "	{	printf(\"cpu0: steps 0,1: no -DMEMLIM set\\n\");", "	}",
    " #else", "	if (verbose)", "	printf(\"cpu0: steps 0,1: -DMEMLIM=%%d "
                               "Mb - (hashtable %%g Mb + workqueues %%g "
                               "Mb)\\n\",",
    "		MEMLIM, ((double)n/(1048576.)), ((double) NCORE * LWQ_SIZE + "
    "GWQ_SIZE)/(1048576.));",
    "#endif", "	get_mem = NCORE * sizeof(double) + (1 + CS_NR) * "
              "sizeof(void *)+ 4*sizeof(void *) + 2*sizeof(double);",
    "	/* NCORE * is_alive + search_terminated + CS_NR * sh_lock + 6 gr vars "
    "*/",
    "	get_mem += 4 * NCORE * sizeof(void *);", /* prfree, prfull, prcnt, prmax
                                                    */
    " #ifdef FULL_TRAIL",
    "	get_mem += (NCORE) * sizeof(Stack_Tree *);",
    "	/* NCORE * stack_last */", " #endif",
    "	x = (volatile char *) prep_state_mem((size_t) get_mem);",
    "	shmid_X = (void *) x;", "	if (x == NULL)",
    "	{	printf(\"cpu0: could not allocate shared memory, see ./pan "
    "--\\n\");",
    "		exit(1);", "	}",
    "	search_terminated = (volatile unsigned int *) x; /* comes first */",
    "	x += sizeof(void *); /* maintain alignment */", "",
    "	is_alive   = (volatile double *) x;", "	x += NCORE * sizeof(double);",
    "", "	sh_lock   = (volatile int *) x;",
    "	x += CS_NR * sizeof(void *); /* allow 1 word per entry */", "",
    "	grfree    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grfull    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grcnt    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grmax    = (volatile int *) x;", "	x += sizeof(void *);",
    "	prfree = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prfull = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prcnt = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prmax = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	gr_readmiss    = (volatile double *) x;", "	x += sizeof(double);",
    "	gr_writemiss    = (volatile double *) x;", "	x += sizeof(double);",
    "", "	#ifdef FULL_TRAIL",
    "		stack_last = (volatile Stack_Tree **) x;",
    "		x += NCORE * sizeof(Stack_Tree *);", "	#endif", "",
    "	#ifndef BITSTATE", "		H_tab = (struct H_el **) emalloc(n);",
    "	#endif", "#else", "	#ifndef MEMLIM",
    "		#warning MEMLIM not set", /* cannot happen */
    "		#define MEMLIM	(2048)", "	#endif", "",
    "	if (core_id == 0 && verbose)",
    "		printf(\"cpu0: step 0: -DMEMLIM=%%d Mb - (hashtable %%g Mb + "
    "workqueues %%g Mb) = %%g Mb for state storage\\n\",",
    "		MEMLIM, ((double)n/(1048576.)), ((double) NCORE * LWQ_SIZE + "
    "GWQ_SIZE)/(1048576.),",
    "		(memlim - memcnt - (double) n - ((double) NCORE * LWQ_SIZE + "
    "GWQ_SIZE))/(1048576.));",
    "	#ifndef BITSTATE", "		H_tab = (struct H_el **) "
                           "prep_shmid_S((size_t) n);	/* hash_table */",
    "	#endif",
    "	get_mem = memlim - memcnt - ((double) NCORE) * LWQ_SIZE - GWQ_SIZE;",
    "	if (get_mem <= 0)",
    "	{	Uerror(\"internal error -- shared state memory\");", "	}", "",
    "	if (core_id == 0 && verbose)",
    "	{	printf(\"cpu0: step 2: shared state memory %%g Mb\\n\",",
    "			get_mem/(1048576.));", "	}",
    "	x = dc_mem_start = (char *) prep_state_mem((size_t) get_mem);	/* for "
    "states */",
    "	if (x == NULL)",
    "	{	printf(\"cpu%%d: insufficient memory -- aborting\\n\", "
    "core_id);",
    "		exit(1);", "	}", "",
    "	search_terminated = (volatile unsigned int *) x; /* comes first */",
    "	x += sizeof(void *); /* maintain alignment */", "",
    "	is_alive   = (volatile double *) x;", "	x += NCORE * sizeof(double);",
    "", "	sh_lock   = (volatile int *) x;",
    "	x += CS_NR * sizeof(int);", "",
    "	grfree    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grfull    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grcnt    = (volatile int *) x;", "	x += sizeof(void *);",
    "	grmax    = (volatile int *) x;", "	x += sizeof(void *);",
    "	prfree = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prfull = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prcnt = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	prmax = (volatile int *) x;", "	x += NCORE * sizeof(void *);",
    "	gr_readmiss = (volatile double *) x;", "	x += sizeof(double);",
    "	gr_writemiss = (volatile double *) x;", "	x += sizeof(double);",
    "", " #ifdef FULL_TRAIL", "	stack_last = (volatile Stack_Tree **) x;",
    "	x += NCORE * sizeof(Stack_Tree *);", " #endif",
    "	if (((long)x)&(sizeof(void *)-1))	/* word alignment */",
    "	{	x += sizeof(void *)-(((long)x)&(sizeof(void *)-1)); /* 64-bit "
    "align */",
    "	}", "", "	#ifdef COLLAPSE", "	ncomps = (unsigned long *) x;",
    "	x += (256+2) * sizeof(unsigned long);", "	#endif", "",
    "	dc_shared = (sh_Allocater *) x; /* in shared memory */",
    "	x += sizeof(sh_Allocater);", "",
    "	if (core_id == 0)	/* root only */",
    "	{	dc_shared->dc_id     = shmid_M;",
    "		dc_shared->dc_start  = (void *) dc_mem_start;",
    "		dc_shared->dc_arena  = x;",
    "		dc_shared->pattern   = 1234567;",
    "		dc_shared->dc_size   = (long) get_mem - (long) (x - "
    "dc_mem_start);",
    "		dc_shared->nxt       = NULL;", "	}", "#endif", "}", "",
    "#if defined(WIN32) || defined(WIN64) || defined(__i386__) || "
    "defined(__x86_64__)",
    "extern BOOLEAN InterlockedBitTestAndSet(LONG volatile* Base, LONG Bit);",
    "int", "tas(volatile LONG *s)", /* atomic test and set */
    "{	return InterlockedBitTestAndSet(s, 1);", "}", "#else",
    "	#error missing definition of test and set operation for this platform",
    "#endif", "", "void", "cleanup_shm(int val)", "{	int m;",
    "	static int nibis = 0;", "", "	if (nibis != 0)",
    "	{	printf(\"cpu%%d: Redundant call to cleanup_shm(%%d)\\n\", "
    "core_id, val);",
    "		return;", "	} else", "	{	nibis = 1;", "	}",
    "	if (search_terminated != NULL)",
    "	{	*search_terminated |= 16; /* cleanup_shm */", "	}", "",
    "	for (m = 0; m < NR_QS; m++)", "	{	if (shmid[m] != NULL)",
    "		{	UnmapViewOfFile((char *) shared_mem[m]);",
    "			CloseHandle(shmid[m]);", "	}	}",
    "#ifdef SEP_STATE", "	UnmapViewOfFile((void *) shmid_X);",
    "	CloseHandle((void *) shmid_M);", "#else", "	#ifdef BITSTATE",
    "		if (shmid_S != NULL)",
    "		{	UnmapViewOfFile(SS);",
    "			CloseHandle(shmid_S);", "		}", "	#else",
    "		if (core_id == 0 && verbose)",
    "		{	printf(\"cpu0: done, %%ld Mb of shared state memory "
    "left\\n\",",
    "				dc_shared->dc_size / (long)(1048576));",
    "		}", "		if (shmid_S != NULL)",
    "		{	UnmapViewOfFile(H_tab);",
    "			CloseHandle(shmid_S);", "		}",
    "		shmid_M = (void *) (dc_shared->dc_id);",
    "		UnmapViewOfFile((char *) dc_shared->dc_start);",
    "		CloseHandle(shmid_M);", "	#endif", "#endif",
    "	/* detached from shared memory - so cannot use cpu_printf */",
    "	if (verbose)",
    "	{	printf(\"cpu%%d: done -- got %%d states from queue\\n\",",
    "			core_id, nstates_get);", "	}", "}", "", "void",
    "mem_get(void)", "{	SM_frame   *f;", "	int is_parent;", "",
    "#if defined(MA) && !defined(SEP_STATE)",
    "	#error MA requires SEP_STATE in multi-core mode", "#endif",
    "#ifdef BFS", "	#error BFS is not supported in multi-core mode",
    "#endif", "#ifdef SC", "	#error SC is not supported in multi-core mode",
    "#endif", "	init_shm();	/* we are single threaded when this starts */",
    "	signal(SIGINT, give_up);	/* windows control-c interrupt */", "",
    "	if (core_id == 0 && verbose)", "	{	printf(\"cpu0: step 4: "
                                       "creating additional workers (proxy "
                                       "%%d)\\n\",",
    "			proxy_pid);", "	}", "#if 0",
    "	if NCORE > 1 the child or the parent should fork N-1 more times",
    "	the parent is the only process with core_id == 0 and is_parent > 0",
    "	the others (workers) have is_parent = 0 and core_id = 1..NCORE-1",
    "#endif", "	if (core_id == 0)			/* root starts "
              "up the workers */",
    "	{	worker_pids[0] = (DWORD) getpid();	/* for completeness */",
    "		while (++core_id < NCORE)	/* first worker sees core_id = "
    "1 */",
    "		{	char cmdline[64];",
    "			STARTUPINFO si = { sizeof(si) };",
    "			PROCESS_INFORMATION pi;", "",
    "			if (proxy_pid == core_id)	/* always non-zero */",
    "			{	sprintf(cmdline, \"pan_proxy.exe -r %%s-Q%%d "
    "-Z%%d\",",
    "					o_cmdline, getpid(), core_id);",
    "			} else",
    "			{	sprintf(cmdline, \"pan.exe %%s-Q%%d -Z%%d\",",
    "					o_cmdline, getpid(), core_id);",
    "			}", "			if (verbose) printf(\"cpu%%d: "
                            "spawn %%s\\n\", core_id, cmdline);",
    "", "			is_parent = CreateProcess(0, cmdline, 0, 0, "
        "FALSE, 0, 0, 0, &si, &pi);",
    "			if (is_parent == 0)",
    "			{	Uerror(\"fork failed\");",
    "			}",
    "			worker_pids[core_id] = pi.dwProcessId;",
    "			worker_handles[core_id] = pi.hProcess;",
    "			if (verbose)",
    "			{	cpu_printf(\"created core %%d, pid %%d\\n\",",
    "					core_id, pi.dwProcessId);",
    "			}",
    "			if (proxy_pid == core_id)	/* we just created the "
    "receive half */",
    "			{	/* add proxy send, store pid in proxy_pid_snd "
    "*/",
    "				sprintf(cmdline, \"pan_proxy.exe -s %%s-Q%%d "
    "-Z%%d -Y%%d\",",
    "					o_cmdline, getpid(), core_id, "
    "worker_pids[proxy_pid]);",
    "				if (verbose) printf(\"cpu%%d: spawn %%s\\n\", "
    "core_id, cmdline);",
    "				is_parent = CreateProcess(0, cmdline, 0,0, "
    "FALSE, 0,0,0, &si, &pi);",
    "				if (is_parent == 0)",
    "				{	Uerror(\"fork failed\");",
    "				}",
    "				proxy_pid_snd = pi.dwProcessId;",
    "				proxy_handle_snd = pi.hProcess;",
    "				if (verbose)",
    "				{	cpu_printf(\"created core %%d, pid %%d "
    "(send proxy)\\n\",",
    "						core_id, pi.dwProcessId);",
    "		}	}	}",
    "		core_id = 0;		/* reset core_id for root process */",
    "	} else	/* worker */",
    "	{	static char db0[16];	/* good for up to 10^6 cores */",
    "		static char db1[16];",
    "		tprefix = db0; sprefix = db1;",
    "		sprintf(tprefix, \"cpu%%d_trail\", core_id);	/* avoid "
    "conflicts on file access */",
    "		sprintf(sprefix, \"cpu%%d_rst\", core_id);",
    "		memcnt = 0;	/* count only additionally allocated memory */",
    "	}", "	if (verbose)", "	{	cpu_printf(\"starting core_id "
                               "%%d -- pid %%d\\n\", core_id, getpid());",
    "	}", "	if (core_id == 0 && !remote_party)",
    "	{	new_state();	/* root starts the search */",
    "		if (verbose)",
    "		cpu_printf(\"done with 1st dfs, nstates %%g (put %%d states), "
    "start reading q\\n\",",
    "			nstates, nstates_put);",
    "		dfs_phase2 = 1;", "	}",
    "	Read_Queue(core_id);	/* all cores */", "", "	if (verbose)",
    "	{	cpu_printf(\"put %%6d states into queue -- got %%6d\\n\",",
    "			nstates_put, nstates_get);", "	}", "	done = 1;",
    "	wrapup();", "	exit(0);", "}", "#endif", /* WIN32 || WIN64 */
    "", "#ifdef BITSTATE", "void", "init_SS(unsigned long n)", "{",
    "	SS = (uchar *) prep_shmid_S((size_t) n);",
    "	init_HT(0L);", /* locks and shared memory for Stack_Tree allocations */
    "}", "#endif",     /* BITSTATE */
    "", "#endif",      /* NCORE>1 */
    0,
};
