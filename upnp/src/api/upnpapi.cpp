/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2011-2012 France Telecom All rights reserved.
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

/*!
 * \addtogroup UPnPAPI
 *
 * @{
 *
 * \file
 */

#include "config.h"

#include "upnpapi.h"

#include "ThreadPool.h"
#include "UpnpLib.h"
#include "UpnpLog.h"
#include "UpnpStdInt.h"
#include "UpnpUniStd.h" /* for close() */
#include "httpreadwrite.h"
#include "membuffer.h"
#include "soaplib.h"
#include "ssdplib.h"
#include "sysdep.h"
#include "uuid.h"
#include "winutil.h"

/* Needed for GENA */
#include "gena.h"
#include "miniserver.h"
#include "service_table.h"

#ifdef INTERNAL_WEB_SERVER
#include "urlconfig.h"

#include "VirtualDir.h"
#include "webserver.h"
#endif /* INTERNAL_WEB_SERVER */

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
/* Do not include these files */
#else
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/types.h>
#endif

// ifr_netmask is not defined on eg OmniOS/Solaris, but since
// ifru_netmask/ifru_addr are all just union members, this should work
#ifndef ifr_netmask // it's a define if it exists
#define ifr_netmask ifr_addr
#endif

#ifdef UPNP_ENABLE_OPEN_SSL
#include <openssl/ssl.h>
#endif

#ifndef IN6_IS_ADDR_GLOBAL
#define IN6_IS_ADDR_GLOBAL(a) \
        ((((__const uint32_t *)(a))[0] & htonl((uint32_t)0x70000000)) == \
                htonl((uint32_t)0x20000000))
#endif /* IN6_IS_ADDR_GLOBAL */

#ifndef IN6_IS_ADDR_ULA
#define IN6_IS_ADDR_ULA(a) \
        ((((__const uint32_t *)(a))[0] & htonl((uint32_t)0xfe000000)) == \
                htonl((uint32_t)0xfc000000))
#endif /* IN6_IS_ADDR_ULA */

/*!
 * \brief (Windows Only) Initializes the Windows Winsock library.
 *
 * \return UPNP_E_SUCCESS on success, UPNP_E_INIT_FAILED on failure.
 */
static int WinsockInit(void)
{
        int retVal = UPNP_E_SUCCESS;
#ifdef _WIN32
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        wVersionRequested = MAKEWORD(2, 2);
        err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0) {
                /* Tell the user that we could not find a usable */
                /* WinSock DLL.                                  */
                retVal = UPNP_E_INIT_FAILED;
                goto exit_function;
        }
        /* Confirm that the WinSock DLL supports 2.2.
         * Note that if the DLL supports versions greater
         * than 2.2 in addition to 2.2, it will still return
         * 2.2 in wVersion since that is the version we
         * requested. */
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
                /* Tell the user that we could not find a usable
                 * WinSock DLL. */
                WSACleanup();
                retVal = UPNP_E_INIT_FAILED;
                goto exit_function;
        }
        /* The WinSock DLL is acceptable. Proceed. */
exit_function:
#else
#endif
        return retVal;
}

/*!
 * \brief Initializes the global mutexes used by the UPnP SDK.
 *
 * \return UPNP_E_SUCCESS on success or UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitMutexes(UpnpLib *p)
{
#ifdef __CYGWIN__
        /* On Cygwin, pthread_mutex_init() fails without this memset. */
        /* TODO: Fix Cygwin so we don't need this memset(). */
        UpnpLib_clear_GlobalHndRWLock(p);
#endif
        if (pthread_rwlock_init(UpnpLib_getnc_GlobalHndRWLock(p), NULL)) {
                return UPNP_E_INIT_FAILED;
        }

        if (pthread_mutex_init(UpnpLib_getnc_gUUIDMutex(p), NULL)) {
                return UPNP_E_INIT_FAILED;
        }
        /* initialize subscribe mutex. */
#ifdef INCLUDE_CLIENT_APIS
        if (pthread_mutex_init(
                    UpnpLib_getnc_GlobalClientSubscribeMutex(p), NULL) != 0) {
                return UPNP_E_INIT_FAILED;
        }
#endif
        return UPNP_E_SUCCESS;
}

/*!
 * \brief Initializes the global threadm pools used by the UPnP SDK.
 *
 * \return UPNP_E_SUCCESS on success or UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitThreadPools(UpnpLib *p)
{
        int ret = UPNP_E_SUCCESS;
        ThreadPoolAttr attr;

        TPAttrInit(&attr);
        TPAttrSetMaxThreads(&attr, MAX_THREADS);
        TPAttrSetMinThreads(&attr, MIN_THREADS);
        TPAttrSetStackSize(&attr, THREAD_STACK_SIZE);
        TPAttrSetJobsPerThread(&attr, JOBS_PER_THREAD);
        TPAttrSetIdleTime(&attr, THREAD_IDLE_TIME);
        TPAttrSetMaxJobsTotal(&attr, MAX_JOBS_TOTAL);

        if (ThreadPoolInit(p, UpnpLib_getnc_gSendThreadPool(p), &attr) !=
                UPNP_E_SUCCESS) {
                ret = UPNP_E_INIT_FAILED;
                goto exit_function;
        }

        if (ThreadPoolInit(p, UpnpLib_getnc_gRecvThreadPool(p), &attr) !=
                UPNP_E_SUCCESS) {
                ret = UPNP_E_INIT_FAILED;
                goto exit_function;
        }

        if (ThreadPoolInit(p, UpnpLib_getnc_gMiniServerThreadPool(p), &attr) !=
                UPNP_E_SUCCESS) {
                ret = UPNP_E_INIT_FAILED;
                goto exit_function;
        }

exit_function:
        if (ret != UPNP_E_SUCCESS) {
                UpnpLib_set_UpnpSdkInit(p, 0);
                UpnpFinish(p);
        }

        return ret;
}

/*!
 * \brief Performs the initial steps in initializing the UPnP SDK.
 *
 * \li Winsock library is initialized for the process (Windows specific).
 * \li The logging (for debug messages) is initialized.
 * \li Mutexes, Handle table and thread pools are allocated and initialized.
 * \li Callback functions for SOAP and GENA are set, if they're enabled.
 * \li The SDK timer thread is initialized.
 *
 * \return UPNP_E_SUCCESS on success.
 */
static int UpnpInitPreamble(UpnpLib *p)
{
        int retVal = UPNP_E_SUCCESS;
#ifdef UPNP_HAVE_OPTSSDP
        uuid_upnp nls_uuid;
        Upnp_SID nls_uuid_sid;
#endif /* UPNP_HAVE_OPTSSDP */

        retVal = WinsockInit();
        if (retVal != UPNP_E_SUCCESS) {
                return retVal;
        }

        /* needed by SSDP or other parts. */
        srand((unsigned int)time(NULL));

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpInitPreamble\n");

        /* Initialize SDK global mutexes. */
        retVal = UpnpInitMutexes(p);
        if (retVal != UPNP_E_SUCCESS) {
                return retVal;
        }

#ifdef UPNP_HAVE_OPTSSDP
        /* Create the NLS uuid. */
        uuid_create(p, &nls_uuid);
        upnp_uuid_unpack(&nls_uuid, nls_uuid_sid);
        UpnpLib_strcpy_gUpnpSdkNLSuuid(p, nls_uuid_sid);
#endif /* UPNP_HAVE_OPTSSDP */

        /* Initializes the handle list. */
        HandleLock();
        UpnpLib_clear_HandleTable(p);
        HandleUnlock();

        /* Initialize SDK global thread pools. */
        retVal = UpnpInitThreadPools(p);
        if (retVal != UPNP_E_SUCCESS) {
                return retVal;
        }

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SOAP == 0
        SetSoapCallback(soap_device_callback);
#endif
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INTERNAL_WEB_SERVER
#if EXCLUDE_GENA == 0
        SetGenaCallback(genaCallback);
#endif
#endif /* INTERNAL_WEB_SERVER */

        /* Initialize the SDK timer thread. */
        retVal = TimerThreadInit(UpnpLib_getnc_gTimerThread(p),
                UpnpLib_getnc_gSendThreadPool(p));
        if (retVal != UPNP_E_SUCCESS) {
                UpnpFinish(p);

                return retVal;
        }

        return UPNP_E_SUCCESS;
}

/*!
 * \brief Finishes initializing the UPnP SDK.
 *	\li The MiniServer is started, if enabled.
 *	\li The WebServer is started, if enabled.
 *
 * \return UPNP_E_SUCCESS on success or  UPNP_E_INIT_FAILED if a mutex could not
 * 	be initialized.
 */
static int UpnpInitStartServers(
        /*! [inout] Library handle */
        UpnpLib *p,
        /*! [in] Local Port to listen for incoming connections. */
        unsigned short DestPort)
{
#if EXCLUDE_MINISERVER == 0 || EXCLUDE_WEB_SERVER == 0
        int retVal = 0;
#endif

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Entering UpnpInitStartServers\n");

#if EXCLUDE_MINISERVER == 0
        UpnpLib_set_LOCAL_PORT_V4(p, DestPort);
        UpnpLib_set_LOCAL_PORT_V6(p, DestPort);
        UpnpLib_set_LOCAL_PORT_V6_ULA_GUA(p, DestPort);
        retVal = StartMiniServer(p);
        if (retVal != UPNP_E_SUCCESS) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "Miniserver failed to start\n");
                UpnpFinish(p);
                return retVal;
        }
#endif

#if EXCLUDE_WEB_SERVER == 0
        membuffer_init(UpnpLib_getnc_gDocumentRootDir(p));
        retVal = UpnpEnableWebserver(p, WEB_SERVER_ENABLED);
        if (retVal != UPNP_E_SUCCESS) {
                UpnpFinish(p);
                return retVal;
        }
#endif

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpInitStartServers\n");

        return UPNP_E_SUCCESS;
}

UpnpLog *UpnpGetLog(UpnpLib *p) { return UpnpLib_get_Log(p); }

int UpnpInit2(UpnpLib **LibraryHandle,
        const char *IfName,
        unsigned short DestPort,
        const char *logFileName)
{
        int retVal = UPNP_E_INIT;
        UpnpLib *p = *LibraryHandle;
        UpnpLog *l;

        /* Require that *LibraryHandle is NULL to make sure it is not a
         * reinitialization or something worse. */
        if (p) {
                printf("%s(%d): UpnpInit2 with IfName=%s, DestPort=%d. Error, "
                       "Library "
                       "Handle is not NULL\n",
                        __FILE__,
                        __LINE__,
                        IfName ? IfName : "NULL",
                        DestPort);
                goto exit_function_no_mutex_unlock;
        }

        p = UpnpLib_new();
        if (!p) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function_no_mutex_unlock;
        }
        l = UpnpLog_new();
        if (!l) {
                retVal = UPNP_E_OUTOF_MEMORY;
                UpnpLib_delete(p);
                goto exit_function_no_mutex_unlock;
        }
        UpnpSetLogFileName(l, logFileName);
        UpnpLib_set_Log(p, l);
        pthread_mutex_lock(UpnpLib_getnc_gSDKInitMutex(p));

        /* Check if we're already initialized. (Should never happen now) */
        if (UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_INIT;
                goto exit_function;
        }

        /* Perform initialization preamble. */
        retVal = UpnpInitPreamble(p);
        if (retVal != UPNP_E_SUCCESS) {
                goto exit_function;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "UpnpInit2 with IfName=%s, DestPort=%d.\n",
                IfName ? IfName : "NULL",
                DestPort);

        /* Retrieve interface information (Addresses, index, etc). */
        retVal = UpnpGetIfInfo(p, IfName);
        if (retVal != UPNP_E_SUCCESS) {
                goto exit_function;
        }

        /* Set the UpnpSdkInit flag to 1 to indicate we're successfully
         * initialized. */
        UpnpLib_set_UpnpSdkInit(p, 1);

        /* Finish initializing the SDK. */
        retVal = UpnpInitStartServers(p, DestPort);
        if (retVal != UPNP_E_SUCCESS) {
                UpnpLib_set_UpnpSdkInit(p, 0);
                goto exit_function;
        }
        *LibraryHandle = p;

exit_function:
        pthread_mutex_unlock(UpnpLib_getnc_gSDKInitMutex(p));
exit_function_no_mutex_unlock:

        return retVal;
}

#ifdef UPNP_ENABLE_OPEN_SSL
int UpnpInitSslContext(
        UpnpLib *p, int initOpenSslLib, const SSL_METHOD *sslMethod)
{
        if (UpnpLib_get_gSslCtx(p)) {
                return UPNP_E_INIT;
        }
        if (initOpenSslLib) {
                SSL_load_error_strings();
                SSL_library_init();
                OpenSSL_add_all_algorithms();
        }
        UpnpLib_set_gSslCtx(p, SSL_CTX_new(sslMethod));
        if (!UpnpLib_get_gSslCtx(p)) {
                return UPNP_E_INIT_FAILED;
        }

        return UPNP_E_SUCCESS;
}
#endif

#ifdef DEBUG
/*!
 * \brief Prints thread pool statistics.
 */
void PrintThreadPoolStats(
        /*! Library Handle. */
        UpnpLib *p,
        /*! [in] The thread pool. */
        ThreadPool *tp,
        /*! [in] The file name that called this function, use the macro
         * __FILE__. */
        const char *DbgFileName,
        /*! [in] The line number that the function was called, use the
         * macro
         * __LINE__. */
        int DbgLineNo,
        /*! [in] The message. */
        const char *msg)
{
        ThreadPoolStats stats;
        ThreadPoolGetStats(tp, &stats);
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                DbgFileName,
                DbgLineNo,
                "%s\n"
                "High Jobs pending: %d\n"
                "Med Jobs Pending: %d\n"
                "Low Jobs Pending: %d\n"
                "Average wait in High Q in milliseconds: %lf\n"
                "Average wait in Med Q in milliseconds: %lf\n"
                "Average wait in Low Q in milliseconds: %lf\n"
                "Max Threads Used: %d\n"
                "Worker Threads: %d\n"
                "Persistent Threads: %d\n"
                "Idle Threads: %d\n"
                "Total Threads: %d\n"
                "Total Work Time: %lf\n"
                "Total Idle Time: %lf\n",
                msg,
                stats.currentJobsHQ,
                stats.currentJobsMQ,
                stats.currentJobsLQ,
                stats.avgWaitHQ,
                stats.avgWaitMQ,
                stats.avgWaitLQ,
                stats.maxThreads,
                stats.workerThreads,
                stats.persistentThreads,
                stats.idleThreads,
                stats.totalThreads,
                stats.totalWorkTime,
                stats.totalIdleTime);
}
#else
#define PrintThreadPoolStats(p, tp, DbgFileName, DbgLineNo, msg) \
        do { \
        } while (0)
