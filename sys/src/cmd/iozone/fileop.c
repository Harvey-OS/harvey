/*
 * Author: Don Capps
 * 3/13/2006
 *
 *       Author: Don Capps (capps@iozone.org)
 *               7417 Crenshaw
 *               Plano, TX 75025
 *
 *  Copyright 2006, 2007, 2008, 2009   Don Capps.
 *
 *  License to freely use and distribute this software is hereby granted 
 *  by the author, subject to the condition that this copyright notice 
 *  remains intact.  The author retains the exclusive right to publish 
 *  derivative works based on this work, including, but not limited to,
 *  revised versions of this work",
 *
 *
  fileop [-f X ]|[-l # -u #] [-s Y] [-e] [-b] [-w] [-d <dir>] [-t] [-v] [-h]
       -f #      Force factor. X^3 files will be created and removed.
       -l #      Lower limit on the value of the Force factor.
       -u #      Upper limit on the value of the Force factor.
       -s #      Optional. Sets filesize for the create/write. May use suffix 'K' or 'M'.
       -e        Excel importable format.
       -b        Output best case.
       -w        Output worst case.
       -d <dir>  Specify starting directory.
       -U <dir>  Mount point to remount between tests.
       -t        Verbose output option.
       -v        Version information.
       -h        Help text.
 *
 * X is a force factor. The total number of files will
 *   be X * X * X   ( X ^ 3 )
 *   The structure of the file tree is:
 *   X number of Level 1 directories, with X number of
 *   level 2 directories, with X number of files in each
 *   of the level 2 directories.
 *
 *   Example:  fileop 2
 *
 *           dir_1                        dir_2
 *          /     \                      /     \
 *    sdir_1       sdir_2          sdir_1       sdir_2
 *    /     \     /     \          /     \      /     \
 * file_1 file_2 file_1 file_2   file_1 file_2 file_1 file_2
 *
 * Each file will be created, and then 1 byte is written to the file.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <limits.h>

#if defined(Windows)
#include <Windows.h>
#endif
#if !defined(PATH_MAX)
#define PATH_MAX 255
#endif

#if defined(_SUA_)
extern char *optarg;
extern char *opterr;
int fsync();
int getopt();
#endif
int junk, *junkp;
int x,excel;
int verbose = 0;
int sz = 1;
char *mbuffer;
int incr = 1;
#define _STAT_CREATE 0
#define _STAT_WRITE 1
#define _STAT_CLOSE 2
#define _STAT_LINK 3
#define _STAT_UNLINK 4
#define _STAT_DELETE 5
#define _STAT_STAT 6
#define _STAT_ACCESS 7
#define _STAT_CHMOD 8
#define _STAT_READDIR 9
#define _STAT_DIR_CREATE 10
#define _STAT_DIR_DELETE 11
#define _STAT_READ 12
#define _STAT_OPEN 13
#define _STAT_DIR_TRAVERSE 14
#define _NUM_STATS 15
struct stat_struct {
	double starttime;
	double endtime;
	double speed;
	double best;
	double worst;
	double dummy;
	double total_time;
	double dummy1;
	long long counter;
} volatile stats[_NUM_STATS];


static double time_so_far(void);
void dir_create(int);
void dir_traverse(int);
void dir_delete(int);
void file_create(int);
void file_stat(int);
void file_access(int);
void file_chmod(int);
void file_readdir(int);
void file_delete(int);
void file_link(int);
void file_unlink(int);
void file_read(int);
void splash(void);
void usage(void);
void bzero();
void clear_stats();
int validate(char *, int , char );

#define THISVERSION "        $Revision: 1.61 $"
/*#define NULL 0*/

char version[]=THISVERSION;
char thedir[PATH_MAX]="."; /* Default is to use the current directory */
const char *mountname=NULL; /* Default is not to unmount anything between the tests */

int cret;
int lower, upper,range;
int i;
int best, worst;
int dirlen;

/************************************************************************/
/* Routine to purge the buffer cache by unmounting drive.		*/
/************************************************************************/
void purge_buffer_cache()
{
	if (!mountname)
		return;

	char cwd[PATH_MAX];
	char command[1024];
	int ret,i;

	junkp=(int *)getcwd(cwd, sizeof(cwd));
	junk=chdir("/");
	strcpy(command,"umount ");
	strcat(command, mountname);
        /*
           umount might fail if the device is still busy, so
           retry unmounting several times with increasing delays
        */
        for (i = 1; i < 10; ++i) {
               ret = system(command);
               if (ret == 0)
                       break;
               sleep(i); /* seconds */
        }
	strcpy(command,"mount ");
	strcat(command, mountname);
	junk=system(command);
	junk=chdir(cwd);
}

