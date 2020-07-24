///////////////////////////////////////////////////////////////////////////////
//
/// \file       xzdec.c
/// \brief      Simple single-threaded tool to uncompress .xz or .lzma files
//
//  Author:     Lasse Collin
//  Modified by: Michael Schierl
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"
#include "lzma.h"
#include "tuklib_common.h"

#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"


static void
tuklib_exit(int status, int err_status, int show_error)
{
	if (status != err_status) {
		// Close stdout. If something goes wrong,
		// print an error message to stderr.
		const int ferror_err = ferror(stdout);
		const int fclose_err = fclose(stdout);
		if (ferror_err || fclose_err) {
			status = err_status;

			// If it was fclose() that failed, we have the reason
			// in errno. If only ferror() indicated an error,
			// we have no idea what the reason was.
			if (show_error)
				fprintf(stderr, "xzdec: %s: %s\n",
						"Writing to standard "
							"output failed",
						fclose_err ? strerror(errno)
							: "Unknown error");
		}
	}

	if (status != err_status) {
		// Close stderr. If something goes wrong, there's
		// nothing where we could print an error message.
		// Just set the exit status.
		const int ferror_err = ferror(stderr);
		const int fclose_err = fclose(stderr);
		if (fclose_err || ferror_err)
			status = err_status;
	}

	exit(status);
}


/// Error messages are suppressed if this is zero, which is the case when
/// --quiet has been given at least twice.
static int display_errors = 2;


static void lzma_attribute((__format__(__printf__, 1, 2)))
my_errorf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	if (display_errors) {
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
	}

	va_end(ap);
	return;
}

static void
help(void)
{
	printf(
"Usage: xzdec [OPTION]... [FILE]...\n"
"Decompress files in the .xz format to standard output.\n"
"\n"
"  -d, --decompress   (ignored, only decompression is supported)\n"
"  -k, --keep         (ignored, files are never deleted)\n"
"  -c, --stdout       (ignored, output is always written to standard output)\n"
"  -q, --quiet        specify *twice* to suppress errors\n"
"  -Q, --no-warn      (ignored, the exit status 2 is never used)\n"
"  -h, --help         display this help and exit\n"
"  -V, --version      display the version number and exit\n"
"\n"
"With no FILE, or when FILE is -, read standard input.\n");

	tuklib_exit(EXIT_SUCCESS, EXIT_FAILURE, display_errors);
}

static void
version(void)
{
	printf("xzdec 5.2.5-min\n");
	tuklib_exit(EXIT_SUCCESS, EXIT_FAILURE, display_errors);
}


/// Parses command line options.
static void
parse_options(int argc, char **argv)
{
	static const char short_opts[] = "cdkM:hqQV";
	static const struct option long_opts[] = {
		{ "stdout",       no_argument,         NULL, 'c' },
		{ "to-stdout",    no_argument,         NULL, 'c' },
		{ "decompress",   no_argument,         NULL, 'd' },
		{ "uncompress",   no_argument,         NULL, 'd' },
		{ "keep",         no_argument,         NULL, 'k' },
		{ "quiet",        no_argument,         NULL, 'q' },
		{ "no-warn",      no_argument,         NULL, 'Q' },
		{ "help",         no_argument,         NULL, 'h' },
		{ "version",      no_argument,         NULL, 'V' },
		{ NULL,           0,                   NULL, 0   }
	};

	int c;

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL))
			!= -1) {
		switch (c) {
		case 'c':
		case 'd':
		case 'k':
		case 'Q':
			break;

		case 'q':
			if (display_errors > 0)
				--display_errors;

			break;

		case 'h':
			help();

		case 'V':
			version();

		default:
			exit(EXIT_FAILURE);
		}
	}

	return;
}


