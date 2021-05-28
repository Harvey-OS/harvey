/*
 *  Clzip - LZMA lossless data compressor
 * Copyright (C) 2010-2017 Antonio Diaz Diaz.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Exit status: 0 for a normal exit, 1 for environmental problems
 * (file not found, invalid flags, I/O errors, etc), 2 to indicate a
 * corrupt or invalid input file, 3 for an internal consistency error
 * (eg, bug) which caused lzip to panic.
 */

#define _DEFINE_INLINES
#include "lzip.h"
#include "decoder.h"
#include "encoder_base.h"
#include "encoder.h"
#include "fast_encoder.h"

int	verbosity = 0;

char *argv0 = "lzip";

struct {
	char * from;
	char * to;
} known_extensions[] = {
	{ ".lz",  ""     },
	{ ".tlz", ".tar" },
	{ 0,      0      }
};

typedef struct Lzma_options Lzma_options;
struct Lzma_options {
	int	dict_size;		/* 4 KiB .. 512 MiB */
	int	match_len_limit;	/* 5 .. 273 */
};

enum Mode { m_compress, m_decompress, };

char	*output_filename = nil;
int	outfd = -1;
bool delete_output_on_interrupt = false;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [-[0-9]cdv] [file...]\n", argv0);
	exit(2);
}

char *
bad_version(unsigned version)
{
	static char buf[80];

	snprintf(buf, sizeof buf, "Version %ud member format not supported.",
	    version);
	return buf;
}

char *
format_ds(unsigned dict_size)
{
	enum { bufsize = 16, factor = 1024 };
	char *prefix[8] = { "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi" };
	char *p = "";
	char *np = "  ";
	unsigned num = dict_size, i;
	bool exact = (num % factor == 0);
	static char buf[bufsize];

	for (i = 0; i < 8 && (num > 9999 || (exact && num >= factor)); ++i) {
		num /= factor;
		if (num % factor != 0)
			exact = false;
		p = prefix[i];
		np = "";
	}
	snprintf( buf, bufsize, "%s%4ud %sB", np, num, p );
	return buf;
}

static void
show_header(unsigned dict_size)
{
	if (verbosity >= 3)
		fprintf(stderr, "dictionary %s.  ", format_ds( dict_size) );
}

static uvlong
getnum(char *ptr, uvlong llimit, uvlong ulimit)
{
	int bad;
	uvlong result;
	char	*tail;

	bad = 0;
	result = strtoull(ptr, &tail, 0);
	if (tail == ptr) {
		show_error( "Bad or missing numerical argument.", 0, true );
		exit(1);
	}

	if (!errno && tail[0]) {
		unsigned factor = (tail[1] == 'i') ? 1024 : 1000;
		int	i, exponent = 0;		/* 0 = bad multiplier */

		switch (tail[0]) {
		case 'Y':
			exponent = 8;
			break;
		case 'Z':
			exponent = 7;
			break;
		case 'E':
			exponent = 6;
			break;
		case 'P':
			exponent = 5;
			break;
		case 'T':
			exponent = 4;
			break;
		case 'G':
			exponent = 3;
			break;
		case 'M':
			exponent = 2;
			break;
		case 'K':
			if (factor == 1024)
				exponent = 1;
			break;
		case 'k':
			if (factor == 1000)
				exponent = 1;
			break;
		}
		if (exponent <= 0) {
			show_error( "Bad multiplier in numerical argument.", 0, true );
			exit(1);
		}
		for (i = 0; i < exponent; ++i) {
			if (ulimit / factor >= result)
				result *= factor;
			else {
				bad++;
				break;
			}
		}
	}
	if (bad || result < llimit || result > ulimit) {
		show_error( "Numerical argument out of limits.", 0, false );
		exit(1);
	}
	return result;
}

static int
get_dict_size(char *arg)
{
	char	*tail;
	long bits = strtol(arg, &tail, 0);

	if (bits >= min_dict_bits &&
	    bits <= max_dict_bits && *tail == 0)
		return (1 << bits);
	return getnum(arg, min_dict_size, max_dict_size);
}

