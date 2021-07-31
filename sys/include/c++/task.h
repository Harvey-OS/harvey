/*ident	"@(#)C++env:incl-master/const-headers/task.h	1.6" */
/**************************************************************************
			Copyright (c) 1984 AT&T
	  		  All Rights Reserved  	

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
	
	The copyright notice above does not evidence any   	
	actual or intended publication of such source code.

*****************************************************************************/

#ifndef TASKH
#define TASKH

#pragma "ape/libap.a"
#pragma "c++/libC.a"
#pragma "c++/libtask.a"

/*	HEADER FILE FOR THE TASK SYSTEM		*/

#include <stdio.h>
#include "label.h"		/* like setjmp, but really works */

class object;
class sched;	/* : public object */
class timer;	/* : public sched  */
class task;	/* : public sched  */
class qhead;	/* : public object */
class qtail;	/* : public object */
class team;

#if defined(_SHARED_ONLY)
#define	DEFAULT_MODE	SHARED
#else
#define	DEFAULT_MODE	DEDICATED
#endif

/* error codes */
#define task_error_messages						\
macro_start								\
macro(0,E_ERROR,"")							\
macro(1,E_OLINK,"object::delete(): has chain")				\
macro(2,E_ONEXT,"object::delete(): on chain")				\
macro(3,E_GETEMPTY,"qhead::get(): empty")				\
macro(4,E_PUTOBJ,"qtail::put(): object on other queue")			\
macro(5,E_PUTFULL,"qtail::put(): full")					\
macro(6,E_BACKOBJ,"qhead::putback(): object on other queue")		\
macro(7,E_BACKFULL,"qhead::putback(): full")				\
macro(8,E_SETCLOCK,"sched::setclock(): clock!=0")			\
macro(9,E_CLOCKIDLE,"sched::schedule(): clock_task not idle")		\
macro(10,E_RESTERM,"sched::insert(): cannot schedule terminated sched")	\
macro(11,E_RESRUN,"sched::schedule(): running")				\
macro(12,E_NEGTIME,"sched::schedule(): clock<0")			\
macro(13,E_RESOBJ,"sched::schedule(): task or timer on other queue")	\
macro(14,E_HISTO,"histogram::histogram(): bad arguments")		\
macro(15,E_STACK,"task::restore() or task::task(): stack overflow")	\
macro(16,E_STORE,"new: free store exhausted")				\
macro(17,E_TASKMODE,"task::task(): bad mode")				\
macro(18,E_TASKDEL,"task::~task(): not terminated")			\
macro(19,E_TASKPRE,"task::preempt(): not running")			\
macro(20,E_TIMERDEL,"timer::~timer(): not terminated")			\
macro(21,E_SCHTIME,"sched::schedule(): runchain corrupted: bad time")	\
macro(22,E_SCHOBJ,"sched object used directly (not as base)")		\
macro(23,E_QDEL,"queue::~queue(): not empty")				\
macro(24,E_RESULT,"task::result(): thistask->result()")			\
macro(25,E_WAIT,"task::wait(): wait for self")				\
macro(26,E_FUNCS,"FrameLayout::FrameLayout(): function start")		\
macro(27,E_FRAMES,"FrameLayout::FrameLayout(): frame size")		\
macro(28,E_REGMASK,"task::fudge_return(): unexpected register mask")	\
macro(29,E_FUDGE_SIZE,"task::fudge_return(): frame too big")		\
macro(30,E_NO_HNDLR,"sigFunc - no handler for signal")			\
macro(31,E_BADSIG,"illegal signal number")				\
macro(32,E_LOSTHNDLR,"Interrupt_handler::~Interrupt_handler(): signal handler not on chain")			\
macro_end(E_LOSTHNDLR)
#define macro_start
#define macro(num,name,string) const name = num ;
#define macro_end(last_name) const MAXERR = last_name;
task_error_messages
#undef macro_start
#undef macro
#undef macro_end


typedef int (*PFIO)(int,object*);
typedef void (*PFV)();

/* print flags, used as arguments to class print functions */
#define CHAIN		1
#define VERBOSE		2
#define STACK		4

/* DATA STRUCTURES */
/*
	object --> olink --> olink ...
	   |         |         |
	  ...        V         V
	   |        task      task
	   V
	object --> ...
*/

class olink
/*	the building block for chains of task pointers */
{
friend object;
	olink*	l_next;
	task*	l_task;
		olink(task* t, olink* l) { l_task=t; l_next=l; };
};

class object
{
friend sched;
friend task;
public:
	enum objtype { OBJECT, TIMER, TASK, QHEAD, QTAIL, INTHANDLER };
private:
	olink*	o_link;
	static task*	thxstxsk;
public:
	object*	o_next;
	virtual objtype	o_type()	{ return OBJECT; }

		object()	{ o_link=0; o_next=0; }
	virtual	~object();