static void
uncompress(lzma_stream *strm, FILE *file, const char *filename)
{
	lzma_ret ret;

	// Initialize the decoder
#ifdef LZMADEC
	ret = lzma_alone_decoder(strm, UINT64_MAX);
#else
	ret = lzma_stream_decoder(strm, UINT64_MAX, LZMA_CONCATENATED);
#endif

	// The only reasonable error here is LZMA_MEM_ERROR.
	if (ret != LZMA_OK) {
		my_errorf("%s", ret == LZMA_MEM_ERROR ? strerror(ENOMEM)
				: "Internal error (bug)");
		exit(EXIT_FAILURE);
	}

	// Input and output buffers
	uint8_t in_buf[BUFSIZ];
	uint8_t out_buf[BUFSIZ];

	strm->avail_in = 0;
	strm->next_out = out_buf;
	strm->avail_out = BUFSIZ;

	lzma_action action = LZMA_RUN;

	while (true) {
		if (strm->avail_in == 0) {
			strm->next_in = in_buf;
			strm->avail_in = fread(in_buf, 1, BUFSIZ, file);

			if (ferror(file)) {
				// POSIX says that fread() sets errno if
				// an error occurred. ferror() doesn't
				// touch errno.
				my_errorf("%s: Error reading input file: %s",
						filename, strerror(errno));
				exit(EXIT_FAILURE);
			}

#ifndef LZMADEC
			// When using LZMA_CONCATENATED, we need to tell
			// liblzma when it has got all the input.
			if (feof(file))
				action = LZMA_FINISH;
#endif
		}

		ret = lzma_code(strm, action);

		// Write and check write error before checking decoder error.
		// This way as much data as possible gets written to output
		// even if decoder detected an error.
		if (strm->avail_out == 0 || ret != LZMA_OK) {
			const size_t write_size = BUFSIZ - strm->avail_out;

			if (fwrite(out_buf, 1, write_size, stdout)
					!= write_size) {
				// Wouldn't be a surprise if writing to stderr
				// would fail too but at least try to show an
				// error message.
				my_errorf("Cannot write to standard output: "
						"%s", strerror(errno));
				exit(EXIT_FAILURE);
			}

			strm->next_out = out_buf;
			strm->avail_out = BUFSIZ;
		}

		if (ret != LZMA_OK) {
			if (ret == LZMA_STREAM_END) {
#ifdef LZMADEC
				// Check that there's no trailing garbage.
				if (strm->avail_in != 0
						|| fread(in_buf, 1, 1, file)
							!= 0
						|| !feof(file))
					ret = LZMA_DATA_ERROR;
				else
					return;
#else
				// lzma_stream_decoder() already guarantees
				// that there's no trailing garbage.
				assert(strm->avail_in == 0);
				assert(action == LZMA_FINISH);
				assert(feof(file));
				return;
#endif
			}

			const char *msg;
			switch (ret) {
			case LZMA_MEM_ERROR:
				msg = strerror(ENOMEM);
				break;

			case LZMA_FORMAT_ERROR:
				msg = "File format not recognized";
				break;

			case LZMA_OPTIONS_ERROR:
				// FIXME: Better message?
				msg = "Unsupported compression options";
				break;

			case LZMA_DATA_ERROR:
				msg = "File is corrupt";
				break;

			case LZMA_BUF_ERROR:
				msg = "Unexpected end of input";
				break;

			default:
				msg = "Internal error (bug)";
				break;
			}

			my_errorf("%s: %s", filename, msg);
			exit(EXIT_FAILURE);
		}
	}
}


int
main(int argc, char **argv)
{

	// Parse the command line options.
	parse_options(argc, argv);

	// The same lzma_stream is used for all files that we decode. This way
	// we don't need to reallocate memory for every file if they use same
	// compression settings.
	lzma_stream strm = LZMA_STREAM_INIT;

	// Some systems require setting stdin and stdout to binary mode.

	if (optind == argc) {
		// No filenames given, decode from stdin.
		uncompress(&strm, stdin, "(stdin)");
	} else {
		// Loop through the filenames given on the command line.
		do {
			// "-" indicates stdin.
			if (strcmp(argv[optind], "-") == 0) {
				uncompress(&strm, stdin, "(stdin)");
			} else {
				FILE *file = fopen(argv[optind], "rb");
				if (file == NULL) {
					my_errorf("%s: %s", argv[optind],
							strerror(errno));
					exit(EXIT_FAILURE);
				}

				uncompress(&strm, file, argv[optind]);
				fclose(file);
			}
		} while (++optind < argc);
	}

	tuklib_exit(EXIT_SUCCESS, EXIT_FAILURE, display_errors);
}
