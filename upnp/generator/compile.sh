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
UpnpExtraHeaders
UpnpFileInfo
UpnpStateVarComplete
UpnpStateVarRequest
UpnpSubscriptionRequest
)

FILES_OTHERS=(
GenlibClientSubscription
SSDPResultData
)

FILES_TEST=(
TestClass
)

ALL_FILES=("${FILES_API[@]}" "${FILES_OTHERS[@]}" "${FILES_TEST[@]}")

echo
INCLUDES="-I. -I../.. -I../../upnp/src/inc -I../../ixml/inc -I../../upnp/inc/"
for FILE in "${ALL_FILES[@]}"; do
	compile $FILE
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

