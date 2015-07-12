/* 
 * Here is a very simple set of routines to write an Excel worksheet 
 * Microsoft BIFF format. The Excel version is set to 2.0 so that it 
 * will work with all versions of Excel.
 *
 * Author: Don Capps 
 */

/* 
 * Note: rows and colums should not exceed 255 or this code will 
 * act poorly
 */

#ifdef Windows
#include <Windows.h>
#endif
#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#if defined(__AIX__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#if defined(OSV5) || defined(linux) || defined (__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__APPLE__) || defined(__DragonFly__)
#include <string.h>
#endif

#if defined(linux) || defined(__DragonFly__) || defined(IOZ_macosx)
#include <unistd.h>
#include <stdlib.h>
#endif

#if (defined(solaris) && defined( __LP64__ )) || defined(__s390x__) || defined(FreeBSD)
/* If we are building for 64-bit Solaris, all functions that return pointers
 * must be declared before they are used; otherwise the compiler will assume
 * that they return ints and the top 32 bits of the pointer will be lost,
 * causing segmentation faults.  The following includes take care of this.
 * It should be safe to add these for all other OSs too, but we're only
 * doing it for Solaris now in case another OS turns out to be a special case.
 */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#endif
/* Little Endian */
#define ENDIAN_1  1
/* Big Endian    */
#define ENDIAN_2  2
/* Middle Endian */
#define ENDIAN_3  3
/* Middle Endian */
#define ENDIAN_4  4

int junk, *junkp;


#ifdef HAVE_ANSIC_C
/************************************************************************/
/* Here is the API... Enjoy 						*/
/************************************************************************/
/* Create worksheet 							*/
int create_xls(char *);			
/*    Args: Filename							*/
/*    									*/
/* Close worksheet 							*/
void close_xls(int);			
/*   Args: file descriptor						*/
/*    									*/
/* Put a 16 bit integer in worksheet 					*/
void do_int(int,int,int,int);		
/*    Args: file descriptor,						*/
/*    	  value,							*/
/*	  row,								*/
/*	  column							*/

/* Put a double in 8 byte float 					*/
void do_float(int,double,int,int); 	
/*    Args: file descriptor,						*/
/*    	  value,							*/
/*	  row,								*/
/*	  column							*/
/* Put a string in worksheet 						*/
void do_label(int,char *,int,int);	
/*    Args: file descriptor,						*/
/*   	  string,							*/
/*	  row,								*/
/*	  column							*/
/************************************************************************/

char libbif_version[] = "Libbif Version $Revision: 3.26 $";
void do_eof(int );		/* Used internally */
void do_header(int );		/* Used internally */
int endian(void);
#endif

#define BOF 0x9
#define INTEGER 0x2
#define FLOAT 0x3
#define LABEL 0x4
#define EXCEL_VERS 0x2
#define WORKSHEET 0x10

struct bof_record{ 		/* Beginning of file */
	char hi_opcode;
	char lo_opcode;
	char hi_length;
	char lo_length;
	char hi_version;	/* Excel version */
	char lo_version;
	char hi_filetype;
	char lo_filetype;
	};
struct int_record {
	char hi_opcode;		/* Type 2 of record */
	char lo_opcode;
	char hi_length;
	char lo_length;
	char hi_row;
	char lo_row;
	char hi_column;
	char lo_column;
	char rgbhi;
	char rgbmed;
	char rgblo;
	char hi_data;
	char lo_data;
	};
struct label_record {
	char hi_opcode;		/* Type 4 of record */
	char lo_opcode;
	char hi_length;
	char lo_length;
	char hi_row;
	char lo_row;
	char hi_column;
	char lo_column;
	char rgbhi;
	char rgbmed;
	char rgblo;
	char string_length;
	char str_array[256];
	};
struct float_record {		/* Type 3 record */
	char hi_opcode;
	char lo_opcode;
	char hi_length;
	char lo_length;
	char hi_row;
	char lo_row;
	char hi_column;
	char lo_column;
	char rgbhi;
	char rgbmed;
	char rgblo;
	double data;
	};
/*
 * Write the EOF and close the file 
 */
