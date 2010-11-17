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

#include "sample_util.h"
#include "upnp_tv_ctrlpt.h"
#include "upnp_tv_device.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*! Tags for valid commands issued at the command prompt. */
enum cmdloop_tvcmds {
	PRTHELP = 0, PRTFULLHELP, POWON, POWOFF,
	SETCHAN, SETVOL, SETCOL, SETTINT, SETCONT, SETBRT,
	CTRLACTION, PICTACTION, CTRLGETVAR, PICTGETVAR,
	PRTDEV, LSTDEV, REFRESH, EXITCMD
};

/*! Data structure for parsing commands from the command line. */
struct cmdloop_commands {
	/* the string  */
	const char *str;
	/* the command */
	int cmdnum;
	/* the number of arguments */
	int numargs;
	/* the args */
	const char *args;
} cmdloop_commands;

/*! Mappings between command text names, command tag,
 * and required command arguments for command line
 * commands */
static struct cmdloop_commands cmdloop_cmdlist[] = {
	{"Help", PRTHELP, 1, ""},
	{"HelpFull", PRTFULLHELP, 1, ""},
	{"ListDev", LSTDEV, 1, ""},
	{"Refresh", REFRESH, 1, ""},
	{"PrintDev", PRTDEV, 2, "<devnum>"},
	{"PowerOn", POWON, 2, "<devnum>"},
	{"PowerOff", POWOFF, 2, "<devnum>"},
	{"SetChannel", SETCHAN, 3, "<devnum> <channel (int)>"},
	{"SetVolume", SETVOL, 3, "<devnum> <volume (int)>"},
	{"SetColor", SETCOL, 3, "<devnum> <color (int)>"},
	{"SetTint", SETTINT, 3, "<devnum> <tint (int)>"},
	{"SetContrast", SETCONT, 3, "<devnum> <contrast (int)>"},
	{"SetBrightness", SETBRT, 3, "<devnum> <brightness (int)>"},
	{"CtrlAction", CTRLACTION, 2, "<devnum> <action (string)>"},
	{"PictAction", PICTACTION, 2, "<devnum> <action (string)>"},
	{"CtrlGetVar", CTRLGETVAR, 2, "<devnum> <varname (string)>"},
	{"PictGetVar", PICTGETVAR, 2, "<devnum> <varname (string)>"},
	{"Exit", EXITCMD, 1, ""}
};

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
 * \brief Print help info for this application.
 */
void TvCtrlPointPrintShortHelp(void)
{
	SampleUtil_Print(
		"Commands:\n"
    		"  Help\n"
    		"  HelpFull\n"
    		"  ListDev\n"
    		"  Refresh\n"
    		"  PrintDev      <devnum>\n"
    		"  PowerOn       <devnum>\n"
    		"  PowerOff      <devnum>\n"
    		"  SetChannel    <devnum> <channel>\n"
    		"  SetVolume     <devnum> <volume>\n"
    		"  SetColor      <devnum> <color>\n"
    		"  SetTint       <devnum> <tint>\n"
    		"  SetContrast   <devnum> <contrast>\n"
    		"  SetBrightness <devnum> <brightness>\n"
    		"  CtrlAction    <devnum> <action>\n"
    		"  PictAction    <devnum> <action>\n"
    		"  CtrlGetVar    <devnum> <varname>\n"
    		"  PictGetVar    <devnum> <action>\n"
    		"  Exit\n");
}

