/* $Id: mhub.c,v 1.7 2005/07/29 16:40:51 soyt Exp $
******************************************************************************

   mhub - A LibGII -> mouse protocol converter

   Copyright (C) 1998-2000 Marcus Sundberg	[marcus@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#include "config.h" /* For autoconf defines */
#include <ggi/gii.h>
#include <ggi/errors.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#define MAX_OUTPUTS 20

#define PROT_PS2	1
#define PROT_MSC	2
#define PROT_INTELLI	3

typedef struct {
	uint32_t from, to;
} remap;

static char *progname;
static gii_input_t inp;
static int num_dataout = 0;
static int num_butout = 0;
static int data_fds[MAX_OUTPUTS];
static char *data_names[MAX_OUTPUTS];
static int data_prot[MAX_OUTPUTS];
static int but_fds[MAX_OUTPUTS];
static char *but_names[MAX_OUTPUTS];
static int num_remaps = 0;
static remap *remaps = NULL;

static int debug = 0;
static int exit_on_sigpipe = 0;
static unsigned int min_extended_button = 4;
static int wheel_up_button = 0;
static int wheel_down_button = 0;

static uint32_t buttonstate = 0;

static const char missing_arg[] = "%s: missing argument for option '%c'\n";

#define VERSION "0.7"

static const char version[] =
"mhub v%s\n"
"(C) 1998 Marcus Sundberg [marcus@ggi-project.org]\n";

static const char help[] =
"Usage: %s OPTIONS\n"
"Read pointer events from the LibGII default device and output mouse\n"
"protocol data and/or mhub data to the specified files.\n"
"\n"
"  -h                         display this help and exit\n"
"  -v                         output version information and exit\n"
"  -d                         print contents of received events to stderr\n"
"  -s                         terminate program when a SIGPIPE signal is\n"
"                               received. Default is to ignore it\n"
"  -e BUTNR                   only generate mhub events for buttons >= BUTNR\n"
"                               default is 4\n"
"  -b FILE                    output mhub events to FILE\n"
"  -2 FILE                    output PS/2 format data to FILE\n"
"  -i FILE                    output (serial) IntelliMouse format data to FILE\n"
"  -m FILE                    output MouseSystems format data to FILE\n"
"  -r FROM TO                 remap button FROM to button TO\n"
"  -w BUTNR                   transform wheel up motion to a BUTNR mhub event\n"
"  -W BUTNR                   transform wheel down motion to a BUTNR mhub event\n"
"\n"
"A maximum of 20 mhub event outputs and 20 mouse data outputs are supported.\n"
"The number of remaps are unlimited and all remaps occur before any other\n"
"processing.\n"
"Mouse data outputs may all use different protocols.\n"
"\n";

static void
print_version(void)
{
	fprintf(stderr, version, VERSION);
}

static void
print_help(void)
{
	fprintf(stderr, help, progname);
}


