/* $Id: input.c,v 1.20 2005/08/05 16:13:47 cegger Exp $
******************************************************************************

   Mouse: input

   Copyright (C) 1998 Andrew Apted	[andrew@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]

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
#include "config.h"

#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>	/* for snprintf() */
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <ctype.h>
#include <errno.h>

#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_LINUX_KDEV_T_H
#ifdef HAVE_LINUX_MAJOR_H
#define HAVE_LINUX_DEVICE_CHECK
#include <linux/kdev_t.h>   /* only needed for MAJOR() macro */
#include <linux/major.h>    /* only needed for MISC_MAJOR */
#endif
#endif

#define MAX_OPTLEN	255
#define MAX_MOUSE_NAMES		8
#define MAX_LINELEN     2048


typedef struct {
	/* Synonyms */
	const char *names[MAX_MOUSE_NAMES];

	/* The protocol name to pass to input-mouse */
	const char *name;
	
	/* Serial parameters.  If the mouse is not a serial mouse
	 * (for example, a busmouse), then default_baud should be < 0.
	 */
	int default_baud, cflag;
} MouseType;

#define C_NORM	(CREAD | CLOCAL | HUPCL)

typedef struct {
	int fd;
	int have_old_termios;
	struct termios old_termios;
	MouseType *type;
	int readonly;
} l_mouse_priv;

#define L_MOUSE_PRIV(inp)  ((l_mouse_priv *) inp->priv)


/* ---------------------------------------------------------------------- */


/* The Holy Table of Mouse Types
*/

static MouseType mice_types[] =
{
    {{ "Microsoft", "ms", NULL }, "ms", B1200, C_NORM|CS7 },
    {{ "ms3", "IntelliMouse", "mman+", NULL }, "ms3", B1200, C_NORM|CS7 },
    {{ "MouseSystems", "msc", NULL }, "msc", B1200, C_NORM|CS8|CSTOPB },
    {{ "Logitech", "logi", NULL }, "logi", B1200, C_NORM|CS8|CSTOPB },
    {{ "MMSeries", "mm", NULL }, "logi", B1200, C_NORM|CS8|PARENB|PARODD },
    {{ "Sun", NULL }, "sun", B1200, C_NORM|CS8|CSTOPB },
    {{ "MouseMan", "mman", NULL }, "mman", B1200, C_NORM|CS7 },
    {{ "BusMouse", "bm", NULL }, "sun", -1, 0 },
    {{ "PS/2", "ps2", NULL }, "ps2", -1, 0 },
    {{ "mmanps2", "MouseManPlusPS/2", NULL }, "mmanps2", -1, 0 },
    {{ "imps2", "IMPS/2", NULL }, "imps2", -1, 0 },
    {{ "lnxusb", "LinuxUSB", NULL }, "lnxusb", -1, 0 },

    /* Terminator */
    {{ NULL }, NULL, -1, 0 }
};


/* ---------------------------------------------------------------------- */


static int find_mouse(char *name)
{
	int m, n;

	for (m=0; mice_types[m].name != NULL; m++) {
		MouseType *mtype = mice_types+m;
		for (n=0; (n < MAX_MOUSE_NAMES) && 
			     (mtype->names[n] != NULL); n++) {
			if (strcasecmp(mtype->names[n], name) == 0) {
				return m;  /* found it */
			}
		}
	}
	fprintf(stderr, "Unknown mouse type '%s'\n", name);

	return GGI_ENOTFOUND;
}

#if 0
static int find_baud(char *baudname)
{
	switch (atoi(baudname))
	{
		case 9600: return B9600;
		case 4800: return B4800;
		case 2400: return B2400;
		case 1200: return B1200;
	}

	fprintf(stderr, "Baud rate '%s' not supported\n", baudname);
	
	return B1200;  /* !!! */
}
#endif

static const char *serialfailstr =
"Warning: failed to set serial parameters for mouse device.\n"
"         Your mouse may not work as expected.\n";