void TvCtrlPointPrintLongHelp(void)
{
	SampleUtil_Print(
		"\n"
		"******************************\n"
		"* TV Control Point Help Info *\n"
		"******************************\n"
		"\n"
		"This sample control point application automatically searches\n"
		"for and subscribes to the services of television device emulator\n"
		"devices, described in the tvdevicedesc.xml description document.\n"
		"It also registers itself as a tv device.\n"
		"\n"
		"Commands:\n"
		"  Help\n"
		"       Print this help info.\n"
		"  ListDev\n"
		"       Print the current list of TV Device Emulators that this\n"
		"         control point is aware of.  Each device is preceded by a\n"
		"         device number which corresponds to the devnum argument of\n"
		"         commands listed below.\n"
		"  Refresh\n"
		"       Delete all of the devices from the device list and issue new\n"
		"         search request to rebuild the list from scratch.\n"
		"  PrintDev       <devnum>\n"
		"       Print the state table for the device <devnum>.\n"
		"         e.g., 'PrintDev 1' prints the state table for the first\n"
		"         device in the device list.\n"
		"  PowerOn        <devnum>\n"
		"       Sends the PowerOn action to the Control Service of\n"
		"         device <devnum>.\n"
		"  PowerOff       <devnum>\n"
		"       Sends the PowerOff action to the Control Service of\n"
		"         device <devnum>.\n"
		"  SetChannel     <devnum> <channel>\n"
		"       Sends the SetChannel action to the Control Service of\n"
		"         device <devnum>, requesting the channel to be changed\n"
		"         to <channel>.\n"
		"  SetVolume      <devnum> <volume>\n"
		"       Sends the SetVolume action to the Control Service of\n"
		"         device <devnum>, requesting the volume to be changed\n"
		"         to <volume>.\n"
		"  SetColor       <devnum> <color>\n"
		"       Sends the SetColor action to the Control Service of\n"
		"         device <devnum>, requesting the color to be changed\n"
		"         to <color>.\n"
		"  SetTint        <devnum> <tint>\n"
		"       Sends the SetTint action to the Control Service of\n"
		"         device <devnum>, requesting the tint to be changed\n"
		"         to <tint>.\n"
		"  SetContrast    <devnum> <contrast>\n"
		"       Sends the SetContrast action to the Control Service of\n"
		"         device <devnum>, requesting the contrast to be changed\n"
		"         to <contrast>.\n"
		"  SetBrightness  <devnum> <brightness>\n"
		"       Sends the SetBrightness action to the Control Service of\n"
		"         device <devnum>, requesting the brightness to be changed\n"
		"         to <brightness>.\n"
		"  CtrlAction     <devnum> <action>\n"
		"       Sends an action request specified by the string <action>\n"
		"         to the Control Service of device <devnum>.  This command\n"
		"         only works for actions that have no arguments.\n"
		"         (e.g., \"CtrlAction 1 IncreaseChannel\")\n"
		"  PictAction     <devnum> <action>\n"
		"       Sends an action request specified by the string <action>\n"
		"         to the Picture Service of device <devnum>.  This command\n"
		"         only works for actions that have no arguments.\n"
		"         (e.g., \"PictAction 1 DecreaseContrast\")\n"
		"  CtrlGetVar     <devnum> <varname>\n"
		"       Requests the value of a variable specified by the string <varname>\n"
		"         from the Control Service of device <devnum>.\n"
		"         (e.g., \"CtrlGetVar 1 Volume\")\n"
		"  PictGetVar     <devnum> <action>\n"
		"       Requests the value of a variable specified by the string <varname>\n"
		"         from the Picture Service of device <devnum>.\n"
		"         (e.g., \"PictGetVar 1 Tint\")\n"
		"  Exit\n"
		"       Exits the control point application.\n");
}

/*!
 * \briefPrint the list of valid command line commands to the user
 */
void TvCtrlPointPrintCommands()
{
	int i;
	int numofcmds = (sizeof cmdloop_cmdlist) / sizeof (cmdloop_commands);

	SampleUtil_Print("Valid Commands:\n");
	for (i = 0; i < numofcmds; ++i) {
		SampleUtil_Print("  %-14s %s\n",
			cmdloop_cmdlist[i].str, cmdloop_cmdlist[i].args);
	}
	SampleUtil_Print("\n");
}

/*!
 * \brief Function that receives commands from the user at the command prompt
 * during the lifetime of the device, and calls the appropriate
 * functions for those commands. 
 */
void *TvCtrlPointCommandLoop(void *args)
{
	char cmdline[100];

	while (1) {
		SampleUtil_Print("\n>> ");
		fgets(cmdline, 100, stdin);
		TvCtrlPointProcessCommand(cmdline);
	}

	return NULL;
	args = args;
}