#endif /* DEBUG */

int UpnpFinish(UpnpLib *p)
{
#ifdef INCLUDE_DEVICE_APIS
        UpnpDevice_Handle device_handle;
#endif
#ifdef INCLUDE_CLIENT_APIS
        UpnpClient_Handle client_handle;
#endif
        struct Handle_Info *temp;
#ifdef UPNP_ENABLE_OPEN_SSL
        SSL_CTX *ctx = UpnpLib_get_gSslCtx(p);

        if (ctx) {
                SSL_CTX_free(ctx);
                UpnpLib_set_gSslCtx(p, 0);
        }
#endif
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpFinish: UpnpSdkInit is %d\n",
                UpnpLib_get_UpnpSdkInit(p));
        if (UpnpLib_get_UpnpSdkInit(p) == 1) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpFinish: UpnpSdkInit is ONE\n");
        }
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gSendThreadPool(p),
                __FILE__,
                __LINE__,
                "Send Thread Pool");
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gRecvThreadPool(p),
                __FILE__,
                __LINE__,
                "Recv Thread Pool");
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gMiniServerThreadPool(p),
                __FILE__,
                __LINE__,
                "MiniServer Thread Pool");
#ifdef INCLUDE_DEVICE_APIS
        while (GetDeviceHandleInfo(p, 0, AF_INET, &device_handle, &temp) ==
                HND_DEVICE) {
                UpnpUnRegisterRootDevice(p, device_handle);
        }
        while (GetDeviceHandleInfo(p, 0, AF_INET6, &device_handle, &temp) ==
                HND_DEVICE) {
                UpnpUnRegisterRootDevice(p, device_handle);
        }
#endif
#ifdef INCLUDE_CLIENT_APIS
        while (HND_CLIENT == GetClientHandleInfo(p, &client_handle, &temp)) {
                UpnpUnRegisterClient(p, client_handle);
        }
#endif
        TimerThreadShutdown(UpnpLib_getnc_gTimerThread(p));
#if EXCLUDE_MINISERVER == 0
        StopMiniServer(p);
#endif
#if EXCLUDE_WEB_SERVER == 0
        web_server_destroy(p);
#endif
        ThreadPoolShutdown(UpnpLib_getnc_gMiniServerThreadPool(p));
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gMiniServerThreadPool(p),
                __FILE__,
                __LINE__,
                "MiniServer Thread Pool");
        ThreadPoolShutdown(UpnpLib_getnc_gRecvThreadPool(p));
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gRecvThreadPool(p),
                __FILE__,
                __LINE__,
                "Send Thread Pool");
        ThreadPoolShutdown(UpnpLib_getnc_gSendThreadPool(p));
        PrintThreadPoolStats(p,
                UpnpLib_getnc_gRecvThreadPool(p),
                __FILE__,
                __LINE__,
                "Recv Thread Pool");
#ifdef INCLUDE_CLIENT_APIS
        pthread_mutex_destroy(UpnpLib_getnc_GlobalClientSubscribeMutex(p));
#endif
        pthread_rwlock_destroy(UpnpLib_getnc_GlobalHndRWLock(p));
        pthread_mutex_destroy(UpnpLib_getnc_gUUIDMutex(p));
        /* remove all virtual dirs */
        UpnpRemoveAllVirtualDirs(p);
        UpnpLib_set_UpnpSdkInit(p, 0);
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpFinish: UpnpSdkInit is :%d:\n",
                UpnpLib_get_UpnpSdkInit(p));
        UpnpCloseLog(UpnpLib_get_Log(p));

        return UPNP_E_SUCCESS;
}

unsigned short UpnpGetServerPort(UpnpLib *p)
{
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return 0;
        }

        return UpnpLib_get_LOCAL_PORT_V4(p);
}

unsigned short UpnpGetServerPort6(UpnpLib *p)
{
#ifdef UPNP_ENABLE_IPV6
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return 0;
        }

        return UpnpLib_get_LOCAL_PORT_V6(p);
#else
        return 0;
#endif
}

unsigned short UpnpGetServerUlaGuaPort6(UpnpLib *p)
{
#ifdef UPNP_ENABLE_IPV6
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return 0;
        }

        return UpnpLib_get_LOCAL_PORT_V6_ULA_GUA(p);
#else
        return 0;
#endif
}

const char *UpnpGetServerIpAddress(UpnpLib *p)
{
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return NULL;
        }

        return UpnpLib_get_gIF_IPV4_cstr(p);
}

const char *UpnpGetServerIp6Address(UpnpLib *p)
{
#ifdef UPNP_ENABLE_IPV6
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return NULL;
        }

        return UpnpLib_get_gIF_IPV6_cstr(p);
#else
        return NULL;
#endif
}

const char *UpnpGetServerUlaGuaIp6Address(UpnpLib *p)
{
#ifdef UPNP_ENABLE_IPV6
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return NULL;
        }

        return UpnpLib_get_gIF_IPV6_ULA_GUA_cstr(p);
#else
        return NULL;
#endif
}

/*!
 * \brief Get a free handle.
 *
 * \return On success, an integer greater than zero or
 * UPNP_E_OUTOF_HANDLE on failure.
 */
static int GetFreeHandle(UpnpLib *p)
{
        /* Handle 0 is not used as NULL translates to 0 when passed as a
         * handle
         */
        int i = 1;
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);

        while (i < HANDLE_TABLE_MAX_NUM_ELEMENTS && HandleTable->handle[i]) {
                ++i;
        }
        if (i == HANDLE_TABLE_MAX_NUM_ELEMENTS) {
                return UPNP_E_OUTOF_HANDLE;
        } else {
                return i;
        }
}

/*!
 * \brief Free handle.
 *
 * \return UPNP_E_SUCCESS if successful or UPNP_E_INVALID_HANDLE if not
 */
static int FreeHandle(
        /*! Library handle. */
        UpnpLib *p,
        /*! [in] Handle index. */
        int Upnp_Handle)
{
        int ret = UPNP_E_INVALID_HANDLE;
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "FreeHandle: entering, Handle is %d\n",
                Upnp_Handle);
        if (Upnp_Handle < 1 || Upnp_Handle >= HANDLE_TABLE_MAX_NUM_ELEMENTS) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "FreeHandle: Handle %d is out of range\n",
                        Upnp_Handle);
        } else if (!HandleTable->handle[Upnp_Handle]) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "FreeHandle: HandleTable[%d] is NULL\n",
                        Upnp_Handle);
        } else {
                free(HandleTable->handle[Upnp_Handle]);
                HandleTable->handle[Upnp_Handle] = NULL;
                ret = UPNP_E_SUCCESS;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "FreeHandle: exiting, ret = %d.\n",
                ret);

        return ret;
}

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice(UpnpLib *p,
        const char *DescUrl,
        Upnp_FunPtr Fun,
        const void *Cookie,
        UpnpDevice_Handle *Hnd)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        struct Handle_Info *HInfo = NULL;
        int retVal = 0;
#if EXCLUDE_GENA == 0
        int hasServiceTable = 0;
#endif /* EXCLUDE_GENA */

        HandleLock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRegisterRootDevice\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }

        if (Hnd == NULL || Fun == NULL || DescUrl == NULL ||
                strlen(DescUrl) == (size_t)0) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }

        *Hnd = GetFreeHandle(p);
        if (*Hnd == UPNP_E_OUTOF_HANDLE) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }

        HInfo = (struct Handle_Info *)std::calloc(1, sizeof(struct Handle_Info));
        if (HInfo == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        HandleTable->handle[*Hnd] = HInfo;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Root device URL is %s\n",
                DescUrl);

        HInfo->aliasInstalled = 0;
        HInfo->HType = HND_DEVICE;
        strncpy(HInfo->DescURL, DescUrl, sizeof(HInfo->DescURL) - 1);
        strncpy(HInfo->LowerDescURL, DescUrl, sizeof(HInfo->LowerDescURL) - 1);
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Following Root Device URL will be used when answering "
                "to "
                "legacy CPs %s\n",
                HInfo->LowerDescURL);
        HInfo->Callback = Fun;
        HInfo->Cookie = (const char *)Cookie;
        HInfo->MaxAge = DEFAULT_MAXAGE;
        HInfo->DeviceList = NULL;
        HInfo->ServiceList = NULL;
        HInfo->DescDocument = NULL;
#ifdef INCLUDE_CLIENT_APIS
        ListInit(&HInfo->SsdpSearchList, NULL, NULL);
        HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
        HInfo->MaxSubscriptions = UPNP_INFINITE;
        HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
        HInfo->DeviceAf = AF_INET;

        retVal = UpnpDownloadXmlDoc(p, HInfo->DescURL, &(HInfo->DescDocument));
        if (retVal != UPNP_E_SUCCESS) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice: error downloading "
                        "Document: "
                        "%d\n",
                        retVal);
#ifdef INCLUDE_CLIENT_APIS
                ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
                FreeHandle(p, *Hnd);
                goto exit_function;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice: Valid Description\n"
                "UpnpRegisterRootDevice: DescURL : %s\n",
                HInfo->DescURL);

        HInfo->DeviceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "device");
        if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
                ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
                ixmlDocument_free(HInfo->DescDocument);
                FreeHandle(p, *Hnd);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice: No devices found for "
                        "RootDevice\n");
                retVal = UPNP_E_INVALID_DESC;
                goto exit_function;
        }

        HInfo->ServiceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "serviceList");
        if (!HInfo->ServiceList) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice: No services found for "
                        "RootDevice\n");
        }

#if EXCLUDE_GENA == 0
        /*
         * GENA SET UP
         */
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice: Gena Check\n");
        HInfo->ServiceTable = {};
        hasServiceTable = getServiceTable(p,
                (IXML_Node *)HInfo->DescDocument,
                &HInfo->ServiceTable,
                HInfo->DescURL);
        if (hasServiceTable) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice: GENA Service Table\n"
                        "Here are the known services:\n");
                printServiceTable(p, &HInfo->ServiceTable, UPNP_DEBUG, API);
        } else {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "\nUpnpRegisterRootDevice: Empty service "
                        "table\n");
        }
#endif /* EXCLUDE_GENA */

        UpnpLib_set_UpnpSdkDeviceRegisteredV4(p, 1);

        retVal = UPNP_E_SUCCESS;

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting RegisterRootDevice, return value == %d\n",
                retVal);
        HandleUnlock();

        return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

/*!
 * \brief Fills the sockadr_in with miniserver information.
 */
static int GetDescDocumentAndURL(
        /*! Library handle. */
        UpnpLib *p,
        /* [in] pointer to server address structure. */
        Upnp_DescType descriptionType,
        /* [in] . */
        char *description,
        /* [in] . */
        int config_baseURL,
        /* [in] . */
        int AddressFamily,
        /* [out] . */
        IXML_Document **xmlDoc,
        /* [out] . */
        char descURL[LINE_SIZE]);

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice2(UpnpLib *p,
        Upnp_DescType descriptionType,
        const char *description_const,
        size_t bufferLen, /* ignored */
        int config_baseURL,
        Upnp_FunPtr Fun,
        const void *Cookie,
        UpnpDevice_Handle *Hnd)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        struct Handle_Info *HInfo = NULL;
        int retVal = 0;
#if EXCLUDE_GENA == 0
        int hasServiceTable = 0;
#endif /* EXCLUDE_GENA */
        char *description = (char *)description_const;
        (void)bufferLen;

        HandleLock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRegisterRootDevice2\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }

        if (Hnd == NULL || Fun == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }

        *Hnd = GetFreeHandle(p);
        if (*Hnd == UPNP_E_OUTOF_HANDLE) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }

        HInfo = (struct Handle_Info *)std::calloc(1, sizeof(struct Handle_Info));
        if (HInfo == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        HandleTable->handle[*Hnd] = HInfo;

        /* prevent accidental removal of a non-existent alias */
        HInfo->aliasInstalled = 0;

        retVal = GetDescDocumentAndURL(p,
                descriptionType,
                description,
                config_baseURL,
                AF_INET,
                &HInfo->DescDocument,
                HInfo->DescURL);
        if (retVal != UPNP_E_SUCCESS) {
                FreeHandle(p, *Hnd);
                goto exit_function;
        }

        strncpy(HInfo->LowerDescURL,
                HInfo->DescURL,
                sizeof(HInfo->LowerDescURL) - 1);
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Following Root Device URL will be used when answering "
                "to "
                "legacy CPs %s\n",
                HInfo->LowerDescURL);
        HInfo->aliasInstalled = config_baseURL != 0;
        HInfo->HType = HND_DEVICE;
        HInfo->Callback = Fun;
        HInfo->Cookie = (const char *)Cookie;
        HInfo->MaxAge = DEFAULT_MAXAGE;
        HInfo->DeviceList = NULL;
        HInfo->ServiceList = NULL;
#ifdef INCLUDE_CLIENT_APIS
        ListInit(&HInfo->SsdpSearchList, NULL, NULL);
        HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
        HInfo->MaxSubscriptions = UPNP_INFINITE;
        HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
        HInfo->DeviceAf = AF_INET;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice2: Valid Description\n"
                "UpnpRegisterRootDevice2: DescURL : %s\n",
                HInfo->DescURL);

        HInfo->DeviceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "device");
        if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
                ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
                ixmlDocument_free(HInfo->DescDocument);
                FreeHandle(p, *Hnd);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice2: No devices found for "
                        "RootDevice\n");
                retVal = UPNP_E_INVALID_DESC;
                goto exit_function;
        }

        HInfo->ServiceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "serviceList");
        if (!HInfo->ServiceList) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice2: No services found "
                        "for "
                        "RootDevice\n");
        }

#if EXCLUDE_GENA == 0
        /*
         * GENA SET UP
         */
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice2: Gena Check\n");
        HInfo->ServiceTable = {};
        hasServiceTable = getServiceTable(p,
                (IXML_Node *)HInfo->DescDocument,
                &HInfo->ServiceTable,
                HInfo->DescURL);
        if (hasServiceTable) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice2: GENA Service Table\n"
                        "Here are the known services: \n");
                printServiceTable(p, &HInfo->ServiceTable, UPNP_DEBUG, API);
        } else {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "\nUpnpRegisterRootDevice2: Empty service "
                        "table\n");
        }