void
set_mode(enum Mode *program_modep, enum Mode new_mode)
{
	if (*program_modep != m_compress && *program_modep != new_mode) {
		show_error( "Only one operation can be specified.", 0, true );
		exit(1);
	}
	*program_modep = new_mode;
}

static int
extension_index(char *name)
{
	int	eindex;

	for (eindex = 0; known_extensions[eindex].from; ++eindex) {
		char * ext = known_extensions[eindex].from;
		unsigned name_len = strlen(name);
		unsigned ext_len = strlen(ext);

		if (name_len > ext_len &&
		    strncmp(name + name_len - ext_len, ext, ext_len) == 0)
			return eindex;
	}
	return - 1;
}

int
open_instream(char *name, Dir *, bool, bool)
{
	int infd = open(name, OREAD);

	if (infd < 0)
		show_file_error( name, "Can't open input file", errno );
	return infd;
}

static int
open_instream2(char *name, Dir *in_statsp, enum Mode program_mode,
	int eindex, bool recompress, bool to_stdout)
{
	bool no_ofile = to_stdout;

	if (program_mode == m_compress && !recompress && eindex >= 0) {
		if (verbosity >= 0)
			fprintf( stderr, "%s: Input file '%s' already has '%s' suffix.\n",
			    argv0, name, known_extensions[eindex].from);
		return - 1;
	}
	return open_instream(name, in_statsp, no_ofile, false);
}

/* assure at least a minimum size for buffer 'buf' */
void *
resize_buffer(void *buf, unsigned min_size)
{
	buf = realloc(buf, min_size);
	if (!buf) {
		show_error("Not enough memory.", 0, false);
		cleanup_and_fail(1);
	}
	return buf;
}

static void
set_c_outname(char *name, bool multifile)
{
	output_filename = resize_buffer(output_filename, strlen(name) + 5 +
	    strlen(known_extensions[0].from) + 1);
	strcpy(output_filename, name);
	if (multifile)
		strcat( output_filename, "00001" );
	strcat(output_filename, known_extensions[0].from);
}

static void
set_d_outname(char *name, int eindex)
{
	unsigned name_len = strlen(name);
	if (eindex >= 0) {
		char * from = known_extensions[eindex].from;
		unsigned from_len = strlen(from);

		if (name_len > from_len) {
			output_filename = resize_buffer(output_filename, name_len +
			    strlen(known_extensions[eindex].to) + 1);
			strcpy(output_filename, name);
			strcpy(output_filename + name_len - from_len, known_extensions[eindex].to);
			return;
		}
	}
	output_filename = resize_buffer(output_filename, name_len + 4 + 1);
	strcpy(output_filename, name);
	strcat(output_filename, ".out");
	if (verbosity >= 1)
		fprintf( stderr, "%s: Can't guess original name for '%s' -- using '%s'\n",
		    argv0, name, output_filename);
}

static bool
open_outstream(bool force, bool)
{
	int flags = OWRITE;

	if (force)
		flags |= OTRUNC;
	else
		flags |= OEXCL;

	outfd = create(output_filename, flags, 0666);
	if (outfd >= 0)
		delete_output_on_interrupt = true;
	else if (verbosity >= 0)
		fprintf(stderr, "%s: Can't create output file '%s': %r\n",
		    argv0, output_filename);
	return outfd >= 0;
}

static bool
check_tty(int, enum Mode program_mode)
{
	if (program_mode == m_compress && isatty(outfd) ||
	    program_mode == m_decompress && isatty(infd)) {
		usage();
		return false;
	}
	return true;
}

void
cleanup_and_fail(int retval)
{
	if (delete_output_on_interrupt) {
		delete_output_on_interrupt = false;
		if (verbosity >= 0)
			fprintf(stderr, "%s: Deleting output file '%s', if it exists.\n",
			    argv0, output_filename);
		if (outfd >= 0) {
			close(outfd);
			outfd = -1;
		}
		if (remove(output_filename) != 0)
			fprintf(stderr, "%s: can't remove output file %s: %r\n",
				argv0, output_filename);
	}
	exit(retval);
}