int main(int argc, char **argv)
{
	if(argc == 1)
	{
		usage();
		exit(1);
	}
	while((cret = getopt(argc,argv,"hbwetvf:s:l:u:d:U:i: ")) != EOF){
		switch(cret){
                case 'h':
                        usage();
                        exit(0);
                        break;
                case 'd' :
                        dirlen=strlen(optarg);
                        if (optarg[dirlen-1]=='/')
                          --dirlen;
                        strncpy(thedir, optarg, dirlen);
			thedir[dirlen] = 0;
                        break;
		case 'U':
			mountname = optarg;
			break;
		case 'i':	/* Increment force by */
			incr=atoi(optarg);
			if(incr < 0)
				incr=1;
			break;
		case 'f':	/* Force factor */
			x=atoi(optarg);
			if(x < 0)
				x=1;
			break;
		case 's':	/* Size of files */
                        sz=atoi(optarg);
                        if(optarg[strlen(optarg)-1]=='k' ||
                                optarg[strlen(optarg)-1]=='K'){
                                sz = (1024 * atoi(optarg));
                        }
                        if(optarg[strlen(optarg)-1]=='m' ||
                                optarg[strlen(optarg)-1]=='M'){
                                sz = (1024 * 1024 * atoi(optarg));
                        }
			if(sz < 0)
				sz=1;
			break;
		case 'l':	/* lower force value */
			lower=atoi(optarg);
			range=1;
			if(lower < 0)
				lower=1;
			break;
		case 'v':	/* version */
			splash();
			exit(0);
			break;
		case 'u':	/* upper force value */
			upper=atoi(optarg);
			range=1;
			if(upper < 0)
				upper=1;
			break;
		case 't':	/* verbose */
			verbose=1;
			break;
		case 'e':	/* Excel */
			excel=1;
			break;
		case 'b':	/* Best */
			best=1;
			break;
		case 'w':	/* Worst */
			worst=1;
			break;
		}
	}
	mbuffer=(char *)malloc(sz);
	memset(mbuffer,'a',sz);
	if(!excel)
	  printf("\nFileop:  Working in %s, File size is %d,  Output is in Ops/sec. (A=Avg, B=Best, W=Worst)\n", thedir, sz);
	if(!verbose)
	{
#ifdef Windows
	   	printf(" .     %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %10s\n",
       	   	"mkdir","chdir","rmdir","create","open","read","write","close","stat",
		"access","chmod","readdir","delete"," Total_files");
#else

	   	printf(" .     %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %10s\n",
       	   	"mkdir","chdir","rmdir","create","open", "read","write","close","stat",
		"access","chmod","readdir","link  ","unlink","delete",
		" Total_files");
#endif
	}
	junk=chdir(thedir); /* change starting point */
	if(x==0)
		x=1;
	if(range==0)
		lower=upper=x;
	for(i=lower;i<=upper;i+=incr)
	{
		clear_stats();
		x=i;
	   /*
	    * Dir Create test 
	    */
	   purge_buffer_cache();
	   dir_create(x);

	   if(verbose)
	   {
	      printf("mkdir:   Dirs = %9lld ",stats[_STAT_DIR_CREATE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_DIR_CREATE].total_time);
	      printf("         Avg mkdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_DIR_CREATE].counter/stats[_STAT_DIR_CREATE].total_time,
			stats[_STAT_DIR_CREATE].total_time/stats[_STAT_DIR_CREATE].counter);
	      printf("         Best mkdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[_STAT_DIR_CREATE].best,stats[_STAT_DIR_CREATE].best);
	      printf("         Worst mkdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[_STAT_DIR_CREATE].worst,stats[_STAT_DIR_CREATE].worst);
	   }

	   /*
	    * Dir Traverse test
	    */
	   purge_buffer_cache();
	   dir_traverse(x);

	   if(verbose)
	   {
	      printf("chdir:   Dirs = %9lld ",stats[_STAT_DIR_TRAVERSE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_DIR_TRAVERSE].total_time);
	      printf("         Avg chdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_DIR_TRAVERSE].counter/stats[_STAT_DIR_TRAVERSE].total_time,
			stats[_STAT_DIR_TRAVERSE].total_time/stats[_STAT_DIR_TRAVERSE].counter);
	      printf("         Best chdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[_STAT_DIR_TRAVERSE].best,stats[_STAT_DIR_TRAVERSE].best);
	      printf("         Worst chdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[_STAT_DIR_TRAVERSE].worst,stats[_STAT_DIR_TRAVERSE].worst);
	   }

	   /*
	    * Dir delete test
	    */
	   purge_buffer_cache();
	   dir_delete(x);

	   if(verbose)
	   {
	   printf("rmdir:   Dirs = %9lld ",stats[_STAT_DIR_DELETE].counter);
	   printf("Total Time = %12.9f seconds\n",stats[_STAT_DIR_DELETE].total_time);
	   printf("         Avg rmdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_DIR_DELETE].counter/stats[_STAT_DIR_DELETE].total_time,
			stats[_STAT_DIR_DELETE].total_time/stats[_STAT_DIR_DELETE].counter);
	   printf("         Best rmdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[_STAT_DIR_DELETE].best,stats[_STAT_DIR_DELETE].best);
	   printf("         Worst rmdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[_STAT_DIR_DELETE].worst,stats[_STAT_DIR_DELETE].worst);
	   }

	   /*
	    * Create test 
	    */
	   purge_buffer_cache();
	   file_create(x);
	   if(verbose)
	   {
	      printf("create:  Files = %9lld ",stats[_STAT_CREATE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_CREATE].total_time);
	      printf("         Avg create(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_CREATE].counter/stats[_STAT_CREATE].total_time,
			stats[_STAT_CREATE].total_time/stats[_STAT_CREATE].counter);
	      printf("         Best create(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_CREATE].best,stats[_STAT_CREATE].best);
	      printf("         Worst create(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_CREATE].worst,stats[_STAT_CREATE].worst);
	      printf("write:   Files = %9lld ",stats[_STAT_WRITE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_WRITE].total_time);
	      printf("         Avg write(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_WRITE].counter/stats[_STAT_WRITE].total_time,
			stats[_STAT_WRITE].total_time/stats[_STAT_WRITE].counter);
	      printf("         Best write(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_WRITE].best,stats[_STAT_WRITE].best);
   	      printf("         Worst write(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_WRITE].worst,stats[_STAT_WRITE].worst);
	      printf("close:   Files = %9lld ",stats[_STAT_CLOSE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_CLOSE].total_time);
	      printf("         Avg close(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_CLOSE].counter/stats[_STAT_CLOSE].total_time,
			stats[_STAT_CLOSE].total_time/stats[_STAT_CLOSE].counter);
	      printf("         Best close(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_CLOSE].best,stats[_STAT_CLOSE].best);
	      printf("         Worst close(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_CLOSE].worst,stats[_STAT_CLOSE].worst);
	   }

	   /*
	    * Stat test 
	    */
	   purge_buffer_cache();
	   file_stat(x);

	   if(verbose)
	   {
	      printf("stat:    Files = %9lld ",stats[_STAT_STAT].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_STAT].total_time);
	      printf("         Avg stat(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_STAT].counter/stats[_STAT_STAT].total_time,
			stats[_STAT_STAT].total_time/stats[_STAT_STAT].counter);
	      printf("         Best stat(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_STAT].best,stats[_STAT_STAT].best);
	      printf("         Worst stat(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_STAT].worst,stats[_STAT_STAT].worst);
	   }
	   /*
	    * Read test 
	    */
	   purge_buffer_cache();
	   file_read(x);

	   if(verbose)
	   {
	      printf("open:    Files = %9lld ",stats[_STAT_OPEN].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_OPEN].total_time);
	      printf("         Avg open(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_OPEN].counter/stats[_STAT_OPEN].total_time,
			stats[_STAT_OPEN].total_time/stats[_STAT_OPEN].counter);
	      printf("         Best open(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_OPEN].best,stats[_STAT_OPEN].best);
	      printf("         Worst open(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_OPEN].worst,stats[_STAT_OPEN].worst);

	      printf("read:    Files = %9lld ",stats[_STAT_READ].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_READ].total_time);
	      printf("         Avg read(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_READ].counter/stats[_STAT_READ].total_time,
			stats[_STAT_READ].total_time/stats[_STAT_READ].counter);
	      printf("         Best read(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_READ].best,stats[_STAT_READ].best);
	      printf("         Worst read(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_READ].worst,stats[_STAT_READ].worst);
	   }

	   /*
	    * Access test 
	    */
	   purge_buffer_cache();
	   file_access(x);
	   if(verbose)
	   {
	      printf("access:  Files = %9lld ",stats[_STAT_ACCESS].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_ACCESS].total_time);
	      printf("         Avg access(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_ACCESS].counter/stats[_STAT_ACCESS].total_time,
			stats[_STAT_ACCESS].total_time/stats[_STAT_ACCESS].counter);
	      printf("         Best access(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_ACCESS].best,stats[_STAT_ACCESS].best);
	      printf("         Worst access(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_ACCESS].worst,stats[_STAT_ACCESS].worst);
	   }
	   /*
	    * Chmod test 
	    */
	   purge_buffer_cache();
	   file_chmod(x);

	   if(verbose)
	   {
	      printf("chmod:   Files = %9lld ",stats[_STAT_CHMOD].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_CHMOD].total_time);
	      printf("         Avg chmod(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_CHMOD].counter/stats[_STAT_CHMOD].total_time,
			stats[_STAT_CHMOD].total_time/stats[_STAT_CHMOD].counter);
	      printf("         Best chmod(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_CHMOD].best,stats[_STAT_CHMOD].best);
	      printf("         Worst chmod(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_CHMOD].worst,stats[_STAT_CHMOD].worst);
	   }
	   /*
	    * readdir test 
	    */
	   purge_buffer_cache();
	   file_readdir(x);

	   if(verbose)
	   {
	      printf("readdir: Files = %9lld ",stats[_STAT_READDIR].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_READDIR].total_time);
	      printf("         Avg readdir(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_READDIR].counter/stats[_STAT_READDIR].total_time,
			stats[_STAT_READDIR].total_time/stats[_STAT_READDIR].counter);
	      printf("         Best readdir(s)/sec  = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_READDIR].best,stats[_STAT_READDIR].best);
	      printf("         Worst readdir(s)/sec = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_READDIR].worst,stats[_STAT_READDIR].worst);
	   }
#if !defined(Windows)
	   /*
	    * link test 
	    */
	   purge_buffer_cache();
	   file_link(x);
	   if(verbose)
	   {
	      printf("link:    Files = %9lld ",stats[_STAT_LINK].counter);
	      printf("Total Time = %12.9f seconds\n",stats[_STAT_LINK].total_time);
	      printf("         Avg link(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_LINK].counter/stats[_STAT_LINK].total_time,
			stats[_STAT_LINK].total_time/stats[_STAT_LINK].counter);
	      printf("         Best link(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_LINK].best,stats[_STAT_LINK].best);
	      printf("         Worst link(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_LINK].worst,stats[_STAT_LINK].worst);
	   }
	   /*
	    * unlink test 
	    */
	   purge_buffer_cache();
	   file_unlink(x);
	   if(verbose)
	   {
	      printf("unlink:  Files = %9lld ",stats[_STAT_UNLINK].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_UNLINK].total_time);
	      printf("         Avg unlink(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_UNLINK].counter/stats[_STAT_UNLINK].total_time,
			stats[_STAT_UNLINK].total_time/stats[_STAT_UNLINK].counter);
	      printf("         Best unlink(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_UNLINK].best,stats[_STAT_UNLINK].best);
	      printf("         Worst unlink(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_UNLINK].worst,stats[_STAT_UNLINK].worst);
	   }
#endif
	   /*
	    * Delete test 
	    */
	   purge_buffer_cache();
	   file_delete(x);
	   if(verbose)
	   {
	      printf("delete:  Files = %9lld ",stats[_STAT_DELETE].counter);
	      printf("Total Time = %12.9f seconds\n", stats[_STAT_DELETE].total_time);
	      printf("         Avg delete(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[_STAT_DELETE].counter/stats[_STAT_DELETE].total_time,
			stats[_STAT_DELETE].total_time/stats[_STAT_DELETE].counter);
	      printf("         Best delete(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[_STAT_DELETE].best,stats[_STAT_DELETE].best);
	      printf("         Worst delete(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[_STAT_DELETE].worst,stats[_STAT_DELETE].worst);
	   }
	   if(!verbose)
	   {
	         printf("%c %4d %7.0f ",'A',x,stats[_STAT_DIR_CREATE].counter/stats[_STAT_DIR_CREATE].total_time);
	         printf("%7.0f ",stats[_STAT_DIR_TRAVERSE].counter/stats[_STAT_DIR_TRAVERSE].total_time);
	         printf("%7.0f ",stats[_STAT_DIR_DELETE].counter/stats[_STAT_DIR_DELETE].total_time);
	         printf("%7.0f ",stats[_STAT_CREATE].counter/stats[_STAT_CREATE].total_time);
	         printf("%7.0f ",stats[_STAT_OPEN].counter/stats[_STAT_OPEN].total_time);
	         printf("%7.0f ",stats[_STAT_READ].counter/stats[_STAT_READ].total_time);
	         printf("%7.0f ",stats[_STAT_WRITE].counter/stats[_STAT_WRITE].total_time);
	         printf("%7.0f ",stats[_STAT_CLOSE].counter/stats[_STAT_CLOSE].total_time);
	         printf("%7.0f ",stats[_STAT_STAT].counter/stats[_STAT_STAT].total_time);
	         printf("%7.0f ",stats[_STAT_ACCESS].counter/stats[_STAT_ACCESS].total_time);
	         printf("%7.0f ",stats[_STAT_CHMOD].counter/stats[_STAT_CHMOD].total_time);
	         printf("%7.0f ",stats[_STAT_READDIR].counter/stats[_STAT_READDIR].total_time);
#ifndef Windows
	         printf("%7.0f ",stats[_STAT_LINK].counter/stats[_STAT_LINK].total_time);
	         printf("%7.0f ",stats[_STAT_UNLINK].counter/stats[_STAT_UNLINK].total_time);
#endif
	         printf("%7.0f ",stats[_STAT_DELETE].counter/stats[_STAT_DELETE].total_time);
	         printf("%10d ",x*x*x);
	         printf("\n");
  	   	 fflush(stdout);

		if(best)
		{
	         printf("%c %4d %7.0f ",'B',x, 1/stats[_STAT_DIR_CREATE].best);
	         printf("%7.0f ",1/stats[_STAT_DIR_TRAVERSE].best);
	         printf("%7.0f ",1/stats[_STAT_DIR_DELETE].best);
	         printf("%7.0f ",1/stats[_STAT_CREATE].best);
	         printf("%7.0f ",1/stats[_STAT_OPEN].best);
	         printf("%7.0f ",1/stats[_STAT_READ].best);
	         printf("%7.0f ",1/stats[_STAT_WRITE].best);
	         printf("%7.0f ",1/stats[_STAT_CLOSE].best);
	         printf("%7.0f ",1/stats[_STAT_STAT].best);
	         printf("%7.0f ",1/stats[_STAT_ACCESS].best);
	         printf("%7.0f ",1/stats[_STAT_CHMOD].best);
	         printf("%7.0f ",1/stats[_STAT_READDIR].best);
#ifndef Windows
	         printf("%7.0f ",1/stats[_STAT_LINK].best);
	         printf("%7.0f ",1/stats[_STAT_UNLINK].best);
#endif
	         printf("%7.0f ",1/stats[_STAT_DELETE].best);
	         printf("%10d ",x*x*x);
		 printf("\n");
  	   	 fflush(stdout);
		}
		if(worst)
		{
	         printf("%c %4d %7.0f ",'W',x, 1/stats[_STAT_DIR_CREATE].worst);
	         printf("%7.0f ",1/stats[_STAT_DIR_TRAVERSE].worst);
	         printf("%7.0f ",1/stats[_STAT_DIR_DELETE].worst);
	         printf("%7.0f ",1/stats[_STAT_CREATE].worst);
	         printf("%7.0f ",1/stats[_STAT_OPEN].worst);
	         printf("%7.0f ",1/stats[_STAT_READ].worst);
	         printf("%7.0f ",1/stats[_STAT_WRITE].worst);
	         printf("%7.0f ",1/stats[_STAT_CLOSE].worst);
	         printf("%7.0f ",1/stats[_STAT_STAT].worst);
	         printf("%7.0f ",1/stats[_STAT_ACCESS].worst);
	         printf("%7.0f ",1/stats[_STAT_CHMOD].worst);
	         printf("%7.0f ",1/stats[_STAT_READDIR].worst);
#ifndef Windows
	         printf("%7.0f ",1/stats[_STAT_LINK].worst);
	         printf("%7.0f ",1/stats[_STAT_UNLINK].worst);
#endif
	         printf("%7.0f ",1/stats[_STAT_DELETE].worst);
	         printf("%10d ",x*x*x);
		 printf("\n");
  	   	 fflush(stdout);
		}
	   }
	}
	return(0);
}

void 
dir_create(int x)
{
	int i,j,k;
	int ret;
	char buf[100];
	stats[_STAT_DIR_CREATE].best=(double)99999.9;
	stats[_STAT_DIR_CREATE].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  stats[_STAT_DIR_CREATE].starttime=time_so_far();
	  ret=mkdir(buf,0777);
	  if(ret < 0)
	  {
	      printf("Mkdir failed\n");
	      exit(1);
	  }
	  stats[_STAT_DIR_CREATE].endtime=time_so_far();
	  stats[_STAT_DIR_CREATE].speed=stats[_STAT_DIR_CREATE].endtime-stats[_STAT_DIR_CREATE].starttime;
	  if(stats[_STAT_DIR_CREATE].speed < (double)0.0)
		stats[_STAT_DIR_CREATE].speed=(double)0.0;
	  stats[_STAT_DIR_CREATE].total_time+=stats[_STAT_DIR_CREATE].speed;
	  stats[_STAT_DIR_CREATE].counter++;
	  if(stats[_STAT_DIR_CREATE].speed < stats[_STAT_DIR_CREATE].best)
	 	stats[_STAT_DIR_CREATE].best=stats[_STAT_DIR_CREATE].speed;
	  if(stats[_STAT_DIR_CREATE].speed > stats[_STAT_DIR_CREATE].worst)
		 stats[_STAT_DIR_CREATE].worst=stats[_STAT_DIR_CREATE].speed;
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    stats[_STAT_DIR_CREATE].starttime=time_so_far();
	    ret=mkdir(buf,0777);
	    if(ret < 0)
	    {
	      printf("Mkdir failed\n");
	      exit(1);
	    }
	    stats[_STAT_DIR_CREATE].endtime=time_so_far();
	    stats[_STAT_DIR_CREATE].speed=stats[_STAT_DIR_CREATE].endtime-stats[_STAT_DIR_CREATE].starttime;
	    if(stats[_STAT_DIR_CREATE].speed < (double)0.0)
		stats[_STAT_DIR_CREATE].speed=(double) 0.0;
	    stats[_STAT_DIR_CREATE].total_time+=stats[_STAT_DIR_CREATE].speed;
	    stats[_STAT_DIR_CREATE].counter++;
	    if(stats[_STAT_DIR_CREATE].speed < stats[_STAT_DIR_CREATE].best)
		 stats[_STAT_DIR_CREATE].best=stats[_STAT_DIR_CREATE].speed;
	    if(stats[_STAT_DIR_CREATE].speed > stats[_STAT_DIR_CREATE].worst)
		 stats[_STAT_DIR_CREATE].worst=stats[_STAT_DIR_CREATE].speed;
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_dir_%d_%d_%d",i,j,k);
	      stats[_STAT_DIR_CREATE].starttime=time_so_far();
	      ret=mkdir(buf,0777);
	      if(ret < 0)
	      {
	        printf("Mkdir failed\n");
	        exit(1);
	      }
	      stats[_STAT_DIR_CREATE].endtime=time_so_far();
	      stats[_STAT_DIR_CREATE].speed=stats[_STAT_DIR_CREATE].endtime-stats[_STAT_DIR_CREATE].starttime;
	      if(stats[_STAT_DIR_CREATE].speed < (double)0.0)
		stats[_STAT_DIR_CREATE].speed=(double) 0.0;
	      stats[_STAT_DIR_CREATE].total_time+=stats[_STAT_DIR_CREATE].speed;
	      stats[_STAT_DIR_CREATE].counter++;
	      if(stats[_STAT_DIR_CREATE].speed < stats[_STAT_DIR_CREATE].best)
		 stats[_STAT_DIR_CREATE].best=stats[_STAT_DIR_CREATE].speed;
	      if(stats[_STAT_DIR_CREATE].speed > stats[_STAT_DIR_CREATE].worst)
		 stats[_STAT_DIR_CREATE].worst=stats[_STAT_DIR_CREATE].speed;
	      junk=chdir(buf);
	      junk=chdir("..");
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void
dir_traverse(int x)
{
	int i,j,k;
	char buf[100];
	double time1, time2;
	stats[_STAT_DIR_TRAVERSE].best=(double)99999.9;
	stats[_STAT_DIR_TRAVERSE].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  stats[_STAT_DIR_TRAVERSE].starttime=time_so_far();
	  junk=chdir(buf);
	  stats[_STAT_DIR_TRAVERSE].endtime=time_so_far();
	  time1=stats[_STAT_DIR_TRAVERSE].endtime-stats[_STAT_DIR_TRAVERSE].starttime;
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    stats[_STAT_DIR_TRAVERSE].starttime=time_so_far();
	    junk=chdir(buf);
	    stats[_STAT_DIR_TRAVERSE].endtime=time_so_far();
	    time2=stats[_STAT_DIR_TRAVERSE].endtime-stats[_STAT_DIR_TRAVERSE].starttime;
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_dir_%d_%d_%d",i,j,k);
	      stats[_STAT_DIR_TRAVERSE].starttime=time_so_far();
	      junk=chdir(buf);
	      junk=chdir("..");
	      stats[_STAT_DIR_TRAVERSE].endtime=time_so_far();
	      stats[_STAT_DIR_TRAVERSE].speed=stats[_STAT_DIR_TRAVERSE].endtime-stats[_STAT_DIR_TRAVERSE].starttime;
	      if(stats[_STAT_DIR_TRAVERSE].speed < (double)0.0)
		stats[_STAT_DIR_TRAVERSE].speed=(double) 0.0;
	      stats[_STAT_DIR_TRAVERSE].total_time+=stats[_STAT_DIR_TRAVERSE].speed;
	      stats[_STAT_DIR_TRAVERSE].counter++;
	      if(stats[_STAT_DIR_TRAVERSE].speed < stats[_STAT_DIR_TRAVERSE].best)
		 stats[_STAT_DIR_TRAVERSE].best=stats[_STAT_DIR_TRAVERSE].speed;
	      if(stats[_STAT_DIR_TRAVERSE].speed > stats[_STAT_DIR_TRAVERSE].worst)
		 stats[_STAT_DIR_TRAVERSE].worst=stats[_STAT_DIR_TRAVERSE].speed;
	    }
	    stats[_STAT_DIR_TRAVERSE].starttime=time_so_far();
	    junk=chdir("..");
	    stats[_STAT_DIR_TRAVERSE].endtime=time_so_far();
	    stats[_STAT_DIR_TRAVERSE].speed=time2+stats[_STAT_DIR_TRAVERSE].endtime-stats[_STAT_DIR_TRAVERSE].starttime;
	    if(stats[_STAT_DIR_TRAVERSE].speed < (double)0.0)
		stats[_STAT_DIR_TRAVERSE].speed=(double) 0.0;
	    stats[_STAT_DIR_TRAVERSE].total_time+=stats[_STAT_DIR_TRAVERSE].speed;
	    stats[_STAT_DIR_TRAVERSE].counter++;
	    if(stats[_STAT_DIR_TRAVERSE].speed < stats[_STAT_DIR_TRAVERSE].best)
		 stats[_STAT_DIR_TRAVERSE].best=stats[_STAT_DIR_TRAVERSE].speed;
	    if(stats[_STAT_DIR_TRAVERSE].speed > stats[_STAT_DIR_TRAVERSE].worst)
		 stats[_STAT_DIR_TRAVERSE].worst=stats[_STAT_DIR_TRAVERSE].speed;
	  }
	  stats[_STAT_DIR_TRAVERSE].starttime=time_so_far();
	  junk=chdir("..");
	  stats[_STAT_DIR_TRAVERSE].endtime=time_so_far();
	  stats[_STAT_DIR_TRAVERSE].speed=time1+stats[_STAT_DIR_TRAVERSE].endtime-stats[_STAT_DIR_TRAVERSE].starttime;
	  if(stats[_STAT_DIR_TRAVERSE].speed < (double)0.0)
		stats[_STAT_DIR_TRAVERSE].speed=(double)0.0;
	  stats[_STAT_DIR_TRAVERSE].total_time+=stats[_STAT_DIR_TRAVERSE].speed;
	  stats[_STAT_DIR_TRAVERSE].counter++;
	  if(stats[_STAT_DIR_TRAVERSE].speed < stats[_STAT_DIR_TRAVERSE].best)
	 	stats[_STAT_DIR_TRAVERSE].best=stats[_STAT_DIR_TRAVERSE].speed;
	  if(stats[_STAT_DIR_TRAVERSE].speed > stats[_STAT_DIR_TRAVERSE].worst)
		 stats[_STAT_DIR_TRAVERSE].worst=stats[_STAT_DIR_TRAVERSE].speed;
	}
}

void 
file_create(int x)
{
	int i,j,k;
	int fd;
	int ret;
	char buf[100];
	char value;
	stats[_STAT_CREATE].best=(double)999999.9;
	stats[_STAT_CREATE].worst=(double)0.0;
	stats[_STAT_WRITE].best=(double)999999.9;
	stats[_STAT_WRITE].worst=(double)0.0;
	stats[_STAT_CLOSE].best=(double)999999.9;
	stats[_STAT_CLOSE].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  ret=mkdir(buf,0777);
	  if(ret < 0)
	  {
	      printf("Mkdir failed\n");
	      exit(1);
	  }
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    ret=mkdir(buf,0777);
	    if(ret < 0)
	    {
	      printf("Mkdir failed\n");
	      exit(1);
	    }
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      value=(char) ((i^j^k) & 0xff);
	      memset(mbuffer,value,sz);
	      stats[_STAT_CREATE].starttime=time_so_far();
	      fd=creat(buf,O_RDWR|0600);
	      if(fd < 0)
	      {
	        printf("Create failed\n");
	        exit(1);
	      }
	      stats[_STAT_CREATE].endtime=time_so_far();
	      stats[_STAT_CREATE].speed=stats[_STAT_CREATE].endtime-stats[_STAT_CREATE].starttime;
	      if(stats[_STAT_CREATE].speed < (double)0.0)
		stats[_STAT_CREATE].speed=(double)0.0;
	      stats[_STAT_CREATE].total_time+=stats[_STAT_CREATE].speed;
	      stats[_STAT_CREATE].counter++;
	      if(stats[_STAT_CREATE].speed < stats[_STAT_CREATE].best)
		 stats[_STAT_CREATE].best=stats[_STAT_CREATE].speed;
	      if(stats[_STAT_CREATE].speed > stats[_STAT_CREATE].worst)
		 stats[_STAT_CREATE].worst=stats[_STAT_CREATE].speed;

	      stats[_STAT_WRITE].starttime=time_so_far();
	      junk=write(fd,mbuffer,sz);
	      stats[_STAT_WRITE].endtime=time_so_far();
	      stats[_STAT_WRITE].counter++;
	      stats[_STAT_WRITE].speed=stats[_STAT_WRITE].endtime-stats[_STAT_WRITE].starttime;
	      if(stats[_STAT_WRITE].speed < (double)0.0)
		stats[_STAT_WRITE].speed=(double)0.0;
	      stats[_STAT_WRITE].total_time+=stats[_STAT_WRITE].speed;
	      if(stats[_STAT_WRITE].speed < stats[_STAT_WRITE].best)
		 stats[_STAT_WRITE].best=stats[_STAT_WRITE].speed;
	      if(stats[_STAT_WRITE].speed > stats[_STAT_WRITE].worst)
		 stats[_STAT_WRITE].worst=stats[_STAT_WRITE].speed;

	      fsync(fd);
	      stats[_STAT_CLOSE].starttime=time_so_far();
	      close(fd);
	      stats[_STAT_CLOSE].endtime=time_so_far();
	      stats[_STAT_CLOSE].speed=stats[_STAT_CLOSE].endtime-stats[_STAT_CLOSE].starttime;
	      if(stats[_STAT_CLOSE].speed < (double)0.0)
		stats[_STAT_CLOSE].speed=(double)0.0;
	      stats[_STAT_CLOSE].total_time+=stats[_STAT_CLOSE].speed;
	      stats[_STAT_CLOSE].counter++;
	      if(stats[_STAT_CLOSE].speed < stats[_STAT_CLOSE].best)
		 stats[_STAT_CLOSE].best=stats[_STAT_CLOSE].speed;
	      if(stats[_STAT_CLOSE].speed > stats[_STAT_CLOSE].worst)
		 stats[_STAT_CLOSE].worst=stats[_STAT_CLOSE].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_stat(int x)
{
	int i,j,k,y;
	char buf[100];
	struct stat mystat;
	stats[_STAT_STAT].best=(double)99999.9;
	stats[_STAT_STAT].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      stats[_STAT_STAT].starttime=time_so_far();
	      y=stat(buf,&mystat);
	      if(y < 0)
	      {
	        printf("Stat failed\n");
	        exit(1);
	      }
	      stats[_STAT_STAT].endtime=time_so_far();
	      stats[_STAT_STAT].speed=stats[_STAT_STAT].endtime-stats[_STAT_STAT].starttime;
	      if(stats[_STAT_STAT].speed < (double)0.0)
		stats[_STAT_STAT].speed=(double)0.0;
	      stats[_STAT_STAT].total_time+=stats[_STAT_STAT].speed;
	      stats[_STAT_STAT].counter++;
	      if(stats[_STAT_STAT].speed < stats[_STAT_STAT].best)
		 stats[_STAT_STAT].best=stats[_STAT_STAT].speed;
	      if(stats[_STAT_STAT].speed > stats[_STAT_STAT].worst)
		 stats[_STAT_STAT].worst=stats[_STAT_STAT].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_access(int x)
{
	int i,j,k,y;
	char buf[100];
	stats[_STAT_ACCESS].best=(double)999999.9;
	stats[_STAT_ACCESS].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      stats[_STAT_ACCESS].starttime=time_so_far();
	      y=access(buf,W_OK|F_OK);
	      if(y < 0)
	      {
	        printf("access failed\n");
		perror("what");
	        exit(1);
	      }
	      stats[_STAT_ACCESS].endtime=time_so_far();
	      stats[_STAT_ACCESS].speed=stats[_STAT_ACCESS].endtime-stats[_STAT_ACCESS].starttime;
	      if(stats[_STAT_ACCESS].speed < (double)0.0)
		stats[_STAT_ACCESS].speed=(double)0.0;
	      stats[_STAT_ACCESS].total_time+=stats[_STAT_ACCESS].speed;
	      stats[_STAT_ACCESS].counter++;
	      if(stats[_STAT_ACCESS].speed < stats[_STAT_ACCESS].best)
		 stats[_STAT_ACCESS].best=stats[_STAT_ACCESS].speed;
	      if(stats[_STAT_ACCESS].speed > stats[_STAT_ACCESS].worst)
		 stats[_STAT_ACCESS].worst=stats[_STAT_ACCESS].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_chmod(int x)
{
	int i,j,k,y;
	char buf[100];
	stats[_STAT_CHMOD].best=(double)999999.9;
	stats[_STAT_CHMOD].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      stats[_STAT_CHMOD].starttime=time_so_far();
	      y=chmod(buf,0666);
	      if(y < 0)
	      {
	        printf("chmod failed\n");
		perror("what");
	        exit(1);
	      }
	      stats[_STAT_CHMOD].endtime=time_so_far();
	      stats[_STAT_CHMOD].speed=stats[_STAT_CHMOD].endtime-stats[_STAT_CHMOD].starttime;
	      if(stats[_STAT_CHMOD].speed < (double)0.0)
		stats[_STAT_CHMOD].speed=(double)0.0;
	      stats[_STAT_CHMOD].total_time+=stats[_STAT_CHMOD].speed;
	      stats[_STAT_CHMOD].counter++;
	      if(stats[_STAT_CHMOD].speed < stats[_STAT_CHMOD].best)
		 stats[_STAT_CHMOD].best=stats[_STAT_CHMOD].speed;
	      if(stats[_STAT_CHMOD].speed > stats[_STAT_CHMOD].worst)
		 stats[_STAT_CHMOD].worst=stats[_STAT_CHMOD].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_readdir(int x)
{
	int i,j,ret1;
	char buf[100];
	DIR *dirbuf;
	struct dirent *y;
	stats[_STAT_READDIR].best=(double)999999.9;
	stats[_STAT_READDIR].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    dirbuf=opendir(".");
	    if(dirbuf==0)
	    {
		printf("opendir failed\n");
		exit(1);
	    }
	    stats[_STAT_READDIR].starttime=time_so_far();
	    y=readdir(dirbuf);
	    if(y == 0)
	    {
	      printf("readdir failed\n");
	      exit(1);
	    }
	    stats[_STAT_READDIR].endtime=time_so_far();
	    stats[_STAT_READDIR].speed=stats[_STAT_READDIR].endtime-stats[_STAT_READDIR].starttime;
	      if(stats[_STAT_READDIR].speed < (double)0.0)
		stats[_STAT_READDIR].speed=(double)0.0;
	    stats[_STAT_READDIR].total_time+=stats[_STAT_READDIR].speed;
	    stats[_STAT_READDIR].counter++;
	    if(stats[_STAT_READDIR].speed < stats[_STAT_READDIR].best)
		 stats[_STAT_READDIR].best=stats[_STAT_READDIR].speed;
	    if(stats[_STAT_READDIR].speed > stats[_STAT_READDIR].worst)
		 stats[_STAT_READDIR].worst=stats[_STAT_READDIR].speed;
	    ret1=closedir(dirbuf);
	    if(ret1 < 0)
	    {
	      printf("closedir failed\n");
	      exit(1);
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_link(int x)
{
	int i,j,k,y;
	char buf[100];
	char bufn[100];
	stats[_STAT_LINK].best=(double)999999.9;
	stats[_STAT_LINK].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      sprintf(bufn,"fileop_file_%d_%d_%dL",i,j,k);
	      stats[_STAT_LINK].starttime=time_so_far();
	      y=link(buf,bufn);
	      if(y < 0)
	      {
	        printf("Link failed\n");
	        exit(1);
	      }
	      stats[_STAT_LINK].endtime=time_so_far();
	      stats[_STAT_LINK].speed=stats[_STAT_LINK].endtime-stats[_STAT_LINK].starttime;
	      if(stats[_STAT_LINK].speed < (double)0.0)
		stats[_STAT_LINK].speed=(double)0.0;
	      stats[_STAT_LINK].total_time+=stats[_STAT_LINK].speed;
	      stats[_STAT_LINK].counter++;
	      if(stats[_STAT_LINK].speed < stats[_STAT_LINK].best)
		 stats[_STAT_LINK].best=stats[_STAT_LINK].speed;
	      if(stats[_STAT_LINK].speed > stats[_STAT_LINK].worst)
		 stats[_STAT_LINK].worst=stats[_STAT_LINK].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_unlink(int x)
{
	int i,j,k,y;
	char buf[100];
	char bufn[100];
	stats[_STAT_UNLINK].best=(double)999999.9;
	stats[_STAT_UNLINK].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      sprintf(bufn,"fileop_file_%d_%d_%dL",i,j,k);
	      stats[_STAT_UNLINK].starttime=time_so_far();
	      y=unlink(bufn);
	      if(y < 0)
	      {
	        printf("Unlink failed\n");
	        exit(1);
	      }
	      stats[_STAT_UNLINK].endtime=time_so_far();
	      stats[_STAT_UNLINK].speed=stats[_STAT_UNLINK].endtime-stats[_STAT_UNLINK].starttime;
	      if(stats[_STAT_UNLINK].speed < (double)0.0)
		stats[_STAT_UNLINK].speed=(double)0.0;
	      stats[_STAT_UNLINK].total_time+=stats[_STAT_UNLINK].speed;
	      stats[_STAT_UNLINK].counter++;
	      if(stats[_STAT_UNLINK].speed < stats[_STAT_UNLINK].best)
		 stats[_STAT_UNLINK].best=stats[_STAT_UNLINK].speed;
	      if(stats[_STAT_UNLINK].speed > stats[_STAT_UNLINK].worst)
		 stats[_STAT_UNLINK].worst=stats[_STAT_UNLINK].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void
dir_delete(int x)
{
	int i,j,k;
	char buf[100];
	stats[_STAT_DIR_DELETE].best=(double)99999.9;
	stats[_STAT_DIR_DELETE].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_dir_%d_%d_%d",i,j,k);
	      junk=chdir(buf);
	      junk=chdir("..");
	      stats[_STAT_DIR_DELETE].starttime=time_so_far();
	      rmdir(buf);
	      stats[_STAT_DIR_DELETE].endtime=time_so_far();
	      stats[_STAT_DIR_DELETE].speed=stats[_STAT_DIR_DELETE].endtime-stats[_STAT_DIR_DELETE].starttime;
	      if(stats[_STAT_DIR_DELETE].speed < (double)0.0)
		stats[_STAT_DIR_DELETE].speed=(double)0.0;
	      stats[_STAT_DIR_DELETE].total_time+=stats[_STAT_DIR_DELETE].speed;
	      stats[_STAT_DIR_DELETE].counter++;
	      if(stats[_STAT_DIR_DELETE].speed < stats[_STAT_DIR_DELETE].best)
		 stats[_STAT_DIR_DELETE].best=stats[_STAT_DIR_DELETE].speed;
	      if(stats[_STAT_DIR_DELETE].speed > stats[_STAT_DIR_DELETE].worst)
		 stats[_STAT_DIR_DELETE].worst=stats[_STAT_DIR_DELETE].speed;
	    }
	    junk=chdir("..");
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    stats[_STAT_DIR_DELETE].starttime=time_so_far();
	    rmdir(buf);
	    stats[_STAT_DIR_DELETE].endtime=time_so_far();
	    stats[_STAT_DIR_DELETE].speed=stats[_STAT_DIR_DELETE].endtime-stats[_STAT_DIR_DELETE].starttime;
	      if(stats[_STAT_DIR_DELETE].speed < (double)0.0)
		stats[_STAT_DIR_DELETE].speed=(double)0.0;
	    stats[_STAT_DIR_DELETE].total_time+=stats[_STAT_DIR_DELETE].speed;
	    stats[_STAT_DIR_DELETE].counter++;
	    if(stats[_STAT_DIR_DELETE].speed < stats[_STAT_DIR_DELETE].best)
		 stats[_STAT_DIR_DELETE].best=stats[_STAT_DIR_DELETE].speed;
	    if(stats[_STAT_DIR_DELETE].speed > stats[_STAT_DIR_DELETE].worst)
		 stats[_STAT_DIR_DELETE].worst=stats[_STAT_DIR_DELETE].speed;
	  }
	  junk=chdir("..");
	  sprintf(buf,"fileop_L1_%d",i);
	  stats[_STAT_DIR_DELETE].starttime=time_so_far();
	  rmdir(buf);
	  stats[_STAT_DIR_DELETE].endtime=time_so_far();
	  stats[_STAT_DIR_DELETE].speed=stats[_STAT_DIR_DELETE].endtime-stats[_STAT_DIR_DELETE].starttime;
	  if(stats[_STAT_DIR_DELETE].speed < (double)0.0)
		stats[_STAT_DIR_DELETE].speed=(double)0.0;
	  stats[_STAT_DIR_DELETE].total_time+=stats[_STAT_DIR_DELETE].speed;
	  stats[_STAT_DIR_DELETE].counter++;
	  if(stats[_STAT_DIR_DELETE].speed < stats[_STAT_DIR_DELETE].best)
		 stats[_STAT_DIR_DELETE].best=stats[_STAT_DIR_DELETE].speed;
	  if(stats[_STAT_DIR_DELETE].speed > stats[_STAT_DIR_DELETE].worst)
		 stats[_STAT_DIR_DELETE].worst=stats[_STAT_DIR_DELETE].speed;
	}
}

void
file_delete(int x)
{
	int i,j,k;
	char buf[100];
	stats[_STAT_DELETE].best=(double)999999.9;
	stats[_STAT_DELETE].worst=(double)0.0;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      stats[_STAT_DELETE].starttime=time_so_far();
	      unlink(buf);
	      stats[_STAT_DELETE].endtime=time_so_far();
	      stats[_STAT_DELETE].speed=stats[_STAT_DELETE].endtime-stats[_STAT_DELETE].starttime;
	      if(stats[_STAT_DELETE].speed < (double)0.0)
		stats[_STAT_DELETE].speed=(double)0.0;
	      stats[_STAT_DELETE].total_time+=stats[_STAT_DELETE].speed;
	      stats[_STAT_DELETE].counter++;
	      if(stats[_STAT_DELETE].speed < stats[_STAT_DELETE].best)
		 stats[_STAT_DELETE].best=stats[_STAT_DELETE].speed;
	      if(stats[_STAT_DELETE].speed > stats[_STAT_DELETE].worst)
		 stats[_STAT_DELETE].worst=stats[_STAT_DELETE].speed;
	    }
	    junk=chdir("..");
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    rmdir(buf);
	  }
	  junk=chdir("..");
	  sprintf(buf,"fileop_L1_%d",i);
	  rmdir(buf);
	}
}
void 
file_read(int x)
{
	int i,j,k,y,fd;
	char buf[100];
	char value;
	stats[_STAT_READ].best=(double)99999.9;
	stats[_STAT_READ].worst=(double)0.00000000;
	stats[_STAT_OPEN].best=(double)99999.9;
	stats[_STAT_OPEN].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprintf(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprintf(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++)
	    {
	      sprintf(buf,"fileop_file_%d_%d_%d",i,j,k);
	      value=(char)((i^j^k) &0xff);
	      stats[_STAT_OPEN].starttime=time_so_far();
	      fd=open(buf,O_RDONLY);
	      if(fd < 0)
	      {
	        printf("Open failed\n");
	        exit(1);
	      }
	      stats[_STAT_OPEN].endtime=time_so_far();
	      stats[_STAT_OPEN].speed=stats[_STAT_OPEN].endtime-stats[_STAT_OPEN].starttime;
	      if(stats[_STAT_OPEN].speed < (double)0.0)
		stats[_STAT_OPEN].speed=(double)0.0;
	      stats[_STAT_OPEN].total_time+=stats[_STAT_OPEN].speed;
	      stats[_STAT_OPEN].counter++;
	      if(stats[_STAT_OPEN].speed < stats[_STAT_OPEN].best)
		 stats[_STAT_OPEN].best=stats[_STAT_OPEN].speed;
	      if(stats[_STAT_OPEN].speed > stats[_STAT_OPEN].worst)
		 stats[_STAT_OPEN].worst=stats[_STAT_OPEN].speed;

	      stats[_STAT_READ].starttime=time_so_far();
	      y=read(fd,mbuffer,sz);
	      if(y < 0)
	      {
	        printf("Read failed\n");
	        exit(1);
	      }
	      if(validate(mbuffer,sz, value) !=0)
		printf("Error: Data Mis-compare\n");;
	      stats[_STAT_READ].endtime=time_so_far();
	      close(fd);
	      stats[_STAT_READ].speed=stats[_STAT_READ].endtime-stats[_STAT_READ].starttime;
	      if(stats[_STAT_READ].speed < (double)0.0)
		stats[_STAT_READ].speed=(double)0.0;
	      stats[_STAT_READ].total_time+=stats[_STAT_READ].speed;
	      stats[_STAT_READ].counter++;
	      if(stats[_STAT_READ].speed < stats[_STAT_READ].best)
		 stats[_STAT_READ].best=stats[_STAT_READ].speed;
	      if(stats[_STAT_READ].speed > stats[_STAT_READ].worst)
		 stats[_STAT_READ].worst=stats[_STAT_READ].speed;
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

/************************************************************************/
/* Time measurement routines. Thanks to Iozone :-)			*/
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
	/* For Windows the time_of_day() is useless. It increments in 55 milli second   */
	/* increments. By using the Win32api one can get access to the high performance */
	/* measurement interfaces. With this one can get back into the 8 to 9  		*/
	/* microsecond resolution.							*/
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);
        bigcounter=(double)counter.HighPart *(double)0xffffffff +
                (double)counter.LowPart;
        wintime = (double)(bigcounter/(double)freq.LowPart);
        return((double)wintime);
#else
#if defined (OSFV4) || defined(OSFV3) || defined(OSFV5)
  struct timespec gp;

  if (getclock(TIMEOFDAY, (struct timespec *) &gp) == -1)
    perror("getclock");
  return (( (double) (gp.tv_sec)) +
    ( ((float)(gp.tv_nsec)) * 0.000000001 ));
#else
  struct timeval tp;

  if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
    perror("gettimeofday");
  return ((double) (tp.tv_sec)) +
    (((double) tp.tv_usec) * 0.000001 );
#endif
#endif
}

void
splash(void)
{
	printf("\n");
	printf("     --------------------------------------\n");
	printf("     |              Fileop                | \n");
	printf("     | %s          | \n",version);
	printf("     |                                    | \n");
	printf("     |                by                  |\n");
	printf("     |                                    | \n");
	printf("     |             Don Capps              |\n");
	printf("     --------------------------------------\n");
	printf("\n");
}

void 
usage(void)
{
  splash();
  printf("     fileop [-f X ]|[-l # -u #] [-s Y] [-e] [-b] [-w] [-d <dir>] [-t] [-v] [-h]\n");
  printf("\n");
  printf("     -f #      Force factor. X^3 files will be created and removed.\n");
  printf("     -l #      Lower limit on the value of the Force factor.\n");
  printf("     -u #      Upper limit on the value of the Force factor.\n");
  printf("     -s #      Optional. Sets filesize for the create/write. May use suffix 'K' or 'M'.\n");
  printf("     -e        Excel importable format.\n");
  printf("     -b        Output best case results.\n");
  printf("     -i #      Increment force factor by this increment.\n");
  printf("     -w        Output worst case results.\n");
  printf("     -d <dir>  Specify starting directory.\n");
  printf("     -U <dir>  Mount point to remount between tests.\n");
  printf("     -t        Verbose output option.\n");
  printf("     -v        Version information.\n");
  printf("     -h        Help text.\n");
  printf("\n");
  printf("     The structure of the file tree is:\n");
  printf("     X number of Level 1 directories, with X number of\n");
  printf("     level 2 directories, with X number of files in each\n");
  printf("     of the level 2 directories.\n");
  printf("\n");
  printf("     Example:  fileop 2\n");
  printf("\n");
  printf("             dir_1                        dir_2\n");
  printf("            /     \\                      /     \\ \n");
  printf("      sdir_1       sdir_2          sdir_1       sdir_2\n");
  printf("      /     \\     /     \\          /     \\      /     \\ \n");
  printf("   file_1 file_2 file_1 file_2   file_1 file_2 file_1 file_2\n");
  printf("\n");
  printf("   Each file will be created, and then Y bytes is written to the file.\n");
  printf("\n");
}
void
clear_stats()
{
	int i;
	for(i=0;i<_NUM_STATS;i++)
		bzero((char *)&stats[i],sizeof(struct stat_struct));
}
int
validate(char *buffer, int size, char value)
{
	register int i;
	register char *cp;
	register char v1;
	v1=value;
	cp = buffer;
	for(i=0;i<size;i++)
	{
		if(*cp++ != v1)
			return(1);
	}
	return(0);
}