#endif /* EXCLUDE_GENA */

        UpnpLib_set_UpnpSdkDeviceRegisteredV4(p, 1);

        retVal = UPNP_E_SUCCESS;

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting RegisterRootDevice2, return value == %d\n",
                retVal);
        HandleUnlock();

        return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice3(UpnpLib *p,
        const char *DescUrl,
        Upnp_FunPtr Fun,
        const void *Cookie,
        UpnpDevice_Handle *Hnd,
        int AddressFamily)
{
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRegisterRootDevice3\n");
        return UpnpRegisterRootDevice4(
                p, DescUrl, Fun, Cookie, Hnd, AddressFamily, NULL);
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpRegisterRootDevice4(UpnpLib *p,
        const char *DescUrl,
        Upnp_FunPtr Fun,
        const void *Cookie,
        UpnpDevice_Handle *Hnd,
        int AddressFamily,
        const char *LowerDescUrl)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        struct Handle_Info *HInfo;
        int retVal = 0;
#if EXCLUDE_GENA == 0
        int hasServiceTable = 0;
#endif /* EXCLUDE_GENA */

        HandleLock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRegisterRootDevice4\n");
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }
        if (Hnd == NULL || Fun == NULL || DescUrl == NULL ||
                strlen(DescUrl) == (size_t)0 ||
                (AddressFamily != AF_INET && AddressFamily != AF_INET6)) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        *Hnd = GetFreeHandle(p);
        if (*Hnd == UPNP_E_OUTOF_HANDLE) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        HInfo = (struct Handle_Info *)std::calloc(1, sizeof(struct Handle_Info));
        if (HInfo == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        HandleTable->handle[*Hnd] = HInfo;
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Root device URL is %s\n",
                DescUrl);
        HInfo->aliasInstalled = 0;
        HInfo->HType = HND_DEVICE;
        strncpy(HInfo->DescURL, DescUrl, sizeof(HInfo->DescURL) - 1);
        if (LowerDescUrl == NULL)
                strncpy(HInfo->LowerDescURL,
                        DescUrl,
                        sizeof(HInfo->LowerDescURL) - 1);
        else
                strncpy(HInfo->LowerDescURL,
                        LowerDescUrl,
                        sizeof(HInfo->LowerDescURL) - 1);
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Following Root Device URL will be used when answering "
                "to "
                "legacy CPs %s\n",
                HInfo->LowerDescURL);
        HInfo->Callback = Fun;
        HInfo->Cookie = (const char *)Cookie;
        HInfo->MaxAge = DEFAULT_MAXAGE;
        HInfo->DeviceList = NULL;
        HInfo->ServiceList = NULL;
        HInfo->DescDocument = NULL;
#ifdef INCLUDE_CLIENT_APIS
        ListInit(&HInfo->SsdpSearchList, NULL, NULL);
        HInfo->ClientSubList = NULL;
#endif /* INCLUDE_CLIENT_APIS */
        HInfo->MaxSubscriptions = UPNP_INFINITE;
        HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
        HInfo->DeviceAf = AddressFamily;
        retVal = UpnpDownloadXmlDoc(p, HInfo->DescURL, &(HInfo->DescDocument));
        if (retVal != UPNP_E_SUCCESS) {
#ifdef INCLUDE_CLIENT_APIS
                ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
                FreeHandle(p, *Hnd);
                goto exit_function;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice4: Valid Description\n"
                "UpnpRegisterRootDevice4: DescURL : %s\n",
                HInfo->DescURL);

        HInfo->DeviceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "device");
        if (!HInfo->DeviceList) {
#ifdef INCLUDE_CLIENT_APIS
                ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
                ixmlDocument_free(HInfo->DescDocument);
                FreeHandle(p, *Hnd);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice4: No devices found for "
                        "RootDevice\n");
                retVal = UPNP_E_INVALID_DESC;
                goto exit_function;
        }

        HInfo->ServiceList = ixmlDocument_getElementsByTagName(
                HInfo->DescDocument, "serviceList");
        if (!HInfo->ServiceList) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice4: No services found "
                        "for "
                        "RootDevice\n");
        }

#if EXCLUDE_GENA == 0
        /*
         * GENA SET UP
         */
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "UpnpRegisterRootDevice4: Gena Check\n");
        HInfo->ServiceTable = {};
        hasServiceTable = getServiceTable(p,
                (IXML_Node *)HInfo->DescDocument,
                &HInfo->ServiceTable,
                HInfo->DescURL);
        if (hasServiceTable) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "UpnpRegisterRootDevice4: GENA Service Table \n"
                        "Here are the known services: \n");
                printServiceTable(p, &HInfo->ServiceTable, UPNP_DEBUG, API);
        } else {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "\nUpnpRegisterRootDevice4: Empty service "
                        "table\n");
        }
#endif /* EXCLUDE_GENA */

        switch (AddressFamily) {
        case AF_INET:
                UpnpLib_set_UpnpSdkDeviceRegisteredV4(p, 1);
                break;
        default:
                UpnpLib_set_UpnpSdkDeviceRegisteredV6(p, 1);
        }

        retVal = UPNP_E_SUCCESS;

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting RegisterRootDevice4, return value == %d\n",
                retVal);
        HandleUnlock();

        return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpUnRegisterRootDevice(UpnpLib *p, UpnpDevice_Handle Hnd)
{
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpUnRegisterRootDevice\n");
        return UpnpUnRegisterRootDeviceLowPower(p, Hnd, -1, -1, -1);
}