static int
parse_args(int *argc, char **argv)
{
	int count = *argc;
	int i;

	for (i = 1; i < count; i++) {
		if (*argv[i] != '-') {
			fprintf(stderr, "%s: unknown arg: %s\n",
				progname, argv[i]);
			return GGI_EARGINVAL;
		}
		switch (argv[i][1]) {
		case 'v':
			print_version();
			return 1;
			break;
		case 'h':
			print_help();
			return 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 's':
			exit_on_sigpipe = 1;
			break;
		case 'b':
			if (num_butout >= MAX_OUTPUTS) {
				fprintf(stderr, "%s: too many button"
					" outputs specified\n", progname);
				return GGI_ENOSPACE;
			}
			if (argv[i][2] != '\0') {
				but_names[num_butout] = argv[i]+2;
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'b');
					return GGI_EARGREQ;
				}
				but_names[num_butout] = argv[i];
			}
			num_butout++;
			break;
		case '2':
			if (num_dataout >= MAX_OUTPUTS) {
				fprintf(stderr, "%s: too many data outputs"
					" specified\n", progname);
				return GGI_ENOSPACE;
			}
			if (argv[i][2] != '\0') {
				data_names[num_dataout] = argv[i]+2;
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, '2');
					return GGI_EARGREQ;
				}
				data_names[num_dataout] = argv[i];
			}
			data_prot[num_dataout] = PROT_PS2;
			num_dataout++;
			break;
		case 'i':
			if (num_dataout >= MAX_OUTPUTS) {
				fprintf(stderr, "%s: too many data outputs"
					" specified\n", progname);
				return GGI_ENOSPACE;
			}
			if (argv[i][2] != '\0') {
				data_names[num_dataout] = argv[i]+2;
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'i');
					return GGI_EARGREQ;
				}
				data_names[num_dataout] = argv[i];
			}
			data_prot[num_dataout] = PROT_INTELLI;
			num_dataout++;
			break;
		case 'm':
			if (num_dataout >= MAX_OUTPUTS) {
				fprintf(stderr, "%s: too many data outputs"
					" specified\n", progname);
				return GGI_ENOSPACE;
			}
			if (argv[i][2] != '\0') {
				data_names[num_dataout] = argv[i]+2;
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'm');
					return GGI_EARGREQ;
				}
				data_names[num_dataout] = argv[i];
			}
			data_prot[num_dataout] = PROT_MSC;
			num_dataout++;
			break;
		case 'e':
			if (argv[i][2] != '\0') {
				min_extended_button = strtol(argv[i]+2, 
							     NULL, 0);
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'e');
					return GGI_EARGREQ;
				}
				min_extended_button = strtol(argv[i], NULL, 0);
			}
			break;
		case 'r':
		{
			int from, to;
			if (argv[i][2] != '\0') {
				from = strtol(argv[i]+2, 
					      NULL, 0);
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'r');
					return GGI_EARGREQ;
				}
				from = strtol(argv[i], NULL, 0);
			}
			if (++i >= count) {
				fprintf(stderr, missing_arg,
					progname, 'r');
				return GGI_EARGREQ;
			}
			to = strtol(argv[i], NULL, 0);	
			if ((remaps = realloc(remaps,
					      sizeof(remap)*(num_remaps+1)))
			    == NULL) {
				fprintf(stderr, "%s: out of memory\n",
					progname);
				return GGI_ENOMEM;
			}
			remaps[num_remaps].from = from;
			remaps[num_remaps].to = to;
			num_remaps++;
			break;
		}
		case 'w':
			if (argv[i][2] != '\0') {
				wheel_up_button = strtol(argv[i]+2, 
							 NULL, 0);
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'w');
					return GGI_EARGREQ;
				}
				wheel_up_button = strtol(argv[i], NULL, 0);
			}
			break;
			
		case 'W':
			if (argv[i][2] != '\0') {
				wheel_down_button = strtol(argv[i]+2, 
							   NULL, 0);
			} else {
				if (++i >= count) {
					fprintf(stderr, missing_arg,
						progname, 'W');
					return GGI_EARGREQ;
				}
				wheel_down_button = strtol(argv[i], NULL, 0);
			}
			break;
			
		default:
			fprintf(stderr, "%s: unknown option: %s\n",
				progname, argv[i]+1);
			return GGI_EUNKNOWN;
		}
	}
	return 0;
}


static int
open_array(int num, int *fds, char **names)
{
	int i;

	for (i = 0; i < num; i++) {
		int tmpfd;
		if (strcmp(names[i], "-") == 0) {
			fds[i] = 1; /* stdout */
			continue;
		}
		/* This looks weird, but it seems to be the only way to
		   open a non-connected pipe for writing without blocking!
		   (Adding O_NONBLOCK to the second open() will make it fail
		   if the pipe is not connected) */
		tmpfd = open(names[i], O_RDONLY|O_CREAT|O_NONBLOCK, 00644);
		if (tmpfd < 0) {
			fprintf(stderr, "%s: unable to open file for reading:"
				" %s\n", progname, names[i]);
			return GGI_ENODEVICE;
		}
		fds[i] = open(names[i], O_WRONLY, 00644);
		close(tmpfd);
		if (fds[i] < 0) {
			fprintf(stderr, "%s: unable to open file for writing:"
				" %s\n", progname, names[i]);
			return GGI_ENODEVICE;
		}
		fcntl(fds[i], F_SETFL, O_NONBLOCK);
	}
	
	return 0;
}


static int
open_outputs(void)
{
	if (open_array(num_butout, but_fds, but_names) != 0 
	    || open_array(num_dataout, data_fds, data_names) != 0) {
		return -1;
	}

	return 0;
}


static int
output_intelli(int fd, uint32_t butstate, int idx, int idy, int wheel)
{
	uint8_t buf[4] = {0x40, 0x00, 0x00, 0x00};
	int8_t dx = idx;
	int8_t dy = idy;

	buf[0] |= ((butstate & 0x01) << 5) | ((butstate & 0x02) << 3);
	buf[3] |= ((butstate & 0x04) << 2) | ((butstate & 0x08) << 2);

	buf[0] |= ((dx >> 6) & 0x03) | ((dy >> 4) & 0x0c);
	buf[1] = dx & 0x3f;
	buf[2] = dy & 0x3f;
	buf[3] |= (wheel < 0) ? (wheel + 16) & 0x0f : wheel & 0x0f;

	if (write(fd, buf, 4) != 4) {
		return -1;
	}

	return 0;
}


static int
output_msc(int fd, uint32_t butstate, int dx, int dy)
{
	uint8_t buf[5] = {0x80, 0x00, 0x00, 0x00, 0x00};
	
	buf[0] |= 0x07 ^ ( ((butstate&1) << 2) |
			   ((butstate&2) >> 1) |
			   ((butstate&4) >> 1)  );
	
	buf[1] = dx/2;
	buf[2] = -dy/2;
	buf[3] =  dx - buf[1];
	buf[4] = -dy - buf[2];
	
	if (write(fd, buf, 5) != 5) {
		return -1;
	}
	
	return 0;
}


