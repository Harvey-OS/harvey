/* 
 * Mpeg Audio Player (see version.h for version number)
 * ------------------------
 * copyright (c) 1995,1996,1997,1998,1999 by Michael Hipp, All rights reserved.
 * See also 'README' !
 *
 */

#include <stdlib.h>
#include <sys/types.h>
#if !defined(WIN32) && !defined(GENERIC)
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#if 0
#define SET_RT 
#endif


#ifdef SET_RT
#include <sched.h>
#endif

#include "mpg123.h"
#include "getlopt.h"
#include "buffer.h"
#include "term.h"

#include "version.h"

static void usage(char *dummy);
static void long_usage(char *);
static void print_title(void);

struct parameter param = { 
  FALSE , /* aggressiv */
  FALSE , /* shuffle */
  FALSE , /* remote */
  DECODE_AUDIO , /* write samples to audio device */
  FALSE , /* silent operation */
  0 ,     /* second level buffer size */
  TRUE ,  /* resync after stream error */
  0 ,     /* verbose level */
#ifdef TERM_CONTROL
  FALSE , /* term control */
#endif
  -1 ,     /* force mono */
  0 ,     /* force stereo */
  0 ,     /* force 8bit */
  0 ,     /* force rate */
  0 , 	  /* down sample */
  FALSE , /* checkrange */
  0 ,	  /* doublespeed */
  0 ,	  /* halfspeed */
  0 ,	  /* force_reopen, always (re)opens audio device for next song */
  FALSE,  /* try to run process in 'realtime mode' */   
  { 0,},  /* wav,cdr,au Filename */
};

char *listname = NULL;
char *listnamedir = NULL;
char *equalfile = NULL;
long outscale  = 32768;
long numframes = -1;
long startFrame= 0;
int frontend_type = 0;
int buffer_fd[2];
int buffer_pid;

char **shufflist= NULL;
int *shuffleord= NULL;
int shuffle_listsize = 0;

static int intflag = FALSE;

int OutputDescriptor;

#if !defined(WIN32) && !defined(GENERIC)
static void catch_child(void)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

static void catch_interrupt(void)
{
  intflag = TRUE;
}
#endif

static struct frame fr;
struct audio_info_struct ai;
txfermem *buffermem = NULL;
#define FRAMEBUFUNIT (18 * 64 * 4)

void set_synth_functions(struct frame *fr);

char *handle_remote(void)
{
	switch(frontend_type) {
		case FRONTEND_SAJBER:
#if defined(FRONTEND) && !defined(NOSAJBER)
			control_sajber(&fr);
#endif
			break;
		case FRONTEND_TK3PLAY:
#ifdef FRONTEND
			control_tk3play(&fr);
#endif
			break;
		default:
#if 0
			fgets(remote_buffer,1024,stdin);
			remote_buffer[strlen(remote_buffer)-1]=0;
  
			switch(remote_buffer[0]) {
				case 'P':
					return remote_buffer+1;        
			}

#if !defined(WIN32) && !defined(GENERIC)
			if(param.usebuffer) 
				buffer_resync();	
#endif
#else
			control_generic(&fr);
#endif
			break;
	}

	return NULL;    
}

void init_output(void)
{
  static int init_done = FALSE;

  if (init_done)
    return;
  init_done = TRUE;
#ifndef NOXFERMEM
  if (param.usebuffer) {
    unsigned int bufferbytes;
    sigset_t newsigset, oldsigset;
    if (param.usebuffer < 32)
      param.usebuffer = 32; /* minimum is 32 Kbytes! */
    bufferbytes = (param.usebuffer * 1024);
    bufferbytes -= bufferbytes % FRAMEBUFUNIT;
	/* +1024 for NtoM rounding problems */
    xfermem_init (&buffermem, bufferbytes ,0,1024);
    pcm_sample = (unsigned char *) buffermem->data;
    pcm_point = 0;
    sigemptyset (&newsigset);
    sigaddset (&newsigset, SIGUSR1);
    sigprocmask (SIG_BLOCK, &newsigset, &oldsigset);
    catchsignal (SIGCHLD, catch_child);
    switch ((buffer_pid = fork())) {
      case -1: /* error */
        perror("fork()");
#ifdef TERM_CONTROL
	if(param.term_ctrl)
		term_restore();
#endif
        exit(1);
      case 0: /* child */
        if(rd)
          rd->close(rd); /* child doesn't need the input stream */
        xfermem_init_reader (buffermem);
        buffer_loop (&ai, &oldsigset);
        xfermem_done_reader (buffermem);
        xfermem_done (buffermem);
        _exit(0);
      default: /* parent */
        xfermem_init_writer (buffermem);
        param.outmode = DECODE_BUFFER;
    }
  }
  else {
#endif
	/* + 1024 for NtoM rate converter */
    if (!(pcm_sample = (unsigned char *) malloc(audiobufsize * 2 + 1024))) {
      perror ("malloc()");
#ifdef TERM_CONTROL
      if(param.term_ctrl)
      	term_restore();
#endif
      exit (1);
#ifndef NOXFERMEM
    }
#endif
  }

  switch(param.outmode) {
    case DECODE_AUDIO:
      if(audio_open(&ai) < 0) {
        perror("audio");
#ifdef TERM_CONTROL
	if(param.term_ctrl)
		term_restore();
#endif
        exit(1);
      }
      break;
    case DECODE_WAV:
      wav_open(&ai,param.filename);
      break;
    case DECODE_AU:
      au_open(&ai,param.filename);
      break;
    case DECODE_CDR:
      cdr_open(&ai,param.filename);
      break;
  }
}