	void	remember(task*);	// save for alert
	void	forget(task*);	// remove all occurrences of task from chain
	void	alert();	// prepare IDLE tasks for scheduling
	virtual int	pending();  // TRUE if this object should be waited for
	virtual	void	print(int, int =0); // 1st arg VERBOSE, CHAIN, or STACK
	static int	task_error(int, object*);
				// the central error function
	int	task_error(int);	// obsolete; use static version
	static task*	this_task()	{ return thxstxsk; }
	static	PFIO	error_fct;	// user-supplied error function
};

// fake compatibility with previous version
#define thistask (object::this_task())

void _print_error(int);

class sched : public object {	// only instances of subclasses are used
friend timer;
friend task;
friend object;
friend void _print_error(int n);
//friend SIG_FUNC_TYP sigFunc;
public:
	enum statetype { IDLE=1, RUNNING=2, TERMINATED=4 };
private:
	static int	keep_waiting_count;
	static sched*	runchain;    // list of ready-to-run scheds (by s_time)
	static sched*	priority_sched;	// if non-zero, sched to run next
	static	long	clxck;
	static int	exit_status;
	long	s_time;		/* time to sched; result after cancel() */
	statetype	s_state;	/* IDLE, RUNNING, TERMINATED */
	void	schedule();	/* sched clock_task or front of runchain */
	virtual	void	resume();
	void	insert(long,object*); /* sched for long time units, t_alert=obj */
	void	remove();	/* remove from runchain & make IDLE */

protected:
		sched() : s_time(0), s_state(IDLE) {}
public:

	static void	setclock(long);
	static long	get_clock() { return clxck; }
	static sched*	get_run_chain() { return runchain; }
	static int	get_exit_status() { return exit_status; }
	static void	set_exit_status( int i ) { exit_status = i; }
	sched*	get_priority_sched() { return priority_sched; }
	static	task*	clock_task;	// awoken at each clock tick
	long	rdtime()	{ return s_time; };
	statetype	rdstate()	{ return s_state; };
	int	pending()	{ return s_state != TERMINATED; }
	int	keep_waiting()	{ return keep_waiting_count++; }
	int	dont_wait()	{ return keep_waiting_count--; }

	void	cancel(int);
	int	result();
	virtual void	setwho(object* t);  // who alerted me
	void	print(int, int =0);
	static	PFV	exit_fct;	// user-supplied exit function
};
// for compatibility with pre-2.0 releases,
// but conflicts with time.h
//#define clock (sched::get_clock())
inline void	setclock(long i) { sched::setclock(i); }

// for compatibility with pre-2.0 releases
#define run_chain (sched::get_run_chain())

class timer : public sched {
	void	resume();
public:
		timer(long);
		~timer();
	void	reset(long);
	object::objtype	o_type()	{ return TIMER; }
	void	setwho(object*)	{ }  // do nothing
	void	print(int, int =0);
};

/* check stack size if set */
extern _hwm;

class task : public sched {
friend sched;
friend void __task__init();
public:
	enum modetype { DEDICATED=1, SHARED=2 };
private:
	static task*	txsk_chxin;   	// list of all tasks
	static team*	team_to_delete;	// delete this team after task switch
	int	curr_hwm();		// "high water mark"
					//	(how high stack has risen)
	int	swap_stack(int*,int*);	// initialize child stack */
	void	fudge_return(int*);	//used in starting new tasks
	void	copy_share();		// used in starting shared tasks
	void	get_size();		// ditto -- saves size of active stack
	void	resume();
	void	swap(int);
	void	swapjmp();
	void	doswap(task *, int);
	Label	copy_task(Label, unsigned long *, unsigned long *);

	// simple check for stack overflow--not used for main task
	void	settrap();
	void	checktrap();

	/* WARNING: t_framep, th, and t_ap are manipulated as offsets from
	 * task by swap(); those, and t_basep, t_size, and t_savearea are
	 * manipulated as offsets by sswap().
	 * Do not insert new data members before these.
	 */
	Label	t_env;		// stuff needed to restore process
	unsigned long	t_size;	// size of active stack (used for SHARED)
				// holds hwm after cancel()
	long	t_trap;		// used for stack overflow check
	team	*t_team;	// stack and info for sharing
	char	*t_savearea;	// area SHARED stack saved
	char	*t_save;	// area SHARED stack saved

	modetype	t_mode;		/* DEDICATED/SHARED stack */
	int	t_stacksize;

	object*	t_alert;	/* object that inserted you */
protected:
		task(char *name=0, modetype mode=DEFAULT_MODE, int stacksize=0);
public:
		~task();

	object::objtype	o_type()	{ return TASK; }
	task*	t_next;		/* insertion in "task_chain" */
	char*	t_name;