/* Set permissions, owner and times. */
static void
close_and_set_permissions(Dir *)
{
	if (close(outfd) != 0) {
		show_error( "Error closing output file", errno, false );
		cleanup_and_fail(1);
	}
	outfd = -1;
	delete_output_on_interrupt = false;
}

static bool
next_filename(void)
{
	int i, j;
	unsigned name_len = strlen(output_filename);
	unsigned ext_len = strlen(known_extensions[0].from);

	if ( name_len >= ext_len + 5 )			/* "*00001.lz" */
		for (i = name_len - ext_len - 1, j = 0; j < 5; --i, ++j) {
			if (output_filename[i] < '9') {
				++output_filename[i];
				return true;
			} else
				output_filename[i] = '0';
		}
	return false;
}

typedef struct Poly_encoder Poly_encoder;
struct Poly_encoder {
	LZ_encoder_base *eb;
	LZ_encoder *e;
	FLZ_encoder *fe;
};

static int
compress(uvlong member_size, uvlong volume_size,
	int infd, Lzma_options *encoder_options, Pretty_print *pp,
	Dir *in_statsp, bool zero)
{
	int retval = 0;
	uvlong in_size = 0, out_size = 0, partial_volume_size = 0;
	uvlong cfile_size = in_statsp? in_statsp->length / 100: 0;
	Poly_encoder encoder = { 0, 0, 0 }; /* polymorphic encoder */
	bool error = false;

	if (verbosity >= 1)
		Pp_show_msg(pp, 0);

	if (zero) {
		encoder.fe = (FLZ_encoder *)malloc(sizeof * encoder.fe);
		if (!encoder.fe || !FLZe_init(encoder.fe, infd, outfd))
			error = true;
		else
			encoder.eb = &encoder.fe->eb;
	} else {
		File_header header;

		if (Fh_set_dict_size(header, encoder_options->dict_size) &&
		    encoder_options->match_len_limit >= min_match_len_limit &&
		    encoder_options->match_len_limit <= max_match_len)
			encoder.e = (LZ_encoder *)malloc(sizeof * encoder.e);
		else
			internal_error( "invalid argument to encoder." );
		if (!encoder.e || !LZe_init(encoder.e, Fh_get_dict_size(header),
		    encoder_options->match_len_limit, infd, outfd))
			error = true;
		else
			encoder.eb = &encoder.e->eb;
	}
	if (error) {
		Pp_show_msg( pp, "Not enough memory. Try a smaller dictionary size." );
		return 1;
	}

	for(;;) {			/* encode one member per iteration */
		uvlong size;
		vlong freevolsz;

		size = member_size;
		if (volume_size > 0) {
			freevolsz = volume_size - partial_volume_size;
			if (size > freevolsz)
				size = freevolsz;	/* limit size */
		}
		show_progress(in_size, &encoder.eb->mb, pp, cfile_size); /* init */
		if ((zero && !FLZe_encode_member(encoder.fe, size)) ||
		    (!zero && !LZe_encode_member(encoder.e, size))) {
			Pp_show_msg( pp, "Encoder error." );
			retval = 1;
			break;
		}
		in_size += Mb_data_position(&encoder.eb->mb);
		out_size += Re_member_position(&encoder.eb->renc);
		if (Mb_data_finished(&encoder.eb->mb))
			break;
		if (volume_size > 0) {
			partial_volume_size += Re_member_position(&encoder.eb->renc);
			if (partial_volume_size >= volume_size - min_dict_size) {
				partial_volume_size = 0;
				if (delete_output_on_interrupt) {
					close_and_set_permissions(in_statsp);
					if (!next_filename()) {
						Pp_show_msg( pp, "Too many volume files." );
						retval = 1;
						break;
					}
					if (!open_outstream(true, !in_statsp)) {
						retval = 1;
						break;
					}
				}
			}
		}
		if (zero)
			FLZe_reset(encoder.fe);
		else
			LZe_reset(encoder.e);
	}

	if (retval == 0 && verbosity >= 1)
		if (in_size == 0 || out_size == 0)
			fputs( " no data compressed.\n", stderr );
		else {
			if (0)
				fprintf(stderr,
				    "%6.3f:1, %6.3f bits/byte, %5.2f%% saved, ",
				    (double)in_size / out_size,
				    (8.0 * out_size) / in_size,
				    100.0 * (1.0 - (double)out_size/in_size));
			fprintf(stderr, "%llud in, %llud out.\n",
				in_size, out_size);
		}
	LZeb_free(encoder.eb);
	if (zero)
		free(encoder.fe);
	else
		free(encoder.e);
	return retval;
}

