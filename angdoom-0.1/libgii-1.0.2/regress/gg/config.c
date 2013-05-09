#include "config.h"
#include <ggi/gg.h>
#include <string.h>

#include "../testsuite.inc.c"


static const char *configfile = "./test.conf";


static void testcase1(const char *desc)
{
	void *cfg = NULL;
	int err;

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	err = ggLoadConfig(configfile, &cfg);
	if (err) {
		printfailure("expected return value: 0\n"
			"actual return value: %i\n", err);
		return;
	}

	ggFreeConfig(cfg);

	printsuccess();
	return;
}


static void testcase2(const char *desc)
{
	void *cfg = NULL;
	int n, err;
	struct gg_target_iter match;

	const char *input = "fake0";
	const char *expected_names[] = {
		"real1",
		"real3",
		"real0",
		NULL
	};
	const char *expected_options[] = {
		"-arg0",
		"-arg2:-arg1",
		"",
		NULL
	};

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;


	err = ggLoadConfig(configfile, &cfg);

	match.input = input;
	match.config = cfg;
	ggConfigIterTarget(&match);
	n = 0;
	err = 0;
	GG_ITER_FOREACH(&match) {
		if (strcmp(match.target, expected_names[n]) != 0) {
			printfailure("expected target value: '%s'\n"
				"actual target value: '%s'\n",
				expected_names[n], match.target);
			err = 1;
			break;
		}

		if (strcmp(match.options, expected_options[n]) != 0) {
			printfailure("expected option value: '%s'\n"
				"actual option value: '%s'\n",
				expected_options[n], match.options);
			err = 1;
			break;
		}
		n++;
	}
	GG_ITER_DONE(&match);

	ggFreeConfig(cfg);

	if (err) return;


	printsuccess();
	return;
}

static void testcase3(const char *desc)
{
	void *cfg = NULL;
	int n, err;
	struct gg_location_iter match;

	const char *names[] = {
		"real1",
		"real3",
		NULL
	};
	const char *expected_module[] = {
		"bar.module",
		"foo.module",
		NULL
	};

	const char *expected_symbol[] = {
		"",
		"Xsymbol",
		NULL
	};


	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;

	err = ggLoadConfig(configfile, &cfg);

	n = 0;
	err = 0;
	while(names[n] != NULL) {
		match.name = names[n];
		match.config = cfg;
		ggConfigIterLocation(&match);
		GG_ITER_FOREACH(&match) {
			if (strcmp(match.location, expected_module[n]) != 0) {
				printfailure("expected location value: '%s'\n"
					"actual location value: '%s'\n",
					expected_module[n], match.location);
				err = 1;
				break;
			}
			if (strcmp(match.symbol, expected_symbol[n]) != 0) {
				printfailure("expected symbol value: '%s'\n"
					"actual symbol value: '%s'\n",
					expected_symbol[n], match.symbol);
				err = 1;
				break;
			}
		}
		GG_ITER_DONE(&match);
		n++;

		if (err) break;
	}

	ggFreeConfig(cfg);

	if (err) return;

	printsuccess();
	return;
}

int main(int argc, char * const argv[])
{	
	parseopts(argc, argv);

	printdesc("Regression testsuite for libgg config parser\n\n");


	ggInit();

	testcase1("Test ggLoadConfig()/ggFreeConfig()");
	testcase2("Test ggConfigIterTarget()");
	testcase3("Test ggConfigIterLocation()");

	ggExit();

	printsummary();

	return 0;
}
