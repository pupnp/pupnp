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

#include "common_data.h"
#include "sample_util.h"
#include "tv_device.h"

#include <stdarg.h>
#include <stdio.h>

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

int main(int argc, char *argv[])
{
	int rc;
	ithread_t cmdloop_thread;
#ifdef WIN32
#else
	int sig;
	sigset_t sigs_to_catch;
#endif
	int code;

	device_main(argc, argv);
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