static uchar
xdigit(unsigned value)
{
	if (value <= 9)
		return '0' + value;
	if (value <= 15)
		return 'A' + value - 10;
	return 0;
}

static bool
show_trailing_data(uchar *data, int size, Pretty_print *pp, bool all,
	bool ignore_trailing)
{
	if (verbosity >= 4 || !ignore_trailing) {
		char buf[128];
		int i, len = snprintf(buf, sizeof buf, "%strailing data = ",
		    all? "": "first bytes of ");

		if (len < 0)
			len = 0;
		for (i = 0; i < size && len + 2 < sizeof buf; ++i) {
			buf[len++] = xdigit(data[i] >> 4);
			buf[len++] = xdigit(data[i] & 0x0F);
			buf[len++] = ' ';
		}
		if (len < sizeof buf)
			buf[len++] = '\'';
		for (i = 0; i < size && len < sizeof buf; ++i) {
			if (isprint(data[i]))
				buf[len++] = data[i];
			else
				buf[len++] = '.';
		}
		if (len < sizeof buf)
			buf[len++] = '\'';
		if (len < sizeof buf)
			buf[len] = 0;
		else
			buf[sizeof buf - 1] = 0;
		Pp_show_msg(pp, buf);
		if (!ignore_trailing)
			show_file_error(pp->name, trailing_msg, 0);
	}
	return ignore_trailing;
}

static int
decompress(int infd, Pretty_print *pp, bool ignore_trailing)
{
	uvlong partial_file_pos = 0;
	Range_decoder rdec;
	int retval = 0;
	bool first_member;

	if (!Rd_init(&rdec, infd)) {
		show_error( "Not enough memory.", 0, false );
		cleanup_and_fail(1);
	}

	for (first_member = true; ; first_member = false) {
		int result, size;
		unsigned dict_size;
		File_header header;
		LZ_decoder decoder;

		Rd_reset_member_position(&rdec);
		size = Rd_read_data(&rdec, header, Fh_size);
		if (Rd_finished(&rdec))		/* End Of File */ {
			if (first_member || Fh_verify_prefix(header, size)) {
				Pp_show_msg( pp, "File ends unexpectedly at member header." );
				retval = 2;
			} else if (size > 0 && !show_trailing_data(header, size, pp,
			    true, ignore_trailing))
				retval = 2;
			break;
		}
		if (!Fh_verify_magic(header)) {
			if (first_member) {
				show_file_error(pp->name, bad_magic_msg, 0);
				retval = 2;
			} else if (!show_trailing_data(header, size, pp,
			    false, ignore_trailing))
				retval = 2;
			break;
		}
		if (!Fh_verify_version(header)) {
			Pp_show_msg(pp, bad_version(Fh_version(header)));
			retval = 2;
			break;
		}
		dict_size = Fh_get_dict_size(header);
		if (!isvalid_ds(dict_size)) {
			Pp_show_msg(pp, bad_dict_msg);
			retval = 2;
			break;
		}

		if (verbosity >= 2 || (verbosity == 1 && first_member)) {
			Pp_show_msg(pp, 0);
			show_header(dict_size);
		}

		if (!LZd_init(&decoder, &rdec, dict_size, outfd)) {
			Pp_show_msg( pp, "Not enough memory." );
			retval = 1;
			break;
		}
		result = LZd_decode_member(&decoder, pp);
		partial_file_pos += Rd_member_position(&rdec);
		LZd_free(&decoder);
		if (result != 0) {
			if (verbosity >= 0 && result <= 2) {
				Pp_show_msg(pp, 0);
				fprintf(stderr, "%s: %s at pos %llud\n",
					argv0, (result == 2?
					"file ends unexpectedly":
					"decoder error"), partial_file_pos);
			}
			retval = 2;
			break;
		}
		if (verbosity >= 2) {
			fputs("done\n", stderr);
			Pp_reset(pp);
		}
	}
	Rd_free(&rdec);
	if (verbosity == 1 && retval == 0)
		fputs("done\n", stderr);
	return retval;
}

