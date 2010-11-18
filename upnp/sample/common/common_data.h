/*
 * Copyright (C) 2010 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
 */

#ifndef COMMON_DATA
#define COMMON_DATA

/*!
 * \file
 */

#ifdef ALLOC_CMD_LINE
	/*! Tags for valid commands issued at the command prompt. */
	enum cmdloop_tvcmds {
		PRTHELP = 0,
		PRTFULLHELP,
		POWON,
		POWOFF,
		SETCHAN,
		SETVOL,
		SETCOL,
		SETTINT,
		SETCONT,
		SETBRT,
		CTRLACTION,
		PICTACTION,
		CTRLGETVAR,
		PICTGETVAR,
		PRTDEV,
		LSTDEV,
		REFRESH,
		EXITCMD
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
		{"Help",          PRTHELP,     1, ""},
		{"HelpFull",      PRTFULLHELP, 1, ""},
		{"ListDev",       LSTDEV,      1, ""},
		{"Refresh",       REFRESH,     1, ""},
		{"PrintDev",      PRTDEV,      2, "<devnum>"},
		{"PowerOn",       POWON,       2, "<devnum>"},
		{"PowerOff",      POWOFF,      2, "<devnum>"},
		{"SetChannel",    SETCHAN,     3, "<devnum> <channel (int)>"},
		{"SetVolume",     SETVOL,      3, "<devnum> <volume (int)>"},
		{"SetColor",      SETCOL,      3, "<devnum> <color (int)>"},
		{"SetTint",       SETTINT,     3, "<devnum> <tint (int)>"},
		{"SetContrast",   SETCONT,     3, "<devnum> <contrast (int)>"},
		{"SetBrightness", SETBRT,      3, "<devnum> <brightness (int)>"},
		{"CtrlAction",    CTRLACTION,  2, "<devnum> <action (string)>"},
		{"PictAction",    PICTACTION,  2, "<devnum> <action (string)>"},
		{"CtrlGetVar",    CTRLGETVAR,  2, "<devnum> <varname (string)>"},
		{"PictGetVar",    PICTGETVAR,  2, "<devnum> <varname (string)>"},
		{"Exit", EXITCMD, 1, ""}
	};
#else /* ALLOC_CMD_LINE */
#endif /* ALLOC_CMD_LINE */

#ifdef ALLOC_COMMON_DATA
	/*! Service types for tv services. */
	const char *TvServiceType[] = {
		"urn:schemas-upnp-org:service:tvcontrol:1",
		"urn:schemas-upnp-org:service:tvpicture:1"
	};
#else /* ALLOC_COMMON_DATA */
	extern const char *TvServiceType[];
#endif /* ALLOC_COMMON_DATA */

#endif /* COMMON_DATA */

