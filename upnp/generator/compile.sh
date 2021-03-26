#! /bin/bash

#make clean && make && ./generator

function compile
{
	local FILE=$1
	echo -n "Compiling $FILE.c ... "
	gcc -Wall ${INCLUDES} -c $FILE.c
	echo "done!"
}

function movefile
{
	local FILE=$1
	DESTINATION=$2
	echo -n "Moving ${FILE} to $DESTINATION ... "
	mv ${FILE} ${DESTINATION}
	echo "done!"
}

FILES_API=(
UpnpActionComplete
UpnpActionRequest
UpnpDiscovery
UpnpEvent
UpnpEventSubscribe
UpnpHttpHeaders
UpnpFileInfo
UpnpStateVarComplete
UpnpStateVarRequest
UpnpSubscriptionRequest
)

FILES_OTHERS=(
GenlibClientSubscription
SSDPResultData
UpnpLib
)

FILES_TEST=(
TestClass
)

ALL_FILES=("${FILES_API[@]}" "${FILES_OTHERS[@]}" "${FILES_TEST[@]}")

echo
INCLUDES="-I."
INCLUDES="${INCLUDES} -I../.."
INCLUDES="${INCLUDES} -I../../upnp/src/inc"
INCLUDES="${INCLUDES} -I../../ixml/inc"
INCLUDES="${INCLUDES} -I../../upnp/inc"
INCLUDES="${INCLUDES} -I../../upnp/src/threadutil"
for FILE in "${ALL_FILES[@]}"; do
	compile $FILE
        clang-format -i --style=file $FILE.c
        clang-format -i --style=file $FILE.h
done

rm *.o

echo
for FILE in "${FILES_API[@]}"; do
	movefile $FILE.h ../inc
done

for FILE in "${FILES_API[@]}"; do
	movefile $FILE.c ../src/api
done

FILE=${FILES_OTHERS[0]}
movefile $FILE.h ../src/inc
movefile $FILE.c ../src/genlib/client_table

FILE=${FILES_OTHERS[1]}
movefile $FILE.h ../src/ssdp
movefile $FILE.c ../src/ssdp

FILE=${FILES_OTHERS[2]}
movefile $FILE.h ../src/inc
movefile $FILE.c ../src