int UpnpUnRegisterRootDeviceLowPower(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        int PowerState,
        int SleepPeriod,
        int RegistrationState)
{
        int retVal = 0;
        struct Handle_Info *HInfo = NULL;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpUnRegisterRootDeviceLowPower\n");
#if EXCLUDE_GENA == 0
        if (genaUnregisterDevice(p, Hnd) != UPNP_E_SUCCESS)
                return UPNP_E_INVALID_HANDLE;
#endif

        HandleLock();
        switch (GetHandleInfo(p, Hnd, &HInfo)) {
        case HND_INVALID:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        default:
                break;
        }
        HInfo->PowerState = PowerState;
        if (SleepPeriod < 0)
                SleepPeriod = -1;
        HInfo->SleepPeriod = SleepPeriod;
        HInfo->RegistrationState = RegistrationState;
        HandleUnlock();

#if EXCLUDE_SSDP == 0
        retVal = AdvertiseAndReply(p,
                -1,
                Hnd,
                (enum SsdpSearchType)0,
                (struct sockaddr *)NULL,
                (char *)NULL,
                (char *)NULL,
                (char *)NULL,
                HInfo->MaxAge);
#endif

        HandleLock();
        switch (GetHandleInfo(p, Hnd, &HInfo)) {
        case HND_INVALID:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        default:
                break;
        }
        ixmlNodeList_free(HInfo->DeviceList);
        ixmlNodeList_free(HInfo->ServiceList);
        ixmlDocument_free(HInfo->DescDocument);
#ifdef INCLUDE_CLIENT_APIS
        ListDestroy(&HInfo->SsdpSearchList, 0);
#endif /* INCLUDE_CLIENT_APIS */
#ifdef INTERNAL_WEB_SERVER
        if (HInfo->aliasInstalled)
                web_server_set_alias(p, NULL, NULL, 0, 0);
#endif /* INTERNAL_WEB_SERVER */
        switch (HInfo->DeviceAf) {
        case AF_INET:
                UpnpLib_set_UpnpSdkDeviceRegisteredV4(p, 0);
                break;
        case AF_INET6:
                UpnpLib_set_UpnpSdkDeviceRegisteredV6(p, 0);
                break;
        default:
                break;
        }
        FreeHandle(p, Hnd);
        HandleUnlock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpUnRegisterRootDeviceLowPower\n");

        return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpRegisterClient(
        UpnpLib *p, Upnp_FunPtr Fun, const void *Cookie, UpnpClient_Handle *Hnd)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        struct Handle_Info *HInfo;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRegisterClient \n");
        if (Fun == NULL || Hnd == NULL)
                return UPNP_E_INVALID_PARAM;

        HandleLock();
        if ((HANDLE_TABLE_MAX_NUM_ELEMENTS - 1) <=
                (UpnpLib_get_UpnpSdkClientRegistered(p) +
                        UpnpLib_get_UpnpSdkDeviceRegisteredV4(p) +
                        UpnpLib_get_UpnpSdkDeviceRegisteredV6(p))) {
                HandleUnlock();
                return UPNP_E_ALREADY_REGISTERED;
        }
        if ((*Hnd = GetFreeHandle(p)) == UPNP_E_OUTOF_HANDLE) {
                HandleUnlock();
                return UPNP_E_OUTOF_MEMORY;
        }
        HInfo = (struct Handle_Info *)malloc(sizeof(struct Handle_Info));
        if (HInfo == NULL) {
                HandleUnlock();
                return UPNP_E_OUTOF_MEMORY;
        }
        HInfo->HType = HND_CLIENT;
        HInfo->Callback = Fun;
        HInfo->Cookie = (const char *)Cookie;
        HInfo->ClientSubList = NULL;
        ListInit(&HInfo->SsdpSearchList, NULL, NULL);
#ifdef INCLUDE_DEVICE_APIS
        HInfo->MaxAge = 0;
        HInfo->MaxSubscriptions = UPNP_INFINITE;
        HInfo->MaxSubscriptionTimeOut = UPNP_INFINITE;
#endif
        HandleTable->handle[*Hnd] = HInfo;
        UpnpLib_set_UpnpSdkClientRegistered(
                p, UpnpLib_get_UpnpSdkClientRegistered(p) + 1);
        HandleUnlock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpRegisterClient \n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnRegisterClient(UpnpLib *p, UpnpClient_Handle Hnd)
{
        struct Handle_Info *HInfo;
        ListNode *node = NULL;
        SsdpSearchArg *searchArg = NULL;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpUnRegisterClient \n");

        HandleLock();
        if (!UpnpLib_get_UpnpSdkClientRegistered(p)) {
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

#if EXCLUDE_GENA == 0
        if (genaUnregisterClient(p, Hnd) != UPNP_E_SUCCESS)
                return UPNP_E_INVALID_HANDLE;
#endif
        HandleLock();
        switch (GetHandleInfo(p, Hnd, &HInfo)) {
        case HND_INVALID:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        default:
                break;
        }
        /* clean up search list */
        node = ListHead(&HInfo->SsdpSearchList);
        while (node != NULL) {
                searchArg = (SsdpSearchArg *)node->item;
                if (searchArg) {
                        free(searchArg->searchTarget);
                        free(searchArg);
                }
                ListDelNode(&HInfo->SsdpSearchList, node, 0);
                node = ListHead(&HInfo->SsdpSearchList);
        }
        ListDestroy(&HInfo->SsdpSearchList, 0);
        FreeHandle(p, Hnd);
        UpnpLib_set_UpnpSdkClientRegistered(
                p, UpnpLib_get_UpnpSdkClientRegistered(p) - 1);
        HandleUnlock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpUnRegisterClient \n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER
/*!
 * \brief Determines alias for given name which is a file name or URL.
 *
 * \return UPNP_E_SUCCESS on success, nonzero on failure.
 */
static int GetNameForAlias(
        /*! [in] Name of the file. */
        char *name,
        /*! [out] Pointer to alias string. */
        char **alias)
{
        char *ext;
        char *al;

        ext = strrchr(name, '.');
        if (ext == NULL || strcasecmp(ext, ".xml") != 0) {
                return UPNP_E_EXT_NOT_XML;
        }

        al = strrchr(name, '/');
        if (al == NULL) {
                *alias = name;
        } else {
                *alias = al;
        }

        return UPNP_E_SUCCESS;
}

/*!
 * \brief Fill the sockadr with IPv4 miniserver information.
 */
static void get_server_addr(
        /*! Library handle. */
        UpnpLib *p,
        /*! [out] pointer to server address structure. */
        struct sockaddr *serverAddr)
{
        struct sockaddr_in *sa4 = (struct sockaddr_in *)serverAddr;

        *serverAddr = {};

        sa4->sin_family = AF_INET;
        inet_pton(AF_INET, UpnpLib_get_gIF_IPV4_cstr(p), &sa4->sin_addr);
        sa4->sin_port = htons(UpnpLib_get_LOCAL_PORT_V4(p));
}

/*!
 * \brief Fill the sockadr with IPv6 miniserver information.
 */
static void get_server_addr6(
        /*! Library handle. */
        UpnpLib *p,
        /*! [out] pointer to server address structure. */
        struct sockaddr *serverAddr)
{
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)serverAddr;

        *serverAddr = {};

        sa6->sin6_family = AF_INET6;
        inet_pton(AF_INET6, UpnpLib_get_gIF_IPV6_cstr(p), &sa6->sin6_addr);
        sa6->sin6_port = htons(UpnpLib_get_LOCAL_PORT_V6(p));
}

static int GetDescDocumentAndURL(UpnpLib *p,
        Upnp_DescType descriptionType,
        char *description,
        int config_baseURL,
        int AddressFamily,
        IXML_Document **xmlDoc,
        char descURL[LINE_SIZE])
{
        int retVal = 0;
        char *membuf = NULL;
        char aliasStr[LINE_SIZE] = {};
        char *temp_str = NULL;
        FILE *fp = NULL;
        int fd = 0;
        size_t fileLen;
        size_t num_read;
        time_t last_modified;
        struct stat file_info;
        struct sockaddr_storage serverAddr;
        int rc = UPNP_E_SUCCESS;

        if (description == NULL)
                return UPNP_E_INVALID_PARAM;
        /* non-URL description must have configuration specified */
        if (descriptionType != (enum Upnp_DescType_e)UPNPREG_URL_DESC &&
                !config_baseURL)
                return UPNP_E_INVALID_PARAM;
        /* Get XML doc and last modified time */
        if (descriptionType == (enum Upnp_DescType_e)UPNPREG_URL_DESC) {
                retVal = UpnpDownloadXmlDoc(p, description, xmlDoc);
                if (retVal != UPNP_E_SUCCESS)
                        return retVal;
                last_modified = time(NULL);
        } else if (descriptionType ==
                (enum Upnp_DescType_e)UPNPREG_FILENAME_DESC) {
                int ret = 0;

                fp = fopen(description, "rb");
                if (!fp) {
                        rc = UPNP_E_FILE_NOT_FOUND;
                        ret = 1;
                        goto exit_function1;
                }
                fd = fileno(fp);
                if (fd == -1) {
                        rc = UPNP_E_FILE_NOT_FOUND;
                        ret = 1;
                        goto exit_function1;
                }
                retVal = fstat(fd, &file_info);
                if (retVal == -1) {
                        rc = UPNP_E_FILE_NOT_FOUND;
                        ret = 1;
                        goto exit_function1;
                }
                fileLen = (size_t)file_info.st_size;
                last_modified = file_info.st_mtime;
                membuf = (char *)malloc(fileLen + (size_t)1);
                if (!membuf) {
                        rc = UPNP_E_OUTOF_MEMORY;
                        ret = 1;
                        goto exit_function1;
                }
                num_read = fread(membuf, (size_t)1, fileLen, fp);
                if (num_read != fileLen) {
                        rc = UPNP_E_FILE_READ_ERROR;
                        ret = 1;
                        goto exit_function2;
                }
                membuf[fileLen] = 0;
                rc = ixmlParseBufferEx(membuf, xmlDoc);
        exit_function2:
                if (membuf) {
                        free(membuf);
                }
        exit_function1:
                if (fp) {
                        fclose(fp);
                }
                if (ret) {
                        return rc;
                }
        } else if (descriptionType == (enum Upnp_DescType_e)UPNPREG_BUF_DESC) {
                last_modified = time(NULL);
                rc = ixmlParseBufferEx(description, xmlDoc);
        } else {
                return UPNP_E_INVALID_PARAM;
        }

        if (rc != IXML_SUCCESS &&
                descriptionType != (enum Upnp_DescType_e)UPNPREG_URL_DESC) {
                if (rc == IXML_INSUFFICIENT_MEMORY)
                        return UPNP_E_OUTOF_MEMORY;
                else
                        return UPNP_E_INVALID_DESC;
        }
        /* Determine alias */
        if (config_baseURL) {
                if (descriptionType == (enum Upnp_DescType_e)UPNPREG_BUF_DESC) {
                        strncpy(aliasStr,
                                "description.xml",
                                sizeof(aliasStr) - 1);
                } else {
                        /* URL or filename */
                        retVal = GetNameForAlias(description, &temp_str);
                        if (retVal != UPNP_E_SUCCESS) {
                                ixmlDocument_free(*xmlDoc);
                                return retVal;
                        }
                        if (strlen(temp_str) > (LINE_SIZE - 1)) {
                                ixmlDocument_free(*xmlDoc);
                                return UPNP_E_URL_TOO_BIG;
                        }
                        strncpy(aliasStr, temp_str, sizeof(aliasStr) - 1);
                }
                if (AddressFamily == AF_INET) {
                        get_server_addr(p, (struct sockaddr *)&serverAddr);
                } else {
                        get_server_addr6(p, (struct sockaddr *)&serverAddr);
                }

                /* config */
                retVal = configure_urlbase(p,
                        *xmlDoc,
                        (struct sockaddr *)&serverAddr,
                        aliasStr,
                        last_modified,
                        descURL);
                if (retVal != UPNP_E_SUCCESS) {
                        ixmlDocument_free(*xmlDoc);
                        return retVal;
                }
        } else {
                /* Manual */
                if (strlen(description) >= LINE_SIZE) {
                        ixmlDocument_free(*xmlDoc);
                        return UPNP_E_URL_TOO_BIG;
                }
                strncpy(descURL, description, LINE_SIZE);
        }

        assert(*xmlDoc != NULL);

        return UPNP_E_SUCCESS;
}

#else /* INTERNAL_WEB_SERVER */ /* no web server */
static int GetDescDocumentAndURL(Upnp_DescType descriptionType,
        char *description,
        int config_baseURL,
        int AddressFamily,
        IXML_Document **xmlDoc,
        char descURL[LINE_SIZE])
{
        int retVal = 0;

        if (descriptionType != (enum Upnp_DescType_e)UPNPREG_URL_DESC ||
                config_baseURL) {
                return UPNP_E_NO_WEB_SERVER;
        }

        if (description == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        if (strlen(description) >= LINE_SIZE) {
                return UPNP_E_URL_TOO_BIG;
        }
        strncpy(descURL, description, LINE_SIZE);

        retVal = UpnpDownloadXmlDoc(description, xmlDoc);
        if (retVal != UPNP_E_SUCCESS) {
                return retVal;
        }

        return UPNP_E_SUCCESS;
}
#endif /* INTERNAL_WEB_SERVER */
#endif /* INCLUDE_DEVICE_APIS */

/*******************************************************************************
 *
 *                                  SSDP interface
 *
 ******************************************************************************/

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0
int UpnpSendAdvertisement(UpnpLib *p, UpnpDevice_Handle Hnd, int Exp)
{
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendAdvertisement \n");
        return UpnpSendAdvertisementLowPower(p, Hnd, Exp, -1, -1, -1);
}

int UpnpSendAdvertisementLowPower(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        int Exp,
        int PowerState,
        int SleepPeriod,
        int RegistrationState)
{
        struct Handle_Info *SInfo = NULL;
        int retVal = 0, *ptrMx;
        upnp_timeout *adEvent;
        ThreadPoolJob job = {};

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendAdvertisementLowPower \n");

        HandleLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (Exp < 1)
                Exp = DEFAULT_MAXAGE;
        if (Exp <= AUTO_ADVERTISEMENT_TIME * 2)
                Exp = (AUTO_ADVERTISEMENT_TIME + 1) * 2;
        SInfo->MaxAge = Exp;
        SInfo->PowerState = PowerState;
        if (SleepPeriod < 0)
                SleepPeriod = -1;
        SInfo->SleepPeriod = SleepPeriod;
        SInfo->RegistrationState = RegistrationState;
        HandleUnlock();
        retVal = AdvertiseAndReply(p,
                1,
                Hnd,
                (enum SsdpSearchType)0,
                (struct sockaddr *)NULL,
                (char *)NULL,
                (char *)NULL,
                (char *)NULL,
                Exp);

        if (retVal != UPNP_E_SUCCESS)
                return retVal;
        ptrMx = (int *)malloc(sizeof(int));
        if (ptrMx == NULL)
                return UPNP_E_OUTOF_MEMORY;
        adEvent = (upnp_timeout *)malloc(sizeof(upnp_timeout));

        if (adEvent == NULL) {
                free(ptrMx);
                return UPNP_E_OUTOF_MEMORY;
        }
        *ptrMx = Exp;
        adEvent->handle = Hnd;
        adEvent->Event = ptrMx;

        HandleLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                free(adEvent);
                free(ptrMx);
                return UPNP_E_INVALID_HANDLE;
        }
#ifdef SSDP_PACKET_DISTRIBUTE
        TPJobInit(&job, AutoAdvertise, adEvent);
        TPJobSetFreeFunction(&job, (free_routine)free_upnp_timeout);
        TPJobSetPriority(&job, MED_PRIORITY);
        if ((retVal = TimerThreadSchedule(UpnpLib_getnc_gTimerThread(p),
                     ((Exp / 2) - (AUTO_ADVERTISEMENT_TIME)),
                     REL_SEC,
                     &job,
                     SHORT_TERM,
                     &(adEvent->eventId))) != UPNP_E_SUCCESS) {
                HandleUnlock();
                free(adEvent);
                free(ptrMx);
                return retVal;
        }
#else
        TPJobInit(&job, (start_routine)AutoAdvertise, adEvent);
        TPJobSetFreeFunction(&job, (free_routine)free_upnp_timeout);
        TPJobSetPriority(&job, MED_PRIORITY);
        if ((retVal = TimerThreadSchedule(&gTimerThread,
                     Exp - AUTO_ADVERTISEMENT_TIME,
                     REL_SEC,
                     &job,
                     SHORT_TERM,
                     &(adEvent->eventId))) != UPNP_E_SUCCESS) {
                HandleUnlock();
                free(adEvent);
                free(ptrMx);
                return retVal;
        }
#endif

        HandleUnlock();
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSendAdvertisementLowPower \n");

        return retVal;
}
#endif /* EXCLUDE_SSDP == 0 */
#endif /* INCLUDE_DEVICE_APIS */

#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS

int UpnpSearchAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        int Mx,
        const char *Target_const,
        const void *Cookie_const)
{
        struct Handle_Info *SInfo = NULL;
        char *Target = (char *)Target_const;
        int retVal;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSearchAsync\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (Mx < 1)
                Mx = DEFAULT_MX;

        if (Target == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }

        HandleUnlock();
        retVal = SearchByTarget(p, Hnd, Mx, Target, (void *)Cookie_const);
        if (retVal != 1)
                return retVal;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSearchAsync \n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */
#endif

/*******************************************************************************
 *
 *                                  GENA interface
 *
 ******************************************************************************/

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptions(
        UpnpLib *p, UpnpDevice_Handle Hnd, int MaxSubscriptions)
{
        struct Handle_Info *SInfo = NULL;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSetMaxSubscriptions \n");

        HandleLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if ((MaxSubscriptions != UPNP_INFINITE) && (MaxSubscriptions < 0)) {
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        SInfo->MaxSubscriptions = MaxSubscriptions;
        HandleUnlock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSetMaxSubscriptions \n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpSetMaxSubscriptionTimeOut(
        UpnpLib *p, UpnpDevice_Handle Hnd, int MaxSubscriptionTimeOut)
{
        struct Handle_Info *SInfo = NULL;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSetMaxSubscriptionTimeOut\n");

        HandleLock();

        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if ((MaxSubscriptionTimeOut != UPNP_INFINITE) &&
                (MaxSubscriptionTimeOut < 0)) {
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }

        SInfo->MaxSubscriptionTimeOut = MaxSubscriptionTimeOut;
        HandleUnlock();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSetMaxSubscriptionTimeOut\n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpSubscribeAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *EvtUrl_const,
        int TimeOut,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;
        char *EvtUrl = (char *)EvtUrl_const;
        ThreadPoolJob job = {};

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSubscribeAsync\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (EvtUrl == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (TimeOut != UPNP_INFINITE && TimeOut < 1) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (Fun == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        HandleUnlock();

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));
        if (Param == NULL) {
                return UPNP_E_OUTOF_MEMORY;
        }

        Param->FunName = SUBSCRIBE;
        Param->Handle = Hnd;
        strncpy(Param->Url, EvtUrl, sizeof(Param->Url) - 1);
        Param->TimeOut = TimeOut;
        Param->Fun = Fun;
        Param->Cookie = (const char *)Cookie_const;

        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);
        TPJobSetPriority(&job, MED_PRIORITY);
        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSubscribeAsync\n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpSubscribe(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *EvtUrl_const,
        int *TimeOut,
        Upnp_SID SubsId)
{
        int retVal;
        struct Handle_Info *SInfo = NULL;
        UpnpString *EvtUrl = UpnpString_new();
        UpnpString *SubsIdTmp = UpnpString_new();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSubscribe\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }

        if (EvtUrl == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        if (EvtUrl_const == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        UpnpString_set_String(EvtUrl, EvtUrl_const);

        if (SubsIdTmp == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        if (SubsId == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        UpnpString_set_String(SubsIdTmp, SubsId);

        if (TimeOut == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                retVal = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        HandleUnlock();

        retVal = genaSubscribe(p, Hnd, EvtUrl, TimeOut, SubsIdTmp);
        *SubsId = {};
        strncpy(SubsId, UpnpString_get_String(SubsIdTmp), sizeof(Upnp_SID) - 1);

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSubscribe, retVal=%d\n",
                retVal);

        UpnpString_delete(SubsIdTmp);
        UpnpString_delete(EvtUrl);

        return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnSubscribe(UpnpLib *p, UpnpClient_Handle Hnd, const Upnp_SID SubsId)
{
        struct Handle_Info *SInfo = NULL;
        int retVal;
        UpnpString *SubsIdTmp = UpnpString_new();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpUnSubscribe\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }
        if (SubsIdTmp == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        if (SubsId == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        UpnpString_set_String(SubsIdTmp, SubsId);

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                retVal = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        HandleUnlock();

        retVal = genaUnSubscribe(p, Hnd, SubsIdTmp);

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpUnSubscribe, retVal=%d\n",
                retVal);

        UpnpString_delete(SubsIdTmp);

        return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpUnSubscribeAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        Upnp_SID SubsId,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        int retVal = UPNP_E_SUCCESS;
        ThreadPoolJob job = {};
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpUnSubscribeAsync\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }

        if (SubsId == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        if (Fun == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                retVal = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        HandleUnlock();

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));
        if (Param == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }

        Param->FunName = UNSUBSCRIBE;
        Param->Handle = Hnd;
        strncpy(Param->SubsId, SubsId, sizeof(Param->SubsId) - 1);
        Param->Fun = Fun;
        Param->Cookie = (const char *)Cookie_const;
        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);
        TPJobSetPriority(&job, MED_PRIORITY);
        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpUnSubscribeAsync\n");

        return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpRenewSubscription(
        UpnpLib *p, UpnpClient_Handle Hnd, int *TimeOut, const Upnp_SID SubsId)
{
        struct Handle_Info *SInfo = NULL;
        int retVal;
        UpnpString *SubsIdTmp = UpnpString_new();

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRenewSubscription\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                retVal = UPNP_E_FINISH;
                goto exit_function;
        }
        if (SubsIdTmp == NULL) {
                retVal = UPNP_E_OUTOF_MEMORY;
                goto exit_function;
        }
        if (SubsId == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        UpnpString_set_String(SubsIdTmp, SubsId);

        if (TimeOut == NULL) {
                retVal = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                retVal = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        HandleUnlock();

        retVal = genaRenewSubscription(p, Hnd, SubsIdTmp, TimeOut);

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpRenewSubscription, retVal=%d\n",
                retVal);

        UpnpString_delete(SubsIdTmp);

        return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int UpnpRenewSubscriptionAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        int TimeOut,
        Upnp_SID SubsId,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        ThreadPoolJob job = {};
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpRenewSubscriptionAsync\n");
        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (TimeOut != UPNP_INFINITE && TimeOut < 1) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (SubsId == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (Fun == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        HandleUnlock();

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));
        if (Param == NULL) {
                return UPNP_E_OUTOF_MEMORY;
        }

        Param->FunName = RENEW;
        Param->Handle = Hnd;
        strncpy(Param->SubsId, SubsId, sizeof(Param->SubsId) - 1);
        Param->Fun = Fun;
        Param->Cookie = (const char *)Cookie_const;
        Param->TimeOut = TimeOut;

        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);
        TPJobSetPriority(&job, MED_PRIORITY);
        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpRenewSubscriptionAsync\n");

        return UPNP_E_SUCCESS;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpNotify(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        const char *DevID_const,
        const char *ServName_const,
        const char **VarName_const,
        const char **NewVal_const,
        int cVariables)
{
        struct Handle_Info *SInfo = NULL;
        int retVal;
        char *DevID = (char *)DevID_const;
        char *ServName = (char *)ServName_const;
        char **VarName = (char **)VarName_const;
        char **NewVal = (char **)NewVal_const;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpNotify\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (DevID == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (ServName == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (VarName == NULL || NewVal == NULL || cVariables < 0) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }

        HandleUnlock();
        retVal = genaNotifyAll(
                p, Hnd, DevID, ServName, VarName, NewVal, cVariables);

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpNotify\n");

        return retVal;
}

int UpnpNotifyExt(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        const char *DevID_const,
        const char *ServName_const,
        IXML_Document *PropSet)
{
        struct Handle_Info *SInfo = NULL;
        int retVal;
        char *DevID = (char *)DevID_const;
        char *ServName = (char *)ServName_const;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpNotify \n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        if (DevID == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }
        if (ServName == NULL) {
                HandleUnlock();
                return UPNP_E_INVALID_PARAM;
        }

        HandleUnlock();
        retVal = genaNotifyAllExt(p, Hnd, DevID, ServName, PropSet);

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpNotify \n");

        return retVal;
}
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INCLUDE_DEVICE_APIS
int UpnpAcceptSubscription(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        const char *DevID_const,
        const char *ServName_const,
        const char **VarName_const,
        const char **NewVal_const,
        int cVariables,
        const Upnp_SID SubsId)
{
        int ret = 0;
        int line = 0;
        struct Handle_Info *SInfo = NULL;
        char *DevID = (char *)DevID_const;
        char *ServName = (char *)ServName_const;
        char **VarName = (char **)VarName_const;
        char **NewVal = (char **)NewVal_const;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpAcceptSubscription\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                line = __LINE__;
                ret = UPNP_E_FINISH;
                goto exit_function;
        }

        HandleReadLock();

        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        if (DevID == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        if (ServName == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        if (SubsId == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
/* Now accepts an empty state list, so the code below is
 * commented out */
#if 0
	if (VarName == NULL || NewVal == NULL || cVariables < 0) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
#endif

        HandleUnlock();

        line = __LINE__;
        ret = genaInitNotify(
                p, Hnd, DevID, ServName, VarName, NewVal, cVariables, SubsId);

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                line,
                "Exiting UpnpAcceptSubscription, ret = %d\n",
                ret);

        return ret;
}

int UpnpAcceptSubscriptionExt(UpnpLib *p,
        UpnpDevice_Handle Hnd,
        const char *DevID_const,
        const char *ServName_const,
        IXML_Document *PropSet,
        const Upnp_SID SubsId)
{
        int ret = 0;
        int line = 0;
        struct Handle_Info *SInfo = NULL;
        char *DevID = (char *)DevID_const;
        char *ServName = (char *)ServName_const;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpAcceptSubscription\n");

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                line = __LINE__;
                ret = UPNP_E_FINISH;
                goto exit_function;
        }

        HandleReadLock();

        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_DEVICE:
                break;
        default:
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_HANDLE;
                goto exit_function;
        }
        if (DevID == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        if (ServName == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
        if (SubsId == NULL) {
                HandleUnlock();
                line = __LINE__;
                ret = UPNP_E_INVALID_PARAM;
                goto exit_function;
        }
/* Now accepts an empty state list, so the code below is
 * commented out */
#if 0
	if (PropSet == NULL) {
		HandleUnlock();
		line = __LINE__;
		ret = UPNP_E_INVALID_PARAM;
		goto exit_function;
	}
#endif

        HandleUnlock();

        line = __LINE__;
        ret = genaInitNotifyExt(p, Hnd, DevID, ServName, PropSet, SubsId);

exit_function:
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                line,
                "Exiting UpnpAcceptSubscription, ret = %d.\n",
                ret);

        return ret;
}
#endif /* INCLUDE_DEVICE_APIS */
#endif /* EXCLUDE_GENA == 0 */

/*******************************************************************************
 *
 *                                  SOAP interface
 *
 ******************************************************************************/

#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS
int UpnpSendAction(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *ServiceType_const,
        const char *DevUDN_const,
        IXML_Document *Action,
        IXML_Document **RespNodePtr)
{
        struct Handle_Info *SInfo = NULL;
        int retVal = 0;
        char *ActionURL = (char *)ActionURL_const;
        char *ServiceType = (char *)ServiceType_const;
        /* udn not used? */
        /*char *DevUDN = (char *)DevUDN_const;*/

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendAction\n");
        if (DevUDN_const != NULL) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "non NULL DevUDN is ignored\n");
        }
        DevUDN_const = NULL;

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        if (ServiceType == NULL || Action == NULL || RespNodePtr == NULL ||
                DevUDN_const != NULL) {

                return UPNP_E_INVALID_PARAM;
        }

        retVal = SoapSendAction(p, ActionURL, ServiceType, Action, RespNodePtr);

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSendAction\n");

        return retVal;
}

int UpnpSendActionEx(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *ServiceType_const,
        const char *DevUDN_const,
        IXML_Document *Header,
        IXML_Document *Action,
        IXML_Document **RespNodePtr)
{
        struct Handle_Info *SInfo = NULL;
        int retVal = 0;
        char *ActionURL = (char *)ActionURL_const;
        char *ServiceType = (char *)ServiceType_const;
        /* udn not used? */
        /*char *DevUDN = (char *)DevUDN_const;*/

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendActionEx\n");

        if (Header == NULL) {
                retVal = UpnpSendAction(p,
                        Hnd,
                        ActionURL_const,
                        ServiceType_const,
                        DevUDN_const,
                        Action,
                        RespNodePtr);
                return retVal;
        }

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        if (ServiceType == NULL || Action == NULL || RespNodePtr == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        retVal = SoapSendActionEx(
                p, ActionURL, ServiceType, Header, Action, RespNodePtr);

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSendAction \n");

        return retVal;
}

int UpnpSendActionAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *ServiceType_const,
        const char *DevUDN_const,
        IXML_Document *Act,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        int rc;
        ThreadPoolJob job = {};
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;
        DOMString tmpStr;
        char *ActionURL = (char *)ActionURL_const;
        char *ServiceType = (char *)ServiceType_const;
        /* udn not used? */
        /*char *DevUDN = (char *)DevUDN_const;*/

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendActionAsync\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        if (ServiceType == NULL || Act == NULL || Fun == NULL ||
                DevUDN_const != NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        tmpStr = ixmlPrintNode((IXML_Node *)Act);
        if (tmpStr == NULL) {
                return UPNP_E_INVALID_ACTION;
        }

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));

        if (Param == NULL) {
                ixmlFreeDOMString(tmpStr);
                return UPNP_E_OUTOF_MEMORY;
        }

        Param->FunName = ACTION;
        Param->Handle = Hnd;
        strncpy(Param->Url, ActionURL, sizeof(Param->Url) - 1);
        strncpy(Param->ServiceType,
                ServiceType,
                sizeof(Param->ServiceType) - 1);

        rc = ixmlParseBufferEx(tmpStr, &(Param->Act));
        if (rc != IXML_SUCCESS) {
                free(Param);
                ixmlFreeDOMString(tmpStr);
                if (rc == IXML_INSUFFICIENT_MEMORY) {
                        return UPNP_E_OUTOF_MEMORY;
                } else {
                        return UPNP_E_INVALID_ACTION;
                }
        }
        ixmlFreeDOMString(tmpStr);
        Param->Cookie = (const char *)Cookie_const;
        Param->Fun = Fun;

        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);

        TPJobSetPriority(&job, MED_PRIORITY);
        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSendActionAsync \n");

        return UPNP_E_SUCCESS;
}

int UpnpSendActionExAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *ServiceType_const,
        const char *DevUDN_const,
        IXML_Document *Header,
        IXML_Document *Act,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;
        DOMString tmpStr;
        DOMString headerStr = NULL;
        char *ActionURL = (char *)ActionURL_const;
        char *ServiceType = (char *)ServiceType_const;
        ThreadPoolJob job = {};
        int retVal = 0;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpSendActionExAsync\n");

        if (Header == NULL) {
                retVal = UpnpSendActionAsync(p,
                        Hnd,
                        ActionURL_const,
                        ServiceType_const,
                        DevUDN_const,
                        Act,
                        Fun,
                        Cookie_const);
                return retVal;
        }

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        if (ServiceType == NULL || Act == NULL || Fun == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        headerStr = ixmlPrintNode((IXML_Node *)Header);

        tmpStr = ixmlPrintNode((IXML_Node *)Act);
        if (tmpStr == NULL) {
                ixmlFreeDOMString(headerStr);
                return UPNP_E_INVALID_ACTION;
        }

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));
        if (Param == NULL) {
                ixmlFreeDOMString(tmpStr);
                ixmlFreeDOMString(headerStr);
                return UPNP_E_OUTOF_MEMORY;
        }

        Param->FunName = ACTION;
        Param->Handle = Hnd;
        strncpy(Param->Url, ActionURL, sizeof(Param->Url) - 1);
        strncpy(Param->ServiceType,
                ServiceType,
                sizeof(Param->ServiceType) - 1);
        retVal = ixmlParseBufferEx(headerStr, &(Param->Header));
        if (retVal != IXML_SUCCESS) {
                free(Param);
                ixmlFreeDOMString(tmpStr);
                ixmlFreeDOMString(headerStr);
                if (retVal == IXML_INSUFFICIENT_MEMORY) {
                        return UPNP_E_OUTOF_MEMORY;
                } else {
                        return UPNP_E_INVALID_ACTION;
                }
        }

        retVal = ixmlParseBufferEx(tmpStr, &(Param->Act));
        if (retVal != IXML_SUCCESS) {
                ixmlDocument_free(Param->Header);
                free(Param);
                ixmlFreeDOMString(tmpStr);
                ixmlFreeDOMString(headerStr);
                if (retVal == IXML_INSUFFICIENT_MEMORY) {
                        return UPNP_E_OUTOF_MEMORY;
                } else {
                        return UPNP_E_INVALID_ACTION;
                }
        }

        ixmlFreeDOMString(tmpStr);
        ixmlFreeDOMString(headerStr);

        Param->Cookie = (const char *)Cookie_const;
        Param->Fun = Fun;

        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);

        TPJobSetPriority(&job, MED_PRIORITY);
        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpSendActionAsync\n");

        return UPNP_E_SUCCESS;
}

int UpnpGetServiceVarStatusAsync(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *VarName_const,
        Upnp_FunPtr Fun,
        const void *Cookie_const)
{
        ThreadPoolJob job = {};
        struct Handle_Info *SInfo = NULL;
        struct UpnpNonblockParam *Param;
        char *ActionURL = (char *)ActionURL_const;
        char *VarName = (char *)VarName_const;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpGetServiceVarStatusAsync\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }
        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        if (VarName == NULL || Fun == NULL)
                return UPNP_E_INVALID_PARAM;

        Param = (struct UpnpNonblockParam *)std::calloc(1,
                sizeof(struct UpnpNonblockParam));
        if (Param == NULL) {
                return UPNP_E_OUTOF_MEMORY;
        }

        Param->FunName = STATUS;
        Param->Handle = Hnd;
        strncpy(Param->Url, ActionURL, sizeof(Param->Url) - 1);
        strncpy(Param->VarName, VarName, sizeof(Param->VarName) - 1);
        Param->Fun = Fun;
        Param->Cookie = (const char *)Cookie_const;

        TPJobInit(&job, UpnpThreadDistribution, Param);
        TPJobSetFreeFunction(&job, (free_routine)free);

        TPJobSetPriority(&job, MED_PRIORITY);

        if (ThreadPoolAdd(UpnpLib_getnc_gSendThreadPool(p), &job, NULL) != 0) {
                free(Param);
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpGetServiceVarStatusAsync\n");

        return UPNP_E_SUCCESS;
}

int UpnpGetServiceVarStatus(UpnpLib *p,
        UpnpClient_Handle Hnd,
        const char *ActionURL_const,
        const char *VarName_const,
        DOMString *StVar)
{
        struct Handle_Info *SInfo = NULL;
        int retVal = 0;
        char *StVarPtr;
        char *ActionURL = (char *)ActionURL_const;
        char *VarName = (char *)VarName_const;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpGetServiceVarStatus\n");

        HandleReadLock();
        switch (GetHandleInfo(p, Hnd, &SInfo)) {
        case HND_CLIENT:
                break;
        default:
                HandleUnlock();
                return UPNP_E_INVALID_HANDLE;
        }

        HandleUnlock();

        if (ActionURL == NULL) {
                return UPNP_E_INVALID_PARAM;
        }
        if (VarName == NULL || StVar == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        retVal = SoapGetServiceVarStatus(p, ActionURL, VarName, &StVarPtr);
        *StVar = StVarPtr;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpGetServiceVarStatus \n");

        return retVal;
}
#endif /* INCLUDE_CLIENT_APIS */
#endif /* EXCLUDE_SOAP */

/*******************************************************************************
 *
 *                             Client API
 *
 ******************************************************************************/

int UpnpOpenHttpPost(UpnpLib *p,
        const char *url,
        void **handle,
        const char *contentType,
        int contentLength,
        int timeout)
{
        int status = http_OpenHttpConnection(p, url, handle, timeout);
        if (status == UPNP_E_SUCCESS) {
                return http_MakeHttpRequest(p,
                        UPNP_HTTPMETHOD_POST,
                        url,
                        handle,
                        NULL,
                        contentType,
                        contentLength,
                        timeout);
        }
        return status;
}

int UpnpWriteHttpPost(void *handle, char *buf, size_t *size, int timeout)
{
        return http_WriteHttpRequest(handle, buf, size, timeout);
}

int UpnpCloseHttpPost(UpnpLib *p, void *handle, int *httpStatus, int timeout)
{
        int status = http_EndHttpRequest(handle, timeout);
        if (status == UPNP_E_SUCCESS) {
                /* status = */ http_GetHttpResponse(
                        p, handle, NULL, NULL, NULL, httpStatus, timeout);
        }
        status = http_CloseHttpConnection(p, handle);
        return status;
}

int UpnpOpenHttpGet(UpnpLib *p,
        const char *url,
        void **handle,
        char **contentType,
        int *contentLength,
        int *httpStatus,
        int timeout)
{
        int status = UpnpOpenHttpConnection(p, url, handle, timeout);
        if (status == UPNP_E_SUCCESS) {
                status = UpnpMakeHttpRequest(p,
                        UPNP_HTTPMETHOD_GET,
                        url,
                        *handle,
                        NULL,
                        NULL,
                        0,
                        timeout);
        }
        if (status == UPNP_E_SUCCESS) {
                status = UpnpEndHttpRequest(*handle, timeout);
        }
        if (status == UPNP_E_SUCCESS) {
                status = UpnpGetHttpResponse(p,
                        *handle,
                        NULL,
                        contentType,
                        contentLength,
                        httpStatus,
                        timeout);
        }
        return status;
}

int UpnpOpenHttpGetProxy(UpnpLib *p,
        const char *url,
        const char *proxy_str,
        void **handle,
        char **contentType,
        int *contentLength,
        int *httpStatus,
        int timeout)
{
        int status = UpnpOpenHttpConnection(p, proxy_str, handle, timeout);
        if (status == UPNP_E_SUCCESS) {
                status = UpnpMakeHttpRequest(p,
                        UPNP_HTTPMETHOD_GET,
                        url,
                        *handle,
                        NULL,
                        NULL,
                        0,
                        timeout);
        }
        if (status == UPNP_E_SUCCESS) {
                status = UpnpEndHttpRequest(*handle, timeout);
        }
        if (status == UPNP_E_SUCCESS) {
                status = UpnpGetHttpResponse(p,
                        *handle,
                        NULL,
                        contentType,
                        contentLength,
                        httpStatus,
                        timeout);
        }
        return status;
}

int UpnpOpenHttpGetEx(UpnpLib *p,
        const char *url_str,
        void **Handle,
        char **contentType,
        int *contentLength,
        int *httpStatus,
        int lowRange,
        int highRange,
        int timeout)
{
        return http_OpenHttpGetEx(p,
                url_str,
                Handle,
                contentType,
                contentLength,
                httpStatus,
                lowRange,
                highRange,
                timeout);
}

int UpnpCancelHttpGet(void *Handle) { return http_CancelHttpGet(Handle); }

int UpnpCloseHttpGet(UpnpLib *p, void *Handle)
{
        return UpnpCloseHttpConnection(p, Handle);
}

int UpnpReadHttpGet(
        UpnpLib *p, void *Handle, char *buf, size_t *size, int timeout)
{
        return http_ReadHttpResponse(p, Handle, buf, size, timeout);
}

int UpnpHttpGetProgress(void *Handle, size_t *length, size_t *total)
{
        return http_HttpGetProgress(Handle, length, total);
}

int UpnpOpenHttpConnection(
        UpnpLib *p, const char *url, void **handle, int timeout)
{
        return http_OpenHttpConnection(p, url, handle, timeout);
}

int UpnpMakeHttpRequest(UpnpLib *p,
        Upnp_HttpMethod method,
        const char *url,
        void *handle,
        UpnpString *headers,
        const char *contentType,
        int contentLength,
        int timeout)
{
        return http_MakeHttpRequest(p,
                method,
                url,
                handle,
                headers,
                contentType,
                contentLength,
                timeout);
}

int UpnpWriteHttpRequest(void *handle, char *buf, size_t *size, int timeout)
{
        return http_WriteHttpRequest(handle, buf, size, timeout);
}

int UpnpEndHttpRequest(void *handle, int timeout)
{
        return http_EndHttpRequest(handle, timeout);
}

int UpnpGetHttpResponse(UpnpLib *p,
        void *handle,
        UpnpString *headers,
        char **contentType,
        int *contentLength,
        int *httpStatus,
        int timeout)
{
        return http_GetHttpResponse(p,
                handle,
                headers,
                contentType,
                contentLength,
                httpStatus,
                timeout);
}

int UpnpReadHttpResponse(
        UpnpLib *p, void *handle, char *buf, size_t *size, int timeout)
{
        return http_ReadHttpResponse(p, handle, buf, size, timeout);
}

int UpnpCloseHttpConnection(UpnpLib *p, void *handle)
{
        return http_CloseHttpConnection(p, handle);
}

int UpnpDownloadUrlItem(
        UpnpLib *p, const char *url, char **outBuf, char *contentType)
{
        int ret_code;
        size_t dummy;

        if (url == NULL || outBuf == NULL || contentType == NULL)
                return UPNP_E_INVALID_PARAM;
        ret_code = http_Download(
                p, url, HTTP_DEFAULT_TIMEOUT, outBuf, &dummy, contentType);
        if (ret_code > 0)
                /* error reply was received */
                ret_code = UPNP_E_INVALID_URL;

        return ret_code;
}

int UpnpDownloadXmlDoc(UpnpLib *p, const char *url, IXML_Document **xmlDoc)
{
        int ret_code;
        char *xml_buf;
        char content_type[LINE_SIZE];

        if (url == NULL || xmlDoc == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        ret_code = UpnpDownloadUrlItem(p, url, &xml_buf, content_type);
        if (ret_code != UPNP_E_SUCCESS) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "Error downloading document, retCode: %d\n",
                        ret_code);
                return ret_code;
        }

        if (strncasecmp(content_type, "text/xml", strlen("text/xml"))) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_INFO,
                        API,
                        __FILE__,
                        __LINE__,
                        "Not text/xml\n");
                /* Linksys WRT54G router returns
                 * "CONTENT-TYPE: application/octet-stream".
                 * Let's be nice to Linksys and try to parse document
                 * anyway. If the data sent is not a xml file,
                 * ixmlParseBufferEx will fail and the function will
                 * return UPNP_E_INVALID_DESC too. */
#if 0
		free(xml_buf);
		return UPNP_E_INVALID_DESC;
#endif
        }

        ret_code = ixmlParseBufferEx(xml_buf, xmlDoc);
        free(xml_buf);
        if (ret_code != IXML_SUCCESS) {
                if (ret_code == IXML_INSUFFICIENT_MEMORY) {
                        UpnpPrintf(UpnpLib_get_Log(p),
                                UPNP_CRITICAL,
                                API,
                                __FILE__,
                                __LINE__,
                                "Out of memory, ixml error code: %d\n",
                                ret_code);
                        return UPNP_E_OUTOF_MEMORY;
                } else {
                        UpnpPrintf(UpnpLib_get_Log(p),
                                UPNP_CRITICAL,
                                API,
                                __FILE__,
                                __LINE__,
                                "Invalid Description, ixml error code: "
                                "%d\n",
                                ret_code);
                        return UPNP_E_INVALID_DESC;
                }
        } else {
#ifdef DEBUG
                xml_buf = ixmlPrintNode((IXML_Node *)*xmlDoc);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "Printing the Parsed xml document \n %s\n",
                        xml_buf);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "****************** END OF Parsed XML Doc "
                        "*****************\n");
                ixmlFreeDOMString(xml_buf);