static int
output_ps2(int fd, uint32_t butstate, int dx, int dy)
{
	uint8_t buf[3] = {0x08, 0x00, 0x00};
	
	buf[0] |= (butstate & 0x07);
	
	if (dx < 0) {
		buf[0] |= 0x10;
		buf[1] = dx + 256;
	} else {
		buf[1] = dx;
	}
	if (dy > 0) {
		buf[0] |= 0x20;
		buf[2] = 256 - dy;
	} else {
		buf[2] = -dy;
	}

	if (write(fd, buf, 3) != 3) {
		return -1;
	}
	
	return 0;
}


static int
output_extended(int fd, uint32_t button, int nr, int press)
{
	uint8_t buf[2];

	buf[0] = button;
	if (press) buf[0] |= (1<<7);
	
	buf[1] = nr;

	if (write(fd, buf, 2) != 2) {
		return -1;
	}

	return 0;
}


static int
process_event(int press, uint32_t butstate, uint32_t button, int dx, int dy,
	      int dz, int wheel)
{
	int i;

	if (wheel < 0 && wheel_up_button > 0) {
		for (i = 0; i < num_butout; i++) {
			output_extended(but_fds[i], (uint32_t)wheel_up_button,
					-wheel, 0);
		}
	}
	if (wheel > 0 && wheel_down_button > 0) {
		for (i = 0; i < num_butout; i++) {
			output_extended(but_fds[i], (uint32_t)wheel_down_button,
					wheel, 0);
		}
	}
	if (button >= min_extended_button) {
		for (i = 0; i < num_butout; i++) {
			output_extended(but_fds[i], button, 0, press);
		}
	}
	for (i = 0; i < num_dataout; i++) {
		switch (data_prot[i]) {
		case PROT_PS2:
			output_ps2(data_fds[i], butstate, dx, dy);
			break;
		case PROT_MSC:
			output_msc(data_fds[i], butstate, dx, dy);
			break;
		case PROT_INTELLI:
			output_intelli(data_fds[i], butstate, dx, dy, wheel);
			break;
		default:
			break;
		}
	}

	return 0;
}


static inline uint32_t
do_remap(uint32_t button)
{
	int i;
	
	for (i = 0; i < num_remaps; i++) {
		if (remaps[i].from == button) {
			return remaps[i].to;
		}
	}
	
	return button;
}


static void 
sigpipe_handler(int dummy)
{
	signal(SIGPIPE, sigpipe_handler);
}

int
main(int argc, char *argv[])
{
	int ret;
	gii_event gev;

	progname = argv[0];

	if ((ret = parse_args(&argc, argv)) != 0) {
		exit(ret > 0 ? 0 : 1);
	}

	if (open_outputs() != 0) {
		exit(1);
	}

	if (giiInit() != 0) {
		fprintf(stderr, "%s: unable to init LibGII\n", progname);
		exit(1);
	}
	
	if ((inp = giiOpen(NULL)) == NULL) {
		fprintf(stderr, "%s: unable to open default GII input\n",
			progname);
		exit(1);
	}
	giiSetEventMask(inp, emPointer);
	
	if (!exit_on_sigpipe) signal(SIGPIPE, sigpipe_handler);

	while ((giiEventRead(inp, &gev, emAll))) {
		uint32_t button;
		switch (gev.any.type) {
		case evPtrRelative:
			if (debug) fprintf(stderr, 
				"Move\tX: %3d, Y: %3d, Z: %3d, Wheel: %3d\n",
			       gev.pmove.x, gev.pmove.y,
			       gev.pmove.z, gev.pmove.wheel);
			process_event(0, buttonstate, 0,
				      gev.pmove.x, gev.pmove.y,
				      gev.pmove.z, gev.pmove.wheel);
			break;
		case evPtrButtonPress:
			if (debug) fprintf(stderr, "\tPress: %u\n",
					   gev.pbutton.button);
			button = do_remap(gev.pbutton.button);
			buttonstate |= 1 << (button - 1);
			process_event(1, buttonstate, button,
				      0, 0, 0, 0);
			break;
		case evPtrButtonRelease:
			if (debug) fprintf(stderr, "\t\tRelease: %u\n",
					   gev.pbutton.button);
			button = do_remap(gev.pbutton.button);
			buttonstate &= ~(1 << (button - 1));
			process_event(0, buttonstate, button,
				      0, 0, 0, 0);
			break;
		default:
			if (debug) fprintf(stderr, "OTHER EVENT\n");
			break;
		}
	}

	giiClose(inp);
	giiExit();
	
	return 0;
}
