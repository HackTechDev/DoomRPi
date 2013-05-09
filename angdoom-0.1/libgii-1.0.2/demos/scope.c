#include "config.h"
#include <ggi/gg.h>

int main(int ac, char **av) {
	gg_scope scope;
	void * addr;
	int i;

	if (ac < 2) {
		printf("Usage: %s <location> [symbol] [symbol] ...\n", av[0]);
		return 0;
	}
	if (ggInit() < 0) {
		printf("ggInit failed.\n");
		return 0;
	}
	
	scope = ggGetScope(av[1]);
	if (scope == NULL) {
		printf("ggGetScope(\"%s\") failed.\n", av[1]);
		ggExit();
		return 0;
	}
	
	for(i=2;i<ac;i++) {
		addr = ggFromScope(scope, av[i]);
		printf("%s : %p\n", av[i], addr);
	}
	
	ggDelScope(scope);
	ggExit();
	return 0;
}