#endif
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "Exiting UpnpDownloadXmlDoc\n");

                return UPNP_E_SUCCESS;
        }
}

/*!
 * \brief Computes prefix length from IPv6 netmask.
 *
 * \return The IPv6 prefix length.
 */
static unsigned UpnpComputeIpv6PrefixLength(struct sockaddr_in6 *Netmask)
{
        unsigned prefix_length = 0;
        size_t i = 0;

        if (Netmask == NULL) {
                return prefix_length;
        }
        for (i = 0; i < sizeof(Netmask->sin6_addr); i++) {
                while (Netmask->sin6_addr.s6_addr[i]) {
                        prefix_length += (Netmask->sin6_addr.s6_addr[i] & 0x01);
                        Netmask->sin6_addr.s6_addr[i] >>= 1;
                }
        }

        return prefix_length;
}

int UpnpGetIfInfo(UpnpLib *p, const char *IfName)
{
#ifdef _WIN32
        /* ---------------------------------------------------- */
        /* WIN32 implementation will use the IpHlpAPI library.  */
        /* ---------------------------------------------------- */
        PIP_ADAPTER_ADDRESSES adapts = NULL;
        PIP_ADAPTER_ADDRESSES adapts_item;
        PIP_ADAPTER_UNICAST_ADDRESS uni_addr;
        SOCKADDR *ip_addr;
        struct in_addr v4_addr;
        struct in6_addr v6_addr;
        ULONG adapts_sz = 0;
        ULONG ret;
        int ifname_found = 0;
        int valid_addr_found = 0;
        char inet_addr4[INET_ADDRSTRLEN];
        char inet_addr6[INET6_ADDRSTRLEN];

        /* Get Adapters addresses required size. */
        ret = GetAdaptersAddresses(AF_UNSPEC,
                GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
                NULL,
                adapts,
                &adapts_sz);
        if (ret != ERROR_BUFFER_OVERFLOW) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "GetAdaptersAddresses failed to find list of "
                        "adapters\n");
                return UPNP_E_INIT;
        }
        /* Allocate enough memory. */
        adapts = (PIP_ADAPTER_ADDRESSES)malloc(adapts_sz);
        if (adapts == NULL) {
                return UPNP_E_OUTOF_MEMORY;
        }
        /* Do the call that will actually return the info. */
        ret = GetAdaptersAddresses(AF_UNSPEC,
                GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
                NULL,
                adapts,
                &adapts_sz);
        if (ret != ERROR_SUCCESS) {
                free(adapts);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "GetAdaptersAddresses failed to find list of "
                        "adapters\n");
                return UPNP_E_INIT;
        }
        /* Copy interface name, if it was provided. */
        if (IfName) {
                if (strlen(IfName) >= LINE_SIZE) {
                        free(adapts);
                        return UPNP_E_INVALID_INTERFACE;
                }
                UpnpLib_strcpy_gIF_NAME(p, IfName);
                ifname_found = 1;
        }
        for (adapts_item = adapts; adapts_item != NULL;
                adapts_item = adapts_item->Next) {
                if (adapts_item->Flags & IP_ADAPTER_NO_MULTICAST ||
                        adapts_item->OperStatus != IfOperStatusUp) {
                        continue;
                }
                /*
                 * Partial fix for Windows: Friendly name is wchar
                 * string, but currently p->gIF_NAME is char string. For
                 * now try to convert it, which will work with many (but
                 * not all) adapters. A full fix would require a lot of
                 * big changes (p->gIF_NAME to wchar string?).
                 */
                if (!ifname_found) {
                        /* We have found a valid interface name. Keep
                         * it. */
                        char tmpIfName[LINE_SIZE];
                        wcstombs(tmpIfName,
                                adapts_item->FriendlyName,
                                LINE_SIZE);
                        UpnpLib_strcpy_gIF_NAME(p, tmpIfName);
                        ifname_found = 1;
                } else {
                        char tmpIfName[LINE_SIZE] = {0};
                        wcstombs(tmpIfName,
                                adapts_item->FriendlyName,
                                sizeof(tmpIfName));
                        if (strncmp(UpnpLib_get_gIF_NAME_cstr(p),
                                    tmpIfName,
                                    LINE_SIZE) != 0) {
                                /* This is not the interface we're
                                 * looking for.
                                 */
                                continue;
                        }
                }
                /* Loop thru this adapter's unicast IP addresses. */
                uni_addr = adapts_item->FirstUnicastAddress;
                while (uni_addr) {
                        ip_addr = uni_addr->Address.lpSockaddr;
                        switch (ip_addr->sa_family) {
                        case AF_INET:
                                memcpy(&v4_addr,
                                        &((struct sockaddr_in *)ip_addr)
                                                 ->sin_addr,
                                        sizeof(v4_addr));
                                /* TODO: Retrieve IPv4 netmask */
                                valid_addr_found = 1;
                                break;
                        case AF_INET6:
                                /* TODO: Retrieve IPv6 ULA or GUA
                                 * address and its prefix */
                                /* Only keep IPv6 link-local addresses.
                                 */
                                if (IN6_IS_ADDR_LINKLOCAL(
                                            &((struct sockaddr_in6 *)ip_addr)
                                                     ->sin6_addr)) {
                                        memcpy(&v6_addr,
                                                &((struct sockaddr_in6 *)
                                                                ip_addr)
                                                         ->sin6_addr,
                                                sizeof(v6_addr));
                                        /* TODO: Retrieve IPv6 LLA
                                         * prefix */
                                        valid_addr_found = 1;
                                }
                                break;
                        default:
                                if (valid_addr_found == 0) {
                                        /* Address is not IPv4 or IPv6
                                         * and no valid address has  */
                                        /* yet been found for this
                                         * interface.
                                         * Discard interface name. */
                                        ifname_found = 0;
                                }
                                break;
                        }
                        /* Next address. */
                        uni_addr = uni_addr->Next;
                }
                if (valid_addr_found == 1) {
                        UpnpLib_set_gIF_INDEX(p, adapts_item->IfIndex);
                        break;
                }
        }
        free(adapts);
        /* Failed to find a valid interface, or valid address. */
        if (ifname_found == 0 || valid_addr_found == 0) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "Failed to find an adapter with valid IP "
                        "addresses for "
                        "use.\n");
                return UPNP_E_INVALID_INTERFACE;
        }
        inet_ntop(AF_INET, &v4_addr, inet_addr4, sizeof inet_addr4);
        UpnpLib_strcpy_gIF_IPV4(p, inet_addr4);
        inet_ntop(AF_INET6, &v6_addr, inet_addr6, sizeof inet_addr6);
        UpnpLib_strcpy_gIF_IPV6(p, inet_addr6);
