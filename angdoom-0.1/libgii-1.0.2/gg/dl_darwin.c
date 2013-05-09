/* $Id: dl_darwin.c,v 1.8 2005/07/21 06:54:56 cegger Exp $
***************************************************************************

   LibGG - Module loading code for Darwin

   Copyright (C) 2002  Christoph Egger   [Christoph_Egger@t-online.de]

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

***************************************************************************
*/

#include <stdlib.h>
#include <string.h>

#include <ggi/gg.h>
#include "plat.h"


struct gg_dlhand_darwin_t {
	NSObjectFileImage objectFileImage;
	NSModule nsmodule;
	int nsmodule_flags;
};

/* Error Handling
 */
static NSObjectFileImageReturnCode dlerror_code = NSObjectFileImageSuccess;
static char dlerror_filename[PATH_MAX];

/* The first two functions allow the programmer to direct the
 * link edit processing of  undefined  symbols  and  multiply
 * defined symbols.  The third function allows the programmer
 * to catch all other link editor errors.
 */
static void ggDarwinErrorUndefined(const char *symbolName)
{
	/* load a module, where symbolName is defined or
	 * let the application fail here completely with
	 * an appropriate error message here!
	 */

	/* leave it as a stub right now */
	return;
}	/* ggDarwinErrorUndefined */

static NSModule ggDarwinErrorMultiple(NSSymbol s, NSModule oldModule, NSModule newModule)
{
	/* get the version of the GGI lib, that
	 * called ggLoadConfig (or the one in libggi),
	 * select and return the right module.
	 */

	/* leave it as a stub right now */
	return newModule;
}	/* ggDarwinErrorMultiple */

static void ggDarwinErrorLinkEdit(NSLinkEditErrors errorClass, int errorNumber,
			const char *fileName, const char *errorString)
{
	/* Handle as many errorNumber's as possible and fail
	 * completely with an error message on non-handled
	 * errorNumber's.
	 */

	/* leave it as a stub right now */
	return;
}	/* ggDarwinErrorLinkEdit */


static void splitstring(const char *filename,
			const char **pathname, const char **modulename)
{
	const char *tmp;

	tmp = (const char *)strrchr(filename, '/');
	if (tmp == NULL) {
		pathname[0] = "./";
		modulename[0] = filename;
	} else {
		pathname[0] = filename;
		modulename[0] = &tmp[1];
	}	/* if */

	return;
}	/* splitstring */



/* ggDarwinDLOpen implements a "dlopen" wrapper
 */
gg_dlhand ggDarwinDLOpen(const char *filename, int flags)
{
	gg_dlhand ret = NULL;
	struct gg_dlhand_darwin_t *darwin_ret = NULL;
	NSLinkEditErrorHandlers error_handlers;
	const char *pathname, *modulename;

	darwin_ret = (struct gg_dlhand_darwin_t *)malloc(sizeof(struct gg_dlhand_darwin_t));
	if (darwin_ret == NULL) return ret;

	ret = (void *)darwin_ret;

	if (flags & GG_MODULE_GLOBAL) {
		darwin_ret->nsmodule_flags = NSLINKMODULE_OPTION_NONE;
	} else {
		darwin_ret->nsmodule_flags = NSLINKMODULE_OPTION_PRIVATE;
	}	/* if */

	splitstring(filename, &pathname, &modulename);

	dlerror_code = NSCreateObjectFileImageFromFile(pathname, &darwin_ret->objectFileImage);
	ggstrlcpy(dlerror_filename, filename, sizeof(dlerror_filename));

	switch (dlerror_code) {
	case NSObjectFileImageSuccess:
		break;

	case NSObjectFileImageFailure:
	case NSObjectFileImageInappropriateFile:
	case NSObjectFileImageArch:
	case NSObjectFileImageFormat:
	case NSObjectFileImageAccess:
		goto err0;
	}	/* switch */

	/* Install our own error handlers */
	error_handlers.undefined = ggDarwinErrorUndefined;
	error_handlers.multiple = ggDarwinErrorMultiple;
	error_handlers.linkEdit = ggDarwinErrorLinkEdit;

#if 0
	/* Let the default handlers do their work
	 * as long as our own error handlers are stubs.
	 */
	NSInstallLinkEditErrorHandlers(&error_handlers);
#endif

	/* try to load the module now */
	darwin_ret->nsmodule = NSLinkModule(darwin_ret->objectFileImage,
					modulename, darwin_ret->nsmodule_flags);

	/* Either we return successful here or the error handlers
	 * already aborted/exited the app before.
	 */
	return ret;

err0:
	free(ret);
	return NULL;
}	/* ggDarwinDLOpen */


