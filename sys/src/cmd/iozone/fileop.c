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
 *   Example:  fileop -f 2
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

#include <u.h>
#include <libc.h>

enum{
	pathMax =		255,
	statCreate =	0,
	statWrite = 	1,
	statClose = 	2,
	statDelete =	5,
	statStat =		6,
	statAccess =	7,
	statChmod =		8,
	statReaddir =	9,
	statDirCreate =	10,
	statDirDelete =	11,
	statRead =		12,
	statOpen =		13,
	statDirTraverse=14,
	numStats =		15
};


int junk, *junkp;
int x,excel;
int verbose = 0;
int sz = 1;
char *mbuffer;
int incr = 1;



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
} volatile stats[numStats];

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
void file_read(int);
void splash(void);
void usage(void);
void bzero();
void clear_stats();
int validate(char *, int , char );
char *getcwd(char *buf, size_t len);
void rmdir(char *);
void be2vlong(int64_t *, char *);
	
char version[]="        $Revision: 1.61 $";
char thedir[pathMax]="."; /* Default is to use the current directory */
const char *mountname=nil; /* Default is not to unmount anything between the tests */
const char *servername;

int cret;
int lower, upper,range;
int i;
int best, worst;
int dirlen;

/************************************************************************/
/* Routine to purge the buffer cache by unmounting drive.		*/
/************************************************************************/
void purge_buffer_cache(){
	int fd;
	if (!mountname)
		return;

	char cwd[pathMax];
	
	int ret,i;

	junkp=(int *)getcwd(cwd, sizeof(cwd));
	junk=chdir("/");
	
        /*
           umount might fail if the device is still busy, so
           retry unmounting several times with increasing delays
        */
        for (i = 1; i < 10; ++i) {
			ret = unmount(nil, mountname);
			if (ret == 0)
				break;
			sleep(i); /* seconds */
        }

	fd = open(servername, ORDWR);
	if(fd < 0){
		fprint(2, "can't open %s", servername);
		exits("open");
	}
	mount(fd, -1, mountname, 0, "", 'M');

	junk=chdir(cwd);
}

