/*!
 * \file
 */
#include "config.h"

#include "UpnpLog.h"

#include "ithread.h"

#include <stdarg.h>
#include <stdio.h>

#if linux
#include <sys/syscall.h>
#endif

UpnpLog::UpnpLog()
: m_logLevel(UPNP_NONE)
, m_logFp(0)
, m_logIsStderr(false)
, m_logFileName()
, m_logCallback(0)
{
        ithread_mutex_init(&m_logMutex, NULL);
}

UpnpLog::~UpnpLog() { ithread_mutex_destroy(&m_logMutex); }

int UpnpLog::DebugAtThisLevel(Upnp_LogLevel DLevel, Dbg_Module Module)
{
        return (DLevel <= logLevel()) &&
                (DEBUG_ALL || (Module == SSDP && DEBUG_SSDP) ||
                        (Module == SOAP && DEBUG_SOAP) ||
                        (Module == GENA && DEBUG_GENA) ||
                        (Module == TPOOL && DEBUG_TPOOL) ||
                        (Module == MSERV && DEBUG_MSERV) ||
                        (Module == DOM && DEBUG_DOM) ||
                        (Module == HTTP && DEBUG_HTTP));
}

void UpnpLog::SetLogConfigFromEnvironment()
{
        char *logFile = getenv("UPNP_LOG_FILE");
        if (logFile) {
                setLogFileName(logFile);
        }
        char *logLevel = getenv("UPNP_LOG_LEVEL");
        if (logLevel) {
                Upnp_LogLevel mapped = UpnpLogLevelFromStr(logLevel);
                if (mapped != UPNP_LOG_LEVEL_ERROR) {
                        setLogLevel(mapped);
                }
        }
}