/* ggDarwinDLSym implements a "dlsym" wrapper
 */
void *ggDarwinDLSym(gg_dlhand handle, const char *symbol)
{
	void *nsaddr = NULL;
	NSSymbol nssymbol = 0;
	struct gg_dlhand_darwin_t *darwin_module;

	darwin_module = (struct gg_dlhand_darwin_t *)handle;

	switch (darwin_module->nsmodule_flags) {
	case NSLINKMODULE_OPTION_NONE:
		nssymbol = NSLookupAndBindSymbol(symbol);
		break;

	case NSLINKMODULE_OPTION_PRIVATE:
		nssymbol = NSLookupSymbolInModule(darwin_module->nsmodule,
				symbol);
		break;

	}	/* switch */

	nsaddr = NSAddressOfSymbol(nssymbol);

	/* no error handling needed here. The error handlers
	 * are called, when an error occurs.
	 */

	return nsaddr;
}	/* ggDarwinDLSym */


/* ggDarwinDLCose implements a "dlclose() wrapper
 */
void ggDarwinDLClose(gg_dlhand handle)
{
	struct gg_dlhand_darwin_t *darwin_module;

	darwin_module = (struct gg_dlhand_darwin_t *)handle;

	NSUnLinkModule(darwin_module->nsmodule, NSUNLINKMODULE_OPTION_NONE);

	/* no error checking needed here, because
	 * the error handlers are called, if necessary
	 */

#ifdef HAVE_NSDESTROYOBJECTFILEIMAGE
	/* Darwin < 6.1 doesn't have this function implemented */
	NSDestroyObjectFileImage(darwin_module->objectFileImage);
#endif

	free(darwin_module);
	return;
}	/* ggDarwinDLClose */



static char error_str[PATH_MAX];

/* ggDarwinDLError implements a "dlerror()" wrapper
 */
const char *ggDarwinDLError(void)
{
	memset(error_str, 0, sizeof(char) * PATH_MAX);

	switch (dlerror_code) {
	case NSObjectFileImageSuccess:
		snprintf(error_str, PATH_MAX,
			"%s: Indicates the API was successful and it returned a "
			"valid NSObjectFileImage for the host machine's cpu "
			"architecture.", dlerror_filename);
		break;

	case NSObjectFileImageFailure:
		snprintf(error_str, PATH_MAX, "%s: No NSObjectFileImage was returned.",
			dlerror_filename);
		break;

	case NSObjectFileImageInappropriateFile:
		snprintf(error_str, PATH_MAX, "%s: No appropriate type of object file.",
			dlerror_filename);
		break;

	case NSObjectFileImageArch:
		snprintf(error_str, PATH_MAX, "%s: The host machine's "
			"cpu architecture could not be found in the file.",
			dlerror_filename);
		break;

	case NSObjectFileImageFormat:
		snprintf(error_str, PATH_MAX, "%s: Mach-O format is malformed.",
			dlerror_filename);
		break;

	case NSObjectFileImageAccess:
		snprintf(error_str, PATH_MAX, "%s: File could not be accessed.",
			dlerror_filename);
		break;

	}	/* switch */

	return error_str;
}	/* ggDarwinDLError */