void shuffle_files(int numfiles)
{
    int loop, rannum;

    srand(time(NULL));
    if(shuffleord)
      free(shuffleord);
    shuffleord = (int *) malloc((numfiles + 1) * sizeof(int));
    if (!shuffleord) {
	perror("malloc");
	exit(1);
    }
    /* write songs in 'correct' order */
    for (loop = 0; loop < numfiles; loop++) {
	shuffleord[loop] = loop;
    }

    /* now shuffle them */
    if(numfiles >= 2) {
      for (loop = 0; loop < numfiles; loop++) {
	rannum = (rand() % (numfiles * 4 - 4)) / 4;
        rannum += (rannum >= loop);
	shuffleord[loop] ^= shuffleord[rannum];
	shuffleord[rannum] ^= shuffleord[loop];
	shuffleord[loop] ^= shuffleord[rannum];
      }
    }

#if 0
    /* print them */
    for (loop = 0; loop < numfiles; loop++) {
	fprintf(stderr, "%d ", shuffleord[loop]);
    }
#endif

}

char *find_next_file (int argc, char *argv[])
{
    static FILE *listfile = NULL;
    static char line[1024];
    char linetmp [1024];
    char * slashpos;
    int i;

    /* Get playlist dirname to append it to the files in playlist */
    if (listname) {
        if ((slashpos=strrchr(listname, '/'))) {
            listnamedir=strdup (listname);
            listnamedir[1 + slashpos - listname] = 0;
        }
    }

    if (listname || listfile) {
        if (!listfile) {
            if (!*listname || !strcmp(listname, "-")) {
                listfile = stdin;
                listname = NULL;
            }
            else if (!strncmp(listname, "http://", 7))  {
		int fd;
                fd = http_open(listname);
                listfile = fdopen(fd,"r");
            }
            else if (!(listfile = fopen(listname, "rb"))) {
                perror (listname);
#ifdef TERM_CONTROL
		if(param.term_ctrl)
			term_restore();
#endif
                exit (1);
            }
            if (param.verbose)
                fprintf (stderr, "Using playlist from %s ...\n",
                        listname ? listname : "standard input");
        }
        do {
            if (fgets(line, 1023, listfile)) {
                line[strcspn(line, "\t\n\r")] = '\0';
#if !defined(WIN32)
                /* MS-like directory format */
                for (i=0;line[i]!='\0';i++)
                    if (line [i] == '\\')
                        line [i] = '/';
#endif
                if (line[0]=='\0' || line[0]=='#')
                    continue;
		if ((listnamedir) && (line[0]!='/') && (line[0]!='\\')){
                    strcpy (linetmp, listnamedir);
                    strcat (linetmp, line);
		    strcpy (line, linetmp);
                }
                return (line);
            }
            else {
                if (listname)
                   fclose (listfile);
                listname = NULL;
                listfile = NULL;
            }
        } while (listfile);
    }
    if (loptind < argc)
    	return (argv[loptind++]);
    return (NULL);
}

void init_input (int argc, char *argv[])
{
    int mallocsize = 0;
    char *tempstr;

    shuffle_listsize = 0;

    if (!param.shuffle || param.remote) 
      return;

    while ((tempstr = find_next_file(argc, argv))) {
	if (shuffle_listsize + 2 > mallocsize) {
	    mallocsize += 8;
	    shufflist = (char **) realloc(shufflist, mallocsize * sizeof(char *));
	    if (!shufflist) {
		perror("realloc");
		exit(1);
	    }
	}
	if (!(shufflist[shuffle_listsize] = (char *) malloc(strlen(tempstr) + 1))) {
	    perror("malloc");
	    exit(1);
	}
	strcpy(shufflist[shuffle_listsize], tempstr);
	shuffle_listsize++;
    }
    if (shuffle_listsize) {
	if (shuffle_listsize + 1 < mallocsize) {
	    shufflist = (char **) realloc(shufflist, (shuffle_listsize + 1) * sizeof(char *));
	}
	shufflist[shuffle_listsize] = NULL;
    }
    shuffle_files(shuffle_listsize);
}

