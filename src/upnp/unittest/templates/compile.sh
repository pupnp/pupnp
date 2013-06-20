#! /bin/bash

#
# The purpose of this file is to make it easier to find problems in
# Template Objects compilation.
#
# Because Template Objects are a result of preprocessing, the are normally
# invisible to the human eye. So, we paint him! Using the C pre-processor.
#
# The issue is that compilation error messages give all errors at the same
# lines. Using a combination of the pre-processor and indent, we are able
# to produce a good readable code, so that we can debug the templates.
#

if [[ "$1" == "" ]]; then
	echo "Error: no file name given, please enter the C file name."
	exit -1
fi

FILENAME=$(basename $1 .c)

top_srcdir=../../..

INCLUDES=
INCLUDES="${INCLUDES} -I${top_srcdir}"
INCLUDES="${INCLUDES} -I${top_srcdir}/ixml/inc"
INCLUDES="${INCLUDES} -I${top_srcdir}/threadutil/inc" \
INCLUDES="${INCLUDES} -I${top_srcdir}/upnp/inc" \
INCLUDES="${INCLUDES} -I${top_srcdir}/upnp/src/inc"

gcc ${INCLUDES} -E ${FILENAME}.c | grep -v ^# | indent -linux > ${FILENAME}.pp.c
gcc ${INCLUDES} -Wall -c ${FILENAME}.pp.c