int TvCtrlPointProcessCommand(char *cmdline)
{
	char cmd[100];
	char strarg[100];
	int arg_val_err = -99999;
	int arg1 = arg_val_err;
	int arg2 = arg_val_err;
	int cmdnum = -1;
	int numofcmds = (sizeof cmdloop_cmdlist) / sizeof (cmdloop_commands);
	int cmdfound = 0;
	int i;
	int rc;
	int invalidargs = 0;
	int validargs;

	validargs = sscanf(cmdline, "%s %d %d", cmd, &arg1, &arg2);
	for (i = 0; i < numofcmds; ++i) {
		if (strcasecmp(cmd, cmdloop_cmdlist[i].str ) == 0) {
			cmdnum = cmdloop_cmdlist[i].cmdnum;
			cmdfound++;
			if (validargs != cmdloop_cmdlist[i].numargs)
				invalidargs++;
			break;
		}
	}
	if (!cmdfound) {
		SampleUtil_Print("Command not found; try 'Help'\n");
		return TV_SUCCESS;
	}
	if (invalidargs) {
		SampleUtil_Print("Invalid arguments; try 'Help'\n");
		return TV_SUCCESS;
	}
	switch (cmdnum) {
	case PRTHELP:
		TvCtrlPointPrintShortHelp();
		break;
	case PRTFULLHELP:
		TvCtrlPointPrintLongHelp();
		break;
	case POWON:
		TvCtrlPointSendPowerOn(arg1);
		break;
	case POWOFF:
		TvCtrlPointSendPowerOff(arg1);
		break;
	case SETCHAN:
		TvCtrlPointSendSetChannel(arg1, arg2);
		break;
	case SETVOL:
		TvCtrlPointSendSetVolume(arg1, arg2);
		break;
	case SETCOL:
		TvCtrlPointSendSetColor(arg1, arg2);
		break;
	case SETTINT:
		TvCtrlPointSendSetTint(arg1, arg2);
		break;
	case SETCONT:
		TvCtrlPointSendSetContrast(arg1, arg2);
		break;
	case SETBRT:
		TvCtrlPointSendSetBrightness(arg1, arg2);
		break;
	case CTRLACTION:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			TvCtrlPointSendAction(TV_SERVICE_CONTROL, arg1, strarg,
				NULL, NULL, 0);
		else
			invalidargs++;
		break;
	case PICTACTION:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			TvCtrlPointSendAction(TV_SERVICE_PICTURE, arg1, strarg,
				NULL, NULL, 0);
		else
			invalidargs++;
		break;
	case CTRLGETVAR:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			TvCtrlPointGetVar(TV_SERVICE_CONTROL, arg1, strarg);
		else
			invalidargs++;
		break;
	case PICTGETVAR:
		/* re-parse commandline since second arg is string. */
		validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
		if (validargs == 3)
			TvCtrlPointGetVar(TV_SERVICE_PICTURE, arg1, strarg);
		else
			invalidargs++;
		break;
	case PRTDEV:
		TvCtrlPointPrintDevice(arg1);
		break;
	case LSTDEV:
		TvCtrlPointPrintList();
		break;
	case REFRESH:
		TvCtrlPointRefresh();
		break;
	case EXITCMD:
		rc = TvCtrlPointStop();
		exit(rc);
		break;
	default:
		SampleUtil_Print("Command not implemented; see 'Help'\n");
		break;
	}
	if(invalidargs)
		SampleUtil_Print("Invalid args in command; see 'Help'\n");

	return TV_SUCCESS;
}

int device_main(int argc, char **argv)
{
    unsigned int portTemp = 0;
    char *ip_address = NULL,
     *desc_doc_name = NULL,
     *web_dir_path = NULL;
    unsigned short port = 0;
    int i = 0;

    SampleUtil_Initialize(linux_print);
    /* Parse options */
    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-ip" ) == 0 ) {
            ip_address = argv[++i];
        } else if( strcmp( argv[i], "-port" ) == 0 ) {
            sscanf( argv[++i], "%u", &portTemp );
        } else if( strcmp( argv[i], "-desc" ) == 0 ) {
            desc_doc_name = argv[++i];
        } else if( strcmp( argv[i], "-webdir" ) == 0 ) {
            web_dir_path = argv[++i];
        } else if( strcmp( argv[i], "-help" ) == 0 ) {
            SampleUtil_Print(
		"Usage: %s -ip ipaddress -port port\n"
		" -desc desc_doc_name -webdir web_dir_path\n"
		" -help (this message)\n\n", argv[0]);
            SampleUtil_Print(
		"\tipaddress:\tIP address of the device (must match desc. doc)\n"
		"\t\te.g.: 192.168.0.4\n\n"
		"\tport:\tPort number to use for receiving UPnP messages (must match desc. doc)\n"
		"\t\te.g.: 5431\n\n"
		"\tdesc_doc_name: name of device description document\n"
		"\t\te.g.: tvcombodesc.xml\n\n"
		"\tweb_dir_path: Filesystem path where web files related to the device are stored\n"
		"\t\te.g.: /upnp/sample/web\n\n");
		return 1;
        }
    }
    port = (unsigned short)portTemp;
    return TvDeviceStart(ip_address, port, desc_doc_name, web_dir_path, linux_print);
}

int main(int argc, char **argv)
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
	rc = TvCtrlPointStart(linux_print, NULL);
	if (rc != TV_SUCCESS) {
		SampleUtil_Print("Error starting UPnP TV Control Point\n");
		return rc;
	}
	/* start a command loop thread */
	code = ithread_create(&cmdloop_thread, NULL, TvCtrlPointCommandLoop, NULL);
#ifdef WIN32
	ithread_join(cmdloop_thread, NULL);
#else
	/* Catch Ctrl-C and properly shutdown */
	sigemptyset(&sigs_to_catch);
	sigaddset(&sigs_to_catch, SIGINT);
	sigwait(&sigs_to_catch, &sig);
	SampleUtil_Print("Shutting down on signal %d...\n", sig);
#endif
	TvDeviceStop();
	rc = TvCtrlPointStop();

	return rc;
}