char *get_next_file(int argc, char **argv)
{
    static int curfile = 0;
    char *newfile;

    if (!param.shuffle) {
	return find_next_file(argc, argv);
    }
    if (!shufflist || !shufflist[curfile]) {
	return NULL;
    }
    if(param.shuffle == 1) {
        if (shuffleord) {
   	    newfile = shufflist[shuffleord[curfile]];
        } else {
  	    newfile = shufflist[curfile];
        }
        curfile++;
    }
    else {
       newfile = shufflist[ rand() % shuffle_listsize ];
    }

    return newfile;
}

static void set_output_h(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_HEADPHONES;
  else
    ai.output |= AUDIO_OUT_HEADPHONES;
}
static void set_output_s(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_INTERNAL_SPEAKER;
  else
    ai.output |= AUDIO_OUT_INTERNAL_SPEAKER;
}
static void set_output_l(char *a)
{
  if(ai.output <= 0)
    ai.output = AUDIO_OUT_LINE_OUT;
  else
    ai.output |= AUDIO_OUT_LINE_OUT;
}

static void set_output (char *arg)
{
    switch (*arg) {
        case 'h': set_output_h(arg); break;
        case 's': set_output_s(arg); break;
        case 'l': set_output_l(arg); break;
        default:
            fprintf (stderr, "%s: Unknown argument \"%s\" to option \"%s\".\n",
                prgName, arg, loptarg);
            exit (1);
    }
}