static int do_mouse_open(gii_input *inp, char *filename,
			 int dtr, int rts, int baud)
{
	l_mouse_priv *mhook = L_MOUSE_PRIV(inp);
	int ispipe = 0;

	mhook->readonly = 0;
	mhook->fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if (mhook->fd < 0) {
		mhook->readonly = 1;
		mhook->fd = open(filename, O_RDONLY | O_NOCTTY | O_NONBLOCK);
	}

	if (mhook->fd < 0) {
		DPRINT_MISC("linux_mouse: Failed to open '%s'.\n",
			       filename);
		return GGI_ENODEVICE;
	}

	DPRINT_MISC("linux-mouse: Opened mouse file '%s' %s.\n",
		       filename, mhook->readonly ? "ReadOnly" : "Read/Write");

#ifdef HAVE_LINUX_DEVICE_CHECK
	do {
		struct stat m_stat;
			
		if (fstat(mhook->fd, &m_stat) == 0 &&
		    S_ISFIFO(m_stat.st_mode)) {
			ispipe = 1;
		}
	} while (0);
#endif

	if (!ispipe && mhook->type->default_baud >= 0) {
		int dowarn = 0;

		/* Set up the termios state and baud rate */
		tcflush(mhook->fd, TCIOFLUSH);
		if (tcgetattr(mhook->fd, &mhook->old_termios) == 0) {
			struct termios tio = mhook->old_termios;

			if (baud < 0) {
				baud = mhook->type->default_baud;
			}

			tio.c_cflag = mhook->type->cflag | baud;
			tio.c_iflag = IGNBRK;
			tio.c_oflag = 0;
			tio.c_lflag = 0;
			tio.c_cc[VMIN]  = 1;
			tio.c_cc[VTIME] = 0;

			if (tcsetattr(mhook->fd, TCSANOW, &tio) == 0) {
				mhook->have_old_termios = 1;
			} else {
				dowarn = 1;
			}
		} else {
			dowarn = 1;
		}

		/* Set up RTS and DTR modem lines */
		if ((dtr >= 0) || (rts >= 0)) {
#ifdef HAVE_TIOCMSET
			unsigned int modem_lines;

			if (ioctl(mhook->fd, TIOCMGET, &modem_lines) == 0) {
				if (dtr == 0) modem_lines &= ~TIOCM_DTR;
				if (rts == 0) modem_lines &= ~TIOCM_RTS;

				if (dtr > 0) modem_lines |= TIOCM_DTR;
				if (rts > 0) modem_lines |= TIOCM_RTS;
				
				if (ioctl(mhook->fd, TIOCMSET, &modem_lines)
				    != 0) {
					dowarn = 1;
				}
			} else {
				dowarn = 1;
			}
#else /* HAVE_TIOCMSET */
			fprintf(stderr,
				"input-linux-mouse: warning, this system does"
				" not support TIOCMSET\n"
				"        device may not work as expected\n");
#endif /* HAVE_TIOCMSET */
		}
		if (dowarn) fprintf(stderr, serialfailstr);
	}

	return 0;
}

/* !!! All this parsing stuff is probably best done with the
 * ggParseOption() code, with things like "-file=/dev/mouse",
 * "-type=microsoft", "-baud=9600", and that sort of thing...
 */
 
static const char *parse_field(char *dst, int max, const char *src)
{
	int len = 1;   /* includes trailing NUL */

	for (; *src != '\0' && (*src != ','); src++) {

		if (len < max) {
			*dst++ = *src;
			len++;
		}
	}
	*dst = '\0';
	if (*src == ',') src++;

	return src;
}


static int get_from_file(const char *fname, char *protname, char *mdev)
{
	FILE *fp;
	const char *options[] = { "mouse", "mdev", NULL };
	char *optres[2];
	
	*protname = '\0';
	if ((fp = fopen(fname, "r")) == NULL) {
		return GGI_ENOFILE;
	}
	optres[0] = protname;
	optres[1] = mdev;
	while (ggGetFileOpt(fp, options, optres, MAX_OPTLEN) >= 0) continue;
	
	fclose(fp);

	return (*protname=='\0');
}


static void libvga_to_options(char *str, char *options)
{
	char buf[MAX_OPTLEN];
	int len;

	options[0] = '\0';

	/* skip protocol name */
	while (*str != '\0' && ! isspace((uint8_t) *str)) {
		str++;
	}
	if (*str == '\0') return;

	/* Put a terminator after the protocol name */
	*str = '\0';
	str++;

	/* handle the libvga options */
	while (sscanf(str, " %s%n", buf, &len) == 1) {
		
		if (strcasecmp(buf, "SetRTS") == 0) {
			strcat(options, "r1");
		} else
		if (strcasecmp(buf, "ClearRTS") == 0) {
			strcat(options, "r0");
		} else
		if (strcasecmp(buf, "LeaveRTS") == 0) {
			/* nothing to do */
		} else
		if (strcasecmp(buf, "SetDTR") == 0) {
			strcat(options, "d1");
		} else
		if (strcasecmp(buf, "ClearDTR") == 0) {
			strcat(options, "d0");
		} else
		if (strcasecmp(buf, "LeaveDTR") == 0) {
			/* nothing to do */
		} else {
			fprintf(stderr, "linux-mouse: Unknown libvga "
				"mouse option `%s'.\n", buf);
		}

		str += len;
	}
}

#define COMMENT_CHAR '#'

#define SKIPWHITE(str)  {while (isspace((uint8_t)*(str)) && *(str) != '\0' \
				&& *(str) != COMMENT_CHAR) { (str)++; }}