#else
        struct ifaddrs *ifap, *ifa;
        struct in_addr v4_addr = {0};
        struct in_addr v4_netmask = {0};
        struct in6_addr v6_addr = IN6ADDR_ANY_INIT;
        struct in6_addr v6ulagua_addr = IN6ADDR_ANY_INIT;
        unsigned v6_prefix = 0;
        unsigned v6ulagua_prefix = 0;
        int ifname_found = 0;
        int valid_v4_addr_found = 0;
        int valid_v6_addr_found = 0;
        int valid_v6ulagua_addr_found = 0;
        char inet_addr4[INET_ADDRSTRLEN];
        char inet_addr4_netmask[INET_ADDRSTRLEN];
        char inet_addr6[INET6_ADDRSTRLEN];
        char inet_addr6_ula_gua[INET6_ADDRSTRLEN];

        /* Copy interface name, if it was provided. */
        if (IfName) {
                if (strlen(IfName) >= MAX_IF_NAME_SIZE) {
                        return UPNP_E_INVALID_INTERFACE;
                }
                /* strncpy() zero fills the remaining destination bytes.
                 */
                UpnpLib_strcpy_gIF_NAME(p, IfName);
                ifname_found = 1;
        }
        /* Get system interface addresses. */
        if (getifaddrs(&ifap) != 0) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "getifaddrs failed to find list of "
                        "addresses\n");
                return UPNP_E_INIT;
        }
        /* cycle through available interfaces and their addresses. */
        for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
                /* Skip LOOPBACK interfaces, DOWN interfaces, */
                /* interfaces without address (e.g. bonded)*/
                /* and interfaces that don't support MULTICAST. */
                if (!ifa->ifa_addr || (ifa->ifa_flags & IFF_LOOPBACK) ||
                        (!(ifa->ifa_flags & IFF_UP)) ||
                        (!(ifa->ifa_flags & IFF_MULTICAST))) {
                        continue;
                }
                if (!ifname_found) {
                        if (strlen(ifa->ifa_name) < MAX_IF_NAME_SIZE) {
                                /* We have found a valid interface name.
                                 * Keep it. */
                                UpnpLib_strcpy_gIF_NAME(p, ifa->ifa_name);
                                ifname_found = 1;
                        }
                } else {
                        if (strncmp(UpnpLib_get_gIF_NAME_cstr(p),
                                    ifa->ifa_name,
                                    MAX_IF_NAME_SIZE) != 0) {
                                /* Not the interface we're looking for.
                                 */
                                continue;
                        }
                }
                /* Keep interface addresses for later. */
                switch (ifa->ifa_addr->sa_family) {
                case AF_INET:
                        memcpy(&v4_addr,
                                &((struct sockaddr_in *)(ifa->ifa_addr))
                                         ->sin_addr,
                                sizeof(v4_addr));
                        memcpy(&v4_netmask,
                                &((struct sockaddr_in *)(ifa->ifa_netmask))
                                         ->sin_addr,
                                sizeof(v4_netmask));
                        valid_v4_addr_found = 1;
                        break;
                case AF_INET6:
                        if (IN6_IS_ADDR_ULA(
                                    &((struct sockaddr_in6 *)(ifa->ifa_addr))
                                             ->sin6_addr)) {
                                /* Got valid IPv6 ula. */
                                memcpy(&v6ulagua_addr,
                                        &((struct sockaddr_in6
                                                          *)(ifa->ifa_addr))
                                                 ->sin6_addr,
                                        sizeof(v6ulagua_addr));
                                v6ulagua_prefix = UpnpComputeIpv6PrefixLength(
                                        (struct sockaddr_in6
                                                        *)(ifa->ifa_netmask));
                                valid_v6ulagua_addr_found = 1;
                        } else if (IN6_IS_ADDR_GLOBAL(
                                           &((struct sockaddr_in6
                                                             *)(ifa->ifa_addr))
                                                    ->sin6_addr) &&
                                UpnpLib_get_gIF_IPV6_ULA_GUA_Length(p) == 0) {
                                /* got a GUA, should store it
                                 * while no ULA is found */
                                memcpy(&v6ulagua_addr,
                                        &((struct sockaddr_in6
                                                          *)(ifa->ifa_addr))
                                                 ->sin6_addr,
                                        sizeof(v6ulagua_addr));
                                v6ulagua_prefix = UpnpComputeIpv6PrefixLength(
                                        (struct sockaddr_in6
                                                        *)(ifa->ifa_netmask));
                                valid_v6ulagua_addr_found = 1;
                        } else if (IN6_IS_ADDR_LINKLOCAL(
                                           &((struct sockaddr_in6
                                                             *)(ifa->ifa_addr))
                                                    ->sin6_addr)) {
                                memcpy(&v6_addr,
                                        &((struct sockaddr_in6
                                                          *)(ifa->ifa_addr))
                                                 ->sin6_addr,
                                        sizeof(v6_addr));
                                v6_prefix = UpnpComputeIpv6PrefixLength(
                                        (struct sockaddr_in6
                                                        *)(ifa->ifa_netmask));
                                valid_v6_addr_found = 1;
                        }
                        break;
                default:
                        if (IfName == NULL && valid_v4_addr_found == 0 &&
                                valid_v6ulagua_addr_found == 0 &&
                                valid_v6_addr_found == 0) {
                                /* Address is not IPv4 or IPv6 and no
                                 * valid address has  */
                                /* yet been found for this interface.
                                 * Discard interface name. */
                                ifname_found = 0;
                        }
                        break;
                }
        }
        freeifaddrs(ifap);
        /* Failed to find a valid interface, or valid address. */
        if (ifname_found == 0 ||
                (valid_v4_addr_found == 0 && valid_v6_addr_found == 0 &&
                        valid_v6ulagua_addr_found == 0)) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_CRITICAL,
                        API,
                        __FILE__,
                        __LINE__,
                        "Failed to find an adapter with valid IP "
                        "addresses for "
                        "use.\n");
                return UPNP_E_INVALID_INTERFACE;
        }
        if (valid_v4_addr_found) {
                if (inet_ntop(
                            AF_INET, &v4_addr, inet_addr4, sizeof inet_addr4)) {
                        UpnpLib_strcpy_gIF_IPV4(p, inet_addr4);
                } else {
                        return UPNP_E_INVALID_INTERFACE;
                }
                if (inet_ntop(AF_INET,
                            &v4_netmask,
                            inet_addr4_netmask,
                            sizeof inet_addr4_netmask)) {
                        UpnpLib_strcpy_gIF_IPV4_NETMASK(p, inet_addr4_netmask);
                } else {
                        return UPNP_E_INVALID_INTERFACE;
                }
        }
        UpnpLib_set_gIF_INDEX(p, if_nametoindex(UpnpLib_get_gIF_NAME_cstr(p)));
        if (!IN6_IS_ADDR_UNSPECIFIED(&v6_addr)) {
                if (valid_v6_addr_found) {
                        if (inet_ntop(AF_INET6,
                                    &v6_addr,
                                    inet_addr6,
                                    sizeof inet_addr6)) {
                                UpnpLib_strcpy_gIF_IPV6(p, inet_addr6);
                                UpnpLib_set_gIF_IPV6_PREFIX_LENGTH(
                                        p, v6_prefix);
                        } else {
                                return UPNP_E_INVALID_INTERFACE;
                        }
                }
                if (valid_v6ulagua_addr_found) {
                        if (inet_ntop(AF_INET6,
                                    &v6ulagua_addr,
                                    inet_addr6_ula_gua,
                                    sizeof inet_addr6_ula_gua)) {
                                UpnpLib_strcpy_gIF_IPV6_ULA_GUA(
                                        p, inet_addr6_ula_gua);
                                UpnpLib_set_gIF_IPV6_ULA_GUA_PREFIX_LENGTH(
                                        p, v6ulagua_prefix);
                        } else {
                                return UPNP_E_INVALID_INTERFACE;
                        }
                }
        }
