#include "version.h"
#include <stdio.h>

/* Only common/version.c can safely include this.  */
# include "version_gen.h"

#define chipsVERSION "chipsln.0.0.0"

const char *version(void)
{
	return chipsVERSION;
}

char *version_and_exit(const void *unused UNUSED)
{
	printf("%s\n", VERSION);
	if (BUILD_FEATURES[0]) {
		printf("Built with features: %s\n", BUILD_FEATURES);
	}
	exit(0);
}