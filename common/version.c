#include "gen_version.h"
#include "version.h"
#include <stdio.h>

#define chipsVERSION "chipsln.0.0.0"

const char *version(void)
{
	return chipsVERSION;
}

char *version_and_exit(const void *unused)
{
	printf("%s\n"
	       "aka. %s\n"
	       "Built with: %s\n", VERSION, VERSION_NAME, BUILD_FEATURES);
	exit(0);
}
