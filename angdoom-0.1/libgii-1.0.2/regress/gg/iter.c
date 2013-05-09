#include "config.h"
#include <ggi/gg.h>

#include "../testsuite.inc.c"


struct my_iter {
	struct gg_iter iter;
	int left;
};

static int myIterNext(struct my_iter * i) {
	if(i->left--)
		return ITER_YIELD;
	return ITER_DONE;
}

static void myIterInit(struct my_iter *i, int from) {
	GG_ITER_PREPARE(i, myIterNext, NULL);
	i->left = from;
}


static void testcase1(const char *desc)
{
	struct my_iter i;
	int n;
	int err = 0;

	printteststart(__FILE__, __PRETTY_FUNCTION__, EXPECTED2PASS, desc);
	if (dontrun) return;
	
	myIterInit(&i, 20);
	n = 20;
	GG_ITER_FOREACH(&i) {
		n--;
		if (n != i.left) {
			printfailure("expected index value: %i\n"
				"actual index value: %i\n",
				n, i.left);
			err = 1;
			break;
		}
	}
	GG_ITER_DONE(&i);

	if (err) return;

	printsuccess();
	return;
}


int main(int argc, char * const argv[])
{
	parseopts(argc, argv);

	printdesc("Regression testsuite for libgg iterator\n\n");

	testcase1("Countdown");

	printsummary();

	return 0;
}