void set_verbose (char *arg)
{
    param.verbose++;
}
void set_wav(char *arg)
{
  param.outmode = DECODE_WAV;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
void set_cdr(char *arg)
{
  param.outmode = DECODE_CDR;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
void set_au(char *arg)
{
  param.outmode = DECODE_AU;
  strncpy(param.filename,arg,255);
  param.filename[255] = 0;
}
static void SetOutFile(char *Arg)
{
  param.outmode=DECODE_FILE;
  OutputDescriptor=open(Arg,O_WRONLY,0);
  if(OutputDescriptor==-1) {
    fprintf(stderr,"Can't open %s for writing (%s).\n",Arg,strerror(errno));
    exit(1);
  }
}
static void SetOutStdout(char *Arg)
{
  param.outmode=DECODE_FILE;
  OutputDescriptor=1;
}
static void SetOutStdout1(char *Arg)
{
  param.outmode=DECODE_AUDIOFILE;
  OutputDescriptor=1;
}

void not_compiled(char *arg)
{
  fprintf(stderr,"Option '-T / --realtime' not compiled into this binary\n");
}

/* Please note: GLO_NUM expects point to LONG! */
topt opts[] = {
    {'k', "skip",        GLO_ARG | GLO_LONG, 0, &startFrame, 0},
    {'a', "audiodevice", GLO_ARG | GLO_CHAR, 0, &ai.device,  0},
    {'2', "2to1",        0,                  0, &param.down_sample, 1},
    {'4', "4to1",        0,                  0, &param.down_sample, 2},
    {'t', "test",        0,                  0, &param.outmode, DECODE_TEST},
    {'s', "stdout",      0,       SetOutStdout, &param.outmode, DECODE_FILE},
    {'S', "STDOUT",      0,       SetOutStdout1, &param.outmode, DECODE_AUDIOFILE},
    {'O', "outfile",     GLO_ARG | GLO_CHAR, SetOutFile, NULL, NULL},
    {'c', "check",       0,                  0, &param.checkrange, TRUE},
    {'v', "verbose",     0,        set_verbose, 0,           0},
    {'q', "quiet",       0,                  0, &param.quiet,      TRUE},
    {'y', "resync",      0,                  0, &param.tryresync,  FALSE},
    {'0', "single0",     0,                  0, &param.force_mono, 0},
    {0,   "left",        0,                  0, &param.force_mono, 0},
    {'1', "single1",     0,                  0, &param.force_mono, 1},
    {0,   "right",       0,                  0, &param.force_mono, 1},
    {'m', "singlemix",   0,                  0, &param.force_mono, 3},
    {0,   "mix",         0,                  0, &param.force_mono, 3},
    {0,   "mono",        0,                  0, &param.force_mono, 3},
    {0,   "stereo",      0,                  0, &param.force_stereo, 1},
    {0,   "reopen",      0,                  0, &param.force_reopen, 1},
    {'g', "gain",        GLO_ARG | GLO_LONG, 0, &ai.gain,    0},
    {'r', "rate",        GLO_ARG | GLO_LONG, 0, &param.force_rate,  0},
    {0,   "8bit",        0,                  0, &param.force_8bit, 1},
    {0,   "headphones",  0,                  set_output_h, 0,0},
    {0,   "speaker",     0,                  set_output_s, 0,0},
    {0,   "lineout",     0,                  set_output_l, 0,0},
    {'o', "output",      GLO_ARG | GLO_CHAR, set_output, 0,  0},
    {'f', "scale",       GLO_ARG | GLO_LONG, 0, &outscale,   0},
    {'n', "frames",      GLO_ARG | GLO_LONG, 0, &numframes,  0},
#ifdef TERM_CONTROL
    {'C', "control",	 0,		     0, &param.term_ctrl, TRUE},
#endif
    {'b', "buffer",      GLO_ARG | GLO_LONG, 0, &param.usebuffer,  0},
    {'R', "remote",      0,                  0, &param.remote,     TRUE},
    {'d', "doublespeed", GLO_ARG | GLO_LONG, 0, &param.doublespeed,0},
    {'h', "halfspeed",   GLO_ARG | GLO_LONG, 0, &param.halfspeed,  0},
    {'p', "proxy",       GLO_ARG | GLO_CHAR, 0, &proxyurl,   0},
    {'@', "list",        GLO_ARG | GLO_CHAR, 0, &listname,   0},
	/* 'z' comes from the the german word 'zufall' (eng: random) */
    {'z', "shuffle",     0,                  0, &param.shuffle,    1},
    {'Z', "random",      0,                  0, &param.shuffle,    2},
    {'E', "equalizer",	 GLO_ARG | GLO_CHAR, 0, &equalfile,1},
    {0,   "aggressive",	 0,   	             0, &param.aggressive,2},
#if !defined(WIN32) && !defined(GENERIC)
    {'u', "auth",        GLO_ARG | GLO_CHAR, 0, &httpauth,   0},
#endif
#if defined(SET_RT)
    {'T', "realtime",    0,                  0, &param.realtime, TRUE },
#else
    {'T', "realtime",    0,       not_compiled, 0,           0 },    
#endif
    {'w', "wav",         GLO_ARG | GLO_CHAR, set_wav, 0 , 0 },
    {0, "cdr",         GLO_ARG | GLO_CHAR, set_cdr, 0 , 0 },
    {0, "au",         GLO_ARG | GLO_CHAR, set_au, 0 , 0 },
    {'?', "help",       0,              usage, 0,           0 },
    {0 , "longhelp" ,    0,        long_usage, 0,           0 },
    {0, 0, 0, 0, 0, 0}
};

/*
 *   Change the playback sample rate.
 */
static void reset_audio(void)
{
#ifndef NOXFERMEM
	if (param.usebuffer) {
		/* wait until the buffer is empty,
		 * then tell the buffer process to
		 * change the sample rate.   [OF]
		 */
		while (xfermem_get_usedspace(buffermem)	> 0)
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		buffermem->freeindex = -1;
		buffermem->readindex = 0; /* I know what I'm doing! ;-) */
		buffermem->freeindex = 0;
		if (intflag)
			return;
		buffermem->buf[0] = ai.rate; 
		buffermem->buf[1] = ai.channels; 
		buffermem->buf[2] = ai.format;
		buffer_reset();
	}
	else 
#endif
	if (param.outmode == DECODE_AUDIO) {
		/* audio_reset_parameters(&ai); */
		/*   close and re-open in order to flush
		 *   the device's internal buffer before
		 *   changing the sample rate.   [OF]
		 */
		audio_close (&ai);
		if (audio_open(&ai) < 0) {
			perror("audio");
#ifdef TERM_CONTROL
			if(param.term_ctrl)
				term_restore();
#endif
			exit(1);
		}
	}
}

/*
 * play a frame read by read_frame();
 * (re)initialize audio if necessary.
 *
 * needs a major rewrite .. it's incredible ugly!
 */
void play_frame(int init,struct frame *fr)
{
	int clip;
	long newrate;
	long old_rate,old_format,old_channels;

	if(fr->header_change || init) {

		if (!param.quiet && init) {
			if (param.verbose)
				print_header(fr);
			else
				print_header_compact(fr);
		}

		if(fr->header_change > 1 || init) {
			old_rate = ai.rate;
			old_format = ai.format;
			old_channels = ai.channels;

			newrate = freqs[fr->sampling_frequency]>>(param.down_sample);

			fr->down_sample = param.down_sample;
			audio_fit_capabilities(&ai,fr->stereo,newrate);

			/* check, whether the fitter setted our proposed rate */
			if(ai.rate != newrate) {
				if(ai.rate == (newrate>>1) )
					fr->down_sample++;
				else if(ai.rate == (newrate>>2) )
					fr->down_sample+=2;
				else {
					fr->down_sample = 3;
					fprintf(stderr,"Warning, flexibel rate not heavily tested!\n");
				}
				if(fr->down_sample > 3)
					fr->down_sample = 3;
			}

			switch(fr->down_sample) {
				case 0:
				case 1:
				case 2:
					fr->down_sample_sblimit = SBLIMIT>>(fr->down_sample);
					break;
				case 3:
					{
						long n = freqs[fr->sampling_frequency];
                                                long m = ai.rate;

						synth_ntom_set_step(n,m);

						if(n>m) {
							fr->down_sample_sblimit = SBLIMIT * m;
							fr->down_sample_sblimit /= n;
						}
						else {
							fr->down_sample_sblimit = SBLIMIT;
						}
					}
					break;
			}

			init_output();
			if(ai.rate != old_rate || ai.channels != old_channels ||
			   ai.format != old_format || param.force_reopen) {
				if(param.force_mono < 0) {
					if(ai.channels == 1)
						fr->single = 3;
					else
						fr->single = -1;
				}
				else
					fr->single = param.force_mono;

				param.force_stereo &= ~0x2;
				if(fr->single >= 0 && ai.channels == 2) {
					param.force_stereo |= 0x2;
				}

				set_synth_functions(fr);
				init_layer3(fr->down_sample_sblimit);
				reset_audio();
				if(param.verbose) {
					if(fr->down_sample == 3) {
						long n = freqs[fr->sampling_frequency];
						long m = ai.rate;
						if(n > m) {
							fprintf(stderr,"Audio: %2.4f:1 conversion,",(float)n/(float)m);
						}
						else {
							fprintf(stderr,"Audio: 1:%2.4f conversion,",(float)m/(float)n);
						}
					}
					else {
						fprintf(stderr,"Audio: %ld:1 conversion,",(long)pow(2.0,fr->down_sample));
					}
 					fprintf(stderr," rate: %ld, encoding: %s, channels: %d\n",ai.rate,audio_encoding_name(ai.format),ai.channels);
				}
			}
			if (intflag)
				return;
		}
	}

	if (fr->error_protection) {
		getbits(16); /* skip crc */
	}

	/* do the decoding */
	clip = (fr->do_layer)(fr,param.outmode,&ai);

#ifndef NOXFERMEM
	if (param.usebuffer) {
		if (!intflag) {
			buffermem->freeindex =
				(buffermem->freeindex + pcm_point) % buffermem->size;
			if (buffermem->wakeme[XF_READER])
				xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_WAKEUP_INFO);
		}
		pcm_sample = (unsigned char *) (buffermem->data + buffermem->freeindex);
		pcm_point = 0;
		while (xfermem_get_freespace(buffermem) < (FRAMEBUFUNIT << 1))
			if (xfermem_block(XF_WRITER, buffermem) == XF_CMD_TERMINATE) {
				intflag = TRUE;
				break;
			}
		if (intflag)
			return;
	}
#endif

	if(clip > 0 && param.checkrange)
		fprintf(stderr,"%d samples clipped\n", clip);
}

void set_synth_functions(struct frame *fr)
{
	typedef int (*func)(real *,int,unsigned char *,int *);
	typedef int (*func_mono)(real *,unsigned char *,int *);
	int ds = fr->down_sample;
	int p8=0;

	static func funcs[2][4] = { 
		{ synth_1to1,
		  synth_2to1,
		  synth_4to1,
		  synth_ntom } ,
		{ synth_1to1_8bit,
		  synth_2to1_8bit,
		  synth_4to1_8bit,
		  synth_ntom_8bit } 
	};

	static func_mono funcs_mono[2][2][4] = {    
		{ { synth_1to1_mono2stereo ,
		    synth_2to1_mono2stereo ,
		    synth_4to1_mono2stereo ,
		    synth_ntom_mono2stereo } ,
		  { synth_1to1_8bit_mono2stereo ,
		    synth_2to1_8bit_mono2stereo ,
		    synth_4to1_8bit_mono2stereo ,
		    synth_ntom_8bit_mono2stereo } } ,
		{ { synth_1to1_mono ,
		    synth_2to1_mono ,
		    synth_4to1_mono ,
		    synth_ntom_mono } ,
		  { synth_1to1_8bit_mono ,
		    synth_2to1_8bit_mono ,
		    synth_4to1_8bit_mono ,
		    synth_ntom_8bit_mono } }
	};

	if((ai.format & AUDIO_FORMAT_MASK) == AUDIO_FORMAT_8)
		p8 = 1;
	fr->synth = funcs[p8][ds];
	fr->synth_mono = funcs_mono[param.force_stereo?0:1][p8][ds];

	if(p8) {
		make_conv16to8_table(ai.format);
	}
}

int main(int argc, char *argv[])
{
	int result;
	unsigned long frameNum = 0;
	char *fname;
#if !defined(WIN32) && !defined(GENERIC)
	struct timeval start_time, now;
	unsigned long secdiff;
#endif	
	int init;

#ifdef OS2
        _wildcard(&argc,&argv);
#endif

	if(sizeof(short) != 2) {
		fprintf(stderr,"Ouch SHORT has size of %d bytes (required: '2')\n",(int)sizeof(short));
		exit(1);
	}
	if(sizeof(long) < 4) {
		fprintf(stderr,"Ouch LONG has size of %d bytes (required: at least 4)\n",(int)sizeof(long));
	}

	(prgName = strrchr(argv[0], '/')) ? prgName++ : (prgName = argv[0]);

#ifndef NOSAJBER
	if(!strcmp("sajberplay",prgName))
		frontend_type = FRONTEND_SAJBER;
#endif
	if(!strcmp("mpg123m",prgName))
		frontend_type = FRONTEND_TK3PLAY;

	audio_info_struct_init(&ai);


	while ((result = getlopt(argc, argv, opts)))
	switch (result) {
		case GLO_UNKNOWN:
			fprintf (stderr, "%s: Unknown option \"%s\".\n", 
				prgName, loptarg);
			exit (1);
		case GLO_NOARG:
			fprintf (stderr, "%s: Missing argument for option \"%s\".\n",
				prgName, loptarg);
			exit (1);
	}

	if (loptind >= argc && !listname && !frontend_type)
		usage(NULL);

#if !defined(WIN32) && !defined(GENERIC)
	if (param.remote) {
		param.verbose = 0;        
		param.quiet = 1;
	}
#endif

	if (!param.quiet)
		print_title();

	if(param.force_mono >= 0) {
		fr.single = param.force_mono;
	}

	if(param.force_rate && param.down_sample) {
		fprintf(stderr,"Down smapling and fixed rate options not allowed together!\n");
		exit(1);
	}

	audio_capabilities(&ai);

	if(equalfile) { /* tst */
		FILE *fe;
		int i;

		equalizer_cnt = 0;
		for(i=0;i<32;i++) {
			equalizer[0][i] = equalizer[1][i] = 1.0;
			equalizer_sum[0][i] = equalizer_sum[1][i] = 0.0;
		}

		fe = fopen(equalfile,"r");
		if(fe) {
			char line[256];
			for(i=0;i<32;i++) {
				float e1,e0; /* %f -> float! */
				line[0]=0;
				fgets(line,255,fe);
				if(line[0]=='#')
					continue;
				sscanf(line,"%f %f",&e0,&e1);
				equalizer[0][i] = e0;
				equalizer[1][i] = e1;	
			}
			fclose(fe);
		}
		else
			fprintf(stderr,"Can't open equalizer file '%s'\n",equalfile);
	}

#if !defined(WIN32) && !defined(GENERIC) && !defined(MINT) && !defined(__EMX__) && !defined(OS2) && !defined(PLAN9)
	if(param.aggressive) { /* tst */
		int mypid = getpid();
		setpriority(PRIO_PROCESS,mypid,-20);
	}
#endif

#ifdef SET_RT
	if (param.realtime) {  /* Get real-time priority */
	  struct sched_param sp;
	  fprintf(stderr,"Getting real-time priority\n");
	  memset(&sp, 0, sizeof(struct sched_param));
	  sp.sched_priority = sched_get_priority_min(SCHED_FIFO);
	  if (sched_setscheduler(0, SCHED_RR, &sp) == -1)
	    fprintf(stderr,"Can't get real-time priority\n");
	}
#endif

	set_synth_functions(&fr);

	init_input(argc, argv);

	make_decode_tables(outscale);
	init_layer2(); /* inits also shared tables with layer1 */
	init_layer3(fr.down_sample);

#if !defined(WIN32) && !defined(GENERIC)
	catchsignal (SIGINT, catch_interrupt);

	if(frontend_type || param.remote) {
		handle_remote();
		exit(0);
	}
#endif

	while ((fname = get_next_file(argc, argv))) {
		char *dirname, *filename;
		long leftFrames,newFrame;

		if(!*fname || !strcmp(fname, "-"))
			fname = NULL;
		open_stream(fname,-1);
      
		if (!param.quiet) {
			if (split_dir_file(fname ? fname : "standard input",
				&dirname, &filename))
				fprintf(stderr, "\nDirectory: %s", dirname);
			fprintf(stderr, "\nPlaying MPEG stream from %s ...\n", filename);

#if !defined(GENERIC)
{
     const char *term_type;
         term_type = getenv("TERM");
         if (term_type != NULL && !strcmp(term_type,"xterm"))
         {
           fprintf(stderr, "\033]0;%s\007", filename);
         }
}
#endif

		}

#if !defined(WIN32) && !defined(GENERIC)
#ifdef TERM_CONTROL
		if(!param.term_ctrl)
#endif
			gettimeofday (&start_time, NULL);
#endif
		read_frame_init();

		init = 1;
		newFrame = startFrame;
#ifdef TERM_CONTROL
		if(param.term_ctrl)
			term_init();
#endif
		leftFrames = numframes;
		for(frameNum=0;read_frame(&fr) && leftFrames && !intflag;frameNum++) {
#ifdef TERM_CONTROL			
tc_hack:
#endif
			if(frameNum < startFrame || (param.doublespeed && (frameNum % param.doublespeed))) {
				if(fr.lay == 3)
					set_pointer(512);
				continue;
			}
			if(leftFrames > 0)
			  leftFrames--;
			play_frame(init,&fr);
			init = 0;

			if(param.verbose) {
#ifndef NOXFERMEM
				if (param.verbose > 1 || !(frameNum & 0x7))
					print_stat(&fr,frameNum,xfermem_get_usedspace(buffermem),&ai); 
				if(param.verbose > 2 && param.usebuffer)
					fprintf(stderr,"[%08x %08x]",buffermem->readindex,buffermem->freeindex);
#else
				if (param.verbose > 1 || !(frameNum & 0x7))
					print_stat(&fr,frameNum,0,&ai);
#endif
			}
#ifdef TERM_CONTROL
			if(!param.term_ctrl) {
				continue;
			} else {
				long offset;
				if((offset=term_control(&fr))) {
					if(!rd->back_frame(rd, &fr, -offset)) {
						frameNum+=offset;
						if (frameNum < 0)
							frameNum = 0;
					}
				}
			}
#endif

      }

#ifndef NOXFERMEM
	if(param.usebuffer) {
		int s;
		while ((s = xfermem_get_usedspace(buffermem))) {
			struct timeval wait170 = {0, 170000};

			buffer_ignore_lowmem();
			
			if(param.verbose)
				print_stat(&fr,frameNum,s,&ai);
#ifdef TERM_CONTROL
			if(param.term_ctrl) {
				long offset;
				if((offset=term_control(&fr))) {
					if((!rd->back_frame(rd, &fr, -offset)) 
						&& read_frame(&fr)) {
						frameNum+=offset;
						if (frameNum < 0)
							frameNum = 0;
						goto tc_hack;	/* Doh! Gag me with a spoon! */
					}
				}
			}
#endif
			select(0, NULL, NULL, NULL, &wait170);
		}
	}
#endif
	if(param.verbose)
		print_stat(&fr,frameNum,xfermem_get_usedspace(buffermem),&ai); 
#ifdef TERM_CONTROL
	if(param.term_ctrl)
		term_restore();
#endif

	if (!param.quiet) {
		/* 
		 * This formula seems to work at least for
		 * MPEG 1.0/2.0 layer 3 streams.
		 */
		int secs = get_songlen(&fr,frameNum);
		fprintf(stderr,"\n[%d:%02d] Decoding of %s finished.\n", secs / 60,
			secs % 60, filename);
	}

	rd->close(rd);
#if 0
	if(param.remote)
		fprintf(stderr,"@R MPG123\n");        
	if (remflag) {
		intflag = FALSE;
		remflag = FALSE;
	}
#endif
	
      if (intflag) {

/* 
 * When using TERM_CONTROL, there is 'q' to terminate a list of songs, so
 * no pressing need to keep up this first second SIGINT hack that was too
 * often mistaken as a bug. [dk]
 */
#if !defined(WIN32) && !defined(GENERIC)
#ifdef TERM_CONTROL
	if(!param.term_ctrl)
#endif
        {
		gettimeofday (&now, NULL);
        	secdiff = (now.tv_sec - start_time.tv_sec) * 1000;
        	if (now.tv_usec >= start_time.tv_usec)
          		secdiff += (now.tv_usec - start_time.tv_usec) / 1000;
        	else
          		secdiff -= (start_time.tv_usec - now.tv_usec) / 1000;
        	if (secdiff < 1000)
          		break;
	}
#endif
        intflag = FALSE;
      }
    }
#ifndef NOXFERMEM
    if (param.usebuffer) {
      buffer_end();
      xfermem_done_writer (buffermem);
      waitpid (buffer_pid, NULL, 0);
      xfermem_done (buffermem);
    }
    else {
#endif
      audio_flush(param.outmode, &ai);
      free (pcm_sample);
#ifndef NOXFERMEM
    }
#endif

    switch(param.outmode) {
      case DECODE_AUDIO:
        audio_close(&ai);
        break;
      case DECODE_WAV:
        wav_close();
        break;
      case DECODE_AU:
        au_close();
        break;
      case DECODE_CDR:
        cdr_close();
        break;
    }
   
    return 0;
}

static void print_title(void)
{
    fprintf(stderr,"High Performance MPEG 1.0/2.0/2.5 Audio Player for Layer 1, 2 and 3.\n");
    fprintf(stderr,"Version %s (%s). Written and copyrights by Michael Hipp.\n", prgVersion, prgDate);
    fprintf(stderr,"Uses code from various people. See 'README' for more!\n");
    fprintf(stderr,"THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! USE AT YOUR OWN RISK!\n");
}

static void usage(char *dummy)  /* print syntax & exit */
{
   print_title();
   fprintf(stderr,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);
   fprintf(stderr,"supported options [defaults in brackets]:\n");
   fprintf(stderr,"   -v    increase verbosity level       -q    quiet (don't print title)\n");
   fprintf(stderr,"   -t    testmode (no output)           -s    write to stdout\n");
   fprintf(stderr,"   -w <filename> write Output as WAV file\n");
   fprintf(stderr,"   -k n  skip first n frames [0]        -n n  decode only n frames [all]\n");
   fprintf(stderr,"   -c    check range violations         -y    DISABLE resync on errors\n");
   fprintf(stderr,"   -b n  output buffer: n Kbytes [0]    -f n  change scalefactor [32768]\n");
   fprintf(stderr,"   -r n  set/force samplerate [auto]    -g n  set audio hardware output gain\n");
   fprintf(stderr,"   -os,-ol,-oh  output to built-in speaker,line-out connector,headphones\n");
#ifdef NAS
   fprintf(stderr,"                                        -a d  set NAS server\n");
#elif defined(SGI)
   fprintf(stderr,"                                        -a [1..4] set RAD device\n");
#else
   fprintf(stderr,"                                        -a d  set audio device\n");
#endif
   fprintf(stderr,"   -2    downsample 1:2 (22 kHz)        -4    downsample 1:4 (11 kHz)\n");
   fprintf(stderr,"   -d n  play every n'th frame only     -h n  play every frame n times\n");
   fprintf(stderr,"   -0    decode channel 0 (left) only   -1    decode channel 1 (right) only\n");
   fprintf(stderr,"   -m    mix both channels (mono)       -p p  use HTTP proxy p [$HTTP_PROXY]\n");
#ifdef SET_RT
   fprintf(stderr,"   -@ f  read filenames/URLs from f     -T get realtime priority\n");
#else
   fprintf(stderr,"   -@ f  read filenames/URLs from f\n");
#endif
   fprintf(stderr,"   -z    shuffle play (with wildcards)  -Z    random play\n");
   fprintf(stderr,"   -u a  HTTP authentication string     -E f  Equalizer, data from file\n");
   fprintf(stderr,"   -C    enable control keys\n");
   fprintf(stderr,"See the manpage %s(1) or call %s with --longhelp for more information.\n", prgName,prgName);
   exit(1);
}

static void long_usage(char *d)
{
  FILE *o = stderr;

  print_title();
  fprintf(stderr,"\nusage: %s [option(s)] [file(s) | URL(s) | -]\n", prgName);
  fprintf(stderr,"supported options:\n");
  fprintf(o,"\n -k <n> --skip <n>         \n");
  fprintf(o," -a <f> --audiodevice <f>  \n");
  fprintf(o," -2     --2to1             2:1 Downsampling\n");
  fprintf(o," -4     --4to1             4:1 Downsampling\n");
  fprintf(o," -t     --test             \n");
  fprintf(o," -s     --stdout           \n");
  fprintf(o," -S     --STDOUT           Play AND output stream (not implemented yet)\n");
  fprintf(o," -c     --check            \n");
  fprintf(o," -v[*]  --verbose          Increase verboselevel\n");
  fprintf(o," -q     --quiet            Enables quiet mode\n");
  fprintf(o," -y     --resync           DISABLES resync on error\n");
  fprintf(o," -0     --left --single0   Play only left channel\n");
  fprintf(o," -1     --right --single1  Play only right channel\n");
  fprintf(o," -m     --mono --mix       Mix stereo to mono\n");
  fprintf(o,"        --stereo           Duplicate mono channel\n");
  fprintf(o,"        --reopen           Force close/open on audiodevice\n");
  fprintf(o," -g     --gain             Set audio hardware output gain\n");
  fprintf(o," -r     --rate             Force a specific audio output rate\n");
  fprintf(o,"        --8bit             Force 8 bit output\n");
  fprintf(o," -o h   --headphones       Output on headphones\n");
  fprintf(o," -o s   --speaker          Output on speaker\n");
  fprintf(o," -o l   --lineout          Output to lineout\n");
  fprintf(o," -f <n> --scale <n>        Scale output samples (soft gain)\n");
  fprintf(o," -n     --frames <n>       Play only <n> frames of every stream\n");
  fprintf(o," -b <n> --buffer <n>       Set play buffer (\"output cache\")\n");
#ifdef TERM_CONTROL
  fprintf(o," -C     --control          Enable control keys\n");
#endif
#if 0
  fprintf(o," -R     --remote		Generic remote interface\n");
#endif
  fprintf(o," -d     --doublespeed      Play only every second frame\n");
  fprintf(o," -h     --halfspeed        Play every frame twice\n");
  fprintf(o," -p <f> --proxy <f>        Set WWW proxy\n");
  fprintf(o," -@ <f> --list <f>         Play songs in <f> file-list\n");
  fprintf(o," -z     --shuffle          Shuffle song-list before playing\n");
  fprintf(o," -Z     --random           full random play\n");
  fprintf(o,"        --equalizer        Exp.: scales freq. bands acrd. to 'equalizer.dat'\n");
  fprintf(o,"        --aggressive       Tries to get higher priority (nice)\n");
  fprintf(o," -u     --auth             Set auth values for HTTP access\n");
#if defined(SET_RT)
  fprintf(o," -T     --realtime         Tries to get realtime priority\n");
#endif
  fprintf(o," -w <f> --wav <f>          Writes samples as WAV file in <f> (- is stdout)\n");
  fprintf(o,"        --au <f>           Writes samples as Sun AU file in <f> (- is stdout)\n");
  fprintf(o,"        --cdr <f>          Writes samples as CDR file in <f> (- is stdout)\n");

  fprintf(o,"\nSee the manpage %s(1) for more information.\n", prgName);
  exit(0);
}