static char *get_value_from_XF86Config(char *str) 
{
	char *hlp;
	SKIPWHITE(str);
	if (*str=='"') {
		hlp=++str;
		while(*hlp!='"' &&*hlp!='"'&&*hlp!='"') {
			hlp++;
		}
		if (*hlp=='"') *hlp=0;
		return str;
	} else {
		hlp=str;
		while (!isspace((uint8_t)*hlp) && *hlp != '\0' && *hlp != COMMENT_CHAR) 
			hlp++;
		if (isspace((uint8_t)*hlp)) *hlp=0;
		return str;
	}
}

static int get_from_XF86Config(const char *filename,
				char *_devname,char *protname, char *options)
{
	FILE *file;
	char line[MAX_LINELEN];
	int InPointerSection=0;
	int GotKonfig=0;
	int OptionCount=0;
	
	if (NULL==(file=fopen(filename,"r"))) return 1;
	while(fgets(line, MAX_LINELEN, file) != NULL) {
		char *lineptr=line;
		SKIPWHITE(lineptr);
		if (! InPointerSection) {
			if (0==strncasecmp(lineptr,"Section",7)) {
				lineptr+=7;
				SKIPWHITE(lineptr);
				if (0==strncasecmp(lineptr,"\"Pointer\"",9)) {
					InPointerSection=1;
				}
			}
		} else {
			if (0==strncasecmp(lineptr,"EndSection",10)) {
				break;
			} else if (0==strncasecmp(lineptr,"Protocol",8)) {
				lineptr+=8;
				ggstrlcpy(protname,get_value_from_XF86Config(lineptr),MAX_OPTLEN);
				GotKonfig=1;
			} else if (0==strncasecmp(lineptr,"Device",6)) {
				lineptr+=6;
				ggstrlcpy(_devname,get_value_from_XF86Config(lineptr),MAX_OPTLEN);
			} else if (0==strncasecmp(lineptr,"BaudRate",8)) {
				lineptr+=6;
				strcat(options, "b");
				ggstrlcat(options,get_value_from_XF86Config(lineptr),10);
				OptionCount++;
			} else if (0==strncasecmp(lineptr,"ClearRTS",8)) {
				strcat(options, "r0");
				OptionCount++;
			} else if (0==strncasecmp(lineptr,"ClearDTR",8)) {
				strcat(options, "d0");
				OptionCount++;
			} 
			if (OptionCount>3) {
				fprintf(stderr,"linux-mouse: More than 3 mouse options in XF86Config.\n"
						"Parsing of bogus file aborted.\n");
				GotKonfig=0;
				break;
			}
		}
	}
	fclose(file);
	return ! GotKonfig;
}


static void parse_mouse_specifier(const char *spec, char *protname,
				  char *_devname, char *options)
{
	*protname = *_devname = *options = '\0';

	/* LISP-haters should shut their eyes now :) */
	if (spec) {
		parse_field(options, MAX_OPTLEN,
			    parse_field(_devname, MAX_OPTLEN,
					parse_field(protname, MAX_OPTLEN,
						    spec)));
	}

	/* supply defaults for missing bits */
	if (*_devname == '\0') {
		strcpy(_devname, "/dev/mouse");
	}

	if (*protname == '\0' || strcmp(protname, "auto") == 0) {
		/* Protocol hasn't been specified. First try to read from
		   config file, then try autodetecting.
		*/

		/* Make sure we fail if nothing is found */
		*protname = '\0';

		do {
			const char *dirname;
			char fname[2048];
			char appendstr[] = "/input/linux-mouse";

			dirname = ggGetUserDir();
			if (strlen(dirname) + sizeof(appendstr) < 2048) {
				sprintf(fname, "%s%s", dirname, appendstr);
				if (get_from_file(fname, protname, _devname)
				    == 0) {
					return;
				}
			}

			dirname = giiGetConfDir();
			if (strlen(dirname) + sizeof(appendstr) < 2048) {
				sprintf(fname, "%s%s", dirname, appendstr);
				if (get_from_file(fname, protname, _devname)
				    == 0) {
					return;
				}
			}
		} while (0);

		if (strncmp(_devname, "/dev/gpm", 8) == 0) {
			strcpy(protname, "msc");
			return;
		}
#ifdef HAVE_LINUX_DEVICE_CHECK
		do {
			struct stat m_stat;

			if ((stat(_devname, &m_stat) == 0) &&
			    S_ISCHR(m_stat.st_mode) &&
			    (MAJOR(m_stat.st_rdev) == MISC_MAJOR)) {
				switch (MINOR(m_stat.st_rdev)) {
				case 1:
					strcpy(protname, "ps2");
					return;
				default:
					strcpy(protname, "bm");
					return;
				}
			}
		} while (0);
#endif
#ifdef HAVE_READLINK
		do {
			char buf[1024];
			int len;
			
			len = readlink(_devname, buf, 1024);
			if (len > 0 && len < 1024) {
				buf[len] = '\0';
				if (strstr(buf, "gpm") != NULL) {
					strcpy(protname, "msc");
					return;
				}
			}
		} while (0);
#endif

		/* Try to parse XF86Config */
		if (get_from_XF86Config("/etc/X11/XF86Config",
					_devname,protname,options)==0) return;
		if (get_from_XF86Config("/etc/XF86Config",
					_devname,protname,options)==0) return;
		
		/* Try to parse SVGAlib config file as a last resort */
		if (get_from_file("/etc/vga/libvga.config",
				  protname, _devname) == 0) {
			if (strlen(options) == 0) {
				libvga_to_options(protname, options);
			}
			return;
		}

	}
}