void
signal_handler(int sig)
{
	USED(sig);
	show_error("interrupt caught, quitting.", 0, false);
	cleanup_and_fail(1);
}

static void
set_signals(void)
{
}

void
show_error(char *msg, int, bool help)
{
	if (verbosity < 0)
		return;
	if (msg && msg[0])
		fprintf(stderr, "%s: %s: %r\n", argv0, msg);
	if (help)
		fprintf(stderr, "Try '%s --help' for more information.\n",
		    argv0);
}

void
show_file_error(char *filename, char *msg, int errcode)
{
	if (verbosity < 0)
		return;
	fprintf(stderr, "%s: %s: %s", argv0, filename, msg);
	if (errcode > 0)
		fprintf(stderr, ": %r");
	fputc('\n', stderr);
}

void
internal_error(char *msg)
{
	if (verbosity >= 0)
		fprintf( stderr, "%s: internal error: %s\n", argv0, msg );
	exit(3);
}

void
show_progress(uvlong partial_size, Matchfinder_base *m,
	Pretty_print *p, uvlong cfile_size)
{
	static uvlong psize = 0, csize = 0; /* csize=file_size/100 */
	static Matchfinder_base *mb = 0;
	static Pretty_print *pp = 0;

	if (verbosity < 2)
		return;
	if (m) {				/* initialize static vars */
		csize = cfile_size;
		psize = partial_size;
		mb = m;
		pp = p;
	}
	if (mb && pp) {
		uvlong pos = psize + Mb_data_position(mb);

		if (csize > 0)
			fprintf( stderr, "%4llud%%", pos / csize );
		fprintf( stderr, "  %.1f MB\r", pos / 1000000.0 );
		Pp_reset(pp);
		Pp_show_msg(pp, 0);	/* restore cursor position */
	}
}

/*
 * Mapping from gzip/bzip2 style 1..9 compression modes to the corresponding
 * LZMA compression modes.
 */
static Lzma_options option_mapping[] = {
	{ 1 << 16,  16 },
	{ 1 << 20,   5 },
	{ 3 << 19,   6 },
	{ 1 << 21,   8 },
	{ 3 << 20,  12 },
	{ 1 << 22,  20 },
	{ 1 << 23,  36 },
	{ 1 << 24,  68 },
	{ 3 << 23, 132 },
//	{ 1 << 25, max_match_len },	// TODO
	{ 1 << 26, max_match_len },
};