int main(int argc, char **argv)
{
	if(argc == 1)
	{
		usage();
		exits("usage");
	}
	
	char *aux;
	
	ARGBEGIN{
		case 'h':
			usage();
			exits(0);
			break;
		case 'd':
			aux=ARGF();
			dirlen=strlen(aux);
			if (aux[dirlen-1]=='/')
				--dirlen;
			strncpy(thedir, aux, dirlen);
			thedir[dirlen] = 0;
			break;
		case 'U':
			mountname = ARGF();
			break;
		case 'i':	/* Increment force by */
			incr=atoi(ARGF());
			if(incr < 0)
				incr=1;
			break;
		case 'f':	/* Force factor */
			x=atoi(ARGF());
			if(x < 0)
				x=1;
			break;
		case 's':	/* Size of files */
			aux=ARGF();
			sz=atoi(aux);
			int szl=strlen(aux)-1;
			if(aux[szl]=='k' || aux[szl]=='K'){
				sz *= 1024;
            } else if(aux[szl]=='m' || aux[szl]=='M'){
				sz *= 1024 * 1024;
			}
			if(sz < 0)
				sz=1;
			break;
		case 'l':	/* lower force value */
			lower=atoi(ARGF());
			range=1;
			if(lower < 0)
				lower=1;
			break;
		case 'v':	/* version */
			splash();
			exits(0);
			break;
		case 'u':	/* upper force value */
			upper=atoi(ARGF());
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
		default:
			print("Bad flag: '%c' \n", ARGC());
			exits("badflag");
	}ARGEND
	
	
	mbuffer=(char *)malloc(sz);
	memset(mbuffer,'a',sz);
	if(!excel)
	  print("\nFileop:  Working in %s, File size is %d,  Output is in Ops/sec. (A=Avg, B=Best, W=Worst)\n", thedir, sz);
	if(!verbose){
	   	print(" .     %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %7s %10s\n",
       	   	"mkdir","chdir","rmdir","create","open", "read","write","close","stat",
		"access","chmod","readdir","delete", " Total_files");
	}
	junk=chdir(thedir); /* change starting point */
	if(x==0)
		x=1;
	if(range==0)
		lower=upper=x;
	for(i=lower;i<=upper;i+=incr){
		clear_stats();
		x=i;
	   /*
	    * Dir Create test 
	    */
	   purge_buffer_cache();
	   dir_create(x);

		if(verbose){
			print("mkdir:   Dirs = %9lld ",stats[statDirCreate].counter);
			print("Total Time = %12.9f seconds\n", stats[statDirCreate].total_time);
			print("         Avg mkdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[statDirCreate].counter/stats[statDirCreate].total_time,
			stats[statDirCreate].total_time/stats[statDirCreate].counter);
			print("         Best mkdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[statDirCreate].best,stats[statDirCreate].best);
			print("         Worst mkdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[statDirCreate].worst,stats[statDirCreate].worst);
	   }

	   /*
	    * Dir Traverse test
	    */
	   purge_buffer_cache();
	   dir_traverse(x);

	   if(verbose){
		   print("chdir:   Dirs = %9lld ",stats[statDirTraverse].counter);
		   print("Total Time = %12.9f seconds\n", stats[statDirTraverse].total_time);
		   print("         Avg chdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
		   stats[statDirTraverse].counter/stats[statDirTraverse].total_time,
		   stats[statDirTraverse].total_time/stats[statDirTraverse].counter);
		   print("         Best chdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[statDirTraverse].best,stats[statDirTraverse].best);
		   print("         Worst chdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[statDirTraverse].worst,stats[statDirTraverse].worst);
	   }

	   /*
	    * Dir delete test
	    */
	   purge_buffer_cache();
	   dir_delete(x);
 
	   if(verbose){
		   print("rmdir:   Dirs = %9lld ",stats[statDirDelete].counter);
		   print("Total Time = %12.9f seconds\n",stats[statDirDelete].total_time);
		   print("         Avg rmdir(s)/sec     = %12.2f (%12.9f seconds/op)\n",
		   stats[statDirDelete].counter/stats[statDirDelete].total_time,
		   stats[statDirDelete].total_time/stats[statDirDelete].counter);
		   print("         Best rmdir(s)/sec    = %12.2f (%12.9f seconds/op)\n",1/stats[statDirDelete].best,stats[statDirDelete].best);
		   print("         Worst rmdir(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",1/stats[statDirDelete].worst,stats[statDirDelete].worst);
	   }

	   /*
	    * Create test 
	    */
		purge_buffer_cache();
		file_create(x);
		if(verbose){
		   print("create:  Files = %9lld ",stats[statCreate].counter);
		   print("Total Time = %12.9f seconds\n", stats[statCreate].total_time);
		   print("         Avg create(s)/sec    = %12.2f (%12.9f seconds/op)\n",
		   stats[statCreate].counter/stats[statCreate].total_time,
		   stats[statCreate].total_time/stats[statCreate].counter);
		   print("         Best create(s)/sec   = %12.2f (%12.9f seconds/op)\n",
		   1/stats[statCreate].best,stats[statCreate].best);
		   print("         Worst create(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
		   1/stats[statCreate].worst,stats[statCreate].worst);
		   print("write:   Files = %9lld ",stats[statWrite].counter);
		   print("Total Time = %12.9f seconds\n", stats[statWrite].total_time);
		   print("         Avg write(s)/sec     = %12.2f (%12.9f seconds/op)\n",
		   stats[statWrite].counter/stats[statWrite].total_time,
		   stats[statWrite].total_time/stats[statWrite].counter);
		   print("         Best write(s)/sec    = %12.2f (%12.9f seconds/op)\n",
		   1/stats[statWrite].best,stats[statWrite].best);
		   print("         Worst write(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
		   1/stats[statWrite].worst,stats[statWrite].worst);
		   print("close:   Files = %9lld ",stats[statClose].counter);
		   print("Total Time = %12.9f seconds\n", stats[statClose].total_time);
		   print("         Avg close(s)/sec     = %12.2f (%12.9f seconds/op)\n",
		   stats[statClose].counter/stats[statClose].total_time,
		   stats[statClose].total_time/stats[statClose].counter);
		   print("         Best close(s)/sec    = %12.2f (%12.9f seconds/op)\n",
		   1/stats[statClose].best,stats[statClose].best);
		   print("         Worst close(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
		   1/stats[statClose].worst,stats[statClose].worst);
		}

	   /*
	    * Stat test 
	    */
		purge_buffer_cache();
		file_stat(x);

		if(verbose){
			print("stat:    Files = %9lld ",stats[statStat].counter);
			print("Total Time = %12.9f seconds\n", stats[statStat].total_time);
			print("         Avg stat(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[statStat].counter/stats[statStat].total_time,
			stats[statStat].total_time/stats[statStat].counter);
			print("         Best stat(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[statStat].best,stats[statStat].best);
			print("         Worst stat(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statStat].worst,stats[statStat].worst);
		}
	   /*
	    * Read test 
	    */
		purge_buffer_cache();
		file_read(x);

		if(verbose){
			print("open:    Files = %9lld ",stats[statOpen].counter);
			print("Total Time = %12.9f seconds\n", stats[statOpen].total_time);
			print("         Avg open(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[statOpen].counter/stats[statOpen].total_time,
			stats[statOpen].total_time/stats[statOpen].counter);
			print("         Best open(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[statOpen].best,stats[statOpen].best);
			print("         Worst open(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statOpen].worst,stats[statOpen].worst);

			print("read:    Files = %9lld ",stats[statRead].counter);
			print("Total Time = %12.9f seconds\n", stats[statRead].total_time);
			print("         Avg read(s)/sec      = %12.2f (%12.9f seconds/op)\n",
			stats[statRead].counter/stats[statRead].total_time,
			stats[statRead].total_time/stats[statRead].counter);
			print("         Best read(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			1/stats[statRead].best,stats[statRead].best);
			print("         Worst read(s)/sec    = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statRead].worst,stats[statRead].worst);
	   }

	   /*
	    * Access test 
	    */
		purge_buffer_cache();
		file_access(x);
		if(verbose){
			print("access:  Files = %9lld ",stats[statAccess].counter);
			print("Total Time = %12.9f seconds\n", stats[statAccess].total_time);
			print("         Avg access(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[statAccess].counter/stats[statAccess].total_time,
			stats[statAccess].total_time/stats[statAccess].counter);
			print("         Best access(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[statAccess].best,stats[statAccess].best);
			print("         Worst access(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statAccess].worst,stats[statAccess].worst);
		}
	   /*
	    * Chmod test 
	    */
		purge_buffer_cache();
		file_chmod(x);

		if(verbose){
			print("chmod:   Files = %9lld ",stats[statChmod].counter);
			print("Total Time = %12.9f seconds\n", stats[statChmod].total_time);
			print("         Avg chmod(s)/sec     = %12.2f (%12.9f seconds/op)\n",
			stats[statChmod].counter/stats[statChmod].total_time,
			stats[statChmod].total_time/stats[statChmod].counter);
			print("         Best chmod(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			1/stats[statChmod].best,stats[statChmod].best);
			print("         Worst chmod(s)/sec   = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statChmod].worst,stats[statChmod].worst);
		}
	   /*
	    * readdir test 
	    */
		purge_buffer_cache();
		file_readdir(x);

		if(verbose){
			print("readdir: Files = %9lld ",stats[statReaddir].counter);
			print("Total Time = %12.9f seconds\n", stats[statReaddir].total_time);
			print("         Avg readdir(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			stats[statReaddir].counter/stats[statReaddir].total_time,
			stats[statReaddir].total_time/stats[statReaddir].counter);
			print("         Best readdir(s)/sec  = %12.2f (%12.9f seconds/op)\n",
			1/stats[statReaddir].best,stats[statReaddir].best);
			print("         Worst readdir(s)/sec = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statReaddir].worst,stats[statReaddir].worst);
		}

	   /*
	    * Delete test 
	    */
		purge_buffer_cache();
		file_delete(x);
		if(verbose){
			print("delete:  Files = %9lld ",stats[statDelete].counter);
			print("Total Time = %12.9f seconds\n", stats[statDelete].total_time);
			print("         Avg delete(s)/sec    = %12.2f (%12.9f seconds/op)\n",
			stats[statDelete].counter/stats[statDelete].total_time,
			stats[statDelete].total_time/stats[statDelete].counter);
			print("         Best delete(s)/sec   = %12.2f (%12.9f seconds/op)\n",
			1/stats[statDelete].best,stats[statDelete].best);
			print("         Worst delete(s)/sec  = %12.2f (%12.9f seconds/op)\n\n",
			1/stats[statDelete].worst,stats[statDelete].worst);
		}
		if(!verbose){
			print("%c %4d %7.0f ",'A',x,stats[statDirCreate].counter/stats[statDirCreate].total_time);
			print("%7.0f ",stats[statDirTraverse].counter/stats[statDirTraverse].total_time);
			print("%7.0f ",stats[statDirDelete].counter/stats[statDirDelete].total_time);
			print("%7.0f ",stats[statCreate].counter/stats[statCreate].total_time);
			print("%7.0f ",stats[statOpen].counter/stats[statOpen].total_time);
			print("%7.0f ",stats[statRead].counter/stats[statRead].total_time);
			print("%7.0f ",stats[statWrite].counter/stats[statWrite].total_time);
			print("%7.0f ",stats[statClose].counter/stats[statClose].total_time);
			print("%7.0f ",stats[statStat].counter/stats[statStat].total_time);
			print("%7.0f ",stats[statAccess].counter/stats[statAccess].total_time);
			print("%7.0f ",stats[statChmod].counter/stats[statChmod].total_time);
			print("%7.0f ",stats[statReaddir].counter/stats[statReaddir].total_time);
			print("%7.0f ",stats[statDelete].counter/stats[statDelete].total_time);
			print("%10d ",x*x*x);
			print("\n");

			if(best){
				print("%c %4d %7.0f ",'B',x, 1/stats[statDirCreate].best);
				print("%7.0f ",1/stats[statDirTraverse].best);
				print("%7.0f ",1/stats[statDirDelete].best);
				print("%7.0f ",1/stats[statCreate].best);
				print("%7.0f ",1/stats[statOpen].best);
				print("%7.0f ",1/stats[statRead].best);
				print("%7.0f ",1/stats[statWrite].best);
				print("%7.0f ",1/stats[statClose].best);
				print("%7.0f ",1/stats[statStat].best);
				print("%7.0f ",1/stats[statAccess].best);
				print("%7.0f ",1/stats[statChmod].best);
				print("%7.0f ",1/stats[statReaddir].best);
				print("%7.0f ",1/stats[statDelete].best);
				print("%10d ",x*x*x);
				print("\n");
				//fflush(stdout);
			}
			if(worst){
				print("%c %4d %7.0f ",'W',x, 1/stats[statDirCreate].worst);
				print("%7.0f ",1/stats[statDirTraverse].worst);
				print("%7.0f ",1/stats[statDirDelete].worst);
				print("%7.0f ",1/stats[statCreate].worst);
				print("%7.0f ",1/stats[statOpen].worst);
				print("%7.0f ",1/stats[statRead].worst);
				print("%7.0f ",1/stats[statWrite].worst);
				print("%7.0f ",1/stats[statClose].worst);
				print("%7.0f ",1/stats[statStat].worst);
				print("%7.0f ",1/stats[statAccess].worst);
				print("%7.0f ",1/stats[statChmod].worst);
				print("%7.0f ",1/stats[statReaddir].worst);
				print("%7.0f ",1/stats[statDelete].worst);
				print("%10d ",x*x*x);
				print("\n");
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
	stats[statDirCreate].best=(double)99999.9;
	stats[statDirCreate].worst=(double)0.00000000;
	for(i=0;i<x;i++)
	{
	  sprint(buf,"fileop_L1_%d",i);
	  stats[statDirCreate].starttime=time_so_far();
	  ret = create(buf, OREAD, DMDIR | 0777);
	  if(ret < 0)
	  {
	      print("Mkdir failed\n");
	      exits("mkdir");
	  }
	  close(ret);
	  
	  stats[statDirCreate].endtime=time_so_far();
	  stats[statDirCreate].speed=stats[statDirCreate].endtime-stats[statDirCreate].starttime;
	  if(stats[statDirCreate].speed < (double)0.0)
		stats[statDirCreate].speed=(double)0.0;
	  stats[statDirCreate].total_time+=stats[statDirCreate].speed;
	  stats[statDirCreate].counter++;
	  if(stats[statDirCreate].speed < stats[statDirCreate].best)
	 	stats[statDirCreate].best=stats[statDirCreate].speed;
	  if(stats[statDirCreate].speed > stats[statDirCreate].worst)
		 stats[statDirCreate].worst=stats[statDirCreate].speed;
	  junk=chdir(buf);
	  for(j=0;j<x;j++)
	  {
	    sprint(buf,"fileop_L1_%d_L2_%d",i,j);
	    stats[statDirCreate].starttime=time_so_far();
	    ret = create(buf, OREAD, DMDIR | 0777);
		if(ret < 0){
			print("Mkdir failed\n");
			exits("mkdir");
		}
		close(ret);
	    stats[statDirCreate].endtime=time_so_far();
	    stats[statDirCreate].speed=stats[statDirCreate].endtime-stats[statDirCreate].starttime;
	    if(stats[statDirCreate].speed < (double)0.0)
		stats[statDirCreate].speed=(double) 0.0;
	    stats[statDirCreate].total_time+=stats[statDirCreate].speed;
	    stats[statDirCreate].counter++;
	    if(stats[statDirCreate].speed < stats[statDirCreate].best)
			stats[statDirCreate].best=stats[statDirCreate].speed;
	    if(stats[statDirCreate].speed > stats[statDirCreate].worst)
			stats[statDirCreate].worst=stats[statDirCreate].speed;
	    junk=chdir(buf);
	    for(k=0;k<x;k++){
			sprint(buf,"fileop_dir_%d_%d_%d",i,j,k);
			stats[statDirCreate].starttime=time_so_far();
			ret = create(buf, OREAD, DMDIR | 0777);
			if(ret < 0){
				print("Mkdir failed\n");
				exits("mkdir");
		}
		close(ret);
	    stats[statDirCreate].endtime=time_so_far();
	    stats[statDirCreate].speed=stats[statDirCreate].endtime-stats[statDirCreate].starttime;
	    if(stats[statDirCreate].speed < (double)0.0)
			stats[statDirCreate].speed=(double) 0.0;
	    stats[statDirCreate].total_time+=stats[statDirCreate].speed;
	    stats[statDirCreate].counter++;
	    if(stats[statDirCreate].speed < stats[statDirCreate].best)
			stats[statDirCreate].best=stats[statDirCreate].speed;
	    if(stats[statDirCreate].speed > stats[statDirCreate].worst)
			stats[statDirCreate].worst=stats[statDirCreate].speed;
	    junk=chdir(buf);
	    junk=chdir("..");
	    }
	    junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void
dir_traverse(int x){
	int i,j,k;
	char buf[100];
	double time1, time2;
	stats[statDirTraverse].best=(double)99999.9;
	stats[statDirTraverse].worst=(double)0.00000000;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		stats[statDirTraverse].starttime=time_so_far();
		junk=chdir(buf);
		stats[statDirTraverse].endtime=time_so_far();
		time1=stats[statDirTraverse].endtime-stats[statDirTraverse].starttime;
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			stats[statDirTraverse].starttime=time_so_far();
			junk=chdir(buf);
			stats[statDirTraverse].endtime=time_so_far();
			time2=stats[statDirTraverse].endtime-stats[statDirTraverse].starttime;
			for(k=0;k<x;k++){
				sprint(buf,"fileop_dir_%d_%d_%d",i,j,k);
				stats[statDirTraverse].starttime=time_so_far();
				junk=chdir(buf);
				junk=chdir("..");
				stats[statDirTraverse].endtime=time_so_far();
				stats[statDirTraverse].speed=stats[statDirTraverse].endtime-stats[statDirTraverse].starttime;
				if(stats[statDirTraverse].speed < (double)0.0)
					stats[statDirTraverse].speed=(double) 0.0;
				stats[statDirTraverse].total_time+=stats[statDirTraverse].speed;
				stats[statDirTraverse].counter++;
				if(stats[statDirTraverse].speed < stats[statDirTraverse].best)
					stats[statDirTraverse].best=stats[statDirTraverse].speed;
				if(stats[statDirTraverse].speed > stats[statDirTraverse].worst)
					stats[statDirTraverse].worst=stats[statDirTraverse].speed;
			}
			stats[statDirTraverse].starttime=time_so_far();
			junk=chdir("..");
			stats[statDirTraverse].endtime=time_so_far();
			stats[statDirTraverse].speed=time2+stats[statDirTraverse].endtime-stats[statDirTraverse].starttime;
			if(stats[statDirTraverse].speed < (double)0.0)
				stats[statDirTraverse].speed=(double) 0.0;
			stats[statDirTraverse].total_time+=stats[statDirTraverse].speed;
			stats[statDirTraverse].counter++;
			if(stats[statDirTraverse].speed < stats[statDirTraverse].best)
				stats[statDirTraverse].best=stats[statDirTraverse].speed;
			if(stats[statDirTraverse].speed > stats[statDirTraverse].worst)
				stats[statDirTraverse].worst=stats[statDirTraverse].speed;
		}
		stats[statDirTraverse].starttime=time_so_far();
		junk=chdir("..");
		stats[statDirTraverse].endtime=time_so_far();
		stats[statDirTraverse].speed=time1+stats[statDirTraverse].endtime-stats[statDirTraverse].starttime;
		if(stats[statDirTraverse].speed < (double)0.0)
			stats[statDirTraverse].speed=(double)0.0;
		stats[statDirTraverse].total_time+=stats[statDirTraverse].speed;
		stats[statDirTraverse].counter++;
		if(stats[statDirTraverse].speed < stats[statDirTraverse].best)
			stats[statDirTraverse].best=stats[statDirTraverse].speed;
		if(stats[statDirTraverse].speed > stats[statDirTraverse].worst)
			stats[statDirTraverse].worst=stats[statDirTraverse].speed;
	}
}

void 
file_create(int x){
	int i,j,k;
	int fd;
	int ret;
	char buf[100];
	char value;
	stats[statCreate].best=(double)999999.9;
	stats[statCreate].worst=(double)0.0;
	stats[statWrite].best=(double)999999.9;
	stats[statWrite].worst=(double)0.0;
	stats[statClose].best=(double)999999.9;
	stats[statClose].worst=(double)0.0;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		ret = create(buf, OREAD, DMDIR | 0777);
		if(ret < 0){
			print("Mkdir failed\n");
			exits("mkdir");
		}
		close(ret);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			ret = create(buf, OREAD, DMDIR | 0777);
			if(ret < 0){
				print("Mkdir failed\n");
				exits("mkdir");
			}
			close(ret);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
				value=(char) ((i^j^k) & 0xff);
				memset(mbuffer,value,sz);
				stats[statCreate].starttime=time_so_far();
				fd=create(buf, ORDWR, 0600);
				if(fd < 0){
					print("Create failed\n");
					exits("createfile");
				}
				stats[statCreate].endtime=time_so_far();
				stats[statCreate].speed=stats[statCreate].endtime-stats[statCreate].starttime;
				if(stats[statCreate].speed < (double)0.0)
					stats[statCreate].speed=(double)0.0;
				stats[statCreate].total_time+=stats[statCreate].speed;
				stats[statCreate].counter++;
				if(stats[statCreate].speed < stats[statCreate].best)
					stats[statCreate].best=stats[statCreate].speed;
				if(stats[statCreate].speed > stats[statCreate].worst)
					stats[statCreate].worst=stats[statCreate].speed;
				stats[statWrite].starttime=time_so_far();
				junk=write(fd,mbuffer,sz);
				stats[statWrite].endtime=time_so_far();
				stats[statWrite].counter++;
				stats[statWrite].speed=stats[statWrite].endtime-stats[statWrite].starttime;
				if(stats[statWrite].speed < (double)0.0)
					stats[statWrite].speed=(double)0.0;
				stats[statWrite].total_time+=stats[statWrite].speed;
				if(stats[statWrite].speed < stats[statWrite].best)
					stats[statWrite].best=stats[statWrite].speed;
				if(stats[statWrite].speed > stats[statWrite].worst)
					stats[statWrite].worst=stats[statWrite].speed;

				stats[statClose].starttime=time_so_far();
				close(fd);
				stats[statClose].endtime=time_so_far();
				stats[statClose].speed=stats[statClose].endtime-stats[statClose].starttime;
				if(stats[statClose].speed < (double)0.0)
					stats[statClose].speed=(double)0.0;
				stats[statClose].total_time+=stats[statClose].speed;
				stats[statClose].counter++;
				if(stats[statClose].speed < stats[statClose].best)
					stats[statClose].best=stats[statClose].speed;
				if(stats[statClose].speed > stats[statClose].worst)
					stats[statClose].worst=stats[statClose].speed;
			}
			junk=chdir("..");
		}
		junk=chdir("..");
	}
}

void 
file_stat(int x)
{
	int i,j,k;
	char buf[100];
	Dir *dir;
	stats[statStat].best=(double)99999.9;
	stats[statStat].worst=(double)0.00000000;
	for(i=0;i<x;i++){
	  sprint(buf,"fileop_L1_%d",i);
	  junk=chdir(buf);
	  for(j=0;j<x;j++){
	    sprint(buf,"fileop_L1_%d_L2_%d",i,j);
	    junk=chdir(buf);
	    for(k=0;k<x;k++){
			sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
			stats[statStat].starttime=time_so_far();
			dir = dirstat(buf);
			if(dir == nil){
				fprint(2, "file_stat: can't stat %s: %r\n", buf);
				exits("stat");
			}
			stats[statStat].endtime=time_so_far();
			stats[statStat].speed=stats[statStat].endtime-stats[statStat].starttime;
			if(stats[statStat].speed < (double)0.0)
				stats[statStat].speed=(double)0.0;
			stats[statStat].total_time+=stats[statStat].speed;
			stats[statStat].counter++;
			if(stats[statStat].speed < stats[statStat].best)
				stats[statStat].best=stats[statStat].speed;
			if(stats[statStat].speed > stats[statStat].worst)
				stats[statStat].worst=stats[statStat].speed;
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
	stats[statAccess].best=(double)999999.9;
	stats[statAccess].worst=(double)0.0;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
				stats[statAccess].starttime=time_so_far();
				y=access(buf,AWRITE);
				if(y < 0){
					print("access failed\n");
					perror("what");
					exits("access");
				}
				stats[statAccess].endtime=time_so_far();
				stats[statAccess].speed=stats[statAccess].endtime-stats[statAccess].starttime;
				if(stats[statAccess].speed < (double)0.0)
					stats[statAccess].speed=(double)0.0;
				stats[statAccess].total_time+=stats[statAccess].speed;
				stats[statAccess].counter++;
				if(stats[statAccess].speed < stats[statAccess].best)
					stats[statAccess].best=stats[statAccess].speed;
				if(stats[statAccess].speed > stats[statAccess].worst)
					stats[statAccess].worst=stats[statAccess].speed;
			}
			junk=chdir("..");
	  }
	  junk=chdir("..");
	}
}

void 
file_chmod(int x)
{
	int i,j,k;
	char buf[100];
	Dir *dir, ndir;
	stats[statChmod].best=(double)999999.9;
	stats[statChmod].worst=(double)0.0;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
				stats[statChmod].starttime=time_so_far();
			
				dir = dirstat(buf);
				if(dir == nil){
					fprint(2, "chmod: can't stat %s: %r\n", buf);
					exits("chmod");
				}
				nulldir(&ndir);
				ndir.mode = (dir->mode & 0666);
				free(dir);
			
				if(dirwstat(buf, &ndir)==-1){
					fprint(2, "chmod: can't wstat %s: %r\n", buf);
					exits("chmod");
				}

				stats[statChmod].endtime=time_so_far();
				stats[statChmod].speed=stats[statChmod].endtime-stats[statChmod].starttime;
				if(stats[statChmod].speed < (double)0.0)
					stats[statChmod].speed=(double)0.0;
				stats[statChmod].total_time+=stats[statChmod].speed;
				stats[statChmod].counter++;
				if(stats[statChmod].speed < stats[statChmod].best)
					stats[statChmod].best=stats[statChmod].speed;
				if(stats[statChmod].speed > stats[statChmod].worst)
					stats[statChmod].worst=stats[statChmod].speed;
			}
			junk=chdir("..");
		}
		junk=chdir("..");
	}
}

void 
file_readdir(int x){
	int i,j,ret1, fd;
	char buf[100];
	Dir *dirbuf;
	long y;
	stats[statReaddir].best=(double)999999.9;
	stats[statReaddir].worst=(double)0.0;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			fd = open(".", OREAD);
			if(fd<0){
				print("opendir failed\n");
				exits("opendir");
			}
			stats[statReaddir].starttime=time_so_far();
			if((y = dirreadall(fd, &dirbuf))<0){
				print("readdir failed\n");
				exits("readdir");
			}
			stats[statReaddir].endtime=time_so_far();
			stats[statReaddir].speed=stats[statReaddir].endtime-stats[statReaddir].starttime;
			if(stats[statReaddir].speed < (double)0.0)
				stats[statReaddir].speed=(double)0.0;
			stats[statReaddir].total_time+=stats[statReaddir].speed;
			stats[statReaddir].counter++;
			if(stats[statReaddir].speed < stats[statReaddir].best)
				stats[statReaddir].best=stats[statReaddir].speed;
			if(stats[statReaddir].speed > stats[statReaddir].worst)
				stats[statReaddir].worst=stats[statReaddir].speed;
			ret1=close(fd);
			if(ret1 < 0){
				print("closedir failed\n");
				exits("closedir");
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
	stats[statDirDelete].best=(double)99999.9;
	stats[statDirDelete].worst=(double)0.00000000;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_dir_%d_%d_%d",i,j,k);
				junk=chdir(buf);
				junk=chdir("..");
				stats[statDirDelete].starttime=time_so_far();
				rmdir(buf);
				stats[statDirDelete].endtime=time_so_far();
				stats[statDirDelete].speed=stats[statDirDelete].endtime-stats[statDirDelete].starttime;
				if(stats[statDirDelete].speed < (double)0.0)
					stats[statDirDelete].speed=(double)0.0;
				stats[statDirDelete].total_time+=stats[statDirDelete].speed;
				stats[statDirDelete].counter++;
				if(stats[statDirDelete].speed < stats[statDirDelete].best)
					stats[statDirDelete].best=stats[statDirDelete].speed;
				if(stats[statDirDelete].speed > stats[statDirDelete].worst)
					stats[statDirDelete].worst=stats[statDirDelete].speed;
			}
			junk=chdir("..");
			if(junk < 0){
				print("%r\n");
			}
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			stats[statDirDelete].starttime=time_so_far();
			rmdir(buf);
			stats[statDirDelete].endtime=time_so_far();
			stats[statDirDelete].speed=stats[statDirDelete].endtime-stats[statDirDelete].starttime;
			if(stats[statDirDelete].speed < (double)0.0)
				stats[statDirDelete].speed=(double)0.0;
			stats[statDirDelete].total_time+=stats[statDirDelete].speed;
			stats[statDirDelete].counter++;
			if(stats[statDirDelete].speed < stats[statDirDelete].best)
				stats[statDirDelete].best=stats[statDirDelete].speed;
			if(stats[statDirDelete].speed > stats[statDirDelete].worst)
				stats[statDirDelete].worst=stats[statDirDelete].speed;
		}
		junk=chdir("..");
		sprint(buf,"fileop_L1_%d",i);
		stats[statDirDelete].starttime=time_so_far();
		rmdir(buf);
		stats[statDirDelete].endtime=time_so_far();
		stats[statDirDelete].speed=stats[statDirDelete].endtime-stats[statDirDelete].starttime;
		if(stats[statDirDelete].speed < (double)0.0)
			stats[statDirDelete].speed=(double)0.0;
		stats[statDirDelete].total_time+=stats[statDirDelete].speed;
		stats[statDirDelete].counter++;
		if(stats[statDirDelete].speed < stats[statDirDelete].best)
			stats[statDirDelete].best=stats[statDirDelete].speed;
		if(stats[statDirDelete].speed > stats[statDirDelete].worst)
			stats[statDirDelete].worst=stats[statDirDelete].speed;
	}
}

void
file_delete(int x)
{
	int i,j,k;
	char buf[100];
	stats[statDelete].best=(double)999999.9;
	stats[statDelete].worst=(double)0.0;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
				stats[statDelete].starttime=time_so_far();
				remove(buf);
				stats[statDelete].endtime=time_so_far();
				stats[statDelete].speed=stats[statDelete].endtime-stats[statDelete].starttime;
				if(stats[statDelete].speed < (double)0.0)
					stats[statDelete].speed=(double)0.0;
				stats[statDelete].total_time+=stats[statDelete].speed;
				stats[statDelete].counter++;
				if(stats[statDelete].speed < stats[statDelete].best)
					stats[statDelete].best=stats[statDelete].speed;
				if(stats[statDelete].speed > stats[statDelete].worst)
					stats[statDelete].worst=stats[statDelete].speed;
			}
			junk=chdir("..");
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			rmdir(buf);
	  }
	  junk=chdir("..");
	  sprint(buf,"fileop_L1_%d",i);
	  rmdir(buf);
	}
}
void 
file_read(int x)
{
	int i,j,k,y,fd;
	char buf[100];
	char value;
	stats[statRead].best=(double)99999.9;
	stats[statRead].worst=(double)0.00000000;
	stats[statOpen].best=(double)99999.9;
	stats[statOpen].worst=(double)0.00000000;
	for(i=0;i<x;i++){
		sprint(buf,"fileop_L1_%d",i);
		junk=chdir(buf);
		for(j=0;j<x;j++){
			sprint(buf,"fileop_L1_%d_L2_%d",i,j);
			junk=chdir(buf);
			for(k=0;k<x;k++){
				sprint(buf,"fileop_file_%d_%d_%d",i,j,k);
				value=(char)((i^j^k) &0xff);
				stats[statOpen].starttime=time_so_far();
				fd=open(buf,OREAD);
				if(fd < 0){
					print("Open failed\n");
				exits("openfile");
				}
				stats[statOpen].endtime=time_so_far();
				stats[statOpen].speed=stats[statOpen].endtime-stats[statOpen].starttime;
				if(stats[statOpen].speed < (double)0.0)
					stats[statOpen].speed=(double)0.0;
				stats[statOpen].total_time+=stats[statOpen].speed;
				stats[statOpen].counter++;
				if(stats[statOpen].speed < stats[statOpen].best)
					stats[statOpen].best=stats[statOpen].speed;
				if(stats[statOpen].speed > stats[statOpen].worst)
					stats[statOpen].worst=stats[statOpen].speed;

				stats[statRead].starttime=time_so_far();
				y=read(fd,mbuffer,sz);
				if(y < 0){
					print("Read failed\n");
					exits("read");
				}
				if(validate(mbuffer,sz, value) !=0)
					print("Error: Data Mis-compare\n");;
				stats[statRead].endtime=time_so_far();
				close(fd);
				stats[statRead].speed=stats[statRead].endtime-stats[statRead].starttime;
				if(stats[statRead].speed < (double)0.0)
					stats[statRead].speed=(double)0.0;
				stats[statRead].total_time+=stats[statRead].speed;
				stats[statRead].counter++;
				if(stats[statRead].speed < stats[statRead].best)
					stats[statRead].best=stats[statRead].speed;
				if(stats[statRead].speed > stats[statRead].worst)
				stats[statRead].worst=stats[statRead].speed;
			}
			junk=chdir("..");
		}
		junk=chdir("..");
	}
}

/************************************************************************/
/* Time measurement routines. Thanks to Iozone :-)			*/
/************************************************************************/

static double
time_so_far(){
	
	static int fd = -1;
	int tv_sec, tv_usec;
	char b[8];
	int64_t t;

	if(fd <0 )
		if ((fd = open("/dev/bintime", OREAD|OCEXEC)) < 0)
			return 0;
			
	if(pread(fd, b, sizeof b, 0) != sizeof b){
		close(fd);
		fd=-1;
		return 0;
	}
	
	be2vlong(&t, b);
	
	tv_sec = t/1000000000;
	tv_usec = (t/1000)%1000000;
		
	return ((double) (tv_sec)) + (((double)tv_usec) * 0.000001 );
}

void
be2vlong(int64_t *to, char *f)
{
	static uint64_t order = 0x0001020304050607ULL;
	char *t, *o;
	int i;

	t = (char*)to;
	o = (char*)&order;
	for(i = 0; i < 8; i++)
	t[o[i]] = f[i];
}

void
splash(void)
{
	print("\n");
	print("     --------------------------------------\n");
	print("     |              Fileop                | \n");
	print("     | %s          | \n",version);
	print("     |                                    | \n");
	print("     |                by                  |\n");
	print("     |                                    | \n");
	print("     |             Don Capps              |\n");
	print("     --------------------------------------\n");
	print("\n");
}

void 
usage(void)
{
  splash();
  print("     fileop [-f X ]|[-l # -u #] [-s Y] [-e] [-b] [-w] [-d <dir>] [-t] [-v] [-h]\n");
  print("\n");
  print("     -f #      Force factor. X^3 files will be created and removed.\n");
  print("     -l #      Lower limit on the value of the Force factor.\n");
  print("     -u #      Upper limit on the value of the Force factor.\n");
  print("     -s #      Optional. Sets filesize for the create/write. May use suffix 'K' or 'M'.\n");
  print("     -e        Excel importable format.\n");
  print("     -b        Output best case results.\n");
  print("     -i #      Increment force factor by this increment.\n");
  print("     -w        Output worst case results.\n");
  print("     -d <dir>  Specify starting directory.\n");
  print("     -U <dir>  Mount point to remount between tests.\n");
  print("     -t        Verbose output option.\n");
  print("     -v        Version information.\n");
  print("     -h        Help text.\n");
  print("\n");
  print("     The structure of the file tree is:\n");
  print("     X number of Level 1 directories, with X number of\n");
  print("     level 2 directories, with X number of files in each\n");
  print("     of the level 2 directories.\n");
  print("\n");
  print("     Example:  fileop -f 2\n");
  print("\n");
  print("             dir_1                        dir_2\n");
  print("            /     \\                      /     \\ \n");
  print("      sdir_1       sdir_2          sdir_1       sdir_2\n");
  print("      /     \\     /     \\          /     \\      /     \\ \n");
  print("   file_1 file_2 file_1 file_2   file_1 file_2 file_1 file_2\n");
  print("\n");
  print("   Each file will be created, and then Y bytes is written to the file.\n");
  print("\n");
}
void
clear_stats()
{
	int i;
	for(i=0;i<numStats;i++)
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

char*
getcwd(char *buf, size_t len)
{
        int fd;

        fd = open(".", OREAD);
        if(fd < 0) {
                return nil;
        }
        if(fd2path(fd, buf, len) < 0) {
                close(fd);
                return nil;
        }
        close(fd);

        return buf;
}

void
rmdir(char *f){
        char *name;
        int fd, i, j, n, ndir, nname;
        Dir *dirbuf;

        fd = open(f, OREAD);
        if(fd < 0){
			print("Error openning '%s'\n", f);
			exits("open");
        }
        n = dirreadall(fd, &dirbuf);
        close(fd);
        if(n < 0){
			print("Error dirreadall\n");
			exits("dirreadall");
        }

        nname = strlen(f)+1+STATMAX+1;  /* plenty! */
        name = malloc(nname);
        if(name == 0){
			print("Error memory allocation\n");
			exits("memory allocation");
        }

        ndir = 0;
        for(i=0; i<n; i++){
                snprint(name, nname, "%s/%s", f, dirbuf[i].name);
                if(remove(name) != -1)
                        dirbuf[i].qid.type = QTFILE;    /* so we won't recurse */
                else{
                        if(dirbuf[i].qid.type & QTDIR)
                                ndir++;
                        else {
							print("Error removing '%s'\n", name);
							exits("remove");
						}
                }
        }
        if(ndir)
                for(j=0; j<n; j++)
                        if(dirbuf[j].qid.type & QTDIR){
                                snprint(name, nname, "%s/%s", f, dirbuf[j].name);
                                rmdir(name);
                        }
        if(remove(f) == -1) {
			print("Error removing '%s'\n", name);
			exits("remove");
		}
        free(name);
        free(dirbuf);
}

void
bzero(void *a, size_t n)
{
        memset(a, 0, n);
}




