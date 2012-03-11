/* mp3enc - encode audio files in MP3 format */

/*
 *	Command line frontend program
 *
 *	Copyright (c) 1999 Mark Taylor
 *                    2000 Takehiro TOMIANGA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: main.c,v 1.46 2001/03/11 11:24:25 aleidinger Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <memory.h>
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/*
 main.c is example code for how to use libmp3lame.a.  To use this library,
 you only need the library and lame.h.  All other .h files are private
 to the library.
*/
#include "lame.h"

#include "brhist.h"
#include "parse.h"
#include "main.h"
#include "get_audio.h"
#include "timestatus.h"

/*
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
*/

int
parse_args_from_string(lame_global_flags *const gfp, const char *p)
{
	/* Quick & very Dirty */
	int c = 0, ret;
	char *f, *q, *r[128];

	if (p == NULL || *p == '\0')
		return 0;

	f = q = malloc(strlen(p) + 1);
	strcpy (q, p);

	r[c++] = "lhama";
	for (;;) {
		r[c++] = q;
		while (*q != ' ' && *q != '\0')
			q++;
		if (*q == '\0')
			break;
		*q++ = '\0';
	}
	r[c] = NULL;

	ret = parse_args(gfp, c, r);
	free(f);
	return ret;
}

int
main(int argc, char **argv)
{
	int i, iread, imp3, ret;
	short Buffer[2][1152];
	unsigned char mp3buffer[LAME_MAXMP3BUFFER];
	FILE *outf;
	lame_global_flags *gf;
	static char inPath[]  = "-";
	static char outPath[] = "-";
	static const char *mode_names[2][4] = {
		{ "stereo", "j-stereo", "dual-ch", "single-ch" },
		{ "stereo", "force-ms", "dual-ch", "single-ch" }
	};

	/* initialize libmp3lame */
	input_format = sf_unknown;
	if (NULL == (gf = lame_init()) ) {
		fprintf(stderr, "mp3enc: fatal error during initialization\n");
		return 1;
	}
	/*
	 * parse the command line arguments, setting various flags in the
	 * struct 'gf'.  If you want to parse your own arguments,
	 * or call libmp3lame from a program which uses a GUI to set arguments,
	 * skip this call and set the values of interest in the gf struct.
	 * (see the file API & lame.h for documentation about these parameters)
	 */
	parse_args_from_string(gf, getenv("LAMEOPT"));
	ret = parse_args(gf, argc, argv);
	if (ret < 0)
		return ret == -2? 0: 1;
	if (update_interval < 0.)
		update_interval = 2.;

	/*
	 * open the wav/aiff/raw pcm or mp3 input file.  This call will
	 * open the file, try to parse the headers and
	 * set gf.samplerate, gf.num_channels, gf.num_samples.
	 * if you want to do your own file input, skip this call and set
	 * samplerate, num_channels and num_samples yourself.
	 */
	init_infile(gf, inPath);
	if ((outf = init_outfile(outPath, lame_get_decode_only(gf))) == NULL) {
		fprintf(stderr, "Can't init outfile '%s'\n", outPath);
		return -42;
	}

	/*
	 * Now that all the options are set, lame needs to analyze them and
	 * set some more internal options and check for problems
	 */
	i = lame_init_params(gf);
	if (i < 0) {
		if (i == -1)
			display_bitrates(stderr);
		fprintf(stderr, "fatal error during initialization\n");
		return 1;
	}

	if (silent || gf->VBR == vbr_off)
		brhist = 0;			/* turn off VBR histogram */

#ifdef BRHIST
	if (brhist) {
		if (brhist_init(gf, gf->VBR_min_bitrate_kbps,
		    gf->VBR_max_bitrate_kbps))
			/* fail to initialize */
			brhist = 0;
	} else
		brhist_init(gf, 128, 128);	/* Dirty hack */
#endif


#ifdef HAVE_VORBIS
	if (lame_get_ogg( gf ) ) {
		lame_encode_ogg_init(gf);
		gf->VBR = vbr_off;	/* ignore lame's various VBR modes */
	}
#endif
	if (0 == lame_get_decode_only( gf ) && silent < 10 ) {
		if (0)
			fprintf(stderr, "Encoding as %g kHz ",
				1.e-3 * lame_get_out_samplerate( gf ) );

		if (lame_get_ogg(gf)) {
			if (0)
				fprintf(stderr, "VBR Ogg Vorbis\n" );
		} else {
			const char *appendix = "";

			switch (gf->VBR ) {
			case vbr_mt:
			case vbr_rh:
			case vbr_mtrh:
				appendix = "ca. ";
				if (0)
					fprintf(stderr, "VBR(q=%i)", gf->VBR_q );
				break;
			case vbr_abr:
				if (0)
					fprintf(stderr, "average %d kbps",
						gf->VBR_mean_bitrate_kbps);
				break;
			default:
				if (0)
					fprintf(stderr, "%3d Kb/s", gf->brate);
				break;
			}
			if (0)
				fprintf(stderr, " %s MPEG-%u%s Layer III (%s%gx) qval=%i\n",
					mode_names[gf->force_ms][gf->mode],
					2 - gf->version,
					lame_get_out_samplerate(gf) < 16000?
					".5": "", appendix,
					0.1 * (int)(10.*gf->compression_ratio + 0.5),
					lame_get_quality(gf));
		}
		fflush(stderr);
	}

	if (lame_get_decode_only(gf))
		/* decode an mp3 file to a .wav */
		if (mp3_delay_set)
			lame_decoder(gf, outf, mp3_delay, inPath, outPath);
		else
			lame_decoder(gf, outf, gf->encoder_delay, inPath, outPath);
	else {
		/* encode until we hit eof */
		do {
			/* read in 'iread' samples */
			iread = get_audio(gf, Buffer);

			/* status display  */
			if (!silent)
				if (update_interval > 0)
					timestatus_klemm(gf);
				else {
					if (0 == gf->frameNum % 50) {
#ifdef BRHIST
						brhist_jump_back();
#endif
						timestatus(lame_get_out_samplerate(gf),
							gf->frameNum,
							gf->totalframes,
							gf->framesize );
#ifdef BRHIST
						if (brhist)
							brhist_disp(gf);
#endif
					}
				}

			/* encode */
			imp3 = lame_encode_buffer(gf, Buffer[0], Buffer[1], iread,
				mp3buffer, sizeof(mp3buffer));

			/* was our output buffer big enough? */
			if (imp3 < 0) {
				if (imp3 == -1)
					fprintf(stderr, "mp3 buffer is not big enough... \n");
				else
					fprintf(stderr, "mp3 internal error:  error code=%i\n", imp3);
				return 1;
			}

			if (fwrite(mp3buffer, 1, imp3, outf) != imp3) {
				fprintf(stderr, "Error writing mp3 output \n");
				return 1;
			}
		} while (iread);
		/* may return one more mp3 frame */
		imp3 = lame_encode_flush(gf, mp3buffer, sizeof(mp3buffer));
		if (imp3 < 0) {
			if (imp3 == -1)
				fprintf(stderr, "mp3 buffer is not big enough... \n");
			else
				fprintf(stderr, "mp3 internal error:  error code=%i\n", imp3);
			return 1;

		}
		if (!silent) {
#ifdef BRHIST
			brhist_jump_back();
#endif
			timestatus(lame_get_out_samplerate(gf),
				gf->frameNum,
				gf->totalframes,
				gf->framesize);
#ifdef BRHIST
			if (brhist)
				brhist_disp(gf);
			brhist_disp_total(gf);
#endif
			timestatus_finish();
		}

		fwrite(mp3buffer, 1, imp3, outf);
		lame_mp3_tags_fid(gf, outf);	/* add VBR tags to mp3 file */
		lame_close(gf);
		fclose(outf);
	}
	close_infile();
	exit(0);
	return 0;
}
