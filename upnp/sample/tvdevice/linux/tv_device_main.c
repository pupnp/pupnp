/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer. 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution. 
 * - Neither name of Intel Corporation nor the names of its contributors 
 * may be used to endorse or promote products derived from this software 
 * without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#define ALLOC_COMMON_DATA
#include "common_data.h"
#include "sample_util.h"
#include "tv_device.h"

#include <stdarg.h>
#include <stdio.h>

/*!
 * \brief Prints a string to standard out.
 */
void linux_print(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	vfprintf(stdout, format, argList);
	fflush(stdout);
	va_end(argList);
}

/*!
 * \brief Function that receives commands from the user at the command prompt
 * during the lifetime of the device, and calls the appropriate
 * functions for those commands. Only one command, exit, is currently
 * defined.
 */
void *TvDeviceCommandLoop(void *args)
{
	int stoploop = 0;
	char cmdline[100];
	char cmd[100];

	while (!stoploop) {
		sprintf(cmdline, " ");
		sprintf(cmd, " ");
		SampleUtil_Print("\n>> ");
		/* Get a command line */
		fgets(cmdline, 100, stdin);
		sscanf(cmdline, "%s", cmd);
		if( strcasecmp(cmd, "exit") == 0) {
			SampleUtil_Print("Shutting down...\n");
			TvDeviceStop();
			exit(0);
		} else {
			SampleUtil_Print(
				"\n   Unknown command: %s\n\n", cmd);
			SampleUtil_Print(
				"   Valid Commands:\n"
				"     Exit\n\n");
		}
	}

	return NULL;
	args = args;
}

/*!
 * \brief Main entry point for tv device application.
 *
 * Initializes and registers with the sdk.
 * Initializes the state stables of the service.
 * Starts the command loop.
 *
 * Accepts the following optional arguments:
 *	\li \c -ip ipaddress 
 *	\li \c -port port
 *	\li \c -desc desc_doc_name 
 *	\li \c -webdir web_dir_path"
 *	\li \c -help 
 */
int main(int argc, char *argv[])
{
	unsigned int portTemp = 0;
	char *ip_address = NULL;
	char *desc_doc_name = NULL;
	char *web_dir_path = NULL;
	int rc;
	ithread_t cmdloop_thread;
#ifdef WIN32
#else
	int sig;
	sigset_t sigs_to_catch;
#endif
	int code;
	unsigned short port = 0;
	int i = 0;

	SampleUtil_Initialize(linux_print);
	/* Parse options */
	for(i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-ip") == 0) {
			ip_address = argv[++i];
		} else if(strcmp(argv[i], "-port") == 0) {
			sscanf(argv[++i], "%u", &portTemp);
		} else if(strcmp(argv[i], "-desc") == 0) {
			desc_doc_name = argv[++i];
		} else if(strcmp(argv[i], "-webdir") == 0) {
			web_dir_path = argv[++i];
		} else if(strcmp(argv[i], "-help") == 0) {
			SampleUtil_Print(
				"Usage: %s -ip ipaddress -port port"
				" -desc desc_doc_name -webdir web_dir_path"
				" -help (this message)\n", argv[0]);
			SampleUtil_Print(
				"\tipaddress:     IP address of the device"
				" (must match desc. doc)\n"
				"\t\te.g.: 192.168.0.4\n"
				"\tport:          Port number to use for"
				" receiving UPnP messages (must match desc. doc)\n"
				"\t\te.g.: 5431\n"
				"\tdesc_doc_name: name of device description document\n"
				"\t\te.g.: tvdevicedesc.xml\n"
				"\tweb_dir_path: Filesystem path where web files"
				" related to the device are stored\n"
				"\t\te.g.: /upnp/sample/tvdevice/web\n");
			return 1;
		}
	}
	port = (unsigned short)portTemp;
	TvDeviceStart(ip_address, port, desc_doc_name, web_dir_path,
		linux_print, 0);
	/* start a command loop thread */
	code = ithread_create(&cmdloop_thread, NULL, TvDeviceCommandLoop, NULL);
#ifdef WIN32
	ithread_join(cmdloop_thread, NULL);
#else
	/* Catch Ctrl-C and properly shutdown */
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGINT);
	sigwait(&sigs_to_catch, &sig);
	SampleUtil_Print("Shutting down on signal %d...\n", sig);
#endif
	rc = TvDeviceStop();

	return rc;
}

