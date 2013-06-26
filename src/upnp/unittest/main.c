
#include <stdio.h>

#include "templates/templates.h"

int (*tests[])(void) = {
	templates,
};

int main(void)
{
	int ret = 0;
	int i;
	int n = (sizeof tests) / (sizeof tests[0]);
	int result;
	
	for (i = 0; i < n; ++i) {
		result = tests[i]();
		printf("Test number %3d: %s.\n", i + 1, !result ? "PASS" : "FAIL");
		ret += result;
	}
	printf("\n"
		"Failed in %d tests.\n"
		"Passed in %d tests.\n",
		ret, n - ret);

	return ret;
}