#endif
        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_INFO,
                API,
                __FILE__,
                __LINE__,
                "Interface name=%s, index=%d, v4=%s, v6=%s, ULA or GUA "
                "v6=%s\n",
                UpnpLib_get_gIF_NAME_cstr(p),
                UpnpLib_get_gIF_INDEX(p),
                UpnpLib_get_gIF_IPV4_cstr(p),
                UpnpLib_get_gIF_IPV6_cstr(p),
                UpnpLib_get_gIF_IPV6_ULA_GUA_cstr(p));

        return UPNP_E_SUCCESS;
}

/*!
 * \brief Schedule async functions in threadpool.
 */
#ifdef INCLUDE_CLIENT_APIS
void UpnpThreadDistribution(UpnpLib *p, void *in)
{
        int errCode = 0;
        struct UpnpNonblockParam *Param = (struct UpnpNonblockParam *)in;

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Inside UpnpThreadDistribution \n");

        switch (Param->FunName) {
#if EXCLUDE_GENA == 0
        case SUBSCRIBE: {
                UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
                UpnpString *Sid = UpnpString_new();

                UpnpEventSubscribe_strcpy_PublisherUrl(evt, Param->Url);
                errCode = genaSubscribe(p,
                        Param->Handle,
                        UpnpEventSubscribe_get_PublisherUrl(evt),
                        (int *)&Param->TimeOut,
                        Sid);
                UpnpEventSubscribe_set_ErrCode(evt, errCode);
                UpnpEventSubscribe_set_TimeOut(evt, Param->TimeOut);
                UpnpEventSubscribe_set_SID(evt, Sid);
                Param->Fun(
                        p, UPNP_EVENT_SUBSCRIBE_COMPLETE, evt, Param->Cookie);
                UpnpString_delete(Sid);
                UpnpEventSubscribe_delete(evt);
                free(Param);
                break;
        }
        case UNSUBSCRIBE: {
                UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
                UpnpEventSubscribe_strcpy_SID(evt, Param->SubsId);
                errCode = genaUnSubscribe(
                        p, Param->Handle, UpnpEventSubscribe_get_SID(evt));
                UpnpEventSubscribe_set_ErrCode(evt, errCode);
                UpnpEventSubscribe_strcpy_PublisherUrl(evt, "");
                UpnpEventSubscribe_set_TimeOut(evt, 0);
                Param->Fun(
                        p, UPNP_EVENT_UNSUBSCRIBE_COMPLETE, evt, Param->Cookie);
                UpnpEventSubscribe_delete(evt);
                free(Param);
                break;
        }
        case RENEW: {
                UpnpEventSubscribe *evt = UpnpEventSubscribe_new();
                UpnpEventSubscribe_strcpy_SID(evt, Param->SubsId);
                errCode = genaRenewSubscription(p,
                        Param->Handle,
                        UpnpEventSubscribe_get_SID(evt),
                        &Param->TimeOut);
                UpnpEventSubscribe_set_ErrCode(evt, errCode);
                UpnpEventSubscribe_set_TimeOut(evt, Param->TimeOut);
                Param->Fun(p, UPNP_EVENT_RENEWAL_COMPLETE, evt, Param->Cookie);
                UpnpEventSubscribe_delete(evt);
                free(Param);
                break;
        }
#endif /* EXCLUDE_GENA == 0 */
#if EXCLUDE_SOAP == 0
        case ACTION: {
                UpnpActionComplete *Evt = UpnpActionComplete_new();
                IXML_Document *actionResult = NULL;
                int errCode = SoapSendAction(p,
                        Param->Url,
                        Param->ServiceType,
                        Param->Act,
                        &actionResult);
                UpnpActionComplete_set_ErrCode(Evt, errCode);
                UpnpActionComplete_set_ActionRequest(Evt, Param->Act);
                UpnpActionComplete_set_ActionResult(Evt, actionResult);
                UpnpActionComplete_strcpy_CtrlUrl(Evt, Param->Url);
                Param->Fun(p, UPNP_CONTROL_ACTION_COMPLETE, Evt, Param->Cookie);
                free(Param);
                UpnpActionComplete_delete(Evt);
                break;
        }
        case STATUS: {
                UpnpStateVarComplete *Evt = UpnpStateVarComplete_new();
                DOMString currentVal = NULL;
                int errCode = SoapGetServiceVarStatus(
                        p, Param->Url, Param->VarName, &currentVal);
                UpnpStateVarComplete_set_ErrCode(Evt, errCode);
                UpnpStateVarComplete_strcpy_CtrlUrl(Evt, Param->Url);
                UpnpStateVarComplete_strcpy_StateVarName(Evt, Param->VarName);
                UpnpStateVarComplete_set_CurrentVal(Evt, currentVal);
                Param->Fun(
                        p, UPNP_CONTROL_GET_VAR_COMPLETE, Evt, Param->Cookie);
                free(Param);
                UpnpStateVarComplete_delete(Evt);
                break;
        }
#endif /* EXCLUDE_SOAP == 0 */
        default:
                break;
        }

        UpnpPrintf(UpnpLib_get_Log(p),
                UPNP_DEBUG,
                API,
                __FILE__,
                __LINE__,
                "Exiting UpnpThreadDistribution\n");
}
#endif /* INCLUDE_CLIENT_APIS */

/* Assumes at most one client */
Upnp_Handle_Type GetClientHandleInfo(UpnpLib *p,
        UpnpClient_Handle *client_handle_out,
        struct Handle_Info **HndInfo)
{
        UpnpClient_Handle client;

        for (client = 1; client < HANDLE_TABLE_MAX_NUM_ELEMENTS; client++) {
                switch (GetHandleInfo(p, client, HndInfo)) {
                case HND_CLIENT:
                        *client_handle_out = client;
                        return HND_CLIENT;
                        break;
                default:
                        break;
                }
        }

        *client_handle_out = -1;
        return HND_INVALID;
}

Upnp_Handle_Type GetDeviceHandleInfo(UpnpLib *p,
        UpnpDevice_Handle start,
        int AddressFamily,
        UpnpDevice_Handle *device_handle_out,
        struct Handle_Info **HndInfo)
{
#ifdef INCLUDE_DEVICE_APIS
        /* Check if we've got a registered device of the address family
         * specified. */
        if ((AddressFamily == AF_INET &&
                    !UpnpLib_get_UpnpSdkDeviceRegisteredV4(p)) ||
                (AddressFamily == AF_INET6 &&
                        !UpnpLib_get_UpnpSdkDeviceRegisteredV6(p))) {
                *device_handle_out = -1;
                return HND_INVALID;
        }
        if (start < 0 || start >= HANDLE_TABLE_MAX_NUM_ELEMENTS - 1) {
                *device_handle_out = -1;
                return HND_INVALID;
        }
        ++start;
        /* Find it. */
        for (*device_handle_out = start;
                *device_handle_out < HANDLE_TABLE_MAX_NUM_ELEMENTS;
                (*device_handle_out)++) {
                switch (GetHandleInfo(p, *device_handle_out, HndInfo)) {
                case HND_DEVICE:
                        if ((*HndInfo)->DeviceAf == AddressFamily) {
                                return HND_DEVICE;
                        }
                        break;
                default:
                        break;
                }
        }
#endif /* INCLUDE_DEVICE_APIS */