	static task*	get_task_chain() { return txsk_chxin; }
	int	waitvec(object**);
	int	waitlist(object* ...);
	void	wait(object* ob);

	void	delay(long);
	long	preempt();
	void	sleep(object* t =0);	// t is remembered
	void	resultis(int);
	void	cancel(int);
	void	setwho(object* t)	{ t_alert = t; }
	void	print(int, int =0);
	object*	who_alerted_me()	{ return t_alert; }
};
// for compatibility
#define task_chain (task::get_task_chain())

// an Interrupt_handler supplies an interrupt routine that runs when the
// interrupt occurs (real time).  Also the Interrupt_handler can be waited for.
class Interrupt_handler : public object {
friend	class Interrupt_alerter;
//friend SIG_FUNC_TYP sigFunc;
	int	id;		// signal or interrupt number
	int	got_interrupt;  // an interrupt has been received
				// but not alerted
	Interrupt_handler	*old;	// previous handler for this signal
	virtual void	interrupt();	// runs at real time
public:
		Interrupt_handler(int sig_num);
		~Interrupt_handler();
	object::objtype	o_type()	{ return INTHANDLER; }
	int	pending();	// FALSE once after interrupt
};


/* QUEUE MANIPULATION (see queue.c) */
/*
	qhead <--> oqueue <--> qtail   (qhead, qtail independent)
	oqueue ->> circular queue of objects
*/

/* qh_modes */
enum qmodetype { EMODE, WMODE, ZMODE };

class oqueue
{
friend qhead;
friend qtail;
	int	q_max;
	int	q_count;
	object*	q_ptr;
	qhead*	q_head;
	qtail*	q_tail;

		oqueue(int m)	{ q_max=m; q_count=0; q_head=0; q_tail=0; };
		~oqueue()	{ (q_count)?object::task_error(E_QDEL,0):0; };

	void	print(int);
};

class qhead : public object
{
friend qtail;
	qmodetype	qh_mode;	/* EMODE,WMODE,ZMODE */
	oqueue*		qh_queue;
public:
			qhead(qmodetype = WMODE, int = 10000);
			~qhead();

	object::objtype		o_type()	{ return QHEAD; }
	object*		get();
	int		putback(object*);

	int		rdcount()	{ return qh_queue->q_count; }
	int		rdmax()		{ return qh_queue->q_max; }
	qmodetype	rdmode()	{ return qh_mode; }
	qtail*		tail();

	qhead*		cut();
	void		splice(qtail*);

	void		setmode(qmodetype m)	{ qh_mode = m; };
	void		setmax(int m)	{ qh_queue->q_max = m; };
	int		pending()	{ return rdcount() == 0; }
	void		print(int, int =0);
};

class qtail : public object
{
friend qhead;
	qmodetype	qt_mode;
	oqueue*		qt_queue;
public:
			qtail(qmodetype = WMODE, int = 10000);
			~qtail();

	object::objtype		o_type()	{ return QTAIL; }
	int		put(object*);

	int		rdspace()	
			{ return qt_queue->q_max - qt_queue->q_count; };
	int		rdmax()		{ return qt_queue->q_max; };
	qmodetype	rdmode()	{ return qt_mode; };

	qtail*		cut();
	void 		splice(qhead*);

	qhead*		head();

	void		setmode(qmodetype m)	{ qt_mode = m; };
	void		setmax(int m)	{ qt_queue->q_max = m; };
	int		pending()	{ return rdspace() == 0; }

	void		print(int, int =0);
};


struct histogram
/*
	"nbin" bins covering the range [l:r] uniformly
	nbin*binsize == r-l
*/
{
	int	l, r;
	int	binsize;
	int	nbin;
	int*	h;
	long	sum;
	long	sqsum;
		histogram(int=16, int=0, int=16);

	void	add(int);
	void	print();
};

/*	the result of randint() is always >= 0	*/

#define DRAW (randx = randx*1103515245 + 12345)
#define ABS(x)	(x&0x7fffffff)
#define MASK(x) ABS(x)

class randint
/*	uniform distribution in the interval [0,MAXINT_AS_FLOAT] */
{
	long	randx;
public:
		randint(long s = 0)	{ randx=s; }
	void	seed(long s)	{ randx=s; }
	int	draw()		{ return MASK(DRAW); }
	float	fdraw();
};

class urand : public randint
/*	uniform distribution in the interval [low,high]	*/
{
public:
	int	low, high;
		urand(int l, int h)	{ low=l; high=h; }
	int	draw();
};

class erand : public randint
/*	exponential distribution random number generator */
{
public:
	int	mean;
		erand(int m) { mean=m; };
	int	draw();
};

// This task will alert Interrupt_handler objects.
class Interrupt_alerter : public task {
public:
		Interrupt_alerter();
		~Interrupt_alerter() { cancel (0); }
};

extern Interrupt_alerter	interrupt_alerter;

#endif
