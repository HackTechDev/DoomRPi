#include "config.h"
#include <ggi/gg.h>
#include <string.h>


static void test_match(void * cfg, int n, char **names)
{
	struct gg_location_iter match;
	
	while(n--) {
		match.name = names[n];
		match.config = cfg;
		ggConfigIterLocation(&match);
		GG_ITER_FOREACH(&match) {
			printf("Found \"%s\" at \"%s\" with symbol \"%s\".\n",
			       match.name, match.location, match.symbol);
		}
		GG_ITER_DONE(&match);
	}
	
}

static void test_input(void * cfg, char * input)
{
	struct gg_target_iter match;

	printf("unrolling target \"%s\"...\n", input);
	
	match.config = cfg;
	match.input = input;
	ggConfigIterTarget(&match);
	GG_ITER_FOREACH(&match) {
		printf("Target \"%s\" with options \"%s\".\n",
		       match.target, match.options);
	}
	GG_ITER_DONE(&match);
	
	printf("done\n");

}

int main(int ac, char **av) {
	
	void * cfg = NULL;
	
	if (ac < 4) {
		printf("usage: %s <config file> -input <input name>\n",av[0]);
		printf("usage: %s <config file> -match <target names>\n",av[0]);
		exit(0);
	}
	ggInit();
	ggLoadConfig(av[1], &cfg);
	
	if(!strcmp(av[2], "-input")) {
		test_input(cfg, av[3]);
	}
	if(!strcmp(av[2], "-match")) {
		test_match(cfg, ac-3, av+3);
	}
	
	ggFreeConfig(cfg);
	ggExit();
	return 0;
}