void
main(int argc, char *argv[])
{
	int num_filenames, infd, i, retval = 0;
	bool filenames_given = false, force = false, ignore_trailing = true,
		recompress = false,
		stdin_used = false, to_stdout = false, zero = false;
	uvlong max_member_size = 0x0008000000000000ULL;
	uvlong max_volume_size = 0x4000000000000000ULL;
	uvlong member_size = max_member_size;
	uvlong volume_size = 0;
	char *default_output_filename = "";
	char **filenames = nil;
	enum Mode program_mode = m_compress;
	Lzma_options encoder_options = option_mapping[6];  /* default = "-6" */
	Pretty_print pp;

	CRC32_init();

	ARGBEGIN {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		zero = (ARGC() == '0');
		encoder_options = option_mapping[ARGC() - '0'];
		break;
	case 'a':
		ignore_trailing = false;
		break;
	case 'b':
		member_size = getnum(EARGF(usage()), 100000, max_member_size);
		break;
	case 'c':
		to_stdout = true;
		break;
	case 'd':
		set_mode(&program_mode, m_decompress);
		break;
	case 'f':
		force = true;
		break;
	case 'F':
		recompress = true;
		break;
	case 'm':
		encoder_options.match_len_limit =
		    getnum(EARGF(usage()), min_match_len_limit, max_match_len);
		zero = false;
		break;
	case 'o':
		default_output_filename = EARGF(usage());
		break;
	case 'q':
		verbosity = -1;
		break;
	case 's':
		encoder_options.dict_size = get_dict_size(EARGF(usage()));
		zero = false;
		break;
	case 'S':
		volume_size = getnum(EARGF(usage()), 100000, max_volume_size);
		break;
	case 'v':
		if (verbosity < 4)
			++verbosity;
		break;
	default:
		usage();
	} ARGEND

	num_filenames = max(1, argc);
	filenames = resize_buffer(filenames, num_filenames * sizeof filenames[0]);
	filenames[0] = "-";
	for (i = 0; i < argc; ++i) {
		filenames[i] = argv[i];
		if (strcmp(filenames[i], "-") != 0)
			filenames_given = true;
	}

	if (program_mode == m_compress) {
		Dis_slots_init();
		Prob_prices_init();
	}

	if (!to_stdout && (filenames_given || default_output_filename[0]))
		set_signals();

	Pp_init(&pp, filenames, num_filenames, verbosity);

	output_filename = resize_buffer(output_filename, 1);
	for (i = 0; i < num_filenames; ++i) {
		char *input_filename = "";
		int tmp, eindex;
		Dir in_stats;
		Dir *in_statsp;

		output_filename[0] = 0;
		if ( !filenames[i][0] || strcmp( filenames[i], "-" ) == 0 ) {
			if (stdin_used)
				continue;
			else
				stdin_used = true;
			infd = 0;
			if (to_stdout || !default_output_filename[0])
				outfd = 1;
			else {
				if (program_mode == m_compress)
					set_c_outname(default_output_filename,
						volume_size > 0);
				else {
					output_filename = resize_buffer(output_filename,
					    strlen(default_output_filename)+1);
					strcpy(output_filename,
						default_output_filename);
				}
				if (!open_outstream(force, true)) {
					if (retval < 1)
						retval = 1;
					close(infd);
					continue;
				}
			}
		} else {
			eindex = extension_index(input_filename = filenames[i]);
			infd = open_instream2(input_filename, &in_stats,
				program_mode, eindex, recompress, to_stdout);
			if (infd < 0) {
				if (retval < 1)
					retval = 1;
				continue;
			}
			if (to_stdout)
				outfd = 1;
			else {
				if (program_mode == m_compress)
					set_c_outname(input_filename,
						volume_size > 0);
				else
					set_d_outname(input_filename, eindex);
				if (!open_outstream(force, false)) {
					if (retval < 1)
						retval = 1;
					close(infd);
					continue;
				}
			}
		}

		Pp_set_name(&pp, input_filename);
		if (!check_tty(infd, program_mode)) {
			if (retval < 1)
				retval = 1;
			cleanup_and_fail(retval);
		}

		in_statsp = input_filename[0]? &in_stats: nil;
		if (program_mode == m_compress)
			tmp = compress(member_size, volume_size, infd,
				&encoder_options, &pp, in_statsp, zero);
		else
			tmp = decompress(infd, &pp, ignore_trailing);
		if (tmp > retval)
			retval = tmp;
		if (tmp)
			cleanup_and_fail(retval);

		if (delete_output_on_interrupt)
			close_and_set_permissions(in_statsp);
		if (input_filename[0])
			close(infd);
	}
	if (outfd >= 0 && close(outfd) != 0) {
		show_error("Can't close stdout", errno, false);
		if (retval < 1)
			retval = 1;
	}
	free(output_filename);
	free(filenames);
	exit(retval);
}
