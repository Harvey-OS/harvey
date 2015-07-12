/************************************************************************/
/*	Original Author: 						*/
/*		William Norcott (wnorcott@us.oracle.com)		*/
/*              4 Dunlap Drive						*/
/*              Nashua, NH 03060					*/
/*  									*/
/************************************************************************/
/*      Enhancements by:						*/
/*	 	Don Capps (capps@iozone.org)				*/
/* 		7417 Crenshaw						*/
/* 		Plano, TX 75025						*/
/*  									*/
/************************************************************************/
/*  Copyright 1991, 1992, 1994, 1998, 2000, 2001  William D. Norcott	*/
/************************************************************************/
/*									*/
/* Iozone is based on the original work done by William Norrcot. It has	*/
/* been enhanced so that it provides a more complete filesystem   	*/
/* characterization.							*/
/* Its purpose is to provide automated filesystem characterization.	*/
/* Enhancements have been made by:					*/
/*									*/
/* Don Capps	   	         capps@iozone.org			*/ 
/*									*/
/* Iozone can perform single stream and multi stream I/O		*/
/* also it now performs read, write, re-read, re-write, 		*/
/* read backwards, read/write random, re-read record, 			*/
/* pread, re-pread, re-pwrite, preadv, re-preadv, pwritev,		*/
/* stride read, and re-pwritev,mmap, POSIX async I/O, NFS               */
/* cluster testing and much more.					*/
/*									*/
/* The frontend now uses getopt() and the user can control many more	*/
/* of the actions.							*/
/*									*/
/*									*/
/************************************************************************/
/* THIS SOFTWARE IS PROVIDED BY DON CAPPS AND THE IOZONE CREW "AS IS    */
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED    */
/* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A      */
/* PARTICULAR PURPOSE ARE DISCLAIMED.					*/
/*									*/
/* IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY      */
/* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL   */
/* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE    */
/* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS        */
/* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER */
/* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      */
/* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE.       */
/************************************************************************/

/************************************************************************/
/* For the beginner... 						        */
/* 									*/
/* 1. make linux  (linux, hpux, convex, hpux_no_ansi)			*/
/* 2. type ./iozone -Ra							*/
/* 									*/
/*  Hint: Type make    (it will give you a list of valid targets)	*/
/* 									*/
/************************************************************************/


/* The version number */
#define THISVERSION "        Version $Revision: 3.430 $"

#if defined(linux)
  #define _GNU_SOURCE
#endif
/* Include for Cygnus development environment for Windows */
#if defined (Windows)
#include <windows.h>
#include <errno.h>
#else
#if defined(linux) || defined(solaris) || defined(IOZ_macosx) || defined(__AIX__) || defined(FreeBSD) || defined(_HPUX_SOURCE) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <errno.h>
#else
extern  int errno;   /* imported for errors */
extern  int h_errno; /* imported for errors */
#endif
#endif


#include <sys/types.h>
#include <sys/stat.h>
#if defined (__LP64__) || defined(OSF_64) || defined(__alpha__) || defined(__arch64__) || defined(_LP64) || defined(__s390x__) || defined(__AMD64__)
#define MODE "\tCompiled for 64 bit mode."
#define _64BIT_ARCH_
#else
#define MODE "\tCompiled for 32 bit mode."
#endif

#ifndef NO_THREADS
#include <pthread.h>
#endif

#if defined(HAVE_ANSIC_C) && defined(linux)
#include <stdlib.h>
#include <sys/wait.h>
#endif

#ifdef HAVE_PROTO
#include "proto.h"
#else
int atoi();
int close();
int unlink();
int main();
void record_command_line();
#if !defined(linux)
int wait();
#endif
int fsync();
void srand48();
long lrand48();
void create_list();
void Poll();
void print_header();
void Kill();
long long l_min();
long long l_max();
long long mythread_create();
int gen_new_buf();
void touch_dedup();
void init_by_array64(unsigned long long *, unsigned long long );
unsigned long long genrand64_int64(void);
#endif

#include <fcntl.h>

char *help[] = {
"    Usage: iozone [-s filesize_kB] [-r record_size_kB] [-f [path]filename] [-h]",
"                  [-i test] [-E] [-p] [-a] [-A] [-z] [-Z] [-m] [-M] [-t children]",
"                  [-l min_number_procs] [-u max_number_procs] [-v] [-R] [-x] [-o]",
"                  [-d microseconds] [-F path1 path2...] [-V pattern] [-j stride]",
"                  [-T] [-C] [-B] [-D] [-G] [-I] [-H depth] [-k depth] [-U mount_point]",
"                  [-S cache_size] [-O] [-L cacheline_size] [-K] [-g maxfilesize_kB]",
"                  [-n minfilesize_kB] [-N] [-Q] [-P start_cpu] [-e] [-c] [-b Excel.xls]",
"                  [-J milliseconds] [-X write_telemetry_filename] [-w] [-W]",
"                  [-Y read_telemetry_filename] [-y minrecsize_kB] [-q maxrecsize_kB]",
"                  [-+u] [-+m cluster_filename] [-+d] [-+x multiplier] [-+p # ]",
"                  [-+r] [-+t] [-+X] [-+Z] [-+w percent dedupable] [-+y percent_interior_dedup]",
"                  [-+C percent_dedup_within]",
" ",
"           -a  Auto mode",
"           -A  Auto2 mode",
"           -b Filename  Create Excel worksheet file",
"           -B  Use mmap() files",
"           -c  Include close in the timing calculations",
"           -C  Show bytes transferred by each child in throughput testing",
"           -d #  Microsecond delay out of barrier",
"           -D  Use msync(MS_ASYNC) on mmap files",
"           -e  Include flush (fsync,fflush) in the timing calculations",
"           -E  Run extension tests",
"           -f filename  to use",
"           -F filenames  for each process/thread in throughput test",
"           -g #  Set maximum file size (in kBytes) for auto mode (or #m or #g)",
"           -G  Use msync(MS_SYNC) on mmap files",
"           -h  help",
"           -H #  Use POSIX async I/O with # async operations",
"           -i #  Test to run (0=write/rewrite, 1=read/re-read, 2=random-read/write",
"                 3=Read-backwards, 4=Re-write-record, 5=stride-read, 6=fwrite/re-fwrite",
"                 7=fread/Re-fread, 8=random_mix, 9=pwrite/Re-pwrite, 10=pread/Re-pread",
"                 11=pwritev/Re-pwritev, 12=preadv/Re-preadv)",
"           -I  Use VxFS VX_DIRECT, O_DIRECT,or O_DIRECTIO for all file operations",
"           -j #  Set stride of file accesses to (# * record size)",
"           -J #  milliseconds of compute cycle before each I/O operation",
"           -k #  Use POSIX async I/O (no bcopy) with # async operations",
"           -K  Create jitter in the access pattern for readers",
"           -l #  Lower limit on number of processes to run",
"           -L #  Set processor cache line size to value (in bytes)",
"           -m  Use multiple buffers",
"           -M  Report uname -a output",
"           -n #  Set minimum file size (in kBytes) for auto mode (or #m or #g)",
"           -N  Report results in microseconds per operation",
"           -o  Writes are synch (O_SYNC)",
"           -O  Give results in ops/sec.",
"           -p  Purge on",
"           -P #  Bind processes/threads to processors, starting with this cpu",
"           -q #  Set maximum record size (in kBytes) for auto mode (or #m or #g)",
"           -Q  Create offset/latency files",
"           -r #  record size in Kb",
"              or -r #k .. size in kB",
"              or -r #m .. size in MB",
"              or -r #g .. size in GB",
"           -R  Generate Excel report",
"           -s #  file size in Kb",
"              or -s #k .. size in kB",
"              or -s #m .. size in MB",
"              or -s #g .. size in GB",
"           -S #  Set processor cache size to value (in kBytes)",
"           -t #  Number of threads or processes to use in throughput test",
"           -T  Use POSIX pthreads for throughput tests",
"           -u #  Upper limit on number of processes to run",
"           -U  Mount point to remount between tests",
"           -v  version information",
"           -V #  Verify data pattern write/read",
"           -w  Do not unlink temporary file",
"           -W  Lock file when reading or writing",
"           -x  Turn off stone-walling",
"           -X filename  Write telemetry file. Contains lines with (offset reclen compute_time) in ascii",
"           -y #  Set minimum record size (in kBytes) for auto mode (or #m or #g)",
"           -Y filename  Read  telemetry file. Contains lines with (offset reclen compute_time) in ascii",
"           -z  Used in conjunction with -a to test all possible record sizes",
"           -Z  Enable mixing of mmap I/O and file I/O",
"           -+b #,#  burst size (KB),sleep between burst (mili-second)",
"           -+E Use existing non-Iozone file for read-only testing",
"           -+F Truncate file before write in thread_mix_test",
"           -+J Include think time (-j #) in throughput calculation",
"           -+K Sony special. Manual control of test 8.",
"           -+m  Cluster_filename   Enable Cluster testing",
"           -+d  File I/O diagnostic mode. (To troubleshoot a broken file I/O subsystem)",
"           -+u  Enable CPU utilization output (Experimental)",
"           -+x # Multiplier to use for incrementing file and record sizes",
"           -+p # Percentage of mix to be reads",
"           -+r Enable O_RSYNC|O_SYNC for all testing.",
"           -+t Enable network performance test. Requires -+m ",
"           -+n No retests selected.",
"           -+k Use constant aggregate data set size.",
"           -+q Delay in seconds between tests.",
"           -+l Enable record locking mode.",
"           -+L Enable record locking mode, with shared file.",
"           -+B Sequential mixed workload.",
#if defined(O_DSYNC)
"           -+D Enable O_DSYNC mode.",
#endif
#ifndef NO_MADVISE
"           -+A #  Enable madvise. 0 = normal, 1=random, 2=sequential",
"                                  3=dontneed, 4=willneed",
#endif
"           -+N Do not truncate existing files on sequential writes.",
"           -+S # Dedup-able data is limited to sharing within each numerically",
"                 identified file set.",
"           -+W # Add this value to the child thread ID, so that additional files",
"                 can be added while maintaining the proper dedupability with previously",
"                 existing files that are within the same seed group (-+S).",
"           -+V Enable shared file. No locking.",
#if defined(Windows)
"           -+U Windows Unbufferd I/O API (Very Experimental)",
#endif
"           -+X Enable short circuit mode for filesystem testing ONLY",
"               ALL Results are NOT valid in this mode.",
"           -+Z Enable old data set compatibility mode. WARNING.. Published",
"               hacks may invalidate these results and generate bogus, high",
"               values for results.",
"           -+w ## Percent of dedup-able data in buffers.",
"           -+y ## Percent of dedup-able within & across files in buffers.",
"           -+C ## Percent of dedup-able within & not across files in buffers.",
"           -+H Hostname    Hostname of the PIT server.",
"           -+P Service     Service  of the PIT server.",
"           -+z Enable latency histogram logging.",
"" };

char *head1[] = {
  "       'Iozone' Filesystem Benchmark Program",
  " ",
THISVERSION,
  MODE,
  " ",
  "       Original Author: William Norcott (wnorcott@us.oracle.com)",
  "               4 Dunlap Drive",
  "               Nashua, NH 03060",
  " ",
  "       Enhancements: Don Capps (capps@iozone.org)",
  "        	  7417 Crenshaw",
  "        	  Plano, TX 75025",
  " ",
  "  Copyright 1991, 1992, 1994, 1998, 1999, 2002   William D. Norcott",
  " ",
  "  License to freely use and distribute this software is hereby granted ",
  "  by the author, subject to the condition that this copyright notice ",
  "  remains intact.  The author retains the exclusive right to publish ",
  "  derivative works based on this work, including, but not limited to, ",
  "  revised versions of this work",
  " ",
  "  Other contributors:",
  " ",
  "  Don Capps       (Network Appliance)	capps@iozone.org",
  " ",
  ""};

/******************************************************************

    INCLUDE FILES (system-dependent)

******************************************************************/
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <fcntl.h>
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__APPLE__) && !defined(__DragonFly__)
#include <malloc.h>
#endif
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__DragonFly__)
#include <stdlib.h>
#include <string.h>
#endif

#if defined (__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__APPLE__) || defined(__DragonFly__)
#ifndef O_SYNC
#define O_SYNC O_FSYNC
#endif
#endif

#if defined (__FreeBSD__) || defined(__DSragonFly__)
#ifndef O_RSYNC
#define O_RSYNC O_FSYNC
#endif
#endif

#if ((defined(solaris) && defined(__LP64__)) || defined(__s390x__))
/* If we are building for 64-bit Solaris, all functions that return pointers
 * must be declared before they are used; otherwise the compiler will assume
 * that they return ints and the top 32 bits of the pointer will be lost,
 * causing segmentation faults.  The following includes take care of this.
 * It should be safe to add these for all other OSs too, but we're only
 * doing it for Solaris now in case another OS turns out to be a special case.
 */
#include <strings.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif
#if ( defined(solaris) && defined(studio11) )
#include <strings.h>
#include <stdlib.h>
#endif

#if defined(OSFV5) || defined(linux)
#include <string.h>
#endif

#if defined(linux)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef MAP_FAILED
#define MAP_FAILED -1
#endif

#ifdef generic
typedef long long off64_t;
#endif

#if defined(__DragonFly__)
#define __off64_t_defined
typedef off_t off64_t;
#endif


#ifndef solaris
#ifndef off64_t
#ifndef _OFF64_T
#ifndef __AIX__
#ifndef __off64_t_defined
#ifndef SCO_Unixware_gcc
#ifndef UWIN
#ifndef __DragonFly__
typedef long long off64_t;
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#ifdef __AIX__
#include <fcntl.h>
#endif

#ifdef VXFS
#include <sys/fs/vx_ioctl.h>
#endif

#ifdef unix
#if defined (__APPLE__) || defined(__FreeBSD__) || defined(__DragonFly__) \
   || defined(_SUA_)
#include <sys/time.h>
#endif
#include <sys/times.h>
#include <sys/file.h>
#include <sys/resource.h>
#ifndef NULL
#define NULL 0
#endif

#ifndef nolimits
#include <limits.h>
#endif
#endif

#ifdef HAVE_ANSIC_C
#define VOLATILE volatile
#else 
#define VOLATILE 
#endif

#include <sys/time.h>

#ifdef SHARED_MEM
#include <sys/shm.h>
#endif

#if defined(bsd4_2) && !defined(MS_SYNC)
#define MS_SYNC 0
#define MS_ASYNC 0
#endif

#if defined(bsd4_4) || defined(__DragonFly__)
#define MAP_ANONYMOUS MAP_ANON
#endif

#if defined(SCO_Unixware_gcc) || defined(solaris) || defined(UWIN) || defined(SCO)
#define MAP_FILE (0)
#endif

#if defined(IRIX) || defined(IRIX64) || defined(Windows) || defined(bsd4_2) || defined(bsd4_4) || defined(SCO) || defined(Solaris) || defined(SCO_Unixware_gcc)
long long page_size = 4096;
#define GOT_PAGESIZE 1
#elif defined(NBPG)
long long page_size = NBPG;
#define GOT_PAGESIZE 1
#elif defined(old_linux)
#include <asm/page.h>
long long page_size = PAGE_SIZE;
#define GOT_PAGESIZE 1
#elif !defined(GOT_PAGESIZE)
long long page_size = 4096; /* Used when all else fails */
#endif

#ifdef HAVE_PREAD
#ifdef HAVE_PREADV
#define PVECMAX 16

#ifdef _HPUX_SOURCE
#define PER_VECTOR_OFFSET
#include <sys/puio.h>
struct piovec piov[PVECMAX];
#else
#include <sys/uio.h>
struct iovec piov[PVECMAX];
#define piov_base iov_base
#define piov_len iov_len
#endif

#endif
#endif

#define DEDUPSEED 0x2719362


/*
 * In multi thread/process throughput mode each child keeps track of
 * statistics and communicates them through various flavors of
 * shared memory, and via messages.
 */
struct child_stats {
	long long flag;		/* control space */
	long long flag1;	/* pad */
	float walltime;	        /* child elapsed time */
	float cputime;		/* child CPU time */
	float throughput; 	/* Throughput in either kb/sec or ops/sec */
	float actual;	   	/* Either actual kb read or # of ops performed */
} VOLATILE *child_stat;

/*
 * Used for cpu time statistics.
 */
struct runtime {
	float	walltime;
	float	cputime;
	float	cpuutil;
};

#ifdef __convex_spp
#include <sys/cnx_ail.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


/* 
 * Messages the controlling process sends to children.
 * Internal representation that is arch specific.
 * This is used when using the network distributed mode.
 */
struct client_command {
	char c_host_name[100];
	char c_pit_hostname[40];
	char c_pit_service[8];
	char c_client_name[100];
	char c_working_dir[200];
	char c_file_name[200];
	char c_path_dir[200];
	char c_execute_name[200];
	char c_write_traj_filename[200];
	char c_read_traj_filename[200];
	int c_oflag;
	int c_mfflag;
	int c_unbuffered;
	int c_noretest;
	int c_notruncate;
	int c_read_sync;
	int c_jflag;
	int c_async_flag;
	int c_k_flag;
	int c_h_flag;
	int c_mflag;
	int c_pflag;
	int c_stride_flag;
	int c_verify;
	int c_sverify;
	int c_odsync;
	int c_diag_v;
	int c_dedup;
	int c_dedup_interior;
	int c_dedup_compress;
	int c_dedup_mseed;
	int c_chid_skew;
	int c_hist_summary;
	int c_op_rate;
	int c_op_rate_flag;
	int c_Q_flag;
	int c_inc_think;
	int c_L_flag;
	int c_OPS_flag;
	int c_mmapflag;
	int c_mmapasflag;
	int c_mmapnsflag;
	int c_mmapssflag;
	int c_no_copy_flag;
	int c_include_close;
	int c_include_flush;
	int c_disrupt_flag;
	int c_compute_flag;
	int c_xflag;
	int c_MS_flag;
	int c_mmap_mix;
	int c_Kplus_flag;
	int c_stop_flag;
	int c_w_traj_flag;
	int c_r_traj_flag;
	int c_direct_flag;
	int c_cpuutilflag;
	int c_seq_mix;
	int c_del_flag;
	int c_client_number;
	int c_command;
	int c_testnum;
	int c_no_unlink;
	int c_no_write;
	int c_file_lock;
	int c_rec_lock;
	int c_Kplus_readers;
	int c_multiplier;
	int c_share_file;
	int c_pattern;
	int c_version;
	int c_base_time;
	int c_num_child;
	int c_pct_read;
	int c_advise_op;
	int c_advise_flag;
	int c_restf;
	int c_mygen;
	long long c_stride;
	long long c_rest_val;
	long long c_delay;
	long long c_purge;
	long long c_fetchon;
	long long c_numrecs64;
	long long c_reclen;
	long long c_child_flag;
	long long c_delay_start;
	long long c_depth;
	float c_compute_time;
};	

/*
 * All data in this is in string format for portability in a 
 * hetrogeneous environment.
 *
 * Messages that the master will send to the clients
 * over the socket. This provides neutral format
 * so that heterogeneous clusters will work.
 * This is used when using the network distributed mode.
 * WARNING !!! This data structure MUST not be bigger
 * than 1448 bytes or fragmentation will kick your butt.
 */
struct client_neutral_command {
	char c_host_name[100];
	char c_pit_hostname[40];
	char c_pit_service[8];
	char c_client_name[100];
	char c_working_dir[200];
	char c_file_name[200];
	char c_path_dir[200];
	char c_execute_name[200];
	char c_write_traj_filename[200];
	char c_read_traj_filename[200];
	char c_oflag[2];
	char c_mfflag[2];
	char c_unbuffered[2];
	char c_noretest[2];
	char c_notruncate[2];
	char c_read_sync[2];
	char c_jflag[2];
	char c_async_flag[2];
	char c_k_flag[2];
	char c_h_flag[2];
	char c_mflag[2];
	char c_pflag[2];
	char c_stride_flag[2];
	char c_verify[2];
	char c_sverify[2];
	char c_odsync[2];
	char c_diag_v[2];
	char c_dedup[4];
	char c_dedup_interior[4];
	char c_dedup_compress[4];
	char c_dedup_mseed[4];
	char c_chid_skew[4];
	char c_hist_summary[4];
	char c_op_rate[4];
	char c_op_rate_flag[2];
	char c_Q_flag[2];
	char c_inc_think[2];
	char c_L_flag[2];
	char c_OPS_flag[2];
	char c_mmapflag[2];
	char c_mmapasflag[2];
	char c_mmapnsflag[2];
	char c_mmapssflag[2];
	char c_no_copy_flag[2];
	char c_include_close[2];
	char c_include_flush[2];
	char c_disrupt_flag[2];
	char c_compute_flag[2];
	char c_stop_flag[2];
	char c_xflag[2];
	char c_MS_flag[2];
	char c_mmap_mix[2];
	char c_Kplus_flag[2];
	char c_w_traj_flag[2];		/* small int */
	char c_r_traj_flag[2];		/* small int */
	char c_direct_flag[2]; 		/* small int */
	char c_cpuutilflag[2]; 		/* small int */
	char c_seq_mix[2]; 		/* small int */
	char c_del_flag[2];		/* small int */
	char c_stride[10]; 		/* small long long */
	char c_rest_val[10]; 		/* small long long */
	char c_purge[10]; 		/* very small long long */
	char c_fetchon[10]; 		/* very small long long */
	char c_multiplier[10]; 		/* small int */
	char c_share_file[10];		/* small int */
	char c_file_lock[10]; 		/* small int */
	char c_rec_lock[10]; 		/* small int */
	char c_Kplus_readers[10];	/* small int */
	char c_client_number[20]; 	/* int */
	char c_command[20]; 		/* int */
	char c_testnum[20]; 		/* int */
	char c_no_unlink[4]; 		/* int */
	char c_no_write[4]; 		/* int */
	char c_pattern[20]; 		/* int */
	char c_version[20]; 		/* int */
	char c_base_time[20]; 		/* int */
	char c_num_child[20]; 		/* int */
	char c_pct_read[6]; 		/* small int */
	char c_advise_op[4]; 		/* small int */
	char c_advise_flag[4]; 		/* small int */
	char c_restf[4]; 		/* small int */
	char c_mygen[20]; 		/* long */
	char c_depth[20]; 		/* small long long */
	char c_child_flag[40]; 		/* small long long */
	char c_delay[80]; 		/* long long */
	char c_numrecs64[80]; 		/* long long */
	char c_reclen[80]; 		/* long long */
	char c_delay_start[80]; 	/* long long */
	char c_compute_time[80]; 	/* float */
};	

/* 
 * Messages the clients will send to the master.
 * Internal representation on each client and the master.
 * This is used when using the network distributed mode.
 */
struct master_command {
	char m_host_name[100];
	char m_client_name[100];
	char m_stop_flag; 
	int m_client_number;
	int m_client_error;
	int m_child_port;
	int m_child_async_port;
	int m_command;
	int m_testnum;
	int m_version;
	int m_mygen;
	float m_throughput;
	float m_cputime;
	float m_walltime;
	float m_actual;
	long long m_child_flag;
};	

/*
 * Messages that the clients will send to the master
 * over the socket. This provides neutral format
 * so that heterogeneous clusters will work.
 * This is used when using the network distributed mode.
 */
struct master_neutral_command {
	char m_host_name[100];
	char m_client_name[100];
	char m_client_number[20];	/* int */
	char m_client_error[20];	/* int */
	char m_stop_flag[4];		/* char +space */
	char m_child_port[20];		/* int */
	char m_child_async_port[20];	/* int */
	char m_command[20];		/* int */
	char m_testnum[20];		/* int */
	char m_version[20];		/* int */
	char m_mygen[20];		/* int */
	char m_throughput[80];		/* float */
	char m_cputime[80];		/* float */
	char m_walltime[80];		/* float */
	char m_actual[80];		/* float */
	char m_child_flag[80];		/* long long */
};	


/*
 * Possible values for the commands sent to the master
 */
#define R_CHILD_JOIN        1
#define R_STAT_DATA         2
#define R_FLAG_DATA         3

/*
 * Possible values for the master's commands sent to a client
 *
 * The R_FLAG_DATA is also used by the master to tell the 
 * client to update its flags.
 */
#define R_JOIN_ACK        4
#define R_STOP_FLAG       5
#define R_TERMINATE       6
#define R_DEATH           7


/* These are the defaults for the processor. They can be 
 * over written by the command line options.
 */
#define CACHE_LINE_SIZE 32
#define CACHE_SIZE ( 1024 * 1024 )


#define MEG (1024 * 1024)

/*
 * For stride testing use a prime number to avoid stripe
 * wrap hitting the same spindle.
 */
#define STRIDE 17



/************************************************************************/
/*									*/
/*    DEFINED CONSTANTS							*/
/*									*/
/* Never add a comment to the end of a #define. Some compilers will     */
/* choke and fail the compile.						*/
/************************************************************************/

/*
 * Size of buffer for capturing the machine's name.
 */
#define IBUFSIZE 100
/*
 * How many I/Os before a non-uniform access.
 */
#define DISRUPT 100

/*
 * Set the crossover size. This is where the small transfers
 * are skipped to save time. There is an option to 
 * disable the skipping.
 */
#define LARGE_REC 65536

/* Default number of kilobytes in file */
#define KILOBYTES 512 			

/* Default number of bytes in a record */
#define RECLEN 1024			

/* Default size of file in bytes*/
#define FILESIZE (KILOBYTES*1024)	

/* Default number of records */
#define NUMRECS FILESIZE/RECLEN		

#ifdef __bsdi__
/* At 8 Meg switch to large records */
#define CROSSOVER (8*1024)		
/*maximum buffer size*/
#define MAXBUFFERSIZE (8*1024*1024)		
#else
/* At 16 Meg switch to large records */
#define CROSSOVER (16*1024)		
/* Maximum buffer size*/
#define MAXBUFFERSIZE (16*1024*1024)		
#endif

/* Maximum number of children. Threads/procs/clients */
#define	MAXSTREAMS	256		

/* Minimum buffer size */
#define MINBUFFERSIZE 128
/* If things ran way too fast */
#define TOOFAST 10
/* Set the maximum number of types of tests */
#define MAXTESTS 12
/* Default fill pattern for verification */
#define PATTERN get_pattern();
#define PATTERN1 0xBB
/* Used for Excel internal tables */
#define MAX_X 100			
/* Used for Excel internal tables */
#define MAX_Y 512			

#define USAGE  "\tUsage: For usage information type iozone -h \n\n"


/* Maximum number of characters in filename */
#define MAXNAMESIZE 1000                

/*
 * Define the typical output that the user will see on their
 * screen.
 */
#ifdef NO_PRINT_LLD
#ifdef HAVE_PREAD
#include <sys/times.h>
#if defined(HAVE_PREAD) && defined(HAVE_PREADV)
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s%8s%9s%8s%10s%10s%10s%9s%9s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#else
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s%8s%9s%8s%10s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#endif
#else
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#endif
#endif

#ifndef NO_PRINT_LLD
#ifdef HAVE_PREAD
#include <sys/times.h>
#if defined(HAVE_PREAD) && defined(HAVE_PREADV)
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s%8s%9s%8s%10s%10s%10s%9s%9s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#else
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s%8s%9s%8s%10s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#endif
#else
#define CONTROL_STRING2 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%8s%9s\n"
#define CONTROL_STRING3 "%16s%8s%9s%9s%8s%10s%8s %9s%9s %9s %9s%9s%9s%9s%9s\n"
#define CONTROL_STRING4 "%16s%8s%9s%9s%8s%10s\n"
#endif
#endif

/* 
    For 'auto mode', these defines determine the number of iterations
    to perform for both the file size and the record length.
*/ 

/* Start with 64 kbyte minimum file size by default */
#define KILOBYTES_START       64
/* Default maximum file size. This is 512 MBytes */
#define KILOBYTES_END (1024*512)
/* Default starting record size */
#define RECLEN_START  4096
/* Default maximum record size */
#define RECLEN_END    (MAXBUFFERSIZE)
/* Multiplier for each itteration on file and record size */
#define MULTIPLIER    2

/*
 * Assign numeric values to each of the tests.
 */
#define WRITER_TEST		0
#define READER_TEST		1
#define RANDOM_RW_TEST		2
#define REVERSE_TEST		3
#define REWRITE_REC_TEST	4
#define STRIDE_READ_TEST	5
#define FWRITER_TEST		6
#define FREADER_TEST		7
#define RANDOM_MIX_TEST		8
#ifdef HAVE_PREAD
#define PWRITER_TEST		9
#define PREADER_TEST		10	
#endif /* HAVE_PREAD */
#ifdef HAVE_PREADV
#define PWRITEV_TEST		11
#define PREADV_TEST		12
#endif /* HAVE_PREADV */

#define WRITER_MASK		(1 << WRITER_TEST)
#define READER_MASK		(1 << READER_TEST)
#define RANDOM_RW_MASK		(1 << RANDOM_RW_TEST)
#define RANDOM_MIX_MASK		(1 << RANDOM_MIX_TEST)
#define REVERSE_MASK		(1 << REVERSE_TEST)
#define REWRITE_REC_MASK	(1 << REWRITE_REC_TEST)
#define STRIDE_READ_MASK	(1 << STRIDE_READ_TEST)
#define FWRITER_MASK		(1 << FWRITER_TEST)
#define FREADER_MASK		(1 << FREADER_TEST)
#ifdef HAVE_PREAD
#define PWRITER_MASK		(1 << PWRITER_TEST)
#define PREADER_MASK		(1 << PREADER_TEST)
#endif /* HAVE_PREAD */
#ifdef HAVE_PREADV
#define PWRITEV_MASK		(1 << PWRITEV_TEST)
#define PREADV_MASK		(1 << PREADV_TEST)
#endif /* HAVE_PREADV */

/*
 * child_stat->flag values and transitions
 */
/* Parent initializes children to HOLD */
#define CHILD_STATE_HOLD	0	
/* Child tells master when it's READY */
#define CHILD_STATE_READY	1	
/* Parent tells child to BEGIN */
#define CHILD_STATE_BEGIN	2	
/* Child tells parent that it's DONE */
#define CHILD_STATE_DONE	3	

#define MERSENNE

/******************************************************************/
/*								  */
/*    FUNCTION DECLARATIONS					  */
/*								  */
/******************************************************************/
char *initfile();
/*int pit_gettimeofday( struct timeval *, struct timezone *, char *, char *);*/
int pit_gettimeofday( );
static int openSckt( const char *, const char *, unsigned int );
static void pit( int, struct timeval *);
void mmap_end();
void alloc_pbuf();
void auto_test();		/* perform automatic test series  */
void show_help();		/* show development help          */
static double time_so_far();	/* time since start of program    */
#ifdef unix
static double utime_so_far();	/* user time 			  */
static double stime_so_far();	/* system time   		  */
static double clk_tck();	/* Get clocks/tick		  */
static double cputime_so_far();
#else
#define cputime_so_far()	time_so_far()
#endif
static void update_burst_sleep(int, long long, double *);
static double time_so_far1();	/* time since start of program    */
void get_resolution();
void get_rusage_resolution();
void signal_handler();		/* clean up if user interrupts us */
void begin();			/* The main worker in the app     */
void fetchit();			/* Prime on chip cache		  */
void purgeit();			/* Purge on chip cache		  */
void throughput_test();		/* Multi process throughput 	  */
void multi_throughput_test();	/* Multi process throughput 	  */
void prepage();			/* Pre-fault user buffer	  */
void get_date();
int get_pattern();		/* Set pattern based on version   */
#ifdef HAVE_ANSIC_C
float do_compute(float);	/* compute cycle simulation       */
#else
float do_compute();		/* compute cycle simulation       */
#endif
void write_perf_test();		/* write/rewrite test		  */
void fwrite_perf_test();	/* fwrite/refwrite test		  */
void fread_perf_test();		/* fread/refread test		  */
void read_perf_test();		/* read/reread test		  */
void mix_perf_test();		/* read/reread test		  */
void random_perf_test();	/* random read/write test	  */
void reverse_perf_test();	/* reverse read test		  */
void rewriterec_perf_test();	/* rewrite record test		  */
void read_stride_perf_test();	/* read with stride test	  */
#ifdef HAVE_PREAD
void pread_perf_test();		/* pread/re-pread test		  */
void pwrite_perf_test();	/* pwrite/re-pwrite test	  */
#endif /* HAVE_PREAD */
#ifdef HAVE_PREADV
void preadv_perf_test();	/* preadv/re-preadv test	  */
void pwritev_perf_test();	/* pwritev/re-pwritev test	  */
#endif /* HAVE_PREADV */
void store_dvalue();		/* Store doubles array 		  */
void dump_excel();
void dump_throughput();
int sp_start_child_send();
int sp_start_master_listen();
#ifdef HAVE_ANSIC_C
#if defined (HAVE_PREAD) && defined(_LARGEFILE64_SOURCE)
ssize_t pwrite64(); 
ssize_t pread64(); 
#endif
#if !defined(linux)
char *getenv();
char *inet_ntoa();
int system();
#endif
void my_nap();
void my_unap();
int thread_exit();
#ifdef ASYNC_IO
size_t async_write();
void async_release();
int async_read();
int async_read_no_copy();
size_t async_write_no_copy();
void end_async();
void async_init();
#else
size_t async_write();
size_t async_write_no_copy();
void async_release();
#endif
void do_float();
int create_xls();
void close_xls();
void do_label();
int mylockf(int, int, int);
int mylockr(int,int, int, off64_t, off64_t);
int rand(void);
void srand(unsigned int);
int get_client_info(void);
void exit(int);
void find_remote_shell(char *);
void find_external_mon(char *,char *);
void start_monitor(char *);
void stop_monitor(char *);
void takeoff_cache();
void del_cache();
void fill_area(long long *, long long *, long long);
void fill_buffer(char *,long long ,long long ,char, long long );
void store_value(off64_t);
void store_times(double, double);
static double cpu_util(double, double);
void dump_cputimes(void);
void purge_buffer_cache(void);
char *alloc_mem(long long,int);
void *(thread_rwrite_test)(void *);
void *(thread_write_test)(void *);
void *(thread_fwrite_test)(void *);
void *(thread_fread_test)(void *);
void *(thread_read_test)(void*);
#ifdef HAVE_PREAD
void *(thread_pread_test)(void*);
void *(thread_pwrite_test)(void*);
#endif
void *(thread_cleanup_test)(void*);
void *(thread_cleanup_quick)(void*);
void *(thread_ranread_test)(void *);
void *(thread_mix_test)(void *);
void *(thread_ranwrite_test)(void *);
void *(thread_rread_test)(void *);
void *(thread_reverse_read_test)(void *);
void *(thread_stride_read_test)(void *);
void *(thread_set_base)(void *);
void *(thread_join)(long long, void *);
void disrupt(int);
#if defined(Windows)
void disruptw(HANDLE);
#endif
long long get_traj(FILE *, long long *, float *, long);
void create_temp(off64_t, long long );
FILE *open_w_traj(void);
FILE *open_r_traj(void);
void traj_vers(void);
void r_traj_size(void);
long long w_traj_size(void);
void init_file_sizes();
off64_t get_next_file_size(off64_t);
void add_file_size(off64_t);
void init_file_sizes( off64_t,  off64_t);
off64_t get_next_record_size(off64_t);
void add_record_size(off64_t);
void init_record_sizes( off64_t,  off64_t);
void del_record_sizes( void );
void hist_insert(double );
void dump_hist(char *,int );
void do_speed_check(int);
#else
void do_speed_check();
#if !defined(linux)
char *getenv();
char *inet_ntoa();
int system();
#endif
void my_nap();
void my_unap();
int thread_exit();
void close_xls();
void do_label();
int create_xls();
void do_float();
#ifdef ASYNC_IO
void async_release();
size_t async_write();
size_t async_write_no_copy();
int async_read();
int async_read_no_copy();
#endif
int mylockf();
int mylockr();
int rand();
void srand();
int get_client_info();
void exit();
void find_remote_shell();
void traj_vers();
void r_traj_size();
long long w_traj_size();
FILE *open_w_traj();
FILE *open_r_traj();
void create_temp();
void fill_buffer();
char *alloc_mem();
void *(thread_rwrite_test)();
void *(thread_write_test)();
void *(thread_fwrite_test)();
void *(thread_fread_test)();
void *(thread_read_test)();
void *(thread_cleanup_test)();
void *(thread_ranread_test)();
void *(thread_mix_test)();
void *(thread_ranwrite_test)();
void *(thread_rread_test)();
void *(thread_reverse_read_test)();
void *(thread_stride_read_test)();
void *(thread_set_base)();
void *(thread_join)();
void disrupt();
long long get_traj();
void init_file_sizes();
off64_t get_next_file_size();
void add_file_size();
void init_record_sizes();
off64_t get_next_record_size();
void add_record_size();
void dump_cputimes();
static double cpu_util();
void del_record_sizes();
void hist_insert();
void dump_hist();
#endif

#ifdef _LARGEFILE64_SOURCE
#define I_LSEEK(x,y,z) 	lseek64(x,(off64_t)(y),z)
#define I_OPEN(x,y,z) 	open64(x,(int)(y),(int)(z))
#define I_CREAT(x,y) 	creat64(x,(int)(y))
#define I_FOPEN(x,y) 	fopen64(x,y)
#define I_STAT(x,y) 	stat64(x,y)
#ifdef HAVE_PREAD
#define I_PREAD(a,b,c,d)	pread64(a,b,(size_t)(c),(off64_t)(d))
#define I_PWRITE(a,b,c,d)	pwrite64(a,b,(size_t)(c),(off64_t)(d))
#endif
#define I_MMAP(a,b,c,d,e,f) 	mmap64((void *)(a),(size_t)(b),(int)(c),(int)(d),(int)(e),(off64_t)(f))
#else
#define I_LSEEK(x,y,z) 	lseek(x,(off_t)(y),z)
#define I_OPEN(x,y,z) 	open(x,(int)(y),(int)(z))
#define I_CREAT(x,y) 	creat(x,(int)(y))
#define I_FOPEN(x,y) 	fopen(x,y)
#define I_STAT(x,y) 	stat(x,y)
#ifdef HAVE_PREAD
#define I_PREAD(a,b,c,d)	pread(a,b,(size_t)(c),(off_t)(d))
#define I_PWRITE(a,b,c,d)	pwrite(a,b,(size_t)(c),(off_t)(d))
#endif
#define I_MMAP(a,b,c,d,e,f) 	mmap((void *)(a),(size_t)(b),(int)(c),(int)(d),(int)(e),(off_t)(f))
#endif


/************************************************************************/
/* The list of tests to be called.					*/
/************************************************************************/
void (*func[])() = { 
			write_perf_test, 
			read_perf_test,
			random_perf_test,
			reverse_perf_test,
			rewriterec_perf_test,
			read_stride_perf_test,
			fwrite_perf_test,
			fread_perf_test,
			mix_perf_test 
#ifdef HAVE_PREAD
			,
			pwrite_perf_test,
			pread_perf_test
#ifdef HAVE_PREADV
			,
			pwritev_perf_test,
			preadv_perf_test
#endif /* HAVE_PREADV */
#endif /* HAVE_PREAD */
	};

/*
char *test_output[] = {"                ",
		      "                  ",
		      "                ",
		      "        ",
		      "        ",
		      "        ",
		      "                  ",
		      "                ",
		      "                 ",
		      "                 ",
		      "                   ",
		      "                   \n" };
*/
char *test_output[] = {"                ",
		      "                  ",
		      "                ",
		      "        ",
		      "        ",
		      "        ",
		      "                  ",
		      "                ",
		      "",
		      "                 ",
		      "                   ",
		      "                   ",
		      "                   ",
		      "                   ",
		      "                   ",
		      "                   \n" };
long long test_soutput[] = {2,2,2,1,1,1,2,2,2,2,2,2,2,2};


/******************************************************************/
/*								  */
/*    GLOBAL VARIABLES					          */
/*								  */
/*******************************************************************/

/*
 * Set the size of the shared memory segment for the children
 * to put their results.
 */
#define SHMSIZE  ((( sizeof(struct child_stats) * MAXSTREAMS) )+4096 )
/*
 * Pointer to the shared memory segment.
 */
VOLATILE struct child_stats *shmaddr;
double totaltime,total_time, temp_time ,total_kilos;
off64_t report_array[MAX_X][MAX_Y];
double report_darray[MAX_X][MAXSTREAMS];
double time_res,cputime_res;
long long throughput_array[MAX_X];	/* Filesize & record size are constants */
short current_x, current_y;
long long orig_size;
long long max_x, max_y;
unsigned long long goodkilos;
off64_t kilobytes64 = (off64_t)KILOBYTES;
long long goodrecl;
off64_t offset = 0;               /*offset for random I/O */
off64_t offset64 = 0;               /*offset for random I/O */
off64_t filebytes64;
off64_t r_range[100];
off64_t s_range[100];
int t_range[100];
int t_count = 0;
int r_count,s_count;
char *barray[MAXSTREAMS];
char *haveshm;
extern int optind;
long long onetime, auto_mode, sfd, multi_buffer;
int exit_code = 0;  
int fd;
int sp_msfd,sp_mrfd,sp_csfd,sp_crfd;
int begin_proc,num_processors,ioz_processor_bind;
long long res_prob,rec_prob;
char silent,read_sync;
char master_iozone, client_iozone,distributed;
int bif_fd,s_count;
int bif_row,bif_column;
int dedup_mseed = 1;
int chid_skew = 0;
int hist_summary;
int op_rate;
int op_rate_flag;
char aflag, Eflag, hflag, Rflag, rflag, sflag,del_flag,mix_test;
char diag_v,sent_stop,dedup,dedup_interior,dedup_compress;
char *dedup_ibuf;
char *dedup_temp;
char bif_flag;
int rlocking;
int share_file;
int ecount;
char gflag,nflag;
char yflag,qflag;
#ifdef Windows
char *build_name = "Windows";
#else
char *build_name = NAME;
#endif
char imon_start[256],imon_stop[256]; 
char imon_sync;
char trflag; 
char cpuutilflag;
char seq_mix;
long base_time;
long long mint, maxt; 
long long w_traj_ops, r_traj_ops, w_traj_fsize,r_traj_fsize;
long long r_traj_ops_completed,r_traj_bytes_completed;
long long w_traj_ops_completed,w_traj_bytes_completed;
int w_traj_items, r_traj_items;
char fflag, Uflag,uflag,lflag,include_tflag; 
struct runtime runtimes [MAX_X] [MAX_Y];	/* in parallel with report_array[][] */
long long include_test[50];
long long include_mask;
char RWONLYflag, NOCROSSflag;		/*auto mode 2 - kcollins 8-21-96*/
char mfflag;
long long status, x, y, childids[MAXSTREAMS+1], myid, num_child;
int pct_read,speed_code;
#ifndef NO_THREADS
pthread_t p_childids[MAXSTREAMS+1];
#endif
off64_t next64;
char wol_opened, rol_opened;
FILE *wqfd,*rwqfd,*rqfd,*rrqfd;

extern char *optarg;
#ifndef __AIX__
long long ret;
#else
short ret;
#endif
struct size_entry {
	struct size_entry *next;
	off64_t size;
};
struct size_entry *size_list=0;
struct size_entry *rec_size_list=0;
off64_t maximum_file_size;
off64_t minimum_file_size;

off64_t burst_size_kb_64 = -1;             /* the size of the burst (in KBytes) */
long long burst_sleep_duration_msec = -1;  /* the sleep duration between burst */
long long written_this_burst = 0;          /* How much data was writen so far in this burst*/

char bif_filename [MAXNAMESIZE];           /* name of biff file      */
char filename [MAXNAMESIZE];               /* name of temporary file */
char mountname [MAXNAMESIZE];              /* name of device         */
char dummyfile [MAXSTREAMS][MAXNAMESIZE];  /* name of dummy file     */
char dummyfile1 [MAXNAMESIZE];             /* name of dummy file     */
char *filearray[MAXSTREAMS];		   /* array of file names    */
char tfile[MAXNAMESIZE];
char *buffer,*buffer1, *mbuffer,*mainbuffer;
FILE *pi,*r_traj_fd,*w_traj_fd;
VOLATILE char *pbuffer;
char *default_filename="iozone.tmp"; /*default name of temporary file*/
VOLATILE char stoptime;
char Cflag;
char use_thread = 0;
long long debug1=0;		
long long debug=0;
unsigned long cache_size=CACHE_SIZE;
unsigned long cache_line_size=CACHE_LINE_SIZE;
long long *pstatus;
off64_t min_file_size = KILOBYTES_START;
off64_t max_file_size = KILOBYTES_END;
long long min_rec_size = RECLEN_START;
long long max_rec_size = RECLEN_END;
long long orig_min_rec_size = RECLEN_START;
long long orig_max_rec_size = RECLEN_END;
long long xover = CROSSOVER;
char *throughput_tests[] = {"Initial write","Rewrite","Read","Re-read",
	"Reverse Read","Stride read","Random read","Mixed workload","Random write","Pwrite","Pread","Fwrite","Fread"};
char command_line[1024] = "\0";
#ifdef unix
double sc_clk_tck;
#endif

int argcsave;
char **argvsave;
char splash[80][80];
int splash_line;
char client_filename[256];
char remote_shell[256];
int client_error;

char pit_hostname[40];
char pit_service[8];
int junk;

/* 
 * Host ports used to listen, and handle errors.
 */
#define HOST_LIST_PORT 20000
#define HOST_ESEND_PORT (HOST_LIST_PORT+MAXSTREAMS)
#define HOST_ASEND_PORT (HOST_ESEND_PORT+MAXSTREAMS)
int controlling_host_port = HOST_LIST_PORT;

/* 
 * Childs ports used to listen, and handle errors.
 */
#define CHILD_ESEND_PORT (HOST_ASEND_PORT+MAXSTREAMS)
#define CHILD_LIST_PORT (CHILD_ESEND_PORT+MAXSTREAMS)

/* Childs async message port. Used for stop flag and terminate */
#define CHILD_ALIST_PORT (CHILD_LIST_PORT+MAXSTREAMS)

/* Ports for the network speed code */
#define SP_CHILD_LISTEN_PORT 31000
#define SP_CHILD_ESEND_PORT (SP_CHILD_LISTEN_PORT+10)
#define SP_MASTER_LISTEN_PORT (SP_CHILD_ESEND_PORT+10)
#define SP_MASTER_ESEND_PORT (SP_MASTER_LISTEN_PORT+10)
#define SP_MASTER_RESULTS_PORT (SP_MASTER_ESEND_PORT+10)


#define THREAD_WRITE_TEST 1
#define THREAD_REWRITE_TEST 2
#define THREAD_READ_TEST 3
#define THREAD_REREAD_TEST 4
#define THREAD_STRIDE_TEST 5
#define THREAD_RANDOM_READ_TEST 6
#define THREAD_RANDOM_WRITE_TEST 7
#define THREAD_REVERSE_READ_TEST 8
#define THREAD_RANDOM_MIX_TEST 9
#define THREAD_PWRITE_TEST 10
#define THREAD_PREAD_TEST 11
#define THREAD_FWRITE_TEST 12
#define THREAD_FREAD_TEST 13
#define THREAD_CLEANUP_TEST 14

/*
 * Child states that the master is tracking.
 * The master uses these to determine how to shutdown
 * the clients when some fool hits control-C.
 */
#define C_STATE_ZERO 1
#define C_STATE_WAIT_WHO 2
#define C_STATE_WAIT_BARRIER 3


int c_port,a_port; /* port number */
int child_port; /* Virtualized due to fork */
int child_async_port; /* Virtualized due to fork */
int client_listen_pid; /* Virtualized due to fork */
int master_join_count; /* How many children have joined */
int l_sock,l_async_sock; /* Sockets for listening */
char master_rcv_buf[4096]; /* Master's receive buffer */
int master_listen_pid; /* Pid of the master's async listener proc */
char master_send_buf[4096]; /* Master's send buffer */
char child_rcv_buf[4096]; /* Child's receive buffer */
char child_async_rcv_buf[4096]; /* Child's async recieve buffer */
char child_send_buf[4096]; /* Child's send buffer */
int child_send_socket; /* Child's send socket */
int child_listen_socket; /* Child's listener socket */
int child_listen_socket_async; /* Child's async listener socket */
int master_send_socket; /* Needs to be an array. One for each child*/
int master_send_sockets[MAXSTREAMS]; /* Needs to be an array. One for each child*/
int master_send_async_sockets[MAXSTREAMS]; /* Needs to be an array. One for each child*/
int master_listen_port; /* Master's listener port number */
int master_listen_socket; /* Master's listener socket */
int clients_found; /* Number of clients found in the client file */
FILE *newstdin, *newstdout, *newstderr; /* used for debug in cluster mode.*/
char toutput[20][20]; /* Used to help format the output */
int toutputindex; /* Index to the current output line */
int cdebug = 0; /* Use to turn on child/client debugging in cluster mode. */
int mdebug = 0; /* Use to turn on master debug in cluster mode */
int aggflag; /* Used to indicate constant aggregate data set size */
struct sockaddr_in child_sync_sock, child_async_sock;

/*
 * Change this whenever you change the message format of master or client.
 */
int proto_version = 25;

/******************************************************************************/
/* Tele-port zone. These variables are updated on the clients when one is     */
/* using cluster mode. (-+m)                                                  */
/* Do not touch these unless you have become one with the universe !!         */
/******************************************************************************/
char controlling_host_name[100];
struct child_ident {
	char child_name[100];
	char workdir[200];
	char execute_path[200];
	char file_name[200];
	int state;
	int child_number;
	int child_port;
	int child_async_port;
	int master_socket_num;
	int master_async_socket_num;
}child_idents[MAXSTREAMS];
int Kplus_readers;
char write_traj_filename [MAXNAMESIZE];     /* name of write telemetry file */
char read_traj_filename [MAXNAMESIZE];    /* name of read telemetry file  */
char oflag,jflag,k_flag,h_flag,mflag,pflag,unbuffered,Kplus_flag;
char noretest;
char notruncate;   /* turn off truncation of files */
char async_flag,stride_flag,mmapflag,mmapasflag,mmapssflag,mmapnsflag,mmap_mix;
char verify = 1;
int restf;
char sverify = 1;
char odsync = 0;
char inc_think, Q_flag,OPS_flag;
char L_flag=0;
char no_copy_flag,include_close,include_flush;
char disrupt_flag,compute_flag,xflag,Z_flag, X_flag;
int no_unlink = 0;
int no_write = 0;
int r_traj_flag,w_traj_flag;
int mygen;
char MS_flag;
int advise_op,advise_flag;
int direct_flag;
int current_client_number;
long long chid;
int file_lock;
unsigned int pattern;
long long stride = STRIDE;
long long delay,purge,fetchon;
off64_t  numrecs64 = (off64_t)NUMRECS;
long long reclen = RECLEN;
long long delay_start,depth;
VOLATILE char *stop_flag;		/* Used to stop all children */
float compute_time;
int multiplier = MULTIPLIER;
long long rest_val;
#if defined(Windows)
	HANDLE hand;
#endif

/******************************************************************************/
/* End of Tele-port zone.                                                     */
/******************************************************************************/


/* 
 * Prototypes
 * Sort of... Full prototypes break non-ansi C compilers. No protos is 
 * a bit sloppy, so the compromise is this.
 */
void child_send();
int start_child_listen();
int start_child_listen_async();
void start_child_listen_loop();
void child_listen();
void child_listen_async();
void stop_child_send();
void stop_child_listen();
void cleanup_comm();
void master_send();
int start_master_send();
int start_master_listen();
int check_filename();
void master_listen();
void stop_master_send();
void stop_master_listen();
long long start_child_proc();
int parse_client_line();
void wait_dist_join();
void tell_children_begin();
void start_master_listen_loop();
void wait_for_master_go();
void tell_master_ready();
void stop_master_listen_loop();
void tell_master_stats();
void become_client();
int pick_client();
long long start_child_proc();
int start_master_send();
void child_listen();
int start_child_listen();
void stop_master_send();
void stop_master_listen();
void stop_child_send();
void stop_child_listen();
void master_send();
void child_send();
void master_listen();
int start_master_listen();
void child_remove_files();
void terminate_child_async();
void distribute_stop();
void send_stop();
void cleanup_children();


/****************************************************************/
/*								*/
/*   MAIN () 							*/
/*								*/
/****************************************************************/

int
main(argc,argv) 
int argc;
char **argv;
{

	long long fileindx,i,tval;
	long long ind;
	int ret;
	FILE *pi;
	char reply[IBUFSIZE];
	unsigned char inp_pat;
	time_t time_run;
	char *port,*m,*subarg;
	int num_child1;
	int cret;
	int anwser,bind_cpu;
	char *evalue;


	strcpy(tfile,"iozone"); /* Dummy name prefix */
	anwser=bind_cpu=0;
	/* Used to make fread/fwrite do something better than their defaults */
	setvbuf( stdout, NULL, _IONBF, (size_t) NULL );
	setvbuf( stderr, NULL, _IONBF, (size_t) NULL );
	
	/* Save the master's name */
	gethostname(controlling_host_name,100);

	/* Let user activate mdebug or cdebug via environmental variables */
	evalue = (char *)NULL;
	evalue=(char *)getenv("CDEBUG");
	if(evalue)
		cdebug=atoi(evalue);
	evalue = (char *)NULL;
	evalue=(char *)getenv("MDEBUG");
	if(evalue)
		mdebug=atoi(evalue);

	srand(time(0));
	mygen=rand(); /* Pick a random generation number */

	/* Try to find the actual VM page size, if possible */
#if defined (solaris) || defined (_HPUX_SOURCE) || defined (linux) || defined(IRIX) || defined (IRIX64)
#ifndef __convex_spp
	page_size=getpagesize();
#endif
#endif
	/* Try to find the actual number of ticks per second */
#ifdef unix
	sc_clk_tck = clk_tck();
#endif
	for(ind=0;ind<MAXSTREAMS;ind++)
		filearray[ind]=(char *)tfile;

	/* base_time=(long)time_so_far(); */
	myid=(long long)getpid(); 	/* save the master's PID */
	/* get_resolution(); 		 Get clock resolution */
	time_run = time(0);		/* Start a timer */
	(void)find_external_mon(imon_start, imon_stop);

	/*
 	 * Save the splash screen for later display. When in distributed network
	 * mode this output does not get displayed on the clients.
	 */
	sprintf(splash[splash_line++],"\tIozone: Performance Test of File I/O\n");
    	sprintf(splash[splash_line++],"\t%s\n\t%s\n", THISVERSION,MODE);
    	sprintf(splash[splash_line++],"\t\tBuild: %s \n\n",build_name);
    	sprintf(splash[splash_line++],"\tContributors:William Norcott, Don Capps, Isom Crawford, Kirby Collins\n");
	sprintf(splash[splash_line++],"\t             Al Slater, Scott Rhine, Mike Wisner, Ken Goss\n");
    	sprintf(splash[splash_line++],"\t             Steve Landherr, Brad Smith, Mark Kelly, Dr. Alain CYR,\n");
    	sprintf(splash[splash_line++],"\t             Randy Dunlap, Mark Montague, Dan Million, Gavin Brebner,\n");
    	sprintf(splash[splash_line++],"\t             Jean-Marc Zucconi, Jeff Blomberg, Benny Halevy, Dave Boone,\n");
    	sprintf(splash[splash_line++],"\t             Erik Habbinga, Kris Strecker, Walter Wong, Joshua Root,\n");
    	sprintf(splash[splash_line++],"\t             Fabrice Bacchella, Zhenghua Xue, Qin Li, Darren Sawyer,\n");
    	sprintf(splash[splash_line++],"\t             Vangel Bojaxhi, Ben England, Vikentsi Lapa.\n\n");
	sprintf(splash[splash_line++],"\tRun began: %s\n",ctime(&time_run));
	argcsave=argc;
	argvsave=argv;

    	signal(SIGINT, signal_handler);	 	/* handle user interrupt */
    	signal(SIGTERM, signal_handler);	/* handle kill from shell */

        /********************************************************/
        /* Allocate and align buffer with beginning of the 	*/
        /* on chip data cache. 					*/
        /********************************************************/

     	buffer = (char *) alloc_mem((long long)(MAXBUFFERSIZE + (2 * cache_size)),(int)0);
	if(buffer == 0) {
        	perror("Memory allocation failed:");
        	exit(1);
        }

#ifdef _64BIT_ARCH_
     	buffer = (char *) ((long long )(buffer + cache_size ) & 
		~(cache_size-1));
#else
     	buffer = (char *) ((long)(buffer + cache_size ) & 
		~((long)cache_size-1));
#endif
	mainbuffer = buffer;

	/* de-dup input buf */
     	buffer1 = (char *) alloc_mem((long long)(MAXBUFFERSIZE + (2 * cache_size)),(int)0);
	if(buffer1 == 0) {
        	perror("Memory allocation failed:");
        	exit(1);
        }

#ifdef _64BIT_ARCH_
     	buffer1 = (char *) ((long long )(buffer1 + cache_size ) & 
		~(cache_size-1));
#else
     	buffer1 = (char *) ((long)(buffer1 + cache_size ) & 
		~((long)cache_size-1));
#endif
	dedup_ibuf = buffer1;
	touch_dedup(buffer1, MAXBUFFERSIZE);

#ifdef FOOB
	/* de-dup temp buf */
     	buffer1 = (char *) alloc_mem((long long)(MAXBUFFERSIZE + (2 * cache_size)),(int)0);
	if(buffer1 == 0) {
        	perror("Memory allocation failed:");
        	exit(1);
        }

#ifdef _64BIT_ARCH_
     	buffer1 = (char *) ((long long )(buffer1 + cache_size ) & 
		~(cache_size-1));
#else
     	buffer1 = (char *) ((long)(buffer1 + cache_size ) & 
		~((long)cache_size-1));
#endif
#endif
	dedup_temp = mainbuffer;

	fetchon++;  /* By default, prefetch the CPU cache lines associated with the buffer */
  	strcpy(filename,default_filename); 	/* Init default filename */
  	sprintf(dummyfile[0],"%s.DUMMY",default_filename);
	if(argc <=1){
		printf(USAGE);
		exit(255);
	}
	auto_mode = 0; 		/* Default is to disable auto mode */
	inp_pat = PATTERN; 	/* Init default pattern for verification */
	/* Fill the entire pattern variable with the same character */
	pattern = ((inp_pat << 24) | (inp_pat << 16) | (inp_pat << 8) | inp_pat);

	/*
	 * Parse all of the options that the user specified.
	 */
	while((cret = getopt(argc,argv,"ZQNIBDGCTOMREWovAxamwphcezKJ:j:k:V:r:t:s:f:F:d:l:u:U:S:L:H:+:P:i:b:X:Y:g:n:y:q: ")) != EOF){
		switch(cret){
		case 'k':	/* Async I/O with no bcopys */
			depth = (long long)(atoi(optarg));
			if(depth <0)
				depth=0;
			/*
			if(depth > 60)
				depth=60;
			*/
#ifdef NO_PRINT_LLD
			sprintf(splash[splash_line++],"\tPOSIX Async I/O (no bcopy). Depth %ld \n",depth);
#else
			sprintf(splash[splash_line++],"\tPOSIX Async I/O (no bcopy). Depth %lld \n",(long long)depth);
#endif
			no_copy_flag=1;
			async_flag++;
			k_flag++;
			break;
		case 'T':	/* Switch to POSIX thread based */
#ifndef NO_THREADS
			use_thread++;
#else
			printf("\tThreads not supported in this version\n");
			exit(2);
#endif
			break;
		case 'H':	/* Use POSIX async_io */
			h_flag++;
			depth = (long long)(atoi(optarg));
			if(depth <0)
				depth=0;
			/*
			 * Hmmm. many systems fail is strange ways when the maximum
			 * number of async I/Os per user or proc is exceeded.
			 */
			/*
			if(depth > 60)
				depth=60;
			*/
#ifdef NO_PRINT_LLD
			sprintf(splash[splash_line++],"\tPOSIX async I/O (with bcopy). Depth %ld\n",depth);
#else
			sprintf(splash[splash_line++],"\tPOSIX async I/O (with bcopy). Depth %lld\n",(long long)depth);
#endif
			async_flag++;
			break;
		case 'I':	/* Use VXFS direct advisory or O_DIRECT from Linux or AIX , or O_DIRECTIO for TRU64  or Solaris directio */
#ifdef VXFS
			direct_flag++;
			sprintf(splash[splash_line++],"\tVxFS advanced feature SET_CACHE, VX_DIRECT enabled\n");
			break;
#endif
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined(__FreeBSD__) || defined(solaris)
			direct_flag++;
			sprintf(splash[splash_line++],"\tO_DIRECT feature enabled\n");
			break;
#endif
#if defined(TRU64)
			direct_flag++;
			sprintf(splash[splash_line++],"\tO_DIRECTIO feature enabled\n");
			break;
#endif
#else
			break;
#endif
#if defined(Windows)
			sprintf(splash[splash_line++],"\tO_DIRECTIO feature not available in Windows version.\n");
			break;
#endif
		case 'B':	/* Use mmap file for test file */
			sprintf(splash[splash_line++],"\tUsing mmap files\n");
			mmapflag++;
			mmapnsflag++;
			break;
		case 'D':	/* Use async msync mmap file */
			sprintf(splash[splash_line++],"\tUsing msync(MS_ASYNC) on mmap files\n");
			mmapflag++;
			mmapasflag++;
			mmapnsflag=0;
			break;
		case 'G':	/* Use msync sync for mmap file */
			sprintf(splash[splash_line++],"\tUsing msync(MS_SYNC) on mmap files\n");
			mmapssflag++;
			mmapnsflag=0;
			break;
		case 'C':	/* show children xfer counts */
			Cflag++;
			break;
		case 'Q':	/* Enable output offset/latency files */
	    		sprintf(splash[splash_line++],"\tOffset/latency files enabled.\n");
			Q_flag++;
			break;
		case 'x':	/* Disable stone_wall */
	    		sprintf(splash[splash_line++],"\tStonewall disabled\n");
			xflag++;
			break;

		case 'a':	/* auto mode */
			fetchon=1;
			purge=0;
			multi_buffer=0;
	    		auto_mode = 1;
			aflag++;
	    		sprintf(splash[splash_line++],"\tAuto Mode\n");
			break;
		case 'c':	/* Include close in timing */
			include_close++;
	    		sprintf(splash[splash_line++],"\tInclude close in write timing\n");
			break;
		case 'e':	/* Include fsync in timing */
			include_flush++;
	    		sprintf(splash[splash_line++],"\tInclude fsync in write timing\n");
			break;
		case 'A':	/* auto2 mode. Soon to go away. Please use -az */
			fetchon=1;
			purge=0;
			multi_buffer=0;
	    		auto_mode = 1;
			aflag++;
	    		sprintf(splash[splash_line++],"\tAuto Mode 2. This option is obsolete. Use -az -i0 -i1 \n");
			RWONLYflag++;
			NOCROSSflag++;
			include_tflag++;	/* automatically set WRITER_TEST and READER_TEST */
			include_test[WRITER_TEST]++;
			include_test[READER_TEST]++;
			break;
		case 's': 	/* Set file size */
#ifdef NO_PRINT_LLD
			sscanf(optarg,"%ld",&kilobytes64);
#else
			sscanf(optarg,"%lld",&kilobytes64);
#endif
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				;
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				kilobytes64 = kilobytes64 * 1024;
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				kilobytes64 = kilobytes64 *1024 * 1024;
			}
			if(kilobytes64 <= 0)
				kilobytes64=512;

			s_range[s_count++]=kilobytes64;
			max_file_size = (off64_t)s_range[s_count-1];   /* Make visable globally */
			min_file_size = (off64_t)s_range[0];   /* Make visable globally */

#ifdef NO_PRINT_LLD
	    		sprintf(splash[splash_line++],"\tFile size set to %ld kB\n",kilobytes64);
#else
	    		sprintf(splash[splash_line++],"\tFile size set to %lld kB\n",kilobytes64);
#endif
			sflag++;
			break;
		case 'l': 	/* Set lower thread/proc limit  */
			mint = (long long)(atoi(optarg));
			if(mint <= 0)
			{
				mint=1;
				num_child=1;
			}else
				num_child=mint;
			if(mint > (unsigned long long)MAXSTREAMS){
			  printf("Invalid options: maximum streams for ");
			  printf("throughput is MAXSTREAMS\n");
			  exit(4);
			}
			lflag++;
			trflag++;
			if(Uflag)
			{
				printf("Can not run throughput tests with unmount & remounts.\n");
				exit(5);
			}
			if (burst_size_kb_64 != -1)
			{
				printf("Can not run throughput tests with the burst option.\n");
				exit(5);
			}
			break;
		case 'u': 	/* Set upper thread/proc limit  */
			maxt = (long long)(atoi(optarg));
			if(maxt <= 0)
				maxt=1;
			if(maxt > MAXSTREAMS){
			  printf("Invalid options: maximum streams for ");
			  printf("throughput is MAXSTREAMS\n");
			  exit(6);
			}
			uflag++;
			trflag++;
			if(Uflag)
			{
				printf("Can not run throughput tests with unmount & remounts.\n");
				exit(7);
			}
			if (burst_size_kb_64 != -1)
			{
				printf("Can not run throughput tests with the burst option.\n");
				exit(7);
			}
			break;
		case 'm':	/* Use multiple buffers */
			fetchon=0;
			multi_buffer=1;
			mflag++;
     			mbuffer = (char *) alloc_mem((long long)MAXBUFFERSIZE,(int)0);
			if(mbuffer == 0) {
                        	perror("Memory allocation failed:");
                          	exit(8);
			}
	    		sprintf(splash[splash_line++],"\tMulti_buffer. Work area %d bytes\n",
				MAXBUFFERSIZE);
			break;
                case 'M':       /* Report machine name and OS */
			bzero(reply,sizeof(reply));
                        pi=popen("uname -a", "r");
			if(pi == (FILE *)0)
			{
				sprintf(splash[splash_line++],"\n\tError using popen() on uname\n");
				sprintf(splash[splash_line++],"\t-M option suppressed.\n");
			}
			else
			{
                        	junk=fread(reply,IBUFSIZE-1,1,pi);
                        	pclose(pi);
				m=reply;
                        	while(*m) /* Strip new line */
				{
					if(*m=='\n')
                               		 	*m=0;
					else	
                               		 	m++;
				}
                        	sprintf(splash[splash_line++],"\n\tMachine = %s\n",reply);
			}
                        break;

		case 'P':	/* Set beginning processor for binding. */
#ifndef NO_THREADS
#if defined(_HPUX_SOURCE) || defined(linux)
#if defined(_HPUX_SOURCE)
			num_processors= pthread_num_processors_np();
#else
      num_processors = sysconf(_SC_NPROCESSORS_ONLN);
#endif      
			begin_proc = atoi(optarg);
			if(begin_proc < 0)
				begin_proc=0;
			if(begin_proc > num_processors)
				begin_proc=0;
                        sprintf(splash[splash_line++],"\tBinding of processors beginning with %d \n",begin_proc);
			ioz_processor_bind++;
#else
			sprintf(splash[splash_line++],"\tProcessor binding not available in this version\n");
#endif
#endif
                        break;
		case 'p':	/* purge the processor cache */
	    		sprintf(splash[splash_line++],"\tPurge Mode On\n");
			fetchon=0;
			pflag++;
			purge=1;
			break;
		case 'h':	/* show help */
			hflag++;
	    		show_help();
			exit(0);
			break;
		case 'E':	/* Extended testing for pread/pwrite... */
			Eflag++;
			break;
		case 'R':	/* Generate Excel compatible Report */
			Rflag++;
	    		sprintf(splash[splash_line++],"\tExcel chart generation enabled\n");
			break;
		case 'o':	/* Open OSYNC */
	    		sprintf(splash[splash_line++],"\tSYNC Mode. \n");
			oflag++;
			break;
		case 'O':	/* Report in Ops/sec instead of kB/sec */
	    		sprintf(splash[splash_line++],"\tOPS Mode. Output is in operations per second.\n");
			OPS_flag++;
			break;
		case 'N':	/* Report in usec/op  */
	    		sprintf(splash[splash_line++],"\tMicroseconds/op Mode. Output is in microseconds per operation.\n");
			MS_flag++;
			break;
		case 'V':	/* Turn on Verify every byte */
			sverify=0;
			inp_pat = (char)(atoi(optarg));
			if(inp_pat == 0)
				inp_pat = PATTERN;
			pattern = ((inp_pat << 24) | (inp_pat << 16) | (inp_pat << 8) 
				| inp_pat);
			verify=1;
	    		sprintf(splash[splash_line++],"\tVerify Mode. Pattern %x\n",pattern);
    			sprintf(splash[splash_line++],"\tPerformance measurements are invalid in this mode.\n");
			break;
		case 'S':	/* Set the processor cache size */
			cache_size = (long)(atoi(optarg)*1024);
			if(cache_size == 0)
				cache_size = CACHE_SIZE;
			break;
		case 'L':	/* Set processor cache line size */
			cache_line_size = (long)(atoi(optarg));
			if(cache_line_size == 0)
				cache_line_size = CACHE_LINE_SIZE;
			break;
		case 'f':	/* Specify the file name */
			if(mfflag) {
			  printf("invalid options: -f and -F are mutually exclusive\n");
			  exit(10);
			}
			fflag++;
			strcpy(filename,optarg);
	  		sprintf(dummyfile[0],"%s.DUMMY",optarg);
			break;
		case 'b':	/* Specify the biff file name */
			Rflag++;
			bif_flag++;
			strcpy(bif_filename,optarg);
			break;
		case 'F':	/* Specify multiple file names for -t */
	 		mfflag++;
			if(fflag) {
			  printf("invalid options: -f and -F are mutually exclusive\n");
			  exit(11);
			}
			if(!trflag) {
			  printf("invalid options: must specify -t N before -F\n");
			  exit(12);
			}
			optind--;
			for(fileindx=0;fileindx<maxt;fileindx++) {
			  filearray[fileindx]=argv[optind++];
			  if(optind > argc) {
#ifdef NO_PRINT_LLD
			    printf("invalid options: not enough filenames for %ld streams\n",num_child);
#else
			    printf("invalid options: not enough filenames for %lld streams\n",num_child);
#endif
			    exit(13);
			  }
			}
			break;
		case 'r':	/* Specify the record size to use */
			rflag++;
			reclen = ((long long)(atoi(optarg))*1024);
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				reclen = (long long)(1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				reclen = (long long)(1024 * 1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				reclen = (long long)(1024 * 1024 * 1024 *(long long)atoi(optarg));
			}
			if(reclen <= 0)
				reclen=(long long)4096;

			r_range[r_count++]=reclen;
			max_rec_size = (off64_t)r_range[r_count-1];   /* Make visable globally */
			min_rec_size = (off64_t)r_range[0];   /* Make visable globally */
#ifdef NO_PRINT_LLD
	    		sprintf(splash[splash_line++],"\tRecord Size %ld kB\n",reclen/1024);
#else
	    		sprintf(splash[splash_line++],"\tRecord Size %lld kB\n",reclen/1024);
#endif
			if(max_rec_size > MAXBUFFERSIZE) {
#ifdef NO_PRINT_LLD
				printf("Error: maximum record size %ld kB is greater than maximum buffer size %ld kB\n ",
					max_rec_size/1024, MAXBUFFERSIZE/1024);
#else
				printf("Error: maximum record size %lld kB is greater than maximum buffer size %lld kB\n ",
					(long long)(max_rec_size/1024LL), (long long)MAXBUFFERSIZE/1024LL);
#endif
				exit(23);
			}
			break;
		case 'J':	/* Specify the compute time in millisecs */
			compute_time = (float)(atoi(optarg));
			compute_time=compute_time/1000;	
			if(compute_time < (float)0)
				compute_time=(float)0;
			else
				compute_flag=1;
			jflag++;
			break;
		case 'j':	/* Specify the stride in records */
			stride = (long long)(atoi(optarg));
			if(stride < 0)
				stride=0;
			stride_flag=1;
			break;
		case 't':	/* Specify the number of children to run */
			num_child1=(atoi(optarg));
			num_child = (long long)num_child1;
			if(num_child > (long long)MAXSTREAMS) {
			  printf("invalid options: maximum streams for throughput is MAXSTREAMS\n");
#ifdef NO_PRINT_LLD
			  printf("Numchild %ld %s\n",num_child,optarg);
#else
			  printf("Numchild %lld %s\n",num_child,optarg);
#endif
			  exit(14);
			}
			if(num_child <= 0)
				num_child = 8;
			if(num_child == 0)
				num_child=1;
                        t_range[t_count++]=num_child;
                        maxt = (maxt>num_child?maxt:num_child);
			trflag++;
			if(Uflag)
			{
				printf("Can not run throughput tests with unmount & remounts.\n");
				exit(15);
			}
			if (burst_size_kb_64 != -1)
			{
				printf("Can not run throughput tests with the burst option.\n");
				exit(15);
			}
			break;
		case 'd':	/* Specify the delay of children to run */
			delay_start = (long long)(atoi(optarg));
			if(delay_start < 0)
				delay_start=0;
			break;
		case 'i':	/* Specify specific tests */
			tval=(long long)(atoi(optarg));
			if(tval < 0) tval=0;
#ifndef HAVE_PREAD
			if(tval > RANDOM_MIX_TEST)
			{
				printf("\tPread tests not available on this operating system.\n");
				exit(183);
			}
#endif
			if(tval > sizeof(func)/sizeof(char *)) 
			{
				tval=0;
				sprintf(splash[splash_line++],"\tSelected test not available on the version.\n");
			}
			include_test[tval]++;
			include_tflag++;
			break;
		case 'v':	/* Show version information */
    			for(ind=0; strlen(head1[ind]); ind++)
    			{
				printf("%s\n", head1[ind]);
    			}
			exit(0);
			break;
		case 'U':	/* Specify the dev name for umount/mount*/
			Uflag++;
			strcpy(mountname,optarg);
			if(trflag)
			{
				printf("Can not run throughput tests with unmount & remounts.\n");
				exit(16);
			}
			break;
		case 'w':	/* Do not unlink files */
			sprintf(splash[splash_line++],"\tSetting no_unlink\n");
			no_unlink = 1;
			break;
		case 'Z':	/* Turn on the mmap and file I/O mixing */
			sprintf(splash[splash_line++],"\tEnable mmap & file I/O mixing.\n");
			mmap_mix = 1;
			break;
		case 'W':	/* Read/Write with file locked */
			file_lock=1;
			sprintf(splash[splash_line++],"\tLock file when reading/writing.\n");
			break;
		case 'K':	/* Cause disrupted read pattern */
			disrupt_flag=1;
			sprintf(splash[splash_line++],"\tDisrupted read patterns selected.\n");
			break;
		case 'X':	/* Open write telemetry file */
			compute_flag=1;
			sverify=2;  /* touch lightly */
			w_traj_flag=1;
			strcpy(write_traj_filename,optarg);
			traj_vers();
			w_traj_size();
			sprintf(splash[splash_line++],"\tUsing write telemetry file \"%s\"\n",
				write_traj_filename);
			w_traj_fd=open_w_traj();
			if(w_traj_fd == (FILE *)0)
				exit(200);
			break;
		case 'Y':	/* Open Read telemetry file */
			compute_flag=1;
			sverify=2;  /* touch lightly */
			r_traj_flag=1;
			strcpy(read_traj_filename,optarg);
			sprintf(splash[splash_line++],"\tUsing read telemetry file \"%s\"\n",
				read_traj_filename);
			traj_vers();
			r_traj_size();
			r_traj_fd=open_r_traj();
			if(r_traj_fd == (FILE*) 0)
				exit(200);
			break;
		case 'n':	/* Set min file size for auto mode */
			nflag=1;
			minimum_file_size = (off64_t)atoi(optarg);
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				;
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				minimum_file_size = (long long)(1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				minimum_file_size = (long long)(1024 * 1024 * (long long)atoi(optarg));
			}
			if(minimum_file_size < RECLEN_START/1024)
				minimum_file_size=(off64_t)(RECLEN_START/1024);
			if(minimum_file_size < page_size/1024)
				minimum_file_size=(off64_t)(page_size/1024);
#ifdef NO_PRINT_LLD
			sprintf(splash[splash_line++],"\tUsing minimum file size of %ld kilobytes.\n",minimum_file_size);
#else
			sprintf(splash[splash_line++],"\tUsing minimum file size of %lld kilobytes.\n",minimum_file_size);
#endif
			break;
		case 'g':	/* Set maximum file size for auto mode */
			gflag=1;
			maximum_file_size = (off64_t)atoi(optarg);
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				;
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				maximum_file_size = (long long)(1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				maximum_file_size = (long long)(1024 * 1024 * (long long)atoi(optarg));
			}
			if(maximum_file_size < RECLEN_START/1024)
				maximum_file_size=(off64_t)(RECLEN_START/1024);
#ifdef NO_PRINT_LLD
			sprintf(splash[splash_line++],"\tUsing maximum file size of %ld kilobytes.\n",maximum_file_size);
#else
			sprintf(splash[splash_line++],"\tUsing maximum file size of %lld kilobytes.\n",maximum_file_size);
#endif
			break;
		case 'z':	/* Set no cross over */
			sprintf(splash[splash_line++],"\tCross over of record size disabled.\n");
			NOCROSSflag=1;
			break;
		case 'y':		/* Set min record size for auto mode */
			yflag=1;
			min_rec_size = ((long long)(atoi(optarg))*1024);
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				min_rec_size = (long long)(1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				min_rec_size = (long long)(1024 * 1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				min_rec_size = (long long)(1024 * 1024 * 1024 *(long long)atoi(optarg));
			}
			if(min_rec_size <= 0)
				min_rec_size=(long long)RECLEN_START;
#ifdef NO_PRINT_LLD
	    		sprintf(splash[splash_line++],"\tUsing Minimum Record Size %ld kB\n", min_rec_size/1024);
#else
	    		sprintf(splash[splash_line++],"\tUsing Minimum Record Size %lld kB\n", min_rec_size/1024);
#endif
			break;
		case 'q':		/* Set max record size for auto mode */
			qflag=1;
			max_rec_size = ((long long)(atoi(optarg))*1024);
			if(optarg[strlen(optarg)-1]=='k' ||
				optarg[strlen(optarg)-1]=='K'){
				max_rec_size = (long long)(1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='m' ||
				optarg[strlen(optarg)-1]=='M'){
				max_rec_size = (long long)(1024 * 1024 * atoi(optarg));
			}
			if(optarg[strlen(optarg)-1]=='g' ||
				optarg[strlen(optarg)-1]=='G'){
				max_rec_size = (long long)(1024 * 1024 * 1024 *(long long)atoi(optarg));
			}
			if(max_rec_size <= 0)
				min_rec_size=(long long)RECLEN_END;
			if(max_rec_size > MAXBUFFERSIZE) {
#ifdef NO_PRINT_LLD
				printf("Error: maximum record size %ld kB is greater than maximum buffer size %ld kB\n ",
					max_rec_size/1024, MAXBUFFERSIZE/1024);
#else
				printf("Error: maximum record size %lld kB is greater than maximum buffer size %lld kB\n ",
					(long long)(max_rec_size/1024LL), (long long)MAXBUFFERSIZE/1024LL);
#endif
				exit(23);
			}
#ifdef NO_PRINT_LLD
			sprintf(splash[splash_line++],"\tUsing Maximum Record Size %ld kB\n", max_rec_size/1024);
#else
			sprintf(splash[splash_line++],"\tUsing Maximum Record Size %lld kB\n", max_rec_size/1024);
#endif
			break;

		/* 
		 * The + operator is for the new extended options mechanism 
		 * Syntax is -+ followed by option leter, and if the optino
		 * takes an operand  then it is implemented below. An example
		 * -+a arg    is shown below. This is a sub option with an argument.
		 * -+b  is shown below. This is a sub option with no argument.
		 */
		case '+':
			/* printf("Plus option = >%s<\n",optarg);*/
			switch (*((char *)optarg))
			{
				case 'a':  /* Example: Has argument */
					subarg=argv[optind++];
					/* if(subarg!=(char *)0)   Error checking. */
					/* printf("Plus option argument = >%s<\n",subarg);*/
					break;
				case 'b':  /* burst */
					if(trflag)
					{
						printf("Can not run throughput tests with the burst option.\n");
						exit(100);
					}
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
						printf("-+b takes an operand !!\n");
						exit(200);
					}
					if (strchr(subarg,',') == NULL)
					{
						printf("-+b operand is of the format #,# (burst size (K/M/G) ,mili-second) !!\n");
						exit(400);
					}
					burst_sleep_duration_msec = atoi(strchr(subarg,',') + 1);
					if (burst_sleep_duration_msec == 0) {
						printf("-+b  cannot parse sleep duration !!\n");
						exit(400);
					}
					*strchr(subarg,',') = '\0';
					#ifdef NO_PRINT_LLD
						sscanf(subarg,"%ld",&burst_size_kb_64);
					#else
						sscanf(subarg,"%lld",&burst_size_kb_64);
					#endif
					if(subarg[strlen(subarg)-1]=='k' ||
						subarg[strlen(subarg)-1]=='K'){
					;
					}
					if(subarg[strlen(subarg)-1]=='m' ||
						subarg[strlen(subarg)-1]=='M'){
						burst_size_kb_64 = burst_size_kb_64 * 1024;
					}
					if(subarg[strlen(optarg)-1]=='g' ||
						subarg[strlen(optarg)-1]=='G'){
						burst_size_kb_64 = burst_size_kb_64 * 1024 * 1024;
					}
					if (burst_size_kb_64 <= 0) {
						printf("-+b  cannot parse burst size '%s'!!\n",subarg);
						exit(400);
					}
					*(subarg + strlen(subarg)) = ','; /* so it will be printed when dumping the argsuments */
					break;
				case 'F':  /* Example: Does not have an argument */
					del_flag = 1;
					break;
				case 'c':  /* Argument is the controlling host name */
					/* I am a client for distributed Iozone */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+c takes an operand !!\n");
					     exit(200);
					}
					strcpy(controlling_host_name,subarg);
					distributed=1;
					client_iozone=1;
					master_iozone=0;
					break;
                                case 'h':  /* Argument is the controlling host name */
                                        subarg=argv[optind++];
                                        if(subarg==(char *)0)
                                        {
                                             printf("-+h takes an operand !!\n");
                                             exit(200);
                                        }
                                        strcpy(controlling_host_name,subarg);
                                        sprintf(splash[splash_line++],"\tHostname = %s\n",controlling_host_name);
                                        break;
				case 'm':  /* I am the controlling process for distributed Iozone */
					   /* Does not have an argument */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+m takes an operand. ( filename )\n");
					     exit(201);
					}
					strcpy(client_filename,subarg);
					ret=get_client_info();
					if(ret <= 0)
					{
						printf("Error reading client file\n");
						exit(178);
					}
					clients_found=ret;
					distributed=1;
					master_iozone=1;
					client_iozone=0;
					sprintf(splash[splash_line++],"\tNetwork distribution mode enabled.\n");
					break;
				case 'N':  /* turn off truncating the file before write test */
					notruncate = 1;
					break;
				case 'u':	/* Set CPU utilization output flag */
					cpuutilflag = 1;	/* only used if R(eport) flag is also set */
					get_rusage_resolution();
    					sprintf(splash[splash_line++],"\tCPU utilization Resolution = %5.3f seconds.\n",cputime_res);
	    				sprintf(splash[splash_line++],"\tCPU utilization Excel chart enabled\n");
					break;
				case 's':  /* Clients operate in silent mode. */
					   /* Does not have an argument */
					silent=1;
					break;
				case 'd':  /* Diagnostics mode */
					sprintf(splash[splash_line++],"\t>>> I/O Diagnostic mode enabled. <<<\n");
    					sprintf(splash[splash_line++],"\tPerformance measurements are invalid in this mode.\n");
					diag_v=1;
					sverify=0;
					break;
				case 'x':  /* Argument is the multiplier for rec size and file size */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+c takes an operand !!\n");
					     exit(200);
					}
					multiplier = atoi(subarg);
					if(multiplier <=1)
						multiplier = 2;
					break;
				case 'i':  /* Argument is the host port */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+i takes an operand !!\n");
					     exit(200);
					}
					controlling_host_port = atoi(subarg);
					break;
				case 'p':  /* Argument is the percentage read */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+p takes an operand !!\n");
					     exit(200);
					}
					pct_read = atoi(subarg);
					if(pct_read < 1)
						pct_read = 1;
					if(pct_read >=100)
						pct_read = 100;
    					sprintf(splash[splash_line++],"\tPercent read in mix test is %d\n",pct_read);
					break;
				case 't':  /* Speed code activated */
					speed_code=1;
					break;
#if defined(_HPUX_SOURCE) || defined(linux) || defined(solaris)
				case 'r':  /* Read sync too */
					read_sync=1;
    					sprintf(splash[splash_line++],"\tRead & Write sync mode active.\n");
					break;
#endif
#ifndef NO_MADVISE
				case 'A':  /* Argument is madvise selector */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					   printf("-+A take an operand !!\n");
					   exit(200);
					}
					advise_flag=1;
					advise_op=atoi(subarg);
					sprintf(splash[splash_line++],"\tMadvise enabled: %d\n",advise_op);
					break;
#endif
				case 'n':	/* Set no-retest */
					noretest = 1;	
    					sprintf(splash[splash_line++],"\tNo retest option selected\n");
					break;
				case 'k':	/* Constant aggregate data set size */
					aggflag=1;
					break;
				case 'q':  /* Argument is the rest time between tests in seconds */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+q takes an operand !!\n");
					     exit(200);
					}
					rest_val = (long long)atoi(subarg);
					if(rest_val <=0)
						rest_val = 0;
					restf=1;
					sprintf(splash[splash_line++],"\tDelay %d seconds between tests enabled.\n",atoi(subarg));
					break;
#if defined(O_DSYNC)
				case 'D':  /* O_DSYNC mode */
					sprintf(splash[splash_line++],"\t>>> O_DSYNC mode enabled. <<<\n");
					odsync=1;
					break;
#endif
				case 'l':  /* Record locking mode */
					sprintf(splash[splash_line++],"\t>>> Record locking mode enabled. <<<\n");
					rlocking=1;
					break;
				case 'L':  /* Record locking mode shared files*/
					sprintf(splash[splash_line++],"\t>>> Record locking, shared file mode enabled. <<<\n");
					share_file=1;
					rlocking=1;
					break;
				case 'J':  /* Thinktime is included in thrhoughput */
					sprintf(splash[splash_line++],"\t>>> Think time is included in throughput cal. <<<\n");
					inc_think=1;
					break;
				case 'V':  /* No Record locking shared files*/
					sprintf(splash[splash_line++],"\t>>> Shared file mode enabled. <<<\n");
					share_file=1;
					break;
				case 'B':  /* Sequential mix */
					sprintf(splash[splash_line++],"\t>>> Sequential Mixed workload. <<<\n");
					seq_mix=1;
					break;
 				        /* Use an existing user file, that does
					 not contain Iozone's pattern. Use file
	                                 for testing, but read only, and no 
                                         delete at the end of the test. Also, 
                                         no pattern verification, but do touch
                                         the pages. */
				case 'E':  
					sprintf(splash[splash_line++],"\t>>> No Verify mode. <<<\n");
					sverify=2;
					no_unlink=1;
					no_write=1;
					break;
				case 'T':  /* Time stamps on */
					L_flag=1;
					break;
				case 'X': /* Short circuit test mode */
					X_flag = 1;
					sverify=1;
					verify=1;
					inp_pat = 0xBB;
					pattern = ((inp_pat << 24) | 
					  (inp_pat << 16) | (inp_pat << 8) |
					   inp_pat);
	    				sprintf(splash[splash_line++],"\tShort circuit mode. For\n");
	    				sprintf(splash[splash_line++],"\t filesystem development testing ONLY !\n");
					break;
				case 'Z': /* Compatibility mode for 0xA5 */	
					Z_flag = 1;
					sverify=1;
					verify=1;
					inp_pat = 0xA5;
					pattern = ((inp_pat << 24) | 
					  (inp_pat << 16) | (inp_pat << 8) |
					   inp_pat);
	    				sprintf(splash[splash_line++],"\tUsing old data sets.\n");
    					sprintf(splash[splash_line++],"\t Performance measurements may be invalid in this\n");
    					sprintf(splash[splash_line++],"\t mode due to published hack.\n");
					break;
#if defined(Windows)
				case 'U':  /* Windows only Unbufferd I/O */
					unbuffered=1;
					sprintf(splash[splash_line++],"\tUnbuffered Windows API usage. >>> Very Experimental <<<\n");
					break;
#endif
				case 'K':  /* Sony special for manual control of test 8 */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+K takes an operand !!\n");
					     exit(204);
					}
					Kplus_readers = (int)atoi(subarg);
					if(Kplus_readers <=0)
						Kplus_readers = 1;
					Kplus_flag=1;
					sprintf(splash[splash_line++],"\tManual control of test 8. >>> Very Experimental. Sony special <<<\n");
					break;
				case 'w':  /* Argument is the percent of dedup */
					   /* Sets size of dedup region across files */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+w takes an operand !!\n");
					     exit(200);
					}
					dedup = atoi(subarg);
					if(dedup <=0)
						dedup = 0;
					if(dedup >100)
						dedup = 100;
					sprintf(splash[splash_line++],"\tDedup activated %d percent.\n",dedup);
					break;
				case 'y':  /* Argument is the percent of interior dedup */
					   /* Sets size of dedup region within and across files */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+y takes an operand !!\n");
					     exit(200);
					}
					dedup_interior = atoi(subarg);
					if(dedup_interior <0)
						dedup_interior = 0;
					if(dedup_interior >100)
						dedup_interior = 100;
					sprintf(splash[splash_line++],"\tDedup within & across %d percent.\n",dedup_interior);
					break;
				case 'C':  /* Argument is the percent of dedupe within & !across */
					   /* Sets size of dedup region within and !across files */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+C takes an operand !!\n");
					     exit(200);
					}
					dedup_compress = atoi(subarg);
					if(dedup_compress <0)
						dedup_compress = 0;
					if(dedup_compress >100)
						dedup_compress = 100;
					sprintf(splash[splash_line++],"\tDedup within %d percent.\n",dedup_compress);
					break;
				case 'S':  /* Argument is the seed for dedup */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+S takes an operand !!\n");
					     exit(200);
					}
					dedup_mseed = atoi(subarg);
					if(dedup_mseed ==0)
						dedup_mseed = 1;
					sprintf(splash[splash_line++],"\tDedup manual seed %d .\n",dedup_mseed);
					break;
				case 'W':  /* Argument is the child_skew for dedup */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+W takes an operand !!\n");
					     exit(200);
					}
					chid_skew = atoi(subarg);
					sprintf(splash[splash_line++],"\tDedup chid_skew %d .\n",chid_skew);
					break;
				case 'H':  /* Argument is hostname of the PIT */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+H takes operand !!\n");
					     exit(200);
					}
					strcpy(pit_hostname,subarg);
					sprintf(splash[splash_line++],"\tPIT_host %s\n",pit_hostname);
					
					break;
				case 'P':  /* Argument is port of the PIT */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+P takes operand !!\n");
					     exit(200);
					}
					strcpy(pit_service,subarg);
					sprintf(splash[splash_line++],"\tPIT_port %s\n",pit_service);
					break;
				case 'z':  /* Enable hist summary*/
					hist_summary=1;
					sprintf(splash[splash_line++],"\tHistogram summary enabled.\n");
					break;
				case 'O':  /* Argument is the Op rate */
					subarg=argv[optind++];
					if(subarg==(char *)0)
					{
					     printf("-+O takes an operand !!\n");
					     exit(200);
					}
					op_rate = atoi(subarg);
					if(op_rate <= 0)
						op_rate = 1;
					op_rate_flag = 1;
					sprintf(splash[splash_line++],"\tRate control active %d Ops/sec .\n",op_rate);
					break;
				default:
					printf("Unsupported Plus option -> %s <-\n",optarg);
					exit(255);
					break;
			}	
			break;
		default:
			printf("Unsupported option -> %s <-\n",optarg);
			exit(255);
		}
	}
	base_time=(long)time_so_far();
	get_resolution(); 		/* Get clock resolution */
	if(speed_code)
	{
		do_speed_check(client_iozone);
		exit(0);
	}
	if(r_count > 1)
	{
		aflag=1;
		rflag=0;
		NOCROSSflag=1;
	}
	if(s_count > 1)
	{
		aflag=1;
		sflag=0;
		NOCROSSflag=1;
	}
	/*
         * If not in silent mode then display the splash screen.
	 */
	for(i=0;i<splash_line;i++)
		if(!silent) printf("%s",splash[i]);
	/*
	 * Save the command line for later 
	 */
	record_command_line(argcsave, argvsave);

	if(pflag) /* Allocate after cache_size is set */
	{
		alloc_pbuf();
	}
	if(distributed && master_iozone)
	{
		if(maxt > clients_found)
		{
			printf("You can not specify more threads/processes than you have in the client file list\n");
			exit(202);
		}
	}
	
	if(!OPS_flag && !MS_flag)
	{
		if(!silent) printf("\tOutput is in kBytes/sec\n");
	}
	if (min_rec_size > max_rec_size) {
#ifdef NO_PRINT_LLD
		printf("Error: minimum record size %ld kB is greater than maximum record size %ld kB\n ",
			min_rec_size/1024, max_rec_size/1024);
#else
		printf("Error: minimum record size %lld kB is greater than maximum record size %lld kB\n ",
			min_rec_size/1024, max_rec_size/1024);
#endif
		exit(23);
        }
	orig_min_rec_size=min_rec_size;
	orig_max_rec_size=max_rec_size;
	/*
 	 * No telemetry files... just option selected 
	 */
	if(compute_flag && jflag  && !(r_traj_flag || w_traj_flag))
		if(!silent) printf("\tCompute time %f seconds for reads and writes.\n",compute_time);
	/*
 	 * Read telemetry file and option selected
	 */
	if(compute_flag && r_traj_flag && !w_traj_flag)
	{
		if(r_traj_items==3)
		{
			if(!silent) printf("\tCompute time from telemetry files for reads.\n");
		}
		else
		{
			if(jflag)
				if(!silent) printf("\tCompute time %f seconds for reads.\n",compute_time);
		}
		if(jflag)
			if(!silent) printf("\tCompute time %f seconds for writes.\n",compute_time);
	}
	/*
 	 * Write telemetry file and option selected
	 */
	if(compute_flag && !r_traj_flag && w_traj_flag)
	{
		if(w_traj_items==3)
		{
			if(!silent) printf("\tCompute time from telemetry files for writes.\n");
		}
		else
		{
			if(jflag)
				if(!silent) printf("\tCompute time %f seconds for writes.\n",compute_time);
		}
		if(jflag)
			if(!silent) printf("\tCompute time %f seconds for reads.\n",compute_time);
	}
	if(compute_flag && r_traj_flag && w_traj_flag && jflag)
	{
		if(r_traj_items==3)
		{
			if(!silent) printf("\tCompute time from telemetry files for reads.\n");
		}
		else
		{
			if(!silent) printf("\tCompute time %f seconds for reads.\n",compute_time);
		}
		if(w_traj_items==3) 
		{
			if(!silent) printf("\tCompute time from telemetry files for writes.\n");
		}
		else
		{
			if(!silent) printf("\tCompute time %f seconds for writes.\n",compute_time);
		}
	}
	if(compute_flag && r_traj_flag && w_traj_flag && !jflag)
	{
		if(r_traj_items==3)
		{
			if(!silent) printf("\tCompute time from telemetry files for reads.\n");
		}
		else
		{
			if(!silent) printf("\tNo compute time for reads.\n");
		}

		if(w_traj_items==3) 
		{
			if(!silent) printf("\tCompute time from telemetry files for writes.\n");
		}
		else
		{
			if(!silent) printf("\tNo compute time for writes.\n");
		}
	}

        /* Enforce only write,rewrite,read,reread */
        if(w_traj_flag || r_traj_flag)
        {
                for(i=2;i<sizeof(func)/sizeof(char *);i++)
                {
                        if(seq_mix && (i==8))
                                ;
                        else
                                include_test[i] = 0;
                }
        }

	if(r_traj_flag)
	{
		if(include_test[READER_TEST] == 0) 
		{
			include_test[WRITER_TEST]=1;
			include_test[READER_TEST]=1;
			include_tflag=1;
		}
	}
	if(w_traj_flag)
	{
		if(include_test[WRITER_TEST] == 0) 
		{
			include_test[WRITER_TEST]=1;
			include_tflag=1;
		}
	}
	if(w_traj_flag && w_traj_fsize != 0)
		kilobytes64=w_traj_fsize/1024;
	if(r_traj_flag && r_traj_fsize != 0)
		kilobytes64=r_traj_fsize/1024;

	if( sverify==0 && (w_traj_flag || r_traj_flag))
	{
		printf("\n\tFull verification not supported in telemetry mode.\n\n");
		exit(17);
	}
	;
	if(disrupt_flag &&(w_traj_flag || r_traj_flag) )
	{
		printf("\n\tDisrupt not supported in telemetry mode.\n\n");
		exit(17);
	}
	if(aflag &&(w_traj_flag || r_traj_flag) )
	{
		printf("\n\tAuto mode not supported in telemetry mode.\n");
		printf("\tTry:   -i 0 -i 1 \n\n");
		exit(17);
	}
	if(sflag && w_traj_flag )
	{
		printf("\n\tSize of file is determined by telemetry file.\n\n");
		exit(17);
	}
	if(rflag && w_traj_flag )
	{
		printf("\n\tRecord size of file is determined by telemetry file.\n\n");
		exit(17);
	}
	if(stride_flag && (w_traj_flag || r_traj_flag))
	{
		printf("\n\tStride size is determined by telemetry file.\n\n");
		exit(17);
	}
	if(trflag && MS_flag)
	{
		printf("\n\tMicrosecond mode not supported in throughput mode.\n\n");
		exit(17);
	}
	if (trflag	/* throughput mode, don't allow auto-mode options: */
		&& (auto_mode || aflag || yflag || qflag || nflag || gflag))
	{
		printf("\n\tCan not mix throughput mode and auto-mode flags.\n\n");
		exit(17);
	}
	if(fflag && trflag)
	{
		printf("\n\tYou must use -F when using multiple threads or processes.\n\n");
		exit(17);
	}
	if(aflag && mfflag)
	{
		printf("\n\tYou must use -f when using auto mode.\n\n");
		exit(17);
	}
	if(async_flag && mmapflag)
	{
		printf("\n\tSorry ... Only mmap or async but not both\n\n");
		exit(18);
	}
#ifndef ASYNC_IO
	if(async_flag)
	{
		printf("\n\tSorry ... This version does not support async I/O\n\n");
		exit(19);
	}
#endif
	if(no_write)
	{
	   if(!include_tflag)
	   {
	     printf("You must specify which tests ( -i # ) when using -+E\n");
	     exit(19);
	   }
	}
	if(include_tflag)
	{
		for(i=0;i<sizeof(func)/sizeof(char *);i++)
			if(include_test[i])
				include_mask|=(long long)(1<<i);
		/* printf(">> %llx",include_mask);  HERE */
	}
	if(no_write) /* Disable if any writer would disturbe existing file */
	{
	   if(include_test[0] || include_test[4] ||
	      include_test[6] || include_test[8] || include_test[9] ||
              include_test[11])
	   {
	      printf("You must disable any test that writes when using -+E\n");
	      exit(20);
	   }
	}
	if(no_write) /* User must specify the existing file name */
	{
	   if(!(fflag | mfflag))
	   {
	      printf("You must use -f or -F when using -+E\n");
	      exit(20);
	   }
	}
	if(h_flag && k_flag)
	{
		printf("\n\tCan not do both -H and -k\n");
		exit(20);
	}
	if((dedup | dedup_interior) && diag_v)
	{
		printf("\n\tCan not do both -+d and -+w\n");
		exit(20);
	}
		
	if(!aflag && !rflag)
		max_rec_size=min_rec_size;

	init_record_sizes(min_rec_size,max_rec_size);
    	if(!silent) printf("\tTime Resolution = %1.6f seconds.\n",time_res);
#ifdef NO_PRINT_LLD
    	if(!silent) printf("\tProcessor cache size set to %ld kBytes.\n",cache_size/1024);
    	if(!silent) printf("\tProcessor cache line size set to %ld bytes.\n",cache_line_size);
	if(!silent) printf("\tFile stride size set to %ld * record size.\n",stride);
#else
    	if(!silent) printf("\tProcessor cache size set to %ld kBytes.\n",cache_size/1024);
    	if(!silent) printf("\tProcessor cache line size set to %ld bytes.\n",cache_line_size);
	if(!silent) printf("\tFile stride size set to %lld * record size.\n",stride);
#endif
	if (burst_size_kb_64 != -1 && !silent)
#ifdef NO_PRINT_LLD
		printf("\tBurst size set to %ld Kbytes.\n\tBurst sleep duration set to %ld msec\n",
		burst_size_kb_64, burst_sleep_duration_msec);
#else
		printf("\tBurst size set to %lld Kbytes.\n\tBurst sleep duration set to %lld msec\n",
		burst_size_kb_64, burst_sleep_duration_msec);
#endif
	if(!rflag)
		reclen=(long long)4096;

	if(uflag && !lflag)
		num_child=mint = 1;
	if(lflag && !uflag)
		maxt = mint;
	if(use_thread)
		port="thread";
	else
		port="process";
	if(lflag || uflag){
#ifdef NO_PRINT_LLD
		if(!silent) printf("\tMin %s = %ld \n",port,mint);
		if(!silent) printf("\tMax %s = %ld \n",port,maxt);
#else
		if(!silent) printf("\tMin %s = %lld \n",port,mint);
		if(!silent) printf("\tMax %s = %lld \n",port,maxt);
#endif
	}
	if(trflag)
	{
		if(num_child > 1)
		{
			if(use_thread)
			{
				port="threads";
			}
			else
			{
				port="processes";
			}
		}

#ifdef NO_PRINT_LLD
		if(!silent) printf("\tThroughput test with %ld %s\n", num_child,port);
#else
		if(!silent) printf("\tThroughput test with %lld %s\n", num_child,port);
#endif
	}
        numrecs64 = (long long)(kilobytes64*1024)/reclen;
        if (reclen >  (long long)MAXBUFFERSIZE) {
#ifdef NO_PRINT_LLD
                printf("Error: Maximum record length is %ld bytes\n",
                                MAXBUFFERSIZE);
#else
                printf("Error: Maximum record length is %lld bytes\n",
                                (long long)MAXBUFFERSIZE);
#endif
                exit(21);
        }
        if (reclen < (long long)MINBUFFERSIZE) {
#ifdef NO_PRINT_LLD
                printf("Error: Minimum record length is %ld bytes\n",
                                MINBUFFERSIZE);
#else
                printf("Error: Minimum record length is %lld bytes\n",
                                (long long)MINBUFFERSIZE);
#endif
                exit(22);
        }
	/* Only bzero or fill that which you will use. The buffer is very large */
	if(verify )	
	{
		fill_buffer((char *)buffer,l_min(reclen,(long long)cache_size),(long long)pattern,(char)sverify,(long long)0);
		if(pflag)
			fill_buffer((char *)pbuffer,l_min(reclen,(long long)cache_size),(long long)pattern,(char)sverify,(long long)0);
		if(mflag)
			fill_buffer((char *)mbuffer,l_min(reclen,(long long)cache_size),(long long)pattern,(char)sverify,(long long)0);
	}
	else
	{
		bzero(buffer,(size_t)l_min(reclen,(long long)cache_size));
	}

#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=begin_proc;
#if defined( _HPUX_SOURCE )
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;
     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	orig_size=kilobytes64;
	if(trflag){
	    (void)multi_throughput_test(mint,maxt);
	    goto out;
	}
	if(trflag && (mint == maxt)){
		auto_mode=0;
		throughput_test();
	    	goto out;
	}
    	if (aflag) {
		print_header();
		auto_test();
		goto out;
	} 
	print_header();
	(void) begin(kilobytes64,reclen);
out:
	if(r_traj_flag)
		fclose(r_traj_fd);
	if(w_traj_flag)
		fclose(w_traj_fd);
	if (!no_unlink)
	{
		if(check_filename(dummyfile[0]))
	      	   unlink(dummyfile[0]);	/* delete the file */
	}
	if(!silent) printf("\niozone test complete.\n");
	if(res_prob)
	{
		printf("Timer resolution is poor. Some small transfers may have \n");
		printf("reported inaccurate results. Sizes %ld kBytes and below.\n",
			(long)(rec_prob/(long long)1024));
	}

	if(Rflag && !trflag){
		dump_excel();
	}
	return(0);	
}

#ifdef HAVE_ANSIC_C
void
record_command_line(int argc, char **argv)
#else
void
record_command_line(argc, argv)
int argc;
char **argv;
#endif
{
	int ix, len = 0;

	/* print and save the entire command line */
	if(!silent) printf("\tCommand line used:");
	for (ix=0; ix < argc; ix++) {
		if(!silent) printf(" %s", argv[ix]);
		if ((len + strlen(argv[ix])) < sizeof(command_line)) {
			strcat (command_line, argv[ix]);
			strcat (command_line, " ");
			len += strlen(argv[ix]) + 1;
		}
		else {
			printf ("Command line too long to save completely.\n");
			break;
		}
	}
	if(!silent) printf("\n");
}

/*************************************************************************/
/* BEGIN()								 */
/* This is the main work horse. It is called from main and from 	 */
/* auto_test. The caller provides the size of file and the record length.*/
/*************************************************************************/
#ifdef HAVE_ANSIC_C
void 
begin(off64_t kilos64,long long reclength)
#else
void
begin(kilos64,reclength)
off64_t kilos64;
long long reclength;
#endif
{
	long long num_tests,test_num,i,j;
	long long data1[MAXTESTS], data2[MAXTESTS];
	num_tests = sizeof(func)/sizeof(char *);
#if defined(HAVE_PREAD) 
	if(!Eflag)
	{
#if defined(HAVE_PREAD) && defined(HAVE_PREADV)
		num_tests -= 4;
#else
		num_tests -= 2;
#endif
		if(mmapflag || async_flag)
		{
			num_tests -= 2;
		}
	}
	else
	{
		if(mmapflag || async_flag)
#if defined(HAVE_PREAD) && defined(HAVE_PREADV)
			num_tests -= 6;
#else
			num_tests -= 4;
#endif
	}
#else
	if(mmapflag || async_flag)
	{
		num_tests -= 2;
	}
#endif

	if(RWONLYflag) num_tests = 2;		/* kcollins 8-21-96*/
	sync(); 		/* just in case there were some dirty */
	sync();
	kilobytes64=kilos64;
	reclen=reclength;
	numrecs64 = (kilobytes64*1024)/reclen;
	store_value(kilobytes64);
	if(r_traj_flag || w_traj_flag)
		store_value((off64_t)0);
	else	
		store_value((off64_t)(reclen/1024));

#ifdef NO_PRINT_LLD
	if(!silent) printf("%16ld",kilobytes64);
	if(r_traj_flag || w_traj_flag)
	{
		if(!silent) printf("%8ld",0);
	}
	else
	{
		if(!silent) printf("%8ld",reclen/1024);
	}
#else
	if(!silent) printf("%16lld",kilobytes64);
	if(r_traj_flag || w_traj_flag)
	{
		if(!silent) printf("%8lld",(long long )0);
	}
	else
	{
		if(!silent) printf("%8lld",reclen/1024);
	}
#endif
	if(include_tflag)
	{
		for(i=0;i<num_tests;i++)
		{
			if(include_mask & (long long)(1<<i))
			   func[i](kilobytes64,reclen,&data1[i],&data2[i]);
			else
			{
			       	if(!silent) printf("%s",test_output[i]); 
				fflush(stdout);
				for(j=0;j<test_soutput[i];j++)
					store_value((off64_t)0);
			}
		}
	}
	else
	{
		for(test_num=0;test_num < num_tests;test_num++)
		{
			func[test_num](kilobytes64,reclen,&data1[test_num],&data2[test_num]);
		};
	}
	if(!silent) printf("\n");
	if(!OPS_flag && !include_tflag){			/* Report in ops/sec ? */
	   if(data1[1]!=0 && data2[1] != 0)
	   {   
		totaltime = data1[1] + data1[0];
		if (totaltime < TOOFAST) 
		{
		   	goodkilos = (TOOFAST/totaltime)*2*kilobytes64;
		    	printf("\nThe test completed too quickly to give a good result\n");
		    	printf("You will get a more precise measure of this machine's\n");
		    	printf("performance by re-running iozone using the command:\n");
#ifdef NO_PRINT_LLD
		    	printf("\n\tiozone %ld ", goodkilos);
		    	printf("\t(i.e., file size = %ld kilobytes64)\n", goodkilos);
#else
		    	printf("\n\tiozone %lld ", goodkilos);
		    	printf("\t(i.e., file size = %lld kilobytes64)\n", goodkilos);
#endif
		}
	   } else {
	    	goodrecl = reclen/2;
	    	printf("\nI/O error during read.  Try again with the command:\n");
#ifdef NO_PRINT_LLD
	    	printf("\n\tiozone %ld %ld ", kilobytes64,  goodrecl);
	    	printf("\t(i.e. record size = %ld bytes)\n",  goodrecl);
#else
	    	printf("\n\tiozone %lld %lld ", kilobytes64,  goodrecl);
	    	printf("\t(i.e. record size = %lld bytes)\n",  goodrecl);
#endif
	   }
	}
	if (!no_unlink)
	{
		if(check_filename(filename))
    		  unlink(filename);	/* delete the file */
	}
					/*stop timer*/
	return ;
}
/******************************************************************

    SHOW_HELP -- show development help of this program

******************************************************************/
#ifdef HAVE_ANSIC_C
void show_help(void)
#else
void show_help()
#endif
{
    	long long i;
    	if(!silent) printf("iozone: help mode\n\n");
    	for(i=0; strlen(help[i]); i++)
    	{
		if(!silent) printf("%s\n", help[i]);
    	}
}
/******************************************************************

    SIGNAL_HANDLER -- clean up if user interrupts the program

******************************************************************/
#ifdef HAVE_ANSIC_C
void signal_handler(void)
#else
void signal_handler()
#endif
{
	long long i;
	if(distributed)
	{
		if(master_iozone)
			cleanup_children();
	}
	if((long long)getpid()==myid)
	{
    		if(!silent) printf("\niozone: interrupted\n\n");
#ifndef VMS
		if (!no_unlink)
		{
		   if(check_filename(filename))
    			unlink(filename);	/* delete the file */
		   for(i=1;i<num_child;i++)
		   {
		      if(check_filename(dummyfile[i]))
    			unlink(dummyfile[i]);	/* delete the file */
		   }
		}
		if (!no_unlink)
		{
		   if(check_filename(dummyfile[0]))
    			unlink(dummyfile[0]);	/* delete the file */
		}
		
#endif
		if(Rflag && !trflag){
			dump_excel();
		}
		if(Rflag && trflag){
			dump_throughput();
		}

	    	if(!silent) printf("exiting iozone\n\n");
		if(res_prob)
		{
			printf("Timer resolution is poor. Some small transfers may have \n");
			printf("reported inaccurate results. Sizes %ld kBytes and below.\n",
				(long)rec_prob/1024);
		}
		if(trflag && !use_thread)
		   for(i=0;i<num_child;i++)
			kill((pid_t)childids[i],SIGTERM);
		if(r_traj_flag)
			fclose(r_traj_fd);
		if(w_traj_flag)
			fclose(w_traj_fd);
	}
	if(sp_msfd)
		close(sp_msfd);
	if(sp_mrfd)
		close(sp_mrfd);
    	exit(exit_code);
}

/****************************************************************/
/*								*/
/*   AUTO_TEST -- perform series of tests and tabulate results  */
/*								*/
/****************************************************************/
#ifdef HAVE_ANSIC_C
void 
auto_test(void)
#else
void auto_test()
#endif
{
    	off64_t kilosi;
	long long recszi,count1;
	long long mult;
	long long xx;

	/****************************************************************/
	/* Start with file size of 1 megabyte and repeat the test 	*/
	/* KILOBYTES_ITER_LIMIT  					*/
	/* times.  Each time we run, the file size is doubled		*/
	/****************************************************************/
/*
        if(sflag) {
          min_file_size = kilobytes64;
          max_file_size = kilobytes64;
        }
        if(rflag) {
          min_rec_size = reclen;
          max_rec_size = reclen;
        }
*/
	if(gflag)
		max_file_size = maximum_file_size;
	if(nflag)
		min_file_size = minimum_file_size;

        if (min_rec_size > (long long)(min_file_size*1024)) {
#ifdef NO_PRINT_LLD
            printf("Error: record length %ld is greater than filesize %ld kB\n ",
                                min_rec_size,min_file_size);
#else
            printf("Error: record length %lld is greater than filesize %lld kB\n ",
                                min_rec_size,min_file_size);
#endif
                exit(23);
        }

	if(NOCROSSflag) xover = max_file_size;

	init_file_sizes(min_file_size, max_file_size);
	del_record_sizes();
	orig_min_rec_size=min_rec_size;
	orig_max_rec_size=max_rec_size;
	init_record_sizes(min_rec_size, max_rec_size);

        for(kilosi=get_next_file_size((off64_t)0); kilosi>0; kilosi=get_next_file_size(kilosi))
        {
	/****************************************************************/
	/* Start with record size of min_rec_size bytes and repeat the 	*/
	/* test, multiplying the record size by MULTIPLIER each time, 	*/
	/* until we reach max_rec_size.	 At the CROSSOVER we stop doing */
	/* small buffers as it takes forever and becomes very 		*/
	/* un-interesting.						*/
	/****************************************************************/
             if(!rflag && !sflag && !yflag && !qflag)
             	if(kilosi > xover){
                	min_rec_size = LARGE_REC;
			mult = orig_min_rec_size/1024;
			del_record_sizes();
			init_record_sizes(min_rec_size, max_rec_size);
		     	/************************************/
			/* Generate dummy entries in the    */
			/* Excel buffer for skipped         */
			/* record sizes			 */
			/************************************/
			for(count1=min_rec_size;
			     	(count1 != orig_min_rec_size) && (
				     	mult <= (kilosi*1024)) ;
						count1=(count1>>1))
			{
			    	current_x=0;
			     	store_value((off64_t)kilosi);
			     	store_value((off64_t)mult);
			     	for(xx=0;xx<20;xx++)
			     		store_value((off64_t)0);
			     	mult=mult*2;
			     	current_y++;
			     	if(current_y>max_y)
			     		max_y=current_y;
			     	current_x=0;
			}
	     }

             for (recszi=get_next_record_size((off64_t)0);recszi!=0;recszi=get_next_record_size(recszi))
             {
                     if(recszi > (kilosi*1024)) 
			break;
                     begin(kilosi, recszi );
		     current_x=0;
		     current_y++;
             }
	}
}


/****************************************************************/
/*								*/
/* THROUGHPUT_TEST () Multi process throughput test		*/
/*								*/
/* Note: States for share memory barrier are:			*/
/* 0 = Child not running or has finished.			*/
/* 1 = Child is ready to begin.					*/
/* 2 = Child is told to begin.					*/
/****************************************************************/
/* Data in shared memory format is:				*/
/*								*/
/* struct child_stats {						*/
/* 	long long flag; 	Used to barrier			*/
/*	double walltime;	Child's elapsed time		*/
/*	double cputime;		Child's CPU time		*/
/* 	double throughput;	Child's throughput		*/
/* 	double actual;		Child's actual read/written	*/
/* } 								*/
/*								*/
/* There is an array of child_stat structures layed out in 	*/
/* shared memory.						*/
/*								*/
/****************************************************************/

#ifdef HAVE_ANSIC_C
void 
throughput_test(void)
#else
void 
throughput_test()
#endif
{
	char *unit;
	double starttime1 = 0;
	double jstarttime = 0;
	double jtime = 0;
	double walltime = 0;
	double cputime = 0;
	char *port;
	char getout;
	long long throughsize = KILOBYTES;
	long long xx,xy,i;
	long long xyz;
	double ptotal;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	VOLATILE char *temp;
	double min_throughput = 0;
	double max_throughput = 0;
	double avg_throughput = 0;
	double min_xfer = 0; 


	toutputindex=0;
	strcpy(&toutput[0][0],throughput_tests[0]);
	ptotal=written_so_far=read_so_far=re_written_so_far=re_read_so_far=0 ;

	if(OPS_flag)
		unit="ops";
	else
		unit="kB";

	if(!haveshm)
	{
		shmaddr=(struct child_stats *)alloc_mem((long long)SHMSIZE,(int)1);
#ifdef _64BIT_ARCH_
		if((long long)shmaddr==(long long)-1)
#else
		if((long )shmaddr==(long)-1)
#endif
		{
			printf("\nShared memory not working\n");
			exit(24);
		}
		haveshm=(char*)shmaddr;
	}
	else
		shmaddr=(struct child_stats *)haveshm;

	if(use_thread)
		stop_flag = &stoptime;
	else
	{
		temp = (char *)&shmaddr[0];
		stop_flag = (char *)&temp[(long long)SHMSIZE]-4;
	}
	for(xyz=0;xyz<num_child;xyz++){ /* all children to state 0 (HOLD) */
		child_stat = (struct child_stats *)&shmaddr[xyz];
		child_stat->flag=CHILD_STATE_HOLD;
		child_stat->actual=0;
		child_stat->throughput=0;
		child_stat->cputime=0;
		child_stat->walltime=0;
	}
	*stop_flag = 0;
	if(!sflag)
		kilobytes64=throughsize;
	if(!rflag)
        	reclen=(long long)4096;
	if(aggflag)
		kilobytes64=orig_size/num_child;
        numrecs64 = (long long)(kilobytes64*1024)/reclen;
	buffer=mainbuffer;
	if(use_thread)
		port="thread";
	else
		port="process";
	if(w_traj_flag)
	{
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tEach %s writes a %ld kByte file in telemetry controlled records\n",
		port,kilobytes64);
#else
	if(!silent) printf("\tEach %s writes a %lld kByte file in telemetry controlled records\n",
		port,kilobytes64);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tEach %s writes a %ld kByte file in %ld kbyte records\n",
		port,kilobytes64,reclen/1024);
#else
	if(!silent) printf("\tEach %s writes a %lld kByte file in %lld kByte records\n",
		port,kilobytes64,reclen/1024);
#endif
	}

	if(fflag)		/* Each child has a file name to write */
	  for(xx=0;xx<num_child;xx++)
	    filearray[xx] = filename;
	myid = (long long)getpid();

	/* rags: skip writer test */
	if(include_tflag)
		if(!(include_mask & (long long)WRITER_MASK))
		{
			store_dvalue( (double)0);
			store_dvalue( (double)0);
			toutputindex++;
			goto next0;
		}

	if((!distributed) || (distributed && master_iozone))
		start_monitor("Write");
	/* Hooks to start the distributed Iozone client/server code */
	if(distributed)
	{
		use_thread=0;  /* Turn of any Posix threads */
		if(master_iozone)
			master_listen_socket = start_master_listen();
		else
			become_client();
	}
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		childids[xx] = start_child_proc(THREAD_WRITE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				if(!use_thread)
					kill((pid_t)childids[xy],SIGTERM);
			}
			exit(25);
		}
		if(childids[xx]!=0 && debug1)
#ifdef NO_PRINT_LLD
			printf("Parent starting slot %ld\n",xx);	
#else
			printf("Parent starting slot %lld\n",xx);	
#endif
		if( childids[xx] == 0 ){
#ifdef _64BIT_ARCH_
		  thread_write_test((void *)xx);
#else
		  thread_write_test((void *)(long)xx);
#endif
		}else {
#ifdef NO_PRINT_LLD
		  sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx], xx);
#else
		  sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx], xx);
#endif
		}
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */

#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx], xx);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx], xx);
#endif
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}

#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create(thread_write_test,(void*)xx);
#else
		childids[xx] = mythread_create(thread_write_test,(void*)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("Thread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,SIGTERM);
			}
			exit(27);
		}
	   }
	}
#endif
	if((long long)getpid() == myid)
	{
		prepage(buffer,reclen);		/* Force copy on write */
				/* wait for children to start */
		if(distributed && master_iozone)
		{
			start_master_listen_loop((int) num_child);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];	
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++)	/* Start all children going */
		{
			if(delay_start!=0)
				Poll((long long)delay_start);
						/* State "go" */
			child_stat = (struct child_stats *)&shmaddr[i];	
			child_stat->flag=CHILD_STATE_BEGIN;
			if(distributed && master_iozone)
				tell_children_begin(i);
		}
		starttime1 = time_so_far();	/* Start parents timer */
		goto waitout;
	}

waitout:
	getout=0;
	if((long long)getpid() == myid) {	/* Parent only */
		starttime1 = time_so_far(); /* Wait for all children */
		for( i = 0; i < num_child; i++){
			child_stat = (struct child_stats *) &shmaddr[i];
			if(distributed && master_iozone)
			{
				printf("\n\tTest running:");
				wait_dist_join();
				break;
			}
			else
			{
			 if(use_thread)
			 {
				thread_join(childids[i],(void *)&pstatus);
			 }
			 else
			 {
		   	 	wait(0);
			 }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* get parents total time */
	if(total_time < (double).000001) 
	{
		total_time=time_res; 
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif

	total_kilos=0;
	ptotal=0;
	walltime = 0.0;
	cputime = 0.0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *) &shmaddr[xyz];
		total_kilos += child_stat->throughput; /* add up the children */
		ptotal += child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		/* Add up the cpu times of all children */
		cputime += child_stat->cputime;

		/* and find the child with the longest wall time */
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *) &shmaddr[xyz];
		child_stat->flag = CHILD_STATE_HOLD; /* Start children at state 0 (HOLD) */
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %2ld initial writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2ld initial writers \t= %10.2f %s/sec\n",num_child,((double)(ptotal)/total_time),unit);
#else
	if(!silent) printf("\tChildren see throughput for %2lld initial writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2lld initial writers \t= %10.2f %s/sec\n",num_child,((double)(ptotal)/total_time),unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU Utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) 
					printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Write");
	/**********************************************************/
	/*************** End of intitial writer *******************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
	*stop_flag=0;
	if(distributed && master_iozone)
	{
		stop_master_listen(master_listen_socket);
		cleanup_comm();
	}

	/**********************************************************/
	/* Re-write throughput performance test. ******************/
	/**********************************************************/
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	total_kilos=0;
	toutputindex=1;
	strcpy(&toutput[1][0],throughput_tests[1]);
	if(noretest)
	{
		store_dvalue( (double)0);
		goto next0;
	}
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Rewrite");
	/* Hooks to start the distributed Iozone client/server code */
	if(distributed)
	{
		use_thread=0;  /* Turn of any Posix threads */
		if(master_iozone)
			master_listen_socket = start_master_listen();
		else
			become_client();
	}
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_REWRITE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(28);
		}
		if(childids[xx] == 0){
#ifdef _64BIT_ARCH_
			thread_rwrite_test((void *)xx);
#else
			thread_rwrite_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_rwrite_test,xx);
#else
		childids[xx] = mythread_create( thread_rwrite_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)myid,(long long)SIGTERM);
			}
			exit(29);
		}
	   }
	}
#endif
	if((long long)myid == getpid())
	{
		if(distributed && master_iozone)
		{
			start_master_listen_loop((int) num_child);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
					/* wait for children to start */
			while(child_stat->flag==CHILD_STATE_HOLD) 
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++)
		{
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
			if(distributed && master_iozone)
				tell_children_begin(i);
		}
		starttime1 = time_so_far();
		goto jump3;
	}
	
jump3:
	getout=0;
	if((long long)myid == getpid()){	/* Parent only here */
		for( i = 0; i < num_child; i++){
			child_stat=(struct child_stats *)&shmaddr[i];
			if(distributed && master_iozone)
			{
				printf("\n\tTest running:");
				wait_dist_join();
				break;
			}
			else
			{
			   if(use_thread)
			   {
				thread_join(childids[i],(void *)&pstatus);
			   }
			   else
			   {
		   		wait(0);
			   }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res;
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents total time */
	if(total_time  < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	

	total_kilos=0;
	ptotal=0;

	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat=(struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
/*
		if (walltime < cputime_res)
			walltime = 0.0;
*/
		if (cputime < cputime_res)
			cputime = 0.0;
	}

	for(xyz=0;xyz<num_child;xyz++){	/* Reset state to 0 (HOLD) */
		child_stat=(struct child_stats *)&shmaddr[xyz];
		child_stat->flag = CHILD_STATE_HOLD;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %2ld rewriters \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2ld rewriters \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %2lld rewriters \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2lld rewriters \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	*stop_flag=0;
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Rewrite");
	/**********************************************************/
	/*************** End of rewrite throughput ****************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
	if(distributed && master_iozone)
	{
		stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next0:
	if(include_tflag)
		if(!(include_mask & (long long)READER_MASK))
			goto next1;
	/**************************************************************/
	/*** Reader throughput tests **********************************/
	/**************************************************************/
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Read");
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[2]);
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	total_kilos=0;
	if(distributed)
	{
		use_thread=0;
		if(master_iozone)
			master_listen_socket=start_master_listen();
		else
			become_client();
	}
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_READ_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(30);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_read_test((void *)xx);
#else
			thread_read_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_read_test,xx);
#else
		childids[xx] = mythread_create( thread_read_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(31);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
		if(distributed && master_iozone)
		{
			start_master_listen_loop((int) num_child);
		}
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat=(struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++)
		{
			child_stat=(struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN; /* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
			if(distributed && master_iozone)
				tell_children_begin(i);
		}
		starttime1 = time_so_far();
		goto jumpend4;
	}
jumpend4:
	getout=0;
	if(myid == (long long)getpid()){	/* Parent here */
		for( i = 0; i < num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			if(distributed && master_iozone)
			{
				printf("\n\tTest running:");
				wait_dist_join();
				break;
			}
			else
			{
			   if(use_thread)
			   {
				thread_join(childids[i],(void *)&pstatus);
			   }
			   else
			   {
				wait(0);
			   }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res; 
		if(jtime < (double).000001)
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res; 
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat=(struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %2ld readers \t\t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2ld readers \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %2lld readers \t\t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2lld readers \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Read");
	/**********************************************************/
	/*************** End of readers throughput ****************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
	if(distributed && master_iozone)
	{
		stop_master_listen(master_listen_socket);
		cleanup_comm();
	}

	/**************************************************************/
	/*** ReReader throughput tests **********************************/
	/**************************************************************/
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[3]);
	if(noretest)
	{
		store_dvalue( (double)0);
		goto next1;
	}
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Reread");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_REREAD_TEST, numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(32);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_rread_test((void *)xx);
#else
			thread_rread_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_rread_test,xx);
#else
		childids[xx] = mythread_create( thread_rread_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(33);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
		goto jumpend2;
	}
	
jumpend2:
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
			   if(use_thread)
			   {
				thread_join(childids[i],(void *)&pstatus);
			   }
			   else
			   {
				wait(0);
			   }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res; 
		if(jtime < (double).000001)
		{
			jtime=time_res;
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	min_throughput=max_throughput=min_xfer=0;
	total_kilos=0;
	ptotal=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
/*
		if (walltime < cputime_res)
			walltime = 0.0;
*/
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld re-readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld re-readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld re-readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld re-readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Reread");
	/**********************************************************/
	/*************** End of re-readers throughput ****************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}

next1:
	if(include_tflag)
		if(!(include_mask & (long long)REVERSE_MASK))
			goto next2;
	sync();
	sleep(2);

	/**************************************************************/
	/*** Reverse reader throughput tests **************************/
	/**************************************************************/
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[4]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Revread");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_REVERSE_READ_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(34);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_reverse_read_test((void *)xx);
#else
			thread_reverse_read_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_reverse_read_test,xx);
#else
		childids[xx] = mythread_create( thread_reverse_read_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(35);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001)
		{
			jtime=time_res;
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		/* walltime += child_stat->walltime; */
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
/*
		if (walltime < cputime_res)
			walltime = 0.0;
*/
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld reverse readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld reverse readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld reverse readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld reverse readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Revread");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next2:
	if(include_tflag)
		if(!(include_mask & (long long)STRIDE_READ_MASK))
			goto next3;
	/**************************************************************/
	/*** stride reader throughput tests **************************/
	/**************************************************************/
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[5]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Strideread");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_STRIDE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(36);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_stride_read_test((void *)xx);
#else
			thread_stride_read_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_stride_read_test,xx);
#else
		childids[xx] = mythread_create( thread_stride_read_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(37);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		/* walltime += child_stat->walltime; */
		cputime += child_stat->cputime;
		/* Get the biggest walltime */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
/*
		if (walltime < cputime_res)
			walltime = 0.0;
*/
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld stride readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld stride readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld stride readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld stride readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Strideread");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
	/**************************************************************/
	/*** random reader throughput tests ***************************/
	/**************************************************************/
next3:
	if(include_tflag)
		if(!(include_mask & (long long)RANDOM_RW_MASK))
			goto next4;
	
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[6]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Randread");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_RANDOM_READ_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(38);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_ranread_test((void *)xx);
#else
			thread_ranread_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_ranread_test,xx);
#else
		childids[xx] = mythread_create( thread_ranread_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(39);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the biggest walltime */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld random readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld random readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld random readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld random readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Randread");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
	/**************************************************************/
	/***  mixed workload throughput tests ***************************/
	/**************************************************************/
next4:
	if(include_tflag)
		if(!(include_mask & (long long)RANDOM_MIX_MASK))
			goto next5;
	
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[7]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Mixed");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_RANDOM_MIX_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(38);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_mix_test((void *)xx);
#else
			thread_mix_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_mix_test,xx);
#else
		childids[xx] = mythread_create( thread_mix_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(39);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the biggest walltime */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld mixed workload \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld mixed workload \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld mixed workload \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld mixed workload \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Mixed");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next5:
	/**************************************************************/
	/*** random writer throughput tests  **************************/
	/**************************************************************/
	if(include_tflag)
		if(!(include_mask & (long long)RANDOM_RW_MASK) || no_write)
			goto next6;
	
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[8]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Randwrite");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_RANDOM_WRITE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(38);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_ranwrite_test((void *)xx);
#else
			thread_ranwrite_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_ranwrite_test,xx);
#else
		childids[xx] = mythread_create( thread_ranwrite_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(39);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the biggest walltime */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld random writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld random writers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld random writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld random writers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Randwrite");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next6:
	/**************************************************************/
	/*** Pwrite writer throughput tests  **************************/
	/**************************************************************/
#ifndef HAVE_PREAD
		goto next7;
#else
	if(include_tflag)
		if(!(include_mask & (long long)PWRITER_MASK))
			goto next7;
	
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[9]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Pwrite");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_PWRITE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(38);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_pwrite_test((void *)xx);
#else
			thread_pwrite_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_pwrite_test,xx);
#else
		childids[xx] = mythread_create( thread_pwrite_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(39);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the biggest walltime*/
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld pwrite writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld pwrite writers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld pwrite writers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld pwrite writers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Pwrite");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
#endif
	/**************************************************************/
	/*** Pread reader throughput tests  **************************/
	/**************************************************************/
next7:

#ifndef HAVE_PREAD
		goto next8;
#else
	if(include_tflag)
		if(!(include_mask & (long long)PREADER_MASK))
			goto next8;
	
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[10]);
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Pread");
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	sync();
	sleep(2);
	*stop_flag=0;
	total_kilos=0;
        /* Hooks to start the distributed Iozone client/server code */
        if(distributed)
        {
                use_thread=0;  /* Turn of any Posix threads */
                if(master_iozone)
                        master_listen_socket = start_master_listen();
                else
                        become_client();
        }
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_PREAD_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(38);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_pread_test((void *)xx);
#else
			thread_pread_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		chid=xx;
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_pread_test,xx);
#else
		childids[xx] = mythread_create( thread_pread_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(39);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
                if(distributed && master_iozone)
                {
                        start_master_listen_loop((int) num_child);
                }
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat = (struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
                       if(distributed && master_iozone)
                                tell_children_begin(i);
		}
		starttime1 = time_so_far();
	}
	
	getout=0;
	if(myid == (long long)getpid()){	 /* Parent here */
		for( i = 0; i < num_child; i++){ /* wait for children to stop */
			child_stat = (struct child_stats *)&shmaddr[i];
                        if(distributed && master_iozone)
                        {
				printf("\n\tTest running:");
                                wait_dist_join();
                                break;
                        }
                        else
                        {
                           if(use_thread)
                           {
                                thread_join(childids[i],(void *)&pstatus);
                           }
                           else
                           {
                                wait(0);
                           }
                        }
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res;
		if(jtime < (double).000001) 
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat = (struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the biggest walltime*/
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %ld pread readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %ld pread readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %lld pread readers \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %lld pread readers \t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Pread");
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
        if(distributed && master_iozone)
	{
                stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
#endif
next8:
	if(include_tflag)
		if(!(include_mask & (long long)FWRITER_MASK))
			goto next9;
	/**************************************************************/
	/*** fwriter throughput tests *********************************/
	/**************************************************************/
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Fwrite");
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[11]);
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	total_kilos=0;
	if(distributed)
	{
		use_thread=0;
		if(master_iozone)
			master_listen_socket=start_master_listen();
		else
			become_client();
	}
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_FWRITE_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(30);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_fwrite_test((void *)xx);
#else
			thread_fwrite_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_fwrite_test,xx);
#else
		childids[xx] = mythread_create( thread_fwrite_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(31);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
		if(distributed && master_iozone)
		{
			start_master_listen_loop((int) num_child);
		}
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat=(struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++)
		{
			child_stat=(struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN; /* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
			if(distributed && master_iozone)
				tell_children_begin(i);
		}
		starttime1 = time_so_far();
		goto jumpend1;
	}
jumpend1:
	getout=0;
	if(myid == (long long)getpid()){	/* Parent here */
		for( i = 0; i < num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			if(distributed && master_iozone)
			{
				printf("\n\tTest running:");
				wait_dist_join();
				break;
			}
			else
			{
			   if(use_thread)
			   {
				thread_join(childids[i],(void *)&pstatus);
			   }
			   else
			   {
				wait(0);
			   }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res; 
		if(jtime < (double).000001)
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res; 
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat=(struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %2ld fwriters \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2ld fwriters \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %2lld fwriters \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2lld fwriters \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Fwrite");
	/**********************************************************/
	/*************** End of fwrite throughput ****************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
	if(distributed && master_iozone)
	{
		stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next9:
	if(include_tflag)
		if(!(include_mask & (long long)FREADER_MASK))
			goto next10;
	/**************************************************************/
	/*** freader throughput tests *********************************/
	/**************************************************************/
	if((!distributed) || (distributed && master_iozone))
		start_monitor("Fread");
	toutputindex++;
	strcpy(&toutput[toutputindex][0],throughput_tests[12]);
	walltime = 0.0;
	cputime = 0.0;
	jstarttime=0;
	total_kilos=0;
	if(distributed)
	{
		use_thread=0;
		if(master_iozone)
			master_listen_socket=start_master_listen();
		else
			become_client();
	}
	if(!use_thread)
	{
	   for(xx = 0; xx< num_child ; xx++){
		chid=xx;
		childids[xx] = start_child_proc(THREAD_FREAD_TEST,numrecs64,reclen);
		if(childids[xx]==-1){
			printf("\nFork failed\n");
			for(xy = 0; xy< xx ; xy++){
				Kill((long long)childids[xy],(long long)SIGTERM);
			}
			exit(30);
		}
		if(childids[xx]==0){
#ifdef _64BIT_ARCH_
			thread_fread_test((void *)xx);
#else
			thread_fread_test((void *)((long)xx));
#endif
		}	
	   }
	}
#ifndef NO_THREADS
	else
	{
	   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
		if(!barray[xx])
		{
			barray[xx]=(char *) alloc_mem((long long)(MAXBUFFERSIZE+cache_size),(int)0);
			if(barray[xx] == 0) {
        		   perror("Memory allocation failed:");
        		   exit(26);
        		}
     			barray[xx] =(char *)(((long)barray[xx] + cache_size ) & 
			~(cache_size-1));
		}
#ifdef _64BIT_ARCH_
		childids[xx] = mythread_create( thread_fread_test,xx);
#else
		childids[xx] = mythread_create( thread_fread_test,(void *)(long)xx);
#endif
		if(childids[xx]==-1){
			printf("\nThread create failed\n");
			for(xy = 0; xy< xx ; xy++){
				kill((pid_t)myid,(int)SIGTERM);
			}
			exit(31);
		}
	   }
	}
#endif
	if(myid == (long long)getpid()){
		if(distributed && master_iozone)
		{
			start_master_listen_loop((int) num_child);
		}
		for(i=0;i<num_child; i++){ /* wait for children to start */
			child_stat=(struct child_stats *)&shmaddr[i];
			while(child_stat->flag==CHILD_STATE_HOLD)
				Poll((long long)1);
		}
		for(i=0;i<num_child; i++)
		{
			child_stat=(struct child_stats *)&shmaddr[i];
			child_stat->flag = CHILD_STATE_BEGIN; /* tell children to go */
			if(delay_start!=0)
				Poll((long long)delay_start);
			if(distributed && master_iozone)
				tell_children_begin(i);
		}
		starttime1 = time_so_far();
		goto jumpend3;
	}
jumpend3:
	getout=0;
	if(myid == (long long)getpid()){	/* Parent here */
		for( i = 0; i < num_child; i++){
			child_stat = (struct child_stats *)&shmaddr[i];
			if(distributed && master_iozone)
			{
				printf("\n\tTest running:");
				wait_dist_join();
				break;
			}
			else
			{
			   if(use_thread)
			   {
				thread_join(childids[i],(void *)&pstatus);
			   }
			   else
			   {
				wait(0);
			   }
			}
			if(!jstarttime)
				jstarttime = time_so_far(); 
		}
		jtime = (time_so_far()-jstarttime)-time_res; 
		if(jtime < (double).000001)
		{
			jtime=time_res; 
		}
	}
	total_time = (time_so_far() - starttime1)-time_res; /* Parents time */
	if(total_time < (double).000001) 
	{
		total_time=time_res; 
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
#ifdef JTIME
	total_time=total_time-jtime;/* Remove the join time */
	if(!silent) printf("\nJoin time %10.2f\n",jtime);
#endif
	
	total_kilos=0;
	ptotal=0;
	min_throughput=max_throughput=min_xfer=0;
	if(!silent) printf("\n");
	for(xyz=0;xyz<num_child;xyz++){
		child_stat=(struct child_stats *)&shmaddr[xyz];
		total_kilos+=child_stat->throughput;
		ptotal+=child_stat->actual;
		if(!min_xfer)
			min_xfer=child_stat->actual;
		if(child_stat->actual < min_xfer)
			min_xfer=child_stat->actual;
		if(!min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput < min_throughput)
			min_throughput=child_stat->throughput;
		if(child_stat->throughput > max_throughput)
			max_throughput=child_stat->throughput;
		cputime += child_stat->cputime;
		/* Get the earliest start time and latest fini time to calc. elapsed time. */
		if (child_stat->walltime < child_stat->cputime)
			child_stat->walltime = child_stat->cputime;
		if (child_stat->walltime > walltime)
			walltime = child_stat->walltime;
	}
	avg_throughput=total_kilos/num_child;
	if(cpuutilflag)
	{
		if (cputime < cputime_res)
			cputime = 0.0;
	}
	if(cpuutilflag)
		store_times (walltime, cputime);	/* Must be Before store_dvalue(). */
	store_dvalue(total_kilos);
#ifdef NO_PRINT_LLD
	if(!silent) printf("\tChildren see throughput for %2ld freaders \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2ld freaders \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#else
	if(!silent) printf("\tChildren see throughput for %2lld freaders \t= %10.2f %s/sec\n", num_child, total_kilos,unit);
	if(!silent && !distributed) printf("\tParent sees throughput for %2lld freaders \t\t= %10.2f %s/sec\n", num_child, (double)(ptotal)/total_time,unit);
#endif
	if(!silent) printf("\tMin throughput per %s \t\t\t= %10.2f %s/sec \n", port,min_throughput,unit);
	if(!silent) printf("\tMax throughput per %s \t\t\t= %10.2f %s/sec\n", port,max_throughput,unit);
	if(!silent) printf("\tAvg throughput per %s \t\t\t= %10.2f %s/sec\n", port,avg_throughput,unit);
	if(!silent) printf("\tMin xfer \t\t\t\t\t= %10.2f %s\n", min_xfer,unit);
	/* CPU% can be > 100.0 for multiple CPUs */
	if(cpuutilflag)
	{
		if(walltime == 0.0)
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 0.0);
		}
		else
		{
			if(!silent) printf("\tCPU utilization: Wall time %8.3f    CPU time %8.3f    CPU utilization %6.2f %%\n\n",
				walltime, cputime, 100.0 * cputime / walltime);
		}
	}
	if(Cflag)
	{
		for(xyz=0;xyz<num_child;xyz++)
		{
			child_stat = (struct child_stats *) &shmaddr[xyz];
			if(cpuutilflag)
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec, wall=%6.3f, cpu=%6.3f, %%=%6.2f\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit, child_stat->walltime, 
					child_stat->cputime, cpu_util(child_stat->cputime, child_stat->walltime));
			}
			else
			{
				if(!silent) printf("\tChild[%ld] xfer count = %10.2f %s, Throughput = %10.2f %s/sec\n",
					(long)xyz, child_stat->actual, unit, child_stat->throughput, unit);
			}
		}
	}
	if((!distributed) || (distributed && master_iozone))
		stop_monitor("Fread");
	/**********************************************************/
	/*************** End of fread throughput ******************/
	/**********************************************************/
	sync();
	sleep(2);
	if(restf)
		sleep((int)rest_val);
	if(distributed && master_iozone)
	{
		stop_master_listen(master_listen_socket);
		cleanup_comm();
	}
next10:
	sleep(2); /* You need this. If you stop and restart the 
		     master_listen it will fail on Linux */
	if (!no_unlink) {
		/**********************************************************/
		/* Cleanup all of the temporary files 			  */
		/* This is not really a test. It behaves like a test so   */
		/* it can unlink all of the same files that the other     */
		/* tests left hanging around.				  */
		/**********************************************************/
		/* Hooks to start the distributed Iozone client/server code */
		if(distributed)
		{
			use_thread=0;  /* Turn of any Posix threads */
			if(master_iozone)
				master_listen_socket = start_master_listen();
			else
				become_client();
		}
		if(!use_thread)
		{
		   for(xx = 0; xx< num_child ; xx++){
			chid=xx;
			childids[xx] = start_child_proc(THREAD_CLEANUP_TEST,numrecs64,reclen);
			if(childids[xx]==-1){
				printf("\nFork failed\n");
				for(xy = 0; xy< xx ; xy++){
					Kill((long long)childids[xy],(long long)SIGTERM);
				}
				exit(28);
			}
			if(childids[xx] == 0){
#ifdef _64BIT_ARCH_
				thread_cleanup_test((void *)xx);
#else
				thread_cleanup_test((void *)((long)xx));
#endif
			}	
		   }
		}
#ifndef NO_THREADS
		else
		{
		   for(xx = 0; xx< num_child ; xx++){	/* Create the children */
#ifdef _64BIT_ARCH_
			childids[xx] = mythread_create( thread_cleanup_test,xx);
#else
			childids[xx] = mythread_create( thread_cleanup_test,(void *)(long)xx);
#endif
			if(childids[xx]==-1){
				printf("\nThread create failed\n");
				for(xy = 0; xy< xx ; xy++){
					Kill((long long)myid,(long long)SIGTERM);
				}
				exit(29);
			}
		   }
		}
#endif
		if((long long)myid == getpid())
		{
			if(distributed && master_iozone)
			{
				start_master_listen_loop((int) num_child);
			}
			for(i=0;i<num_child; i++){
				child_stat = (struct child_stats *)&shmaddr[i];
						/* wait for children to start */
				while(child_stat->flag==CHILD_STATE_HOLD) 
					Poll((long long)1);
			}
			for(i=0;i<num_child; i++)
			{
				child_stat = (struct child_stats *)&shmaddr[i];
				child_stat->flag = CHILD_STATE_BEGIN;	/* tell children to go */
				if(delay_start!=0)
					Poll((long long)delay_start);
				if(distributed && master_iozone)
					tell_children_begin(i);
			}
		}
	
		getout=0;
		if((long long)myid == getpid()){	/* Parent only here */
			for( i = 0; i < num_child; i++){
				child_stat=(struct child_stats *)&shmaddr[i];
				if(distributed && master_iozone)
				{
					printf("\n\tTest cleanup:");
					wait_dist_join();
					break;
				}
				else
				{
				   if(use_thread)
				   {
					thread_join(childids[i],(void *)&pstatus);
				   }
				   else
				   {
			   		wait(0);
				   }
				}
			}
		}

		for(xyz=0;xyz<num_child;xyz++){	/* Reset state to 0 (HOLD) */
			child_stat=(struct child_stats *)&shmaddr[xyz];
			child_stat->flag = CHILD_STATE_HOLD;
		}
		sync();
		sleep(2);
		if(distributed && master_iozone)
		{
			stop_master_listen(master_listen_socket);
#ifdef Windows
			/* windows needs time before shutting down sockets */
			sleep(1);
#endif
			cleanup_comm();
		}
	}
	/********************************************************/
	/* End of cleanup					*/
	/********************************************************/
	sync();
	if(!silent) printf("\n");
	if(!silent) printf("\n");
	return;
}

	
/************************************************************************/
/* Time measurement routines.						*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
static double
time_so_far(void)
#else
static double
time_so_far()
#endif
{
#ifdef Windows
   LARGE_INTEGER freq,counter;
   double wintime,bigcounter;
   struct timeval tp;
   /* For Windows the time_of_day() is useless. It increments in 55 milli 
    * second increments. By using the Win32api one can get access to the 
    * high performance measurement interfaces. With this one can get back 
    * into the 8 to 9 microsecond resolution.
    */
   if(pit_hostname[0]){
     if (pit_gettimeofday(&tp, (struct timezone *) NULL, pit_hostname, 
 		pit_service) == -1)
         perror("pit_gettimeofday");
  	 return ((double) (tp.tv_sec)) + (((double) tp.tv_usec) * 0.000001 );
   }
   else
   {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);
        bigcounter=(double)counter.HighPart *(double)0xffffffff +
                (double)counter.LowPart;
        wintime = (double)(bigcounter/(double)freq.LowPart);
        return((double)wintime);
   }
#else
#if defined (OSFV4) || defined(OSFV3) || defined(OSFV5)
  struct timespec gp;

  if (getclock(TIMEOFDAY, (struct timespec *) &gp) == -1)
    perror("getclock");
  return (( (double) (gp.tv_sec)) +
    ( ((float)(gp.tv_nsec)) * 0.000000001 ));
#else
  struct timeval tp;

  if(pit_hostname[0]){
     if (pit_gettimeofday(&tp, (struct timezone *) NULL, pit_hostname, pit_service) == -1)
         perror("pit_gettimeofday");
  	 return ((double) (tp.tv_sec)) + (((double) tp.tv_usec) * 0.000001 );
  }
  else
  {
     if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
         perror("gettimeofday");
  	 return ((double) (tp.tv_sec)) + (((double) tp.tv_usec) * 0.000001 );
  }
#endif
#endif
}

/************************************************************************/
/* FETCHIT ()								*/
/*									*/
/* Routine to make the on chip data cache hot for this buffer. The	*/
/* on chip cache may have been blown by other code in the application	*/
/* or in the OS.  Remember, on some machines, the data cache is direct  */
/* mapped and virtual indexed.						*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void fetchit(char *buffer,long long length)
#else
void fetchit(buffer,length)
char *buffer;
long long length;
#endif
{
	char *where;
	volatile long long x[4];
	long long i;
	where=(char *)buffer;

	if(cache_line_size == 0) /* This shouldn't be needed */
		cache_line_size=CACHE_LINE_SIZE;

	for(i=0;i<(length/cache_line_size);i++)
	{
		x[(i & 3)]=*(where);
		where+=cache_line_size;
		
	}
}

/************************************************************************/
/* Verify that the buffer contains expected pattern			*/
/************************************************************************/
/* sverify == 0 means full check of pattern for every byte.             */
/* severify == 1 means partial check of pattern for each page.          */
/* sverify == 2 means no check, but partial touch for each page.        */
/************************************************************************/

#ifdef HAVE_ANSIC_C
long long 
verify_buffer(volatile char *buffer,long long length, off64_t recnum, long long recsize,unsigned long long patt,
	char sverify)
#else
long long 
verify_buffer(buffer,length, recnum, recsize,patt,sverify)
char *buffer;
long long length; 
off64_t recnum; 
long long recsize;
unsigned long long patt;
char sverify;
#endif
{
	volatile unsigned long long *where;
	volatile unsigned long long dummy;
	long *de_ibuf, *de_obuf;
	long long j,k;
	off64_t file_position=0;
	off64_t i;
	char *where2;
	char *pattern_ptr;
	long long mpattern,xx2;
	unsigned int seed;
	unsigned long x;
	unsigned long long value,value1;
	unsigned long long a= 0x01020304;
	unsigned long long b = 0x05060708;
	unsigned long long c= 0x01010101;
	unsigned long long d = 0x01010101;
	unsigned long long pattern_buf;
	int lite = 1;	/* Only validate 1 long when running 
			   de-deup validation */

	value = (a<<32) | b;
	value1 = (c<<32) | d;

	/* printf("Verify Sverify %d verify %d diag_v %d\n",sverify,verify,diag_v); */
	x=0;
	xx2=chid;
	if(share_file)
		xx2=(long long)0;
	mpattern=patt;
	pattern_buf=patt;
	where=(unsigned long long *)buffer;
	if(sverify == 2)
	{
	  for(i=0;i<(length);i+=page_size)
	  {
	      dummy = *where;
	      where+=(page_size/sizeof(long long));
	  }
	  return(0);
	}
	if(dedup)
	{
		gen_new_buf((char *)dedup_ibuf,(char *)dedup_temp, (long)recnum, (int)length,(int)dedup, (int) dedup_interior, dedup_compress, 0);
		de_ibuf = (long *)buffer;
		de_obuf = (long *)dedup_temp;
		if(lite)	/* short touch to reduce intrusion */
			length = (long) sizeof(long);
		for(i=0;i<length/sizeof(long);i++)
		{
			if(de_ibuf[i]!= de_obuf[i])
			{
				if(!silent)
#ifdef NO_PRINT_LLD
				   printf("\nDedup mis-compare at %ld\n",
					(long long)((recnum*recsize)+(i*sizeof(long))) );
#else
				   printf("\nDedup mis-compare at %lld\n",
					(long long)((recnum*recsize)+(i*sizeof(long))) );
				   printf("Found %.lx Expecting %.lx \n",de_ibuf[i], de_obuf[i]);
#endif
				return(1);
			}
		}
		return(0);
	}
	if(diag_v)
	{
		if(no_unlink)
			base_time=0;
		seed= (unsigned int)(base_time+xx2+recnum);
	        srand(seed);
	        mpattern=(long long)rand();
	        mpattern=(mpattern<<48) | (mpattern<<32) | (mpattern<<16) | mpattern;
		mpattern=mpattern+value;
	}

	/* printf("verify patt %llx CHid %d\n",mpattern,chid);*/

	where=(unsigned long long *)buffer;

	if(!verify)
		printf("\nOOPS You have entered verify_buffer unexpectedly !!! \n");

	if(sverify == 1)
	{
	  for(i=0;i<(length);i+=page_size)
	  {
	      if((unsigned long long)(*where) != (unsigned long long)((pattern_buf<<32) | pattern_buf))
	      {
		   file_position = (off64_t)( (recnum * recsize)+ i);
	printf("\n\n");
#ifdef NO_PRINT_LLD
	printf("Error in file: Found ?%lx? Expecting ?%lx? addr %lx\n",*where, (long long)((pattern_buf<<32)|pattern_buf),where);
	printf("Error in file: Position %ld \n",file_position);
	printf("Record # %ld Record size %ld kb \n",recnum,recsize/1024);
	printf("where %8.8lx loop %ld\n",where,i);
#else
	printf("Error in file: Found ?%llx? Expecting ?%llx? addr %lx\n",*where, (long long)((pattern_buf<<32)|pattern_buf),((long)where));
	printf("Error in file: Position %lld \n",file_position);
	printf("Record # %lld Record size %lld kb \n",recnum,recsize/1024);
	printf("where %px loop %lld\n",where,(long long)i);
#endif
		   return(1);
	      }
	      where+=(page_size/sizeof(long long));
	  }
	}
	if(sverify == 0)
	{
	  for(i=0;i<(length/cache_line_size);i++)
	  {
	   for(j=0;j<(cache_line_size/sizeof(long long));j++)
	   {
              if(diag_v)
	      {
		 pattern_buf=mpattern;
	      }
	      else
              {
		pattern_buf= mpattern<<32 | mpattern;
	      }

	      pattern_ptr =(char *)&pattern_buf;

	      if(*where != (unsigned long long)pattern_buf)
	      {
		   file_position = (off64_t)( (recnum * recsize))+
			((i*cache_line_size)+(j*sizeof(long long)));
		   where2=(char *)where;
		   for(k=0;k<sizeof(long long);k++){
		   	if(*where2 != *pattern_ptr)
				break;
		   	where2++;
		   	pattern_ptr++;
		   }
		   file_position+=k;
	printf("\n\n");
#ifdef NO_PRINT_LLD
	printf("Error in file: Position %ld %ld %ld \n",i,j,k);
	printf("Error in file: Position %ld \n",file_position);
	printf("Record # %ld Record size %ld kb \n",recnum,recsize/1024);
#else
	printf("Error in file: Position %lld %lld %lld \n",i,j,k);
	printf("Error in file: Position %lld \n",file_position);
	printf("Record # %lld Record size %lld kb \n",recnum,recsize/1024);
#endif
	printf("Found pattern: Char >>%c<< Expecting >>%c<<\n", *where2,*pattern_ptr);
	printf("Found pattern: Hex >>%x<< Expecting >>%x<<\n", *where2,*pattern_ptr);
		   return(1);
	      }
	      where++;
	      if(diag_v)
	         mpattern=mpattern+value1;
	   }	
	  }
	}
	return(0);
}
/************************************************************************/
/* Fill  the buffer 							*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void 
fill_buffer(char *buffer,long long length,long long pattern,char sverify,long long recnum)
#else
void 
fill_buffer(buffer,length,pattern,sverify,recnum)
char *buffer;
long long length;
long long pattern;
long long recnum;
char sverify;
#endif
{
	unsigned long long *where;
	long long i,j,xx2;
	long long mpattern;
	unsigned int seed;
	unsigned long x;
	unsigned long long value,value1;
	unsigned long long a = 0x01020304;
	unsigned long long b = 0x05060708;
	unsigned long long c = 0x01010101;
	unsigned long long d = 0x01010101;

	value = (a << 32) | b;
	value1 = (c << 32) | d;

	xx2=chid;
	if(share_file)
		xx2=(long long)0;
	x=0;
	mpattern=pattern;
	/* printf("Fill: Sverify %d verify %d diag_v %d\n",sverify,verify,diag_v);*/
	if(dedup)
	{
		gen_new_buf((char *)dedup_ibuf,(char *)buffer, (long)recnum, (int)length,(int)dedup, (int) dedup_interior, dedup_compress, 1);
		return;
	}
	if(diag_v)
	{
		/*if(client_iozone)
			base_time=0;
		*/
		if(no_unlink)
			base_time=0;
		seed= (unsigned int)(base_time+xx2+recnum);
	        srand(seed);
		mpattern=(long long)rand();
		mpattern=(mpattern<<48) | (mpattern<<32) | (mpattern<<16) | mpattern;
		mpattern=mpattern+value;
	}
	where=(unsigned long long *)buffer;
	if(sverify == 1)
	{
		for(i=0;i<(length);i+=page_size)
		{
			*where = (long long)((pattern<<32) | pattern);
			where+=(page_size/sizeof(long long)); 
			/* printf("Filling page %lld \n",i/page_size);*/
		}	
	}
	else
	{
		for(i=0;i<(length/cache_line_size);i++)
		{
			for(j=0;j<(cache_line_size/sizeof(long long));j++)
			{
				if(diag_v)
				{
					*where = (long long)(mpattern);
					mpattern=mpattern+value1;
				}
				else
					*where = (long long)((pattern<<32) | pattern);
				where++;
			}	
		}
	}
}

/************************************************************************/
/* PURGEIT() 								*/
/* 	 								*/
/* Routine to make the on chip data cache cold for this buffer.		*/
/* Remember, on some machines, the data cache is direct mapped and	*/
/* virtual indexed.							*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void 
purgeit(char *buffer,long long reclen)
#else
void 
purgeit(buffer,reclen)
char *buffer;
long long reclen;
#endif
{
	char *where;
	long rsize;
	long tsize;
	VOLATILE long long x[200];
	long i,cache_lines_per_rec;
	long cache_lines_per_cache;
	tsize = 200;
	cache_lines_per_rec = (long)(reclen/cache_line_size);
	cache_lines_per_cache = (long)(cache_size/cache_line_size);
	rsize = (long)l_min((long long)cache_lines_per_rec,(long long)cache_lines_per_cache);
#ifdef _64BIT_ARCH_
	where=(char *)pbuffer + ((unsigned long long)buffer & (cache_size-1));
#else
	where=(char *)pbuffer + ((long)buffer & ((long)cache_size-1));
#endif
	for(i=0;i<(rsize);i++)
	{
		x[i%tsize]=*(where);
		where+=cache_line_size;

	}
}

#ifdef HAVE_ANSIC_C
void
prepage(char *buffer,long long reclen)
#else
void
prepage(buffer, reclen)
char *buffer;
long long reclen; 
#endif
{
	char *where;
	long long i;
	where=(char *)buffer;
	for(i=0;i<(reclen/cache_line_size);i++)
	{
		*(where)=PATTERN;
		where+=cache_line_size;
	}
}

/************************************************************************/
/* write_perf_test ()				        		*/
/* Write and re-write test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void write_perf_test(off64_t kilo64,long long reclen ,long long *data1,long long *data2)
#else
void write_perf_test(kilo64,reclen ,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1;
long long *data2;
#endif
{
	double burst_sleep_time_till_now[2] = {0, 0};

	double starttime1;
	double writetime[2];
	double walltime[2], cputime[2];
	double qtime_start,qtime_stop;
	double hist_time;
	double compute_val = (double)0;
#ifdef unix
	double qtime_u_start,qtime_u_stop;
	double qtime_s_start,qtime_s_stop;
#endif
	long long i,j;
	off64_t numrecs64,traj_offset;
	off64_t lock_offset=0;
	long long Index = 0;
	long long file_flags = 0;
	long long traj_size;
	unsigned long long writerate[2];
	off64_t filebytes64;
	int ltest;
	char *maddr;
	char *wmaddr,*free_addr;
	char *pbuff;
	char *nbuff;
	int fd,wval;
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	int test_foo;

#ifdef unix
	qtime_u_start=qtime_u_stop=0;
	qtime_s_start=qtime_s_stop=0;
#endif
	nbuff=wmaddr=free_addr=0;
	traj_offset=0;
	test_foo=0;
	hist_time=qtime_start=qtime_stop=0;
	maddr=0;
	pbuff=mainbuffer;
	if(w_traj_flag)
	{
		filebytes64 = w_traj_fsize;
		numrecs64=w_traj_ops;
	}
	else
	{
		numrecs64 = (kilo64*1024)/reclen;
		filebytes64 = numrecs64*reclen;
	}

	if(Q_flag && (!wol_opened))
	{
		wol_opened++;
		wqfd=fopen("wol.dat","a");
		if(wqfd==0)
		{
			printf("Unable to open wol.dat\n");
			exit(40);
		}
		fprintf(wqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
		rwqfd=fopen("rwol.dat","a");
		if(rwqfd==0)
		{
			printf("Unable to open rwol.dat\n");
			exit(41);
		}
		fprintf(rwqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	fd = 0;
	if(oflag)
		file_flags = O_RDWR|O_SYNC;
	else
		file_flags = O_RDWR;
#if defined(O_DSYNC)
	if(odsync)
		file_flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux) || defined(__FreeBSD__) || defined(__DragonFly__)
	if(read_sync)
		file_flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		file_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		file_flags |=O_DIRECTIO;
#endif
#endif

/* Sanity check */
/* Some filesystems do not behave correctly and fail
 * when this sequence is performned. This is a very
 * bad thing. It breaks many applications and lurks
 * around quietly. This code should never get
 * triggered, but in the case of running iozone on
 * an NFS client, the filesystem type on the server
 * that is being exported can cause this failure.
 * If this failure happens, then the NFS client is
 * going to going to have problems, but the acutal
 * problem is the filesystem on the NFS server.
 * It's not NFS, it's the local filesystem on the
 * NFS server that is not correctly permitting
 * the sequence to function.
 */
/* _SUA_ Services for Unix Applications, under Windows
    does not have a truncate, so this must be skipped */
#if !defined(_SUA_)
        if((fd = I_OPEN(filename, (int)O_CREAT|O_WRONLY,0))<0)
        {
                printf("\nCan not open temp file: %s\n",
                        filename);
                perror("open");
                exit(44);
        }
		if(!notruncate)
		{
			if(check_filename(filename))
			{
				wval=ftruncate(fd,0);
				if(wval < 0)
				{
					printf("\n\nSanity check failed. Do not deploy this filesystem in a production environment !\n");
					exit(44);
				}
			}
			close(fd);

			if(check_filename(filename))
				unlink(filename);
		}
/* Sanity check */

#endif
	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++)
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
		if(j==0)
		{
#if defined(Windows)
	        	if(unbuffered)
		        {
		        	hand=CreateFile(filename,
				  GENERIC_READ|GENERIC_WRITE,
			          FILE_SHARE_WRITE|FILE_SHARE_READ,
				  NULL,OPEN_ALWAYS,FILE_FLAG_NO_BUFFERING|
				  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
				  NULL);
            		}
		        else
		        {
#endif
				if(!notruncate)
				{
	  		   		if((fd = I_CREAT(filename, 0640))<0)
	  		   		{
						printf("\nCan not create temp file: %s\n", 
							filename);
						perror("creat");
						exit(42);
	  		   		}
				}
#if defined(Windows)
			}
#endif
		}
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
		{
#endif
		  if(fd) 
			close(fd);
#if defined(Windows)
		}
#endif

#if defined(Windows)
	       	if(unbuffered)
	        {
	        	hand=CreateFile(filename,
			  GENERIC_READ|GENERIC_WRITE,
		          FILE_SHARE_WRITE|FILE_SHARE_READ,
			  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
			  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
			  NULL);
            	}
	        else
	        {
#endif
	  	   if((fd = I_OPEN(filename, (int)file_flags,0))<0)
	  	   {
			printf("\nCan not open temp file: %s\n", 
				filename);
			perror("open");
			exit(44);
	  	   }
#if defined(Windows)
	  	}
#endif
#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif

		if(file_lock)
			if(mylockf((int) fd, (int) 1, (int)0)!=0)
				printf("File lock for write failed. %d\n",errno);
		if(mmapflag)
		{
			maddr=(char *)initfile(fd,filebytes64,1,PROT_READ|PROT_WRITE);
		}
		if(mmap_mix)
		{
			wval=write(fd, pbuff, (size_t) page_size);
			if(wval != page_size)
			{
#ifdef NO_PRINT_LLD
			    	printf("\nError writing block %ld, fd= %d\n", (long long)0, fd);
#else
			    	printf("\nError writing block %lld, fd= %d\n", (long long)0, fd);
#endif
				if(wval==-1)
					perror("write");
				exit_code = 74; 
				signal_handler();
			}
			I_LSEEK(fd,0,SEEK_SET);
		};
		wval=fsync(fd);
		if(wval==-1){
			perror("fsync");
			signal_handler();
		}
#ifdef ASYNC_IO
		if(async_flag)
			async_init(&gc,fd,direct_flag);
#endif
		pbuff=mainbuffer;
		if(fetchon)
			fetchit(pbuff,reclen);
		if(verify || dedup || dedup_interior)
			fill_buffer(pbuff,reclen,(long long)pattern,sverify,(long long)0);
		starttime1 = time_so_far();
#ifdef unix
		if(Q_flag)
		{
			qtime_u_start=utime_so_far();
			qtime_s_start=stime_so_far();
		}
#endif
		if(w_traj_flag)
		{
			rewind(w_traj_fd);
		}
		compute_val=(double)0;
		w_traj_ops_completed=0;
		w_traj_bytes_completed=0;
		for(i=0; i<numrecs64; i++){
			if(w_traj_flag)
			{
				traj_offset=get_traj(w_traj_fd, (long long *)&traj_size,(float *)&compute_time,(long)1);
				reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
				I_LSEEK(fd,traj_offset,SEEK_SET);
			}
			if(Q_flag)
			{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,(LONG)0,0,FILE_CURRENT);
			else
#endif
				traj_offset=I_LSEEK(fd,0,SEEK_CUR);
			}
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)0,
				  lock_offset, reclen);
			}
			if((verify && diag_v) || dedup || dedup_interior)
				fill_buffer(pbuff,reclen,(long long)pattern,sverify,i);
			if(compute_flag)
				compute_val+=do_compute(compute_time);
			if(multi_buffer)
			{
				Index +=reclen;
				if(Index > (MAXBUFFERSIZE-reclen))
					Index=0;
				pbuff = mbuffer + Index;	
				if(diag_v)
				{
				   if(verify || dedup || dedup_interior)
					fill_buffer(pbuff,reclen,(long long)pattern,sverify,(long long)i);
				}
				else
				{
				   if(verify || dedup || dedup_interior)
					fill_buffer(pbuff,reclen,(long long)pattern,sverify,(long long)0);
				}
			}
			if(async_flag && no_copy_flag)
			{
				free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
				nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
				if(verify || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
				if(purge)
					purgeit(nbuff,reclen);
			}
			if(purge)
				purgeit(pbuff,reclen);
			if(Q_flag || hist_summary)
			{
				qtime_start=time_so_far();
			}
			if(mmapflag)
			{
				wmaddr = &maddr[i*reclen];
				fill_area((long long*)pbuff,(long long*)wmaddr,(long long)reclen);
				if(!mmapnsflag)
				{
				  if(mmapasflag)
				    msync(wmaddr,(size_t)reclen,MS_ASYNC);
				  if(mmapssflag)
				    msync(wmaddr,(size_t)reclen,MS_SYNC);
				}
			}
			else
			{
			  if(async_flag)
			  {
			     if(no_copy_flag)
			       async_write_no_copy(gc, (long long)fd, nbuff, reclen, (i*reclen), depth,free_addr);
			     else
			       async_write(gc, (long long)fd, pbuff, reclen, (i*reclen), depth);
			  }
			  else
			  {
#if defined(Windows)
       			    if(unbuffered)
        		    {
				WriteFile(hand, pbuff, reclen,(LPDWORD)&wval,
					0);
			    }
			    else
			    {
#endif
			    wval=write(fd, pbuff, (size_t ) reclen);
			    if(wval != reclen)
			    {
#ifdef NO_PRINT_LLD
			    	printf("\nError writing block %ld, fd= %d\n", i,
					 fd);
#else
			    	printf("\nError writing block %lld, fd= %d\n", i,
					 fd);
#endif
			    	if(wval == -1)
					perror("write");
				exit_code = 74;
				signal_handler();
			    }
#if defined(Windows)
			    }
#endif
					/* The burst work does not work for async or memory
					 * mapped IO */
					if (wval != -1)
						update_burst_sleep(fd, wval, &burst_sleep_time_till_now[j]);
			  }
			}
			if(hist_summary)
			{
				qtime_stop=time_so_far();
				hist_time =(qtime_stop-qtime_start-time_res);
				hist_insert(hist_time);
			}
			if(Q_flag)
			{
				qtime_stop=time_so_far();
				if(j==0)
#ifdef NO_PRINT_LLD
				fprintf(wqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((qtime_stop-qtime_start-time_res))*1000000,reclen);
				else
				fprintf(rwqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((qtime_stop-qtime_start-time_res))*1000000,reclen);
#else
				fprintf(wqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((qtime_stop-qtime_start-time_res))*1000000,reclen);
				else
				fprintf(rwqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((qtime_stop-qtime_start-time_res))*1000000,reclen);
#endif
			}
			w_traj_ops_completed++;
			w_traj_bytes_completed+=reclen;
			if(rlocking)
			{
				mylockr((int) fd, (int) 0, (int)0,
				  lock_offset, reclen);
			}
		}
#ifdef unix
		if(Q_flag)
		{
		   qtime_u_stop=utime_so_far();
		   qtime_s_stop=stime_so_far();
		   if(j==0)
			 fprintf(wqfd,"\nSystem time %10.3f User time %10.3f Real %10.3f  (seconds)\n",
				(qtime_s_stop-qtime_s_start)/sc_clk_tck,
				(qtime_u_stop-qtime_u_start)/sc_clk_tck,
				time_so_far()-starttime1);
		   else
			fprintf(rwqfd,"\nSystem time %10.3f User time %10.3f Real %10.3f  (seconds)\n",
				(qtime_s_stop-qtime_s_start)/sc_clk_tck,
				(qtime_u_stop-qtime_u_start)/sc_clk_tck,
				time_so_far()-starttime1);
		}
#endif

#ifdef ASYNC_IO
		if(async_flag)
		{
			end_async(gc);
			gc=0;
		}
#endif
		if(include_flush)
		{
			if(mmapflag){
				msync(maddr,(size_t)filebytes64,MS_SYNC);
			}
			else
			{
				wval=fsync(fd);
				if(wval==-1){
					perror("fsync");
					signal_handler();
				}
			}
		}
		if(file_lock)
			if(mylockf((int) fd,(int)0,(int)0))
				printf("Unlock failed %d\n",errno);
		if(include_close)
		{
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
#if defined(Windows)
			if(unbuffered)
				CloseHandle(hand);
			else
#endif
			   wval=close(fd);
			   if(wval==-1){
				perror("close");
				signal_handler();
			   }
		}
		if(inc_think)
		   writetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j]);
		else
		   writetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j])
			-compute_val;
		if(writetime[j] < (double).000001) 
		{
			writetime[j]=time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			if(mmapflag)
				msync(maddr,(size_t)filebytes64,MS_SYNC);/* Clean up before read starts */
			else
			{
				wval=fsync(fd);
				if(wval==-1){
					perror("fsync");
					signal_handler();
				}
			}
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
#if defined(Windows)
			if(unbuffered)
				CloseHandle(hand);
			else
#endif
			wval=close(fd);
			if(wval==-1){
				perror("close");
				signal_handler();
			}
		}
		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
	}
	if(OPS_flag || MS_flag){
	   filebytes64=w_traj_ops_completed;
	   /*filebytes64=filebytes64/reclen;*/
	}else
	   filebytes64=w_traj_bytes_completed;
		
        for(j=0;j<ltest;j++)
        {
		if(MS_flag)
		{
			writerate[j]=1000000.0*(writetime[j] / (double)filebytes64);
			continue;
		}
              else
            {
                  writerate[j] = 
                    (unsigned long long) ((double) filebytes64 / writetime[j]);
            }
		if(!(OPS_flag || MS_flag))
		   writerate[j] >>= 10;
	}
	data1[0]=writerate[0];
	if(noretest)
	{	
		writerate[1]=(long long) 0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)writerate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)writerate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",writerate[0]);
	if(!silent) printf("%9ld",writerate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",writerate[0]);
	if(!silent) printf("%9lld",writerate[1]);
	if(!silent) fflush(stdout);
#endif
}
/************************************************************************/
/* fwrite_perf_test ()				        		*/
/* fWrite and fre-write test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void fwrite_perf_test(off64_t kilo64,long long reclen ,long long *data1,long long *data2)
#else
void fwrite_perf_test(kilo64,reclen ,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1;
long long *data2;
#endif
{
	double burst_sleep_time_till_now[2] = {0, 0};
	double starttime1;
	double writetime[2];
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long i,j;
	off64_t numrecs64;
	long long Index = 0;
	unsigned long long writerate[2];
	off64_t filebytes64;
	FILE *stream = NULL;
	int fd;
	int wval;
	int ltest;
	char *how;
	char *stdio_buf;

	if(mmapflag || async_flag)
		return;
	numrecs64 = (kilo64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	stdio_buf=(char *)malloc((size_t)reclen);
	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++)
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
		if(j==0)
		{
			if(check_filename(filename))
				how="r+"; /* file exists, don't create and zero a new one. */
			else
				how="w+"; /* file doesn't exist. create it. */
		}
		else
			how="r+"; /* re-tests should error out if file does not exist. */
#ifdef IRIX64
		if((stream=(FILE *)fopen(filename,how)) == 0)
		{
			printf("\nCan not fdopen temp file: %s %lld\n", 
				filename,errno);
			perror("fdopen");
			exit(48);
		}
#else
		if((stream=(FILE *)I_FOPEN(filename,how)) == 0)
		{
#ifdef NO_PRINT_LLD
			printf("\nCan not fdopen temp file: %s %d\n", 
				filename,errno);
#else
			printf("\nCan not fdopen temp file: %s %d\n", 
				filename,errno);
#endif
			perror("fdopen");
			exit(49);
		}
#endif
		fd=fileno(stream);
		fsync(fd);
		setvbuf(stream,stdio_buf,_IOFBF,reclen);
		buffer=mainbuffer;
		if(fetchon)
			fetchit(buffer,reclen);
		if(verify || dedup || dedup_interior)
			fill_buffer(buffer,reclen,(long long)pattern,sverify,(long long)0);
		starttime1 = time_so_far();
		compute_val=(double)0;
		for(i=0; i<numrecs64; i++){
			if(compute_flag)
				compute_val+=do_compute(compute_time);
			if(multi_buffer)
			{
				Index +=reclen;
				if(Index > (MAXBUFFERSIZE-reclen))
					Index=0;
				buffer = mbuffer + Index;	
			}
			if((verify & diag_v) || dedup || dedup_interior)
				fill_buffer(buffer,reclen,(long long)pattern,sverify,i);
			if(purge)
				purgeit(buffer,reclen);
			if(fwrite(buffer, (size_t) reclen, 1, stream) != 1)
			{
#ifdef NO_PRINT_LLD
			    	printf("\nError fwriting block %ld, fd= %d\n", i,
					 fd);
#else
			    	printf("\nError fwriting block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("fwrite");
				exit_code = 74;
				signal_handler();
			}
			update_burst_sleep(fd, reclen, &burst_sleep_time_till_now[j]);
		}

		if(include_flush)
		{
			fflush(stream);
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
		if(include_close)
		{
			wval=fclose(stream);
			if(wval==-1){
				perror("fclose");
				signal_handler();
			}
		}
		if(inc_think)
		   writetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j]);
		else
		   writetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j])
			-compute_val;
		if(writetime[j] < (double).000001) 
		{
			writetime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			wval=fflush(stream);
			if(wval==-1){
				perror("fflush");
				signal_handler();
			}
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
			wval=fclose(stream);
			if(wval==-1){
				perror("fclose");
				signal_handler();
			}
		}

		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)(int)rest_val);
	}
	free(stdio_buf);
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
        for(j=0;j<ltest;j++)
        {
		if(MS_flag)
		{
			writerate[j]=1000000.0*(writetime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  writerate[j] = 
		      (unsigned long long) ((double) filebytes64 / writetime[j]);
            }
		if(!(OPS_flag || MS_flag))
			writerate[j] >>= 10;
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(noretest)
	{
		writerate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)writerate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)writerate[1]);
	data1[0]=writerate[0];
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",writerate[0]);
	if(!silent) printf("%9ld",writerate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",writerate[0]);
	if(!silent) printf("%9lld",writerate[1]);
	if(!silent) fflush(stdout);
#endif
}

/************************************************************************/
/* fread_perf_test				        		*/
/* fRead and fre-read test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void fread_perf_test(off64_t kilo64,long long reclen,long long *data1,long long *data2)
#else
void fread_perf_test(kilo64,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1,*data2;
#endif
{
	double starttime2;
	double readtime[2];
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long j;
	off64_t i,numrecs64;
	long long Index = 0;
	unsigned long long readrate[2];
	off64_t filebytes64;
	FILE *stream = 0;
	char *stdio_buf;
	int fd,ltest;

	if(mmapflag || async_flag)
		return;
	numrecs64 = (kilo64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	stdio_buf=(char *)malloc((size_t)reclen);

	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++ )
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}

		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
#ifdef IRIX64
		if((stream=(FILE *)fopen(filename,"r")) == 0)
		{
			printf("\nCan not fdopen temp file: %s\n", 
				filename);
			perror("fdopen");
			exit(51);
		}
#else
		if((stream=(FILE *)I_FOPEN(filename,"r")) == 0)
		{
			printf("\nCan not fdopen temp file: %s\n", 
				filename);
			perror("fdopen");
			exit(52);
		}
#endif
		fd=I_OPEN(filename,O_RDONLY,0);
		fsync(fd);
		close(fd);
		setvbuf(stream,stdio_buf,_IOFBF,reclen);
		buffer=mainbuffer;
		if(fetchon)
			fetchit(buffer,reclen);
		compute_val=(double)0;
		starttime2 = time_so_far();
		for(i=0; i<numrecs64; i++) 
		{
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                buffer = mbuffer + Index;
                        }
			if(purge)
				purgeit(buffer,reclen);
			if(fread(buffer, (size_t) reclen,1, stream) != 1)
			{
#ifdef _64BIT_ARCH_
#ifdef NO_PRINT_LLD
				printf("\nError freading block %lu %lx\n", i,
					(unsigned long long)buffer);
#else
				printf("\nError freading block %llu %llx\n", i,
					(unsigned long long)buffer);
#endif
#else
#ifdef NO_PRINT_LLD
				printf("\nError freading block %lu %lx\n", i,
					(long)buffer);
#else
				printf("\nError freading block %llu %lx\n", i,
					(long)buffer);
#endif
#endif
				perror("read");
				exit(54);
			}
			if(verify){
				if(verify_buffer(buffer,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
					exit(55);
				}
			}
		}
		if(include_flush)
			fflush(stream);
		if(include_close)
		{
			fclose(stream);
		}
		if(inc_think)
		   readtime[j] = ((time_so_far() - starttime2)-time_res);
		else
		   readtime[j] = ((time_so_far() - starttime2)-time_res)
			-compute_val;
		if(readtime[j] < (double).000001) 
		{
			readtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			fflush(stream);
			fclose(stream);
		}
		stream = NULL;
		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
    	}
	free(stdio_buf);
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
        for(j=0;j<ltest;j++)
        {
		if(MS_flag)
		{
			readrate[j]=1000000.0*(readtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  readrate[j] = 
                  (unsigned long long) ((double) filebytes64 / readtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			readrate[j] >>= 10;
	}
	data1[0]=readrate[0];
	data2[0]=1;
	if(noretest)
	{
		readrate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)readrate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)readrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",readrate[0]);
	if(!silent) printf("%9ld",readrate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",readrate[0]);
	if(!silent) printf("%9lld",readrate[1]);
	if(!silent) fflush(stdout);
#endif
}

/************************************************************************/
/* read_perf_test				        		*/
/* Read and re-fread test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void 
read_perf_test(off64_t kilo64,long long reclen,long long *data1,long long *data2)
#else
void 
read_perf_test(kilo64,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1,*data2;
#endif
{
	double starttime2;
	double compute_val = (double)0;
	double readtime[2];
	double walltime[2], cputime[2];
#ifdef unix
	double qtime_u_start,qtime_u_stop;
	double qtime_s_start,qtime_s_stop;
#endif
	long long j;
	long long traj_size;
	off64_t i,numrecs64,traj_offset;
	off64_t lock_offset=0;
	long long Index = 0;
	unsigned long long readrate[2];
	off64_t filebytes64;
	volatile char *buffer1;
	char *nbuff;
	char *maddr;
	char *wmaddr;
	int fd,open_flags;
	int test_foo,ltest;
	long wval;
	double qtime_start,qtime_stop;
	double hist_time;
#ifdef ASYNC_IO
	struct cache *gc=0;

#else
	long long *gc=0;
#endif
#ifdef unix
	qtime_u_start=qtime_u_stop=0;
	qtime_s_start=qtime_s_stop=0;
#endif
	hist_time=qtime_start=qtime_stop=0;
	maddr=0;
	traj_offset=0;
	test_foo=0;
	numrecs64 = (kilo64*1024)/reclen;

	open_flags = O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		open_flags |=O_RSYNC|O_SYNC;
#endif
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		open_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		open_flags |=O_DIRECTIO;
#endif
#endif
	if(r_traj_flag)
	{
		numrecs64=r_traj_ops;
		filebytes64 = r_traj_fsize;
	} else
		filebytes64 = numrecs64*reclen;
	fd = 0;
	if(Q_flag && (!rol_opened))
	{
		rol_opened++;
		rqfd=fopen("rol.dat","a");
		if(rqfd==0)
		{
			printf("Unable to open rol.dat\n");
			exit(56);
		}
		fprintf(rqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
		rrqfd=fopen("rrol.dat","a");
		if(rrqfd==0)
		{
			printf("Unable to open rrol.dat\n");
			exit(57);
		}
		fprintf(rrqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	/* 
	 * begin real testing
	 */
	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++ )
	{

		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}

		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
#if defined(Windows)
	       	if(unbuffered)
	        {
	        	hand=CreateFile(filename,
			  GENERIC_READ|GENERIC_WRITE,
		          FILE_SHARE_WRITE|FILE_SHARE_READ,
			  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
			  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
			  NULL);
            	}
	        else
	        {
#endif
		if((fd = I_OPEN(filename, open_flags,0))<0)
		{
			printf("\nCan not open temporary file %s for read\n",filename);
			perror("open");
			exit(58);
		}
#if defined(Windows)
		}
#endif
#ifdef ASYNC_IO
		if(async_flag)
			async_init(&gc,fd,direct_flag);
#endif

#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
		if(file_lock)
			if(mylockf((int) fd, (int) 1, (int)1) != 0)
				printf("File lock for read failed. %d\n",errno);
		if(mmapflag)
		{
			maddr=(char *)initfile(fd,filebytes64,0,PROT_READ);
		}
#if defined(Windows)
		if(!unbuffered)
#endif
		  fsync(fd);
		/* 
		 *  Need to prime the instruction cache & TLB
		 */
		nbuff=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
#if defined(Windows)
		if(!unbuffered)
		{
#endif
		if(read(fd, (void *)nbuff, (size_t) reclen) != reclen)
		{
#ifdef _64BIT_ARCH_
			printf("\nError reading block %d %llx\n", 0,
				(unsigned long long)nbuff);
#else
			printf("\nError reading block %d %lx\n", 0,
				(long)nbuff);
#endif
			perror("read");
			exit(60);
		}
		I_LSEEK(fd,0,SEEK_SET);
#if defined(Windows)
		}
		if(unbuffered)
			SetFilePointer(hand,(LONG)0,0,FILE_BEGIN);
#endif
		nbuff=mainbuffer;

		if(fetchon)
			fetchit(nbuff,reclen);
		starttime2 = time_so_far();
#ifdef unix
		if(Q_flag || hist_summary)
		{
			qtime_u_start=utime_so_far();
			qtime_s_start=stime_so_far();
		}
#endif
		if(r_traj_flag)
		{
			rewind(r_traj_fd);
		}
		compute_val=(double)0;
		r_traj_ops_completed=0;
		r_traj_bytes_completed=0;
		for(i=0; i<numrecs64; i++) 
		{
			if(disrupt_flag && ((i%DISRUPT)==0))
			{
#if defined(Windows)
	
				if(unbuffered)
				   disruptw(hand);
				else
				   disrupt(fd);
#else
				disrupt(fd);
#endif
			}
			if(r_traj_flag)
			{
				traj_offset=get_traj(r_traj_fd, (long long *)&traj_size,(float *)&compute_time, (long)0);
				reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
				I_LSEEK(fd,traj_offset,SEEK_SET);
			}
			if(Q_flag)
			{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,(LONG)0,0,FILE_CURRENT);
			else
#endif
				traj_offset=I_LSEEK(fd,0,SEEK_CUR);
			}
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                nbuff = mbuffer + Index;
                        }
			if(purge)
				purgeit(nbuff,reclen);
			if(Q_flag || hist_summary)
				qtime_start=time_so_far();
			if(mmapflag)
			{
				wmaddr=&maddr[i*reclen];
				fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
			}
			else
			{
			  if(async_flag)
			  {
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (i*reclen), reclen,
			    	1LL,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (i*reclen),reclen,
			    	1LL,(numrecs64*reclen),depth);
			  }
			  else
			  {
#if defined(Windows)
       			    if(unbuffered)
        		    {
				ReadFile(hand, nbuff, reclen,(LPDWORD)&wval,
					0);
			    }
			    else
#endif
			      wval=read((int)fd, (void*)nbuff, (size_t) reclen);
			    if(wval != reclen)
			    {
#ifdef _64BIT_ARCH_
#ifdef NO_PRINT_LLD
				printf("\nError reading block %ld %lx\n", i,
					(unsigned long long)nbuff);
#else
				printf("\nError reading block %lld %llx\n", i,
					(unsigned long long)nbuff);
#endif
#else
#ifdef NO_PRINT_LLD
				printf("\nError reading block %ld %x\n", i,
					(long)nbuff);
#else
				printf("\nError reading block %lld %lx\n", i,
					(long)nbuff);
#endif
#endif
				perror("read");
				exit(61);
			    }
			  }
			}
			if(verify) {
			  if(async_flag && no_copy_flag)
			  {
				if(verify_buffer(buffer1,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
					exit(62);
				}
			  }
			  else
			  {
				if(verify_buffer(nbuff,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
					exit(63);
				}
			  }
			}
			if(async_flag && no_copy_flag)
				async_release(gc);
			buffer1=0;
			if(hist_summary)
			{
				qtime_stop=time_so_far();
				hist_time =(qtime_stop-qtime_start-time_res);
				hist_insert(hist_time);
			}
			if(Q_flag)
			{
				qtime_stop=time_so_far();
				if(j==0)
#ifdef NO_PRINT_LLD
				fprintf(rqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,(qtime_stop-qtime_start-time_res)*1000000,reclen);
				else
				fprintf(rrqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,(qtime_stop-qtime_start-time_res)*1000000,reclen);
#else
				fprintf(rqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,(qtime_stop-qtime_start-time_res)*1000000,reclen);
				else
				fprintf(rrqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,(qtime_stop-qtime_start-time_res)*1000000,reclen);
#endif
			}
			r_traj_ops_completed++;
			r_traj_bytes_completed+=reclen;
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
		if(file_lock)
			if(mylockf((int) fd, (int) 0, (int)1))
				printf("Read unlock failed. %d\n",errno);
#ifdef unix
		if(Q_flag)
		{
		   qtime_u_stop=utime_so_far();
		   qtime_s_stop=stime_so_far();
		   if(j==0)
			 fprintf(rqfd,"\nSystem time %10.3f User time %10.3f Real %10.3f  (seconds)\n",
				(qtime_s_stop-qtime_s_start)/sc_clk_tck,
				(qtime_u_stop-qtime_u_start)/sc_clk_tck,
				time_so_far()-starttime2);
		   else
			fprintf(rrqfd,"\nSystem time %10.3f User time %10.3f Real %10.3f  (seconds)\n",
				(qtime_s_stop-qtime_s_start)/sc_clk_tck,
				(qtime_u_stop-qtime_u_start)/sc_clk_tck,
				time_so_far()-starttime2);
		}
#endif
#ifdef ASYNC_IO
		if(async_flag)
		{
			end_async(gc);
			gc=0;
		}
#endif
		if(include_flush)
		{
			if(mmapflag)
				msync(maddr,(size_t)filebytes64,MS_SYNC);
			else
				fsync(fd);
		}
		if(include_close)
		{
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
#if defined(Windows)
			if(unbuffered)
				CloseHandle(hand);
			else
#endif
			close(fd);
		}
		if(inc_think)
		   readtime[j] = ((time_so_far() - starttime2)-time_res);
		else
		   readtime[j] = ((time_so_far() - starttime2)-time_res)-compute_val;
		if(readtime[j] < (double).000001) 
		{
			readtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			if(mmapflag)
				msync(maddr,(size_t)filebytes64,MS_SYNC);
			else
				fsync(fd);
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
#if defined(Windows)
			if(unbuffered)
				CloseHandle(hand);
			else
#endif
			close(fd);
		}
		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
    	}
	if(OPS_flag || MS_flag){
	   filebytes64=r_traj_ops_completed;
	   /*filebytes64=filebytes64/reclen;*/
	} else
	   filebytes64=r_traj_bytes_completed;

        for(j=0;j<ltest;j++)
        {
		if(MS_flag)
		{
			readrate[j]=1000000.0*(readtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  readrate[j] = 
                  (unsigned long long) ((double) filebytes64 / readtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			readrate[j] >>= 10;
			
	}
	data1[0]=readrate[0];
	data2[0]=1;
	if(noretest)
	{
		readrate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)readrate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)readrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",readrate[0]);
	if(!silent) printf("%9ld",readrate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",readrate[0]);
	if(!silent) printf("%9lld",readrate[1]);
	if(!silent) fflush(stdout);
#endif
}


/************************************************************************/
/* random_perf_test				        		*/
/* Random read and write test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void random_perf_test(off64_t kilo64,long long reclen,long long *data1,long long *data2)
#else
void random_perf_test(kilo64,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1, *data2;
#endif
{
	double randreadtime[2];
	double starttime2;
	double walltime[2], cputime[2];
	double compute_val = (double)0;
#if defined (bsd4_2) || defined(Windows)
	long long rand1,rand2,rand3;
#endif
	unsigned long long big_rand;
	long long j;
	off64_t i,numrecs64;
	long long Index=0;
	double burst_sleep_time_till_now[2] = {0, 0};
	int flags;
	unsigned long long randreadrate[2];
	off64_t filebytes64;
	off64_t lock_offset=0;
	volatile char *buffer1;
	char *wmaddr,*nbuff;
	char *maddr,*free_addr;
	int fd,wval;
	long long *recnum= 0;
#if defined(VXFS) || defined(solaris)
	int test_foo=0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif
#ifdef MERSENNE
    unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL};
    unsigned long long length=4;
#endif

	maddr=free_addr=0;
	numrecs64 = (kilo64*1024)/reclen;
#ifdef MERSENNE
       init_by_array64(init, length);
#else
#ifdef bsd4_2
        srand(0);
#else
#ifdef Windows
        srand(0);
#else
        srand48(0);
#endif
#endif
#endif
        recnum = (long long *)malloc(sizeof(*recnum)*numrecs64);
        if (recnum){
             /* pre-compute random sequence based on 
		Fischer-Yates (Knuth) card shuffle */
            for(i = 0; i < numrecs64; i++){
                recnum[i] = i;
            }
            for(i = 0; i < numrecs64; i++) {
                long long tmp;
#ifdef MERSENNE
      	       big_rand=genrand64_int64();
#else
#ifdef bsd4_2
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
#ifdef Windows
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
               big_rand = lrand48();
#endif
#endif
#endif
               big_rand = big_rand%numrecs64;
               tmp = recnum[i];
               recnum[i] = recnum[big_rand];
               recnum[big_rand] = tmp;
            }
        }
	else
	{
		fprintf(stderr,"Random uniqueness fallback.\n");
	}
	flags = O_RDWR;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
	fd=0;
	if(oflag)
		flags |= O_SYNC;
#if defined(O_DSYNC)
	if(odsync)
		flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif
	filebytes64 = numrecs64*reclen;
	for( j=0; j<2; j++ )
	{
		if(j==0)
			flags |=O_CREAT;
		if (no_write && (j == 1))
			continue;
		if(cpuutilflag)
		{
		     walltime[j] = time_so_far();
		     cputime[j]  = cputime_so_far();
	     }
	     if(Uflag) /* Unmount and re-mount the mountpoint */
	     {
			purge_buffer_cache();
	     }
	     if((fd = I_OPEN(filename, ((int)flags),0640))<0){
			printf("\nCan not open temporary file for read/write\n");
			perror("open");
			exit(66);
	     }
#ifdef ASYNC_IO
		if(async_flag)
			async_init(&gc,fd,direct_flag);
#endif

#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
	     if(mmapflag)
	     {
			maddr=(char *)initfile(fd,filebytes64,0,PROT_READ|PROT_WRITE);
	     }
	     nbuff=mainbuffer;
	     if(fetchon)
		   fetchit(nbuff,reclen);
#ifdef MERSENNE
    	    init_by_array64(init, length);
#else
#ifdef bsd4_2
	     srand(0);
#else
#ifdef Windows
             srand(0);
#else
             srand48(0);
#endif
#endif
#endif
	     compute_val=(double)0;
	     starttime2 = time_so_far();
	     if ( j==0 ){ /* start read */
		for(i=0; i<numrecs64; i++) {
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                nbuff = mbuffer + Index;
                        }
			if(purge)
				purgeit(nbuff,reclen);
                        if (recnum) {
				offset64 = reclen * (long long)recnum[i];
                        }
			else
			{

#ifdef MERSENNE
      			   big_rand =genrand64_int64();
			   offset64 = reclen * (big_rand%numrecs64);
#else
#ifdef bsd4_2
			   rand1=(long long)rand();
			   rand2=(long long)rand();
			   rand3=(long long)rand();
			   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
                           offset64 = reclen * (big_rand%numrecs64);
#else
#ifdef Windows
			   rand1=(long long)rand();
			   rand2=(long long)rand();
			   rand3=(long long)rand();
			   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
                           offset64 = reclen * (big_rand%numrecs64);
#else
			   offset64 = reclen * (lrand48()%numrecs64);
#endif
#endif
#endif
			}

			if( !(h_flag || k_flag || mmapflag))
			{
			   if(I_LSEEK( fd, offset64, SEEK_SET )<0)
			   {
				perror("lseek");
				exit(68);
			   };
			}
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
			if(mmapflag)
			{
				wmaddr=&maddr[offset64];
				fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
			}
			else
			{
			  if(async_flag)
			  {
			     if(no_copy_flag)
			        async_read_no_copy(gc, (long long)fd, &buffer1, offset64,reclen,
			    	  0LL,(numrecs64*reclen),depth);
			     else
				 async_read(gc, (long long)fd, nbuff, (offset64),reclen,
					    	0LL,(numrecs64*reclen),0LL);
			  }
			  else
			  {
		  	     if(read(fd, (void *)nbuff, (size_t)reclen) != reclen)
		  	     {
#ifdef NO_PRINT_LLD
				 printf("\nError reading block at %ld\n",
					 offset64); 
#else
				 printf("\nError reading block at %lld\n",
					 offset64); 
#endif
				 perror("read");
				 exit(70);
		 	     }
			  }
			}
			if(verify){
			  if(async_flag && no_copy_flag)
			  {
				if(verify_buffer(buffer1,reclen,(off64_t)offset64/reclen,reclen,(long long)pattern,sverify)){
					exit(71);
				}
			  }
			  else
			  {
				if(verify_buffer(nbuff,reclen,(off64_t)offset64/reclen,reclen,(long long)pattern,sverify)){
					exit(72);
				}
			  }
			}
			if(async_flag && no_copy_flag)
				async_release(gc);
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
		}
	     } /* start write */
	     else
	     {
			if(verify || dedup || dedup_interior)
				fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
			for(i=0; i<numrecs64; i++) 
			{
				if(compute_flag)
					compute_val+=do_compute(compute_time);
                        	if(multi_buffer)
                        	{
                               	    Index +=reclen;
                               	    if(Index > (MAXBUFFERSIZE-reclen))
                               	         Index=0;
                               	    nbuff = mbuffer + Index;
                        	}
                                if (recnum) {
				  offset64 = reclen * (long long)recnum[i];
                                }
			        else
			        {
#ifdef bsd4_2
				  rand1=(long long)rand();
				  rand2=(long long)rand();
				  rand3=(long long)rand();
				  big_rand=(rand1<<32)|(rand2<<16)|(rand3);
				  offset64 = reclen * (big_rand%numrecs64);
#else
#ifdef Windows
				  rand1=(long long)rand();
				  rand2=(long long)rand();
				  rand3=(long long)rand();
				  big_rand=(rand1<<32)|(rand2<<16)|(rand3);
				  offset64 = reclen * (big_rand%numrecs64);
#else
				  offset64 = reclen * (lrand48()%numrecs64);
#endif
#endif
				}
				if(async_flag && no_copy_flag)
				{
					free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
					nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
					if(verify || dedup || dedup_interior)
						fill_buffer(nbuff,reclen,(long long)pattern,sverify,offset64/reclen);
				}
				if(purge)
					purgeit(nbuff,reclen);

				if((verify & diag_v) || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,offset64/reclen);

				if (!(h_flag || k_flag || mmapflag))
				{
				  I_LSEEK( fd, offset64, SEEK_SET );
				}
				if(rlocking)
				{
					lock_offset=I_LSEEK(fd,0,SEEK_CUR);
					mylockr((int) fd, (int) 1, (int)0,
					  lock_offset, reclen);
				}
				if(mmapflag)
				{
					wmaddr=&maddr[offset64];
					fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
					if(!mmapnsflag)
					{
					  	if(mmapasflag)
						    	msync(wmaddr,(size_t)reclen,MS_ASYNC);
					  	if(mmapssflag)
					    		msync(wmaddr,(size_t)reclen,MS_SYNC);
					}
				}
				else
				{
			  		if(async_flag)
					{
			     		   if(no_copy_flag)
			       		      async_write_no_copy(gc, (long long)fd, nbuff, reclen, offset64, 
					   	depth,free_addr);
					   else
			      			async_write(gc, (long long)fd, nbuff, reclen, offset64, depth);
			  		}
			  		else
			  		{
			  		  wval=write(fd, nbuff,(size_t)reclen);
			  		  if(wval != reclen)
			  		  {
#ifdef NO_PRINT_LLD
						printf("\nError writing block at %ld\n",
							offset64); 
#else
						printf("\nError writing block at %lld\n",
							offset64); 
#endif
						if(wval==-1)
							perror("write");
						exit_code = 74;
						signal_handler();
			 		  }
					  if (wval != -1)
						update_burst_sleep(fd, wval, &burst_sleep_time_till_now[j]);
					}
				}
				if(rlocking)
				{
					mylockr((int) fd, (int) 0, (int)0,
					  lock_offset, reclen);
				}
			}
	     } 	/* end of modifications	*kcollins:2-5-96 */
#ifdef ASYNC_IO
	     if(async_flag)
	     {
		end_async(gc);
	        gc=0;
             }	
#endif
	     if(include_flush)
	     {
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);/* Clean up before read starts running */
		else
		{
	     		wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
	     }
	     if(include_close)
	     {
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
		wval=close(fd);
		if(wval==-1){
			perror("close");
			signal_handler();
		}
	     }
	     if(inc_think)
	        randreadtime[j] = ((time_so_far() - starttime2)-time_res - burst_sleep_time_till_now[j]);
	     else
	        randreadtime[j] = ((time_so_far() - starttime2)-time_res - burst_sleep_time_till_now[j])-
			compute_val;
	     if(randreadtime[j] < (double).000001) 
	     {
			randreadtime[j]=time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
	     }
	    if(!include_close)
	    {
		if(mmapflag)
		{
			msync(maddr,(size_t)filebytes64,MS_SYNC);/* Clean up before read starts running */
		}
		else
		{
	     		wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
		wval=close(fd);
		if(wval==-1){
			perror("close");
			signal_handler();
		}
 	    }
            if(cpuutilflag)
	    {
	    	cputime[j]  = cputime_so_far() - cputime[j];
	    	if (cputime[j] < cputime_res)
			cputime[j] = 0.0;
	    	walltime[j] = time_so_far() - walltime[j];
		if (walltime[j] < cputime[j])
		   walltime[j] = cputime[j];
	    }
	    if(restf)
		sleep((int)rest_val);
    	}
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
        for(j=0;j<2;j++)
        {
	    if(no_write && (j==1))
	    {
	        randreadrate[1] = 0.0;
		continue;
	    }
	    if(MS_flag)
	    {
		randreadrate[j]=1000000.0*(randreadtime[j] / (double)filebytes64);
		continue;
	    }
            else
            {
                  randreadrate[j] = 
		      (unsigned long long) ((double) filebytes64 / randreadtime[j]);
            }
	    if(!(OPS_flag || MS_flag))
		randreadrate[j] >>= 10;
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
        if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)randreadrate[0]);
        if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)randreadrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",randreadrate[0]);
	if(!silent) printf("%9ld",randreadrate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",randreadrate[0]);
	if(!silent) printf("%9lld",randreadrate[1]);
	if(!silent) fflush(stdout);
#endif
	if(recnum)
		free(recnum);
}

/************************************************************************/
/* reverse_perf_test				        		*/
/* Reverse read test							*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void reverse_perf_test(off64_t kilo64,long long reclen,long long *data1,long long *data2)
#else
void reverse_perf_test(kilo64,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1,*data2;
#endif
{
	double revreadtime[2];
	double starttime2;
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long j;
	off64_t i,numrecs64;
	long long Index = 0;
	unsigned long long revreadrate[2];
	off64_t filebytes64;
	off64_t lock_offset=0;
	int fd,open_flags;
	char *maddr,*wmaddr;
	volatile char *buffer1;
	int ltest;
	char *nbuff;
#if defined(VXFS) || defined(solaris)
	int test_foo=0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	maddr=wmaddr=0;
	open_flags=O_RDONLY;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		open_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		open_flags |=O_DIRECTIO;
#endif
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		open_flags |=O_RSYNC|O_SYNC;
#endif
	numrecs64 = (kilo64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	fd = 0;
	if(noretest)
		ltest=1;
	else
		ltest=2;
	for( j=0; j<ltest; j++ )
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
	 	if((fd = I_OPEN(filename, open_flags,0))<0){
	 		printf("\nCan not open temporary file for read\n");
	 		perror("open");
	 		exit(75);
	 	}
#ifdef ASYNC_IO
		if(async_flag)
			async_init(&gc,fd,direct_flag);
#endif

#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
		if(mmapflag)
		{
			maddr=(char *)initfile(fd,filebytes64,0,PROT_READ);
		}
		fsync(fd);
		nbuff=mainbuffer;
		mbuffer=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
		starttime2 = time_so_far();
		if (!(h_flag || k_flag || mmapflag))
		{
		  if(check_filename(filename))
		  {
		  	if(I_LSEEK( fd, -reclen, SEEK_END )<0)
		  	{
				perror("lseek");
				exit(77);
		  	};
		  }
		  else
		  {
		  	if(I_LSEEK( fd, filebytes64-reclen, SEEK_SET )<0)
		  	{
				perror("lseek");
				exit(77);
		  	};
  		  }
		}
	        compute_val=(double)0;
		for(i=0; i<numrecs64; i++) 
		{
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                nbuff = mbuffer + Index;
                        }

			if(purge)
				purgeit(nbuff,reclen);
			if(mmapflag)
			{
				wmaddr = &maddr[((numrecs64-1)-i)*reclen];
				fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
			}
			else
			if(async_flag)
			{
			    if(no_copy_flag)
			       async_read_no_copy(gc, (long long)fd, &buffer1, ((((numrecs64-1)-i)*reclen)),
			          reclen, -1LL,(numrecs64*reclen),depth);
			    else
			       async_read(gc, (long long)fd, nbuff, (((numrecs64-1)-i)*reclen),
			       	  reclen,-1LL,(numrecs64*reclen),depth);
			}else
			{
				if(read((int)fd, (void*)nbuff, (size_t) reclen) != reclen)
				{
#ifdef NO_PRINT_LLD
					printf("\nError reading block %ld\n", i); 
#else
					printf("\nError reading block %lld\n", i); 
#endif
					perror("read");
					exit(79);
				}
			}
			if(verify){
			   if(async_flag && no_copy_flag)
			   {
				if(verify_buffer(buffer1,reclen,(off64_t)(numrecs64-1)-i,reclen,(long long)pattern,sverify)){
					exit(80);
				}
			   }
			   else
			   {
				if(verify_buffer(nbuff,reclen,(off64_t)(numrecs64-1)-i,reclen,(long long)pattern,sverify)){
					exit(81);
				}
			   }
			}
			if(async_flag && no_copy_flag)
				async_release(gc);
			if(rlocking)
			{
				mylockr((int) fd, (int) 0, (int)1,
				  lock_offset, reclen);
			}
			if (!(h_flag || k_flag || mmapflag))
			{
			  I_LSEEK( fd, (off64_t)-2*reclen, SEEK_CUR );
			}
		}
#ifdef ASYNC_IO
		if(async_flag)
		{
			end_async(gc);
			gc=0;
		}
#endif
		if(include_flush)	
		{
			if(mmapflag)
				msync(maddr,(size_t)filebytes64,MS_SYNC);
			else
				fsync(fd);
		}
		if(include_close)
		{
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
			close(fd);
		}
		if(inc_think)
		   revreadtime[j] = ((time_so_far() - starttime2)-time_res);
		else
		   revreadtime[j] = ((time_so_far() - starttime2)-time_res)
			-compute_val;
		if(revreadtime[j] < (double).000001) 
		{
			revreadtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			if(mmapflag)
				msync(maddr,(size_t)filebytes64,MS_SYNC);
			else
				fsync(fd);
			if(mmapflag)
			{
				mmap_end(maddr,(unsigned long long)filebytes64);
			}
			close(fd);
		}
		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
        }
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	for(j=0;j<ltest;j++){
		if(MS_flag)
		{
			revreadrate[j]=1000000.0*(revreadtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  revreadrate[j] = 
		      (unsigned long long) ((double) filebytes64 / revreadtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			revreadrate[j] >>= 10;
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)revreadrate[0]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",revreadrate[0]);
#else
	if(!silent) printf("%9lld",revreadrate[0]);
#endif
	if(!silent) fflush(stdout);
}

/************************************************************************/
/* rewriterec_perf_test				        		*/
/* Re-write the same record 						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void rewriterec_perf_test(off64_t kilo64 ,long long reclen,long long *data1,long long *data2)
#else
void rewriterec_perf_test(kilo64 ,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1,*data2;
#endif
{
	double burst_sleep_time_till_now = 0;
	double writeintime;
	double starttime1;
	double walltime, cputime;
	double compute_val = (double)0;
	long long i;
	off64_t numrecs64;
	long long flags;
	long long Index=0;
	unsigned long long writeinrate;
	off64_t filebytes64;
	off64_t lock_offset=0;
	int fd,wval;
	char *maddr;
	char *wmaddr,*free_addr,*nbuff;
#if defined(VXFS) || defined(solaris)
	int test_foo=0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	walltime=cputime=0;
	maddr=wmaddr=free_addr=nbuff=0;
	numrecs64 = (kilo64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
/*	flags = O_RDWR|O_CREAT|O_TRUNC;*/
	flags = O_RDWR|O_CREAT;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
	if(oflag)
		flags |= O_SYNC;
#if defined(O_DSYNC)
	if(odsync)
		flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif
/*
	if (!no_unlink)
	{
	   if(check_filename(filename))
		unlink(filename);
	}
*/
	if(Uflag) /* Unmount and re-mount the mountpoint */
	{
		purge_buffer_cache();
	}
        if((fd = I_OPEN(filename, (int)flags,0640))<0)
        {
                    printf("\nCan not open temporary file %s for write.\n",filename);
		    perror("open");
                    exit(84);
        }
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,filebytes64,1,PROT_READ|PROT_WRITE);
	}
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
	wval=fsync(fd);
	if(wval==-1){
		perror("fsync");
		signal_handler();
	}
	nbuff=mainbuffer;
	mbuffer=mainbuffer;
	if(fetchon)
		fetchit(nbuff,reclen);
	/*
	wval=write(fd, nbuff, (size_t) reclen);
	if(wval != reclen)
	{
#ifdef NO_PRINT_LLD
	    	printf("\nError writing block %ld, fd= %d\n", 0, fd);
#else
	    	printf("\nError writing block %lld, fd= %d\n", 0, fd);
#endif
		if(wval==-1)
			perror("write");
		exit_code = 74;
		signal_handler();
	}
	*/
	if(verify || dedup || dedup_interior)
		fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = time_so_far();
		cputime  = cputime_so_far();
	}
	for(i=0; i<numrecs64; i++){
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(compute_time);
        	if(multi_buffer)
        	{
                	Index +=reclen;
                        if(Index > (MAXBUFFERSIZE-reclen))
                                Index=0;
                        nbuff = mbuffer + Index;
                }
		if(async_flag && no_copy_flag)
		{
			free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
			nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
			if(verify || dedup || dedup_interior)
				fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
		}
		if((verify & diag_v) || dedup || dedup_interior)
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
		if(purge)
			purgeit(nbuff,reclen);
		if(mmapflag)
		{
			wmaddr = &maddr[0];
			fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
			if(!mmapnsflag)
			{
			  if(mmapasflag)
			    msync(wmaddr,(size_t)reclen,MS_ASYNC);
			  if(mmapssflag)
			    msync(wmaddr,(size_t)reclen,MS_SYNC);
			}
		}
		else
		{
			  if(async_flag)
			  {
			     if(no_copy_flag)
			       async_write_no_copy(gc, (long long)fd, nbuff, reclen, (i*reclen), depth,free_addr);
			     else
			       async_write(gc, (long long)fd, nbuff, reclen, (i*reclen), depth);
			  }
			  else
			  {
			       wval=write(fd, nbuff, (size_t) reclen);
			       if(wval != reclen)
			       {
#ifdef NO_PRINT_LLD
		    		   printf("\nError writing block %ld, fd= %d\n", i, fd);
#else
		    		   printf("\nError writing block %lld, fd= %d\n", i, fd);
#endif
				   if(wval==-1)
				   	perror("write");
				   exit_code = 74;
				   signal_handler();
			       }
			       if (wval != -1)
						update_burst_sleep(fd, wval, &burst_sleep_time_till_now);
			  }
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)0,
			  lock_offset, reclen);
		}
		if (!(h_flag || k_flag || mmapflag))
		{
		  I_LSEEK(fd, (off64_t)0,SEEK_SET);
		}
	}

#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);/* Clean up before read starts running */
		else
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
	}
	if(include_close)
	{
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
		wval=close(fd);
		if(wval==-1){
			perror("close");
			signal_handler();
		}
	}
	if(inc_think)
	   writeintime = ((time_so_far() - starttime1)-time_res);
	else
	   writeintime = ((time_so_far() - starttime1)-time_res)-
		compute_val;
	if(cpuutilflag)
	{
		cputime  = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		walltime = time_so_far() - walltime;
		if (walltime < cputime)
		   walltime = cputime;
	}
	writeintime  -= burst_sleep_time_till_now;
	if(writeintime < (double).000001) 
	{
		writeintime= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(!include_close)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);/* Clean up before read starts running */
		else
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
		wval=close(fd);
		if(wval==-1){
			perror("close");
			signal_handler();
		}

	}

	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	if(MS_flag)
	{
		writeinrate=1000000.0*(writeintime / (double)filebytes64);
	}
      else
      {
            writeinrate = (unsigned long long) ((double) filebytes64 / writeintime);
      }
	if(!(OPS_flag || MS_flag))
		writeinrate >>= 10;
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime, cputime);
	store_value((off64_t)writeinrate);
#ifdef NO_PRINT_LLD
	if(!silent) printf(" %9ld",writeinrate);
#else
	if(!silent) printf(" %9lld",writeinrate);
#endif
	if(!silent) fflush(stdout);
	if(restf)
		sleep((int)rest_val);
}

/************************************************************************/
/* read_stride_perf_test			        		*/
/* Read with a constant stride test					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void read_stride_perf_test(off64_t kilos64,long long reclen,long long *data1,long long *data2)
#else
void read_stride_perf_test(kilos64,reclen,data1,data2)
off64_t kilos64;
long long reclen;
long long *data1, *data2;
#endif
{
	double strideintime;
	double starttime1;
	double compute_val = (double)0;
	double walltime, cputime;
	off64_t numrecs64,current_position;
	long long Index = 0;
	off64_t i,savepos64 = 0;
	unsigned long long strideinrate;
	off64_t filebytes64;
	off64_t lock_offset=0;
	long long uu;
	off64_t stripewrap=0;
	int fd,open_flags;
	volatile char *buffer1;
	char *nbuff;
	char *maddr;
	char *wmaddr;
#if defined(VXFS) || defined(solaris)
	int test_foo=0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	walltime=cputime=0;
	nbuff=maddr=wmaddr=0;
	open_flags=O_RDONLY;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		open_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		open_flags |=O_DIRECTIO;
#endif
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		open_flags |=O_RSYNC|O_SYNC;
#endif
	next64 = (off64_t)0;
	numrecs64 = (kilos64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	if(Uflag) /* Unmount and re-mount the mountpoint */
	{
		purge_buffer_cache();
	}
        if((fd = I_OPEN(filename, (int)open_flags, 0640))<0)
        {
                    printf("\nCan not open temporary file for read\n");
		    perror("open");
                    exit(86);
        }
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif

#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,filebytes64,0,PROT_READ);
	}
	fsync(fd);
	current_position=0;
	nbuff=mainbuffer;
	mbuffer=mainbuffer;
	if(fetchon)
		fetchit(nbuff,reclen);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = time_so_far();
		cputime  = cputime_so_far();
	}
	for(i=0; i<numrecs64; i++){
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(compute_time);
        	if(multi_buffer)
                {
                       Index +=reclen;
                       if(Index > (MAXBUFFERSIZE-reclen))
                                Index=0;
                       nbuff = mbuffer + Index;
                }
		if(purge)
			purgeit(nbuff,reclen);
		if(verify)
		{
			savepos64=current_position/reclen;
		}
		if(mmapflag)
		{
			wmaddr = &maddr[current_position];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			if(async_flag)
			{
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, current_position,
			      	reclen, stride,(numrecs64*reclen),depth);
			    else
			       async_read(gc, (long long)fd, nbuff, current_position, reclen,
			    	 stride,(numrecs64*reclen),depth);
		   	}
			else
			{
		   	  if((uu=read((int)fd, (void*)nbuff, (size_t) reclen)) != reclen)
		   	  {
#ifdef NO_PRINT_LLD
		    		printf("\nError reading block %ld, fd= %d Filename %s Read returned %ld\n", i, fd,filename,uu);
		    		printf("\nSeeked to %ld Reclen = %ld\n", savepos64,reclen);
#else
		    		printf("\nError reading block %lld, fd= %d Filename %s Read returned %lld\n", i, fd,filename,uu);
		    		printf("\nSeeked to %lld Reclen = %lld\n", savepos64,reclen);
#endif
				perror("read");
		    		exit(88);
		   	  }
			}
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
		current_position+=reclen;
		if(verify){
			if(async_flag && no_copy_flag)
			{
			   if(verify_buffer(buffer1,reclen, (off64_t)savepos64 ,reclen,(long long)pattern,sverify)){
				exit(89);
			   }
			}
			else
			{
			   if(verify_buffer(nbuff,reclen, (off64_t)savepos64 ,reclen,(long long)pattern,sverify)){
				exit(90);
			   }
			}
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
			
		/* This is a bit tricky.  The goal is to read with a stride through
		   the file. The problem is that you need to touch all of the file
		   blocks. So.. the first pass through you read with a constant stride.
		   When you hit eof then add 1 to the beginning offset of the next
		   time through the file. The rub here is that eventually adding
		   1 will cause the initial start location plus the STRIDE to be
		   beyond eof. So... when this happens the initial offset for the
		   next pass needs to be set back to 0.
		*/
		if(current_position + (stride * reclen) >= (numrecs64 * reclen)-reclen) 
		{
			current_position=0;

			stripewrap++;
			  
			if(numrecs64 <= stride)
			{
				current_position=0;
			}
			else
			{
				current_position = (off64_t)((stripewrap)%numrecs64)*reclen;
			}

			if (!(h_flag || k_flag || mmapflag))
			{
			  if(I_LSEEK(fd,current_position,SEEK_SET)<0)
			  {
				perror("lseek");
				exit(91);
			  }
			}
		}
		else			
		{
			current_position+=(stride*reclen)-reclen;
			if (!(h_flag || k_flag || mmapflag))
			{
			  if(I_LSEEK(fd,current_position,SEEK_SET)<0)
			  {
				perror("lseek");
				exit(93);
			  };
			}
		}
	}
	if(cpuutilflag)
	{
		cputime  = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		walltime = time_so_far() - walltime;
		if (walltime < cputime)
		   walltime = cputime;
	}

#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);
		else
			fsync(fd);
	}
	if(include_close)
	{
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
		close(fd);
	}
	if(inc_think)
	   strideintime = ((time_so_far() - starttime1)-time_res);
	else
	   strideintime = ((time_so_far() - starttime1)-time_res)
		-compute_val;
	if(strideintime < (double).000001) 
	{
		strideintime= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(!include_close)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);
		else
			fsync(fd);
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
		close(fd);
	}

	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	if(MS_flag)
	{
		strideinrate=1000000.0*(strideintime / (double)filebytes64);
	}
      else
      {
            strideinrate = (unsigned long long) ((double) filebytes64 / strideintime);
      }
	if(!(OPS_flag || MS_flag))
		strideinrate >>= 10;
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime, cputime);
	store_value((off64_t)strideinrate);
#ifdef NO_PRINT_LLD
	if(!silent) printf(" %9ld",strideinrate);
#else
	if(!silent) printf(" %9lld",strideinrate);
#endif
	if(!silent) fflush(stdout);
	if(restf)
		sleep((int)rest_val);
}

#ifdef HAVE_PREAD
/************************************************************************/
/* pwrite_perf_test				        		*/
/* pwrite and re-write test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void pwrite_perf_test(off64_t kilos64,long long reclen,long long *data1,long long *data2)
#else
void pwrite_perf_test(kilos64,reclen,data1,data2)
off64_t kilos64;
long long reclen;
long long *data1,*data2;
#endif
{
	double burst_sleep_time_till_now[2] = {0, 0};
	double pwritetime[2];
	double starttime1;
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long i,j;
	long long Index = 0;
	unsigned long long pwriterate[2];
	off64_t filebytes64;
	long long flags_here = 0;
	int fd,ltest,wval;
	off64_t numrecs64,traj_offset;
	off64_t lock_offset=0;
	long long traj_size;
#if defined(VXFS) || defined(solaris)
	int test_foo=0;
#endif
	char *nbuff;

	traj_offset=0;
	nbuff=mainbuffer;
	if(w_traj_flag)
	{
		filebytes64 = w_traj_fsize;
		numrecs64=w_traj_ops;
	}
	else
	{
		numrecs64 = (kilos64*1024)/reclen;
		filebytes64 = numrecs64*reclen;
	}
	fd = 0;
	if(oflag){
		flags_here = O_SYNC|O_RDWR;
	}
	else
	{
		flags_here = O_RDWR;
	}
#if defined(O_DSYNC)
	if(odsync)
		flags_here |= O_DSYNC;
#endif

#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags_here |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags_here |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags_here |=O_DIRECTIO;
#endif
#endif
	if(noretest)
		ltest=1;
	else
		ltest=2;
	for( j=0; j<ltest; j++)
	{
		if(j==0)
			flags_here |=O_CREAT;
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
		if((fd = I_OPEN(filename, (int)flags_here,0640))<0)
		{
			printf("\nCan not open temp file: %s\n", 
				filename);
			perror("open");
			exit(97);
		}
#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
		if(direct_flag)
		{
			test_foo = directio(fd, DIRECTIO_ON);
			if(test_foo != 0)
			{
				if(!client_iozone)
					printf("\ndirectio not available.\n");
				exit(3);
			}
		}
#endif
		fsync(fd);
		nbuff=mainbuffer;
		mbuffer=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
		if(verify || dedup || dedup_interior)
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
		starttime1 = time_so_far();
	        compute_val=(double)0;
		if(w_traj_flag)
		{
			rewind(w_traj_fd);
		}
		for(i=0; i<numrecs64; i++){
			if(w_traj_flag)
			{
				traj_offset=get_traj(w_traj_fd, (long long *)&traj_size,(float *)&compute_time,(long)1);
				reclen=traj_size;
			}
			else
				traj_offset=(i * reclen);
			if(rlocking)
			{
				lock_offset=traj_offset;
				mylockr((int) fd, (int) 1, (int)0,
				  lock_offset, reclen);
			}
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                nbuff = mbuffer + Index;
                        }
			if((verify && diag_v) || dedup || dedup_interior)
				fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
			if(purge)
				purgeit(nbuff,reclen);
			if(I_PWRITE(fd, nbuff, reclen, traj_offset) != reclen)
			{
#ifdef NO_PRINT_LLD
			    	printf("\nError pwriting block %ld, fd= %d\n", i,
					 fd);
#else
			    	printf("\nError pwriting block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("pwrite");
				exit_code = 74;
				signal_handler();
			} else {
				update_burst_sleep(fd, reclen, &burst_sleep_time_till_now[j]);
			}
			if(rlocking)
			{
				mylockr((int) fd, (int) 0, (int)0,
				  lock_offset, reclen);
			}
		}
		if(include_flush)
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
		if(include_close)
		{
			wval=close(fd);
			if(wval==-1){
				perror("close");
				signal_handler();
			}
		}
		if(inc_think)
		   pwritetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j]);
		else
		   pwritetime[j] = ((time_so_far() - starttime1)-time_res - burst_sleep_time_till_now[j])
			-compute_val;
		if(pwritetime[j] < (double).000001) 
		{
			pwritetime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
			wval=close(fd);
			if(wval==-1){
				perror("close");
				signal_handler();
			}
		}

		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
	}
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	for(j=0;j<ltest;j++){
		if(MS_flag)
		{
			pwriterate[j]=1000000.0*(pwritetime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  pwriterate[j] = 
		      (unsigned long long) ((double) filebytes64 / pwritetime[j]);
            }
		if(!(OPS_flag || MS_flag))
			pwriterate[j] >>= 10;
	}
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(noretest)
	{
		pwriterate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)pwriterate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)pwriterate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%8ld",pwriterate[0]);
	if(!silent) printf("%9ld",pwriterate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",pwriterate[0]);
	if(!silent) printf("%9lld",pwriterate[1]);
	if(!silent) fflush(stdout);
#endif
}

/************************************************************************/
/* pread_perf_test				        		*/
/* pread and re-pread test						*/
/************************************************************************/
#ifdef HAVE_PREAD
#ifdef HAVE_ANSIC_C
void pread_perf_test(off64_t kilos64,long long reclen,long long *data1,long long *data2)
#else
void pread_perf_test(kilos64,reclen,data1,data2)
off64_t kilos64;
long long reclen;
long long *data1, *data2;
#endif
{
	double starttime2;
	double preadtime[2];
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long numrecs64,i;
	long long j;
	long long Index = 0;
	unsigned long long preadrate[2];
	off64_t filebytes64;
	off64_t lock_offset=0;
	int fd,open_flags;
	int ltest;
	off64_t traj_offset;
	long long traj_size;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	char *nbuff;

	traj_offset=0;
	nbuff=mainbuffer;
	open_flags=O_RDONLY;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		open_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		open_flags |=O_DIRECTIO;
#endif
#endif
#if defined(_HPUX_SOURCE) || defined(linux) || defined(__FreeBSD__) || defined(__DragonFly__)
	if(read_sync)
		open_flags |=O_RSYNC|O_SYNC;
#endif
	if(r_traj_flag)
	{
		filebytes64 = r_traj_fsize;
		numrecs64=r_traj_ops;
	}
	else
	{
		numrecs64 = (kilos64*1024)/reclen;
		filebytes64 = numrecs64*reclen;
	}

	fd = 0;
	if(noretest)
		ltest=1;
	else
		ltest=2;
	for( j=0; j<ltest; j++ ) 		/* Pread and Re-Pread */
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
		if((fd = I_OPEN(filename, (int)open_flags,0))<0)
		{
			printf("\nCan not open temporary file %s for read\n",filename);
			perror("open");
			exit(101);
		}
#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
		nbuff=mainbuffer;
		mbuffer=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
		starttime2 = time_so_far();
	        compute_val=(double)0;
		if(r_traj_flag)
		{
			rewind(r_traj_fd);
		}
		for(i=0; i<numrecs64; i++) 
		{
			if(r_traj_flag)
			{
				traj_offset=get_traj(r_traj_fd, (long long *)&traj_size,(float *)&compute_time,(long)1);
				reclen=traj_size;
			}
			else
				traj_offset=(i * reclen);
			if(rlocking)
			{
				lock_offset=traj_offset;
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
			if(compute_flag)
				compute_val+=do_compute(compute_time);
                        if(multi_buffer)
                        {
                                Index +=reclen;
                                if(Index > (MAXBUFFERSIZE-reclen))
                                        Index=0;
                                nbuff = mbuffer + Index;
                        }

			if(purge)
				purgeit(nbuff,reclen);
			if(I_PREAD(((int)fd), ((void*)nbuff), ((size_t) reclen),traj_offset ) 
					!= reclen)
			{
#ifdef NO_PRINT_LLD
				printf("\nError reading block %ld %lx\n", i,(unsigned long)nbuff);
#else
				printf("\nError reading block %lld %lx\n", i,(unsigned long)nbuff);
#endif
				perror("pread");
				exit(103);
			}
			if(verify){
				if(verify_buffer(nbuff,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
					exit(104);
				}
			}
			if(rlocking)
			{
				lock_offset=I_LSEEK(fd,0,SEEK_CUR);
				mylockr((int) fd, (int) 1, (int)1,
				  lock_offset, reclen);
			}
		}
		if(include_flush)
			fsync(fd);
		if(include_close)
			close(fd);
		if(inc_think)
		   preadtime[j] = ((time_so_far() - starttime2)-time_res);
		else
		   preadtime[j] = ((time_so_far() - starttime2)-time_res)
			-compute_val;
		if(preadtime[j] < (double).000001) 
		{
			preadtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			fsync(fd);
			close(fd);
		}

		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
    	}

	filebytes64 = numrecs64*reclen;
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	for(j=0;j<ltest;j++){
		if(MS_flag)
		{
			preadrate[j]=1000000.0*(preadtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  preadrate[j] = 
			(unsigned long long) ((double) filebytes64 / preadtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			preadrate[j] >>= 10;
	}
	if(noretest)
	{
		preadrate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}

	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)preadrate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)preadrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%8ld",preadrate[0]);
	if(!silent) printf("%9ld",preadrate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%8lld",preadrate[0]);
	if(!silent) printf("%9lld",preadrate[1]);
	if(!silent) fflush(stdout);
#endif
}
#endif

#ifdef HAVE_PREADV
/************************************************************************/
/* pwritev_perf_test				        		*/
/* pwritev and re-pwritev test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void pwritev_perf_test(off64_t kilos64,long long reclen,long long *data1,long long *data2)
#else
void pwritev_perf_test(kilos64,reclen,data1,data2)
off64_t kilos64;
long long reclen;
long long *data1,*data2;
#endif
{
	int	wval;
	double starttime1;
	double pwritevtime[2];
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long list_off[PVECMAX];
	long long numvecs,j,xx;
	unsigned long long pwritevrate[2];
	off64_t filebytes64,i;
	off64_t numrecs64;
	int fd,ltest;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	long long flags_here;
	char *nbuff;
#ifdef MERSENNE
        unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
#endif

	numrecs64 = (kilos64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	nbuff = mainbuffer;
	fd = 0;
	if(oflag)
		flags_here = O_SYNC|O_RDWR;
	else
		flags_here = O_RDWR;
#if defined(O_DSYNC)
	if(odsync)
		flags_here |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags_here |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags_here |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags_here |=O_DIRECTIO;
#endif
#endif
	 
	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++)
	{
		if(j==0)
			flags_here |=O_CREAT;
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}
		if((fd = I_OPEN(filename, (int)flags_here,0640))<0)
		{
			printf("\nCan not open temp file: %s\n", 
				filename);
			perror("open");
			exit(109);
		}
#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
					printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
		if(direct_flag)
		{
			test_foo = directio(fd, DIRECTIO_ON);
			if(test_foo != 0)
			{
				if(!client_iozone)
					printf("\ndirectio not available.\n");
				exit(3);
			}
		}
#endif
		nbuff=mainbuffer;
		mbuffer=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
		numvecs=PVECMAX;
                if(numrecs64 < numvecs) numvecs=numrecs64;
                if(MAXBUFFERSIZE/reclen < PVECMAX) numvecs=MAXBUFFERSIZE/reclen;

#ifdef MERSENNE
    	        init_by_array64(init, length);
#else
#ifdef bsd4_2
	        srand(0);
#else
#ifdef Windows
                srand(0);
#else
	        srand48(0);
#endif
#endif
#endif
		starttime1 = time_so_far();
	        compute_val=(double)0;
		for(i=0; i<numrecs64; i+=numvecs){
			if(compute_flag)
				compute_val+=do_compute(compute_time);
			if((numrecs64-i) < numvecs) 
				numvecs=numrecs64-i;
			create_list((long long *)list_off, reclen, numrecs64);
			for(xx=0;xx<numvecs;xx++)
			{
				piov[xx].piov_base = 
					(caddr_t)(nbuff+(xx * reclen));
				if(verify || dedup || dedup_interior)
					fill_buffer(piov[xx].piov_base,reclen,(long long)pattern,sverify,i);
				piov[xx].piov_len = reclen;
#ifdef PER_VECTOR_OFFSET
				piov[xx].piov_offset = list_off[xx];
#endif
				if(purge)
					purgeit(piov[xx].piov_base,reclen);
			}
			if(pwritev(fd, piov,numvecs
#ifndef PER_VECTOR_OFFSET
				, list_off[0]
#endif
				) != (reclen*numvecs))
			{
#ifdef NO_PRINT_LLD
			    	printf("\nError pwriteving block %ld, fd= %d\n", i,
					 fd);
#else
			    	printf("\nError pwriteving block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("pwritev");
			    	exit(113);
			}
		}

		if(include_flush)
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
		}
		if(include_close)
		{
			wval=close(fd);
			if(wval==-1){
				perror("close");
				signal_handler();
			}
		}
		if(inc_think)
		   pwritevtime[j] = ((time_so_far() - starttime1)-time_res);
		else
		   pwritevtime[j] = ((time_so_far() - starttime1)-time_res)
			-compute_val;
		if(pwritevtime[j] < (double).000001) 
		{
			pwritevtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			wval=fsync(fd);
			if(wval==-1){
				perror("fsync");
				signal_handler();
			}
			wval=close(fd);
			if(wval==-1){
				perror("close");
				signal_handler();
			}
		}

		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
	}
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	for(j=0;j<ltest;j++){
		if(MS_flag)
		{
			pwritevrate[j]=1000000.0*(pwritevtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  pwritevrate[j] = 
		      (unsigned long long) ((double) filebytes64 / pwritevtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			pwritevrate[j] >>= 10;
	}
	if(noretest)
	{
		pwritevrate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}	
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)pwritevrate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)pwritevrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%9ld",pwritevrate[0]);
	if(!silent) printf("%10ld",pwritevrate[1]);
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%9lld",pwritevrate[0]);
	if(!silent) printf("%10lld",pwritevrate[1]);
	if(!silent) fflush(stdout);
#endif
}
#endif


#ifdef HAVE_PREADV
/**************************************************************************/
/* create_list() 							  */
/* Creates a list of PVECMAX entries that are unique (non over lapping ). */
/* Each of these offsets are then used in a vector (preadv/pwritev)	  */
/**************************************************************************/
#ifdef HAVE_ANSIC_C
void create_list(long long *list_off, long long reclen, off64_t numrecs64)
#else
void create_list(list_off, reclen, numrecs64)
long long *list_off;
long long reclen; 
off64_t numrecs64;
#endif
{
	long long found,i,j;
	long long numvecs;
	unsigned long long big_rand = -1;
#if defined (bsd4_2) || defined(Windows)
	long long rand1,rand2,rand3;
#endif

	numvecs = PVECMAX;
	if(numrecs64< numvecs)
		numvecs = numrecs64;
	for(j=0;j<numvecs;j++)
		list_off[j]=0;	
	for(j=0;j<numvecs;j++)
	{
again:
		found = 0;
#ifdef MERSENNE
                big_rand = genrand64_int64();
		offset64 = reclen * (big_rand%numrecs64);
#else
#ifdef bsd4_2
		rand1=(long long)rand();
		rand2=(long long)rand();
		rand3=(long long)rand();
		big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		offset64 = reclen * (big_rand%numrecs64);
#else
#ifdef Windows
		rand1=(long long)rand();
		rand2=(long long)rand();
		rand3=(long long)rand();
		big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		offset64 = reclen * (big_rand%numrecs64);
#else
		offset64 = reclen * (lrand48()%numrecs64);
#endif
#endif
#endif

		for(i=0;i<j;i++)
		{
			if(list_off[i] == offset64)
			{
				found++;
				break;
			}
		}
		if(!found)
			list_off[j]=offset64;
		else
		{
			goto again;
		}
	}
}
#endif

#ifdef HAVE_PREADV
/************************************************************************/
/* preadv_perf_test				        		*/
/* preadv and re-preadv test						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void preadv_perf_test(off64_t kilos64,long long reclen,long long *data1,long long *data2)
#else
void preadv_perf_test(kilos64,reclen,data1,data2)
off64_t kilos64;
long long reclen;
long long *data1,*data2;
#endif
{
	double starttime2;
	double preadvtime[2];
	double walltime[2], cputime[2];
	double compute_val = (double)0;
	long long list_off[PVECMAX];
	long long numvecs,i,j,xx;
	off64_t numrecs64;
	unsigned long long preadvrate[2];
	off64_t filebytes64;
	int fd,open_flags,ltest;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	char *nbuff;
#ifdef MERSENNE
        unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
#endif

	open_flags=O_RDONLY;
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		open_flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		open_flags |=O_DIRECTIO;
#endif
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		open_flags |=O_RSYNC|O_SYNC;
#endif
	numrecs64 = (kilos64*1024)/reclen;
	filebytes64 = numrecs64*reclen;
	nbuff = mainbuffer;
	fd = 0;
	if(noretest)
		ltest=1;
	else
		ltest=2;

	for( j=0; j<ltest; j++ )
	{
		if(cpuutilflag)
		{
			walltime[j] = time_so_far();
			cputime[j]  = cputime_so_far();
		}
		if(Uflag) /* Unmount and re-mount the mountpoint */
		{
			purge_buffer_cache();
		}

		if((fd = I_OPEN(filename, (int)open_flags,0))<0)
		{
			printf("\nCan not open temporary file for preadv\n");
			perror("open");
			exit(114);
		}
#ifdef VXFS
		if(direct_flag)
		{
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
			ioctl(fd,VX_GETCACHE,&test_foo);
			if(test_foo == 0)
			{
				if(!client_iozone)
				  printf("\nVxFS advanced setcache feature not available.\n");
				exit(3);
			}
		}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
		nbuff=mainbuffer;
		mbuffer=mainbuffer;
		if(fetchon)
			fetchit(nbuff,reclen);
		numvecs=PVECMAX;
                if(numrecs64 < numvecs) numvecs=numrecs64;
                if(MAXBUFFERSIZE/reclen < PVECMAX) numvecs=MAXBUFFERSIZE/reclen;

#ifdef MERSENNE
    	       init_by_array64(init, length);
#else
#ifdef bsd4_2
	        srand(0);
#else
#ifdef Windows
                srand(0);
#else
	        srand48(0);
#endif
#endif
#endif
		starttime2 = time_so_far();
	        compute_val=(double)0;
		for(i=0; i<(numrecs64); i+=numvecs) 
		{
			if(compute_flag)
				compute_val+=do_compute(compute_time);
			if((numrecs64-i) < numvecs) 
				numvecs=numrecs64-i;
			create_list((long long *)list_off, reclen, numrecs64);
			for(xx=0;xx<numvecs;xx++)
			{
				piov[xx].piov_base = 
					(caddr_t)(nbuff+(xx * reclen));
				piov[xx].piov_len = reclen;
#ifdef PER_VECTOR_OFFSET
				piov[xx].piov_offset = list_off[xx];
#endif
				if(purge)
				   purgeit(piov[xx].piov_base,reclen);
			}
			if(preadv(fd, piov, numvecs
#ifndef PER_VECTOR_OFFSET
				, list_off[0]
#endif
				) != (numvecs * reclen))
			{
#ifdef NO_PRINT_LLD
				printf("\nError preadving block %ld \n", i);
#else
				printf("\nError preadving block %lld \n", i);
#endif
				perror("preadv");
				exit(116);
			}
		}
		if(include_flush)
			fsync(fd);
		if(include_close)
			close(fd);
		if(inc_think)
		   preadvtime[j] = ((time_so_far() - starttime2)-time_res);
		else
		   preadvtime[j] = ((time_so_far() - starttime2)-time_res)
			-compute_val;
		if(preadvtime[j] < (double).000001) 
		{
			preadvtime[j]= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}
		if(!include_close)
		{
			fsync(fd);
			close(fd);
		}

		if(cpuutilflag)
		{
			cputime[j]  = cputime_so_far() - cputime[j];
			if (cputime[j] < cputime_res)
				cputime[j] = 0.0;
			walltime[j] = time_so_far() - walltime[j];
			if (walltime[j] < cputime[j])
			   walltime[j] = cputime[j];
		}
		if(restf)
			sleep((int)rest_val);
    	}
	if(OPS_flag || MS_flag){
	   filebytes64=filebytes64/reclen;
	}
	for(j=0;j<ltest;j++){
		if(MS_flag)
		{
			preadvrate[j]=1000000.0*(preadvtime[j] / (double)filebytes64);
			continue;
		}
            else
            {
                  preadvrate[j] = 
		      (unsigned long long) ((double) filebytes64 / preadvtime[j]);
            }
		if(!(OPS_flag || MS_flag))
			preadvrate[j] >>= 10;
	}
	if(noretest)
	{
		preadvrate[1]=(long long)0;
		if(cpuutilflag)
		{
			walltime[1]=0.0;
			cputime[1]=0.0;
		}
	}
	
	/* Must save walltime & cputime before calling store_value() for each/any cell.*/
	if(cpuutilflag)
		store_times(walltime[0], cputime[0]);
	store_value((off64_t)preadvrate[0]);
	if(cpuutilflag)
		store_times(walltime[1], cputime[1]);
	store_value((off64_t)preadvrate[1]);
#ifdef NO_PRINT_LLD
	if(!silent) printf("%10ld",preadvrate[0]);
	if(!silent) printf("%9ld",preadvrate[1]);
	if(!silent) printf("\n");
	if(!silent) fflush(stdout);
#else
	if(!silent) printf("%10lld",preadvrate[0]);
	if(!silent) printf("%9lld",preadvrate[1]);
	if(!silent) printf("\n");
	if(!silent) fflush(stdout);
#endif
}
#endif

/************************************************************************/
/* print_header()							*/
/* Prints the header for the output from Iozone.			*/
/************************************************************************/
#endif
#ifdef HAVE_ANSIC_C
void print_header(void)
#else
void print_header()
#endif
{
	if(Eflag)
	{
    	   if(!silent) printf(CONTROL_STRING2,
		" ", 
		" ",
		" ",
		" ",
		" ",
		" ",
		"random",			/*kcollins:2-5-96*/
		"random",			/*kcollins:2-5-96*/
		"bkwd",
		"record",
		"stride",
		" ",
		" ",
		" ",
		" "
#ifdef HAVE_PREAD
		," ",
		" ",
		" ",
		" "
#ifdef HAVE_PREADV
		," ",
		" ",
		" ",
		" "
#endif
#endif
		);
    	if(!silent) printf(CONTROL_STRING2,
		"kB", 
		"reclen",
		"write",
		"rewrite",
		"read",
		"reread",
		"read",				/*kcollins:2-5-96*/
		"write",			/*kcollins:2-5-96*/
		"read",
		"rewrite",
		"read",
		"fwrite",
		"frewrite",
		"fread",
		"freread"
#ifdef HAVE_PREAD
		,"pwrite",
		"repwrite",
		"pread",
		"repread"
#ifdef HAVE_PREADV
		,"pwritev",
		"repwritev",
		"preadv",
		"repreadv"
#endif
#endif
		);
	}else 
	if(RWONLYflag){				/*kcollins 8-21-96*/
    	   if(!silent) printf(CONTROL_STRING4,		/*kcollins 8-21-96*/
		" ", 				/*kcollins 8-21-96*/
		" ",				/*kcollins 8-21-96*/
		" ",				/*kcollins 8-21-96*/
		" ",				/*kcollins 8-21-96*/
		" ",				/*kcollins 8-21-96*/
		" "				/*kcollins 8-21-96*/
		);				/*kcollins 8-21-96*/
    	if(!silent) printf(CONTROL_STRING4,			/*kcollins 8-21-96*/
		"kB", 				/*kcollins 8-21-96*/
		"reclen",			/*kcollins 8-21-96*/
		"write",			/*kcollins 8-21-96*/
		"rewrite",			/*kcollins 8-21-96*/
		"read",				/*kcollins 8-21-96*/
		"reread"			/*kcollins 8-21-96*/
		);				/*kcollins 8-21-96*/
	}else{
	   if(!(mmapflag || async_flag))
	   {
    	   	if(!silent) printf(CONTROL_STRING3,
			" ", 
			" ",
			" ",
			" ",
			" ",
			" ",
			"random",			/*kcollins:2-5-96*/
			"random",			/*kcollins:2-5-96*/
			"bkwd",
			"record",
			"stride",
			"",
			"",
			"",
			""
			);
    		if(!silent) printf(CONTROL_STRING3,
			"kB", 
			"reclen",
			"write",
			"rewrite",
			"read",
			"reread",
			"read",				/*kcollins:2-5-96*/
			"write",			/*kcollins:2-5-96*/
			"read",
			"rewrite",
			"read",
			"fwrite",
			"frewrite",
			"fread",
			"freread" 
			);
		}else
		{
    	   		if(!silent) printf(CONTROL_STRING3,
				" ", 
				" ",
				" ",
				" ",
				" ",
				" ",
				"random",			/*kcollins:2-5-96*/
				"random",			/*kcollins:2-5-96*/
				"bkwd",
				"record",
				"stride",
				"",
				"",
				"",
				""
				);
    			if(!silent) printf(CONTROL_STRING3,
				"kB", 
				"reclen",
				"write",
				"rewrite",
				"read",
				"reread",
				"read",				/*kcollins:2-5-96*/
				"write",			/*kcollins:2-5-96*/
				"read",
				"rewrite",
				"read",
				"",
				"",
				"",
				"" 
				);
		}
	}
}

/************************************************************************/
/* store_value()							*/
/* Stores a value in an in memory array. Used by the report function    */
/* to re-organize the output for Excel					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
store_value(off64_t value)
#else
store_value(value)
off64_t value;
#endif
{
	report_array[current_x][current_y]=value;
	current_x++;
	if(current_x > max_x)
		max_x=current_x;
	if(current_y > max_y)
		max_y=current_y;
	if(max_x >= MAX_X)
	{
		printf("\nMAX_X too small\n");
		exit(117);
	}
	if(max_y >= MAX_Y)
	{
		printf("\nMAX_Y too small\n");
		exit(118);
	}
}

/************************************************************************/
/* store_times()							*/
/* Stores runtime (walltime & cputime) in a memory array.		*/
/* Used by the report function to re-organize the output for Excel	*/
/* For now, must be called immediately before calling store_value() for	*/
/* each cell.								*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
store_times(double walltime, double cputime)
#else
store_times(walltime, cputime)
double walltime, cputime;
#endif
{
	runtimes [current_x][current_y].walltime = walltime;
	runtimes [current_x][current_y].cputime  = cputime;
	runtimes [current_x][current_y].cpuutil  = cpu_util(cputime, walltime);
}

/************************************************************************/
/* dump_report()							*/
/* Dumps the Excel report on standard output.				*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void dump_report(long long who)
#else
dump_report(who)
long long who;
#endif
{
	long long i;
	off64_t current_file_size;
	off64_t rec_size;

	if(bif_flag)
		bif_column++;
	if(!silent) printf("      ");

	/* 
	 * Need to reconstruct the record size list
	 * as the crossover in -a changed the list.
	*/
	del_record_sizes();
	init_record_sizes(orig_min_rec_size, orig_max_rec_size);

	for(rec_size=get_next_record_size(0); rec_size <= orig_max_rec_size;
		rec_size=get_next_record_size(rec_size))
	{
		if (rec_size == 0) break;
		if(bif_flag)
			do_float(bif_fd,(double)(rec_size/1024),bif_row,bif_column++);
#ifdef NO_PRINT_LLD
		if(!silent) printf("  %c%ld%c",'"',rec_size/1024,'"');
#else
		if(!silent) printf("  %c%lld%c",'"',rec_size/1024,'"');
#endif
	}
	if(!silent) printf("\n");
	if(bif_flag)
	{
		bif_column=0;
		bif_row++;
	}

	current_file_size = report_array[0][0];
	if(bif_flag)
	{
		do_float(bif_fd,(double)(current_file_size),bif_row,bif_column++);
	}
#ifdef NO_PRINT_LLD
	if(!silent) printf("%c%ld%c  ",'"',current_file_size,'"');
#else
	if(!silent) printf("%c%lld%c  ",'"',current_file_size,'"');
#endif
	for(i=0;i<=max_y;i++){
		if(report_array[0][i] != current_file_size){
			if(!silent) printf("\n");
			current_file_size = report_array[0][i];
			if(bif_flag)
			{
				bif_row++;
				bif_column=0;
				do_float(bif_fd,(double)(current_file_size),bif_row,bif_column++);
			}
#ifdef NO_PRINT_LLD
			if(!silent) printf("%c%ld%c  ",'"',current_file_size,'"');
#else
			if(!silent) printf("%c%lld%c  ",'"',current_file_size,'"');
#endif
		}
		if(bif_flag)
			do_float(bif_fd,(double)(report_array[who][i]),bif_row,bif_column++);
#ifdef NO_PRINT_LLD
		if(!silent) printf(" %ld ",report_array[who][i]);
#else
		if(!silent) printf(" %lld ",report_array[who][i]);
#endif
	}
	if(bif_flag)
	{
		bif_row++;
		bif_column=0;
	}
	if(!silent) printf("\n");
}

/************************************************************************/
/* Wrapper that dumps each of the collected data sets.			*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void dump_excel(void)
#else
void dump_excel()
#endif
{
	if(bif_flag)
	{
		bif_fd=create_xls(bif_filename);
		do_label(bif_fd,command_line,bif_row++,bif_column);
		do_label(bif_fd," ",bif_row++,bif_column);
                do_label(bif_fd,"The top row is records sizes, the left column is file sizes",bif_row++,bif_column);
	}
	if(!silent) printf("Excel output is below:\n");

    if ((!include_tflag) || (include_mask & (long long)WRITER_MASK)) {
	if(bif_flag)
		do_label(bif_fd,"Writer Report",bif_row++,bif_column);
	if(!silent) printf("\n%cWriter report%c\n",'"','"');
	dump_report(2); 
	if(bif_flag)
		do_label(bif_fd,"Re-writer Report",bif_row++,bif_column);
	if(!silent) printf("\n%cRe-writer report%c\n",'"','"');
	dump_report(3); 
    }

    if ((!include_tflag) || (include_mask & (long long)READER_MASK)) {
	if(bif_flag)
		do_label(bif_fd,"Reader Report",bif_row++,bif_column);
	if(!silent) printf("\n%cReader report%c\n",'"','"');
	dump_report(4); 
	if(bif_flag)
		do_label(bif_fd,"Re-reader Report",bif_row++,bif_column);
	if(!silent) printf("\n%cRe-Reader report%c\n",'"','"');
	dump_report(5); 
    }

	if ((!include_tflag) || (include_mask & (long long)RANDOM_RW_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Random Read Report",bif_row++,bif_column);
		if(!silent) printf("\n%cRandom read report%c\n",'"','"');
		dump_report(6); 
		if(bif_flag)
			do_label(bif_fd,"Random Write Report",bif_row++,bif_column);
		if(!silent) printf("\n%cRandom write report%c\n",'"','"');
		dump_report(7); 
	}

	if ((!include_tflag) || (include_mask & (long long)REVERSE_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Backward Read Report",bif_row++,bif_column);
		if(!silent) printf("\n%cBackward read report%c\n",'"','"');
		dump_report(8); 
	}

	if ((!include_tflag) || (include_mask & (long long)REWRITE_REC_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Record Rewrite Report",bif_row++,bif_column);
		if(!silent) printf("\n%cRecord rewrite report%c\n",'"','"');
		dump_report(9); 
	}

	if ((!include_tflag) || (include_mask & (long long)STRIDE_READ_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Stride Read Report",bif_row++,bif_column);
		if(!silent) printf("\n%cStride read report%c\n",'"','"');
		dump_report(10); 
	}

	if ((!include_tflag) || (include_mask & (long long)FWRITER_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Fwrite Report",bif_row++,bif_column);
		if(!silent) printf("\n%cFwrite report%c\n",'"','"');
		dump_report(11); 
		if(bif_flag)
			do_label(bif_fd,"Re-fwrite Report",bif_row++,bif_column);
		if(!silent) printf("\n%cRe-Fwrite report%c\n",'"','"');
		dump_report(12); 
	}

	if ((!include_tflag) || (include_mask & (long long)FREADER_MASK)) {
		if(bif_flag)
			do_label(bif_fd,"Fread Report",bif_row++,bif_column);
		if(!silent) printf("\n%cFread report%c\n",'"','"');
		dump_report(13); 
		if(bif_flag)
			do_label(bif_fd,"Re-fread Report",bif_row++,bif_column);
		if(!silent) printf("\n%cRe-Fread report%c\n",'"','"');
		dump_report(14); 
	}

#ifdef HAVE_PREAD
	if(Eflag)
	{
		if ((!include_tflag) || (include_mask & (long long)PWRITER_MASK)) {
			if(bif_flag)
				do_label(bif_fd,"Pwrite Report",bif_row++,bif_column);
			if(!silent) printf("\n%cPwrite report%c\n",'"','"');
			dump_report(15); 
			if(bif_flag)
				do_label(bif_fd,"Re-pwrite Report",bif_row++,bif_column);
		 	if(!silent) printf("\n%cRe-Pwrite report%c\n",'"','"');
		 	dump_report(16); 
		}

		if ((!include_tflag) || (include_mask & (long long)PREADER_MASK)) {
			if(bif_flag)
				do_label(bif_fd,"Pread Report",bif_row++,bif_column);
		 	if(!silent) printf("\n%cPread report%c\n",'"','"');
		 	dump_report(17); 
			if(bif_flag)
				do_label(bif_fd,"Re-pread Report",bif_row++,bif_column);
		 	if(!silent) printf("\n%cRe-Pread report%c\n",'"','"');
		 	dump_report(18); 
		}

#ifdef HAVE_PREADV
		if ((!include_tflag) || (include_mask & (long long)PWRITEV_MASK)) {
			if(bif_flag)
				do_label(bif_fd,"Pwritev Report",bif_row++,bif_column);
 			if(!silent) printf("\n%cPwritev report%c\n",'"','"');
 			dump_report(19); 
			if(bif_flag)
				do_label(bif_fd,"Re-pwritev Report",bif_row++,bif_column);
 			if(!silent) printf("\n%cRe-Pwritev report%c\n",'"','"');
 			dump_report(20); 
		}

		if ((!include_tflag) || (include_mask & (long long)PREADV_MASK)) {
			if(bif_flag)
				do_label(bif_fd,"Preadv Report",bif_row++,bif_column);
 			if(!silent) printf("\n%cPreadv report%c\n",'"','"');
 			dump_report(21); 
			if(bif_flag)
				do_label(bif_fd,"Re-preadv Report",bif_row++,bif_column);
 			if(!silent) printf("\n%cRe-Preadv report%c\n",'"','"');
 			dump_report(22); 
		}
#endif
	}
#endif
	if (cpuutilflag)
		dump_cputimes();
	if(bif_flag)
		close_xls(bif_fd);
}

/************************************************************************/
/* dump_times()		 						*/
/* Dumps the Excel CPU times report to stdout and to the bif file.	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void dump_times(long long who)
#else
dump_times(who)
long long who;
#endif
{
	long long i;
	off64_t current_file_size;
	off64_t rec_size;

	if (bif_flag)
		bif_column++;
	if(!silent) printf("      ");

	for (rec_size = get_next_record_size(0); rec_size <= orig_max_rec_size;
		rec_size = get_next_record_size(rec_size))
	{
		if (rec_size == 0) break;
		if (bif_flag)
			do_float(bif_fd, (double)(rec_size/1024), bif_row, bif_column++);
#ifdef NO_PRINT_LLD
		if(!silent) printf("  %c%ld%c",'"',rec_size/1024,'"');
#else
		if(!silent) printf("  %c%lld%c",'"',rec_size/1024,'"');
#endif
	}
	if(!silent) printf("\n");
	if (bif_flag)
	{
		bif_column=0;
		bif_row++;
	}

	current_file_size = report_array[0][0];
	if (bif_flag)
	{
		do_float(bif_fd, (double)(current_file_size), bif_row, bif_column++);
	}
#ifdef NO_PRINT_LLD
	if(!silent) printf("%c%ld%c  ",'"',current_file_size,'"');
#else
	if(!silent) printf("%c%lld%c  ",'"',current_file_size,'"');
#endif
	for (i = 0; i <= max_y; i++) {
		if (report_array[0][i] != current_file_size) {
			if(!silent) printf("\n");
			current_file_size = report_array[0][i];
			if (bif_flag)
			{
				bif_row++;
				bif_column=0;
				do_float(bif_fd, (double)(current_file_size), bif_row, bif_column++);
			}
#ifdef NO_PRINT_LLD
			if(!silent) printf("%c%ld%c  ",'"',current_file_size,'"');
#else
			if(!silent) printf("%c%lld%c  ",'"',current_file_size,'"');
#endif
		}
		if (bif_flag)
			do_float(bif_fd, (double)(runtimes [who][i].cpuutil), bif_row, bif_column++);
		if(!silent) printf(" %6.2f", runtimes [who][i].cpuutil);
	}
	if(!silent) printf("\n");
	if (bif_flag)
	{
		bif_row++;
		bif_column=0;
	}
}

/************************************************************************/
/* Wrapper that dumps each of the collected data sets.			*/
/* This one dumps only the collected CPU times.				*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void dump_cputimes(void)
#else
void dump_cputimes(void)
#endif
{
	bif_row++;
	bif_column = 0;

    if ((!include_tflag) || (include_mask & (long long)WRITER_MASK)) {
	if(bif_flag)
		do_label(bif_fd, "Writer CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
	if(!silent) printf("\n%cWriter CPU utilization report (Zero values should be ignored)%c\n",'"','"');
	dump_times(2); 
	if(bif_flag)
		do_label(bif_fd, "Re-writer CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
	if(!silent) printf("\n%cRe-writer CPU utilization report (Zero values should be ignored)%c\n",'"','"');
	dump_times(3); 
    }

    if ((!include_tflag) || (include_mask & (long long)READER_MASK)) {
	if(bif_flag)
		do_label(bif_fd, "Reader CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
	if(!silent) printf("\n%cReader CPU utilization report (Zero values should be ignored)%c\n",'"','"');
	dump_times(4); 
	if(bif_flag)
		do_label(bif_fd, "Re-reader CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
	if(!silent) printf("\n%cRe-Reader CPU utilization report (Zero values should be ignored)%c\n",'"','"');
	dump_times(5); 
    }

	if ((!include_tflag) || (include_mask & (long long)RANDOM_RW_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Random Read CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cRandom read CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(6); 
		if(bif_flag)
			do_label(bif_fd, "Random Write CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cRandom write CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(7); 
	}

	if ((!include_tflag) || (include_mask & (long long)REVERSE_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Backward Read CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cBackward read CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(8); 
	}

	if ((!include_tflag) || (include_mask & (long long)REWRITE_REC_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Record Rewrite CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cRecord rewrite CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(9); 
	}

	if ((!include_tflag) || (include_mask & (long long)STRIDE_READ_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Stride Read CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cStride read CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(10); 
	}

	if ((!include_tflag) || (include_mask & (long long)FWRITER_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Fwrite CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cFwrite CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(11); 
		if(bif_flag)
			do_label(bif_fd, "Re-fwrite CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cRe-Fwrite CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(12); 
	}

	if ((!include_tflag) || (include_mask & (long long)FREADER_MASK)) {
		if(bif_flag)
			do_label(bif_fd, "Fread CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cFread CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(13); 
		if(bif_flag)
			do_label(bif_fd, "Re-fread CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		if(!silent) printf("\n%cRe-Fread CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		dump_times(14); 
	}

#ifdef HAVE_PREAD
	if(Eflag)
	{
		if ((!include_tflag) || (include_mask & (long long)PWRITER_MASK)) {
			if(bif_flag)
				do_label(bif_fd, "Pwrite CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
			if(!silent) printf("\n%cPwrite CPU utilization report (Zero values should be ignored)%c\n",'"','"');
			dump_times(15); 
			if(bif_flag)
				do_label(bif_fd, "Re-pwrite CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		 	if(!silent) printf("\n%cRe-Pwrite CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		 	dump_times(16); 
		}

		if ((!include_tflag) || (include_mask & (long long)PREADER_MASK)) {
			if(bif_flag)
				do_label(bif_fd, "Pread CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		 	if(!silent) printf("\n%cPread CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		 	dump_times(17); 
			if(bif_flag)
				do_label(bif_fd, "Re-pread CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
		 	if(!silent) printf("\n%cRe-Pread CPU utilization report (Zero values should be ignored)%c\n",'"','"');
		 	dump_times(18); 
		}

#ifdef HAVE_PREADV
		if ((!include_tflag) || (include_mask & (long long)PWRITEV_MASK)) {
			if(bif_flag)
				do_label(bif_fd, "Pwritev CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
 			if(!silent) printf("\n%cPwritev CPU utilization report (Zero values should be ignored)%c\n",'"','"');
 			dump_times(19); 
			if(bif_flag)
				do_label(bif_fd, "Re-pwritev CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
 			if(!silent) printf("\n%cRe-Pwritev CPU utilization report (Zero values should be ignored)%c\n",'"','"');
 			dump_times(20); 
		}

		if ((!include_tflag) || (include_mask & (long long)PREADV_MASK)) {
			if(bif_flag)
				do_label(bif_fd, "Preadv CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
			if(!silent) printf("\n%cPreadv CPU utilization report (Zero values should be ignored)%c\n",'"','"');
 			dump_times(21); 
			if(bif_flag)
				do_label(bif_fd, "Re-preadv CPU utilization report (Zero values should be ignored)", bif_row++, bif_column);
			if(!silent) printf("\n%cRe-Preadv CPU utilization report (Zero values should be ignored)%c\n",'"','"');
			dump_times(22); 
		}
#endif
	}
#endif
}

/************************************************************************/
/* Internal memory allocation mechanism. Uses shared memory or mmap 	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
char *
alloc_mem(long long size, int shared_flag)
#else
char *
alloc_mem(size,shared_flag)
long long size;
int shared_flag;
#endif
{
	long long size1;
	char *addr,*dumb;
	int shmid;
	int tfd;
	long long tmp;
#if defined(solaris) 
        char mmapFileName[]="mmap_tmp_XXXXXX";
#endif

	tmp = 0;
	dumb = (char *)0;
	tfd=0;
	size1=l_max(size,page_size);
	if(!distributed)
	{
		if(!trflag)
		{
			addr=(char *)malloc((size_t)size1);
			return(addr);
		}
		if(use_thread)
		{
			addr=(char *)malloc((size_t)size1);
			return(addr);
		}
	}
	if(!shared_flag)
	{
		addr=(char *)malloc((size_t)size1);
		return(addr);
	}
#ifdef SHARED_MEM
	size1=l_max(size,page_size);
	size1=(size1 +page_size) & ~(page_size-1);
	shmid=(int)shmget((key_t)(IPC_PRIVATE), (size_t)size1 , (int)(IPC_CREAT|0666));
        if(shmid < (int)0)
        {
                printf("\nUnable to get shared memory segment(shmget)\n");
#ifdef NO_PRINT_LLD
                printf("shmid = %d, size = %ld, size1 = %lu, Error %d\n",shmid,size,(size_t)size1,errno);
#else
                printf("shmid = %d, size = %lld, size1 = %lu, Error %d\n",shmid,size,(unsigned long)size1,errno);
#endif
                exit(119);
        }
        /*addr = (char *)shmat(shmid, 0, SHM_W);*/
	/* Some systems will not take the above but
	 * will default to read/write if no flags
	 * are provided. (AIX)
	 * The POSIX standard states that if SHM_RDONLY
	 * is not specified then it will be read/write.
	 */
        addr = (char *)shmat((int)shmid, 0, 0);
#ifdef _64BIT_ARCH_
        if((long long)addr == (long long)-1)
#else
        if((long)addr == (long)-1)
#endif
        {
                printf("\nUnable to get shared memory segment\n");
                printf("..Error %d\n",errno);
                exit(120);
        }
	shmctl(shmid, IPC_RMID, 0);
	return(addr);
#else

	size1=l_max(size,page_size);
	size1=(size1 +page_size) & ~(page_size-1);
#if defined(bsd4_2) && !defined(IOZ_macosx)
	if((tfd = creat("mmap.tmp", 0666))<0)
	{
		printf("Unable to create tmp file\n");
		exit(121);
	}
	addr=(char *)mmap(0,&size1,PROT_WRITE|PROT_READ,
		MAP_ANON|MAP_SHARED, tfd, 0);
	unlink("mmap.tmp");
#else


#if defined(solaris) 
	tfd=mkstemp(mmapFileName);
        if(tfd < 0)
	{
		printf("Unable to create tmp file\n");
		exit(121);
	}
	dumb=(char *)malloc((size_t)size1);
	bzero(dumb,size1);
	write(tfd,dumb,size1);
	free(dumb);
	addr=(char *)mmap(0,(size_t)size1,PROT_WRITE|PROT_READ,
		MAP_SHARED, tfd, 0);
	unlink(mmapFileName);
#else
#if defined(SCO) || defined(SCO_Unixware_gcc) || defined(Windows)
        char mmapFileName[]="mmap_tmp_XXXXXX";
	tfd=mkstemp(mmapFileName);
        if(tfd < 0)
        {
                printf("Unable to create tmp file\n");
                exit(121);
        }
        dumb=(char *)malloc((size_t)size1);
	bzero(dumb,size1);
        write(tfd,dumb,size1);
        free(dumb);
        addr=(char *)mmap(0,(size_t)size1,PROT_WRITE|PROT_READ,
                MAP_SHARED, tfd, 0);
	unlink(mmapFileName);
#else
	addr=(char *)mmap(0,(size_t)size1,PROT_WRITE|PROT_READ,
		MAP_ANONYMOUS|MAP_SHARED, -1, 0);
#endif
#endif
#endif
	if((char *)addr == (char *)-1)
	{
		printf("\nUnable to get memory segment\n");
		printf("Error %d\n",errno);
		exit(122);
	}
	if(debug1)
		printf("Got shared memory for size %d\n",size1);

	return(addr);
#endif
}

/************************************************************************/
/* Implementation of poll() function.					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void Poll(long long time1)
#else
void Poll(time1)
long  long time1;
#endif
{
	struct timeval howlong;
	howlong.tv_sec=(int)(time1/100000);
	howlong.tv_usec=(int)(time1%100000); /* Get into u.s. */
	select(0, 0, 0, 0, &howlong);
}

/************************************************************************/
/* Implementation of max() function.					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
long long l_max(long long one,long long two)
#else
long long l_max(one,two)
long long one,two;
#endif
{
	if(one > two)
		return(one);
	else
		return(two);
}

/************************************************************************/
/* Internal Kill. With stonewalling disabled, kill does nothing 	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void Kill(long long pid,long long sig)
#else
void Kill(pid,sig)
long long pid,sig;
#endif
{
	if(!xflag)
	{
		/*printf("Killing %d\n",pid);*/
		kill((pid_t)pid,(int)sig);
	}
}
/************************************************************************/
/* Implementation of min() function.					*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
long long l_min(long long num1,long long num2)
#else
long long l_min(num1,num2)
long long num1,num2;
#endif
{
	if(num1 >= num2)
		return num2;
	else
		return num1;
}

/************************************************************************/
/* Routine to call throughput tests many times.				*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void
multi_throughput_test(long long mint,long long maxt)
#else
void multi_throughput_test(mint, maxt)
long long mint, maxt;
#endif
{
        int *t_rangeptr, *t_rangecurs;
        int *saveptr = (int *)0;
        int tofree = 0;
	long long i;
        if(t_count == 0){
            t_count = (int) maxt - mint + 1;
            t_rangeptr = (int *) malloc((size_t)sizeof(int)*t_count);
	    saveptr = t_rangeptr;
            tofree = 1;
            t_rangecurs = t_rangeptr;
            for(i=mint; i<= maxt; i++) {
                *(t_rangecurs++) = i;
            }
        }
        else {
            t_rangeptr = &t_range[0];
        }
	for(i=0; i < t_count; i++){
		num_child = *(t_rangeptr++);
		current_client_number=0; /* Need to start with 1 */
		throughput_test();
		current_x=0;
		current_y++;
	}
	if(Rflag)
		dump_throughput();
        if(tofree)
            free(saveptr);

}



/************************************************************************/
/* Routine to purge the buffer cache by unmounting drive.		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
purge_buffer_cache()
#else
purge_buffer_cache()
#endif
{
	char command[1024];
	int ret,i;
	strcpy(command,"umount ");
	strcat(command, mountname);
        /*
           umount might fail if the device is still busy, so
           retry unmounting several times with increasing delays
        */
        for (i = 1; i < 200; ++i) {
               ret = system(command);
               if (ret == 0)
                       break;
               sleep(i); /* seconds */
        }
	strcpy(command,"mount ");
	strcat(command, mountname);
	/*
         mount might fail if the device is still busy, so
         retry mounting several times with increasing delays
        */
        for (i = 1; i < 10; ++i) {
              ret = system(command);
              if (ret == 0)
                   break;
                   sleep(i); /* seconds */
         }
}

/************************************************************************/
/* Thread write test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_write_test(void *x)
#else
void *
thread_write_test( x)
#endif
{

	struct child_stats *child_stat;
	double starttime1 = 0;
	double temp_time;
	double walltime, cputime;
	double compute_val = (double)0;
	float delay = (float)0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	off64_t traj_offset;
	off64_t lock_offset=0;
	off64_t save_offset=0;
	long long flags,traj_size;
	long long w_traj_bytes_completed;
	long long w_traj_ops_completed;
	FILE *w_traj_fd;
	int fd;
	long long recs_per_buffer;
	long long stopped,i;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	long long xx,xx2;
	char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
	char *nbuff;
	char *maddr;
	char *wmaddr,*free_addr;
	char now_string[30];
	int anwser,bind_cpu,wval;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	off64_t filebytes64;
	char tmpname[256];
	FILE *thread_wqfd;
	FILE *thread_Lwqfd;

#ifdef ASYNC_IO
	struct cache *gc=0;

#else
	long long *gc=0;
#endif

	if(compute_flag)
		delay=compute_time;
	nbuff=maddr=wmaddr=free_addr=0;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	thread_wqfd=w_traj_fd=thread_Lwqfd=(FILE *)0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	if(w_traj_flag)
	{
		filebytes64 = w_traj_fsize;
		numrecs64=w_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	w_traj_bytes_completed=w_traj_ops_completed=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
	{
		xx = (long long)((long)x);
	}
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined(_HPUX_SOURCE) || defined(linux)
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1 )
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	/*****************/
	/* Children only */
	/*******************************************************************/
	/* Initial write throughput performance test. **********************/
	/*******************************************************************/
#if defined(Windows)
	if(unbuffered)
	{
		hand=CreateFile(dummyfile[xx],
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_WRITE|FILE_SHARE_READ,
			NULL,OPEN_ALWAYS,FILE_FLAG_NO_BUFFERING|
			FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
			NULL);
		CloseHandle(hand);
	}
#endif
	if(oflag)
		flags=O_RDWR|O_SYNC|O_CREAT;
	else
		flags=O_RDWR|O_CREAT;
	if(del_flag && mix_test)
		flags |=  O_TRUNC;
#if defined(O_DSYNC)
	if(odsync)
		flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
        else
        {
#endif
	if((fd = I_OPEN(dummyfile[xx], (int)flags,0640))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		printf("\nCan not open temp file: %s\n", 
			filename);
		perror("open");
		exit(125);
	}
#if defined(Windows)
	}
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
               if(direct_flag)
               {
                       test_foo = directio(fd, DIRECTIO_ON);
                       if(test_foo != 0)
                       {
                               if(!client_iozone)
                                 printf("\ndirectio not available.\n");
                               exit(3);
                       }
               }
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(filebytes64),1,PROT_READ|PROT_WRITE);
	}
	if(reclen < cache_size )
	{
		recs_per_buffer = cache_size/reclen ;
		nbuff=&nbuff[(xx%recs_per_buffer)*reclen];
	}
	if(fetchon)			/* Prefetch into processor cache */
		fetchit(nbuff,reclen);
	if((verify && !no_copy_flag) || dedup || dedup_interior)
		fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);

	if(w_traj_flag)
		w_traj_fd=open_w_traj();

	child_stat = (struct child_stats *)&shmaddr[xx];	
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag=CHILD_STATE_READY; /* Tell parent child is ready to go */
	if(distributed && client_iozone)
		tell_master_ready(chid);
	if(distributed && client_iozone)
	{
		if(cdebug)
		{
			fprintf(newstdout,"Child %d waiting for go from master\n",(int)xx);
			fflush(newstdout);
		}
		wait_for_master_go(chid);
		if(cdebug)
		{
			fprintf(newstdout,"Child %d received go from master\n",(int)xx);
			fflush(newstdout);
		}
	}
	else
	{
		while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
			Poll((long long)1);
	}

	written_so_far=0;
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->actual = 0;
	child_stat->throughput = 0;
	stopped=0;
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)0) != 0)
			printf("File lock for write failed. %d\n",errno);
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_wol.dat",(int)xx);
		thread_wqfd=fopen(tmpname,"a");
		if(thread_wqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		fprintf(thread_wqfd,"Offset in kBytes   Latency in microseconds Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Write test start: ",now_string);
	}
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}
	if(w_traj_flag)
		rewind(w_traj_fd);
	for(i=0; i<numrecs64; i++){
		if(w_traj_flag)
		{
			traj_offset=get_traj(w_traj_fd, (long long *)&traj_size,(float *)&delay, (long)1);
			reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
			I_LSEEK(fd,traj_offset,SEEK_SET);
		}
		if(Q_flag)
		{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,0,0,FILE_CURRENT);
			else
#endif
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if((verify && !no_copy_flag) || dedup || dedup_interior)
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag && !stopped){
			if(include_flush)
			{
				if(mmapflag)
					msync(maddr,(size_t)filebytes64,MS_SYNC);
				else
					fsync(fd);
			}
			/* Close and re-open to get close in measurment */
			if(include_close)
			{
			  save_offset=I_LSEEK(fd,0,SEEK_CUR);
			  close(fd);
			}
			child_stat->throughput = 
				(time_so_far() - starttime1)-time_res;
			if(include_close)
			{
			  if((fd = I_OPEN(dummyfile[xx], (int)flags,0))<0)
			  {
				client_error=errno;
				if(distributed && client_iozone)
					send_stop();
				printf("\nCan not open temp file: %s\n", 
					filename);
				perror("open");
				exit(125);
			  }
			  I_LSEEK(fd,save_offset,SEEK_SET);
			}
			if(child_stat->throughput < (double).000001) 
			{
				child_stat->throughput = time_res;
				if(rec_prob < reclen)
					rec_prob = reclen;
				res_prob=1;
			}

			if(OPS_flag){
			   /*written_so_far=(written_so_far*1024)/reclen;*/
			   written_so_far=w_traj_ops_completed;
			}
			child_stat->throughput = 
			  (double)written_so_far/child_stat->throughput;
			child_stat->actual = (double)written_so_far;
			if(debug1)
			{
				printf("\n(%ld) Stopped by another\n", (long)xx);
			}
			stopped=1;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
		{
			thread_qtime_start=time_so_far();
		}
again:		
		if(mmapflag)
		{
			wmaddr = &maddr[i*reclen];
			fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
			/*printf("CHid: %lld  Writing offset %lld for length of %lld\n",chid,i*reclen,reclen);*/
			if(!mmapnsflag)
			{
			  if(mmapasflag)
			    msync(wmaddr,(size_t)reclen,MS_ASYNC);
			  if(mmapssflag)
			    msync(wmaddr,(size_t)reclen,MS_SYNC);
			}
		}
		else
		{
		   if(async_flag)
		   {
			     if(no_copy_flag)
			     {
				free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
				nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
				if(verify || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
			        async_write_no_copy(gc, (long long)fd, nbuff, reclen, (i*reclen), depth,free_addr);
			     }
			     else
				async_write(gc, (long long)fd, nbuff, reclen, (i*reclen), depth);
		   }
		   else
		   {
#if defined(Windows)
		      if(unbuffered)
		      {
			WriteFile(hand,nbuff,reclen, (LPDWORD)&wval,0);
		      }
		      else
		      {
#endif
		      wval=write(fd, nbuff, (size_t) reclen);
#if defined(Windows)
		      }
#endif
		      if(wval != reclen)
		      {
			if(*stop_flag && !stopped){
				if(include_flush)
				{
					if(mmapflag)
						msync(maddr,(size_t)filebytes64,MS_SYNC);
					else
						fsync(fd);
				}
				temp_time = time_so_far();
				child_stat->throughput = 
					(temp_time - starttime1)-time_res;
				if(child_stat->throughput < (double).000001) 
				{
					child_stat->throughput= time_res;
					if(rec_prob < reclen)
						rec_prob = reclen;
					res_prob=1;
				}

				if(OPS_flag){
				   /*written_so_far=(written_so_far*1024)/reclen;*/
				   written_so_far=w_traj_ops_completed;
				}
				child_stat->throughput = 
				  (double)written_so_far/child_stat->throughput;
				child_stat->actual = (double)written_so_far;
				if(debug1)
				{
					printf("\n(%ld) Stopped by another\n", (long)xx);
				}
				stopped=1;
				goto again;
			}
			/* Note: Writer must finish even though told
			   to stop. Otherwise the readers will fail.
			   The code will capture bytes transfered
			   before told to stop but let the writer
			   complete.
			*/
#ifdef NO_PRINT_LLD
		    	printf("\nError writing block %ld, fd= %d\n", i,
				 fd);
#else
		    	printf("\nError writing block %lld, fd= %d\n", i,
				 fd);
#endif
			if(wval==-1)
				perror("write");
			if (!no_unlink)
			{
			   if(check_filename(dummyfile[xx]))
				unlink(dummyfile[xx]);
			}
			child_stat->flag = CHILD_STATE_HOLD;
		    	exit(127);
		      }
		    }
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
			      my_unap((unsigned long long)((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_wqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_wqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
		w_traj_ops_completed++;
		w_traj_bytes_completed+=reclen;
		written_so_far+=reclen/1024;
		if(*stop_flag)
		{
			written_so_far-=reclen/1024;
			w_traj_bytes_completed-=reclen;
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)0,
			  lock_offset, reclen);
		}
	}
	

	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)0))
			printf("Write unlock failed. %d\n",errno);
	
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	
	if(include_flush)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);
		else
			fsync(fd);
	}
	if(include_close)
	{
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
	if(!stopped){
		temp_time = time_so_far();
		if(inc_think)
		   child_stat->throughput = ((temp_time - starttime1)-time_res);
		else
		   child_stat->throughput = ((temp_time - starttime1)-time_res)
			-compute_val;
		if(child_stat->throughput < (double).000001) 
		{
			child_stat->throughput= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}

		if(OPS_flag){
		   /*written_so_far=(written_so_far*1024)/reclen;*/
		   written_so_far=w_traj_ops_completed;
		}
		child_stat->throughput =
			(double)written_so_far/child_stat->throughput;
		child_stat->actual = (double)written_so_far;
	}
	if(cdebug)
	{
		fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid, child_stat->throughput,
			child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
	if(distributed && client_iozone)
		tell_master_stats(THREAD_WRITE_TEST, chid, child_stat->throughput, 
			child_stat->actual, 
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
			(long long)CHILD_STATE_HOLD);
			
	if (debug1) {
#ifdef NO_PRINT_LLD
		printf(" child/slot: %ld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#else
		printf(" child/slot: %lld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#endif
			xx, walltime, cputime,
			cpu_util(cputime, walltime));
	}
	child_stat->flag = CHILD_STATE_HOLD; /* Tell parent I'm done */
	stopped=0;
	/*******************************************************************/
	/* End write performance test. *************************************/
	/*******************************************************************/
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)numrecs64*reclen,MS_SYNC); /*Clean up before read starts running*/ 
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
			
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
	if(Q_flag && (thread_wqfd !=0) )
		fclose(thread_wqfd);
	free(dummyfile[xx]);
	if(w_traj_flag)
		fclose(w_traj_fd);

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Write test finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("write",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

#ifdef HAVE_PREAD
/************************************************************************/
/* Thread pwrite test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_pwrite_test(void *x)
#else
void *
thread_pwrite_test( x)
#endif
{

	struct child_stats *child_stat;
	double starttime1 = 0;
	double temp_time;
	double walltime, cputime;
	double compute_val = (double)0;
	float delay = (float)0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	off64_t traj_offset;
	off64_t lock_offset=0;
	long long flags,traj_size;
	long long w_traj_bytes_completed;
	long long w_traj_ops_completed;
	FILE *w_traj_fd;
	int fd;
	long long recs_per_buffer;
	long long stopped,i;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	long long xx,xx2;
	char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
	char *nbuff;
	char *maddr;
	char *wmaddr,*free_addr;
	char now_string[30];
	int anwser,bind_cpu,wval;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	off64_t filebytes64;
	char tmpname[256];
	FILE *thread_wqfd;
	FILE *thread_Lwqfd;

#ifdef ASYNC_IO
	struct cache *gc=0;

#else
	long long *gc=0;
#endif

	if(compute_flag)
		delay=compute_time;
	nbuff=maddr=wmaddr=free_addr=0;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	thread_wqfd=w_traj_fd=thread_Lwqfd=(FILE *)0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	if(w_traj_flag)
	{
		filebytes64 = w_traj_fsize;
		numrecs64=w_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	w_traj_bytes_completed=w_traj_ops_completed=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
	{
		xx = (long long)((long)x);
	}
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined( _HPUX_SOURCE )
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;
     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1 )
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
	   sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
	   sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
	   sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
	   sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	/*****************/
	/* Children only */
	/*******************************************************************/
	/* Initial pwrite throughput performance test. *********************/
	/*******************************************************************/
	if(!notruncate)
	{
		if((fd = I_CREAT(dummyfile[xx], 0640))<0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			perror(dummyfile[xx]);
			exit(123);
		}
		close(fd);
	}
	if(oflag)
		flags=O_RDWR|O_SYNC|O_CREAT;
	else
		flags=O_RDWR|O_CREAT;
#if defined(O_DSYNC)
	if(odsync)
		flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
	if((fd = I_OPEN(dummyfile[xx], (int)flags,0640))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		printf("\nCan not open temp file: %s\n", 
			filename);
		perror("open");
		exit(125);
	}
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(filebytes64),1,PROT_READ|PROT_WRITE);
	}
	if(reclen < cache_size )
	{
		recs_per_buffer = cache_size/reclen ;
		nbuff=&nbuff[(xx%recs_per_buffer)*reclen];
	}
	if(fetchon)			/* Prefetch into processor cache */
		fetchit(nbuff,reclen);
	if((verify && !no_copy_flag) || dedup || dedup_interior)
		fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);

	if(w_traj_flag)
		w_traj_fd=open_w_traj();

	child_stat = (struct child_stats *)&shmaddr[xx];	
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag=CHILD_STATE_READY; /* Tell parent child is ready to go */
	if(distributed && client_iozone)
		tell_master_ready(chid);
	if(distributed && client_iozone)
	{
		if(cdebug)
		{
			fprintf(newstdout,"Child %d waiting for go from master\n",(int)xx);
			fflush(newstdout);
		}
		wait_for_master_go(chid);
		if(cdebug)
		{
			fprintf(newstdout,"Child %d received go from master\n",(int)xx);
			fflush(newstdout);
		}
	}
	else
	{
		while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
			Poll((long long)1);
	}

	written_so_far=0;
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->actual = 0;
	child_stat->throughput = 0;
	stopped=0;
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)0) != 0)
			printf("File lock for write failed. %d\n",errno);
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_pwol.dat",(int)xx);
		thread_wqfd=fopen(tmpname,"a");
		if(thread_wqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		fprintf(thread_wqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Pwrite test start: ",now_string);
	}
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}
	if(w_traj_flag)
		rewind(w_traj_fd);
	for(i=0; i<numrecs64; i++){
		traj_offset= ( i * reclen );
		if(w_traj_flag)
		{
			traj_offset=get_traj(w_traj_fd, (long long *)&traj_size,(float *)&delay, (long)1);
			reclen=traj_size;
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if((verify && !no_copy_flag) || dedup || dedup_interior)
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag && !stopped){
			if(include_flush)
			{
				if(mmapflag)
					msync(maddr,(size_t)filebytes64,MS_SYNC);
				else
					fsync(fd);
			}
			temp_time = time_so_far();
			if(inc_think)
			   child_stat->throughput = ((temp_time - starttime1)-time_res);
			else
			   child_stat->throughput = ((temp_time - starttime1)-time_res)
			      				-compute_val;
			if(child_stat->throughput < (double).000001) 
			{
				child_stat->throughput = time_res;
				if(rec_prob < reclen)
					rec_prob = reclen;
				res_prob=1;
			}

			if(OPS_flag){
			   /*written_so_far=(written_so_far*1024)/reclen;*/
			   written_so_far=w_traj_ops_completed;
			}
			child_stat->throughput = 
			  (double)written_so_far/child_stat->throughput;
			child_stat->actual = (double)written_so_far;
			if(debug1)
			{
				printf("\n(%ld) Stopped by another\n", (long)xx);
			}
			stopped=1;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
		{
			thread_qtime_start=time_so_far();
		}
again:		
		if(mmapflag)
		{
			wmaddr = &maddr[traj_offset];
			fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
			/*printf("CHid: %lld  Writing offset %lld for length of %lld\n",chid,i*reclen,reclen);*/
			if(!mmapnsflag)
			{
			  if(mmapasflag)
			    msync(wmaddr,(size_t)reclen,MS_ASYNC);
			  if(mmapssflag)
			    msync(wmaddr,(size_t)reclen,MS_SYNC);
			}
		}
		else
		{
		   if(async_flag)
		   {
			     if(no_copy_flag)
			     {
				free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
				nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
				if(verify || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
			        async_write_no_copy(gc, (long long)fd, nbuff, reclen, (traj_offset), depth,free_addr);
			     }
			     else
				async_write(gc, (long long)fd, nbuff, reclen, (traj_offset), depth);
		   }
		   else
		   {
		      wval=I_PWRITE(fd, nbuff, reclen, traj_offset);
		      if(wval != reclen)
		      {
			if(*stop_flag && !stopped){
				if(include_flush)
				{
					if(mmapflag)
						msync(maddr,(size_t)filebytes64,MS_SYNC);
					else
						fsync(fd);
				}
				temp_time = time_so_far();
				child_stat->throughput = 
					(temp_time - starttime1)-time_res;
				if(child_stat->throughput < (double).000001) 
				{
					child_stat->throughput= time_res;
					if(rec_prob < reclen)
						rec_prob = reclen;
					res_prob=1;
				}

				if(OPS_flag){
				   /*written_so_far=(written_so_far*1024)/reclen;*/
				   written_so_far=w_traj_ops_completed;
				}
				child_stat->throughput = 
				  (double)written_so_far/child_stat->throughput;
				child_stat->actual = (double)written_so_far;
				if(debug1)
				{
					printf("\n(%ld) Stopped by another\n", (long)xx);
				}
				stopped=1;
				goto again;
			}
			/* Note: Writer must finish even though told
			   to stop. Otherwise the readers will fail.
			   The code will capture bytes transfered
			   before told to stop but let the writer
			   complete.
			*/
#ifdef NO_PRINT_LLD
		    	printf("\nError pwriting block %ld, fd= %d\n", i,
				 fd);
#else
		    	printf("\nError pwriting block %lld, fd= %d\n", i,
				 fd);
#endif
			if(wval==-1)
				perror("pwrite");
			if (!no_unlink)
			{
			   if(check_filename(dummyfile[xx]))
				unlink(dummyfile[xx]);
			}
			child_stat->flag = CHILD_STATE_HOLD;
		    	exit(127);
		      }
		    }
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)0,
			  lock_offset, reclen);
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_wqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_wqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
		w_traj_ops_completed++;
		w_traj_bytes_completed+=reclen;
		written_so_far+=reclen/1024;
		if(*stop_flag)
		{
			written_so_far-=reclen/1024;
			w_traj_bytes_completed-=reclen;
		}
	}
	

	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)0))
			printf("Write unlock failed. %d\n",errno);
	
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	
	if(include_flush)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);
		else
			fsync(fd);
	}
	if(include_close)
	{
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
		close(fd);
	}
	if(!stopped){
		temp_time = time_so_far();
		if(inc_think)
			child_stat->throughput = ((temp_time - starttime1)-time_res);
		else
			child_stat->throughput = ((temp_time - starttime1)-time_res)
				-compute_val;
		if(child_stat->throughput < (double).000001) 
		{
			child_stat->throughput= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}

		if(OPS_flag){
		   /*written_so_far=(written_so_far*1024)/reclen;*/
		   written_so_far=w_traj_ops_completed;
		}
		child_stat->throughput =
			(double)written_so_far/child_stat->throughput;
		child_stat->actual = (double)written_so_far;
	}
	if(cdebug)
	{
		fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid, child_stat->throughput,
			child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
	if(distributed && client_iozone)
		tell_master_stats(THREAD_PWRITE_TEST, chid, child_stat->throughput, 
			child_stat->actual, 
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
			(long long)CHILD_STATE_HOLD);
			
	if (debug1) {
#ifdef NO_PRINT_LLD
		printf(" child/slot: %ld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#else
		printf(" child/slot: %lld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#endif
			xx, walltime, cputime,
			cpu_util(cputime, walltime));
	}
	child_stat->flag = CHILD_STATE_HOLD; /* Tell parent I'm done */
	stopped=0;
	/*******************************************************************/
	/* End pwrite performance test. *************************************/
	/*******************************************************************/
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)numrecs64*reclen,MS_SYNC); /*Clean up before read starts running*/ 
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
			
		close(fd);
	}
	if(Q_flag && (thread_wqfd !=0) )
		fclose(thread_wqfd);
	free(dummyfile[xx]);
	if(w_traj_flag)
		fclose(w_traj_fd);

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Pwrite test finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Pwrite",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}
#endif

/************************************************************************/
/* Thread re-write test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_rwrite_test(void *x)
#else
void *
thread_rwrite_test(x)
#endif
{
	/************************/
	/* Children only here 	*/
	/************************/
	struct child_stats *child_stat;
	long long xx,xx2;
	double compute_val = (double)0;
	double walltime, cputime;
	float delay = (float)0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	off64_t traj_offset;
	off64_t lock_offset=0;
	long long w_traj_bytes_completed;
	long long w_traj_ops_completed;
	int fd;
	FILE *w_traj_fd;
	long long flags = 0;
	double starttime1 = 0;
	double temp_time;
	long long recs_per_buffer,traj_size;
	long long i;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far=0;
	char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
	char *nbuff;
	char *maddr,*free_addr;
	char *wmaddr;
	char now_string[30];
	int anwser,bind_cpu,wval;
	FILE *thread_rwqfd,*thread_Lwqfd;
	char tmpname[256];
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;

#else
	long long *gc=0;
#endif

	if(compute_flag)
		delay=compute_time;
	wmaddr=nbuff=maddr=free_addr=0;
	thread_rwqfd=w_traj_fd=thread_Lwqfd=(FILE *)0;
	hist_time=traj_offset=thread_qtime_stop=thread_qtime_start=0;
	walltime=cputime=0;
	anwser=bind_cpu=0;
	w_traj_bytes_completed=w_traj_ops_completed=0;
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
	if(w_traj_flag)
	{
		filebytes64 = w_traj_fsize;
		numrecs64=w_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx=(long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined( _HPUX_SOURCE )
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;
     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	flags = O_RDWR;
	if(oflag)
		flags|= O_SYNC;
#if defined(O_DSYNC)
	if(odsync)
		flags|= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif

#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_ALWAYS,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
        else
        {
#endif
	if((fd = I_OPEN(dummyfile[xx], (int)flags,0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
#ifdef NO_PRINT_LLD
		printf("\nChild %ld\n",xx);
#else
		printf("\nChild %lld\n",xx);
#endif
		child_stat->flag = CHILD_STATE_HOLD;
		perror(dummyfile[xx]);
		exit(128);
	}
#if defined(Windows)
	}
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),1,PROT_READ|PROT_WRITE);
	}
	if(fetchon)
		fetchit(nbuff,reclen);
	if(w_traj_flag)
		w_traj_fd=open_w_traj();
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_rwol.dat",(int)xx);
		thread_rwqfd=fopen(tmpname,"a");
		if(thread_rwqfd==0)
		{
			printf("Unable to open %s\n",tmpname);
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			exit(40);
		}
		fprintf(thread_rwqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Rewrite test start: ",now_string);
	}
	child_stat->flag = CHILD_STATE_READY;
	if(distributed && client_iozone)
		tell_master_ready(chid);
	if(distributed && client_iozone)
	{
		if(cdebug)
		{
			fprintf(newstdout,"Child %d waiting for go from master\n",(int)xx);
			fflush(newstdout);
		}
		wait_for_master_go(chid);
		if(cdebug)
		{
			fprintf(newstdout,"Child %d received go from master\n",(int)xx);
			fflush(newstdout);
		}
	}
	else
	{
		while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
			Poll((long long)1);
	}
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)0) != 0)
			printf("File lock for write failed. %d\n",errno);
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}
	if(w_traj_flag)
		rewind(w_traj_fd);
	if((verify && !no_copy_flag) || dedup || dedup_interior)
		fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
	for(i=0; i<numrecs64; i++){
		traj_offset= i*reclen ;
		if(w_traj_flag)
		{
			traj_offset=get_traj(w_traj_fd, (long long *)&traj_size,(float *)&delay,(long)1);
			reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
			I_LSEEK(fd,traj_offset,SEEK_SET);
		}
		if(Q_flag)
		{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,(LONG)0,0,FILE_CURRENT);
			else
#endif
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag && !mmapflag)
		{
			if(debug1)
				printf("\nStop_flag 1\n");
			break;
		}
		if((verify && !no_copy_flag) || dedup || dedup_interior)
		{
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
		{
			thread_qtime_start=time_so_far();
		}
		if(mmapflag)
		{
			wmaddr = &maddr[i*reclen];
			if(cdebug)
			{
#ifdef NO_PRINT_LLD
fprintf(newstdout,"Chid: %ld Rewriting offset %ld for length of %ld\n",chid, i*reclen,reclen);
#else
fprintf(newstdout,"Chid: %lld Rewriting offset %lld for length of %lld\n",chid, i*reclen,reclen);
#endif
			  fflush(newstdout);
			}
			fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
			if(!mmapnsflag)
			{
			  if(mmapasflag)
			    msync(wmaddr,(size_t)reclen,MS_ASYNC);
			  if(mmapssflag)
			    msync(wmaddr,(size_t)reclen,MS_SYNC);
			}
		}
		else
		{
		   	if(async_flag)
		   	{
			     if(no_copy_flag)
			     {
				free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
				nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
				if(verify || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,i);
			        async_write_no_copy(gc, (long long)fd, nbuff, reclen, (i*reclen), depth,free_addr);
			     }
			     else
				async_write(gc, (long long)fd, nbuff, reclen, (i*reclen), depth);
		   	}
			else
			{
#if defined(Windows)
			   if(unbuffered)
			   {
				WriteFile(hand,nbuff,reclen,(LPDWORD)&wval,0);
			   }
			   else
#endif
			   wval=write(fd, nbuff, (size_t) reclen);
			   if(wval != reclen)
			   {
				if(*stop_flag)
				{
					if(debug1)
						printf("\nStop_flag 2\n");
					break;
				}
#ifdef NO_PRINT_LLD
		    		printf("\nError writing block %ld, fd= %d\n", i,
					 fd);
#else
		    		printf("\nError writing block %lld, fd= %d\n", i,
					 fd);
#endif
				if(wval==-1)
					perror("write");
				exit_code = 74;
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				signal_handler();
			   }
			}
		}
		re_written_so_far+=reclen/1024;
	   	w_traj_ops_completed++;
		w_traj_bytes_completed+=reclen;
		if(*stop_flag)
		{
			re_written_so_far-=reclen/1024;
			w_traj_bytes_completed-=reclen;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_rwqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_rwqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)0,
			  lock_offset, reclen);
		}
	}
	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)0))
			printf("Write unlock failed. %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
	temp_time=time_so_far();
	child_stat=(struct child_stats *)&shmaddr[xx];
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime1)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime1)-time_res)
		-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(OPS_flag){
	   /*re_written_so_far=(re_written_so_far*1024)/reclen;*/
	   re_written_so_far=w_traj_ops_completed;
	}
	child_stat->throughput = 
		(double)re_written_so_far/child_stat->throughput;
	child_stat->actual = (double)re_written_so_far;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	if(cdebug)
	{
		fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid, child_stat->throughput,
			child_stat->actual);
	  	fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
	if(distributed && client_iozone)
		tell_master_stats(THREAD_REWRITE_TEST, chid, child_stat->throughput, 
			child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
			(long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD;	/* Tell parent I'm done */
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
		else
			fsync(fd);
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
	free(dummyfile[xx]);

	if(Q_flag && (thread_rwqfd !=0) )
		fclose(thread_rwqfd);

	if(w_traj_flag)
		fclose(w_traj_fd);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild Stopping  %ld\n",xx);
#else
		printf("\nChild Stopping  %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Rewrite test finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Rewrite",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

/************************************************************************/
/* Thread read test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_read_test(void *x)
#else
void *
thread_read_test(x)
#endif
{
	long long xx,xx2;
	struct child_stats *child_stat;
	double walltime, cputime;
	long long r_traj_bytes_completed;
	long long r_traj_ops_completed;
	int fd;
	FILE *r_traj_fd,*thread_rqfd;
	FILE *thread_Lwqfd;
	long long flags = 0;
	off64_t traj_offset;
	off64_t lock_offset=0;
	double starttime1 = 0;
	float delay = 0;
	double temp_time;
	double thread_qtime_start,thread_qtime_stop;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double compute_val = (double)0;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	long long recs_per_buffer,traj_size;
	off64_t i;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *nbuff=0;
	char *maddr=0;
	char *wmaddr=0;
	char tmpname[256];
	volatile char *buffer1;
	char now_string[30];
	int anwser,bind_cpu;
	long wval;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	if(compute_flag)
		delay=compute_time;
	thread_rqfd=thread_Lwqfd=r_traj_fd=(FILE *)0;
	hist_time=traj_offset=thread_qtime_stop=thread_qtime_start=0;
	walltime=cputime=0;
	anwser=bind_cpu=0;
	r_traj_bytes_completed=r_traj_ops_completed=0;
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
	if(r_traj_flag)
	{
		filebytes64 = r_traj_fsize;
		numrecs64=r_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
	
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
		flags=O_RDONLY|O_SYNC;
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
		SetFilePointer(hand,(LONG)0,0,FILE_BEGIN);
       	}
	else
	{
#endif
	if((fd = I_OPEN(dummyfile[xx], (int)flags,0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(130);
	}
#if defined(Windows)
	}
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),0,PROT_READ);
	}
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	/*****************/
	/* Children only */
	/*****************/
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_rol.dat",(int)xx);
		thread_rqfd=fopen(tmpname,"a");
		if(thread_rqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		fprintf(thread_rqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Read test start: ",now_string);
	}

	if(r_traj_flag)
		r_traj_fd=open_r_traj();
	if(fetchon)
		fetchit(nbuff,reclen);
	child_stat=(struct child_stats *)&shmaddr[xx];
	child_stat->flag = CHILD_STATE_READY;
	if(distributed && client_iozone)
        {
		tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else
        {
		/* Wait for signal from parent */
                while(child_stat->flag!=CHILD_STATE_BEGIN)   
                        Poll((long long)1);
        }
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)1) != 0)
			printf("File lock for read failed. %d\n",errno);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}

	if(r_traj_flag)
		rewind(r_traj_fd);
	for(i=0; i<numrecs64; i++){
		traj_offset= i*reclen;
		if(disrupt_flag && ((i%DISRUPT)==0))
		{
#if defined(Windows)
	
			if(unbuffered)
			   disruptw(hand);
			else
			   disrupt(fd);
#else
			disrupt(fd);
#endif
		}
		if(r_traj_flag)
		{
			traj_offset=get_traj(r_traj_fd, (long long *)&traj_size,(float *)&delay,(long)0);
			reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
			I_LSEEK(fd,traj_offset,SEEK_SET);
		}
		if(Q_flag)
		{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,0,0,FILE_CURRENT);
			else
#endif
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 2\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
                {
                        thread_qtime_start=time_so_far();
                }
		if(mmapflag)
		{
			wmaddr = &maddr[i*reclen];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			  if(async_flag)
			  {
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (i*reclen), reclen,
			    	1LL,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (i*reclen), reclen,
			    	1LL,(numrecs64*reclen),depth);
			  }
			  else
			  {
#if defined(Windows)
			      if(unbuffered)
			      {
				ReadFile(hand,nbuff,reclen,(LPDWORD)&wval,0);
			      }
			      else
#endif
			      wval=read((int)fd, (void*)nbuff, (size_t) reclen);
			      if(wval != reclen)
			      {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 2\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
		    		printf("\nError reading block %ld, fd= %d\n", i,
					 fd);
#else
		    		printf("\nError reading block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("read");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
		    		exit(132);
			      }
			  }
		}
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			   if(verify_buffer(buffer1,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(133);
			   }
		   }
		   else
		   {
			   if(verify_buffer(nbuff,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(134);
			   }
		   }
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
		read_so_far+=reclen/1024;
		r_traj_bytes_completed+=reclen;
		r_traj_ops_completed++;
		if(*stop_flag)
		{
			read_so_far-=reclen/1024;
			r_traj_bytes_completed-=reclen;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_rqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_rqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }

		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
	}
	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)1))
			printf("Read unlock failed. %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
#if defined(Windows)
		if(unbuffered)
		  CloseHandle(hand);
		else
#endif
		close(fd);
	}
	temp_time = time_so_far();
	child_stat=(struct child_stats *)&shmaddr[xx];
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime1)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime1)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(OPS_flag){
	   /*read_so_far=(read_so_far*1024)/reclen;*/
	   read_so_far=r_traj_ops_completed;
	}
	child_stat->throughput = read_so_far/child_stat->throughput;
	child_stat->actual = read_so_far;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
        if(cdebug)
	{
                fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid,child_stat->throughput,
                        child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
                tell_master_stats(THREAD_READ_TEST, chid, child_stat->throughput,
                        child_stat->actual, 
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD; 	/* Tell parent I'm done */
	/*fsync(fd);*/
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
			mmap_end(maddr,(unsigned long long)filebytes64);
		}else
			fsync(fd);
#if defined(Windows)
		if(unbuffered)
		  CloseHandle(hand);
		else
#endif
		close(fd);
	}
        if(Q_flag && (thread_rqfd !=0) )
                fclose(thread_rqfd);
	free(dummyfile[xx]);
	if(r_traj_flag)
		fclose(r_traj_fd);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Read test finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Read",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

#ifdef HAVE_PREAD
/************************************************************************/
/* Thread pread test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_pread_test(void *x)
#else
void *
thread_pread_test(x)
#endif
{
	long long xx,xx2;
	struct child_stats *child_stat;
	double walltime, cputime;
	long long r_traj_bytes_completed;
	long long r_traj_ops_completed;
	int fd;
	FILE *r_traj_fd,*thread_rqfd;
	FILE *thread_Lwqfd;
	long long flags = 0;
	off64_t traj_offset;
	off64_t lock_offset=0;
	double starttime1 = 0;
	float delay = 0;
	double temp_time;
	double thread_qtime_start,thread_qtime_stop;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double compute_val = (double)0;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	long long recs_per_buffer,traj_size;
	off64_t i;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *nbuff=0;
	char *maddr=0;
	char *wmaddr=0;
	char tmpname[256];
	char now_string[30];
	volatile char *buffer1;
	int anwser,bind_cpu;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif

	if(compute_flag)
		delay=compute_time;
	thread_rqfd=thread_Lwqfd=r_traj_fd=(FILE *)0;
	hist_time=traj_offset=thread_qtime_stop=thread_qtime_start=0;
	walltime=cputime=0;
	anwser=bind_cpu=0;
	r_traj_bytes_completed=r_traj_ops_completed=0;
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
	if(r_traj_flag)
	{
		filebytes64 = r_traj_fsize;
		numrecs64=r_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
	
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#ifdef _HPUX_SOURCE
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
		flags=O_RDONLY|O_SYNC;
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
	if((fd = I_OPEN(dummyfile[xx], (int)flags,0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(130);
	}
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),0,PROT_READ);
	}
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	/*****************/
	/* Children only */
	/*****************/
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_prol.dat",(int)xx);
		thread_rqfd=fopen(tmpname,"a");
		if(thread_rqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		fprintf(thread_rqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Pread test start: ",now_string);
	}

	if(r_traj_flag)
		r_traj_fd=open_r_traj();
	if(fetchon)
		fetchit(nbuff,reclen);
	child_stat=(struct child_stats *)&shmaddr[xx];
	child_stat->flag = CHILD_STATE_READY;
	if(distributed && client_iozone)
        {
		tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else
        {
		/* Wait for signal from parent */
                while(child_stat->flag!=CHILD_STATE_BEGIN)   
                        Poll((long long)1);
        }
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)1) != 0)
			printf("File lock for read failed. %d\n",errno);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}

	if(r_traj_flag)
		rewind(r_traj_fd);
	for(i=0; i<numrecs64; i++){
		traj_offset = i*reclen;
		if(disrupt_flag && ((i%DISRUPT)==0))
		{
			disrupt(fd);
		}
		if(r_traj_flag)
		{
			traj_offset=get_traj(r_traj_fd, (long long *)&traj_size,(float *)&delay,(long)0);
			reclen=traj_size;
			I_LSEEK(fd,traj_offset,SEEK_SET);
		}
		if(Q_flag)
		{
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 2\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
                {
                        thread_qtime_start=time_so_far();
                }
		if(mmapflag)
		{
			wmaddr = &maddr[traj_offset];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			  if(async_flag)
			  {
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (i*reclen), reclen,
			    	1LL,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (traj_offset), reclen,
			    	1LL,(numrecs64*reclen),depth);
			  }
			  else
			  {
				if(I_PREAD((int)fd, (void*)nbuff, (size_t) reclen,(traj_offset) ) != reclen)
			      {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 2\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
		    		printf("\nError preading block %ld, fd= %d\n", i,
					 fd);
#else
		    		printf("\nError preading block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("pread");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
		    		exit(132);
			      }
			  }
		}
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			   if(verify_buffer(buffer1,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(133);
			   }
		   }
		   else
		   {
			   if(verify_buffer(nbuff,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(134);
			   }
		   }
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
		read_so_far+=reclen/1024;
		r_traj_bytes_completed+=reclen;
		r_traj_ops_completed++;
		if(*stop_flag)
		{
			read_so_far-=reclen/1024;
			r_traj_bytes_completed-=reclen;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_rqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_rqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }

		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
	}
	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)1))
			printf("Read unlock failed. %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
		close(fd);
	}
	temp_time = time_so_far();
	child_stat=(struct child_stats *)&shmaddr[xx];
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime1)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime1)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(OPS_flag){
	   /*read_so_far=(read_so_far*1024)/reclen;*/
	   read_so_far=r_traj_ops_completed;
	}
	child_stat->throughput = read_so_far/child_stat->throughput;
	child_stat->actual = read_so_far;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
        if(cdebug)
	{
                fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid,child_stat->throughput,
                        child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
                tell_master_stats(THREAD_READ_TEST, chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD; 	/* Tell parent I'm done */
	/*fsync(fd);*/
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
			mmap_end(maddr,(unsigned long long)filebytes64);
		}else
			fsync(fd);
		close(fd);
	}
        if(Q_flag && (thread_rqfd !=0) )
                fclose(thread_rqfd);
	free(dummyfile[xx]);
	if(r_traj_flag)
		fclose(r_traj_fd);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Pread test finished: ",
			now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Pread",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}
#endif

/************************************************************************/
/* Thread re-read test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_rread_test(void *x)
#else
void *
thread_rread_test(x)
#endif
{
	long long xx,xx2;
	char *nbuff;
	struct child_stats *child_stat;
	int fd;
	FILE *r_traj_fd,*thread_rrqfd;
	FILE *thread_Lwqfd;
	long long r_traj_bytes_completed;
	double walltime, cputime;
	long long r_traj_ops_completed;
	off64_t traj_offset;
	off64_t lock_offset=0;
	long long flags = 0;
	double starttime1 = 0;
	float delay = 0;
	double temp_time;
	double thread_qtime_start,thread_qtime_stop;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double compute_val = (double)0;
	long long recs_per_buffer,traj_size;
	off64_t i;
	off64_t written_so_far, read_so_far, re_written_so_far,
		re_read_so_far;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *maddr=0;
	char *wmaddr=0;
	char now_string[30];
	volatile char *buffer1;
	int anwser,bind_cpu;
	long wval;
	char tmpname[256];
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif
	/*****************/
	/* Children only */
	/*****************/
	if(compute_flag)
		delay=compute_time;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	thread_rrqfd=r_traj_fd=thread_Lwqfd=(FILE *)0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	r_traj_bytes_completed=r_traj_ops_completed=0;
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(r_traj_flag)
	{
		filebytes64 = r_traj_fsize;
		numrecs64=r_traj_ops;
	}
	else
	{
		filebytes64 = numrecs64*reclen;
	}
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
		flags=O_RDONLY|O_SYNC;
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif

#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
		SetFilePointer(hand,(LONG)0,0,FILE_BEGIN);
       	}
	else
	{
#endif
	if((fd = I_OPEN(dummyfile[xx], ((int)flags),0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(135);
	}
#if defined(Windows)
	}
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(filebytes64),0,PROT_READ);
	}
	if(r_traj_flag)
		r_traj_fd=open_r_traj();
	if(fetchon)
		fetchit(nbuff,reclen);
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_rrol.dat",(int)xx);
                thread_rrqfd=fopen(tmpname,"a");
                if(thread_rrqfd==0)
                {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
		fprintf(thread_rrqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Reread test start: ",now_string);
	}

	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag = CHILD_STATE_READY;

	if(distributed && client_iozone)
	{
		tell_master_ready(chid);
		wait_for_master_go(chid);
	}
	else
	
		/* Wait for signal from parent */
                while(child_stat->flag!=CHILD_STATE_BEGIN)   
                        Poll((long long)1);
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)1) != 0)
			printf("File lock for read failed. %d\n",errno);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}

	if(r_traj_flag)
		rewind(r_traj_fd);
	for(i=0; i<numrecs64; i++){
		traj_offset=i*reclen;
		if(disrupt_flag && ((i%DISRUPT)==0))
		{
#if defined(Windows)
	
			if(unbuffered)
			   disruptw(hand);
			else
			   disrupt(fd);
#else
			disrupt(fd);
#endif
		}
		if(r_traj_flag)
		{
			traj_offset=get_traj(r_traj_fd, (long long *)&traj_size,(float *)&delay,(long)0);
			reclen=traj_size;
#if defined(Windows)
			if(unbuffered)
			  SetFilePointer(hand,(LONG)traj_offset,0,FILE_BEGIN);
			else
#endif
			I_LSEEK(fd,traj_offset,SEEK_SET);
		}
		if(Q_flag)
		{
#if defined(Windows)
			if(unbuffered)
			  traj_offset=SetFilePointer(hand,(LONG)0,0,FILE_CURRENT);
			else
#endif
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 3\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
                if(Q_flag || hist_summary || op_rate_flag)
                {
                        thread_qtime_start=time_so_far();
                }
		if(mmapflag)
		{
			wmaddr = &maddr[i*reclen];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			  if(async_flag)
			  {
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (i*reclen),reclen,
			    	1LL,(filebytes64),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (i*reclen),reclen,
			    	1LL,(filebytes64),depth);
			  }
			  else
			  {
#if defined(Windows)
			      if(unbuffered)
			      {
				ReadFile(hand,nbuff,reclen,(LPDWORD)&wval,0);
			      }
			      else
#endif
			      wval=read((int)fd, (void*)nbuff, (size_t) reclen);
			      if(wval != reclen)
			      {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 4\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
		    		printf("\nError writing block %ld, fd= %d\n", i,
					 fd);
#else
		    		printf("\nError writing block %lld, fd= %d\n", i,
					 fd);
#endif
				perror("read");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
		    		exit(137);
			      }
			   }
		}
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			if(verify_buffer(buffer1,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(138);
			}
		   }
		   else
		   {
			if(verify_buffer(nbuff,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(139);
			}
		   }
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
		re_read_so_far+=reclen/1024;
		r_traj_bytes_completed+=reclen;
		r_traj_ops_completed++;
		if(*stop_flag)
		{
			re_read_so_far-=reclen/1024;
			r_traj_bytes_completed-=reclen;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_rrqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_rrqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }

		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
	}
	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)1))
			printf("Read unlock failed. %d\n",errno);
	/*fsync(fd);*/
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)filebytes64);
		}
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
	temp_time = time_so_far();
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime1)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime1)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}

	if(OPS_flag){
	   /*re_read_so_far=(re_read_so_far*1024)/reclen;*/
	   re_read_so_far=r_traj_ops_completed;
	}
	child_stat->throughput = re_read_so_far/child_stat->throughput;
	child_stat->actual = re_read_so_far;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
	if(distributed && client_iozone)
	{
		tell_master_stats(THREAD_REREAD_TEST,chid, child_stat->throughput,
			child_stat->actual, 
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
			(long long)CHILD_STATE_HOLD);
	}
	child_stat->flag = CHILD_STATE_HOLD;	/* Tell parent I'm done */
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(filebytes64),MS_SYNC);
			mmap_end(maddr,(unsigned long long)filebytes64);
		}else
			fsync(fd);
#if defined(Windows)
		if(unbuffered)
			CloseHandle(hand);
		else
#endif
		close(fd);
	}
        if(Q_flag && (thread_rrqfd !=0) )
                fclose(thread_rrqfd);
	free(dummyfile[xx]);
	if(r_traj_flag)
		fclose(r_traj_fd);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Reread test finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Reread",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

/************************************************************************/
/* Thread_reverse_perf_test				        	*/
/* Reverse read test							*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_reverse_read_test(void *x)
#else
void *
thread_reverse_read_test(x)
#endif
{
	long long xx,xx2;
	char *nbuff;
	struct child_stats *child_stat;
	int fd;
	long long flags = 0;
	double walltime, cputime;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double starttime2 = 0;
	float delay = 0;
	double temp_time;
	double compute_val = (double)0;
	long long recs_per_buffer;
	off64_t i,t_offset;
	off64_t lock_offset=0;
	off64_t current_position=0;
	off64_t written_so_far, reverse_read, re_read_so_far,read_so_far;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *maddr=0;
	char *wmaddr=0;
	char now_string[30];
	volatile char *buffer1;
	int anwser,bind_cpu;
	off64_t traj_offset;
	char tmpname[256];
	FILE *thread_revqfd=0;
	FILE *thread_Lwqfd=0;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif
	/*****************/
	/* Children only */
	/*****************/
	if(compute_flag)
		delay=compute_time;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	written_so_far=read_so_far=reverse_read=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
		flags=O_RDONLY|O_SYNC;
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
#endif

	if((fd = I_OPEN(dummyfile[xx], ((int)flags),0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(140);
	}
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),0,PROT_READ);
	}
	if(fetchon)
		fetchit(nbuff,reclen);
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_revol.dat",(int)xx);
                thread_revqfd=fopen(tmpname,"a");
                if(thread_revqfd==0)
                {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
		fprintf(thread_revqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Reverse read start: ",now_string);
	}
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag = CHILD_STATE_READY;
        if(distributed && client_iozone)
        {
                tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else
        {
                while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
                        Poll((long long)1);
        }
	starttime2 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime2;
		cputime = cputime_so_far();
	}

	t_offset = (off64_t)reclen;
	if (!(h_flag || k_flag || mmapflag))
	{
	  if(check_filename(dummyfile[xx]))
	  {
	  	if((I_LSEEK( fd, -t_offset, SEEK_END ))<0)
	  	{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			perror("lseek");
			exit(142);
	  	}
	  }
	  else
	  {
		if(I_LSEEK( fd, (numrecs64*reclen)-t_offset, SEEK_SET )<0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			perror("lseek");
			exit(77);
		}
	  }
	}
	current_position=(reclen*numrecs64)-reclen;
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)1)!=0)
			printf("File lock for read failed. %d\n",errno);
	for(i=0; i<numrecs64; i++) 
	{
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(disrupt_flag && ((i%DISRUPT)==0))
		{
			disrupt(fd);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(Q_flag)
		{
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 3\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
                if(Q_flag || hist_summary || op_rate_flag)
                {
                        thread_qtime_start=time_so_far();
                }
		if(mmapflag)
		{
			wmaddr = &maddr[current_position];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			  if(async_flag)
			  {
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (current_position),
			      	reclen, -1LL,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (current_position),reclen,
			    	-1LL,(numrecs64*reclen),depth);
			  }
			  else
			  {
			      if(read((int)fd, (void*)nbuff, (size_t) reclen) != reclen)
			      {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 4\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
				printf("\nError reading block %ld\n", i); 
#else
				printf("\nError reading block %lld\n", i); 
#endif
				perror("read");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(144);
			      }
			  }
		}
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			if(verify_buffer(buffer1,reclen,(off64_t)(current_position/reclen),reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(145);
			}
		   }
		   else
		   {
			if(verify_buffer(nbuff,reclen,(off64_t)(current_position/reclen),reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(146);
			}
		   }
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
		current_position+=reclen;
		if(async_flag && no_copy_flag)
			async_release(gc);
		t_offset = (off64_t)reclen*2;
		if (!(h_flag || k_flag || mmapflag))
		{
		  I_LSEEK( fd, -t_offset, SEEK_CUR );
		}
		current_position-=(2 *reclen);
		reverse_read +=reclen/1024;
		if(*stop_flag)
		{
			reverse_read -=reclen/1024;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_revqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_revqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }
	}
	if(file_lock)
		if(mylockf((int) fd,(int)0, (int)1))
			printf("Read unlock failed %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}
		close(fd);
	}
	temp_time = time_so_far();
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime2)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime2)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
	if(OPS_flag){
	   reverse_read=(reverse_read*1024)/reclen;
	}
	child_stat->throughput = reverse_read/child_stat->throughput;
	child_stat->actual = reverse_read;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
                tell_master_stats(THREAD_REVERSE_READ_TEST, chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
			(long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD;	/* Tell parent I'm done */
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
		close(fd);
	}
	free(dummyfile[xx]);
        if(Q_flag && (thread_revqfd !=0) )
                fclose(thread_revqfd);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Reverse read finished: ",
			now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Read Backwards",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}
/************************************************************************/
/* Thread_stride_read_test				        	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_stride_read_test(void *x)
#else
void *
thread_stride_read_test(x)
#endif
{
	long long xx,xx2;
	char *nbuff=0;
	struct child_stats *child_stat;
	double walltime, cputime;
	int fd;
	long long flags = 0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double starttime2 = 0;
	float delay = 0;
	double compute_val = (double)0;
	double temp_time;
	long long recs_per_buffer;
	off64_t i;
	off64_t lock_offset=0;
	off64_t savepos64=0;
	off64_t written_so_far, stride_read,re_read_so_far,read_so_far;
	off64_t stripewrap = 0;
	off64_t current_position = 0;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *maddr=0;
	char *wmaddr=0;
	volatile char *buffer1;
	int anwser,bind_cpu;
	off64_t traj_offset;
	char tmpname[256];
	char now_string[30];
	FILE *thread_strqfd=0;
	FILE *thread_Lwqfd=0;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif
	/*****************/
	/* Children only */
	/*****************/
	if(compute_flag)
		delay=compute_time;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	written_so_far=read_so_far=stride_read=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
		flags=O_RDONLY|O_SYNC;
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif
#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif

#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
#endif
	if((fd = I_OPEN(dummyfile[xx], ((int)flags),0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(147);
	}
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif

	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),0,PROT_READ);
	}
	if(fetchon)
		fetchit(nbuff,reclen);
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_strol.dat",(int)xx);
                thread_strqfd=fopen(tmpname,"a");
                if(thread_strqfd==0)
                {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
		fprintf(thread_strqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Stride test start: ",
			now_string);
	}
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag = CHILD_STATE_READY;
        if(distributed && client_iozone)
        {
                tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else

		/* wait for parent to say go */
                while(child_stat->flag!=CHILD_STATE_BEGIN)      
                        Poll((long long)1);
	if(file_lock)
		if(mylockf((int) fd, (int) 1,  (int)1)!=0)
			printf("File lock for write failed. %d\n",errno);
	starttime2 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime2;
		cputime = cputime_so_far();
	}
	for(i=0; i<numrecs64; i++){
		if(disrupt_flag && ((i%DISRUPT)==0))
		{
			disrupt(fd);
		}
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(Q_flag)
		{
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 3\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
                if(Q_flag || hist_summary || op_rate_flag)
                {
                        thread_qtime_start=time_so_far();
                }
		if(verify)
			savepos64=current_position/(off64_t)reclen;
		if(mmapflag)
		{
			wmaddr = &maddr[current_position];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			if(async_flag)
			{
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (current_position),
			      	reclen, stride,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (current_position),reclen,
			    	stride,(numrecs64*reclen),depth);
			}
			else
			{
			  if(read((int)fd, (void*)nbuff, (size_t) reclen) != reclen)
			  {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 4\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
		    		printf("\nError reading block %ld, fd= %d\n", i, fd);
#else
		    		printf("\nError reading block %lld, fd= %d\n", i, fd);
#endif
				perror("read");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
		    		exit(149);
			  }
			}
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
		current_position+=reclen;
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			if(verify_buffer(buffer1,reclen,(off64_t)savepos64,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(150);
			}
		   }
		   else
		   {
			if(verify_buffer(nbuff,reclen,(off64_t)savepos64,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(151);
			}
		   }
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
		if(current_position + (stride * reclen) >= (numrecs64 * reclen)-reclen) 
		{
			current_position=0;

			stripewrap++;
			  
			if(numrecs64 <= stride)
			{
				current_position=0;
			}
			else
			{
				current_position = (off64_t)((stripewrap)%numrecs64)*reclen;
			}
			if (!(h_flag || k_flag || mmapflag))
			{
			  if(I_LSEEK(fd,current_position,SEEK_SET)<0)
			  {
				client_error=errno;
				if(distributed && client_iozone)
					send_stop();
				perror("lseek");
				exit(152);
			  }
			}
		}
		else			
		{
			current_position+=(stride*reclen)-reclen;
			if (!(h_flag || k_flag || mmapflag))
			{
			  if(I_LSEEK(fd,current_position,SEEK_SET)<0)
			  {
				client_error=errno;
				if(distributed && client_iozone)
					send_stop();
				perror("lseek");
				exit(154);
			  };
			}
		}
		stride_read +=reclen/1024;
		if(*stop_flag)
		{
			stride_read -=reclen/1024;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_strqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_strqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }
	}
	if(file_lock)
		if(mylockf((int) fd,(int)0,(int)1))
			printf("Read unlock failed %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}
		close(fd);
	}
	temp_time = time_so_far();
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime2)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime2)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
	if(OPS_flag){
	   stride_read=(stride_read*1024)/reclen;
	}
	child_stat->throughput = stride_read/child_stat->throughput;
	child_stat->actual = stride_read;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
        {
                tell_master_stats(THREAD_STRIDE_TEST,chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
        }
	child_stat->flag = CHILD_STATE_HOLD;	/* Tell parent I'm done */
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
		close(fd);
	}
        if(Q_flag && (thread_strqfd !=0) )
                fclose(thread_strqfd);
	free(dummyfile[xx]);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Stride test finished: ",
			now_string);
		fclose(thread_Lwqfd);
	}
	if(hist_summary)
	   dump_hist("Stride Read",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

/************************************************************************/
/* Thread random test				        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_mix_test(void *x)
#else
void *
thread_mix_test(x)
#endif
{
	int selector;
	int num_readers;
	long xx;

	mix_test = 1;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
	{
		xx = (long)x;
	}
	else
	{
		xx=(long)chid;
	}
#endif
	if(pct_read!=0)
	{
 		num_readers = (pct_read * num_child)/100;
                if(xx < num_readers)
                        selector=0;
                else
                        selector=1;
	}
	else
	{
	   if(Kplus_flag)
	   {
		if(xx+1 <= Kplus_readers)
			selector=0;
		else
			selector=1;
	   }
	   else
	   {
		/* Simple round robin */
		selector= ((int)xx) % 2;
	   }		
	}
	if(selector==0)
	{
		if(seq_mix)
			thread_read_test(x);
		else
			thread_ranread_test(x);
	}
	else
	{
		if(seq_mix)
			thread_write_test(x);
		else
			thread_ranwrite_test(x);
	}
	return(0);
}
/************************************************************************/
/* Thread random read test				        	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_ranread_test(void *x)
#else
void *
thread_ranread_test(x)
#endif
{
	long long xx,xx2;
	struct child_stats *child_stat;
	double walltime, cputime;
	int fd;
	long long flags = 0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	double starttime1 = 0;
	float delay = 0;
	double temp_time;
	double compute_val = (double)0;
	off64_t written_so_far, ranread_so_far, re_written_so_far,re_read_so_far;
	long long recs_per_buffer;
	off64_t current_offset=0;
	off64_t i;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	char *nbuff=0;
	char *maddr=0;
	char *wmaddr=0;
	volatile char *buffer1;
	int anwser,bind_cpu;
	off64_t traj_offset;
	off64_t lock_offset=0;
	char tmpname[256];
	char now_string[30];
	FILE *thread_randrfd=0;
	FILE *thread_Lwqfd=0;
	long long *recnum=0;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
	long long save_pos;
#if defined (bsd4_2) || defined(Windows)
	long long rand1,rand2,rand3;
#endif
	unsigned long long big_rand;
#ifdef ASYNC_IO
	struct cache *gc=0;
#else
	long long *gc=0;
#endif
#ifdef MERSENNE
        unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
#endif

#ifdef MERSENNE
        init_by_array64(init, length);
#else
#ifdef bsd4_2
        srand(0);
#else
#ifdef Windows
        srand(0);
#else
        srand48(0);
#endif
#endif
#endif
        recnum = (long long *)malloc(sizeof(*recnum)*numrecs64);
        if (recnum){
             /* pre-compute random sequence based on 
		Fischer-Yates (Knuth) card shuffle */
            for(i = 0; i < numrecs64; i++){
                recnum[i] = i;
            }
            for(i = 0; i < numrecs64; i++) {
                long long tmp = recnum[i];
#ifdef MERSENNE
      	       big_rand = genrand64_int64();
#else
#ifdef bsd4_2
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
#ifdef Windows
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
               big_rand = lrand48();
#endif
#endif
#endif
               big_rand = big_rand%numrecs64;
               tmp = recnum[i];
               recnum[i] = recnum[big_rand];
               recnum[big_rand] = tmp;
            }
        }
	else
	{
		fprintf(stderr,"Random uniqueness fallback.\n");
	}
	if(compute_flag)
		delay=compute_time;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	written_so_far=ranread_so_far=re_written_so_far=re_read_so_far=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	if(oflag)
	{
		flags=O_RDONLY|O_SYNC;
	}
	else
		flags=O_RDONLY;
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif

#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
#endif
	if((fd = I_OPEN(dummyfile[xx], ((int)flags),0))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		perror(dummyfile[xx]);
		exit(156);
	}
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif

#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif

	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(numrecs64*reclen),0,PROT_READ);
	}
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->throughput = 0;
	child_stat->actual = 0;
	if(debug1)
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	/*****************/
	/* Children only */
	/*****************/
	if(fetchon)
		fetchit(nbuff,reclen);
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_randrol.dat",(int)xx);
                thread_randrfd=fopen(tmpname,"a");
                if(thread_randrfd==0)
                {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
		fprintf(thread_randrfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Random read start: ",
			now_string);
	}
	child_stat=(struct child_stats *)&shmaddr[xx];
	child_stat->flag = CHILD_STATE_READY;
        if(distributed && client_iozone)
        {
                tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else
        {
                while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
                        Poll((long long)1);
        }
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}

#ifdef MERSENNE
        init_by_array64(init, length);
#else
#ifdef bsd4_2
        srand(0);
#else
#ifdef Windows
        srand(0);
#else
	srand48(0);
#endif
#endif
#endif
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)1)!=0)
			printf("File lock for read failed. %d\n",errno);
	for(i=0; i<numrecs64; i++) {
		if(compute_flag)
			compute_val+=do_compute(delay);
		if(*stop_flag)
		{
			if(debug1)
				printf("\n(%ld) Stopped by another 2\n", (long)xx);
			break;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if (recnum) {
			current_offset = reclen * (long long)recnum[i];
                } else {
#ifdef MERSENNE
                   big_rand = genrand64_int64();
		   current_offset = (off64_t)reclen * (big_rand%numrecs64);
#else
#ifdef bsd4_2
		   rand1=(long long)rand();
		   rand2=(long long)rand();
		   rand3=(long long)rand();
		   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		   current_offset = (off64_t)reclen * (big_rand%numrecs64);
#else
#ifdef Windows
		   rand1=(long long)rand();
		   rand2=(long long)rand();
		   rand3=(long long)rand();
		   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		   current_offset = (off64_t)reclen * (big_rand%numrecs64);
#else
		   current_offset = reclen * (lrand48()%numrecs64);
#endif
#endif
#endif
		}

		if (!(h_flag || k_flag || mmapflag))
		{
		  if(I_LSEEK( fd, current_offset, SEEK_SET )<0)
		  {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			perror("lseek");
			exit(158);
		  };
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)1,
			  lock_offset, reclen);
		}
		if(Q_flag || hist_summary || op_rate_flag)
		{
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
                        thread_qtime_start=time_so_far();
		}
		if(mmapflag)
		{
			wmaddr = &maddr[current_offset];
			fill_area((long long*)wmaddr,(long long*)nbuff,(long long)reclen);
		}
		else
		{
			if(async_flag)
			{
			    if(no_copy_flag)
			      async_read_no_copy(gc, (long long)fd, &buffer1, (current_offset),
			      	 reclen, 0LL,(numrecs64*reclen),depth);
			    else
			      async_read(gc, (long long)fd, nbuff, (current_offset), reclen,
			    	0LL,(numrecs64*reclen),depth);
			}
			else
			{
	  		  if(read((int)fd, (void*)nbuff, (size_t)reclen) != reclen)
	  		  {
				if(*stop_flag)
				{
					if(debug1)
						printf("\n(%ld) Stopped by another 2\n", (long)xx);
					break;
				}
#ifdef NO_PRINT_LLD
				printf("\nError reading block at %ld\n",
					 offset); 
#else
				printf("\nError reading block at %lld\n",
					 offset); 
#endif
				perror("ranread");
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(160);
	 		  }
			}
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)1,
			  lock_offset, reclen);
		}
		save_pos=current_offset/reclen;
		current_offset+=reclen;
		if(verify){
		   if(async_flag && no_copy_flag)
		   {
			if(verify_buffer(buffer1,reclen,(off64_t)save_pos,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(161);
			}
		   }
		   else
		   {
			if(verify_buffer(nbuff,reclen,(off64_t)save_pos,reclen,(long long)pattern,sverify)){
				if (!no_unlink)
				{
				   if(check_filename(dummyfile[xx]))
					unlink(dummyfile[xx]);
				}
				child_stat->flag = CHILD_STATE_HOLD;
				exit(162);
			}
		   }
		}
		if(async_flag && no_copy_flag)
			async_release(gc);
		ranread_so_far+=reclen/1024;
		if(*stop_flag)
		{
			ranread_so_far-=reclen/1024;
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
                if(Q_flag)
                {
                        thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
                        fprintf(thread_randrfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
                        fprintf(thread_randrfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
                }
	}
	if(file_lock)
		if(mylockf((int) fd,(int)0,(int)1))
			printf("Read unlock failed %d\n",errno);
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(include_flush)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
		}else
			fsync(fd);
	}
	if(include_close)
	{	
		if(mmapflag)
		{
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}
		close(fd);
	}
	temp_time = time_so_far();
	child_stat=(struct child_stats *)&shmaddr[xx];
	if(inc_think)
		child_stat->throughput = ((temp_time - starttime1)-time_res);
	else
		child_stat->throughput = ((temp_time - starttime1)-time_res)
			-compute_val;
	if(child_stat->throughput < (double).000001) 
	{
		child_stat->throughput= time_res;
		if(rec_prob < reclen)
			rec_prob = reclen;
		res_prob=1;
	}
	if(OPS_flag){
	   ranread_so_far=(ranread_so_far*1024)/reclen;
	}
	child_stat->throughput = ranread_so_far/child_stat->throughput;
	child_stat->actual = ranread_so_far;
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
        if(cdebug)
	{
                fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid,child_stat->throughput,
                        child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
                tell_master_stats(THREAD_RANDOM_READ_TEST, chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD; 	/* Tell parent I'm done */
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)(numrecs64*reclen),MS_SYNC);
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
		close(fd);
	}
        if(Q_flag && (thread_randrfd !=0) )
                fclose(thread_randrfd);
	free(dummyfile[xx]);
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Random read finished: ",now_string);
		fclose(thread_Lwqfd);
	}
	if(recnum)
		free(recnum);
	if(hist_summary)
	   dump_hist("Random Read",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

/************************************************************************/
/* Thread random write test			        		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_ranwrite_test(void *x)
#else
void *
thread_ranwrite_test( x)
#endif
{

	struct child_stats *child_stat;
	double starttime1 = 0;
	double temp_time;
	double walltime, cputime;
	double compute_val = (double)0;
	float delay = (double)0;
	double thread_qtime_stop,thread_qtime_start;
	double hist_time;
	double desired_op_rate_time;
	double actual_rate;
	off64_t traj_offset;
	off64_t current_offset=0;
	long long flags;
	long long w_traj_bytes_completed;
	long long w_traj_ops_completed;
	int fd;
	long long recs_per_buffer;
	long long stopped,i;
	off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
	long long xx,xx2;
	char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
	char *nbuff=0;
	char *maddr=0;
	char *wmaddr=0;
	char *free_addr=0;
	int anwser,bind_cpu,wval;
	off64_t filebytes64;
	off64_t lock_offset=0;
	char tmpname[256];
	char now_string[30];
	FILE *thread_randwqfd=0;
	FILE *thread_Lwqfd=0;
	long long *recnum = 0;
#if defined(VXFS) || defined(solaris)
	int test_foo = 0;
#endif
#if defined (bsd4_2) || defined(Windows)
	long long rand1,rand2,rand3;
#endif
	unsigned long long big_rand;

#ifdef ASYNC_IO
	struct cache *gc=0;

#else
	long long *gc=0;
#endif
#ifdef MERSENNE
        unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
#endif

	if(compute_flag)
		delay=compute_time;
	hist_time=thread_qtime_stop=thread_qtime_start=0;
	traj_offset=walltime=cputime=0;
	anwser=bind_cpu=0;
	filebytes64 = numrecs64*reclen;
	written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
	w_traj_bytes_completed=w_traj_ops_completed=0;
	recs_per_buffer = cache_size/reclen ;
#ifdef MERSENNE
        init_by_array64(init, length);
#else
#ifdef bsd4_2
        srand(0);
#else
#ifdef Windows
        srand(0);
#else
        srand48(0);
#endif
#endif
#endif
        recnum = (long long *) malloc(sizeof(*recnum)*numrecs64);
        if (recnum){
             /* pre-compute random sequence based on 
		Fischer-Yates (Knuth) card shuffle */
            for(i = 0; i < numrecs64; i++){
                recnum[i] = i;
            }
            for(i = 0; i < numrecs64; i++) {
                long long tmp = recnum[i];
#ifdef MERSENNE
      	       big_rand = genrand64_int64();
#else
#ifdef bsd4_2
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
#ifdef Windows
               rand1=(long long)rand();
               rand2=(long long)rand();
               rand3=(long long)rand();
               big_rand=(rand1<<32)|(rand2<<16)|(rand3);
#else
               big_rand = lrand48();
#endif
#endif
#endif
               big_rand = big_rand%numrecs64;
               tmp = recnum[i];
               recnum[i] = recnum[big_rand];
               recnum[big_rand] = tmp;
            }
        }
	else
	{
		fprintf(stderr,"Random uniqueness fallback.\n");
	}
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
	{
		xx = (long long)((long)x);
	}
	else
	{
		xx=chid;
	}
#endif
#ifndef NO_THREADS
#if defined( _HPUX_SOURCE ) || defined ( linux )
	if(ioz_processor_bind)
	{
		 bind_cpu=(begin_proc+(int)xx)%num_processors;
#if defined(_HPUX_SOURCE)
		 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
#else
     cpu_set_t cpuset;

     CPU_ZERO(&cpuset);
     CPU_SET(bind_cpu, &cpuset);

     pthread_setaffinity_np(pthread_self(), sizeof(cpuset),&cpuset);
#endif
		my_nap(40);	/* Switch to new cpu */
	}
#endif
#endif
	if(use_thread)
		nbuff=barray[xx];
	else
		nbuff=buffer;
	if(debug1 )
	{
	   if(use_thread)
#ifdef NO_PRINT_LLD
		printf("\nStarting child %ld\n",xx);
#else
		printf("\nStarting child %lld\n",xx);
#endif
	   else
#ifdef NO_PRINT_LLD
		printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
		printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif
		
	}
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	xx2=xx;
	if(share_file)
		xx2=(long long)0;
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
	}
	/*****************/
	/* Children only */
	/*******************************************************************/
	/* Random write throughput performance test. **********************/
	/*******************************************************************/
	if(oflag)
		flags=O_RDWR|O_SYNC|O_CREAT;
	else
		flags=O_RDWR|O_CREAT;
#if defined(O_DSYNC)
	if(odsync)
		flags |= O_DSYNC;
#endif
#if defined(_HPUX_SOURCE) || defined(linux)
	if(read_sync)
		flags |=O_RSYNC|O_SYNC;
#endif

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
	if(direct_flag)
		flags |=O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		flags |=O_DIRECTIO;
#endif
#endif
#if defined(Windows)
       	if(unbuffered)
        {
        	hand=CreateFile(dummyfile[xx],
		  GENERIC_READ|GENERIC_WRITE,
	          FILE_SHARE_WRITE|FILE_SHARE_READ,
		  NULL,OPEN_EXISTING,FILE_FLAG_NO_BUFFERING|
		  FILE_FLAG_WRITE_THROUGH|FILE_FLAG_POSIX_SEMANTICS,
		  NULL);
       	}
#endif
	if((fd = I_OPEN(dummyfile[xx], ((int)flags),0640))<0)
	{
		client_error=errno;
		if(distributed && client_iozone)
			send_stop();
		printf("\nCan not open temp file: %s\n", 
			filename);
		perror("open");
		exit(125);
	}
#ifdef VXFS
	if(direct_flag)
	{
		ioctl(fd,VX_SETCACHE,VX_DIRECT);
		ioctl(fd,VX_GETCACHE,&test_foo);
		if(test_foo == 0)
		{
			if(!client_iozone)
			  printf("\nVxFS advanced setcache feature not available.\n");
			exit(3);
		}
	}
#endif
#if defined(solaris)
        if(direct_flag)
        {
                test_foo = directio(fd, DIRECTIO_ON);
                if(test_foo != 0)
                {
                        if(!client_iozone)
                          printf("\ndirectio not available.\n");
                        exit(3);
                }
        }
#endif
#ifdef ASYNC_IO
	if(async_flag)
		async_init(&gc,fd,direct_flag);
#endif
	if(mmapflag)
	{
		maddr=(char *)initfile(fd,(filebytes64),1,PROT_READ|PROT_WRITE);
	}
	if(reclen < cache_size )
	{
		recs_per_buffer = cache_size/reclen ;
		nbuff=&nbuff[(xx%recs_per_buffer)*reclen];
	}
	if(fetchon)			/* Prefetch into processor cache */
		fetchit(nbuff,reclen);

	child_stat = (struct child_stats *)&shmaddr[xx];	
	child_stat->throughput = 0;
	child_stat->actual = 0;
	child_stat->flag=CHILD_STATE_READY; /* Tell parent child is ready to go */
	if(distributed && client_iozone)
	{
		tell_master_ready(chid);
		wait_for_master_go(chid);
	}
	else
	{
		while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
			Poll((long long)1);
	}
	written_so_far=0;
	child_stat = (struct child_stats *)&shmaddr[xx];
	child_stat->actual = 0;
	child_stat->throughput = 0;
	stopped=0;
	if(file_lock)
		if(mylockf((int) fd, (int) 1, (int)0) != 0)
			printf("File lock for write failed. %d\n",errno);
	if(Q_flag)
	{
		sprintf(tmpname,"Child_%d_randwol.dat",(int)xx);
		thread_randwqfd=fopen(tmpname,"a");
		if(thread_randwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		fprintf(thread_randwqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
	}
	if(L_flag)
	{
		sprintf(tmpname,"Child_%d.log",(int)xx);
		thread_Lwqfd=fopen(tmpname,"a");
		if(thread_Lwqfd==0)
		{
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			printf("Unable to open %s\n",tmpname);
			exit(40);
		}
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Random write start: ",
			now_string);
	}
	if((verify && !no_copy_flag) || dedup || dedup_interior)
		fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)0);
	starttime1 = time_so_far();
	if(cpuutilflag)
	{
		walltime = starttime1;
		cputime = cputime_so_far();
	}
	for(i=0; i<numrecs64; i++){
		if(compute_flag)
			compute_val+=do_compute(delay);
		if (recnum) {
			current_offset = reclen * (long long)recnum[i];
                } else {
#ifdef bsd4_2
		   rand1=rand();
		   rand2=rand();
		   rand3=rand();
		   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		   current_offset = (off64_t)reclen * (big_rand%numrecs64);
#else
#ifdef Windows
		   rand1=rand();
		   rand2=rand();
		   rand3=rand();
		   big_rand=(rand1<<32)|(rand2<<16)|(rand3);
		   current_offset = (off64_t)reclen * (big_rand%numrecs64);
#else
		   current_offset = reclen * (lrand48()%numrecs64);
#endif
#endif
		}

		if (!(h_flag || k_flag || mmapflag))
		{
		  if(I_LSEEK( fd, current_offset, SEEK_SET )<0)
		  {
			client_error=errno;
			if(distributed && client_iozone)
				send_stop();
			perror("lseek");
			exit(158);
		  };
		}
		if(Q_flag || hist_summary || op_rate_flag)
		{
			traj_offset=I_LSEEK(fd,0,SEEK_CUR);
                        thread_qtime_start=time_so_far();
		}
		if(rlocking)
		{
			lock_offset=I_LSEEK(fd,0,SEEK_CUR);
			mylockr((int) fd, (int) 1, (int)0,
			  lock_offset, reclen);
		}
		if((verify && !no_copy_flag) || dedup || dedup_interior)
			fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)(current_offset/reclen));
		if(*stop_flag && !stopped){
			if(include_flush)
			{
				if(mmapflag)
					msync(maddr,(size_t)filebytes64,MS_SYNC);
				else
					fsync(fd);
			}
			child_stat->throughput = 
				(time_so_far() - starttime1)-time_res;
			if(child_stat->throughput < (double).000001) 
			{
				child_stat->throughput = time_res;
				if(rec_prob < reclen)
					rec_prob = reclen;
				res_prob=1;
			}

			if(OPS_flag){
			   /*written_so_far=(written_so_far*1024)/reclen;*/
			   written_so_far=w_traj_ops_completed;
			}
			child_stat->throughput = 
			  (double)written_so_far/child_stat->throughput;
			child_stat->actual = (double)written_so_far;
			if(debug1)
			{
				printf("\n(%ld) Stopped by another\n", (long)xx);
			}
			stopped=1;
		}
		if(purge)
			purgeit(nbuff,reclen);
		if(Q_flag || hist_summary)
		{
			thread_qtime_start=time_so_far();
		}
again:		
		if(mmapflag)
		{
			wmaddr = &maddr[current_offset];
			fill_area((long long*)nbuff,(long long*)wmaddr,(long long)reclen);
			if(!mmapnsflag)
			{
			  if(mmapasflag)
			    msync(wmaddr,(size_t)reclen,MS_ASYNC);
			  if(mmapssflag)
			    msync(wmaddr,(size_t)reclen,MS_SYNC);
			}
		}
		else
		{
		   if(async_flag)
		   {
			     if(no_copy_flag)
			     {
				free_addr=nbuff=(char *)malloc((size_t)reclen+page_size);
				nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));
				if(verify || dedup || dedup_interior)
					fill_buffer(nbuff,reclen,(long long)pattern,sverify,(long long)(current_offset/reclen));
			        async_write_no_copy(gc, (long long)fd, nbuff, reclen, (current_offset), depth,free_addr);
			     }
			     else
				async_write(gc, (long long)fd, nbuff, reclen, current_offset, depth);
		   }
		   else
		   {
		      wval = write(fd, nbuff, (size_t) reclen);
		      if(wval != reclen)
		      {
			if(*stop_flag && !stopped){
				if(include_flush)
				{
					if(mmapflag)
						msync(maddr,(size_t)filebytes64,MS_SYNC);
					else
						fsync(fd);
				}
				temp_time = time_so_far();
				child_stat->throughput = 
					(temp_time - starttime1)-time_res;
				if(child_stat->throughput < (double).000001) 
				{
					child_stat->throughput= time_res;
					if(rec_prob < reclen)
						rec_prob = reclen;
					res_prob=1;
				}

				if(OPS_flag){
				   /*written_so_far=(written_so_far*1024)/reclen;*/
				   written_so_far=w_traj_ops_completed;
				}
				child_stat->throughput = 
				  (double)written_so_far/child_stat->throughput;
				child_stat->actual = (double)written_so_far;
				if(debug1)
				{
					printf("\n(%ld) Stopped by another\n", (long)xx);
				}
				stopped=1;
				goto again;
			}
			/* Note: Writer must finish even though told
			   to stop. Otherwise the readers will fail.
			   The code will capture bytes transfered
			   before told to stop but let the writer
			   complete.
			*/
#ifdef NO_PRINT_LLD
		    	printf("\nError writing block %ld, fd= %d\n", i,
				 fd);
#else
		    	printf("\nError writing block %lld, fd= %d\n", i,
				 fd);
#endif
			if(wval==-1)
				perror("write");
			if (!no_unlink)
			{
		            if(check_filename(dummyfile[xx]))
				unlink(dummyfile[xx]);
			}
			child_stat->flag = CHILD_STATE_HOLD;
		    	exit(127);
		      }
		    }
		}
		if(rlocking)
		{
			mylockr((int) fd, (int) 0, (int)0,
			  lock_offset, reclen);
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start-time_res);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
/*
printf("Desired rate %g  Actual rate %g Nap %g microseconds\n",desired_op_rate_time,
	actual_rate, (desired_op_rate_time-actual_rate));
*/
			if( actual_rate < desired_op_rate_time)
				my_unap((unsigned long long) ((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_randwqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_randwqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
		w_traj_ops_completed++;
		w_traj_bytes_completed+=reclen;
		written_so_far+=reclen/1024;
		if(*stop_flag)
		{
			written_so_far-=reclen/1024;
			w_traj_bytes_completed-=reclen;
		}
	}
	

	if(file_lock)
		if(mylockf((int) fd, (int) 0, (int)0))
			printf("Write unlock failed. %d\n",errno);
	
#ifdef ASYNC_IO
	if(async_flag)
	{
		end_async(gc);
		gc=0;
	}
#endif
	if(!xflag)
	{
		*stop_flag=1;
		if(distributed && client_iozone)
			send_stop();
	}
	
	if(include_flush)
	{
		if(mmapflag)
			msync(maddr,(size_t)filebytes64,MS_SYNC);
		else
			fsync(fd);
	}
	if(include_close)
	{
		if(mmapflag)
			mmap_end(maddr,(unsigned long long)filebytes64);
		close(fd);
	}
	if(!stopped){
		temp_time = time_so_far();
		if(inc_think)
			child_stat->throughput = ((temp_time - starttime1)-time_res);
		else
			child_stat->throughput = ((temp_time - starttime1)-time_res)
				-compute_val;
		if(child_stat->throughput < (double).000001) 
		{
			child_stat->throughput= time_res;
			if(rec_prob < reclen)
				rec_prob = reclen;
			res_prob=1;
		}

		if(OPS_flag){
		   /*written_so_far=(written_so_far*1024)/reclen;*/
		   written_so_far=w_traj_ops_completed;
		}
		child_stat->throughput =
			(double)written_so_far/child_stat->throughput;
		child_stat->actual = (double)written_so_far;
	}
	child_stat->flag = CHILD_STATE_HOLD; /* Tell parent I'm done */
        if(cdebug)
	{
                fprintf(newstdout,"Child %d: throughput %f actual %f \n",(int)chid,child_stat->throughput,
                        child_stat->actual);
		fflush(newstdout);
	}
	if(cpuutilflag)
	{
		cputime = cputime_so_far() - cputime;
		if (cputime < cputime_res)
			cputime = 0.0;
		child_stat->cputime = cputime;
		walltime = time_so_far() - walltime;
		child_stat->walltime = walltime;
	}
        if(distributed && client_iozone)
                tell_master_stats(THREAD_RANDOM_WRITE_TEST, chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
	stopped=0;
	/*******************************************************************/
	/* End random write performance test. ******************************/
	/*******************************************************************/
	if(debug1)
#ifdef NO_PRINT_LLD
		printf("\nChild finished %ld\n",xx);
#else
		printf("\nChild finished %lld\n",xx);
#endif
	if(!include_close)
	{
		if(mmapflag)
		{
			msync(maddr,(size_t)numrecs64*reclen,MS_SYNC); /*Clean up before read starts running*/ 
			mmap_end(maddr,(unsigned long long)numrecs64*reclen);
		}else
			fsync(fd);
			
		close(fd);
	}
	if(Q_flag && (thread_randwqfd !=0) )
		fclose(thread_randwqfd);
	free(dummyfile[xx]);

	if(L_flag)
	{
		get_date(now_string);
		fprintf(thread_Lwqfd,"%-25s %s","Random write finished: ",
			now_string);
		fclose(thread_Lwqfd);
	}
	if(recnum)
		free(recnum);
	if(hist_summary)
	   dump_hist("Random Write",(int)xx);
	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}

/************************************************************************/
/* Thread cleanup test				        		*/
/* This is not a measurement. It is a mechanism to cleanup all of the   */
/* temporary files that were being used. This becomes very important    */
/* when testing multiple clients over a network :-)                     */
/************************************************************************/
#ifdef HAVE_ANSIC_C
void *
thread_cleanup_test(void *x)
#else
void *
thread_cleanup_test(x)
#endif
{
	long long xx;
	struct child_stats *child_stat;
	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */

	
#ifdef NO_THREADS
	xx=chid;
#else
	if(use_thread)
		xx = (long long)((long)x);
	else
	{
		xx=chid;
	}
#endif
	dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
	if(mfflag)
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s",filearray[xx]);
#else
		sprintf(dummyfile[xx],"%s",filearray[xx]);
#endif
	}
	else
	{
#ifdef NO_PRINT_LLD
		sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx],xx);
#else
		sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx],xx);
#endif
	}
	if(!no_unlink)
	{
	    if(check_filename(dummyfile[xx]))
		unlink(dummyfile[xx]);
	}

	child_stat = (struct child_stats *)&shmaddr[xx];
	/*****************/
	/* Children only */
	/*****************/
	child_stat=(struct child_stats *)&shmaddr[xx];
	child_stat->flag = CHILD_STATE_READY;
	if(distributed && client_iozone)
        {
		tell_master_ready(chid);
                wait_for_master_go(chid);
        }
        else
        {
                while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
                        Poll((long long)1);
        }

	*stop_flag=1;
	if(distributed && client_iozone)
		send_stop();
        if(distributed && client_iozone)
                tell_master_stats(THREAD_CLEANUP_TEST, chid, child_stat->throughput,
                        child_stat->actual,
			child_stat->cputime, child_stat->walltime,
			(char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);
	child_stat->flag = CHILD_STATE_HOLD; 	/* Tell parent I'm done */
	free(dummyfile[xx]);

	if(distributed && client_iozone)
		return(0);
#ifdef NO_THREADS
	exit(0);
#else
	if(use_thread)
		thread_exit();
	else
		exit(0);
#endif
return(0);
}


/************************************************************************/
/* mythread_create() Internal routine that calls pthread_create()	*/
/************************************************************************/
#ifndef NO_THREADS
#ifdef HAVE_ANSIC_C
long long 
mythread_create( void *(*func)(void *),void *x)
#else
long long 
mythread_create( func,x)
void *(*func)(void *);
void *x;
#endif
{
	pthread_t ts;
	pthread_attr_t attr;
	int xx;
	int *yy;
#ifdef _64BIT_ARCH_
	long long meme;
	meme = (long long)x;
#else
	long meme;
	meme = (long)x;
#endif
	yy=(int *)x;


#ifdef OSFV3
	
	xx=(int )pthread_create(&ts, pthread_attr_default,
		func, (void *)yy);

#else
	pthread_attr_init(&attr);
	xx=(int )pthread_create((pthread_t *)&ts, (pthread_attr_t *) &attr,
		func, (void *)yy);
#endif
	bcopy(&ts,&p_childids[meme],sizeof(pthread_t));
	if(xx < (int)0)
		printf("Thread create failed. Returned %d Errno = %d\n",xx,errno);
	if(debug1 )
	{
		printf("\nthread created has an id of %lx\n",ts);
		printf("meme %ld\n",meme);
	}
	return((long long)meme);
}
#else
#ifdef HAVE_ANSIC_C
long long 
mythread_create( void *(*func)(void *),void *x)
#else
long long 
mythread_create( func,x)
void *(*func)(void *);
void *x;
#endif
{
	printf("This version does not support threads\n");
	return(-1);
}
#endif

/************************************************************************/
/* thread_exit() Internal routine that calls pthread_exit()		*/
/************************************************************************/
#ifndef NO_THREADS
#ifdef HAVE_ANSIC_C
int
thread_exit(void)
#else
int
thread_exit()
#endif
{
	pthread_exit((void *)NULL);
return(0);
}
#else
#ifdef HAVE_ANSIC_C
int
thread_exit(void)
#else
int
thread_exit()
#endif
{
	printf("This version does not support threads\n");
	return(-1);
}
#endif

/************************************************************************/
/* mythread_self() Internal function that calls pthread_self()		*/
/************************************************************************/
#ifndef NO_THREADS
#ifdef HAVE_ANSIC_C
pthread_t
mythread_self(void)
#else
pthread_t
mythread_self()
#endif
{
	pthread_t xx;
	xx = pthread_self();
	return(xx);
}
#else
#ifdef HAVE_ANSIC_C
int
mythread_self(void)
#else
int
mythread_self()
#endif
{
	printf("This version does not support threads\n");
	return(-1);
}
#endif

/************************************************************************/
/* Internal thread_join routine... calls pthread_join			*/
/************************************************************************/
#ifndef NO_THREADS
#ifdef HAVE_ANSIC_C
void *
thread_join( long long tid, void *status) 
#else
void *
thread_join( tid, status) 
long long tid;
void *status;
#endif
{
	int xx;
	pthread_t eek;
	pthread_attr_t foo;

	bcopy(&p_childids[tid],&eek,sizeof(pthread_t));
	xx=pthread_join(eek,(void **)&foo);
	if(xx<0)
		printf("Thread join returned error %d\n",errno);
	return(0);
}
#else
#ifdef HAVE_ANSIC_C
void *
thread_join( long long tid, void *status) 
#else
void *
thread_join( tid, status) 
long long tid;
void *status;
#endif
{
	printf("This version does not support threads\n");
	return((void *)-1);
}
#endif


/************************************************************************/
/* Dump the CPU utilization data.					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
dump_throughput_cpu(void)
#else
void
dump_throughput_cpu()
#endif
{
	long long x,y,i,j;
	char *port;
	char *label;
	char print_str[300];
	x=max_x;
	y=max_y;

	port = use_thread ? "threads" : "processes";
	printf("\n\"CPU utilization report Y-axis is type of test X-axis is number of %s\"\n",port);
	if (bif_flag)
	{
		sprintf(print_str, "CPU utilization report Y-axis is type of test X-axis is number of %s", port);
		do_label(bif_fd, print_str, bif_row++, bif_column);
	}
	label = OPS_flag ?  "ops/sec" :
		MS_flag ? "microseconds/op" : "kBytes/sec";
#ifdef NO_PRINT_LLD
	if(!silent) printf("\"Record size = %ld kBytes \"\n", reclen/1024);
#else
	if(!silent) printf("\"Record size = %lld kBytes \"\n", reclen/1024);
#endif
	if(!silent) printf("\"Output is in CPU%%\"\n\n");
	if (bif_flag)
	{
#ifdef NO_PRINT_LLD
		sprintf(print_str, "Record size = %ld kBytes", reclen/1024);
#else
		sprintf(print_str, "Record size = %lld kBytes", reclen/1024);
#endif
		do_label(bif_fd, print_str, bif_row++, bif_column);
		sprintf(print_str, "Output is in CPU%%");
		do_label(bif_fd, print_str, bif_row++, bif_column);
	}
	for (i = 0; i < x; i++)
	{
		if(!silent) printf("\"%15s \"", throughput_tests[i]);
		if (bif_flag)
		{
			sprintf(print_str, "%15s ", throughput_tests[i]);
			do_label(bif_fd, print_str, bif_row, bif_column++);
			bif_column++;
		}
		for (j = 0; j <= y; j++)
		{
			if (bif_flag)
				do_float(bif_fd, runtimes[i][j].cpuutil, bif_row, bif_column++);
			if(!silent) printf(" %10.2f ", runtimes[i][j].cpuutil);
		}
		if(!silent) printf("\n\n");
		if (bif_flag)
		{
			bif_column=0;
			bif_row++;
		}
	}
}


/************************************************************************/
/* Dump the throughput graphs 						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
dump_throughput(void)
#else
void
dump_throughput()
#endif
{
	long long x,y,i,j;
	char *port;
	char *label;
	char print_str[300];
	x=max_x;
	y=max_y;

	if(use_thread)
		port="threads";
	else
		port="processes";
	if(!silent) printf("\n\"Throughput report Y-axis is type of test X-axis is number of %s\"\n",port);
	if(bif_flag)
	{
		bif_fd=create_xls(bif_filename);
		do_label(bif_fd,command_line,bif_row++,bif_column);
		sprintf(print_str,"Throughput report Y-axis is type of test X-axis is number of %s",port);
		do_label(bif_fd,print_str,bif_row++,bif_column);
	}
	if(OPS_flag)
		label="ops/sec";
	else
	if(MS_flag)
		label="microseconds/op";
	else
		label="kBytes/sec";
#ifdef NO_PRINT_LLD
	if(!silent) printf("\"Record size = %ld kBytes \"\n",reclen/1024);
#else
	if(!silent) printf("\"Record size = %lld kBytes \"\n",reclen/1024);
#endif
	if(!silent) printf("\"Output is in %s\"\n\n",label);
	if(bif_flag)
	{
#ifdef NO_PRINT_LLD
		sprintf(print_str,"Record size = %ld kBytes",reclen/1024);
#else
		sprintf(print_str,"Record size = %lld kBytes",reclen/1024);
#endif
		do_label(bif_fd,print_str,bif_row++,bif_column);
		sprintf(print_str,"Output is in %s",label);
		do_label(bif_fd,print_str,bif_row++,bif_column);
	}
	for(i=0;i<=toutputindex;i++)
	{
		   if(!silent) printf("\"%15s \"",toutput[i]);
		   if(bif_flag)
		   {
			   sprintf(print_str,"%15s ",toutput[i]);
			   do_label(bif_fd,print_str,bif_row,bif_column++);
			   bif_column++;
		   }
		   for(j=0;j<=y;j++)
		   {
			   if(bif_flag)
			   {
				   do_float(bif_fd,(double)report_darray[i][j],bif_row,bif_column++);
			   }
			   if(!silent) printf(" %10.2f ",report_darray[i][j]);
		   }
		   if(!silent) printf("\n\n");
		   if(bif_flag)
		   {
			   bif_column=0;
			   bif_row++;
		   }
	}
	if (cpuutilflag)
		dump_throughput_cpu();
	if(bif_flag)
		close_xls(bif_fd);
}

/************************************************************************/
/* store_dvalue()							*/
/* Stores a value in an in memory array. Used by the report function    */
/* to re-organize the output for Excel					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void 
store_dvalue(double value)
#else
void
store_dvalue(value)
double value;
#endif
{
	report_darray[current_x][current_y]=value;
	current_x++;
	if(current_x > max_x)
		max_x=current_x;
	if(current_y > max_y)
		max_y=current_y;
	if(max_x >= MAX_X)
	{
		printf("\nMAX_X too small\n");
		exit(163);
	}
	if(max_y >= MAXSTREAMS)
	{
		printf("\nMAXSTREAMS too small\n");
		exit(164);
	}
}

/************************************************************************/
/* Initialize a file that will be used by mmap.				*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
char *
initfile(int fd, off64_t filebytes,int flag,int prot)
#else
char *
initfile(fd, filebytes,flag, prot)
int fd;
off64_t filebytes;
int flag, prot;
#endif
{
	 char *pa;
	 int mflags=0;
	 long long x;
	 char *tmp,*stmp;
	 int file_flags;
	 long long recs;
	 long long i;
	 int dflag = 0;

	 if(flag)
	 {

#ifdef _HPUX_SOURCE
		/* 
		 * Save time, just have the operating system prealloc 
		 * the file 
		 */
		prealloc(fd,filebytes);
#else
	 	/* 
		  * Allocate a temporary buffer to meet any alignment 
		  * contraints of any method.
		  */
		 tmp=(char *)malloc((size_t)reclen * 2);
		 stmp=tmp;
		 /* 
		  * Align to a reclen boundary.
		  */
		 tmp = (char *)((((long)tmp + (long)reclen))& ~(((long)reclen-1)));
		/* 
		 * Special case.. Open O_DIRECT, and going to be mmap() 
		 * Under Linux, one can not create a sparse file using 
		 * a file that is opened with O_DIRECT 
		 */
	 	file_flags=fcntl(fd,F_GETFL);

#if ! defined(DONT_HAVE_O_DIRECT)
#if defined(linux) || defined(__AIX__) || defined(IRIX) || defined(IRIX64) || defined(Windows) || defined (__FreeBSD__)
		dflag = O_DIRECT;
#endif
#if defined(TRU64)
	if(direct_flag)
		dflag = O_DIRECTIO;
#endif
#endif
	 	if((file_flags & dflag) !=0)
	 	{
	 		recs=filebytes/reclen;
			for (i =0; i<recs ;i++)
			{
				x=write(fd,tmp,(size_t)reclen);
				if(x < 1)
				{
					printf("Unable to write file\n");
					exit(182);
				}
			}
	 	}
		else
		{
			/* Save time, just seek out and touch at the end */
		 	I_LSEEK(fd,(filebytes-reclen),SEEK_SET);
			x=write(fd,tmp,(size_t)reclen);
			if(x < 1)
			{
				printf("Unable to write file\n");
				exit(181);
			}
		}
	 	free(stmp);
	 	I_LSEEK(fd,0,SEEK_SET);
#endif
	 }

#ifdef IRIX64
	if((prot & PROT_WRITE)==PROT_WRITE)
		mflags=MAP_SHARED;
	else
		mflags=MAP_PRIVATE;
#else
#ifdef IRIX
	if((prot & PROT_WRITE)==PROT_WRITE)
		mflags=MAP_SHARED;
	else
		mflags=MAP_PRIVATE;
#else
	if((prot & PROT_WRITE)==PROT_WRITE)
		mflags=MAP_FILE|MAP_SHARED;
	else
		mflags=MAP_FILE|MAP_PRIVATE;
#endif
#endif

#if defined(bsd4_2) && !defined(IOZ_macosx)
	 pa = (char *)mmap( 0,&filebytes, (int)prot, 
	 		(int)mflags, (int)fd, 0);
#else
	 pa = (char *)I_MMAP( ((char *)0),filebytes, prot, 
	 		mflags, fd, 0);
#endif
#ifdef __convex_spp
	if(pa == (char *)-1)
	{
		printf("\nMmap failed, errno %d Flags %x\n",errno,(int)mflags);
		exit(165);
	}
#else
#ifdef linux
	if(pa == (char *)-1)
	{
		printf("Mapping failed, errno %d\n",errno);
		exit(166);
	}
#else
#ifdef bsd4_2
	if(pa == (char *)-1)
	{
		printf("Mapping failed, errno %d\n",errno);
		exit(167);
	}
#else
	if(pa == (char *)MAP_FAILED)
	{
		printf("\nMapping failed, errno %d Flags = %x\n",errno,mflags);
		exit(168);
	}
#endif
#endif
#endif
#ifndef NO_MADVISE
	if(advise_flag)
	{
		switch(advise_op){
		case 0:
			madvise( (char *)pa, (size_t) filebytes, MADV_NORMAL);
			break;
		case 1: 
			madvise( (char *)pa, (size_t) filebytes, MADV_RANDOM);
			break;
		case 2: 
			madvise( (char *)pa, (size_t) filebytes, MADV_SEQUENTIAL);
			break;
		case 3: 
			madvise( (char *)pa, (size_t) filebytes, MADV_DONTNEED);
			break;
		case 4: 
			madvise( (char *)pa, (size_t) filebytes, MADV_WILLNEED);
			break;
		default: 
			break;
		};
	}
	
#endif
	return(pa);

}


/************************************************************************/
/* Release the mmap area.						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
mmap_end( char *buffer, long long size)
#else
void
mmap_end( buffer, size)
char *buffer;
long long size;
#endif
{
	if(munmap(buffer,(size_t)size)<0)
#ifdef NO_PRINT_LLD
		printf("munmap buffer %lx, size %ld failed.\n",(long)buffer,size);
#else
		printf("munmap buffer %lx, size %lld failed.\n",(long)buffer,size);
#endif
	
}

/************************************************************************/
/*	This is an interesting function. How much data to 		*/
/*	copy is a very good question. Here we are using mmap to		*/
/*	perform I/O. If the benchmark touches every byte then		*/
/*	this will include a bcopy of the mmap area into the		*/
/*	users buffer. This is represenative of an application		*/
/*	that reads and touches every byte that it read. If		*/
/*	the benchmark reduces the work to touching only			*/
/*	a long per page then the numbers go up but it			*/
/*	does not reflect the application to well. For now		*/
/*	the best assumption is to believe that the application		*/
/*	will indeed touch every byte.					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void 
fill_area(long long *src_buffer, long long *dest_buffer, long long length)
#else
fill_area( src_buffer, dest_buffer, length)
long long *src_buffer;
long long *dest_buffer;
long long length;
#endif
{
	/*printf("Fill area %d\n",(size_t)length);*/
	bcopy((void *)src_buffer,(void *)dest_buffer,(size_t)length);
}

#ifndef ASYNC_IO
int
async_read()
{
	printf("Your system does not support async I/O\n");
	exit(169);
}
size_t
async_write_no_copy()
{
	printf("Your system does not support async I/O\n");
	exit(170);
}
size_t
async_write()
{
	printf("Your system does not support async I/O\n");
	exit(171);
}
void
async_init()
{
	printf("Your system does not support async I/O\n");
	exit(172);
}
int
async_read_no_copy()
{
	printf("Your system does not support async I/O\n");
	exit(172);
}
void
async_release()
{
	printf("Your system does not support async I/O\n");
	exit(173);
}
#endif

/************************************************************************/
/* Nap in microseconds.							*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
my_nap( int ntime )
#else
void
my_nap( ntime )
int ntime;
#endif
{
	struct timeval nap_time;
	int seconds, microsecs;
	seconds = ntime/1000; /* Now in seconds */
	microsecs = (ntime*1000)%1000000;  /* Remaining microsecs */
	nap_time.tv_sec=seconds;
        nap_time.tv_usec=microsecs;
        select(0,0,0,0,&nap_time);
}
/************************************************************************/
/* Nap in microseconds.							*/
/************************************************************************/
int nap_once;
double nap_res;
#ifdef HAVE_ANSIC_C
void
my_unap( unsigned long long microsecs )
#else
void
my_unap( microsecs )
unsigned long long microsecs;
#endif
{
	double timein, timeout;
	unsigned long usecs;

	usecs = (unsigned long) microsecs;
	usecs = (usecs/1000)*1000;

	timein=time_so_far1();
	usleep(usecs); /* sleep for big chunk of time */

	while(1) /* now handle the remainder, to get finer control */
	{
		timeout=time_so_far1();
		if(timeout-timein > microsecs)
		   break;
	}
}

/************************************************************************/
/* Function that establishes the resolution 				*/
/* of the gettimeofday() function.					*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void
get_resolution(void)
#else
void
get_resolution()
#endif
{
        double starttime, finishtime, besttime = 0;
        long  j,delay;
	int k;

        finishtime=time_so_far1(); /* Warm up the instruction cache */
        starttime=time_so_far1();  /* Warm up the instruction cache */
        delay=j=0;                 /* Warm up the data cache */
	for(k=0;k<10;k++)
	{
	        while(1)
       	 	{
       	         	starttime=time_so_far1();
       	         	for(j=0;j< delay;j++)
       	                ;
       	         	finishtime=time_so_far1();
       	         	if(starttime==finishtime)
       	                 	delay++;
       	         	else
			{
				if(k==0)
					besttime=(finishtime-starttime);
				if((finishtime-starttime) < besttime)
					besttime=(finishtime-starttime);
                       	 	break;
			}
		}
        }
	time_res=besttime/1000000.0;	
}

/************************************************************************/
/* Function that establishes the resolution 				*/
/* of the getrusage() function.						*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void
get_rusage_resolution(void)
#else
void
get_rusage_resolution()
#endif
{
	double starttime, finishtime;
	long  j;

	finishtime=cputime_so_far(); /* Warm up the instruction cache */
	starttime=cputime_so_far();  /* Warm up the instruction cache */
	delay=j=0;		   /* Warm up the data cache */
	while(1)
	{
		starttime=cputime_so_far();
		for(j=0;j< delay;j++)
			;
		finishtime=cputime_so_far();
		if(starttime==finishtime)
			
			delay++;
		else
			break;
	}
	cputime_res = (finishtime-starttime);	 /* in seconds */
}
/************************************************************************/
/* Time measurement routines.						*/
/* Return time in microseconds						*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
static double
time_so_far1(void)
#else
static double
time_so_far1()
#endif
{
     /* For Windows the time_of_day() is useless. It increments in 
        55 milli second  increments. By using the Win32api one can 
	get access to the high performance measurement interfaces. 
	With this one can get back into the 8 to 9 microsecond resolution
      */
#ifdef Windows
	LARGE_INTEGER freq,counter;
	double wintime;
	double bigcounter;
  	struct timeval tp;

  	if(pit_hostname[0]){
  	   pit_gettimeofday(&tp, (struct timezone *) NULL, pit_hostname, 
		pit_service);
	   return ((double) (tp.tv_sec)*1000000.0)+(((double)tp.tv_usec));
	}
	else
	{	

       	   QueryPerformanceFrequency(&freq);
           QueryPerformanceCounter(&counter);
           bigcounter=(double)counter.HighPart *(double)0xffffffff +
                   (double)counter.LowPart;
           wintime = (double)(bigcounter/(double)freq.LowPart);
           return((double)wintime*1000000.0);
	}
#else
#if defined (OSFV4) || defined(OSFV3) || defined(OSFV5)
  struct timespec gp;

  if (getclock(TIMEOFDAY, (struct timespec *) &gp) == -1)
    perror("getclock");
  return (( (double) (gp.tv_sec)*1000000.0) +
    ( ((float)(gp.tv_nsec)) * 0.001 ));
#else
  struct timeval tp;

  if(pit_hostname[0]){
     if (pit_gettimeofday(&tp, (struct timezone *) NULL, pit_hostname, 
		pit_service) == -1)
        perror("pit_gettimeofday");
     return ((double) (tp.tv_sec)*1000000.0) + (((double) tp.tv_usec) );
  }
  else
  {
     if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
        perror("gettimeofday");
     return ((double) (tp.tv_sec)*1000000.0) + (((double) tp.tv_usec) );
  }
#endif
#endif
}

/************************************************************************/
/* Return the clocks per tick for the times() call.			*/
/************************************************************************/
#ifdef unix
#ifdef HAVE_ANSIC_C
static double
clk_tck(void)		/* Get the clocks per tick for times */
#else
static double
clk_tck()		/* Get the clocks per tick for times */
#endif
{
	return((double)sysconf(_SC_CLK_TCK));
}

/************************************************************************/
/* Return the user time in tics as a double.				*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
static double
utime_so_far(void)	 /* Return user time in ticks as double */
#else
static double
utime_so_far()
#endif
{
  	struct tms tp;

  	times(&tp);
  	return ((double) (tp.tms_utime));
}

/************************************************************************/
/* Return the system time in tics as a double.				*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
static double
stime_so_far(void)  	/* Return system time in ticks as double */
#else
static double
stime_so_far()
#endif
{
  	struct tms tp;

  	times(&tp);
  	return ((double) (tp.tms_stime));
}

/************************************************************************/
/* Return the CPU (user + system) time in seconds as a double.		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
static double
cputime_so_far(void)  	/* Return CPU time in seconds as double */
#else
static double
cputime_so_far()
#endif
{
#if 0
  	struct tms tp;

  	times(&tp);
  	return ((double) (tp.tms_utime + tp.tms_stime) / sc_clk_tck);
#else
	struct rusage ru;

	if (getrusage (RUSAGE_SELF, &ru))
		perror ("getrusage");
	return ((double)(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) +
		.000001 *(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec));
#endif
}
#endif

/************************************************************************/
/* Return the CPU utilization ((user + system) / walltime) as a percentage. */
/************************************************************************/
#ifdef HAVE_ANSIC_C
static double
cpu_util(double cputime, double walltime)
#else
static double
cpu_util(cputime, walltime)
double cputime, walltime;
#endif
{
	double cpu;

	if (walltime <= (double)0.0)
	{
		cpu = (double)0.0;
		return cpu;
	}
	if (cputime <= (double)0.0)
		cputime = 0.0;
	if (walltime <= (double)0.0)
		cpu = (double)100.0;
	else {
		cpu = (((double)100.0 * cputime) / walltime);
		/*
		if (cpu > (double)100.0)
			cpu = (double)99.99;
		*/
	}
	return cpu;
}

/************************************************************************/
/* This is a locking function that permits the writes and 		*/
/* reads during the test to hold a file lock. Since each		*/
/* tmp file that Iozone creates is a private file, this seems		*/
/* like a no-op but it turns out that when using Iozone			*/
/* over NFS,  life is very, very different. Some vendors		*/
/* read and write performance goes to zip when locks are held		*/
/* even if there is only one process using the file and having		*/
/* it locked. Some implementations of NFS transition from async		*/
/* to fully sync reads and writes if any locks are used. Euck...	*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
int
mylockf(int fd, int op, int rdwr)
#else
int
mylockf(fd, op, rdwr)
int fd, op, rdwr;
#endif
{
	struct flock myflock;
	int ret;
	if(op==0) /* Generic unlock the whole file */
	{
		myflock.l_type=F_UNLCK;
		myflock.l_whence=SEEK_SET;
		myflock.l_start=0;
		myflock.l_len=0; /* The whole file */
		myflock.l_pid=getpid();
		ret=fcntl(fd,F_SETLKW, &myflock);
	}
	else
		  /* Generic lock the whole file */
	{
		if(rdwr==0)
			myflock.l_type=F_WRLCK; /* Apply write lock */
		else
			myflock.l_type=F_RDLCK; /* Apply read lock */
		myflock.l_whence=SEEK_SET;
		myflock.l_start=0;
		myflock.l_len=0; /* The whole file */
		myflock.l_pid=getpid();
		ret=fcntl(fd,F_SETLKW, &myflock);
	}
	return(ret);
}

#ifdef HAVE_ANSIC_C
int
mylockr(int fd, int op, int rdwr, off64_t offset, off64_t size)
#else
int
mylockr(fd, op, rdwr, offset, size)
int fd, op, rdwr;
off64_t offset;
off64_t size;
#endif
{
	struct flock myflock;
	int ret;
	if(op==0) /* Generic unlock the whole file */
	{
		/*printf("Child: %lld Unlock offset %lld size %lld\n",chid,offset,size);*/
		myflock.l_type=F_UNLCK;
		myflock.l_whence=SEEK_SET;
		myflock.l_start=offset;
		myflock.l_len=size; /* The whole file */
		myflock.l_pid=getpid();
		ret=fcntl(fd,F_SETLKW, &myflock);
	}
	else
		  /* Generic lock the range  */
	{
		if(rdwr==0)
		{
			myflock.l_type=F_WRLCK; /* Apply write lock */
			/* printf("Write ");*/
		}
		else
		{
			myflock.l_type=F_RDLCK; /* Apply read lock */
			/* printf("Read ");*/
		}
		/*printf("Child: %lld Lock offset %lld size %lld\n",chid, offset,size);*/
		myflock.l_whence=SEEK_SET;
		myflock.l_start=offset;
		myflock.l_len=size; /* The whole file */
		myflock.l_pid=getpid();
		ret=fcntl(fd,F_SETLKW, &myflock);
	}
	return(ret);
}
/************************************************************************/
/* This function is used to simulate compute time that does		*/
/* not involve the I/O subsystem.					*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
float
do_compute(float comp_delay)
#else
float
do_compute(comp_delay)
float comp_delay;
#endif
{
	double starttime,tmptime;
	if(comp_delay == (float)0.0)
		return(0.0);
	starttime=time_so_far();
	while(1)
	{	
		tmptime=time_so_far()-starttime;
		if(tmptime >= (double)comp_delay)
			return(tmptime);
	}
	return(0.0);
}

/************************************************************************/
/* This function is intended to cause an interruption			*/
/* in the read pattern. It will make a reader have			*/
/* jitter in its access behavior.					*/
/* When using direct I/O one must use a pagesize transfer.		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
disrupt(int fd)
#else
void
disrupt(fd)
int fd;
#endif
{
	char *nbuff,*free_addr;
	off64_t current;

	free_addr=nbuff=(char *)malloc((size_t)page_size+page_size);
	nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));

	/* Save current position */
	current = I_LSEEK(fd,0,SEEK_CUR);

	/* Move to beginning of file */
	I_LSEEK(fd,0,SEEK_SET);

	/* Read a little of the file */
	if(direct_flag)
		junk=read(fd,nbuff,page_size);
	else
		junk=read(fd,nbuff,1);

	/* Skip into the file */
	I_LSEEK(fd,page_size,SEEK_SET);

	/* Read a little of the file */
	if(direct_flag)
		junk=read(fd,nbuff,page_size);
	else
		junk=read(fd,nbuff,1);

	/* Restore current position in file, before disruption */
	I_LSEEK(fd,current,SEEK_SET);
	free(free_addr);
	
}

#if defined(Windows)
/************************************************************************/
/* This function is intended to cause an interruption			*/
/* in the read pattern. It will make a reader have			*/
/* jitter in its access behavior.					*/
/* When using direct I/O one must use a pagesize transfer.		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void
disruptw(HANDLE hand)
#else
void
disruptw(HANDLE)
int hand;
#endif
{
	char *nbuff,*free_addr;
	off64_t current;
	long retval;

	free_addr=nbuff=(char *)malloc((size_t)page_size+page_size);
	nbuff=(char *)(((long)nbuff+(long)page_size) & (long)~(page_size-1));

	/* Save current position */
	current=SetFilePointer(hand,(LONG)0,0,FILE_CURRENT);

	/* Move to beginning of file */
	SetFilePointer(hand,(LONG)0,0,FILE_BEGIN);

	/* Read a little of the file */
	ReadFile(hand, nbuff, reclen,(LPDWORD)&retval,0);

	/* Skip into the file */
	SetFilePointer(hand,(LONG)page_size,0,FILE_BEGIN);

	/* Read a little of the file */
	ReadFile(hand, nbuff, reclen,(LPDWORD)&retval,0);

	/* Restore current position in file, before disruption */
	SetFilePointer(hand,(LONG)current,0,FILE_BEGIN);
	free(free_addr);
	
}
#endif

/************************************************************************/
/* Read a telemetry file and return the the offset			*/
/* for the next operaton. Also, set the size				*/
/* in the variable given in the param list.				*/
/* which == 0 ... reader calling					*/
/* which == 1 ... writer calling					*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
long long
get_traj(FILE *traj_fd, long long *traj_size, float *delay, long which)
#else
long long
get_traj(traj_fd, traj_size, delay, which)
FILE *traj_fd;
long long *traj_size;
float *delay;
long which;
#endif
{
	long long traj_offset = 0;
	long long tmp2 = 0;
	int tmp = 0;
	int tokens;
	int ret=0;
	char *ret1,*where;
	char buf[200];
	char sbuf[200];
	int got_line;
	
	got_line=0;

	while(got_line==0)
	{
		tokens=0;
		ret1=fgets(buf,200,traj_fd);
		if(ret1==(char *)0)
		{
			printf("\n\n\tEarly end of telemetry file. Results not accurate.\n");
			signal_handler();
		}
		where=(char *)&buf[0];
		strcpy(sbuf,buf);
		if((*where=='#') || (*where=='\n'))
			continue;
		tokens++;
		strtok(where," ");
		while( (char *)(strtok( (char *)0," ")) != (char *)0)
		{
			tokens++;
		}
		got_line=1;
	}
	if(tokens == 3)
	{
#ifdef NO_PRINT_LLD
		ret=sscanf(sbuf,"%ld %ld %d\n",&traj_offset,&tmp2,&tmp);
#else
		ret=sscanf(sbuf,"%lld %lld %d\n",&traj_offset,&tmp2,&tmp);
#endif
	/*printf("\nReading %s trajectory with %d items\n",which?"write":"read",tokens);*/
		*traj_size=tmp2;
		*delay= ((float)tmp/1000);
	}
	if(tokens == 2)
	{ 
#ifdef NO_PRINT_LLD
		ret=sscanf(sbuf,"%ld %ld\n",&traj_offset,traj_size);
#else
		ret=sscanf(sbuf,"%lld %lld\n",&traj_offset,traj_size);
#endif
		*delay=compute_time;
	/*printf("\nReading %s trajectory with %d items\n",which?"write":"read",tokens);*/
	}
	if((tokens != 2) && (tokens !=3))
	{
		printf("\n\tInvalid entry in telemetry file. > %s <\n",sbuf);
		exit(178);
	}
	if(ret==EOF)
	{
		printf("\n\n\tEarly end of telemetry file. Results not accurate.\n");
		signal_handler();
	}
#ifdef DEBUG
#ifdef NO_PRINT_LLD
	if(!silent) printf("\nOffset %ld  Size %ld Compute delay %f\n",traj_offset, *traj_size,*delay);
#else
	if(!silent) printf("\nOffset %lld  Size %lld Compute delay %f\n",traj_offset, *traj_size,*delay);
#endif
#endif
	return(traj_offset);
}

/************************************************************************/
/* Open the read telemetry file and return file pointer. 		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
FILE *
open_r_traj(void)
#else
FILE *
open_r_traj()
#endif
{
	FILE *fd;
	fd=fopen(read_traj_filename,"r");
	if(fd == (FILE *)0)
	{
		printf("Unable to open read telemetry file \"%s\"\n",
			read_traj_filename);
		exit(174);
	}
	return(fd);
}

/************************************************************************/
/* Open the write telemetry file and return file pointer.		*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
FILE *
open_w_traj(void)
#else
FILE *
open_w_traj()
#endif
{
	FILE *fd;
	fd=fopen(write_traj_filename,"r");
	if(fd == (FILE *)0)
	{
		printf("Unable to open write telemetry file \"%s\"\n",
			write_traj_filename);
		exit(175);
	}
	return(fd);
}

/************************************************************************/
/* r_traj_size(void)							*/
/* This function scans the read telemetry file 				*/
/* and establishes the number of entries				*/
/* and the maximum file offset.						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
void 
r_traj_size(void)
#else
void
r_traj_size()
#endif
{
	FILE *fd;
	int ret;
	long long traj_offset = 0;
	long long traj_size = 0;
	long long max_offset = 0;
	int tokens;
	int dummy;
	int lines;
	char buf[200];
	char sbuf[200];
	char *ret1,*where;

	lines=0;
	fd=fopen(read_traj_filename,"r");
	if(fd == (FILE *)0)
	{
		printf("Unable to open read telemetry file \"%s\"\n",
			read_traj_filename);
		exit(174);
	}
	while(1)
	{
		tokens=0;
		ret1=fgets(buf,200,fd);
		if(ret1==(char *)0)
			break;
		where=(char *)&buf[0];
		strcpy(sbuf,buf);
		lines++;
		if((*where=='#') || (*where=='\n'))
			continue;
		tokens++;
		strtok(where," ");
		while( (char *)(strtok( (char *)0," ")) != (char *)0)
		{
			tokens++;
		}
		if(tokens==1)
		{
			printf("\n\tInvalid read telemetry file entry. Line %d",
				lines);
			signal_handler();
		}
#ifdef DEBUG
	printf("Tokens = %d\n",tokens);
#endif
		if(tokens==3)
		{
#ifdef NO_PRINT_LLD
			ret=sscanf(sbuf,"%ld %ld %d\n",&traj_offset,&traj_size,&dummy);
#else
			ret=sscanf(sbuf,"%lld %lld %d\n",&traj_offset,&traj_size,&dummy);
#endif
		}
		if(tokens==2)
		{
#ifdef NO_PRINT_LLD
			ret=sscanf(sbuf,"%ld %ld\n",&traj_offset,&traj_size);
#else
			ret=sscanf(sbuf,"%lld %lld\n",&traj_offset,&traj_size);
#endif
		}
		if((tokens != 2) && (tokens !=3))
		{
			printf("\n\tInvalid read telemetry file. Line %d\n",lines);
			exit(178);
		}
		if(traj_offset + traj_size > max_offset)
			max_offset=traj_offset + traj_size;
		
		r_traj_ops++;
	}	
	r_traj_fsize=max_offset;
#ifdef DEBUG

#ifdef NO_PRINT_LLD
	printf("File size of read %ld Item count %ld\n",r_traj_fsize,r_traj_ops);
#else
	printf("File size of read %lld Item count %lld\n",r_traj_fsize,r_traj_ops);
#endif

#endif
	fclose(fd);
}

/************************************************************************/
/* w_traj_size(void)							*/
/* This function scans the write telemetry file 			*/
/* and establishes the number of entries				*/
/* and the maximum file offset.						*/
/************************************************************************/
#ifdef HAVE_ANSIC_C
long long
w_traj_size(void)
#else
long long
w_traj_size()
#endif
{
	FILE *fd;
	int ret;
	long long traj_offset = 0;
	long long traj_size = 0;
	long long max_offset = 0;
	int dummy;
	int tokens,lines;
	char *ret1;
	char buf[200];
	char sbuf[200];
	char *where;

	lines=0;

	fd=fopen(write_traj_filename,"r");
	if(fd == (FILE *)0)
	{
		printf("Unable to open write telemetry file \"%s\"\n",
			write_traj_filename);
		exit(174);
	}
	while(1)
	{
		tokens=0;
		ret1=fgets(buf,200,fd);
		if(ret1==(char *)0)
			break;
		lines++;
		where=(char *)&buf[0];
		strcpy(sbuf,buf);
		if((*where=='#') || (*where=='\n'))
			continue;
		tokens++;
		strtok(where," ");
		while( (char *)(strtok( (char *)0," ")) != (char *)0)
		{
			tokens++;
		}
		if(tokens==1)
		{
			printf("\n\tInvalid write telemetry file entry. Line %d\n",
				lines);
			signal_handler();
		}
		if(tokens==3)
		{
#ifdef NO_PRINT_LLD
			ret=sscanf(sbuf,"%ld %ld %d\n",&traj_offset,&traj_size,&dummy);
#else
			ret=sscanf(sbuf,"%lld %lld %d",&traj_offset,&traj_size,&dummy);
#endif
		}
		if(tokens==2)
		{
#ifdef NO_PRINT_LLD
			ret=sscanf(sbuf,"%ld %ld\n",&traj_offset,&traj_size);
#else
			ret=sscanf(sbuf,"%lld %lld\n",&traj_offset,&traj_size);
#endif
		}
		if(tokens > 3)
		{
			printf("\n\tInvalid write telemetry file entry. Line %d\n",
				lines);
			exit(174);
		}
		if(traj_offset + traj_size > max_offset)
			max_offset=traj_offset + traj_size;
		
		w_traj_ops++;
	}	
	w_traj_fsize=max_offset;
#ifdef DEBUG
#ifdef NO_PRINT_LLD
	printf("File size of write %ld Item count %ld\n",w_traj_fsize,w_traj_ops);
#else
	printf("File size of write %lld Item count %lld\n",w_traj_fsize,w_traj_ops);
#endif
#endif
	fclose(fd);
	return(max_offset);
}

/************************************************************************/
/* Find which version of the telemetry file format is in use.		*/
/************************************************************************/

#ifdef HAVE_ANSIC_C
void
traj_vers(void)
#else
void
traj_vers()
#endif
{
	FILE *fd;
	char *where;
	char buf[200];
	int things;
	char *ret1;
	
	if(r_traj_flag)
	{
		things=0;
		fd=fopen(read_traj_filename,"r");
		if(fd == (FILE *)0)
		{	
			printf("Unable to open read telemetry file \"%s\"\n", read_traj_filename);
			exit(174);
		}
loop1:
		ret1=fgets(buf,200,fd);
		if(ret1==(char *)0)
		{
			fclose(fd);
			return;
		}
		where=(char *)&buf[0];
		if((*where=='#') || (*where=='\n'))
			goto loop1;
		things++;
		strtok(where," ");
		while( (char *)(strtok( (char *)0," ")) != (char *)0)
		{
			things++;
		}
		r_traj_items=things;
#ifdef DEBUG
		printf("Found %d items in the read telemetry file\n",things);
#endif
	}
	if(w_traj_flag)
	{
		things=0;
		fd=fopen(write_traj_filename,"r");
		if(fd == (FILE *)0)
		{	
			printf("Unable to open write telemetry file \"%s\"\n", write_traj_filename);
			exit(174);
		}
loop2:
		ret1=fgets(buf,200,fd);
		if(ret1==(char *)0)
		{
			fclose(fd);
			return;
		}
		where=(char *)&buf[0];
		if((*where=='#') || (*where=='\n'))
			goto loop2;
		things++;
		strtok(where," ");
		while( (char *)(strtok( (char *)0," ")) != (char *)0)
		{
			things++;
		}
		fclose(fd);
		w_traj_items=things;
#ifdef DEBUG
		printf("Found %d items in the write telemetry file\n",things);
#endif
	}
}

/********************************************************************/
/*							            */
/* Today this initializes the default set of file sizes for Iozone. */
/* in the future it may take input from the command line or	    */
/* from a file.							    */
/*								    */
/********************************************************************/
#ifdef HAVE_ANSIC_C
void
init_file_sizes( off64_t min_f_size,  off64_t max_f_size)
#else
void
init_file_sizes(min_f_size, max_f_size)
off64_t min_f_size;
off64_t max_f_size;
#endif
{
    	off64_t kilosi;
	int x;
	if(s_count > 1)
	{
	        for(x=0; x < s_count; x++)
		{
			kilosi=s_range[x];
			add_file_size((off64_t)kilosi);
		}
	}
	else
	{
	        for(kilosi=min_f_size;kilosi<=max_f_size;kilosi*=multiplier)
		{
			add_file_size((off64_t)kilosi);
		}
	}
}

/********************************************************************/
/* Used to constuct the list of file sizes to test.		    */
/********************************************************************/
#ifdef HAVE_ANSIC_C
void
add_file_size(off64_t size)
#else
void
add_file_size(size)
off64_t size;
#endif
{
	struct size_entry *size_listp;
	struct size_entry *nsize_list;
	
	size_listp=size_list;
	
	if(size_list)
	{
		if(size_listp->next)
			while(size_listp->next!=0)
				size_listp=size_listp->next;
	}
	nsize_list=(struct size_entry *)malloc(sizeof(struct size_entry));
	if(nsize_list==0)
	{
		printf("Malloc failed in add_file_size\n");
		exit(180);
	}	
	nsize_list->next=0;
	nsize_list->size=size;
	if(size_list == 0)
		size_list=nsize_list;
	else
		size_listp->next=nsize_list;
	size_listp=size_list;
}

/********************************************************************/
/* Return the next file size to test.				    */
/********************************************************************/
#ifdef HAVE_ANSIC_C
off64_t
get_next_file_size(off64_t size)
#else
off64_t
get_next_file_size(size)
off64_t size;
#endif
{
	struct size_entry *size_listp;
	
	size_listp=size_list;
	
	for( ; size_listp ; size_listp=size_listp->next )
	{
		if(size_listp->size > size)
			return(size_listp->size);
	}
	return((off64_t)0);
}


/**********************************************************************/
/*							              */
/* Today this initializes the default set of record sizes for Iozone. */
/* in the future it may take input from the command line or	      */
/* from a file.							      */
/*								      */
/**********************************************************************/
#ifdef HAVE_ANSIC_C
void
init_record_sizes( off64_t min_r_size,  off64_t max_r_size)
#else
void
init_record_sizes(min_r_size, max_r_size)
off64_t min_r_size;
off64_t max_r_size;
#endif
{
	int x;
    	off64_t size;
	if(r_count > 1)
	{
	        for(x=0; x < r_count; x++)
		{
			size=r_range[x];
			add_record_size((off64_t)size);
		}
	}
	else
	{
	        for(size=min_r_size;size<=max_r_size;size*=multiplier)
		{
			add_record_size((off64_t)size);
		}
	}
}

#ifdef HAVE_ANSIC_C
void
del_record_sizes(void)
#else
void
del_record_sizes()
#endif
{
        struct size_entry *size_listp;
        struct size_entry *save_item;

        size_listp=rec_size_list;
        if(rec_size_list)
        {
                while(size_listp!=0)
                {
                        save_item=size_listp->next;
                        free(size_listp);
                        size_listp=save_item;
                }
        }
        rec_size_list=0;
}

/********************************************************************/
/* Used to constuct the list of record sizes to test.		    */
/********************************************************************/
#ifdef HAVE_ANSIC_C
void
add_record_size(off64_t size)
#else
void
add_record_size(size)
off64_t size;
#endif
{
	struct size_entry *size_listp;
	struct size_entry *nsize_list;
	
	size_listp=rec_size_list;
	
	if(rec_size_list)
	{
		if(size_listp->next)
			while(size_listp->next!=0)
				size_listp=size_listp->next;
	}
	nsize_list=(struct size_entry *)malloc(sizeof(struct size_entry));
	if(nsize_list==0)
	{
		printf("Malloc failed in add_file_size\n");
		exit(180);
	}	
	nsize_list->next=0;
	nsize_list->size=size;
	if(rec_size_list == 0)
		rec_size_list=nsize_list;
	else
		size_listp->next=nsize_list;
	size_listp=rec_size_list;
}

/********************************************************************/
/* Return the next record size to test.				    */
/********************************************************************/
#ifdef HAVE_ANSIC_C
off64_t
get_next_record_size(off64_t size)
#else
off64_t
get_next_record_size(size)
off64_t size;
#endif
{
	struct size_entry *size_listp;
	
	size_listp=rec_size_list;
	
	for( ; size_listp ; size_listp=size_listp->next )
	{
		if(size_listp->size > size)
			return(size_listp->size);
	}
	return((off64_t)0);
}


/*
 * Socket based communication mechanism.
 * It's intended use is to be the communication mechanism
 * that will be used to get Iozone to run across
 * multiple clients. 1/11/2002  Don Capps
 * The communication model permits a master to send and receive
 * messages to and from clients, and for clients to be able to 
 * send and receive messages to and from the master.
 */
/* 
 * Interfaces are:
	Master:
	  int start_master_listen(void)
		Called to create masters listening port.

	  void master_listen(int sock, int size_of_message)
		Call when master wants to block and read
		a message.

	  int start_master_send(char *child_host_name, int port)
		Call to start a send channel to a client.

	  void master_send(int child_socket_val, char *host_name, 
		    char *send_buffer, int send_size)
		Call to send message to a client.

	  void stop_master_listen(int master_socket_val)
		Call to release the masters listening port.

	  void stop_master_send(int child_socket_val)
		Call to release the masters send port to a client.

	Clients:
	  int start_child_listen(int size_of_message)
		Called to create clients listening port.

	  void child_listen(int sock, int size_of_message)
		Call when client wants to block and read
		a message from the master.

	  void child_send(int child_socket_val, char *controlling_host_name, 
		   char *send_buffer, int send_size)
		Call to send message to the master.

	  void stop_child_listen(int child_socket_val)
		Call to release the clients listening port.

	  void stop_child_send(int child_socket_val)
		Call to release the clients send port to the master.


	Messages are sent in command blocks. The structure is
	client_command for messages from the master to the 
	client, and master_command for messages sent from
	a client to the master.
*/


/*
 * Allocate the master listening port that 
 * all children will use to send messages to the master.
 */
#ifdef HAVE_ANSIC_C
int
start_master_listen(void)
#else
int
start_master_listen()
#endif
{
	int s;
	int rc;
	int tmp_port;
	int sockerr;
	struct sockaddr_in addr;
	int recv_buf_size=65536*4;
	int optval=1;
	struct linger dummy={1,0};

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
                perror("socket failed:");
                exit(19);
        }
	sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
		&recv_buf_size, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 1\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)
		&optval, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 2\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_LINGER, (char *)
		&dummy, sizeof(struct linger));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 2\n");
	}
	tmp_port=HOST_LIST_PORT;
        bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_port = htons(tmp_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(s, (struct sockaddr *)&addr,
                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	addr.sin_port=htons(tmp_port);
			continue;
		}
		master_listen_port = ntohs(addr.sin_port);
        }
	if(rc < 0)
	{
		perror("bind failed\n");
		exit(20);
	}

	if(mdebug)
	   printf("Master listening on socket %d Port %d\n",s,tmp_port);
	return(s);
}

/*
 * Master listens for messages and blocks until
 * something arrives.
 */
struct sockaddr_in listener_sync_sock;

#ifdef HAVE_ANSIC_C
void
master_listen(int sock, int size_of_message)
#else
void
master_listen(sock, size_of_message)
int sock, size_of_message;
#endif
{
        int tsize;
        int s;
        struct sockaddr_in *addr;
        unsigned int me;
        int ns,ret;
	struct master_neutral_command *mnc;

	mnc=(struct master_neutral_command *)&master_rcv_buf[0];
        tsize = size_of_message;
	addr=&listener_sync_sock;
        s = sock;
	me=sizeof(struct sockaddr_in);

        if(mdebug)
          printf("Master in listening mode on socket %d\n",s);
again:
        ret=listen(s,MAXSTREAMS);
        if(ret != 0)
        {
                perror("Master: listen returned error\n");
        }
        if(mdebug)
          printf("Master in accepting connection\n");
        ns=accept(s,(void *)addr,&me);
        if(ns < 0)
        {
                printf("Master socket %d\n",s);
                perror("Master: ***** accept returned error *****\n");
		sleep(1);
		goto again;
        }
        if(mdebug)
          printf("Master in reading from connection\n");

        ret=read(ns,mnc,tsize);
	if(ret < tsize)
	{
            printf("Master read failed. Ret %d Errno %d\n",ret,errno);
	}

        close(ns);
}

/*
 * Child sends message to master.
 */

#ifdef HAVE_ANSIC_C
void
child_send(char *controlling_host_name, struct master_command *send_buffer, int send_size)
#else
void
child_send(controlling_host_name, send_buffer, send_size)
char *controlling_host_name; 
struct master_command *send_buffer; 
int send_size;
#endif
{

        int rc,child_socket_val;
        struct hostent *he;
        int tmp_port;
        struct in_addr *ip;
        struct sockaddr_in cs_addr,cs_raddr;
	struct master_neutral_command outbuf;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;

        if(cdebug)
	{
           fprintf(newstdout,"Start_child_send: %s  Size %d\n",controlling_host_name,send_size);
	   fflush(newstdout);
	}
        he = gethostbyname(controlling_host_name);
        if (he == NULL)
        {
                exit(22);
        }
        ip = (struct in_addr *)he->h_addr_list[0];

over:
        cs_raddr.sin_family = AF_INET;
        cs_raddr.sin_port = htons(controlling_host_port);
        cs_raddr.sin_addr.s_addr = ip->s_addr;
        child_socket_val = socket(AF_INET, SOCK_STREAM, 0);
        if (child_socket_val < 0)
        {
                perror("Child: socket failed:");
                exit(23);
        }
        bzero(&cs_addr, sizeof(struct sockaddr_in));
        tmp_port= CHILD_ESEND_PORT;
        cs_addr.sin_port = htons(tmp_port);
        cs_addr.sin_family = AF_INET;
        cs_addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(child_socket_val, (struct sockaddr *)&cs_addr,
                                                sizeof(struct sockaddr_in));
                if(rc < 0)
                {
                        tmp_port++;
                        cs_addr.sin_port=htons(tmp_port);
                        continue;
                }
        }
        if (rc < 0)
        {
                perror("Child: bind failed\n");
                exit(24);
        }
        if(cdebug)
	{
          fprintf(newstdout,"Child sender bound to port %d Master port %d \n",tmp_port,HOST_LIST_PORT);
	  fflush(newstdout);
	}
again:
	nanosleep(&req,&rem);
        rc = connect(child_socket_val, (struct sockaddr *)&cs_raddr,
                        sizeof(struct sockaddr_in));
        if (rc < 0)
        {
                if((ecount++ < 200) && (errno != EISCONN))
                {
			nanosleep(&req,&rem);
                        /*sleep(1);*/
                        goto again;
                }
                if(cdebug)
                {
          	   fprintf(newstdout,"Child: connect failed. Errno %d \n",errno);
		   fflush(newstdout);
                }
                close(child_socket_val);
		nanosleep(&req,&rem);
                /*sleep(1);*/
                ecount=0;
                goto over;
        }
        ecount=0;
        if(cdebug)
	{
          fprintf(newstdout,"Child connected\n");
	  fflush(newstdout);
	}

	/* NOW send */

	bzero(&outbuf, sizeof(struct master_neutral_command));
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d sending message to %s \n",(int)chid, controlling_host_name);
		fflush(newstdout);
	}
	/* 
	 * Convert internal commands to string format to neutral format for portability
	 */
	strcpy(outbuf.m_host_name,send_buffer->m_host_name);
	strcpy(outbuf.m_client_name,send_buffer->m_client_name);
	sprintf(outbuf.m_client_number,"%d",send_buffer->m_client_number);
	sprintf(outbuf.m_client_error,"%d",send_buffer->m_client_error);
	sprintf(outbuf.m_child_port,"%d",send_buffer->m_child_port);
	sprintf(outbuf.m_child_async_port,"%d",send_buffer->m_child_async_port);
	sprintf(outbuf.m_command,"%d",send_buffer->m_command);
	sprintf(outbuf.m_testnum,"%d",send_buffer->m_testnum);
	sprintf(outbuf.m_version,"%d",send_buffer->m_version);
	sprintf(outbuf.m_mygen,"%d",send_buffer->m_mygen);
	sprintf(outbuf.m_throughput,"%f",send_buffer->m_throughput);
	sprintf(outbuf.m_cputime,"%f", send_buffer->m_cputime);
	sprintf(outbuf.m_walltime,"%f",send_buffer->m_walltime);
	sprintf(outbuf.m_stop_flag,"%d",send_buffer->m_stop_flag);
	sprintf(outbuf.m_actual,"%f",send_buffer->m_actual);
#ifdef NO_PRINT_LLD
	sprintf(outbuf.m_child_flag,"%ld",send_buffer->m_child_flag);
#else
	sprintf(outbuf.m_child_flag,"%lld",send_buffer->m_child_flag);
#endif
	rc=write(child_socket_val,&outbuf,sizeof(struct master_neutral_command));
        if (rc < 0) {
                perror("write failed\n");
                exit(26);
        }
	close(child_socket_val);
}


/*
 * Master sending message to a child
 * There should be a unique child_socket_val for each
 * child.
 */
#ifdef HAVE_ANSIC_C
void
master_send(int child_socket_val, char *host_name, struct client_command *send_buffer, int send_size)
#else
void
master_send(child_socket_val, host_name, send_buffer, send_size)
int child_socket_val;
char *host_name; 
struct client_command *send_buffer; 
int send_size;
#endif
{
	int rc;
	struct client_neutral_command outbuf;

	bzero(&outbuf,sizeof(struct client_neutral_command));
	if(mdebug)
	{
		printf("Master_neutral_command size = %lu\n",(unsigned long)sizeof(struct master_neutral_command));
		printf("Client_neutral_command size = %lu\n",(unsigned long)sizeof(struct client_neutral_command));
	}
	/* 
	 * Convert internal commands to string format for neutral format/portability
	 */
	strcpy(outbuf.c_host_name,send_buffer->c_host_name);
	strcpy(outbuf.c_pit_hostname,send_buffer->c_pit_hostname);
	strcpy(outbuf.c_pit_service,send_buffer->c_pit_service);
	strcpy(outbuf.c_client_name,send_buffer->c_client_name);
	strcpy(outbuf.c_working_dir,send_buffer->c_working_dir);
	strcpy(outbuf.c_file_name,send_buffer->c_file_name);
	strcpy(outbuf.c_path_dir,send_buffer->c_path_dir);
	strcpy(outbuf.c_execute_name,send_buffer->c_execute_name);
	strcpy(outbuf.c_write_traj_filename,send_buffer->c_write_traj_filename);
	strcpy(outbuf.c_read_traj_filename,send_buffer->c_read_traj_filename);
	sprintf(outbuf.c_oflag,"%d",send_buffer->c_oflag);
	sprintf(outbuf.c_mfflag,"%d",send_buffer->c_mfflag);
	sprintf(outbuf.c_unbuffered,"%d",send_buffer->c_unbuffered);
	sprintf(outbuf.c_noretest,"%d",send_buffer->c_noretest);
	sprintf(outbuf.c_notruncate,"%d",send_buffer->c_notruncate);
	sprintf(outbuf.c_read_sync,"%d",send_buffer->c_read_sync);
	sprintf(outbuf.c_jflag,"%d",send_buffer->c_jflag);
	sprintf(outbuf.c_async_flag,"%d",send_buffer->c_async_flag);
	sprintf(outbuf.c_mmapflag,"%d",send_buffer->c_mmapflag);
	sprintf(outbuf.c_k_flag,"%d",send_buffer->c_k_flag);
	sprintf(outbuf.c_h_flag,"%d",send_buffer->c_h_flag);
	sprintf(outbuf.c_mflag,"%d",send_buffer->c_mflag);
	sprintf(outbuf.c_pflag,"%d",send_buffer->c_pflag);
	sprintf(outbuf.c_stride_flag,"%d",send_buffer->c_stride_flag);
	sprintf(outbuf.c_verify,"%d",send_buffer->c_verify);
	sprintf(outbuf.c_sverify,"%d",send_buffer->c_sverify);
	sprintf(outbuf.c_odsync,"%d",send_buffer->c_odsync);
	sprintf(outbuf.c_diag_v,"%d",send_buffer->c_diag_v);
	sprintf(outbuf.c_dedup,"%d",send_buffer->c_dedup);
	sprintf(outbuf.c_dedup_interior,"%d",send_buffer->c_dedup_interior);
	sprintf(outbuf.c_dedup_compress,"%d",send_buffer->c_dedup_compress);
	sprintf(outbuf.c_dedup_mseed,"%d",send_buffer->c_dedup_mseed);
	sprintf(outbuf.c_chid_skew,"%d",send_buffer->c_chid_skew);
	sprintf(outbuf.c_hist_summary,"%d",send_buffer->c_hist_summary);
	sprintf(outbuf.c_op_rate,"%d",send_buffer->c_op_rate);
	sprintf(outbuf.c_op_rate_flag,"%d",send_buffer->c_op_rate_flag);
	sprintf(outbuf.c_Q_flag,"%d",send_buffer->c_Q_flag);
	sprintf(outbuf.c_inc_think,"%d",send_buffer->c_inc_think);
	sprintf(outbuf.c_L_flag,"%d",send_buffer->c_L_flag);
	sprintf(outbuf.c_include_flush,"%d",send_buffer->c_include_flush);
	sprintf(outbuf.c_OPS_flag,"%d",send_buffer->c_OPS_flag);
	sprintf(outbuf.c_mmapnsflag,"%d",send_buffer->c_mmapnsflag);
	sprintf(outbuf.c_mmapssflag,"%d",send_buffer->c_mmapssflag);
	sprintf(outbuf.c_mmapasflag,"%d",send_buffer->c_mmapasflag);
	sprintf(outbuf.c_no_copy_flag,"%d",send_buffer->c_no_copy_flag);
	sprintf(outbuf.c_include_close,"%d",send_buffer->c_include_close);
	sprintf(outbuf.c_disrupt_flag,"%d",send_buffer->c_disrupt_flag);
	sprintf(outbuf.c_compute_flag,"%d",send_buffer->c_compute_flag);
	sprintf(outbuf.c_xflag,"%d",send_buffer->c_xflag);
	sprintf(outbuf.c_MS_flag,"%d",send_buffer->c_MS_flag);
	sprintf(outbuf.c_mmap_mix,"%d",send_buffer->c_mmap_mix);
	sprintf(outbuf.c_Kplus_flag,"%d",send_buffer->c_Kplus_flag);
	sprintf(outbuf.c_w_traj_flag,"%d",send_buffer->c_w_traj_flag);
	sprintf(outbuf.c_r_traj_flag,"%d",send_buffer->c_r_traj_flag);
	sprintf(outbuf.c_direct_flag,"%d",send_buffer->c_direct_flag);
	sprintf(outbuf.c_cpuutilflag,"%d",send_buffer->c_cpuutilflag);
	sprintf(outbuf.c_seq_mix,"%d",send_buffer->c_seq_mix);
	sprintf(outbuf.c_del_flag,"%d",send_buffer->c_del_flag);
	sprintf(outbuf.c_client_number,"%d",send_buffer->c_client_number);
	sprintf(outbuf.c_command,"%d",send_buffer->c_command);
	sprintf(outbuf.c_testnum,"%d",send_buffer->c_testnum);
	sprintf(outbuf.c_no_unlink,"%d",send_buffer->c_no_unlink);
	sprintf(outbuf.c_no_write,"%d",send_buffer->c_no_write);
	sprintf(outbuf.c_file_lock,"%d",send_buffer->c_file_lock);
	sprintf(outbuf.c_rec_lock,"%d",send_buffer->c_rec_lock);
	sprintf(outbuf.c_Kplus_readers,"%d",send_buffer->c_Kplus_readers);
	sprintf(outbuf.c_multiplier,"%d",send_buffer->c_multiplier);
	sprintf(outbuf.c_share_file,"%d",send_buffer->c_share_file);
	sprintf(outbuf.c_pattern,"%d",send_buffer->c_pattern);
	sprintf(outbuf.c_version,"%d",send_buffer->c_version);
	sprintf(outbuf.c_base_time,"%d",send_buffer->c_base_time);
	sprintf(outbuf.c_num_child,"%d",send_buffer->c_num_child);
	sprintf(outbuf.c_pct_read,"%d",send_buffer->c_pct_read);
	sprintf(outbuf.c_advise_op,"%d",send_buffer->c_advise_op);
	sprintf(outbuf.c_advise_flag,"%d",send_buffer->c_advise_flag);
	sprintf(outbuf.c_restf,"%d",send_buffer->c_restf);
	sprintf(outbuf.c_mygen,"%d",send_buffer->c_mygen);
#ifdef NO_PRINT_LLD
	sprintf(outbuf.c_stride,"%ld",send_buffer->c_stride);
	sprintf(outbuf.c_rest_val,"%ld",send_buffer->c_rest_val);
	sprintf(outbuf.c_delay,"%ld",send_buffer->c_delay);
	sprintf(outbuf.c_purge,"%ld",send_buffer->c_purge);
	sprintf(outbuf.c_fetchon,"%ld",send_buffer->c_fetchon);
	sprintf(outbuf.c_numrecs64,"%ld",send_buffer->c_numrecs64);
	sprintf(outbuf.c_reclen,"%ld",send_buffer->c_reclen);
	sprintf(outbuf.c_child_flag,"%ld",send_buffer->c_child_flag);
	sprintf(outbuf.c_delay_start,"%ld",send_buffer->c_delay_start);
	sprintf(outbuf.c_depth,"%ld",send_buffer->c_depth);
#else
	sprintf(outbuf.c_delay,"%lld",send_buffer->c_delay);
	sprintf(outbuf.c_stride,"%lld",send_buffer->c_stride);
	sprintf(outbuf.c_rest_val,"%lld",send_buffer->c_rest_val);
	sprintf(outbuf.c_purge,"%lld",send_buffer->c_purge);
	sprintf(outbuf.c_fetchon,"%lld",send_buffer->c_fetchon);
	sprintf(outbuf.c_numrecs64,"%lld",send_buffer->c_numrecs64);
	sprintf(outbuf.c_reclen,"%lld",send_buffer->c_reclen);
	sprintf(outbuf.c_child_flag,"%lld",send_buffer->c_child_flag);
	sprintf(outbuf.c_delay_start,"%lld",send_buffer->c_delay_start);
	sprintf(outbuf.c_depth,"%lld",send_buffer->c_depth);
#endif
	sprintf(outbuf.c_stop_flag,"%d",send_buffer->c_stop_flag);
	sprintf(outbuf.c_compute_time,"%f",send_buffer->c_compute_time);

	if(mdebug >= 1)
		printf("Master sending message to %s \n",host_name);
        /*rc = send(child_socket_val, (char *)&outbuf, sizeof(struct client_neutral_command), 0);*/
        rc = write(child_socket_val, (char *)&outbuf, sizeof(struct client_neutral_command));
        if (rc < 0)
        {
                perror("write failed\n");
                exit(26);
        }
}

/*
 * Close the childs listening port for messages from the master.
 */
#ifdef HAVE_ANSIC_C
void
stop_child_listen(int child_socket_val)
#else
void
stop_child_listen(child_socket_val)
int child_socket_val;
#endif
{
	close(child_socket_val);
}

/*
 * Close the childs channel for sending messages to the master.
 */
#ifdef HAVE_ANSIC_C
void
O_stop_child_send(int child_socket_val)
#else
void
O_stop_child_send(child_socket_val)
int child_socket_val;
#endif
{
	close(child_socket_val);
}

/*
 * Close the masters listening channel for all clients messages.
 */
#ifdef HAVE_ANSIC_C
void
stop_master_listen(int master_socket_val)
#else
void
stop_master_listen(master_socket_val)
int master_socket_val;
#endif
{
	if(mdebug)
	   printf("Stop master listen\n");
/*
	shutdown(master_socket_val,SHUT_RDWR);
*/
	close(master_socket_val);
	master_socket_val = 0;
}

/*
 * Close the masters send channel a particular child.
 */
#ifdef HAVE_ANSIC_C
void
stop_master_send(int child_socket_val)
#else
void
stop_master_send(child_socket_val)
int child_socket_val;
#endif
{
	close(child_socket_val);
}

/*
 * Start the childs listening service for messages from the master.
 */
#ifdef HAVE_ANSIC_C
int
start_child_listen(int size_of_message)
#else
int
start_child_listen(size_of_message)
int size_of_message;
#endif
{
	int tsize;
	int s;
	int rc;
	int xx;
	int tmp_port;
	int sockerr;
	int recv_buf_size=65536;
	int optval=1;
	struct linger dummy={1,0};
	xx = 0;
	tsize=size_of_message; /* Number of messages to receive */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
                perror("socket failed:");
                exit(19);
        }
	sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
		&recv_buf_size, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 3\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)
		&optval, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 4\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_LINGER, (char *)
		&dummy, sizeof(struct linger));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 4\n");
	}
        bzero(&child_sync_sock, sizeof(struct sockaddr_in));
	tmp_port=CHILD_LIST_PORT+chid;
        child_sync_sock.sin_port = htons(tmp_port);
        child_sync_sock.sin_family = AF_INET;
        child_sync_sock.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(s, (struct sockaddr *)&child_sync_sock,
                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	child_sync_sock.sin_port=htons(tmp_port);
			continue;
		}
        }
	child_port = ntohs(child_sync_sock.sin_port);
	if(cdebug ==1)
	{
		fprintf(newstdout,"Child %d: Listen: Bound at port %d\n",(int)chid, tmp_port);
		fflush(newstdout);
	}
	if(rc < 0)
	{
		fprintf(newstdout,"Child bind failed. Errno %d\n",errno);
		fflush(newstdout);
		exit(20);
	}
	return(s);
}
#ifdef HAVE_ANSIC_C
int
child_attach(int s, int flag)
#else
int
child_attach(s, flag)
int s,flag;
#endif
{
	unsigned int me;
	int ns;
	struct sockaddr_in *addr;
	if(flag)
	{
		addr=&child_async_sock;
		if(cdebug)
		{
			fprintf(newstdout,"Child %d attach async\n",(int)chid);
			fflush(newstdout);
		}
	}
	else
	{
		addr=&child_sync_sock;
		if(cdebug)
		{
			fprintf(newstdout,"Child %d attach sync\n",(int)chid);
			fflush(newstdout);
		}
	}
	me=sizeof(struct sockaddr_in);
	if(cdebug)
	{
		printf("Child %d enters listen\n",(int)chid);
		fflush(stdout);
	}
	listen(s,10);
	if(cdebug)
	{
		fprintf(newstdout,"Child %d enters accept\n",(int)chid);
		fflush(newstdout);
	}
	ns=accept(s,(void *)addr,&me);
	if(cdebug)
	{
		fprintf(newstdout,"Child %d attached for receive. Sock %d  %d\n",
		    (int)chid, ns,errno);
		fflush(newstdout);
	}
	return(ns);
}
	

/*
 * The clients use this to block waiting for a message from
 * the master.
 */
#ifdef HAVE_ANSIC_C
void
child_listen(int sock, int size_of_message)
#else
void
child_listen(sock, size_of_message)
int sock, size_of_message;
#endif
{
	int tsize;
	int rcvd;
	int s;
	int rc;
	char *cnc;

	cnc = (char *)&child_rcv_buf[0];
	bzero(cnc, sizeof(child_rcv_buf));
	s = sock;
	tsize=size_of_message; /* Number of messages to receive */
	rcvd = 0;
	while(rcvd < tsize)
	{
		if(cdebug ==1)
		{
			fprintf(newstdout,"Child %d In recieve \n",(int)chid);
			fflush(newstdout);
		}
		rc=read(s,cnc,size_of_message);
		if(rc < 0)
		{
			fprintf(newstdout,"Read failed. Errno %d \n",errno);
			fflush(newstdout);
			exit(21);
		}
		if(cdebug >= 1)
		{
			fprintf(newstdout,"Child %d: Got %d bytes\n",(int)chid, rc);
			fflush(newstdout);
		}
		rcvd+=rc;
		cnc+=rc;
	}
	if(cdebug >= 1)
	{
		fprintf(newstdout,"Child %d: return from listen\n",(int)chid);
		fflush(newstdout);
	}
}
/*
 * Start the childs async listening service for messages from the master.
 */
#ifdef HAVE_ANSIC_C
int
start_child_listen_async(int size_of_message)
#else
int
start_child_listen_async(size_of_message)
int size_of_message;
#endif
{
	int tsize;
	int s;
	int rc;
	int xx;
	int tmp_port;
	int sockerr;
	int recv_buf_size=65536;
	int optval=1;
	xx = 0;
	tsize=size_of_message; /* Number of messages to receive */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
                perror("socket failed:");
                exit(19);
        }
	sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
		&recv_buf_size, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 5\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)
		&optval, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 6\n");
	}
        bzero(&child_async_sock, sizeof(struct sockaddr_in));
	tmp_port=CHILD_ALIST_PORT;
        child_async_sock.sin_port = htons(tmp_port);
        child_async_sock.sin_family = AF_INET;
        child_async_sock.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(s, (struct sockaddr *)&child_async_sock,
                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	child_async_sock.sin_port=htons(tmp_port);
			continue;
		}
        }
	child_async_port = ntohs(child_async_sock.sin_port);
	if(cdebug ==1)
	{
		fprintf(newstdout,"Child %d: Async Listen: Bound at port %d\n",
			(int)chid,tmp_port);
		fflush(newstdout);
	}
	if(rc < 0)
	{
		fprintf(newstdout,"bind failed. Errno %d \n",errno);
		fflush(newstdout);
		exit(20);
	}
	return(s);
}
/*
 * The clients use this to block waiting for an async message from
 * the master.
 */
#ifdef HAVE_ANSIC_C
void
child_listen_async(int sock, int size_of_message)
#else
void
child_listen_async(sock, size_of_message)
int sock, size_of_message;
#endif
{
	int tsize;
	int rcvd;
	int s;
	int rc;
	char *cnc;

	cnc = &child_async_rcv_buf[0];
	s = sock;
	tsize=size_of_message; /* Number of messages to receive */
	rcvd = 0;
	while(rcvd < tsize)
	{
		if(cdebug ==1)
		{
			fprintf(newstdout,"Child %d In async recieve \n",(int)chid);
			fflush(newstdout);
		}
		rc=read(s,cnc,size_of_message);
		if(rc < 0)
		{
			fprintf(newstdout,"Read failed. Errno %d \n",errno);
			fflush(newstdout);
			exit(21);
		}
		/* Special case. If master gets final results, it can 
                   exit, and close the connection to the async child
                   too quickly. When this happens the child gets a 
		   read() that returns 0. It just needs to exit here.
		*/
		if(rc==0)
			exit(0);
		if(cdebug >= 1)
		{
			fprintf(newstdout,"Child %d: Got %d bytes (async) \n",(int)chid,rc);
			fflush(newstdout);
		}
		rcvd+=rc;
		cnc+=rc;
	}
	if(cdebug >= 1)
	{
		fprintf(newstdout,"Child %d: return from async listen\n",(int)chid);
		fflush(newstdout);
	}
}

/*
 * Start the channel for the master to send a message to 
 * a particular child on a particular port that the child
 * has created for the parent to use to communicate.
 */
#ifdef HAVE_ANSIC_C
int
start_master_send(char *child_host_name, int child_port, struct in_addr *my_s_addr)
#else
int
start_master_send(child_host_name, child_port, my_s_addr)
char *child_host_name; 
int child_port;
struct in_addr *my_s_addr;
#endif
{
	int rc,master_socket_val;
	struct sockaddr_in addr,raddr;
	struct hostent *he;
	int port,tmp_port;
	int ecount = 0;
	struct in_addr *ip;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;

        he = gethostbyname(child_host_name);
        if (he == NULL)
        {
                printf("Master: Bad hostname >%s<\n",child_host_name);
		fflush(stdout);
                exit(22);
        }
	if(mdebug ==1)
	{
	        printf("Master: start master send: %s\n", he->h_name);
		fflush(stdout);
	}
        ip = (struct in_addr *)he->h_addr_list[0];
#ifndef UWIN
	if(mdebug ==1)
	{
        	printf("Master: child name: %s\n", (char *)inet_ntoa(*ip));
        	printf("Master: child Port: %d\n", child_port);
		fflush(stdout);
	}
#endif

	port=child_port;
	my_s_addr->s_addr = ip->s_addr;
	/*port=CHILD_LIST_PORT;*/

        raddr.sin_family = AF_INET;
        raddr.sin_port = htons(port);
        raddr.sin_addr.s_addr = ip->s_addr;
        master_socket_val = socket(AF_INET, SOCK_STREAM, 0);
        if (master_socket_val < 0)
        {
                perror("Master: socket failed:");
                exit(23);
        }
        bzero(&addr, sizeof(struct sockaddr_in));
	tmp_port=HOST_ESEND_PORT;
        addr.sin_port = htons(tmp_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(master_socket_val, (struct sockaddr *)&addr,
                                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	addr.sin_port=htons(tmp_port);
			continue;
		}
        }
	if(mdebug ==1)
	{
		printf("Master: Bound port\n");
		fflush(stdout);
	}
        if (rc < 0)
        {
                perror("Master: bind failed for sync channel to child.\n");
                exit(24);
        }
	nanosleep(&req,&rem);
again:
        rc = connect(master_socket_val, (struct sockaddr *)&raddr, 
			sizeof(struct sockaddr_in));
	if (rc < 0)
        {
		if(ecount++ < 300)
		{
			nanosleep(&req,&rem);
			/*sleep(1);*/
			goto again;
		}
                perror("Master: connect failed\n");
		printf("Error %d\n",errno);
                exit(25);
        }
	if(mdebug ==1)
	{
		printf("Master Connected\n");
		fflush(stdout);
	}
	return (master_socket_val);
}
/*
 * Start the channel for the master to send a message to 
 * a particular child on a particular port that the child
 * has created for the parent to use to communicate.
 */
#ifdef HAVE_ANSIC_C
int
start_master_send_async(char *child_host_name, int child_port, struct in_addr my_s_addr)
#else
int
start_master_send_async(child_host_name, child_port, my_s_addr)
char *child_host_name; 
int child_port;
struct in_addr my_s_addr;
#endif
{
	int rc,master_socket_val;
	struct sockaddr_in addr,raddr;
	int port,tmp_port;
	int ecount = 0;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;


	port=child_port;
	nanosleep(&req,&rem);

over:
        raddr.sin_family = AF_INET;
        raddr.sin_port = htons(port);
        raddr.sin_addr.s_addr = my_s_addr.s_addr;
        master_socket_val = socket(AF_INET, SOCK_STREAM, 0);
        if (master_socket_val < 0)
        {
                perror("Master: async socket failed:");
                exit(23);
        }
        bzero(&addr, sizeof(struct sockaddr_in));
	tmp_port=HOST_ASEND_PORT;
        addr.sin_port = htons(tmp_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(master_socket_val, (struct sockaddr *)&addr,
                                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	addr.sin_port=htons(tmp_port);
			continue;
		}
        }
	if(mdebug ==1)
	{
		printf("Master: Bound async port\n");
		fflush(stdout);
	}
        if (rc < 0)
        {
                perror("Master: bind async failed\n");
                exit(24);
        }
again:

        rc = connect(master_socket_val, (struct sockaddr *)&raddr, 
			sizeof(struct sockaddr_in));
	if (rc < 0)
        {
		if(ecount++ < 300)
		{
		/* Really need this sleep for Windows */
#if defined (Windows)
			sleep(1);
#else
			nanosleep(&req,&rem);
#endif
			goto again;
		}
                perror("Master: async connect failed\n");
                close(master_socket_val);
#if defined (Windows)
		sleep(1);
#else
		nanosleep(&req,&rem);
#endif
                /*sleep(1);*/
                ecount=0;
                goto over;
        }
	if(mdebug ==1)
	{
		printf("Master async Connected\n");
		fflush(stdout);
	}
	return (master_socket_val);
}

/*
 * If not "distributed" then call fork. The "distributed"
 * will start iozone on a remote node.
 */
#ifdef HAVE_ANSIC_C
long long
start_child_proc(int testnum,long long numrecs64, long long reclen)
#else
long long
start_child_proc(testnum, numrecs64, reclen)
int testnum;
long long numrecs64, reclen;
#endif
{
	long long x;
	if(distributed && master_iozone)
	{
		x=(long long)pick_client(testnum,numrecs64, reclen);
	}
	else
	{
		x=(long long)fork();
	}
	if(mdebug)
		printf("Starting proc %d\n",(int)x);	
	return(x);
}	

/*
 * This function picks a client from the list of clients and
 * starts it running on the remote machine. It also waits for
 * the remote process to join and then sends the client
 * the state information it needs to begin to run the 
 * test. The client will initialize its state space, 
 * begin the test and block as the barrier waiting
 * for the master to say go.
 */
#ifdef HAVE_ANSIC_C
int
pick_client(int testnum,long long numrecs64, long long reclen)
#else
int
pick_client(testnum, numrecs64, reclen)
int testnum;
long long numrecs64, reclen;
#endif
{
	int x;
	int c_command,child_index;
	struct client_command cc;
	struct master_command mc;
	struct master_neutral_command *mnc;
	char command[512];
	struct in_addr my_s_addr;
	char my_port_num[12];


	bzero(&cc,sizeof(struct client_command));
	for(x=0;x<512;x++)
		command[x]=0;

	current_client_number++; /* Need to start with 1 */
	x=current_client_number;

	child_idents[x-1].state = C_STATE_ZERO;
	/* Step 1. Now start client going on remote node.	*/

	find_remote_shell(remote_shell);
	sprintf(command,"%s ",remote_shell);
	strcat(command,child_idents[x-1].child_name);
	strcat(command," -n '");
	strcat(command,child_idents[x-1].execute_path);
	strcat(command," -+s -t 1 -r 4 -s 4 -+c ");
	strcat(command,controlling_host_name);
        if (master_listen_port != HOST_LIST_PORT)
        {
          sprintf(my_port_num," -+i %d",master_listen_port);
          strcat(command,my_port_num);
        }
	strcat(command," '");
	usleep(100000); /* Don't spawn rsh too fast... */
	junk=system(command);
/*
	system("remsh rsnperf '/home/capps/niozone/iozone -+s -t 1 -r 4 -s 8 -+c rsnperf'");

*/
	if(mdebug)
		printf("%s",command);
	/* Format example: 					*/
	/*							*/
	/* system("remsh rsnperf '/home/capps/niozone/iozone 	*/
	/*       -+s -t 1 -r 4 -s 8 -+c rsnperf'");		*/
	/*							*/

	/* Step 2. Wait for join from new client.		*/

	child_idents[x-1].state = C_STATE_WAIT_WHO;

	if(mdebug>=1)
		printf("\nMaster listening for child to send join message.\n");
	master_listen(master_listen_socket,sizeof(struct master_neutral_command));
	mnc = (struct master_neutral_command *)&master_rcv_buf[0];

	/* 
	 * Convert from string format back to internal representation
	 */
	sscanf(mnc->m_child_port,"%d",&mc.m_child_port);	
	sscanf(mnc->m_child_async_port,"%d",&mc.m_child_async_port);	
	sscanf(mnc->m_command,"%d",&mc.m_command);	
	sscanf(mnc->m_version,"%d",&mc.m_version);	
	if(mc.m_version != proto_version)
	{
		printf("Client > %s < is not running the same version of Iozone !! C%d M%d\n", child_idents[x-1].child_name, mc.m_version, proto_version);
	}

	c_port = mc.m_child_port; 
	a_port = mc.m_child_async_port; 
	c_command = mc.m_command;
	if(mdebug>=1)
	{
		printf("Master back from listen child Joined.\n");
		printf("Master: Command %d\n",c_command);
	}
	/* Step 3. Then start_master_send() for this client.	*/
	
	if(mdebug>=1)
		printf("Starting master send channel\n");
        master_send_sockets[x-1]= start_master_send(child_idents[x-1].child_name,c_port,
		&my_s_addr); 
	if(mdebug>=1)
		printf("Starting master send async channel\n");
        master_send_async_sockets[x-1]= start_master_send_async(child_idents[x-1].child_name,a_port,
		my_s_addr); 

	child_idents[x-1].master_socket_num = master_send_sockets[x-1];
	child_idents[x-1].master_async_socket_num = master_send_async_sockets[x-1];
	child_idents[x-1].child_number = x-1;
	child_idents[x-1].child_port = c_port;
	child_idents[x-1].child_async_port = a_port;

	/* 								*/
	/* Step 4. Send message to client telling him his name, number, */
	/*             rsize, fsize, and test to run.			*/
	strcpy(cc.c_host_name ,controlling_host_name);
	strcpy(cc.c_pit_hostname ,pit_hostname);
	strcpy(cc.c_pit_service ,pit_service);
	strcpy(cc.c_client_name ,child_idents[x-1].child_name);
	strcpy(cc.c_working_dir ,child_idents[x-1].workdir);
	strcpy(cc.c_file_name ,child_idents[x-1].file_name);
	strcpy(cc.c_write_traj_filename ,write_traj_filename);
	strcpy(cc.c_read_traj_filename ,read_traj_filename);
	cc.c_command = R_JOIN_ACK;
	cc.c_client_number = x-1;
	cc.c_testnum = testnum;
	cc.c_numrecs64 = numrecs64;
	cc.c_reclen = reclen;
	cc.c_oflag = oflag;
	cc.c_mfflag = mfflag;
	cc.c_unbuffered = unbuffered;
	cc.c_noretest = noretest;
	cc.c_notruncate = notruncate;
	cc.c_read_sync = read_sync;
	cc.c_jflag = jflag;
	cc.c_direct_flag = direct_flag;
	cc.c_cpuutilflag = cpuutilflag;
	cc.c_seq_mix = seq_mix;
	cc.c_del_flag = del_flag;
	cc.c_async_flag = async_flag;
	cc.c_k_flag = k_flag;
	cc.c_h_flag = h_flag;
	cc.c_mflag = mflag;
	cc.c_pflag = pflag;
	cc.c_stride_flag = stride_flag;
	cc.c_fetchon = fetchon;
	cc.c_verify = verify;
	cc.c_sverify = sverify;
	cc.c_odsync = odsync;
	cc.c_diag_v = diag_v;
	cc.c_dedup = dedup;
	cc.c_dedup_interior = dedup_interior;
	cc.c_dedup_compress = dedup_compress;
	cc.c_dedup_mseed = dedup_mseed;
	cc.c_chid_skew = chid_skew;
	cc.c_hist_summary = hist_summary;
	cc.c_op_rate = op_rate;
	cc.c_op_rate_flag = op_rate_flag;
	cc.c_file_lock = file_lock;
	cc.c_rec_lock = rlocking;
	cc.c_Kplus_readers = Kplus_readers;
	cc.c_multiplier = multiplier;
	cc.c_share_file = share_file;
	cc.c_pattern = pattern;
	cc.c_version = proto_version;
	cc.c_base_time = base_time;
	cc.c_num_child = (int)num_child;
	cc.c_pct_read = pct_read;
	cc.c_advise_op = advise_op;
	cc.c_advise_flag = advise_flag;
	cc.c_restf = restf;
	cc.c_mygen = mygen;
	cc.c_Q_flag = Q_flag;
	cc.c_inc_think = inc_think;
	cc.c_L_flag = L_flag;
	cc.c_xflag = xflag;
	cc.c_w_traj_flag = w_traj_flag;
	cc.c_r_traj_flag = r_traj_flag;
	cc.c_include_flush = include_flush;
	cc.c_OPS_flag = OPS_flag;
	cc.c_purge = purge;
	cc.c_mmapflag = mmapflag;
	cc.c_mmapasflag = mmapasflag;
	cc.c_mmapnsflag = mmapnsflag;
	cc.c_mmapssflag = mmapssflag;
	cc.c_no_copy_flag = no_copy_flag;
	cc.c_no_unlink = no_unlink;
	cc.c_no_write = no_write;
	cc.c_include_close = include_close;
	cc.c_disrupt_flag = disrupt_flag;
	cc.c_compute_flag = compute_flag;
	cc.c_delay = delay;
	cc.c_stride = stride;
	cc.c_rest_val = rest_val;
	cc.c_delay_start = delay_start;
	cc.c_compute_time = compute_time;
	cc.c_depth = depth;
	cc.c_MS_flag = MS_flag;
	cc.c_mmap_mix = mmap_mix;
	cc.c_Kplus_flag = Kplus_flag;


	if(mdebug)
		printf("Master sending client who he is\n");
	master_send(master_send_sockets[x-1],cc.c_client_name, &cc,sizeof(struct client_command));

	child_idents[x-1].state = C_STATE_WAIT_BARRIER;
	
	/* 								*/
	/* Step 5. Wait until you receive message that the chile is at  */
	/*             the barrier.  					*/
	if(mdebug>=1)
	   printf("Master listening for child to send at barrier message.\n");
	master_listen(master_listen_socket,sizeof(struct master_neutral_command));
	mnc = (struct master_neutral_command *)&master_rcv_buf[0];
	/*
	 * Convert from string back to arch specific 
	 */
	sscanf(mnc->m_client_number,"%d",&mc.m_client_number);	
#ifdef NO_PRINT_LLD
	sscanf(mnc->m_child_flag,"%ld",&mc.m_child_flag);	
#else
	sscanf(mnc->m_child_flag,"%lld",&mc.m_child_flag);	
#endif

	child_index = mc.m_client_number;
	child_stat = (struct child_stats *)&shmaddr[child_index];	
	child_stat->flag = (long long)(mc.m_child_flag);
	if(mdebug>=1)
	   printf("Master sees child %d at barrier message.\n",child_index);

	return(x); /* Tell code above that it is the parent returning */
}

/****************************************************************************************/
/* This is the code that the client will use when it 					*/
/* gets started via remote shell. It is activated by the -+c controller_name option.	*/
/*											*/
/* The steps to this process are:							*/
/* 1. Start client receive channel 							*/
/* 2. Start client send channel 							*/
/* 3. Send message to controller saying I'm joining. 					*/
/* 4. Go into a loop and get all instructions from 					*/
/* 5. Get state information from the master 						*/
/* 6. Change to the working directory 							*/
/* 7. Run the test 									*/
/* 8. Release the listen and send sockets to the master 				*/
/* 											*/
/****************************************************************************************/
#ifdef HAVE_ANSIC_C
void
become_client(void)
#else
void
become_client()
#endif
{
	int x,testnum;
	struct master_command mc;
	struct client_command cc;
	struct client_neutral_command *cnc;
	char client_name[100];
	char *workdir;

	bzero(&mc,sizeof(struct master_command));
	x=fork(); /* Become a daemon so that remote shell will return. */
	if(x != 0)
		exit(0);
	/*
 	 * I am the child 
	 */
	(void)gethostname(client_name,100);

	fflush(stdout);
	fflush(stderr);
	if(cdebug)
	{
		newstdin=freopen("/tmp/don_in","r+",stdin);
		newstdout=freopen("/tmp/don_out","a+",stdout);
		newstderr=freopen("/tmp/don_err","a+",stderr);
	}
	else
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}
	if(cdebug>=1)
	{
		fprintf(newstdout,"My name = %s, Controller's name = %s\n",client_name, controlling_host_name);
		fflush(newstdout);
	}

	/* 1. Start client receive channel 				*/

	l_sock = start_child_listen(sizeof(struct client_neutral_command));
	l_async_sock = start_child_listen_async(sizeof(struct client_neutral_command));

	/* 2. Send message to controller saying I'm joining. 		*/

	strcpy(mc.m_host_name,controlling_host_name);
	strcpy(mc.m_client_name,client_name);
	mc.m_child_port = child_port;
	mc.m_child_async_port = child_async_port;
	mc.m_command = R_CHILD_JOIN;
	mc.m_version = proto_version;
	
	if(cdebug)
	{
		fprintf(newstdout,"Child %s sends JOIN to master %s Host Port %d\n",
			client_name,controlling_host_name,controlling_host_port);
		fflush(newstdout);
	}
	child_send(controlling_host_name,(struct master_command *)&mc, sizeof(struct master_command));

	l_sock=child_attach(l_sock,0);
	l_async_sock=child_attach(l_async_sock,1);
	
	/* 4. Go into a loop and get all instructions from 		*/
        /*    the controlling process. 					*/

	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %s waiting for who am I\n",client_name);
		fflush(newstdout);
	}
	child_listen(l_sock,sizeof(struct client_neutral_command));
	cnc = (struct client_neutral_command *)&child_rcv_buf;
	bzero(&cc, sizeof(struct client_command));
	
	/* Convert from string format to arch format */
	sscanf(cnc->c_command,"%d",&cc.c_command);
	sscanf(cnc->c_client_name,"%s",cc.c_client_name);
	sscanf(cnc->c_client_number,"%d",&cc.c_client_number);
	sscanf(cnc->c_host_name,"%s",cc.c_host_name);
	sscanf(cnc->c_pit_hostname,"%s",cc.c_pit_hostname);

	if(cc.c_command == R_TERMINATE || cc.c_command==R_DEATH)
	{
		if(cdebug)
		{
			fprintf(newstdout,"Child %d received terminate on sync channel !!\n",(int)chid);
			fflush(newstdout);
		}
		exit(1);
	}
	
	if(cdebug)
	{
         fprintf(newstdout,"Child sees: \n Client name %s \n Client_num # %d \n Host_name %s\n"
			,cc.c_client_name,cc.c_client_number,cc.c_host_name);
	 fflush(newstdout);
	}

	/*
	 * Now import all of the values of the flags that the child on this 
	 * machine needs to be able to run the test requested.
	 */

	/* 5. Get state information from the master */

#ifdef NO_PRINT_LLD
	sscanf(cnc->c_numrecs64,"%ld",&cc.c_numrecs64);
	sscanf(cnc->c_reclen,"%ld",&cc.c_reclen);
	sscanf(cnc->c_fetchon,"%ld",&cc.c_fetchon);
	sscanf(cnc->c_purge,"%ld",&cc.c_purge);
	sscanf(cnc->c_delay,"%ld",&cc.c_delay);
	sscanf(cnc->c_stride,"%ld",&cc.c_stride);
	sscanf(cnc->c_rest_val,"%ld",&cc.c_rest_val);
	sscanf(cnc->c_delay_start,"%ld",&cc.c_delay_start);
	sscanf(cnc->c_depth,"%ld",&cc.c_depth);
#else
	sscanf(cnc->c_numrecs64,"%lld",&cc.c_numrecs64);
	sscanf(cnc->c_reclen,"%lld",&cc.c_reclen);
	sscanf(cnc->c_fetchon,"%lld",&cc.c_fetchon);
	sscanf(cnc->c_purge,"%lld",&cc.c_purge);
	sscanf(cnc->c_delay,"%lld",&cc.c_delay);
	sscanf(cnc->c_stride,"%lld",&cc.c_stride);
	sscanf(cnc->c_rest_val,"%lld",&cc.c_rest_val);
	sscanf(cnc->c_delay_start,"%lld",&cc.c_delay_start);
	sscanf(cnc->c_depth,"%lld",&cc.c_depth);
#endif
	sscanf(cnc->c_pit_hostname,"%s",cc.c_pit_hostname);
	sscanf(cnc->c_pit_service,"%s",cc.c_pit_service);
	sscanf(cnc->c_testnum,"%d",&cc.c_testnum);
	sscanf(cnc->c_client_number,"%d",&cc.c_client_number);
	sscanf(cnc->c_working_dir,"%s",cc.c_working_dir);
	sscanf(cnc->c_file_name,"%s",cc.c_file_name);
	sscanf(cnc->c_write_traj_filename,"%s",cc.c_write_traj_filename);
	sscanf(cnc->c_read_traj_filename,"%s",cc.c_read_traj_filename);
	sscanf(cnc->c_noretest,"%d",&cc.c_noretest);
	sscanf(cnc->c_notruncate,"%d",&cc.c_notruncate);
	sscanf(cnc->c_read_sync,"%d",&cc.c_read_sync);
	sscanf(cnc->c_jflag,"%d",&cc.c_jflag);
	sscanf(cnc->c_direct_flag,"%d",&cc.c_direct_flag);
	sscanf(cnc->c_cpuutilflag,"%d",&cc.c_cpuutilflag);
	sscanf(cnc->c_seq_mix,"%d",&cc.c_seq_mix);
	sscanf(cnc->c_del_flag,"%d",&cc.c_del_flag);
	sscanf(cnc->c_async_flag,"%d",&cc.c_async_flag);
	sscanf(cnc->c_k_flag,"%d",&cc.c_k_flag);
	sscanf(cnc->c_h_flag,"%d",&cc.c_h_flag);
	sscanf(cnc->c_mflag,"%d",&cc.c_mflag);
	sscanf(cnc->c_pflag,"%d",&cc.c_pflag);
	sscanf(cnc->c_stride_flag,"%d",&cc.c_stride_flag);
	sscanf(cnc->c_verify,"%d",&cc.c_verify);
	sscanf(cnc->c_sverify,"%d",&cc.c_sverify);
	sscanf(cnc->c_odsync,"%d",&cc.c_odsync);
	sscanf(cnc->c_diag_v,"%d",&cc.c_diag_v);
	sscanf(cnc->c_dedup,"%d",&cc.c_dedup);
	sscanf(cnc->c_dedup_interior,"%d",&cc.c_dedup_interior);
	sscanf(cnc->c_dedup_compress,"%d",&cc.c_dedup_compress);
	sscanf(cnc->c_dedup_mseed,"%d",&cc.c_dedup_mseed);
	sscanf(cnc->c_chid_skew,"%d",&cc.c_chid_skew);
	sscanf(cnc->c_hist_summary,"%d",&cc.c_hist_summary);
	sscanf(cnc->c_op_rate,"%d",&cc.c_op_rate);
	sscanf(cnc->c_op_rate_flag,"%d",&cc.c_op_rate_flag);
	sscanf(cnc->c_file_lock,"%d",&cc.c_file_lock);
	sscanf(cnc->c_rec_lock,"%d",&cc.c_rec_lock);
	sscanf(cnc->c_Kplus_readers,"%d",&cc.c_Kplus_readers);
	sscanf(cnc->c_multiplier,"%d",&cc.c_multiplier);
	sscanf(cnc->c_share_file,"%d",&cc.c_share_file);
	sscanf(cnc->c_pattern,"%d",&cc.c_pattern);
	sscanf(cnc->c_version,"%d",&cc.c_version);
	sscanf(cnc->c_base_time,"%d",&cc.c_base_time);
	sscanf(cnc->c_num_child,"%d",&cc.c_num_child);
	sscanf(cnc->c_pct_read,"%d",&cc.c_pct_read);
	sscanf(cnc->c_advise_op,"%d",&cc.c_advise_op);
	sscanf(cnc->c_advise_flag,"%d",&cc.c_advise_flag);
	sscanf(cnc->c_restf,"%d",&cc.c_restf);
	sscanf(cnc->c_mygen,"%d",&cc.c_mygen);
	sscanf(cnc->c_oflag,"%d",&cc.c_oflag);
	sscanf(cnc->c_mfflag,"%d",&cc.c_mfflag);
	sscanf(cnc->c_unbuffered,"%d",&cc.c_unbuffered);
	sscanf(cnc->c_Q_flag,"%d",&cc.c_Q_flag);
	sscanf(cnc->c_inc_think,"%d",&cc.c_inc_think);
	sscanf(cnc->c_L_flag,"%d",&cc.c_L_flag);
	sscanf(cnc->c_xflag,"%d",&cc.c_xflag);
	sscanf(cnc->c_include_flush,"%d",&cc.c_include_flush);
	sscanf(cnc->c_OPS_flag,"%d",&cc.c_OPS_flag);
	sscanf(cnc->c_mmapflag,"%d",&cc.c_mmapflag);
	sscanf(cnc->c_mmapasflag,"%d",&cc.c_mmapasflag);
	sscanf(cnc->c_mmapnsflag,"%d",&cc.c_mmapnsflag);
	sscanf(cnc->c_mmapssflag,"%d",&cc.c_mmapssflag);
	sscanf(cnc->c_no_copy_flag,"%d",&cc.c_no_copy_flag);
	sscanf(cnc->c_w_traj_flag,"%d",&cc.c_w_traj_flag);
	sscanf(cnc->c_r_traj_flag,"%d",&cc.c_r_traj_flag);
	sscanf(cnc->c_no_unlink,"%d",&cc.c_no_unlink);
	sscanf(cnc->c_no_write,"%d",&cc.c_no_write);
	sscanf(cnc->c_include_close,"%d",&cc.c_include_close);
	sscanf(cnc->c_disrupt_flag,"%d",&cc.c_disrupt_flag);
	sscanf(cnc->c_compute_flag,"%d",&cc.c_compute_flag);
	sscanf(cnc->c_MS_flag,"%d",&cc.c_MS_flag);
	sscanf(cnc->c_mmap_mix,"%d",&cc.c_mmap_mix);
	sscanf(cnc->c_Kplus_flag,"%d",&cc.c_Kplus_flag);
	sscanf(cnc->c_compute_time,"%f",&cc.c_compute_time);

	strcpy(write_traj_filename,cc.c_write_traj_filename);
	strcpy(read_traj_filename,cc.c_read_traj_filename);
	numrecs64 = cc.c_numrecs64;
	strcpy(pit_hostname,cc.c_pit_hostname);
	strcpy(pit_service,cc.c_pit_service);
	reclen = cc.c_reclen;
	testnum = cc.c_testnum;
	chid = cc.c_client_number;
	workdir=cc.c_working_dir;
	oflag = cc.c_oflag;
	/* Child's absolute filename to use is provided */
	mfflag = cc.c_mfflag;
	if(mfflag)
		strcpy(filearray[chid],cc.c_file_name);
	if(cdebug)
	{
		fprintf(newstdout,"File name given %s\n",cc.c_file_name);
		fflush(newstdout);
	}
	unbuffered = cc.c_unbuffered;
	noretest = cc.c_noretest;
	notruncate = cc.c_notruncate;
	read_sync = cc.c_read_sync;
	jflag = cc.c_jflag;
	direct_flag = cc.c_direct_flag;
	cpuutilflag = cc.c_cpuutilflag;
	seq_mix = cc.c_seq_mix;
	del_flag = cc.c_del_flag;
	async_flag = cc.c_async_flag;
	k_flag = cc.c_k_flag;
	h_flag = cc.c_h_flag;
	mflag = cc.c_mflag;
	pflag = cc.c_pflag;
	stride_flag = cc.c_stride_flag;
	fetchon = cc.c_fetchon;
	verify = cc.c_verify;
	diag_v = cc.c_diag_v;
	dedup = cc.c_dedup;
	dedup_interior = cc.c_dedup_interior;
	dedup_compress = cc.c_dedup_compress;
	dedup_mseed = cc.c_dedup_mseed;
	chid_skew = cc.c_chid_skew;
	hist_summary = cc.c_hist_summary;
	op_rate = cc.c_op_rate;
	op_rate_flag = cc.c_op_rate_flag;
	if(diag_v)
		sverify = 0;
	else
		sverify = cc.c_sverify;
	file_lock = cc.c_file_lock;
	rlocking = cc.c_rec_lock;
	Kplus_readers = cc.c_Kplus_readers;
	multiplier = cc.c_multiplier;
	share_file = cc.c_share_file;
	pattern = cc.c_pattern;
	/* proto_version = cc.c_version; Don't copy it back. */
	base_time=cc.c_base_time;
	num_child=(long long)cc.c_num_child;
	pct_read=cc.c_pct_read;
	advise_op=cc.c_advise_op;
	advise_flag=cc.c_advise_flag;
	restf=cc.c_restf;
	mygen=cc.c_mygen;
	Q_flag = cc.c_Q_flag;
	inc_think = cc.c_inc_think;
	L_flag = cc.c_L_flag;
	xflag = cc.c_xflag;
	w_traj_flag = cc.c_w_traj_flag;
	r_traj_flag = cc.c_r_traj_flag;
	include_flush = cc.c_include_flush;
	OPS_flag = cc.c_OPS_flag;
	purge = cc.c_purge;
	mmapflag = cc.c_mmapflag;
	mmapasflag = cc.c_mmapasflag;
	mmapnsflag = cc.c_mmapnsflag;
	mmapssflag = cc.c_mmapssflag; 
	no_copy_flag = cc.c_no_copy_flag;
	no_unlink = cc.c_no_unlink;
	no_write = cc.c_no_write;
	include_close = cc.c_include_close;
	disrupt_flag = cc.c_disrupt_flag;
	compute_flag = cc.c_compute_flag;
	MS_flag = cc.c_MS_flag;
	mmap_mix = cc.c_mmap_mix;
	Kplus_flag = cc.c_Kplus_flag;
	delay = cc.c_delay;
	stride = cc.c_stride;
	rest_val = cc.c_rest_val;
	depth = cc.c_depth;
	delay_start = cc.c_delay_start;
	compute_time = cc.c_compute_time;
	if(cdebug)
	{
		fprintf(newstdout,"Child %d change directory to %s\n",(int)chid,workdir);
		fflush(newstdout);
	}
	if(purge)
		alloc_pbuf();

	/* 6. Change to the working directory */

	if(chdir(workdir)<0)
		client_error=errno;
	start_child_listen_loop(); /* The async channel listener */

	/* Need to start this after getting into the correct directory */
	if(w_traj_flag)
		w_traj_size();
	if(r_traj_flag)
		r_traj_size();

	get_resolution(); 		/* Get my clock resolution */
	if(hist_summary)
	{
		printf("Child got HISTORY flag\n");
	}

	/* 7. Run the test */
	switch(testnum) {

	case THREAD_WRITE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_write_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_write_test((long)0);
		break;
#ifdef HAVE_PREAD
	case THREAD_PWRITE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_pwrite_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_pwrite_test((long)0);
		break;
#endif
	case THREAD_REWRITE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_rewrite_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_rwrite_test((long)0);
		break;
	case THREAD_READ_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_read_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_read_test((long)0);
		break;
#ifdef HAVE_PREAD
	case THREAD_PREAD_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_read_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_pread_test((long)0);
		break;
#endif
	case THREAD_REREAD_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_reread_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_rread_test((long)0);
		break;
	case THREAD_STRIDE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_stride_read_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_stride_read_test((long)0);
		break;
	case THREAD_RANDOM_READ_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running random read test\n",(int)chid);
			fflush(newstdout);
		}
		thread_ranread_test((long)0);
		break;
	case THREAD_RANDOM_WRITE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running random write test\n",(int)chid);
			fflush(newstdout);
		}
		thread_ranwrite_test((long)0);
		break;
	case THREAD_REVERSE_READ_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running reverse read test\n",(int)chid);
			fflush(newstdout);
		}
		thread_reverse_read_test((long)0);
		break;
	case THREAD_RANDOM_MIX_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running mixed workload test\n",(int)chid);
			fflush(newstdout);
		}
		thread_mix_test((long)0);
		break;
	case THREAD_FWRITE_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_fwrite_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_fwrite_test((long)0);
		break;
	case THREAD_FREAD_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running thread_fread_test\n",(int)chid);
			fflush(newstdout);
		}
		thread_fread_test((long)0);
		break;
	case THREAD_CLEANUP_TEST : 
		if(cdebug>=1)
		{
			fprintf(newstdout,"Child %d running cleanup\n",(int)chid);
			fflush(newstdout);
		}
		thread_cleanup_test((long)0);
		break;
	};
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d finished running test.\n",(int)chid);
		fflush(newstdout);
	}
	
	/* 8. Release the listen and send sockets to the master */
	stop_child_listen(l_sock);

	exit(0);
}

/*
 * Clients tell the master their statistics, set the stopped flag, and set shared memory
 * child_flag to tell the master they are finished. Also each client report all statistics.
 */
#ifdef HAVE_ANSIC_C
void
tell_master_stats(testnum , chid, throughput, actual, 
		 cpu_time, wall_time, stop_flag, child_flag)
int testnum; 
long long chid; 
double throughput, actual, wall_time;
float cpu_time;
char stop_flag;
long long child_flag;
/*
void
tell_master_stats(int testnum , long long chid, double tthroughput, 
		double actual, float cpu_time, float wall_time, 
		char stop_flag, long long child_flag)
*/
#else
void
tell_master_stats(testnum , chid, throughput, actual, cpu_time, 
	wall_time, stop_flag, child_flag)
int testnum; 
long long chid; 
double throughput, actual, wall_time;
char stop_flag;
float cpu_time;
long long child_flag;
#endif
{
	struct master_command mc;
	bzero(&mc,sizeof(struct master_command));
	mc.m_client_number = (int) chid;
	mc.m_client_error = (int) client_error;
	mc.m_throughput= throughput;
	mc.m_testnum = testnum;
	mc.m_actual = actual;
	mc.m_cputime = cpu_time;
	mc.m_walltime = wall_time;
	mc.m_stop_flag = stop_flag;
	mc.m_child_flag = child_flag;
	mc.m_command = R_STAT_DATA;
	mc.m_mygen = mygen;
	mc.m_version = proto_version;
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d: Tell master stats and terminate\n",(int)chid);
		fflush(newstdout);
	}
	child_send(controlling_host_name,(struct master_command *)&mc, sizeof(struct master_command));
}
	
/*
 * Stop the master listener loop service.
 * Currently this is not used. The master_join_count
 * variable is used to terminate the loop service.
 */
#ifdef HAVE_ANSIC_C
void
stop_master_listen_loop(void)
#else
void
stop_master_listen_loop()
#endif
{
	if(mdebug>=1)
		printf("Stopping Master listen loop");
	kill(master_listen_pid,SIGKILL);
}


/*
 * Clients tell the master that I am at the barrier and ready
 * for the message to start work.
 */
#ifdef HAVE_ANSIC_C
void
tell_master_ready(long long chid)
#else
void
tell_master_ready(chid)
long long chid;
#endif
{
	struct master_command mc;
	bzero(&mc,sizeof(struct master_command));
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d: Tell master to go\n",(int)chid);
		fflush(newstdout);
	}
	mc.m_command = R_FLAG_DATA;
	mc.m_mygen = mygen;
	mc.m_version = proto_version;
	mc.m_child_flag = CHILD_STATE_READY; 
	mc.m_client_number = (int)chid; 
	mc.m_client_error = client_error;
	child_send(controlling_host_name,(struct master_command *)&mc, sizeof(struct master_command));
}

/*
 * Clients wait at a barrier for the master to tell them
 * to begin work. This is the function where they wait.
 */
#ifdef HAVE_ANSIC_C
void
wait_for_master_go(long long chid)
#else
void
wait_for_master_go(chid)
long long chid;
#endif
{
	struct client_neutral_command *cnc;
	struct client_command cc;
	bzero(&cc,sizeof(struct client_command));
	child_listen(l_sock,sizeof(struct client_neutral_command));
	cnc = (struct client_neutral_command *)child_rcv_buf;
	sscanf(cnc->c_command,"%d",&cc.c_command);
	if(cc.c_command == R_TERMINATE || cc.c_command==R_DEATH)
	{
		if(cdebug)
		{
			fprintf(newstdout,"Child %d received terminate on sync channel at barrier !!\n",(int)chid);
			fflush(newstdout);
		}
		exit(1);
	}
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d return from wait_for_master_go\n",(int)chid);
		fflush(newstdout);
	}
}

/*
 * Create a master listener for receiving data from the
 * many children. As the children finish they will send
 * their statistics and terminate. When the master_join_count 
 * goes to zero then it is time to stop this service.
 * When this service exits then the parent will know
 * that all of the children are done.
 */
#ifdef HAVE_ANSIC_C
void
start_master_listen_loop(int num)
#else
void
start_master_listen_loop(num)
int num;
#endif
{
	int i;
	struct child_stats *child_stat;
	struct master_neutral_command *mnc;
	struct master_command mc;
	int temp;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;


	master_join_count=num;
	master_listen_pid=fork();
	if(master_listen_pid!=0)
		return;
	if(mdebug>=1)
		printf("Starting Master listen loop m %d c %d count %d\n",master_iozone, 
			client_iozone,num);

	while(master_join_count)
	{
		master_listen(master_listen_socket,sizeof(struct master_neutral_command));
		mnc=(struct master_neutral_command *)&master_rcv_buf[0];
		
		/*
		 * Convert from string format to arch format
		 */
		sscanf(mnc->m_command,"%d",&mc.m_command);
		sscanf(mnc->m_client_number,"%d",&mc.m_client_number);
		sscanf(mnc->m_client_error,"%d",&mc.m_client_error);
		sscanf(mnc->m_mygen,"%d",&mc.m_mygen);
		sscanf(mnc->m_version,"%d",&mc.m_version);
		if(mc.m_version != proto_version)
		{
			printf("Client # %d is not running the same version of Iozone !\n",
				mc.m_client_number);
		}		
		if(mc.m_client_error != 0)
		{
			printf("\nClient # %d reporting an error %s !\n",
				mc.m_client_number,strerror(mc.m_client_error));
		}		
#ifdef NO_PRINT_LLD
		sscanf(mnc->m_child_flag,"%ld",&mc.m_child_flag);
#else
		sscanf(mnc->m_child_flag,"%lld",&mc.m_child_flag);
#endif
		sscanf(mnc->m_actual,"%f",&mc.m_actual);
		sscanf(mnc->m_throughput,"%f",&mc.m_throughput);
		sscanf(mnc->m_cputime,"%f",&mc.m_cputime);
		sscanf(mnc->m_walltime,"%f",&mc.m_walltime);
		sscanf(mnc->m_stop_flag,"%d",&temp);
		mc.m_stop_flag = temp;

		switch(mc.m_command) {
		case R_STAT_DATA:
			if(mc.m_mygen != mygen)
			{
				/* 
				 * >>> You are NOT one of my children !!!  <<<
				 * Probably children left behind from another run !!!
				 * Ignore their messages, and go on without them.
				 */
				printf("*** Unknown Iozone children responding !!! ***\n");
				continue;
			}
			i = mc.m_client_number;
			if(mdebug)
				printf("loop: R_STAT_DATA for client %d\n",i);
			child_stat = (struct child_stats *)&shmaddr[i];	
			child_stat->flag = mc.m_child_flag;
			child_stat->actual = mc.m_actual;
			child_stat->throughput = mc.m_throughput;
			child_stat->cputime = mc.m_cputime;
			child_stat->walltime = mc.m_walltime;
			*stop_flag = mc.m_stop_flag;
			master_join_count--;
			break;
		case R_FLAG_DATA:
			if(mc.m_mygen != mygen)
			{
				/* You are NOT one of my children !!! */
				printf("*** Unknown Iozone children responding !!! ***\n");
				continue;
			}
			if(mdebug)
				printf("loop: R_FLAG_DATA: Client %d flag %d \n",
				  (int)mc.m_client_number,
				  (int)mc.m_child_flag);
			i = mc.m_client_number;
			child_stat = (struct child_stats *)&shmaddr[i];	
			child_stat->flag = (long long)(mc.m_child_flag);
			break;
		case R_STOP_FLAG:
			if(mc.m_mygen != mygen)
			{
				/* You are NOT one of my children !!! */
				printf("*** Unknown Iozone children responding !!! ***\n");
				continue;
			}
			if(mdebug)
			  printf("Master loop: R_STOP_FLAG: Client %d STOP_FLAG \n",
				  (int)mc.m_client_number);
			*stop_flag=1;
			distribute_stop();
			break;
		}
			
	}
	/* Let the clients report results before exiting.
           Also, exiting too quickly can close the async
           socket to the child, and cause it to become ill.
           On Solaris, it gets stuck in a 0=read() loop. */

#if defined(Windows)
	sleep(1);
#else
	nanosleep(&req,&rem);
#endif

	exit(0);
}
/*
 * Create a client listener for receiving async data from the
 * the master. 
 */
#ifdef HAVE_ANSIC_C
void
start_child_listen_loop(void)
#else
void
start_child_listen_loop()
#endif
{
	int i;
	struct child_stats *child_stat;
	struct client_command cc;
	struct client_neutral_command *cnc;

	client_listen_pid=fork();
	if(client_listen_pid!=0)
		return;
	if(cdebug>=1)
	{
		fprintf(newstdout,"Child %d starting client listen loop\n",(int)chid);
		fflush(newstdout);
	}
	while(1)
	{
		bzero(&cc,sizeof(struct client_command));
		child_listen_async(l_async_sock,sizeof(struct client_neutral_command));
		cnc=(struct client_neutral_command *)&child_async_rcv_buf;
		/*
		 * Convert from string format to arch format
	 	 */
		sscanf(cnc->c_command,"%d",&cc.c_command);
		sscanf(cnc->c_client_number,"%d",&cc.c_client_number);
		sscanf(cnc->c_stop_flag,"%d",&cc.c_stop_flag);

		switch(cc.c_command) {
		case R_STOP_FLAG:
			i = cc.c_client_number;
			if(cdebug)
			{
				fprintf(newstdout,"child loop: R_STOP_FLAG for client %d\n",i);
				fflush(newstdout);
			}
			child_stat = (struct child_stats *)&shmaddr[i];	
			*stop_flag = cc.c_stop_flag; /* In shared memory with other copy */
			sent_stop=1;
			break;
		case R_TERMINATE:
			if(cdebug)
			{
				fprintf(newstdout,"Child loop: R_TERMINATE: Client %d \n",
				  (int)cc.c_client_number);
				fflush(newstdout);
			}
			sleep(2);
			/* Aync listener goes away */
			stop_child_listen(l_async_sock);
			exit(0);
		case R_DEATH:
			if(cdebug)
			{
				fprintf(newstdout,"Child loop: R_DEATH: Client %d \n",
				  (int)cc.c_client_number);
				fflush(newstdout);
			}
			i = cc.c_client_number;
			child_remove_files(i);
			sleep(2);
			/* Aync listener goes away */
			stop_child_listen(l_async_sock);
			exit(0);
		}
			
	}
}

/*
 * The controlling process "master" tells the children to begin.
 */

#ifdef HAVE_ANSIC_C
void
tell_children_begin(long long childnum)
#else
void
tell_children_begin(childnum)
long long childnum;
#endif
{
	struct client_command cc;
	int x;
	bzero(&cc,sizeof(struct client_command));
	x = (int) childnum;
	if(mdebug>=1)
		printf("Master: Tell child %d to begin\n",x);
	cc.c_command = R_FLAG_DATA;
	cc.c_child_flag = CHILD_STATE_BEGIN; 
	cc.c_client_number = (int)childnum; 
	master_send(master_send_sockets[x],child_idents[x].child_name, &cc,sizeof(struct client_command));
}

/*
 * The master waits here for all of the the children to terminate.
 * When the children are done the the master_join_count will be at zero
 * and the master_listen_loop will exit. This function waits for this to happen.
 */
#ifdef HAVE_ANSIC_C
void
wait_dist_join(void)
#else
void
wait_dist_join()
#endif
{
	wait(0);
	if(mdebug)
		printf("Master: All children have finished. Sending terminate\n");
	terminate_child_async(); /* All children are done, so terminate their async channel */
	current_client_number=0; /* start again */
}


/* 
 * This function reads a file that contains client information. 
 * The information is:
 * 	client name (DNS usable name)
 *	client working directory (where to run the test)
 * 	client directory that contains the Iozone executable.
 *
 * If the first character in a line is a # then it is a comment.
 * The maximum number of clients is MAXSTREAMS.
 */
#ifdef HAVE_ANSIC_C
int
get_client_info(void)
#else
int
get_client_info()
#endif
{
	FILE *fd;
	char *ret1;
	int count;
	char buffer[800];
	count=0;
	fd=fopen(client_filename,"r");
	if(fd == (FILE *)NULL)
	{
		printf("Unable to open client file \"%s\"\n",
			client_filename);
		exit(176);
	}
	while(1)
	{
          	if (count > MAXSTREAMS) {                                                                           
            	  printf("Too many lines in client file - max of %d supported\n",
			MAXSTREAMS);
            	  exit(7);
          	}
		ret1=fgets(buffer,800,fd);
		if(ret1== (char *)NULL)
			break;
		count+=parse_client_line(buffer,count);
	}
	fclose(fd);
	return(count);
}


/*
 * This function parses a line from the client file. It is
 * looking for:
 *	Client name  (DNS usable)
 *	Client working directory (where to run the test )
 *	Client path to Iozone executable.
 *
 * Lines that start with # are comments.
 */

#ifdef HAVE_ANSIC_C
int 
parse_client_line(char *buffer,int line_num)
#else
int
parse_client_line(buffer, line_num)
char *buffer;
int line_num;
#endif
{
	int num;
	/* Format is clientname, workdir, execute_path */
	/* If column #1 contains a # symbol then skip this line */

	if(buffer[0]=='#')
		return(0);
	num=sscanf(buffer,"%s %s %s %s\n",
		child_idents[line_num].child_name,
		child_idents[line_num].workdir,
		child_idents[line_num].execute_path,
		child_idents[line_num].file_name);
	if((num < 0) || ((num > 0) && (num !=3) && (num !=4)))
	{
		printf("Bad Client Identity at entry %d\n",line_num);
		printf("Client: -> %s  Workdir: -> %s  Execute_path: -> %s \n",
		child_idents[line_num].child_name,
		child_idents[line_num].workdir,
		child_idents[line_num].execute_path);
		exit(203);
	}
	if(num == 4)
		mfflag++;

	return(1);
}

/* 
 * This is a mechanism that the child uses to remove all
 * of its temporary files. Only used at terminate time.
 */
#ifdef HAVE_ANSIC_C
void
child_remove_files(int i)
#else
void
child_remove_files(i)
int i;
#endif
{

	char *dummyfile[MAXSTREAMS];           /* name of dummy file     */
	dummyfile[i]=(char *)malloc((size_t)MAXNAMESIZE);
	if(mfflag)
	{
		sprintf(dummyfile[i],"%s",filearray[i]);
	}
	else
	{
		sprintf(dummyfile[i],"%s.DUMMY.%d",filearray[i],i);
	}
	if(cdebug)
	{
		fprintf(newstdout,"Child %d remove: %s \n",(int)chid, dummyfile[i]);
		fflush(newstdout);
	}
	if(check_filename(dummyfile[i]))
		unlink(dummyfile[i]);
}

	
/*
 * The master tells the child async listener that it is time
 * to terminate its services.
 */
#ifdef HAVE_ANSIC_C
void
terminate_child_async(void)
#else
void
terminate_child_async()
#endif
{
	int i;
	struct client_command cc;
	bzero(&cc,sizeof(struct client_command));
	cc.c_command = R_TERMINATE;
	for(i=0;i<num_child;i++)
	{
		child_idents[i].state = C_STATE_ZERO;
		cc.c_client_number = (int)i; 
		if(mdebug)
			printf("Master terminating async channels to children.\n");
		master_send(master_send_async_sockets[i],child_idents[i].child_name, &cc,sizeof(struct client_command));
	}
}

/*
 * The master has received an update to the stop flag and is
 * now distributing this to all of the clients.
 */
#ifdef HAVE_ANSIC_C
void
distribute_stop(void)
#else
void
distribute_stop()
#endif
{
	int i;
	struct client_command cc;

	/*
	 * Only send one stop to the clients. Each client will
	 * send stop to the master, but the master only needs
	 * to distribute the first stop. Without this, the 
	 * master may distribute too many stops and overflow
	 * the socket buffer on the client.
	 */
	if(sent_stop)
	{
		if(mdebug)
		{
		  s_count++;
		  printf("Master not send stop %d\n",s_count);
		}
		return;
	}
	bzero(&cc,sizeof(struct client_command));
	cc.c_command = R_STOP_FLAG;
	cc.c_stop_flag = 1;
	for(i=0;i<num_child;i++)
	{
		cc.c_client_number = (int)i; 
		if(mdebug)
			printf("Master distributing stop flag to child %d\n",i);
		master_send(master_send_async_sockets[i],child_idents[i].child_name, &cc,sizeof(struct client_command));
	}
	sent_stop=1;
}

/*
 * Child is sending its stop flag to the master.
 */
#ifdef HAVE_ANSIC_C
void
send_stop(void)
#else
void
send_stop()
#endif
{
	struct master_command mc;

	bzero(&mc, sizeof(struct master_command));
	mc.m_command = R_STOP_FLAG;
	mc.m_mygen = mygen;
	mc.m_version = proto_version;
	mc.m_client_number = chid;
	mc.m_client_error = client_error;
	if(cdebug)
	{
		fprintf(newstdout,"Child %d sending stop flag to master\n",(int)chid);
		fflush(newstdout);
	}
        child_send(controlling_host_name,(struct master_command *)&mc, sizeof(struct master_command));
	client_error=0;  /* clear error, it has been delivered */
}

/*
 * This is very tricky stuff. There are points in time when 
 * someone can hit control-c and cause the master to want to die.
 * Ok..now how does the master contact all the clients and tell
 * them to stop ?  The clients may be in 3 different states.
 * Not started yet, Joined and waiting for the WHO information, 
 * or at the barrier.  If the client is not started... cool.
 * ignore it. If the client has joined and is waiting at WHO
 * then the client does not have an async listener yet. So 
 * the master only needs to tell the client (sync) channel
 * to terminate. If the client is at the barrier then the 
 * client has two processes. One at the barrier and another
 * that is providing the async services. So... the master
 * needs to terminate both of these processes.
 */
#ifdef HAVE_ANSIC_C
void
cleanup_children(void)
#else
void
cleanup_children()
#endif
{
	int i;
	struct client_command cc;
	bzero(&cc,sizeof(struct client_command));
	cc.c_command = R_DEATH;
	for(i=0;i<num_child;i++)
	{
		cc.c_client_number = (int)i; 
		/* Child not started yet */
		if(child_idents[i].state == C_STATE_ZERO)
			;
		/* Child is waiting for who info */
		if(child_idents[i].state == C_STATE_WAIT_WHO)
		{
			if(mdebug)
				printf("Master sending signaled death to child !!\n");
			master_send(master_send_sockets[i],child_idents[i].child_name, &cc,sizeof(struct client_command));
		}
		/* Child is waiting at the barrier */
		if(child_idents[i].state == C_STATE_WAIT_BARRIER)
		{
			if(mdebug)
				printf("Master sending signaled death to child !!\n");
			master_send(master_send_sockets[i],child_idents[i].child_name, &cc,sizeof(struct client_command));
			if(mdebug)
				printf("Master sending signaled death to child async !!\n");
			master_send(master_send_async_sockets[i],child_idents[i].child_name, &cc,sizeof(struct client_command));
		}
			
	}
}

/*
 * This closes the file descriptors that were created for the master send and async send
 * at the end of each phase of the throughput testing.
 */
#ifdef HAVE_ANSIC_C
void
cleanup_comm(void)
#else
void
cleanup_comm()
#endif
{
	int i;
	for(i=0;i<num_child;i++)
	{
        	close(master_send_sockets[i]);
        	close(master_send_async_sockets[i]);
	}
}

#ifdef HAVE_ANSIC_C
void
find_remote_shell(char *shell)
#else
void
find_remote_shell(shell)
char *shell;
#endif
{
	char *value;
	value=(char *)getenv("RSH");
	if(value)
	{
		strcpy(shell,value);
		return;
	}
#ifdef _HPUX_SOURCE
	strcpy(shell,"remsh");
#else
	strcpy(shell,"rsh");
#endif
	return;
}	
#ifdef HAVE_ANSIC_C
void
find_external_mon(char * imon_start, char * imon_stop)
#else
void
find_external_mon(imon_start,imon_stop)
char *imon_start,*imon_stop;
#endif
{
	char *start,*stop,*sync;
	imon_start[0]=(char)0;
	imon_stop[0]=(char)0;
	start=(char *)getenv("IMON_START");
	if(start)
	{
		strcpy(imon_start,start);
	}
	stop=(char *)getenv("IMON_STOP");
	if(stop)
	{
		strcpy(imon_stop,stop);
	}
	sync=(char *)getenv("IMON_SYNC");
	if(sync)
	{
		imon_sync=1;
	}

	return;
}	

/*
 * This test is only valid in throughput mode.
 */

#ifdef HAVE_ANSIC_C
void 
mix_perf_test(off64_t kilo64,long long reclen,long long *data1,long long *data2)
#else
void 
mix_perf_test(kilo64,reclen,data1,data2)
off64_t kilo64;
long long reclen;
long long *data1,*data2;
#endif
{
	return;
/*
	printf("\nMix mode test only valid in throughput mode.\n");
	signal_handler();
	exit(180);
*/
}

/*
 * Speed check code 
 */
char *sp_dest; /* Name of destination for messages */

int sp_child_listen_port = SP_CHILD_LISTEN_PORT;
int sp_child_esend_port  = SP_CHILD_ESEND_PORT;

int sp_master_listen_port = SP_MASTER_LISTEN_PORT;
int sp_master_esend_port  = SP_MASTER_ESEND_PORT;

int sp_master_results_port = SP_MASTER_RESULTS_PORT;

struct in_addr sp_my_cs_addr;
struct in_addr sp_my_ms_addr;
struct sockaddr_in sp_child_sync_sock, sp_child_async_sock;
struct sockaddr_in sp_master_sync_sock, sp_master_async_sock;
char *sp_buf;
char sp_command[1024];
char sp_remote_shell[100];
int sp_child_mode;
int sp_count,sp_msize,sp_once;
int sp_tcount;
double sp_start_time,sp_finish_time;
void sp_send_result(int, int, float );
void sp_get_result(int , int );
void sp_do_child_t(void);
void sp_do_master_t(void);
void speed_main(char *, char *, long long ,long long , int);
int sp_cret;
char sp_remote_host[256];
char sp_master_host[256];
char sp_location[256];


/*
 * This is the front end for the speed check code
 */
#ifdef HAVE_ANSIC_C
void
speed_main(char *client_name, char *e_path, long long reclen,
	long long kilos, int client_flag)
#else
void
speed_main(client_name, e_path, reclen, kilos, client_flag)
char *client_name;
char *e_path;
long long reclen;
long long kilos;
int client_flag;
#endif
{
	int x;


	strcpy(sp_master_host,controlling_host_name);
	sp_msize=(int)reclen;
	sp_count=((int)kilos*1024)/(int)reclen;
	if(!client_flag)
	{
		printf("\n");
		strcpy(sp_remote_host,client_name);
		strcpy(sp_location,e_path);
	}

	if(client_flag)
		sp_child_mode=1;
	sp_buf=(char *)malloc(sp_msize);
	bzero(sp_buf,sp_msize); /* get page faults out of the way */

	if(sp_child_mode)
	{
		close(0);
		close(1);
		close(2);
		if(cdebug)
		{
			newstdin=freopen("/tmp/don_in","r+",stdin);
			newstdout=freopen("/tmp/don_out","a+",stdout);
			newstderr=freopen("/tmp/don_err","a+",stderr);
		}
		sp_dest=sp_master_host;
		sp_do_child_t();
		free(sp_buf);
		exit(0);
	}
	x=fork();
	if(x==0)
	{
		find_remote_shell(sp_remote_shell);
	 sprintf(sp_command,"%s %s %s -+s -t 1 -r %d -s %d -+c %s -+t ",
			sp_remote_shell, sp_remote_host, 
			sp_location, (int)reclen/1024, 
			(int)kilos,sp_master_host);
		/*printf("%s\n",sp_command);*/
		junk=system(sp_command);
		exit(0);
	}
	else
	{
		if(!sp_once)
		{
		printf("***************************************************\n");
		printf("* >>>>>     Client Network Speed check      <<<<< *\n");
		printf("***************************************************\n\n");
			printf("Master: %s\n",sp_master_host);
			printf("Transfer size %d bytes  \n",sp_msize);
			printf("Count %d\n",sp_count);
			printf("Total size %d kbytes  \n\n",
				(sp_msize*sp_count)/1024);
			sp_once=1;
		}
		sp_dest=sp_remote_host;
		sleep(1);
		sp_do_master_t();
		free(sp_buf);
	}
}

/*
 * Get results back from the client.
 */
#ifdef HAVE_ANSIC_C
void
sp_get_result(int port,int flag)
#else
void
sp_get_result(port,flag)
int port,flag;
#endif
{
	int tcfd;
	float throughput;
	int count;
	char mybuf[1024];
	int sp_offset,xx;

	tcfd=sp_start_master_listen(port, 1024);
	sp_offset=0;
	while(sp_offset < 1024)
	{
		xx=read(tcfd,&mybuf[sp_offset],1024);
		sp_offset+=xx;
	}
	sscanf(mybuf,"%d %f",&count,&throughput);
	if(!flag)
		printf("%-20s received  %10d kBytes @ %10.2f kBytes/sec \n",
			sp_remote_host,count,throughput);
	else
		printf("%-20s  sent     %10d kBytes @ %10.2f kBytes/sec \n",
			sp_remote_host,count,throughput);
	close(tcfd);
}

/*
 * Send results to the master.
 */
#ifdef HAVE_ANSIC_C
void
sp_send_result(int port, int count, float throughput)
#else
void
sp_send_result(port, count, throughput)
int port,count; 
float throughput;
#endif
{
	int msfd;
	char mybuf[1024];
	sprintf(mybuf,"%d %f",count, throughput);
	msfd=sp_start_child_send(sp_dest, port, &sp_my_cs_addr);
	junk=write(msfd,mybuf,1024);
	if(cdebug)
	{
		fprintf(newstdout,"Sending result\n");
		fflush(newstdout);
	}
	close(msfd);
}

/*
 * Start the channel for the master to send a message to 
 * a child on a port that the child
 * has created for the parent to use to communicate.
 */
#ifdef HAVE_ANSIC_C
int
sp_start_master_send(char *sp_child_host_name, int sp_child_listen_port, struct in_addr *sp_my_ms_addr)
#else
int
sp_start_master_send(sp_child_host_name, sp_child_listen_port, sp_my_ms_addr)
char *sp_child_host_name; 
int sp_child_listen_port;
struct in_addr *sp_my_ms_addr;
#endif
{
	int rc,master_socket_val;
	struct sockaddr_in addr,raddr;
	struct hostent *he;
	int port,tmp_port;
	int ecount=0;
	struct in_addr *ip;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;

        he = gethostbyname(sp_child_host_name);
        if (he == NULL)
        {
                printf("Master: Bad hostname >%s<\n",sp_child_host_name);
		fflush(stdout);
                exit(22);
        }
	if(mdebug ==1)
	{
	        printf("Master: start master send: %s\n", he->h_name);
		fflush(stdout);
	}
        ip = (struct in_addr *)he->h_addr_list[0];
#ifndef UWIN
	if(mdebug ==1)
	{
        	printf("Master: child name: %s\n", (char *)inet_ntoa(*ip));
        	printf("Master: child Port: %d\n", sp_child_listen_port);
		fflush(stdout);
	}
#endif

	port=sp_child_listen_port;
	sp_my_ms_addr->s_addr = ip->s_addr;
	/*port=CHILD_LIST_PORT;*/

        raddr.sin_family = AF_INET;
        raddr.sin_port = htons(port);
        raddr.sin_addr.s_addr = ip->s_addr;
        master_socket_val = socket(AF_INET, SOCK_STREAM, 0);
        if (master_socket_val < 0)
        {
                perror("Master: socket failed:");
                exit(23);
        }
        bzero(&addr, sizeof(struct sockaddr_in));
	tmp_port=sp_master_esend_port;
        addr.sin_port = htons(tmp_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(master_socket_val, (struct sockaddr *)&addr,
                                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	addr.sin_port=htons(tmp_port);
			continue;
		}
        }
	if(mdebug ==1)
	{
		printf("Master: Bound port\n");
		fflush(stdout);
	}
        if (rc < 0)
        {
                perror("Master: bind failed for sync channel to child.\n");
                exit(24);
        }
#if defined(Windows)
	sleep(1);
#else
	nanosleep(&req,&rem);
#endif

again:
        rc = connect(master_socket_val, (struct sockaddr *)&raddr, 
			sizeof(struct sockaddr_in));
	if (rc < 0)
        {
		if(ecount++ < 300)
		{
#if defined(Windows)
			sleep(1);
#else
			nanosleep(&req,&rem);
#endif
			/*sleep(1);*/
			goto again;
		}
                perror("Master: connect failed\n");
		printf("Error %d\n",errno);
                exit(25);
        }
	if(mdebug ==1)
	{
		printf("Master Connected\n");
		fflush(stdout);
	}
	return (master_socket_val);
}

/*
 * Start the childs listening service for messages from the master.
 */
#ifdef HAVE_ANSIC_C
int
sp_start_child_listen(int listen_port, int size_of_message)
#else
int
sp_start_child_listen(listen_port, size_of_message)
int listen_port;
int size_of_message;
#endif
{
	int tsize;
	int s,ns;
	unsigned int me;
	int rc;
	int xx;
	int tmp_port;
	struct sockaddr_in *addr;
	int sockerr;
	int recv_buf_size=65536;
	int optval=1;
	xx = 0;
	me=sizeof(struct sockaddr_in);
	tsize=size_of_message; /* Number of messages to receive */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
                perror("socket failed:");
                exit(19);
        }
	sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
		&recv_buf_size, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 7\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)
		&optval, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 8\n");
	}
        bzero(&sp_child_sync_sock, sizeof(struct sockaddr_in));
	tmp_port=sp_child_listen_port;
        sp_child_sync_sock.sin_port = htons(tmp_port);
        sp_child_sync_sock.sin_family = AF_INET;
        sp_child_sync_sock.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(s, (struct sockaddr *)&sp_child_sync_sock,
                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	sp_child_sync_sock.sin_port=htons(tmp_port);
			continue;
		}
        }
	sp_child_listen_port = ntohs(sp_child_sync_sock.sin_port);
	if(cdebug ==1)
	{
		fprintf(newstdout,"Child: Listen: Bound at port %d\n", tmp_port);
		fflush(newstdout);
	}
	if(rc < 0)
	{
		if(cdebug ==1)
		{
		   fprintf(newstdout,"bind failed. Errno %d\n", errno);
		   fflush(newstdout);
		}
		exit(20);
	}

	addr=&sp_child_async_sock;
	listen(s,10);
	if(cdebug)
	{
		fprintf(newstdout,"Child enters accept\n");
		fflush(newstdout);
	}
	ns=accept(s,(void *)addr,&me);
	if(cdebug)
	{
		fprintf(newstdout,"Child attached for receive. Sock %d  %d\n", ns,errno);
		fflush(newstdout);
	}
	close(s);
	return(ns);
}


/*
 * The client runs this code
 */
#ifdef HAVE_ANSIC_C
void
sp_do_child_t(void)
#else
void
sp_do_child_t()
#endif
{
	int i,y;
	int offset;
	int sp_tcount=0;
	/* child */
	/*
	 * Child reads from master 
	 */
	sp_crfd=sp_start_child_listen(sp_child_listen_port, sp_msize);
	sp_start_time=time_so_far();
	for(i=0;i<sp_count;i++)
	{
		offset=0;
		while(offset<sp_msize)
		{
			y=read(sp_crfd,&sp_buf[offset],sp_msize-offset);
			if(y < 0)
			{
			      	if(cdebug)
				{
					fprintf(newstdout,"Child error %d offset %d\n",
						errno,offset);
					fflush(newstdout);
				}
				exit(1);
			}
			offset+=y;
			if(cdebug)
			{
				fprintf(newstdout,"Child offset %d read %d\n",offset,y);
				fflush(newstdout);
			}
		}
		sp_tcount+=offset;
	}
	sp_finish_time=time_so_far();

	close(sp_crfd);
	sleep(1); /* Wait for master to get into sp_get_result */
	sp_send_result(sp_master_results_port, sp_tcount/1024, 
		(float)(sp_tcount/1024)/(sp_finish_time-sp_start_time));

	sleep(1);
	/*
	 * Child writes to master 
	 */
	sp_csfd=sp_start_child_send(sp_dest, sp_master_listen_port,
		&sp_my_cs_addr);
	sp_tcount=0;
	offset=0;
	sp_start_time=time_so_far();
	for(i=0;i<sp_count;i++)
	{
		y=write(sp_csfd,sp_buf,sp_msize);
		sp_tcount+=y;
	}
	sp_finish_time=time_so_far();
	close(sp_csfd);
	sleep(1);
	sp_send_result(sp_master_results_port, sp_tcount/1024, 
		(float)(sp_tcount/1024)/(sp_finish_time-sp_start_time));
	if(cdebug)
	{
		fprintf(newstdout,"child exits\n");
		fflush(newstdout);
	}
}

/*
 * The master runs this code.
 */
#ifdef HAVE_ANSIC_C
void
sp_do_master_t(void)
#else
void
sp_do_master_t()
#endif
{
	int i,y,sp_offset;
	int sp_tcount = 0;


	/*
	 * Master writes to child 
	 */
	sp_msfd=sp_start_master_send(sp_dest, sp_child_listen_port,
		&sp_my_ms_addr);
	sp_start_time=time_so_far();
	for(i=0;i<sp_count;i++)
	{
		y=write(sp_msfd,sp_buf,sp_msize);
		sp_tcount+=y;
	}
	sp_finish_time=time_so_far();
	close(sp_msfd);
	sp_msfd=0;
	sp_get_result(sp_master_results_port,0);
	printf("%-20s  sent     %10d kbytes @ %10.2f kBytes/sec \n",
		sp_master_host,sp_tcount/1024, 
		(float)(sp_tcount/1024)/(sp_finish_time-sp_start_time));

	/* printf("\n"); */
	/*
	 * Master reads from child 
	 */
	sp_mrfd=sp_start_master_listen(sp_master_listen_port, sp_msize);
	sp_offset=0;
	sp_start_time=time_so_far();
	sp_tcount=0;
	for(i=0;i<sp_count;i++)
	{
		sp_offset=0;
		while(sp_offset<sp_msize)
		{
			y=read(sp_mrfd,&sp_buf[sp_offset],sp_msize-sp_offset);
			if(y < 0)
			{
			      	printf("Master error %d offset %d\n",errno,
					sp_offset);
				exit(1);
			}
			sp_offset+=y;
			/* printf("Master offset %d read %d\n",offset,y);*/
		}
		sp_tcount+=sp_offset;
	}
	sp_finish_time=time_so_far();
	sp_get_result(sp_master_results_port,1);
	printf("%-20s received  %10d kbytes @ %10.2f kBytes/sec \n",
		sp_master_host,sp_tcount/1024,
		(float)(sp_tcount/1024)/(sp_finish_time-sp_start_time));
	printf("\n");
	wait(NULL);
	close(sp_mrfd);
	sp_mrfd=0;
}

/*
 * Start the master listening service for messages from the child.
 */
#ifdef HAVE_ANSIC_C
int
sp_start_master_listen(int sp_master_listen_port, int sp_size_of_message)
#else
int
sp_start_master_listen(sp_master_listen_port, sp_size_of_message)
int sp_size_of_message;
int sp_master_listen_port;
#endif
{
	int tsize;
	int s,ns;
	unsigned int me;
	int rc;
	int xx;
	int tmp_port;
	struct sockaddr_in *addr;
	int sockerr;
	int recv_buf_size=65536;
	int optval=1;
	xx = 0;
	me=sizeof(struct sockaddr_in);
	tsize=sp_size_of_message; /* Number of messages to receive */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        {
                perror("socket failed:");
                exit(19);
        }
	sockerr = setsockopt (s, SOL_SOCKET, SO_RCVBUF, (char *)
		&recv_buf_size, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 9\n");
	}
	sockerr = setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)
		&optval, sizeof(int));
	if ( sockerr == -1 ) {
		perror("Error in setsockopt 10\n");
	}
        bzero(&sp_master_sync_sock, sizeof(struct sockaddr_in));
	tmp_port=sp_master_listen_port;
        sp_master_sync_sock.sin_port = htons(tmp_port);
        sp_master_sync_sock.sin_family = AF_INET;
        sp_master_sync_sock.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(s, (struct sockaddr *)&sp_master_sync_sock,
                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	sp_master_sync_sock.sin_port=htons(tmp_port);
			continue;
		}
        }
	sp_master_listen_port = ntohs(sp_master_sync_sock.sin_port);
	if(mdebug ==1)
	{
		printf("Master: Listen: Bound at port %d\n", tmp_port);
		fflush(stdout);
	}
	if(rc < 0)
	{
		perror("bind failed\n");
		exit(20);
	}

	addr=&sp_master_async_sock;
	listen(s,10);
	if(mdebug)
	{
		printf("Master enters accept\n");
		fflush(stdout);
	}
	ns=accept(s,(void *)addr,&me);
	if(mdebug)
	{
		printf("Master attached for receive. Sock %d  %d\n", ns,errno);
		fflush(stdout);
	}
	close(s);
	return(ns);
}

/*
 * Start the channel for the child to send a message to 
 * the master.
 */
#ifdef HAVE_ANSIC_C
int
sp_start_child_send(char *sp_master_host_name, int sp_master_listen_port, struct in_addr *sp_my_cs_addr)
#else
int
sp_start_child_send(sp_master_host_name, sp_master_listen_port, sp_my_cs_addr)
char *sp_master_host_name; 
int sp_master_listen_port;
struct in_addr *sp_my_cs_addr;
#endif
{
	int rc,sp_child_socket_val;
	struct sockaddr_in addr,raddr;
	struct hostent *he;
	int port,tmp_port;
	struct in_addr *ip;
	int ecount=0;
	struct timespec req,rem;

	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rem.tv_sec = 0;
	rem.tv_nsec = 10000000;

        he = gethostbyname(sp_master_host_name);
        if (he == NULL)
        {
                printf("Child: Bad hostname >%s<\n",sp_master_host_name);
		fflush(stdout);
                exit(22);
        }
	if(cdebug ==1)
	{
	        fprintf(newstdout,"Child: start child send: %s\n", he->h_name);
	        fprintf(newstdout,"To: %s at port %d\n",sp_master_host_name,
			sp_master_listen_port);
		fflush(newstdout);
	}
        ip = (struct in_addr *)he->h_addr_list[0];

	port=sp_master_listen_port;
	sp_my_cs_addr->s_addr = ip->s_addr;

        raddr.sin_family = AF_INET;
        raddr.sin_port = htons(port);
        raddr.sin_addr.s_addr = ip->s_addr;
        sp_child_socket_val = socket(AF_INET, SOCK_STREAM, 0);
        if (sp_child_socket_val < 0)
        {
                perror("child: socket failed:");
                exit(23);
        }
        bzero(&addr, sizeof(struct sockaddr_in));
	tmp_port=sp_child_esend_port;
        addr.sin_port = htons(tmp_port);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        rc = -1;
        while (rc < 0)
        {
                rc = bind(sp_child_socket_val, (struct sockaddr *)&addr,
                                                sizeof(struct sockaddr_in));
		if(rc < 0)
		{
			tmp_port++;
                	addr.sin_port=htons(tmp_port);
			continue;
		}
        }
	if(cdebug ==1)
	{
		fprintf(newstdout,"Child: Bound port %d\n",tmp_port);
		fflush(newstdout);
	}
        if (rc < 0)
        {
                perror("Child: bind failed for sync channel to child.\n");
                exit(24);
        }
#if defined(Windows)
	sleep(1);
#else
	nanosleep(&req,&rem);
#endif
again:
        rc = connect(sp_child_socket_val, (struct sockaddr *)&raddr, 
			sizeof(struct sockaddr_in));
	if (rc < 0)
        {
		if(ecount++<300)
		{
#if defined(Windows)
			sleep(1);
#else
			nanosleep(&req,&rem);
#endif
			goto again;
		}

		fprintf(newstdout,"child: connect failed. Errno %d \n",errno);
		fflush(newstdout);
		exit(25);
        }
	if(cdebug ==1)
	{
		fprintf(newstdout,"child Connected\n");
		fflush(newstdout);
	}
	return (sp_child_socket_val);
}

#ifdef HAVE_ANSIC_C
void
do_speed_check(int client_flag)
#else
void
do_speed_check(client_flag)
int client_flag;
#endif
{
	int i;
	if(client_flag)
	{
		speed_main(" "," ",reclen,kilobytes64,client_flag);
	}
	else
	{
		printf("Checking %d clients\n",clients_found);
		for(i=0;i<clients_found;i++)
		{
			speed_main(child_idents[i].child_name,
				child_idents[i].execute_path,
				reclen, kilobytes64,client_flag);
		}
	}
}

#ifdef HAVE_ANSIC_C
void
get_date(char *where)
#else
get_date(where)
char *where;
#endif
{
	time_t t;
	char *value;
	t=time(0);
	value=(char *)ctime(&t);
	strcpy(where,value);
}

/* Richard Sharpe decided to hack up Samba and
 * have it detect Iozone running, and then
 * produce the data without doing any actual
 * I/O. This was a HIGHLY questionable thing to
 * be doing (my opinion). It may have been a lab
 * experiment that was accidentally released into 
 * the wild, but now that it is, no choice but
 * to prevent its use. So... the pattern
 * that he was locking on to, is now random,
 * and will change with every release of Iozone.
 * See: http://lists.samba.org/archive/samba-technical/2005-April/040541.html
 */

#ifdef HAVE_ANSIC_C
int
get_pattern(void)
#else
get_pattern(void)
#endif
{
        int i,x,y;
        char cp[100],*ptr;
        int pat;
	unsigned char inp_pat;
	unsigned int temp;

        y=0;
        ptr=&cp[0];
        strcpy(cp,THISVERSION);
        x=strlen(THISVERSION);
        for(i=0;i<x;i++)
                y+=*ptr++;
        srand(y);
        pat=(rand()& 0xff);
	/* For compatibility with old 0xa5 data sets. */
	if(Z_flag)
		pat=0xa5;
	/* Lock pattern to 0xBB, for filesystem short circuit debug */
	if(X_flag)
		pat=PATTERN1;
	/* Set global pattern */
	inp_pat = pat;
	temp =((inp_pat << 24) | (inp_pat << 16) | (inp_pat << 8) | inp_pat);
        return(pat);
}

/* 
 * Allocate the buffer for purge. 
*/
#ifdef HAVE_ANSIC_C
void
alloc_pbuf(void)
#else
alloc_pbuf(void)
#endif
{
	pbuffer = (char *) alloc_mem((long long)(3 * cache_size),(int)0);
	if(pbuffer == 0) {
              	perror("Memory allocation failed:");
               	exit(9);
	}
#ifdef _64BIT_ARCH_
	pbuffer = (char *) 
		(((unsigned long long)pbuffer + cache_size ) 
		& ~(cache_size-1));
#else
	pbuffer = (char *) 
		(((long)pbuffer + (long)cache_size ) 
		& ~((long)cache_size-1));
#endif
}

/* 
 * Check to see if the file descriptor points at a file
 * or a device.
 */
#ifdef HAVE_ANSIC_C
int
check_filename(char *name)
#else
check_filename(name)
char *name;
#endif
{
#ifdef _LARGEFILE64_SOURCE
	struct stat64 mystat;
#else
	struct stat mystat;
#endif
	int x;
	if(strlen(name)==0)
		return(0);
	/* Lets try stat first.. may get an error if file is too big */
	x=I_STAT(name,&mystat);
	if(x<0)
	{
		return(0);
		/* printf("Stat failed %d\n",x); */
	}
	if(mystat.st_mode & S_IFREG)
	{
		/*printf("Is a regular file\n");*/
		return(1);
	}
	return(0);
}

#ifdef HAVE_ANSIC_C
void
start_monitor(char *test)
#else
start_monitor(test)
char *test;
#endif
{
	char command_line[256];
	if(strlen(imon_start)!=0)
	{
		if(imon_sync)
		   sprintf(command_line,"%s %s",imon_start,test);
		else
		   sprintf(command_line,"%s %s&",imon_start,test);
		junk=system(command_line);
	}
}
#ifdef HAVE_ANSIC_C
void
stop_monitor(char *test)
#else
stop_monitor(test)
char *test;
#endif
{
	char command_line[256];
	if(strlen(imon_stop)!=0)
	{
		if(imon_sync)
		   sprintf(command_line,"%s %s",imon_stop,test);
		else
		   sprintf(command_line,"%s %s &",imon_stop,test);
		junk=system(command_line);
	}
}

/* 
 * As quickly as possible, generate a new buffer that
 * can not be easily compressed, or de-duped. Also
 * permit specified percentage of buffer to be updated.
 *
 * ibuf ... input buffer
 * obuf ... output buffer
 * seed ... Seed to use for srand, rand -> xor ops
 *          Seed composed from: blocknumber
		(do not include childnum as you want duplicates)
 * size ... size of buffers. (in bytes)
 * percent. Percent of buffer to modify.
 * percent_interior. Percent of buffer that is dedupable within 
 *                   and across files 
 * percent_compress. Percent of buffer that is dedupable within 
 *                   but not across files 
 *
 * Returns 0 (zero) for success, and -1 (minus one) for failure.
 */
int
gen_new_buf(char *ibuf, char *obuf, long seed, int size, int percent,
	int percent_interior, int percent_compress, int all)
{
	register int *ip, *op; /* Register for speed 	*/
	register int iseed; 	/* Register for speed 	*/
	register int isize; 	/* Register for speed 	*/
	register int cseed;	/* seed for dedupable for within & ! across */
	register int x,w; 	/* Register for speed 	*/
	register int value; 	/* Register for speed 	*/
	register int interior_size; 	/* size of interior dedup region */
	register int compress_size; 	/* size of compression dedup region */
	if(ibuf == NULL)	/* no input buf 	*/
		return(-1);
	if(obuf == NULL)	/* no output buf 	*/
		return(-1);
	if((percent > 100) || (percent < 0)) /* percent check */
		return(-1);
	if(size == 0)		/* size check 		*/
		return(-1);
	srand(seed+1+(((int)numrecs64)*dedup_mseed)); /* set random seed */
	iseed = rand();		/* generate random value */
	isize = (size * percent)/100; /* percent that is dedupable */
	interior_size = ((isize * percent_interior)/100);/* /sizeof(int) */
	compress_size =((interior_size * percent_compress)/100);
	ip = (int *)ibuf;	/* pointer to input buf */
	op = (int *)obuf;	/* pointer to output buf */
	if(all == 0)		/* Special case for verify only */
		isize = sizeof(int);
	/* interior_size = dedup_within + dedup_across */
	for(w=0;w<interior_size;w+=sizeof(int))	
	{
		*op=0xdeadbeef+dedup_mseed;
		*ip=0xdeadbeef+dedup_mseed;
		op++;
		ip++;
	}	
	/* Prepare for dedup within but not across */
	w=interior_size - compress_size;
	op=(int *)&obuf[w];
	ip=(int *)&ibuf[w];
	srand(chid+chid_skew+1+dedup_mseed);            /* set randdom seed 	*/
	cseed = rand();		/* generate random value */
	for(w=(interior_size-compress_size);w<interior_size;w+=sizeof(int))	
	{
		*op=*ip ^ cseed; /* do the xor op */
		op++;
		ip++;
	}	
 	/* isize = dedup across only */
	for(x=interior_size;x<isize;x+=sizeof(int))	/* tight loop for transformation */
	{
		*op=*ip ^ iseed; /* do the xor op */
		op++;
		ip++;
	}	
	if(all == 0)		/* Special case for verify only */
		return(0);
	/* make the rest of the buffer non-dedupable */
	if(100-percent > 0)
	{
		srand(1+seed+((chid+chid_skew+1)*(int)numrecs64)*dedup_mseed);
		value=rand();
/* printf("Non-dedup value %x seed %x\n",value,seed);*/
		for( ; x<size;x+=sizeof(int))
			*op++=(*ip++)^value; /* randomize the remainder */
	}
	return(0);
}
/* 
 * Used to touch all of the buffers so that the CPU data
 * cache is hot, and not part of the measurement.
 */
void
touch_dedup(char *i, int size)
{
	register int x;
	register int *ip;
	ip = (int *)i;
	srand(DEDUPSEED);
	for(x=0;x<size/sizeof(int);x++)
	{
		*ip=rand(); /* fill initial buffer */
		ip++;
	}
}

/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/



#define NN 312
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */


/* The array for the state vector */
static unsigned long long mt[NN]; 
/* mti==NN+1 means mt[NN] is not initialized */
static int mti=NN+1; 

/* initializes mt[NN] with a seed */
void init_genrand64(unsigned long long seed)
{
    mt[0] = seed;
    for (mti=1; mti<NN; mti++) 
        mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
void init_by_array64(unsigned long long init_key[],
		     unsigned long long key_length)
{
    unsigned long long i, j, k;
    init_genrand64(19650218ULL);
    i=1; j=0;
    k = (NN>key_length ? NN : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 3935559000370003845ULL))
          + init_key[j] + j; /* non linear */
        i++; j++;
        if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=NN-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 2862933555777941757ULL))
          - i; /* non linear */
        i++;
        if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
    }

    mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0, 2^64-1]-interval */
unsigned long long genrand64_int64(void)
{
    int i;
    unsigned long long x;
    static unsigned long long mag01[2]={0ULL, MATRIX_A};

    if (mti >= NN) { /* generate NN words at one time */

        /* if init_genrand64() has not been called, */
        /* a default initial seed is used     */
        if (mti == NN+1) 
            init_genrand64(5489ULL); 

        for (i=0;i<NN-MM;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        for (;i<NN-1;i++) {
            x = (mt[i]&UM)|(mt[i+1]&LM);
            mt[i] = mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        x = (mt[NN-1]&UM)|(mt[0]&LM);
        mt[NN-1] = mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

        mti = 0;
    }
  
    x = mt[mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

/* generates a random number on [0, 2^63-1]-interval */
long long genrand64_int63(void)
{
    return (long long)(genrand64_int64() >> 1);
}

/* generates a random number on [0,1]-real-interval */
double genrand64_real1(void)
{
    return (genrand64_int64() >> 11) * (1.0/9007199254740991.0);
}

/* generates a random number on [0,1)-real-interval */
double genrand64_real2(void)
{
    return (genrand64_int64() >> 11) * (1.0/9007199254740992.0);
}

/* generates a random number on (0,1)-real-interval */
double genrand64_real3(void)
{
    return ((genrand64_int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
}

#ifdef MT_TEST

int main(void)
{
    int i;
    unsigned long long init[4]={0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL}, length=4;
    init_by_array64(init, length);
    printf("1000 outputs of genrand64_int64()\n");
    for (i=0; i<1000; i++) {
      printf("%20llu ", genrand64_int64());
      if (i%5==4) printf("\n");
    }
    printf("\n1000 outputs of genrand64_real2()\n");
    for (i=0; i<1000; i++) {
      printf("%10.8f ", genrand64_real2());
      if (i%5==4) printf("\n");
    }
    return 0;
}
#endif

/*----------------------------------------------------------------------*/
/* 									*/
/* The PIT Programmable Interdimensional Timer 				*/
/* 									*/
/* This is used to measure time, when you know something odd is going   */
/* to be happening with your wrist watch. For example, you have entered	*/
/* a temporal distortion field where time its-self is not moving        */
/* as it does in your normal universe. ( thing either intense 		*/
/* gravitational fields bending space-time, or virtual machines playing */
/* with time )  							*/
/* So.. you need to measure time, but with respect to a normal 		*/
/* space-time.  So.. we deal with this by calling for time from another */
/* machine, but do so with a very similar interface to that of 		*/
/* gettimeofday().							*/
/* To activate this, one only needs to set an environmental variable.   */
/*      Example:   setenv IOZ_PIT hostname_of_PIT_server		*/
/* The environmental variable tells this client where to go to get 	*/
/* correct timeofday time stamps, with the usual gettimeofday() 	*/
/* resolution. (microsecond resolution)					*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* The PIT client: Adapted from source found on the web for someone's   */
/* daytime client code. (Used in many examples for network programming  */
/* Reads PIT info over a socket from a PIT server. 			*/
/* The PIT server sends its raw microsecond version of gettimeofday 	*/
/* The PIT client converts this back into timeval structure format.     */
/* Written by: Don Capps. [ capps@iozone.org ] 				*/
/*----------------------------------------------------------------------*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<      */
/* >>>> DON'T forget, you must put a definition for PIT <<<<<<<<<<      */
/* >>>> in /etc/services  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<      */
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<      */
#define DFLT_SERVICE   "PIT"     /* Default service name.            	*/
#define INVALID_DESC   -1        /* Invalid file (socket) descriptor.	*/
#define MAXBFRSIZE     256       /* Max bfr sz to read remote TOD.   	*/

/*
** Type definitions (for convenience).
*/
#if defined(Windows)
int false = 0;
int true = 1;
#else
typedef enum { false = 0, true } boolean;
#endif
typedef struct sockaddr_in       sockaddr_in_t;
typedef struct sockaddr_in6      sockaddr_in6_t;

/*
 * Routine to mimic gettimeofday() using a remote PIT server 
 */
#if defined(_SUA_)
struct timezone {
	int 				tz_minuteswest;				
	int 				tz_dsttime;							
};
#endif

int 
pit_gettimeofday( struct timeval *tp, struct timezone *foo,
	char *pit_hostname, char *pit_service)
{
	int            	sckt;          /* socket descriptor */
	unsigned scopeId = 0;

	/* See if the interdimensional rift is active */
	
	if(pit_hostname[0] == 0)
	{
		return gettimeofday(tp,foo);
	}

 	if ( ( sckt = openSckt( pit_hostname,
                           pit_service,
                           scopeId ) ) == INVALID_DESC )
   	{
      		fprintf( stderr,
                  "Sorry... a connectionless socket could "
                  "not be set up.\n");
                return -1;
   	}
	/*
   	** Get the remote PIT.
   	*/
   	pit( sckt ,tp );
	close(sckt);
    	return 0;
}

/*
 * Opens a socket for the PIT to use to get the time
 * from a remote time server ( A PIT server ) 
 */
static int openSckt( const char   *host,
                     const char   *service,
                     unsigned int  scopeId )
{
   struct addrinfo *ai;
   int              aiErr;
   struct addrinfo *aiHead;
   struct addrinfo  hints;
   sockaddr_in6_t  *pSadrIn6;
   int              sckt;
   /*
    * Initialize the 'hints' structure for getaddrinfo(3).
   */
   memset( &hints, 0, sizeof( hints ) );
   hints.ai_family   = PF_UNSPEC;     /* IPv4 or IPv6 records */
   hints.ai_socktype = SOCK_STREAM;    /* Connection oriented communication.*/
   hints.ai_protocol = IPPROTO_TCP;   /* TCP transport layer protocol only. */
   /*
   ** Look up the host/service information.
   */
   if ( ( aiErr = getaddrinfo( host,
                               service,
                               &hints,
                               &aiHead ) ) != 0 )
   {
      fprintf( stderr, "(line %d): ERROR - %s.\n", __LINE__, 
	 gai_strerror( aiErr ) );
      return INVALID_DESC;
   }
   /*
   ** Go through the list and try to open a connection.  Continue until either
   ** a connection is established or the entire list is exhausted.
   */
   for ( ai = aiHead,   sckt = INVALID_DESC;
         ( ai != NULL ) && ( sckt == INVALID_DESC );
         ai = ai->ai_next )
   {
      /*
      ** IPv6 kluge.  Make sure the scope ID is set.
      */
      if ( ai->ai_family == PF_INET6 )
      {
         pSadrIn6 = (sockaddr_in6_t*) ai->ai_addr;
         if ( pSadrIn6->sin6_scope_id == 0 )
         {
            pSadrIn6->sin6_scope_id = scopeId;
         }  /* End IF the scope ID wasn't set. */
      }  /* End IPv6 kluge. */
      /*
      ** Create a socket.
      */
      sckt = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
      if(sckt == -1)
      {
         sckt = INVALID_DESC;
         continue;   /* Try the next address record in the list. */
      }
      /*
      ** Set the target destination for the remote host on this socket.  That
      ** is, this socket only communicates with the specified host.
      */
      if (connect( sckt, ai->ai_addr, ai->ai_addrlen ) )
      {
         (void) close( sckt );   /* Could use system call again here, 
					but why? */
         sckt = INVALID_DESC;
         continue;   /* Try the next address record in the list. */
      }
   }  /* End FOR each address record returned by getaddrinfo(3). */
   /*
   ** Clean up & return.
   */
   freeaddrinfo( aiHead );
   return sckt;
}  /* End openSckt() */

/*
 * Read the PIT, and convert this back into timeval 
 * info, and store it in the timeval structure that was
 * passed in.
 */
static void pit( int sckt, struct timeval *tp)
{
   char bfr[ MAXBFRSIZE+1 ];
   int  inBytes;
   long long value;
   /*
   ** Send a datagram to the server to wake it up.  The content isn't
   ** important, but something must be sent to let it know we want the TOD.
   */
   junk=write( sckt, "Are you there?", 14 );
   /*
   ** Read the PIT from the remote host.
   */
   inBytes = read( sckt, bfr, MAXBFRSIZE );
   bfr[ inBytes ] = '\0';   /* Null-terminate the received string. */
   /* 
    * Convert result to timeval structure format 
    */
   sscanf(bfr,"%llu\n",&value);
   tp->tv_sec = (long)(value / 1000000);
   tp->tv_usec = (long)(value % 1000000);
}  

/* sync does not exist in SUA */
#if defined(_SUA_)
sync()
{
}
#endif


#define BUCKETS 40
long long buckets[BUCKETS];
long long bucket_val[BUCKETS] =
	{ 20,40,60,80,100,
	200,400,600,800,1000,
	2000,4000,6000,8000,10000,
	12000,14000,16000,18000,20000,
	40000,60000,80000,100000,
	200000,400000,600000,800000,1000000,
	2000000,4000000,6000000,8000000,10000000,
	20000000,30000000,60000000,90000000,120000000,120000001};
/*
 * Buckets: (Based on a Netapp internal consensus)
 *     0       1       2       3      4 
 *  <=20us  <=40us  <=60us  <=80us  <=100us
 * 
 *    5        6       7       8       9
 * <=200us  <=400us <=600us <=88us  <=1ms
 * 
 *   10       11      12      13      14
 * <=2ms    <=4ms   <=6ms   <=8ms   <=10ms
 * 
 *   15       16      17      18      19
 * <=12ms   <=14ms  <=16ms  <=18ms  <=20ms
 *
 *   20       21      22      23      24
 * <=20ms   <=40ms  <=60ms  <=80ms  <=100ms
 * 
 *   25       26      27      28      29
 * <=200ms  <=400ms <=600ms <=800ms <=1s
 * 
 *   30       31      32      33      34
 * <=2s     <=4s    <=6s    <=8s    <=10s
 *
 *   35       36      37      38      39
 * <=20s    <=30s   <=60    <=90s    >90
 */
	
/*
	fp=fopen("/tmp/iozone_latency_summary.txt","a");
	dump_hist(fp);
*/

void
hist_insert(double my_value)
{
	int k;
	long long value;
	
	/* Convert to micro-seconds */
	value = (long long)(my_value * 1000000);
	for(k=0;k<BUCKETS;k++)
	{
	   if(k < (BUCKETS-1)) 
	   {
	     if(value <= bucket_val[k])
	     {
		buckets[k]++;
		break;
	     }
	   }
	   else
	   {
	     if(value > bucket_val[k])
	     {
		buckets[k]++;
		break;
	     }
	   }
	}
}

void
dump_hist(char *what,int id)
{
   FILE *fp;

   char name[256];
 
   sprintf(name,"%s_child_%d.txt","Iozone_histogram",id);

   fp = fopen(name,"a");

#ifndef NO_PRINT_LLD
   fprintf(fp,"Child: %d Op: %s\n",id,what);
   fprintf(fp,"Band 1:   ");
   fprintf(fp," 20us:%-7.1lld ",buckets[0]);
   fprintf(fp," 40us:%-7.1lld ",buckets[1]);
   fprintf(fp," 60us:%-7.1lld ",buckets[2]);
   fprintf(fp," 80us:%-7.1lld ",buckets[3]);
   fprintf(fp,"100us:%-7.1lld \n",buckets[4]);

   fprintf(fp,"Band 2:   ");
   fprintf(fp,"200us:%-7.1lld ",buckets[5]);
   fprintf(fp,"400us:%-7.1lld ",buckets[6]);
   fprintf(fp,"600us:%-7.1lld ",buckets[7]);
   fprintf(fp,"800us:%-7.1lld ",buckets[8]);
   fprintf(fp,"  1ms:%-7.1lld \n",buckets[9]);

   fprintf(fp,"Band 3:   ");
   fprintf(fp,"  2ms:%-7.1lld ",buckets[10]);
   fprintf(fp,"  4ms:%-7.1lld ",buckets[11]);
   fprintf(fp,"  6ms:%-7.1lld ",buckets[12]);
   fprintf(fp,"  8ms:%-7.1lld ",buckets[13]);
   fprintf(fp," 10ms:%-7.1lld \n",buckets[14]);

   fprintf(fp,"Band 4:   ");
   fprintf(fp," 12ms:%-7.1lld ",buckets[15]);
   fprintf(fp," 14ms:%-7.1lld ",buckets[16]);
   fprintf(fp," 16ms:%-7.1lld ",buckets[17]);
   fprintf(fp," 18ms:%-7.1lld ",buckets[18]);
   fprintf(fp," 20ms:%-7.1lld \n",buckets[19]);

   fprintf(fp,"Band 5:   ");
   fprintf(fp," 40ms:%-7.1lld ",buckets[20]);
   fprintf(fp," 60ms:%-7.1lld ",buckets[21]);
   fprintf(fp," 80ms:%-7.1lld ",buckets[22]);
   fprintf(fp,"100ms:%-7.1lld \n",buckets[23]);

   fprintf(fp,"Band 6:   ");
   fprintf(fp,"200ms:%-7.1lld ",buckets[24]);
   fprintf(fp,"400ms:%-7.1lld ",buckets[25]);
   fprintf(fp,"600ms:%-7.1lld ",buckets[26]);
   fprintf(fp,"800ms:%-7.1lld ",buckets[27]);
   fprintf(fp,"   1s:%-7.1lld \n",buckets[28]);

   fprintf(fp,"Band 7:   ");
   fprintf(fp,"   2s:%-7.1lld ",buckets[29]);
   fprintf(fp,"   4s:%-7.1lld ",buckets[30]);
   fprintf(fp,"   6s:%-7.1lld ",buckets[31]);
   fprintf(fp,"   8s:%-7.1lld ",buckets[32]);
   fprintf(fp,"  10s:%-7.1lld \n",buckets[33]);

   fprintf(fp,"Band 8:   ");
   fprintf(fp,"  20s:%-7.1lld ",buckets[34]);
   fprintf(fp,"  40s:%-7.1lld ",buckets[35]);
   fprintf(fp,"  60s:%-7.1lld ",buckets[36]);
   fprintf(fp,"  80s:%-7.1lld ",buckets[37]);
   fprintf(fp," 120s:%-7.1lld \n",buckets[38]);

   fprintf(fp,"Band 9:   ");
   fprintf(fp,"120+s:%-7.1lld \n\n",buckets[39]);
#else
   fprintf(fp,"Child: %d Op: %s\n",id,what);
   fprintf(fp,"Band 1:   ");
   fprintf(fp," 20us:%-7.1ld ",buckets[0]);
   fprintf(fp," 40us:%-7.1ld ",buckets[1]);
   fprintf(fp," 60us:%-7.1ld ",buckets[2]);
   fprintf(fp," 80us:%-7.1ld ",buckets[3]);
   fprintf(fp,"100us:%-7.1ld \n",buckets[4]);

   fprintf(fp,"Band 2:   ");
   fprintf(fp,"200us:%-7.1ld ",buckets[5]);
   fprintf(fp,"400us:%-7.1ld ",buckets[6]);
   fprintf(fp,"600us:%-7.1ld ",buckets[7]);
   fprintf(fp,"800us:%-7.1ld ",buckets[8]);
   fprintf(fp,"  1ms:%-7.1ld \n",buckets[9]);

   fprintf(fp,"Band 3:   ");
   fprintf(fp,"  2ms:%-7.1ld ",buckets[10]);
   fprintf(fp,"  4ms:%-7.1ld ",buckets[11]);
   fprintf(fp,"  6ms:%-7.1ld ",buckets[12]);
   fprintf(fp,"  8ms:%-7.1ld ",buckets[13]);
   fprintf(fp," 10ms:%-7.1ld \n",buckets[14]);

   fprintf(fp,"Band 4:   ");
   fprintf(fp," 12ms:%-7.1ld ",buckets[15]);
   fprintf(fp," 14ms:%-7.1ld ",buckets[16]);
   fprintf(fp," 16ms:%-7.1ld ",buckets[17]);
   fprintf(fp," 18ms:%-7.1ld ",buckets[18]);
   fprintf(fp," 20ms:%-7.1ld \n",buckets[19]);

   fprintf(fp,"Band 5:   ");
   fprintf(fp," 40ms:%-7.1ld ",buckets[20]);
   fprintf(fp," 60ms:%-7.1ld ",buckets[21]);
   fprintf(fp," 80ms:%-7.1ld ",buckets[22]);
   fprintf(fp,"100ms:%-7.1ld \n",buckets[23]);

   fprintf(fp,"Band 6:   ");
   fprintf(fp,"200ms:%-7.1ld ",buckets[24]);
   fprintf(fp,"400ms:%-7.1ld ",buckets[25]);
   fprintf(fp,"600ms:%-7.1ld ",buckets[26]);
   fprintf(fp,"800ms:%-7.1ld ",buckets[27]);
   fprintf(fp,"   1s:%-7.1ld \n",buckets[28]);

   fprintf(fp,"Band 7:   ");
   fprintf(fp,"   2s:%-7.1ld ",buckets[29]);
   fprintf(fp,"   4s:%-7.1ld ",buckets[30]);
   fprintf(fp,"   6s:%-7.1ld ",buckets[31]);
   fprintf(fp,"   8s:%-7.1ld ",buckets[32]);
   fprintf(fp,"  10s:%-7.1ld \n",buckets[33]);

   fprintf(fp,"Band 8:   ");
   fprintf(fp,"  20s:%-7.1ld ",buckets[34]);
   fprintf(fp,"  40s:%-7.1ld ",buckets[35]);
   fprintf(fp,"  60s:%-7.1ld ",buckets[36]);
   fprintf(fp,"  80s:%-7.1ld ",buckets[37]);
   fprintf(fp," 120s:%-7.1ld \n",buckets[38]);

   fprintf(fp,"Band 9:   ");
   fprintf(fp,"120+s:%-7.1ld \n\n",buckets[39]);
#endif
   fclose(fp);
}

#ifdef HAVE_ANSIC_C
void * thread_fwrite_test(void *x)
#else
void * thread_fwrite_test( x)
#endif
{

        struct child_stats *child_stat;
        double starttime1 = 0;
        double temp_time;
        double hist_time;
	double desired_op_rate_time;
	double actual_rate;
        double walltime, cputime;
        double compute_val = (double)0;
        float delay = (float)0;
        double thread_qtime_stop,thread_qtime_start;
        off64_t traj_offset;
        long long w_traj_bytes_completed;
        long long w_traj_ops_completed;
        FILE *w_traj_fd;
        int fd;
        long long recs_per_buffer;
        long long stopped,i;
        off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
        long long xx,xx2;
        char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
        char *nbuff;
        char *maddr;
        char *wmaddr,*free_addr;
        char now_string[30];
        int anwser,bind_cpu,wval;
#if defined(VXFS) || defined(solaris)
        int test_foo = 0;
#endif
        off64_t filebytes64;
        char tmpname[256];
        FILE *thread_wqfd;
        FILE *thread_Lwqfd;
        char *filename;

        char *stdio_buf;
        char *how;
        long long Index = 0;
        FILE *stream = NULL;
        static int First_Run = 1;

        if(compute_flag)
                delay=compute_time;
        nbuff=maddr=wmaddr=free_addr=0;
        thread_qtime_stop=thread_qtime_start=0;
        thread_wqfd=w_traj_fd=thread_Lwqfd=(FILE *)0;
        traj_offset=walltime=cputime=0;
        anwser=bind_cpu=0;

        if(w_traj_flag)
        {
                filebytes64 = w_traj_fsize;
                numrecs64=w_traj_ops;
        }
        else
        {
                filebytes64 = numrecs64*reclen;
        }
        written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
        w_traj_bytes_completed=w_traj_ops_completed=0;
        recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
        xx=chid;
#else
        if(use_thread)
        {
                xx = (long long)((long)x);
        }
        else
        {
                xx=chid;
        }
#endif
#ifndef NO_THREADS
#ifdef _HPUX_SOURCE
        if(ioz_processor_bind)
        {
                 bind_cpu=(begin_proc+(int)xx)%num_processors;
                 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
                my_nap(40);     /* Switch to new cpu */
        }
#endif
#endif
        if(use_thread)
                nbuff=barray[xx];
        else
                nbuff=buffer;
        if(debug1 )
        {
           if(use_thread)
#ifdef NO_PRINT_LLD
                printf("\nStarting child %ld\n",xx);
#else
                printf("\nStarting child %lld\n",xx);
#endif
           else
#ifdef NO_PRINT_LLD
                printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
                printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif

        }
        dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
        xx2=xx;
        if(share_file)
                xx2=(long long)0;
        if(mfflag)
        {
#ifdef NO_PRINT_LLD
           sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
           sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
        }
        else
        {
#ifdef NO_PRINT_LLD
           sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
           sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
        }
        filename=dummyfile[xx];

        if(mmapflag || async_flag)
                return(0);

        stdio_buf=(char *)malloc((size_t)reclen);

        if(Uflag) /* Unmount and re-mount the mountpoint */
        {
                purge_buffer_cache();
        }

        if(First_Run==1)
        {
		First_Run=0;
                if(check_filename(filename))
                        how="r+"; /* file exists, don't create and zero a new one. */
                else
                        how="w+"; /* file doesn't exist. create it. */
        }
        else
                how="r+"; /* re-tests should error out if file does not exist. */

#ifdef IRIX64
        if((stream=(FILE *)fopen(filename,how)) == 0)
        {
                printf("\nCan not fdopen temp file: %s %lld\n",
                        filename,errno);
                perror("fdopen");
                exit(48);
        }
#else
       if((stream=(FILE *)I_FOPEN(filename,how)) == 0)
        {
#ifdef NO_PRINT_LLD
                printf("\nCan not fdopen temp file: %s %d\n",
                        filename,errno);
#else
                printf("\nCan not fdopen temp file: %s %d\n",
                        filename,errno);
#endif
                perror("fdopen");
                exit(49);
        }
#endif
        fd=fileno(stream);
        fsync(fd);
        if(direct_flag)
          setvbuf(stream,stdio_buf,_IONBF,reclen);
        else
          setvbuf(stream,stdio_buf,_IOFBF,reclen);

	if(use_thread)
            buffer=nbuff;
	else
            buffer=mainbuffer;
        if(fetchon)
                fetchit(buffer,reclen);
        if(verify || dedup || dedup_interior)
                fill_buffer(buffer,reclen,(long long)pattern,sverify,(long long)0);

        compute_val=(double)0;

       /*******************************************************************/

        child_stat = (struct child_stats *)&shmaddr[xx];
        child_stat->throughput = 0;
        child_stat->actual = 0;
        child_stat->flag=CHILD_STATE_READY; /* Tell parent child is ready to go */

        if(distributed && client_iozone)
                tell_master_ready(chid);
        if(distributed && client_iozone)
        {
                if(cdebug)
                {
                        printf("Child %d waiting for go from master\n",(int)xx);
                        fflush(stdout);
                }
                wait_for_master_go(chid);
                if(cdebug)
                {
                        printf("Child %d received go from master\n",(int)xx);
                        fflush(stdout);
                }
        }
        else
        {
                while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
                        Poll((long long)1);
        }

        written_so_far=0;
        child_stat = (struct child_stats *)&shmaddr[xx];
        child_stat->actual = 0;
        child_stat->throughput = 0;
        stopped=0;
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_fwol.dat",(int)xx);
                thread_wqfd=fopen(tmpname,"a");
                if(thread_wqfd==0)
                {
                        client_error=errno;
                        if(distributed && client_iozone)
                                send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
                fprintf(thread_wqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
        if(L_flag)
        {
                sprintf(tmpname,"Child_%d.log",(int)xx);
                thread_Lwqfd=fopen(tmpname,"a");
                if(thread_Lwqfd==0)
                {
                        client_error=errno;
                        if(distributed && client_iozone)
                                send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
                get_date(now_string);
                fprintf(thread_Lwqfd,"%-25s %s","fwrite test start: ",now_string);
        }
        starttime1 = time_so_far();
        if(cpuutilflag)
        {
                walltime = starttime1;
                cputime = cputime_so_far();
        }
       /*******************************************************************/
        for(i=0; i<numrecs64; i++){
                if(compute_flag)
                        compute_val+=do_compute(compute_time);
                if(multi_buffer)
                {
                        Index +=reclen;
                        if(Index > (MAXBUFFERSIZE-reclen))
                                Index=0;
                        buffer = mbuffer + Index;
                }
                if((verify & diag_v) || dedup || dedup_interior)
                        fill_buffer(buffer,reclen,(long long)pattern,sverify,i);
                if(purge)
                        purgeit(buffer,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
		{
			thread_qtime_start=time_so_far();
		}
                if(fwrite(buffer, (size_t) reclen, 1, stream) != 1)
                {
#ifdef NO_PRINT_LLD
                        printf("\nError fwriting block %ld, fd= %d\n", i,
                                 fd);
#else
                        printf("\nError fwriting block %lld, fd= %d\n", i,
                                 fd);
#endif
                        perror("fwrite");
			exit_code = 74;
                        signal_handler();
                }
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
			if( actual_rate < desired_op_rate_time)
			      my_unap((unsigned long long)((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_wqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_wqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
                w_traj_ops_completed++;
                w_traj_bytes_completed+=reclen;
                written_so_far+=reclen/1024;
        }
        if(include_flush)
        {
                fflush(stream);
                wval=fsync(fd);
                if(wval==-1){
                        perror("fsync");
                        signal_handler();
                }
        }
        if(include_close)
        {
                wval=fclose(stream);
                if(wval==-1){
                        perror("fclose");
                        signal_handler();
                }
        }
        /*******************************************************************/
        if(!stopped){
                temp_time = time_so_far();
		if(inc_think)
                	child_stat->throughput = ((temp_time - starttime1)-time_res);
		else
                	child_stat->throughput = ((temp_time - starttime1)-time_res)
                        	-compute_val;
                if(child_stat->throughput < (double).000001)
                {
                        child_stat->throughput= time_res;
                        if(rec_prob < reclen)
                                rec_prob = reclen;
                        res_prob=1;
                }

                if(OPS_flag){
                   /*written_so_far=(written_so_far*1024)/reclen;*/
                   written_so_far=w_traj_ops_completed;
                }
                child_stat->throughput =
                        (double)written_so_far/child_stat->throughput;
                child_stat->actual = (double)written_so_far;
        }
        if(cdebug)
        {
                printf("Child %d: throughput %f actual %f \n",(int)chid, child_stat->throughput,
                        child_stat->actual);
                fflush(stdout);
        }
        if(cpuutilflag)
        {
                cputime = cputime_so_far() - cputime;
                if (cputime < cputime_res)
                        cputime = 0.0;
                child_stat->cputime = cputime;
                walltime = time_so_far() - walltime;
                child_stat->walltime = walltime;
        }
        if(distributed && client_iozone)
                tell_master_stats(THREAD_FWRITE_TEST, chid, child_stat->throughput,
                        child_stat->actual,
                        child_stat->cputime, child_stat->walltime,
                        (char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);

        if (debug1) {
#ifdef NO_PRINT_LLD
                printf(" child/slot: %ld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#else
                printf(" child/slot: %lld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#endif
                        xx, walltime, cputime,
                        cpu_util(cputime, walltime));
        }
        child_stat->flag = CHILD_STATE_HOLD; /* Tell parent I'm done */
        stopped=0;

        /*******************************************************************/
        /* End fwrite performance test. *************************************/
        /*******************************************************************/

        if(debug1)
#ifdef NO_PRINT_LLD
                printf("\nChild finished %ld\n",xx);
#else
                printf("\nChild finished %lld\n",xx);
#endif
        if(Q_flag && (thread_wqfd !=0) )
                fclose(thread_wqfd);
        free(dummyfile[xx]);
        if(w_traj_flag)
                fclose(w_traj_fd);

        if(L_flag)
        {
                get_date(now_string);
                fprintf(thread_Lwqfd,"%-25s %s","Fwrite test finished: ",now_string);
                fclose(thread_Lwqfd);
        }
        /*******************************************************************/
        if(!include_close)
        {
                wval=fflush(stream);
                if(wval==-1){
                        perror("fflush");
                        signal_handler();
                }
                wval=fsync(fd);
                if(wval==-1){
                        perror("fsync");
                        signal_handler();
                }
                wval=fclose(stream);
                if(wval==-1){
                        perror("fclose");
                        signal_handler();
                }
        }

        if(restf)
                sleep((int)(int)rest_val);

        free(stdio_buf);
        if(OPS_flag || MS_flag){
           filebytes64=filebytes64/reclen;
        }
        /*******************************************************************/
	if(hist_summary)
	   dump_hist("Fwrite",(int)xx);
        if(distributed && client_iozone)
                return(0);
#ifdef NO_THREADS
        exit(0);
#else
        if(use_thread)
                thread_exit();
        else
                exit(0);
#endif

return(0);
}


#ifdef HAVE_ANSIC_C
void * thread_fread_test(void *x)
#else
void * thread_fread_test( x)
#endif
{
        struct child_stats *child_stat;
        double starttime1 = 0;
        double temp_time;
        double hist_time;
	double desired_op_rate_time;
	double actual_rate;
        double walltime, cputime;
        double compute_val = (double)0;
        float delay = (float)0;
        double thread_qtime_stop,thread_qtime_start;
        off64_t traj_offset;
        long long w_traj_bytes_completed;
        long long w_traj_ops_completed;
        FILE *w_traj_fd;
        int fd;
        long long recs_per_buffer;
        long long stopped,i;
        off64_t written_so_far, read_so_far, re_written_so_far,re_read_so_far;
        long long xx,xx2;
        char *dummyfile [MAXSTREAMS];           /* name of dummy file     */
        char *nbuff;
        char *maddr;
        char *wmaddr,*free_addr;
        char now_string[30];
        int anwser,bind_cpu;
#if defined(VXFS) || defined(solaris)
        int test_foo = 0;
#endif
        off64_t filebytes64;
        char tmpname[256];
        FILE *thread_wqfd;
        FILE *thread_Lwqfd;

        if(compute_flag)
                delay=compute_time;
        nbuff=maddr=wmaddr=free_addr=0;
        thread_qtime_stop=thread_qtime_start=0;
        thread_wqfd=w_traj_fd=thread_Lwqfd=(FILE *)0;
        traj_offset=walltime=cputime=0;
        anwser=bind_cpu=0;
        char *stdio_buf;
        long long Index = 0;
        FILE *stream = NULL;
        char *filename;

        if(w_traj_flag)
        {
                filebytes64 = w_traj_fsize;
                numrecs64=w_traj_ops;
        }
        else
        {
                filebytes64 = numrecs64*reclen;
        }
        written_so_far=read_so_far=re_written_so_far=re_read_so_far=0;
        w_traj_bytes_completed=w_traj_ops_completed=0;
        recs_per_buffer = cache_size/reclen ;
#ifdef NO_THREADS
        xx=chid;
#else
        if(use_thread)
        {
                xx = (long long)((long)x);
        }
        else
        {
                xx=chid;
        }
#endif
#ifndef NO_THREADS
#ifdef _HPUX_SOURCE
        if(ioz_processor_bind)
        {
                 bind_cpu=(begin_proc+(int)xx)%num_processors;
                 pthread_processor_bind_np(PTHREAD_BIND_FORCED_NP,
                         (pthread_spu_t *)&anwser, (pthread_spu_t)bind_cpu, pthread_self());
                my_nap(40);     /* Switch to new cpu */
        }
#endif
#endif
        if(use_thread)
                nbuff=barray[xx];
        else
                nbuff=buffer;
        if(debug1 )
        {
           if(use_thread)
#ifdef NO_PRINT_LLD
                printf("\nStarting child %ld\n",xx);
#else
                printf("\nStarting child %lld\n",xx);
#endif
           else
#ifdef NO_PRINT_LLD
                printf("\nStarting process %d slot %ld\n",getpid(),xx);
#else
                printf("\nStarting process %d slot %lld\n",getpid(),xx);
#endif

        }
        dummyfile[xx]=(char *)malloc((size_t)MAXNAMESIZE);
        xx2=xx;
        if(share_file)
                xx2=(long long)0;
        if(mfflag)
        {
#ifdef NO_PRINT_LLD
           sprintf(dummyfile[xx],"%s",filearray[xx2]);
#else
           sprintf(dummyfile[xx],"%s",filearray[xx2]);
#endif
        }
        else
        {
#ifdef NO_PRINT_LLD
           sprintf(dummyfile[xx],"%s.DUMMY.%ld",filearray[xx2],xx2);
#else
           sprintf(dummyfile[xx],"%s.DUMMY.%lld",filearray[xx2],xx2);
#endif
        }

        filename=dummyfile[xx];

        if(mmapflag || async_flag)
                return(0);

        stdio_buf=(char *)malloc((size_t)reclen);

	if(Uflag) /* Unmount and re-mount the mountpoint */
	{
		purge_buffer_cache();
	}
#ifdef IRIX64
	if((stream=(FILE *)fopen(filename,"r")) == 0)
	{
		printf("\nCan not fdopen temp file: %s\n",
			filename);
		perror("fdopen");
		exit(51);
	}
#else
	if((stream=(FILE *)I_FOPEN(filename,"r")) == 0)
	{
		printf("\nCan not fdopen temp file: %s\n",
			filename);
		perror("fdopen");
		exit(52);
	}
#endif
	fd=I_OPEN(filename,O_RDONLY,0);
	fsync(fd);
	close(fd);
        if(direct_flag)
          setvbuf(stream,stdio_buf,_IONBF,reclen);
        else
          setvbuf(stream,stdio_buf,_IOFBF,reclen);

        if(use_thread)
	    buffer=nbuff;
	else
	    buffer=mainbuffer;
	if(fetchon)
		fetchit(buffer,reclen);
	compute_val=(double)0;

       /*******************************************************************/
       /*******************************************************************/

        child_stat = (struct child_stats *)&shmaddr[xx];
        child_stat->throughput = 0;
        child_stat->actual = 0;
        child_stat->flag=CHILD_STATE_READY; /* Tell parent child is ready to go */

        if(distributed && client_iozone)
                tell_master_ready(chid);
        if(distributed && client_iozone)
        {
                if(cdebug)
                {
                        printf("Child %d waiting for go from master\n",(int)xx);
                        fflush(stdout);
                }
                wait_for_master_go(chid);
                if(cdebug)
                {
                        printf("Child %d received go from master\n",(int)xx);
                        fflush(stdout);
                }
        }
        else
        {
                while(child_stat->flag!=CHILD_STATE_BEGIN)   /* Wait for signal from parent */
                        Poll((long long)1);
        }

        written_so_far=0;
        child_stat = (struct child_stats *)&shmaddr[xx];
        child_stat->actual = 0;
        child_stat->throughput = 0;
        stopped=0;
        if(Q_flag)
        {
                sprintf(tmpname,"Child_%d_frol.dat",(int)xx);
                thread_wqfd=fopen(tmpname,"a");
                if(thread_wqfd==0)
                {
                        client_error=errno;
                        if(distributed && client_iozone)
                                send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
                fprintf(thread_wqfd,"Offset in kBytes   Latency in microseconds  Transfer size in bytes\n");
        }
        if(L_flag)
        {
                sprintf(tmpname,"Child_%d.log",(int)xx);
                thread_Lwqfd=fopen(tmpname,"a");
                if(thread_Lwqfd==0)
                {
                        client_error=errno;
                        if(distributed && client_iozone)
                                send_stop();
                        printf("Unable to open %s\n",tmpname);
                        exit(40);
                }
                get_date(now_string);
                fprintf(thread_Lwqfd,"%-25s %s","fread test start: ",now_string);
        }
        starttime1 = time_so_far();
        if(cpuutilflag)
        {
                walltime = starttime1;
                cputime = cputime_so_far();
        }



       /*******************************************************************/

	for(i=0; i<numrecs64; i++)
	{
		if(compute_flag)
			compute_val+=do_compute(compute_time);
		if(multi_buffer)
		{
			Index +=reclen;
			if(Index > (MAXBUFFERSIZE-reclen))
				Index=0;
			buffer = mbuffer + Index;
		}
		if(purge)
			purgeit(buffer,reclen);
		if(Q_flag || hist_summary || op_rate_flag)
		{
			thread_qtime_start=time_so_far();
		}
		if(fread(buffer, (size_t) reclen,1, stream) != 1)
		{
#ifdef _64BIT_ARCH_
#ifdef NO_PRINT_LLD
			printf("\nError freading block %ld %x\n", i,
				(unsigned long)buffer);
#else
			printf("\nError freading block %lld %llx\n", i,
				(unsigned long long)buffer);
#endif
#else
#ifdef NO_PRINT_LLD
			printf("\nError freading block %ld %lx\n", i,
				(long)buffer);
#else
			printf("\nError freading block %lld %lx\n", i,
				(long)buffer);
#endif
#endif
			perror("read");
			exit(54);
		}
		if(verify){
			if(verify_buffer(buffer,reclen,(off64_t)i,reclen,(long long)pattern,sverify)){
				exit(55);
			}
		}
		if(hist_summary)
		{
			thread_qtime_stop=time_so_far();
			hist_time =(thread_qtime_stop-thread_qtime_start);
			hist_insert(hist_time);
		}
		if(op_rate_flag)
		{
			thread_qtime_stop=time_so_far();
			desired_op_rate_time = ((double)1.0/(double)op_rate);
			actual_rate = (double)(thread_qtime_stop-thread_qtime_start);
			if( actual_rate < desired_op_rate_time)
			      my_unap((unsigned long long)((desired_op_rate_time-actual_rate)*1000000.0 ));
		}
		if(Q_flag)
		{
			thread_qtime_stop=time_so_far();
#ifdef NO_PRINT_LLD
			fprintf(thread_wqfd,"%10.1ld %10.0f %10.1ld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#else
			fprintf(thread_wqfd,"%10.1lld %10.0f %10.1lld\n",(traj_offset)/1024,((thread_qtime_stop-thread_qtime_start-time_res))*1000000,reclen);
#endif
		}
                w_traj_ops_completed++;
                w_traj_bytes_completed+=reclen;
                written_so_far+=reclen/1024;

	}
	if(include_flush)
		fflush(stream);
	if(include_close)
	{
		fclose(stream);
	}

        /*******************************************************************/

        if(!stopped){
                temp_time = time_so_far();
		if(inc_think)
                	child_stat->throughput = ((temp_time - starttime1)-time_res);
		else
                	child_stat->throughput = ((temp_time - starttime1)-time_res)
                        	-compute_val;
                if(child_stat->throughput < (double).000001)
                {
                        child_stat->throughput= time_res;
                        if(rec_prob < reclen)
                                rec_prob = reclen;
                        res_prob=1;
                }

                if(OPS_flag){
                   /*written_so_far=(written_so_far*1024)/reclen;*/
                   written_so_far=w_traj_ops_completed;
                }
                child_stat->throughput =
                        (double)written_so_far/child_stat->throughput;
                child_stat->actual = (double)written_so_far;
        }
        if(cdebug)
        {
                printf("Child %d: throughput %f actual %f \n",(int)chid, child_stat->throughput,
                        child_stat->actual);
                fflush(stdout);
        }
        if(cpuutilflag)
        {
                cputime = cputime_so_far() - cputime;
                if (cputime < cputime_res)
                        cputime = 0.0;
                child_stat->cputime = cputime;
                walltime = time_so_far() - walltime;
                child_stat->walltime = walltime;
        }
        if(distributed && client_iozone)
                tell_master_stats(THREAD_FREAD_TEST, chid, child_stat->throughput,
                        child_stat->actual,
                        child_stat->cputime, child_stat->walltime,
                        (char)*stop_flag,
                        (long long)CHILD_STATE_HOLD);

        if (debug1) {
#ifdef NO_PRINT_LLD
                printf(" child/slot: %ld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#else
                printf(" child/slot: %lld, wall-cpu: %8.3f %8.3fC" " -> %6.2f%%\n",
#endif
                        xx, walltime, cputime,
                        cpu_util(cputime, walltime));
        }
        child_stat->flag = CHILD_STATE_HOLD; /* Tell parent I'm done */
        stopped=0;

        /*******************************************************************/
        /* End fead performance test.  *************************************/
        /*******************************************************************/

        if(debug1)
#ifdef NO_PRINT_LLD
                printf("\nChild finished %ld\n",xx);
#else
                printf("\nChild finished %lld\n",xx);
#endif
        if(Q_flag && (thread_wqfd !=0) )
                fclose(thread_wqfd);
        free(dummyfile[xx]);
        if(w_traj_flag)
                fclose(w_traj_fd);

        if(L_flag)
        {
                get_date(now_string);
                fprintf(thread_Lwqfd,"%-25s %s","Fread test finished: ",now_string);
                fclose(thread_Lwqfd);
        }

        /*******************************************************************/

        if(!include_close)
	{
		fflush(stream);
		fclose(stream);
	}
        stream = NULL;

        if(restf)
                sleep((int)(int)rest_val);

        free(stdio_buf);
        if(OPS_flag || MS_flag){
           filebytes64=filebytes64/reclen;
        }

        /*******************************************************************/
	if(hist_summary)
	   dump_hist("Fread",(int)xx);
        if(distributed && client_iozone)
                return(0);
#ifdef NO_THREADS
        exit(0);
#else
        if(use_thread)
                thread_exit();
        else
                exit(0);
#endif

return(0);
}

static void update_burst_sleep(int fd, long long reclen, double *burst_sleep_time_till_now)
{
	if (burst_size_kb_64 == -1 || burst_sleep_duration_msec == -1)
		return;
	written_this_burst += reclen;
	if (written_this_burst >= (burst_size_kb_64 * 1024)) {
		struct timespec req, rem;
		int err;
		double sleep_start_time;

		written_this_burst = 0;
		rem.tv_sec = burst_sleep_duration_msec/1000;
		rem.tv_nsec = (burst_sleep_duration_msec%1000)*1000000;
		fsync(fd);
		sleep_start_time = time_so_far();
		do {
			req = rem;
			err = nanosleep(&req, &rem);
		} while ((err == -1) && (errno == EINTR));
		*burst_sleep_time_till_now += (time_so_far() - sleep_start_time);
	}
}