static char *parse_opt_int(char *opt, int *val)
{
	*val = 0;

	for (; *opt != '\0' && isdigit((uint8_t)*opt); opt++) {
		*val = ((*val) * 10) + ((*opt) - '0');
	}

	return opt;
}

static void parse_options(char *opt, int *baud, int *dtr, int *rts)
{
	while (*opt != '\0') switch (*opt++) {

		case 'b': case 'B':    /* baud */
			opt = parse_opt_int(opt, baud);
			break;

		case 'd': case 'D':    /* dtr */
			opt = parse_opt_int(opt, dtr);
			break;

		case 'r': case 'R':    /* rts */
			opt = parse_opt_int(opt, rts);
			break;

		default:
			fprintf(stderr, "Unknown mouse option "
				"'%c' -- rest ignored.\n", *opt);
			return;
	}
}


/* ---------------------------------------------------------------------- */


static int GII_mouse_close(gii_input *inp)
{
	l_mouse_priv *mhook = L_MOUSE_PRIV(inp);

	DPRINT_MISC("linux_mouse cleanup\n");
	
	if (mhook->have_old_termios) {
		if (tcsetattr(mhook->fd, TCSANOW, &mhook->old_termios) < 0) {
			perror("Error restoring serial parameters");
		}
	}

	close(mhook->fd);
	free(mhook);

	DPRINT_MISC("linux_mouse: exit OK.\n");

	return 0;
}


EXPORTFUNC int GIIdl_linux_mouse(gii_input *inp, const char *args, void *argptr);

int GIIdl_linux_mouse(gii_input *inp, const char *args, void *argptr)
{
	l_mouse_priv *mhook;
	char protname[MAX_OPTLEN+1];
	char _devname[MAX_OPTLEN+1];
	char options[MAX_OPTLEN+1];
	char argstring[MAX_OPTLEN+1+128];
	int mindex, ret;
	int dtr=-1, rts=-1, baud=-1;
	gii_input *mouseinp;
	const char *spec = "";
	
	DPRINT_MISC("linux_mouse starting.(args=\"%s\",argptr=%p)\n",
		    args, argptr);

	/* Initialize */
	if (args && *args) {
		spec = args;
	}

	/* parse the mouse specifier */
	parse_mouse_specifier(spec, protname, _devname, options);
	parse_options(options, &baud, &dtr, &rts);
	DPRINT_MISC("linux_mouse: prot=`%s' dev=`%s' opts=`%s'\n",
		    protname, _devname, options);

	if (*protname == '\0' || (mindex = find_mouse(protname)) < 0) {
		return GGI_EARGINVAL;
	}

	/* allocate mouse private structure */
	if ((mhook = inp->priv = malloc(sizeof(l_mouse_priv))) == NULL) {
		return GGI_ENOMEM;
	}

	mhook->have_old_termios = 0;
 	/* Open mouse */
	mhook->type = mice_types + mindex;

	if ((ret = do_mouse_open(inp, _devname, dtr, rts, baud)) < 0) {
		free(mhook);
		return ret;
	}

	inp->GIIseteventmask   = NULL;
	inp->GIIgeteventmask   = NULL;
	inp->GIIgetselectfdset = NULL;
	inp->GIIclose = GII_mouse_close;

	inp->targetcan = 0;
	inp->curreventmask = 0;

	inp->maxfd = 0;

	/* protname length is (guaranteed to have been) checked above */
	snprintf(argstring, MAX_OPTLEN+1+128,
		"input-mouse:%d,%s", mhook->fd, mhook->type->name);

	if ((mouseinp = giiOpen(argstring, NULL)) == NULL) {
		GII_mouse_close(inp);
		return GGI_ENODEVICE;
	}

	inp = giiJoinInputs(inp, mouseinp);

	DPRINT_MISC("linux_mouse fully up\n");

	return 0;
}