        *device_handle_out = -1;
        return HND_INVALID;
}

Upnp_Handle_Type GetDeviceHandleInfoForPath(UpnpLib *p,
        const char *path,
        int AddressFamily,
        UpnpDevice_Handle *device_handle_out,
        struct Handle_Info **HndInfo,
        service_info **serv_info)
{
#ifdef INCLUDE_DEVICE_APIS
        /* Check if we've got a registered device of the address family
         * specified. */
        if ((AddressFamily == AF_INET &&
                    !UpnpLib_get_UpnpSdkDeviceRegisteredV4(p)) ||
                (AddressFamily == AF_INET6 &&
                        !UpnpLib_get_UpnpSdkDeviceRegisteredV6(p))) {
                *device_handle_out = -1;
                return HND_INVALID;
        }
        /* Find it. */
        for (*device_handle_out = 1;
                *device_handle_out < HANDLE_TABLE_MAX_NUM_ELEMENTS;
                (*device_handle_out)++) {
                switch (GetHandleInfo(p, *device_handle_out, HndInfo)) {
                case HND_DEVICE:
                        if ((*HndInfo)->DeviceAf == AddressFamily) {
                                if ((*serv_info = FindServiceControlURLPath(p,
                                             &(*HndInfo)->ServiceTable,
                                             path)) ||
                                        (*serv_info = FindServiceEventURLPath(p,
                                                 &(*HndInfo)->ServiceTable,
                                                 path))) {
                                        return HND_DEVICE;
                                }
                        }
                        break;
                default:
                        break;
                }
        }
#endif /* INCLUDE_DEVICE_APIS */

        *device_handle_out = -1;
        return HND_INVALID;
}

Upnp_Handle_Type GetHandleInfo(
        UpnpLib *p, UpnpClient_Handle Hnd, struct Handle_Info **HndInfo)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        Upnp_Handle_Type ret = HND_INVALID;
        Upnp_LogLevel logLevel = UPNP_DEBUG;

        UpnpPrintf(UpnpLib_get_Log(p),
                logLevel,
                API,
                __FILE__,
                __LINE__,
                "GetHandleInfo: entering, Handle is %d\n",
                Hnd);

        if (Hnd < 1 || Hnd >= HANDLE_TABLE_MAX_NUM_ELEMENTS) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        logLevel,
                        API,
                        __FILE__,
                        __LINE__,
                        "GetHandleInfo: Handle out of range\n");
                goto ExitFunction;
        }
        if (!HandleTable->handle[Hnd]) {
                UpnpPrintf(UpnpLib_get_Log(p),
                        logLevel,
                        API,
                        __FILE__,
                        __LINE__,
                        "GetHandleInfo: HandleTable[%d] is NULL\n",
                        Hnd);
                goto ExitFunction;
        }
        *HndInfo = (struct Handle_Info *)HandleTable->handle[Hnd];
        ret = ((struct Handle_Info *)*HndInfo)->HType;

ExitFunction:
        UpnpPrintf(UpnpLib_get_Log(p),
                logLevel,
                API,
                __FILE__,
                __LINE__,
                "GetHandleInfo: exiting\n");

        return ret;
}

int PrintHandleInfo(UpnpLib *p, UpnpClient_Handle Hnd)
{
        handle_table_t *HandleTable = UpnpLib_getnc_HandleTable(p);
        struct Handle_Info *HndInfo;

        if (HandleTable->handle[Hnd]) {
                HndInfo = HandleTable->handle[Hnd];
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "Printing information for Handle_%d\n",
                        Hnd);
                UpnpPrintf(UpnpLib_get_Log(p),
                        UPNP_DEBUG,
                        API,
                        __FILE__,
                        __LINE__,
                        "HType_%d\n",
                        HndInfo->HType);
#ifdef INCLUDE_DEVICE_APIS
                switch (HndInfo->HType) {
                case HND_CLIENT:
                        break;
                default:
                        UpnpPrintf(UpnpLib_get_Log(p),
                                UPNP_DEBUG,
                                API,
                                __FILE__,
                                __LINE__,
                                "DescURL_%s\n",
                                HndInfo->DescURL);
                }
#endif /* INCLUDE_DEVICE_APIS */
        } else {
                return UPNP_E_INVALID_HANDLE;
        }

        return UPNP_E_SUCCESS;
}

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0
void AutoAdvertise(UpnpLib *p, void *input)
{
        upnp_timeout *event = (upnp_timeout *)input;

        UpnpSendAdvertisement(p, event->handle, *((int *)event->Event));
        free_upnp_timeout(event);
}
#endif /* EXCLUDE_SSDP == 0 */
#endif /* INCLUDE_DEVICE_APIS */

#ifdef INTERNAL_WEB_SERVER
int UpnpSetWebServerRootDir(UpnpLib *p, const char *rootDir)
{
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
        if ((rootDir == NULL) || (strlen(rootDir) == 0)) {
                return UPNP_E_INVALID_PARAM;
        }
        membuffer_destroy(UpnpLib_getnc_gDocumentRootDir(p));

        return web_server_set_root_dir(p, rootDir);
}
#endif /* INTERNAL_WEB_SERVER */

int UpnpAddVirtualDir(UpnpLib *p,
        const char *newDirName,
        const void *cookie,
        const void **oldcookie)
{
        virtualDirList *pNewVirtualDir;
        virtualDirList *pLast;
        virtualDirList *pCurVirtualDir;
        char dirName[NAME_SIZE] = {};
        virtualDirList *pVirtualDirList = UpnpLib_get_pVirtualDirList(p);

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                /* SDK is not initialized */
                return UPNP_E_FINISH;
        }
        if (!newDirName || !strlen(newDirName)) {
                return UPNP_E_INVALID_PARAM;
        }
        if (*newDirName != '/') {
                if (strlen(newDirName) >= sizeof(dirName) - 1) {
                        return UPNP_E_INVALID_PARAM;
                }
                dirName[0] = '/';
                strncpy(dirName + 1, newDirName, sizeof(dirName) - 1);
        } else {
                if (strlen(newDirName) >= sizeof(dirName)) {
                        return UPNP_E_INVALID_PARAM;
                }
                strncpy(dirName, newDirName, sizeof(dirName));
        }
        /* dirName is now properly filled. All .dirName fields have the
         * same size, so strncpy() properly zero fills everything. */

        pCurVirtualDir = pVirtualDirList;
        while (pCurVirtualDir) {
                /* already has this entry */
                if (strcmp(pCurVirtualDir->dirName, dirName) == 0) {
                        if (oldcookie) {
                                *oldcookie = pCurVirtualDir->cookie;
                        }
                        pCurVirtualDir->cookie = cookie;
                        return UPNP_E_SUCCESS;
                }
                pCurVirtualDir = pCurVirtualDir->next;
        }

        pNewVirtualDir = (virtualDirList *)malloc(sizeof(virtualDirList));
        if (!pNewVirtualDir) {
                return UPNP_E_OUTOF_MEMORY;
        }
        pNewVirtualDir->next = NULL;
        if (oldcookie) {
                *oldcookie = NULL;
        }
        pNewVirtualDir->cookie = cookie;
        strncpy(pNewVirtualDir->dirName,
                dirName,
                sizeof(pNewVirtualDir->dirName));
        if (!pVirtualDirList) { /* first virtual dir */
                pVirtualDirList = pNewVirtualDir;
        } else {
                pLast = pVirtualDirList;
                while (pLast->next) {
                        pLast = pLast->next;
                }
                pLast->next = pNewVirtualDir;
        }
        UpnpLib_set_pVirtualDirList(p, pVirtualDirList);

        return UPNP_E_SUCCESS;
}

int UpnpRemoveVirtualDir(UpnpLib *p, const char *dirName)
{
        virtualDirList *pPrev;
        virtualDirList *pCur;
        int found = 0;
        virtualDirList *pVirtualDirList;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }

        if (dirName == NULL) {
                return UPNP_E_INVALID_PARAM;
        }

        pVirtualDirList = UpnpLib_get_pVirtualDirList(p);
        if (!pVirtualDirList) {
                return UPNP_E_INVALID_PARAM;
        }
        /* Handle the special case where the directory that we are */
        /* removing is the first in the list. */
        if (strcmp(pVirtualDirList->dirName, dirName) == 0) {
                pPrev = pVirtualDirList;
                pVirtualDirList = pVirtualDirList->next;
                UpnpLib_set_pVirtualDirList(p, pVirtualDirList);
                free(pPrev);
                return UPNP_E_SUCCESS;
        }

        pCur = pVirtualDirList->next;
        pPrev = pVirtualDirList;

        while (pCur != NULL) {
                if (strcmp(pCur->dirName, dirName) == 0) {
                        pPrev->next = pCur->next;
                        free(pCur);
                        found = 1;
                        break;
                } else {
                        pPrev = pCur;
                        pCur = pCur->next;
                }
        }

        if (found == 1) {
                return UPNP_E_SUCCESS;
        } else {
                return UPNP_E_INVALID_PARAM;
        }
}

void UpnpRemoveAllVirtualDirs(UpnpLib *p)
{
        virtualDirList *pCur;
        virtualDirList *pNext;

        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return;
        }

        pCur = UpnpLib_get_pVirtualDirList(p);
        while (pCur != NULL) {
                pNext = pCur->next;
                free(pCur);

                pCur = pNext;
        }
        UpnpLib_set_pVirtualDirList(p, 0);
}

int UpnpEnableWebserver(UpnpLib *p, int enable)
{
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return UPNP_E_FINISH;
        }
#ifdef INTERNAL_WEB_SERVER
        if (enable) {
                int retVal = web_server_init(p);
                if (retVal != UPNP_E_SUCCESS) {
                        return retVal;
                }
                UpnpLib_set_bWebServerState(p, WEB_SERVER_ENABLED);
                SetHTTPGetCallback(web_server_callback);
        } else {
                web_server_destroy(p);
                UpnpLib_set_bWebServerState(p, WEB_SERVER_DISABLED);
                SetHTTPGetCallback(NULL);
        }

        return UPNP_E_SUCCESS;
#else /* Internal web server disabled */
        return UPNP_E_NO_WEB_SERVER;
#endif /* INTERNAL_WEB_SERVER */
}

/*!
 * \brief Checks if the webserver is enabled or disabled.
 *
 * \return 1, if webserver is enabled or 0, if webserver is disabled.
 */
int UpnpIsWebserverEnabled(UpnpLib *p)
{
        if (!UpnpLib_get_UpnpSdkInit(p)) {
                return 0;
        }

        return UpnpLib_get_bWebServerState(p) == WEB_SERVER_ENABLED;
}

void UpnpSetHostValidateCallback(
        UpnpLib *p, WebCallback_HostValidate callback, void *cookie)
{
        UpnpLib_set_webCallback_HostValidate(p, callback);
        UpnpLib_set_webCallback_HostValidateCookie(p, cookie);
}

void UpnpSetAllowLiteralHostRedirection(UpnpLib *p, int enable)
{
        UpnpLib_set_allowLiteralHostRedirection(p, enable);
}

int UpnpVirtualDir_set_GetInfoCallback(UpnpLib *p, VDCallback_GetInfo callback)
{
        int ret = UPNP_E_SUCCESS;

        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->get_info = callback;
        }

        return ret;
}

int UpnpVirtualDir_set_OpenCallback(UpnpLib *p, VDCallback_Open callback)
{
        int ret = UPNP_E_SUCCESS;

        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->open = callback;
        }

        return ret;
}

int UpnpVirtualDir_set_ReadCallback(UpnpLib *p, VDCallback_Read callback)
{
        int ret = UPNP_E_SUCCESS;
        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->read = callback;
        }

        return ret;
}

int UpnpVirtualDir_set_WriteCallback(UpnpLib *p, VDCallback_Write callback)
{
        int ret = UPNP_E_SUCCESS;
        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->write = callback;
        }

        return ret;
}

int UpnpVirtualDir_set_SeekCallback(UpnpLib *p, VDCallback_Seek callback)
{
        int ret = UPNP_E_SUCCESS;
        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->seek = callback;
        }

        return ret;
}

int UpnpVirtualDir_set_CloseCallback(UpnpLib *p, VDCallback_Close callback)
{
        int ret = UPNP_E_SUCCESS;
        if (!callback) {
                ret = UPNP_E_INVALID_PARAM;
        } else {
                UpnpLib_getnc_virtualDirCallback(p)->close = callback;
        }

        return ret;
}

int UpnpSetContentLength(
        UpnpLib *p, UpnpClient_Handle Hnd, size_t contentLength)
{
        int errCode = UPNP_E_SUCCESS;
        struct Handle_Info *HInfo = NULL;

        do {
                if (!UpnpLib_get_UpnpSdkInit(p)) {
                        errCode = UPNP_E_FINISH;
                        break;
                }

                HandleLock();

                switch (GetHandleInfo(p, Hnd, &HInfo)) {
                case HND_DEVICE:
                        break;
                default:
                        HandleUnlock();
                        return UPNP_E_INVALID_HANDLE;
                }
                if (contentLength > MAX_SOAP_CONTENT_LENGTH) {
                        errCode = UPNP_E_OUTOF_BOUNDS;
                        break;
                }
                UpnpLib_set_g_maxContentLength(p, contentLength);
        } while (0);

        HandleUnlock();
        return errCode;
}

int UpnpSetMaxContentLength(UpnpLib *p, size_t contentLength)
{
        int errCode = UPNP_E_SUCCESS;

        do {
                if (!UpnpLib_get_UpnpSdkInit(p)) {
                        errCode = UPNP_E_FINISH;
                        break;
                }
                UpnpLib_set_g_maxContentLength(p, contentLength);
        } while (0);

        return errCode;
}

int UpnpSetEventQueueLimits(UpnpLib *p, int maxLen, int maxAge)
{
        UpnpLib_set_g_UpnpSdkEQMaxLen(p, maxLen);
        UpnpLib_set_g_UpnpSdkEQMaxAge(p, maxAge);

        return UPNP_E_SUCCESS;
}

/* Internal Unit tests */

#include "httpreadwrite.h"

int UpnpInternalUnitTest(const char *test_name)
{
        if (!strcmp(test_name, "httpreadwrite")) {
                return unit_test_httpwrite();
        }
        printf("Test name was not recognized: %s\n", test_name);

        return 0;
}

/* @} UPnPAPI */
