/* Shared helpers for the GHSA-xr5m-v53v-jfq9 regression tests.
 *
 * The bug is a complexity bug: the documents parse correctly, only too
 * slowly. So the tests compare the parse cost of an adversarial document
 * against a control document with the same number of nodes, where the
 * quadratic walk cannot grow. Absolute times vary with the machine and the
 * build type (Debug, sanitizers); the ratio between the two does not. */

#ifndef POC_GHSA_XR5M_H
#define POC_GHSA_XR5M_H

#include "ixml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* A linear parser keeps the ratio near 1; the quadratic one reaches 50 and
 * more at the node counts used here. */
#define XR5M_MAX_RATIO 8.0

/* Repeat each measurement until at least this much CPU time has elapsed, so
 * that a coarse clock() (15 ms ticks on Windows) does not dominate. */
#define XR5M_MIN_CLOCKS (CLOCKS_PER_SEC / 10)

/* Append n copies of s at *p. */
static char *xr5m_repeat(char *p, const char *s, int n)
{
	size_t len = strlen(s);
	int i;

	for (i = 0; i < n; i++) {
		memcpy(p, s, len);
		p += len;
	}
	return p;
}

/* Parse buf repeatedly and return the average cost of one parse, in clock()
 * ticks. Returns a negative value if the document does not parse. */
static double xr5m_parse_cost(const char *buf)
{
	clock_t start = clock();
	clock_t elapsed;
	int runs = 0;

	do {
		IXML_Document *doc = NULL;
		int rc = ixmlParseBufferEx(buf, &doc);
		if (rc != IXML_SUCCESS || !doc) {
			fprintf(stderr, "parse failed: rc=%d\n", rc);
			return -1.0;
		}
		ixmlDocument_free(doc);
		runs++;
		elapsed = clock() - start;
	} while (elapsed < XR5M_MIN_CLOCKS);

	return (double)elapsed / runs;
}

/* Returns 0 if parsing test costs no more than XR5M_MAX_RATIO times parsing
 * control, 1 otherwise. */
static int xr5m_check_ratio(
	const char *name, const char *test, const char *control)
{
	double t = xr5m_parse_cost(test);
	double c = xr5m_parse_cost(control);
	double ratio;

	if (t < 0 || c <= 0)
		return 1;
	ratio = t / c;
	printf("%s: %.2f ms vs control %.2f ms, ratio %.1f (max %.1f)\n",
		name,
		t * 1000.0 / CLOCKS_PER_SEC,
		c * 1000.0 / CLOCKS_PER_SEC,
		ratio,
		XR5M_MAX_RATIO);
	return ratio > XR5M_MAX_RATIO;
}

#endif /* POC_GHSA_XR5M_H */