void UpnpLog::DisplayFileAndLine(
        const char *file, int line, Upnp_LogLevel DLevel, Dbg_Module Module)
{
        char timebuf[26];
        time_t now = time(NULL);
        struct tm *timeinfo;
        const char *smod;

        switch (Module) {
        case SSDP:
                smod = "SSDP";
                break;
        case SOAP:
                smod = "SOAP";
                break;
        case GENA:
                smod = "GENA";
                break;
        case TPOOL:
                smod = "TPOL";
                break;
        case MSERV:
                smod = "MSER";
                break;
        case DOM:
                smod = "DOM ";
                break;
        case API:
                smod = "API ";
                break;
        case HTTP:
                smod = "HTTP";
                break;
        default:
                smod = "UNKN";
                break;
        }

        timeinfo = localtime(&now);
        strftime(timebuf, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
        fprintf(logFp(),
                "%s %s UPNP:%s [0x%016lX][%s:%d] ",
                timebuf,
                UpnpLogLevelToStr(DLevel),
                smod,
#ifdef __PTW32_DLLPORT
                (unsigned long int)ithread_self().p
#else
#if linux
                (unsigned long int)syscall(SYS_gettid)
#else
                (unsigned long int)ithread_self()
#endif
#endif
                ,
                file,
                line);
        fflush(logFp());
}

int UpnpLog::InitLog()
{
        /* Env vars override other config */
        SetLogConfigFromEnvironment();

        /* If the user did not ask for logging do nothing */
        if (logLevel() == UPNP_NONE) {
                return 0;
        }
        if (logFp()) {
                if (!logIsStderr()) {
                        fclose(logFp());
                        setLogFp(0);
                }
        }
        setLogIsStderr(false);
        const char *fname = logFileName().c_str();
        if (fname) {
                setLogFp(fopen(fname, "a"));
                if (!logFp()) {
                        fprintf(stderr,
                                "Failed to open LogFileName (%s): %s\n",
                                fname,
                                strerror(errno));
                }
        }
        if (!logFp()) {
                setLogFp(stderr);
                setLogIsStderr(true);
        }

        return 0;
}

void UpnpLog::CloseLog()
{
        /* Calling lock() assumes that someone called UpnpInitLog(), but
         * this is reasonable as it is called from UpnpInit2(). We risk a
         * crash if we do this without a lock.*/
        logMutexLock();
        if (logFp() && !logIsStderr()) {
                fclose(logFp());
        }
        setLogFp(0);
        setLogIsStderr(false);
        logMutexUnlock();
}

void UpnpLog::Printf(Upnp_LogLevel DLevel,
        Dbg_Module Module,
        const char *file,
        int line,
        const char *FmtStr,
        ...)
{
        va_list ArgList;

        va_start(ArgList, FmtStr);
        logMutexLock();
        if (logCallback()) {
                char *buffer;
                int size;
#ifndef _WIN32
                size = vsnprintf(NULL, 0, FmtStr, ArgList) + 1;
#else
                size = _vscprintf(FmtStr, ArgList) + 1;
#endif
                buffer = new char[size]();
                if (!buffer) {
                        goto exit_function;
                }
                vsnprintf(buffer, size, FmtStr, ArgList);
                logCallback()(DLevel, Module, file, &line, buffer);
                delete[] buffer;
        }
        if (!DebugAtThisLevel(DLevel, Module)) {
                goto exit_function;
        }
        if (!logFp()) {
                goto exit_function;
        }
        if (file) {
                DisplayFileAndLine(file, line, DLevel, Module);
                vfprintf(logFp(), FmtStr, ArgList);
                fflush(logFp());
        }

exit_function:
        logMutexUnlock();
        va_end(ArgList);
}

/*
 * The C API
 */

UpnpLog *UpnpLog_new() { return new UpnpLog(); }

void UpnpLog_delete(UpnpLog *p) { delete p; }

const char *UpnpLogLevelToStr(Upnp_LogLevel level)
{
        switch (level) {
        case UPNP_CRITICAL:
                return "CRIT";
        case UPNP_ERROR:
                return "ERRO";
        case UPNP_INFO:
                return "INFO";
        case UPNP_DEBUG:
                return "DEBG";
        default:
                return "UNKN";
        }
}

Upnp_LogLevel UpnpLogLevelFromStr(char *level)
{
        if (strcmp(level, "CRIT") == 0) {
                return UPNP_CRITICAL;
        }
        if (strcmp(level, "ERRO") == 0) {
                return UPNP_ERROR;
        }
        if (strcmp(level, "INFO") == 0) {
                return UPNP_INFO;
        }
        if (strcmp(level, "DEBUG") == 0) {
                return UPNP_DEBUG;
        }
        if (strcmp(level, "NONE") == 0) {
                return UPNP_NONE;
        }
        return UPNP_LOG_LEVEL_ERROR;
}

/* This routine is called from UpnpInit2().
 * It can be called again, for example to rotate the log file, and we try to
 * avoid multiple calls to the mutex init, with a risk of race, probably not a
 * problem, and not worth fixing. */
int UpnpInitLog(UpnpLog *p) { return p->InitLog(); }

void UpnpSetLogLevel(UpnpLog *p, Upnp_LogLevel log_level)
{
        p->setLogLevel(log_level);
}

void UpnpCloseLog(UpnpLog *p) { p->CloseLog(); }

void UpnpSetLogFileName(UpnpLog *p, const char *newgLogFileName)
{
        if (!p->logFileName().empty()) {
                p->setLogFileName(0);
        }
        if (newgLogFileName && *newgLogFileName) {
                p->setLogFileName(newgLogFileName);
        }
}

void UpnpPrintf(UpnpLog *p,
        Upnp_LogLevel DLevel,
        Dbg_Module Module,
        const char *DbggLogFileName,
        int DbgLineNo,
        const char *FmtStr,
        ...)
{
        va_list argList;

        va_start(argList, FmtStr);
        if (p) {
                p->Printf(DLevel,
                        Module,
                        DbggLogFileName,
                        DbgLineNo,
                        FmtStr,
                        argList);
        } else {
                vprintf(FmtStr, argList);
        }
        va_end(argList);
}

void UpnpSetLogCallback(UpnpLog *p, LogCallback callback)
{
        p->setLogCallback(callback);
}