#ifdef HAVE_ANSIC_C
void
close_xls(int fd)
{
#else
close_xls(fd)
int fd;
{
#endif
	do_eof(fd);
	close(fd);
}

/*
 * Create xls worksheet. Create file and put the BOF record in it.
 */
#ifdef HAVE_ANSIC_C
int
create_xls(char *name)
{
#else
create_xls(name)
char *name;
{
#endif
	int fd;
	unlink(name);
#ifdef Windows
	fd=open(name,O_BINARY|O_CREAT|O_RDWR,0666);
#else
	fd=open(name,O_CREAT|O_RDWR,0666);
#endif
	if(fd<0)
	{
		printf("Error opening file %s\n",name);
		exit(-1);
	}
	do_header(fd);
	return(fd);
}
	
#ifdef HAVE_ANSIC_C
void
do_header(int fd) /* Stick the BOF at the beginning of the file */
{
#else
do_header(fd) 
int fd;
{
#endif
	struct bof_record bof;
	bof.hi_opcode=BOF;
	bof.lo_opcode = 0x0;
	bof.hi_length=0x4;
	bof.lo_length=0x0;
	bof.hi_version=EXCEL_VERS;
	bof.lo_version=0x0;
	bof.hi_filetype=WORKSHEET;
	bof.lo_filetype=0x0;
	junk=write(fd,&bof,sizeof(struct bof_record));
}

/*
 * Put an integer (16 bit) in the worksheet 
 */
#ifdef HAVE_ANSIC_C
void
do_int(int fd,int val, int row, int column)
{
#else
do_int(fd,val,row,column)
int fd,val,row,column;
{
#endif
	struct int_record intrec;
	short s_row,s_column;
	s_row=(short)row;
	s_column=(short)column;
        intrec.hi_opcode=INTEGER;
        intrec.lo_opcode=0x00;
        intrec.hi_length=0x09;
        intrec.lo_length=0x00;
        intrec.rgbhi=0x0;
        intrec.rgbmed=0x0;
        intrec.rgblo=0x0;
        intrec.hi_row=(char)s_row&0xff;
        intrec.lo_row=(char)(s_row>>8)&0xff;
        intrec.hi_column=(char)(s_column&0xff);
        intrec.lo_column=(char)(s_column>>8)&0xff;
        intrec.hi_data=(val & 0xff);
        intrec.lo_data=(val & 0xff00)>>8;
	junk=write(fd,&intrec,13);
}

/* Note: This routine converts Big Endian to Little Endian 
 * and writes the record out.
 */

/* 
 * Put a double in the worksheet as 8 byte float in IEEE format.
 */
#ifdef HAVE_ANSIC_C
void
do_float(int fd, double value, int row, int column)
{
#else
do_float(fd, value, row, column)
int fd;
double value;
int row,column;
{
#endif
	struct float_record floatrec;
	short s_row,s_column;
	unsigned char *sptr,*dptr;
	s_row=(short)row;
	s_column=(short)column;
        floatrec.hi_opcode=FLOAT;
        floatrec.lo_opcode=0x00;
        floatrec.hi_length=0xf;
        floatrec.lo_length=0x00;
        floatrec.rgbhi=0x0;
        floatrec.rgbmed=0x0;
        floatrec.rgblo=0x0;
        floatrec.hi_row=(char)(s_row&0xff);
        floatrec.lo_row=(char)((s_row>>8)&0xff);
        floatrec.hi_column=(char)(s_column&0xff);
        floatrec.lo_column=(char)((s_column>>8)&0xff);
	sptr =(unsigned char *) &value;
	dptr =(unsigned char *) &floatrec.data;

	if(endian()==ENDIAN_2) /* Big Endian */
	{
	   dptr[0]=sptr[7]; /* Convert to Little Endian */
	   dptr[1]=sptr[6];
	   dptr[2]=sptr[5];
	   dptr[3]=sptr[4];
	   dptr[4]=sptr[3];
	   dptr[5]=sptr[2];
	   dptr[6]=sptr[1];
	   dptr[7]=sptr[0];
	}
	if(endian()==ENDIAN_3)  /* Middle Endian */
	{
	   dptr[0]=sptr[4]; /* 16 bit swapped ARM */
	   dptr[1]=sptr[5];
	   dptr[2]=sptr[6];
	   dptr[3]=sptr[7];
	   dptr[4]=sptr[0];
	   dptr[5]=sptr[1];
	   dptr[6]=sptr[2];
	   dptr[7]=sptr[3];
	}

	if(endian()==ENDIAN_1) /* Little Endian */
	{
	   dptr[0]=sptr[0]; /* Do not convert to Little Endian */
	   dptr[1]=sptr[1];
	   dptr[2]=sptr[2];
	   dptr[3]=sptr[3];
	   dptr[4]=sptr[4];
	   dptr[5]=sptr[5];
	   dptr[6]=sptr[6];
	   dptr[7]=sptr[7];
	}
	if(endian()==-1) /* Unsupported architecture */
	{
	   dptr[0]=0;
	   dptr[1]=0;
	   dptr[2]=0;
	   dptr[3]=0;
	   dptr[4]=0;
	   dptr[5]=0;
	   dptr[6]=0;
	   dptr[7]=0;
	   printf("Excel output not supported on this architecture.\n");
	}
	junk=write(fd,&floatrec,11); /* Don't write floatrec. Padding problems */
	junk=write(fd,&floatrec.data,8); /* Write value seperately */
}

/*
 * Put a string as a label in the worksheet.
 */
#ifdef HAVE_ANSIC_C
void
do_label(int fd, char *string, int row, int column)
{
#else
do_label(fd, string, row, column)
int fd;
char *string;
int row,column;
{
#endif
	struct label_record labelrec;
	short s_row,s_column;
	int i;
	for(i=0;i<255;i++)
		labelrec.str_array[i]=0;
	s_row=(short)row;
	s_column=(short)column;
	i=strlen(string);
        labelrec.hi_opcode=LABEL;
        labelrec.lo_opcode=0x00;
        labelrec.hi_length=0x08; /* 264 total bytes */
        labelrec.lo_length=0x01;
        labelrec.rgblo=0x0;
        labelrec.rgbmed=0x0;
        labelrec.rgbhi=0x0;
        labelrec.hi_row=(char)(s_row&0xff);
        labelrec.lo_row=(char)((s_row>>8)&0xff);
        labelrec.hi_column=(char)(s_column&0xff);
        labelrec.lo_column=(char)((s_column>>8)&0xff);
	labelrec.string_length=i;
	if(i > 255) /* If too long then terminate it early */
		string[254]=0;
	i=strlen(string);
	strcpy(labelrec.str_array,string);

	junk=write(fd,&labelrec,sizeof(struct label_record));

}

/* 
 * Write the EOF in the file 
 */
#ifdef HAVE_ANSIC_C
void
do_eof(int fd) 
{
#else
do_eof(fd) 
int fd;
{
#endif
	char buf[]={0x0a,0x00,0x00,0x00};
	junk=write(fd,buf,4);
}
	
/* 
 * Routine to determine the Endian-ness of the system. This
 * is needed for Iozone to convert doubles (floats) into
 * Little-endian format. This is needed for Excel to be 
 * able to interpret the file 
 */
int
endian(void)
{
	long long foo = 0x0102030405060708LL;
	long foo1 = 0x012345678;
	unsigned char *c,c1,c2,c3,c4,c5,c6,c7,c8;
	c=(unsigned char *)&foo;
	c1=*c++;
	c2=*c++;
	c3=*c++;
	c4=*c++;
	c5=*c++;
	c6=*c++;
	c7=*c++;
	c8=*c;

	/*--------------------------------------------------------------*/
	/* printf("%x %x %x %x %x %x %x %x\n",c1,c2,c3,c4,c5,c6,c7,c8); */
	/*--------------------------------------------------------------*/

	/* Little Endian format ? ( Intel ) */
	if( (c1==0x08) && (c2==0x07) && (c3==0x06) && (c4==0x05) &&
		(c5==0x04) && (c6==0x03) && (c7==0x02) && (c8==0x01) )
		return(ENDIAN_1);
	/* Big Endian format ?    ( Sparc, Risc... */
	if( (c1==0x01) && (c2==0x02) && (c3==0x03) && (c4==0x04) &&
		(c5==0x05) && (c6==0x06) && (c7==0x07) && (c8==0x08) )
		return(ENDIAN_2);
	/* Middle Endian format ? ( ARM ... ) */
	if( (c1==0x04) && (c2==0x03) && (c3==0x02) && (c4==0x01) &&
		(c5==0x08) && (c6==0x07) && (c7==0x06) && (c8==0x05) )
		return(ENDIAN_3);
	c=(unsigned char *)&foo1;
	c1=*c++;
	c2=*c++;
	c3=*c++;
	c4=*c++;
	/* Another middle endian format ? ( PDP-11 ... ) */
	if( (c1==0x34) && (c2==0x12) && (c3==0x78) && (c4==0x56))
		return(ENDIAN_4);

	return(-1);
}
