

#include "autoconfig.h"


#include "ixmldebug.h"


#include <stdarg.h>
#include <stdio.h>


/*!
 * \file
 */


#ifdef DEBUG
void IxmlPrintf(
	const char *FmtStr,
	... )
{
	va_list ArgList;
	
	va_start(ArgList, FmtStr);
	vfprintf(stdout, FmtStr, ArgList);
	fflush(stdout);
	va_end(ArgList);
}
#endif

