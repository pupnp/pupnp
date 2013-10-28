/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation 
 * All rights reserved. 
 * Copyright (c) 2012 France Telecom All rights reserved. 
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
 **************************************************************************/

/*!
 * \file
 *
 * \brief Defines the Web Server and has functions to carry out
 * operations of the Web Server.
 */

#include "config.h"

#if EXCLUDE_WEB_SERVER == 0

#include "webserver.h"


#include "FileInfo.h"
#include "httpparser.h"
#include "httpreadwrite.h"
#include "ithread.h"
#include "membuffer.h"
#include "ssdplib.h"
#include "statcodes.h"
#include "strintmap.h"
#include "unixutil.h"
#include "upnp.h"
#include "upnpapi.h"
#include "UpnpIntTypes.h"
#include "UpnpStdInt.h"
#include "upnputil.h"
#include "VirtualDir.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef WIN32
	 #define snprintf _snprintf
#endif

/*!
 * Response Types.
 */
enum resp_type {
	RESP_FILEDOC,
	RESP_XMLDOC,
	RESP_HEADERS,
	RESP_WEBDOC,
	RESP_POST
};

/* mapping of file extension to content-type of document */
struct document_type_t {
	/*! . */
	const char *file_ext;
	/*! . */
	const char *content_type;
	/*! . */
	const char *content_subtype;
};

struct xml_alias_t {
	/*! name of DOC from root; e.g.: /foo/bar/mydesc.xml */
	membuffer name;
	/*! the XML document contents */
	membuffer doc;
	/*! . */
	time_t last_modified;
	/*! . */
	int *ct;
};

static const char *gMediaTypes[] = {
	/*! 0. */
	NULL,
	/*! 1. */
	"audio",
	/*! 2. */
	"video",
	/*! 3. */
	"image",
	/*! 4. */
	"application",
	/*! 5. */
	"text"
};

/*
 * Defines.
 */

/* index into 'gMediaTypes' */
#define AUDIO_STR        "\1"
#define VIDEO_STR        "\2"
#define IMAGE_STR        "\3"
#define APPLICATION_STR  "\4"
#define TEXT_STR         "\5"

/* int index */
#define APPLICATION_INDEX 4
#define TEXT_INDEX        5

/* general */
#define NUM_MEDIA_TYPES       69
#define NUM_HTTP_HEADER_NAMES 33

#define ASCTIME_R_BUFFER_SIZE 26
#ifdef WIN32
static char *web_server_asctime_r(const struct tm *tm, char *buf)
{
	if (tm == NULL || buf == NULL)
		return NULL;

	asctime_s(buf, ASCTIME_R_BUFFER_SIZE, tm);
	return buf;
}
#else
#define web_server_asctime_r asctime_r
#endif

/* sorted by file extension; must have 'NUM_MEDIA_TYPES' extensions */
static const char *gEncodedMediaTypes =
	"aif\0" AUDIO_STR "aiff\0"
	"aifc\0" AUDIO_STR "aiff\0"
	"aiff\0" AUDIO_STR "aiff\0"
	"asf\0" VIDEO_STR "x-ms-asf\0"
	"asx\0" VIDEO_STR "x-ms-asf\0"
	"au\0" AUDIO_STR "basic\0"
	"avi\0" VIDEO_STR "msvideo\0"
	"bmp\0" IMAGE_STR "bmp\0"
	"dcr\0" APPLICATION_STR "x-director\0"
	"dib\0" IMAGE_STR "bmp\0"
	"dir\0" APPLICATION_STR "x-director\0"
	"dxr\0" APPLICATION_STR "x-director\0"
	"gif\0" IMAGE_STR "gif\0"
	"hta\0" TEXT_STR "hta\0"
	"htm\0" TEXT_STR "html\0"
	"html\0" TEXT_STR "html\0"
	"jar\0" APPLICATION_STR "java-archive\0"
	"jfif\0" IMAGE_STR "pjpeg\0"
	"jpe\0" IMAGE_STR "jpeg\0"
	"jpeg\0" IMAGE_STR "jpeg\0"
	"jpg\0" IMAGE_STR "jpeg\0"
	"js\0" APPLICATION_STR "x-javascript\0"
	"kar\0" AUDIO_STR "midi\0"
	"m3u\0" AUDIO_STR "mpegurl\0"
	"mid\0" AUDIO_STR "midi\0"
	"midi\0" AUDIO_STR "midi\0"
	"mov\0" VIDEO_STR "quicktime\0"
	"mp2v\0" VIDEO_STR "x-mpeg2\0"
	"mp3\0" AUDIO_STR "mpeg\0"
	"mpe\0" VIDEO_STR "mpeg\0"
	"mpeg\0" VIDEO_STR "mpeg\0"
	"mpg\0" VIDEO_STR "mpeg\0"
	"mpv\0" VIDEO_STR "mpeg\0"
	"mpv2\0" VIDEO_STR "x-mpeg2\0"
	"pdf\0" APPLICATION_STR "pdf\0"
	"pjp\0" IMAGE_STR "jpeg\0"
	"pjpeg\0" IMAGE_STR "jpeg\0"
	"plg\0" TEXT_STR "html\0"
	"pls\0" AUDIO_STR "scpls\0"
	"png\0" IMAGE_STR "png\0"
	"qt\0" VIDEO_STR "quicktime\0"
	"ram\0" AUDIO_STR "x-pn-realaudio\0"
	"rmi\0" AUDIO_STR "mid\0"
	"rmm\0" AUDIO_STR "x-pn-realaudio\0"
	"rtf\0" APPLICATION_STR "rtf\0"
	"shtml\0" TEXT_STR "html\0"
	"smf\0" AUDIO_STR "midi\0"
	"snd\0" AUDIO_STR "basic\0"
	"spl\0" APPLICATION_STR "futuresplash\0"
	"ssm\0" APPLICATION_STR "streamingmedia\0"
	"swf\0" APPLICATION_STR "x-shockwave-flash\0"
	"tar\0" APPLICATION_STR "tar\0"
	"tcl\0" APPLICATION_STR "x-tcl\0"
	"text\0" TEXT_STR "plain\0"
	"tif\0" IMAGE_STR "tiff\0"
	"tiff\0" IMAGE_STR "tiff\0"
	"txt\0" TEXT_STR "plain\0"
	"ulw\0" AUDIO_STR "basic\0"
	"wav\0" AUDIO_STR "wav\0"
	"wax\0" AUDIO_STR "x-ms-wax\0"
	"wm\0" VIDEO_STR "x-ms-wm\0"
	"wma\0" AUDIO_STR "x-ms-wma\0"
	"wmv\0" VIDEO_STR "x-ms-wmv\0"
	"wvx\0" VIDEO_STR "x-ms-wvx\0"
	"xbm\0" IMAGE_STR "x-xbitmap\0"
	"xml\0" TEXT_STR "xml\0"
	"xsl\0" TEXT_STR "xml\0"
	"z\0" APPLICATION_STR "x-compress\0"
	"zip\0" APPLICATION_STR "zip\0" "\0";
/* *** end *** */

/*!
 * module variables - Globals, static and externs.
 */

static struct document_type_t gMediaTypeList[NUM_MEDIA_TYPES];

/*! Global variable. A local dir which serves as webserver root. */
membuffer gDocumentRootDir;

/*! XML document. */
static struct xml_alias_t gAliasDoc;
static ithread_mutex_t gWebMutex;
extern str_int_entry Http_Header_Names[NUM_HTTP_HEADER_NAMES];

/*!
 * \brief Decodes list and stores it in gMediaTypeList.
 */
static UPNP_INLINE void media_list_init(void)
{
	int i;
	const char *s = gEncodedMediaTypes;
	struct document_type_t *doc_type;

	for (i = 0; *s != '\0'; i++) {
		doc_type = &gMediaTypeList[i];
		doc_type->file_ext = s;
		/* point to type. */
		s += strlen(s) + 1;
		doc_type->content_type = gMediaTypes[(int)*s];
		/* point to subtype. */
		s++;
		doc_type->content_subtype = s;
		/* next entry. */
		s += strlen(s) + 1;
	}
	assert(i == NUM_MEDIA_TYPES);
}

/*!
 * \brief Based on the extension, returns the content type and content
 * subtype.
 *
 * \return
 * \li \c 0 on success
 * \li \c -1 on error
 */
static UPNP_INLINE int search_extension(
	/*! [in] . */
	const char *extension,
	/*! [out] . */
	const char **con_type,
	/*! [out] . */
	const char **con_subtype)
{
	int top, mid, bot;
	int cmp;

	top = 0;
	bot = NUM_MEDIA_TYPES - 1;

	while (top <= bot) {
		mid = (top + bot) / 2;
		cmp = strcasecmp(extension, gMediaTypeList[mid].file_ext);
		if (cmp > 0) {
			/* look below mid. */
			top = mid + 1;
		} else if (cmp < 0) {
			/* look above mid. */
			bot = mid - 1;
		} else {
			/* cmp == 0 */
			*con_type = gMediaTypeList[mid].content_type;
			*con_subtype = gMediaTypeList[mid].content_subtype;
			return 0;
		}
	}

	return -1;
}

/*!
 * \brief Based on the extension, clones an XML string based on type and
 * content subtype. If content type and sub type are not found, unknown
 * types are used.
 *
 * \return
 * \li \c 0 on success.
 * \li \c UPNP_E_OUTOF_MEMORY - on memory allocation failures.
 */
static UPNP_INLINE int get_content_type(
	/*! [in] . */
	const char *filename,
	/*! [out] . */
	OUT UpnpFileInfo *fileInfo)
{
	const char *extension;
	const char *type;
	const char *subtype;
	int ctype_found = FALSE;
	char *temp = NULL;
	size_t length = 0;
	int rc = 0;

	UpnpFileInfo_set_ContentType(fileInfo, NULL);
	/* get ext */
	extension = strrchr(filename, '.');
	if (extension != NULL)
		if (search_extension(extension + 1, &type, &subtype) == 0)
			ctype_found = TRUE;
	if (!ctype_found) {
		/* unknown content type */
		type = gMediaTypes[APPLICATION_INDEX];
		subtype = "octet-stream";
	}
	length = strlen(type) + strlen("/") + strlen(subtype) + 1;
	temp = malloc(length);
	if (!temp)
		return UPNP_E_OUTOF_MEMORY;
	rc = snprintf(temp, length, "%s/%s", type, subtype);
	if (rc < 0 || (unsigned int) rc >= length) {
		free(temp);
		return UPNP_E_OUTOF_MEMORY;
	}
	UpnpFileInfo_set_ContentType(fileInfo, temp);
	free(temp);
	if (!UpnpFileInfo_get_ContentType(fileInfo))
		return UPNP_E_OUTOF_MEMORY;

	return 0;
}

/*!
 * \brief Initialize the global XML document. Allocate buffers for the XML
 * document.
 */
static UPNP_INLINE void glob_alias_init(void)
{
	struct xml_alias_t *alias = &gAliasDoc;

	membuffer_init(&alias->doc);
	membuffer_init(&alias->name);
	alias->ct = NULL;
	alias->last_modified = 0;
}

/*!
 * \brief Check for the validity of the XML object buffer.
 *
 * \return BOOLEAN.
 */
static UPNP_INLINE int is_valid_alias(
	/*! [in] XML alias object. */
	const struct xml_alias_t *alias)
{
	return alias->doc.buf != NULL;
}

/*!
 * \brief Copy the contents of the global XML document into the local output
 * parameter.
 */
static void alias_grab(
	/*! [out] XML alias object. */
	struct xml_alias_t *alias)
{
	ithread_mutex_lock(&gWebMutex);
	assert(is_valid_alias(&gAliasDoc));
	memcpy(alias, &gAliasDoc, sizeof(struct xml_alias_t));
	*alias->ct = *alias->ct + 1;
	ithread_mutex_unlock(&gWebMutex);
}

/*!
 * \brief Release the XML document referred to by the input parameter. Free
 * the allocated buffers associated with this object.
 */
static void alias_release(
	/*! [in] XML alias object. */
	struct xml_alias_t *alias)
{
	ithread_mutex_lock(&gWebMutex);
	/* ignore invalid alias */
	if (!is_valid_alias(alias)) {
		ithread_mutex_unlock(&gWebMutex);
		return;
	}
	assert(*alias->ct > 0);
	*alias->ct -= 1;
	if (*alias->ct <= 0) {
		membuffer_destroy(&alias->doc);
		membuffer_destroy(&alias->name);
		free(alias->ct);
	}
	ithread_mutex_unlock(&gWebMutex);
}

int web_server_set_alias(const char *alias_name, 
	const char *alias_content, size_t alias_content_length,
	time_t last_modified)
{
	int ret_code;
	struct xml_alias_t alias;

	alias_release(&gAliasDoc);
	if (alias_name == NULL) {
		/* don't serve aliased doc anymore */
		return 0;
	}
	assert(alias_content != NULL);
	membuffer_init(&alias.doc);
	membuffer_init(&alias.name);
	alias.ct = NULL;
	do {
		/* insert leading /, if missing */
		if (*alias_name != '/')
			if (membuffer_assign_str(&alias.name, "/") != 0)
				break;	/* error; out of mem */
		ret_code = membuffer_append_str(&alias.name, alias_name);
		if (ret_code != 0)
			break;	/* error */
		if ((alias.ct = (int *)malloc(sizeof(int))) == NULL)
			break;	/* error */
		*alias.ct = 1;
		membuffer_attach(&alias.doc, (char *)alias_content,
				 alias_content_length);
		alias.last_modified = last_modified;
		/* save in module var */
		ithread_mutex_lock(&gWebMutex);
		gAliasDoc = alias;
		ithread_mutex_unlock(&gWebMutex);

		return 0;
	} while (FALSE);
	/* error handler */
	/* free temp alias */
	membuffer_destroy(&alias.name);
	membuffer_destroy(&alias.doc);
	free(alias.ct);

	return UPNP_E_OUTOF_MEMORY;
}

int web_server_init()
{
	int ret = 0;

	if (bWebServerState == WEB_SERVER_DISABLED) {
		/* decode media list */
		media_list_init();
		membuffer_init(&gDocumentRootDir);
		glob_alias_init();
		pVirtualDirList = NULL;

		/* Initialize callbacks */
		virtualDirCallback.get_info = NULL;
		virtualDirCallback.open = NULL;
		virtualDirCallback.read = NULL;
		virtualDirCallback.write = NULL;
		virtualDirCallback.seek = NULL;
		virtualDirCallback.close = NULL;

		if (ithread_mutex_init(&gWebMutex, NULL) == -1)
			ret = UPNP_E_OUTOF_MEMORY;
		else
			bWebServerState = WEB_SERVER_ENABLED;
	}

	return ret;
}

void web_server_destroy(void)
{
	if (bWebServerState == WEB_SERVER_ENABLED) {
		membuffer_destroy(&gDocumentRootDir);
		alias_release(&gAliasDoc);

		ithread_mutex_lock(&gWebMutex);
		memset(&gAliasDoc, 0, sizeof(struct xml_alias_t));
		ithread_mutex_unlock(&gWebMutex);

		ithread_mutex_destroy(&gWebMutex);
		bWebServerState = WEB_SERVER_DISABLED;
	}
}

/*!
 * \brief Release memory allocated for the global web server root directory
 * and the global XML document. Resets the flag bWebServerState to
 * WEB_SERVER_DISABLED.
 *
 * \return Integer.
 */
static int get_file_info(
	/*! [in] Filename having the description document. */
	const char *filename,
	/*! [out] File information object having file attributes such as filelength,
	 * when was the file last modified, whether a file or a directory and
	 * whether the file or directory is readable. */
	OUT UpnpFileInfo *info)
{
	int code;
	struct stat s;
	FILE *fp;
	int rc = 0;
	time_t aux_LastModified;
	struct tm date;
	char buffer[ASCTIME_R_BUFFER_SIZE];

	UpnpFileInfo_set_ContentType(info, NULL);
	code = stat(filename, &s);
	if (code == -1)
		return -1;
	if (S_ISDIR(s.st_mode))
		UpnpFileInfo_set_IsDirectory(info, TRUE);
	else if (S_ISREG(s.st_mode))
		UpnpFileInfo_set_IsDirectory(info, FALSE);
	else
		return -1;
	/* check readable */
	fp = fopen(filename, "r");
	UpnpFileInfo_set_IsReadable(info, fp != NULL);
	if (fp)
		fclose(fp);
	UpnpFileInfo_set_FileLength(info, s.st_size);
	UpnpFileInfo_set_LastModified(info, s.st_mtime);
	rc = get_content_type(filename, info);
	aux_LastModified = UpnpFileInfo_get_LastModified(info);
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		"file info: %s, length: %lld, last_mod=%s readable=%d\n",
		filename,
		(long long)UpnpFileInfo_get_FileLength(info),
		web_server_asctime_r(http_gmtime_r(&aux_LastModified, &date), buffer),
		UpnpFileInfo_get_IsReadable(info));

	return rc;
}

int web_server_set_root_dir(const char *root_dir)
{
	size_t index;
	int ret;

	ret = membuffer_assign_str(&gDocumentRootDir, root_dir);
	if (ret != 0)
		return ret;
	/* remove trailing '/', if any */
	if (gDocumentRootDir.length > 0) {
		index = gDocumentRootDir.length - 1;	/* last char */
		if (gDocumentRootDir.buf[index] == '/')
			membuffer_delete(&gDocumentRootDir, index, 1);
	}

	return 0;
}

/*!
 * \brief Compare the files names between the one on the XML alias the one
 * passed in as the input parameter. If equal extract file information.
 *
 * \return
 * \li \c TRUE - On Success
 * \li \c FALSE if request is not an alias
 */
static UPNP_INLINE int get_alias(
	/*! [in] request file passed in to be compared with. */
	const char *request_file,
	/*! [out] xml alias object which has a file name stored. */
	struct xml_alias_t *alias,
	/*! [out] File information object which will be filled up if the file
	 * comparison succeeds. */
	UpnpFileInfo *info)
{
	int cmp = strcmp(alias->name.buf, request_file);
	if (cmp == 0) {
		UpnpFileInfo_set_FileLength(info, (off_t)alias->doc.length);
		UpnpFileInfo_set_IsDirectory(info, FALSE);
		UpnpFileInfo_set_IsReadable(info, TRUE);
		UpnpFileInfo_set_LastModified(info, alias->last_modified);
	}

	return cmp == 0;
}

/*!
 * \brief Compares filePath with paths from the list of virtual directory
 * lists.
 *
 * \return BOOLEAN.
 */
static int isFileInVirtualDir(
	/*! [in] Directory path to be tested for virtual directory. */
	char *filePath)
{
	virtualDirList *pCurVirtualDir;
	size_t webDirLen;

	pCurVirtualDir = pVirtualDirList;
	while (pCurVirtualDir != NULL) {
		webDirLen = strlen(pCurVirtualDir->dirName);
		if (webDirLen) {
			if (pCurVirtualDir->dirName[webDirLen - 1] == '/') {
				if (strncmp(pCurVirtualDir->dirName, filePath,
						webDirLen) == 0)
					return !0;
			} else {
				if (strncmp(pCurVirtualDir->dirName, filePath,
						webDirLen) == 0 &&
				    (filePath[webDirLen] == '/' ||
				     filePath[webDirLen] == '\0' ||
				     filePath[webDirLen] == '?'))
					return !0;
			}
		}
		pCurVirtualDir = pCurVirtualDir->next;
	}

	return 0;
}

/*!
 * \brief Converts input string to upper case.
 */
static void ToUpperCase(
	/*! Input string to be converted. */
	char *s)
{
	while (*s) {
		*s = (char)toupper(*s);
		++s;
	}
}

/*!
 * \brief Finds a substring from a string in a case insensitive way.
 *
 * \return A pointer to the first occurence of s2 in s1.
 */
static char *StrStr(
	/*! Input string. */
	char *s1,
	/*! Input sub-string. */
	const char *s2)
{
	char *Str1;
	char *Str2;
	const char *Ptr;
	char *ret = NULL;

	Str1 = strdup(s1);
	if (!Str1)
		goto error1;
	Str2 = strdup(s2);
	if (!Str2)
		goto error2;
	ToUpperCase(Str1);
	ToUpperCase(Str2);
	Ptr = strstr(Str1, Str2);
	if (!Ptr)
		ret = NULL;
	else
		ret = s1 + (Ptr - Str1);

	free(Str2);
error2:
	free(Str1);
error1:
	return ret;
}

/*!
 * \brief Finds next token in a string.
 *
 * \return Pointer to the next token.
 */
static char *StrTok(
	/*! String containing the token. */
	char **Src,
	/*! Set of delimiter characters. */
	const char *Del)
{
	char *TmpPtr;
	char *RetPtr;

	if (*Src != NULL) {
		RetPtr = *Src;
		TmpPtr = strstr(*Src, Del);
		if (TmpPtr != NULL) {
			*TmpPtr = '\0';
			*Src = TmpPtr + strlen(Del);
		} else
			*Src = NULL;

		return RetPtr;
	}

	return NULL;
}

/*!
 * \brief Returns a range of integers from a string.
 *
 * \return Always returns 1.
 */
static int GetNextRange(
	/*! string containing the token / range. */
	char **SrcRangeStr,
	/*! gets the first byte of the token. */
	off_t *FirstByte,
	/*! gets the last byte of the token. */
	off_t *LastByte)
{
	char *Ptr;
	char *Tok;
	int i;
	int64_t F = -1;
	int64_t L = -1;
	int Is_Suffix_byte_Range = 1;

	if (*SrcRangeStr == NULL)
		return -1;
	Tok = StrTok(SrcRangeStr, ",");
	if ((Ptr = strstr(Tok, "-")) == NULL)
		return -1;
	*Ptr = ' ';
	sscanf(Tok, "%" SCNd64 "%" SCNd64, &F, &L);
	if (F == -1 || L == -1) {
		*Ptr = '-';
		for (i = 0; i < (int)strlen(Tok); i++) {
			if (Tok[i] == '-') {
				break;
			} else if (isdigit(Tok[i])) {
				Is_Suffix_byte_Range = 0;
				break;
			}
		}
		if (Is_Suffix_byte_Range) {
			*FirstByte = (off_t) L;
			*LastByte = (off_t) F;
			return 1;
		}
	}
	*FirstByte = (off_t) F;
	*LastByte = (off_t) L;

	return 1;
}

/*!
 * \brief Fills in the Offset, read size and contents to send out as an HTTP
 * Range Response.
 *
 * \return
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 * \li \c HTTP_OK
 */
static int CreateHTTPRangeResponseHeader(
	/*! String containing the range. */
	char *ByteRangeSpecifier,
	/*! Length of the file. */
	off_t FileLength,
	/*! [out] SendInstruction object where the range operations will be stored. */
	struct SendInstruction *Instr)
{
	off_t FirstByte, LastByte;
	char *RangeInput;
	char *Ptr;
	int rc = 0;

	Instr->IsRangeActive = 1;
	Instr->ReadSendSize = FileLength;
	if (!ByteRangeSpecifier)
		return HTTP_BAD_REQUEST;
	RangeInput = malloc(strlen(ByteRangeSpecifier) + 1);
	if (!RangeInput)
		return HTTP_INTERNAL_SERVER_ERROR;
	memset(RangeInput, 0, strlen(ByteRangeSpecifier) + 1);
	strncpy(RangeInput, ByteRangeSpecifier, strlen(ByteRangeSpecifier));
	/* CONTENT-RANGE: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
	if (StrStr(RangeInput, "bytes") == NULL ||
	    (Ptr = StrStr(RangeInput, "=")) == NULL) {
		free(RangeInput);
		Instr->IsRangeActive = 0;
		return HTTP_BAD_REQUEST;
	}
	/* Jump = */
	Ptr = Ptr + 1;
	if (FileLength < 0) {
		free(RangeInput);
		return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
	}
	if (GetNextRange(&Ptr, &FirstByte, &LastByte) != -1) {
		if (FileLength < FirstByte) {
			free(RangeInput);
			return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
		}
		if (FirstByte >= 0 && LastByte >= 0 && LastByte >= FirstByte) {
			if (LastByte >= FileLength)
				LastByte = FileLength - 1;
			Instr->RangeOffset = FirstByte;
			Instr->ReadSendSize = LastByte - FirstByte + 1;
			/* Data between two range. */
			rc = snprintf(Instr->RangeHeader,
				sizeof(Instr->RangeHeader),
				"CONTENT-RANGE: bytes %" PRId64
				"-%" PRId64 "/%" PRId64 "\r\n",
				(int64_t)FirstByte,
				(int64_t)LastByte,
				(int64_t)FileLength);
			if (rc < 0 || (unsigned int) rc >= sizeof(Instr->RangeHeader)) {
				free(RangeInput);
				return HTTP_INTERNAL_SERVER_ERROR;
			}
		} else if (FirstByte >= 0 && LastByte == -1
			   && FirstByte < FileLength) {
			Instr->RangeOffset = FirstByte;
			Instr->ReadSendSize = FileLength - FirstByte;
			rc = snprintf(Instr->RangeHeader,
				sizeof(Instr->RangeHeader),
				"CONTENT-RANGE: bytes %" PRId64
				"-%" PRId64 "/%" PRId64 "\r\n",
				(int64_t)FirstByte,
				(int64_t)(FileLength - 1),
				(int64_t)FileLength);
			if (rc < 0 || (unsigned int) rc >= sizeof(Instr->RangeHeader)) {
				free(RangeInput);
				return HTTP_INTERNAL_SERVER_ERROR;
			}
		} else if (FirstByte == -1 && LastByte > 0) {
			if (LastByte >= FileLength) {
				Instr->RangeOffset = 0;
				Instr->ReadSendSize = FileLength;
				rc = snprintf(Instr->RangeHeader,
					sizeof(Instr->RangeHeader),
					"CONTENT-RANGE: bytes 0-%" PRId64
					"/%" PRId64 "\r\n",
					(int64_t)(FileLength - 1),
					(int64_t)FileLength);
			} else {
				Instr->RangeOffset = FileLength - LastByte;
				Instr->ReadSendSize = LastByte;
				rc = snprintf(Instr->RangeHeader,
					sizeof(Instr->RangeHeader),
					"CONTENT-RANGE: bytes %" PRId64
					"-%" PRId64 "/%" PRId64 "\r\n",
					(int64_t)(FileLength - LastByte),
					(int64_t)FileLength - 1,
					(int64_t)FileLength);
			}
			if (rc < 0 || (unsigned int) rc >= sizeof(Instr->RangeHeader)) {
				free(RangeInput);
				return HTTP_INTERNAL_SERVER_ERROR;
			}
		} else {
			free(RangeInput);
			return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
		}
	} else {
		free(RangeInput);
		return HTTP_REQUEST_RANGE_NOT_SATISFIABLE;
	}

	free(RangeInput);
	return HTTP_OK;
}

/*!
 * \brief Get header id from the request parameter and take appropriate
 * action based on the ids as an HTTP Range Response.
 *
 * \return
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 * \li \c HTTP_OK
 */
static int CheckOtherHTTPHeaders(
	/*! [in] HTTP Request message. */
	http_message_t *Req,
	/*! [out] Send Instruction object to data for the response. */
	struct SendInstruction *RespInstr,
	/*! Size of the file containing the request document. */
	off_t FileSize)
{
	http_header_t *header;
	ListNode *node;
	/*NNS: dlist_node* node; */
	int index, RetCode = HTTP_OK;
	char *TmpBuf;
	size_t TmpBufSize = LINE_SIZE;

	TmpBuf = (char *)malloc(TmpBufSize);
	if (!TmpBuf)
		return HTTP_INTERNAL_SERVER_ERROR;
	node = ListHead(&Req->headers);
	while (node != NULL) {
		header = (http_header_t *) node->item;
		/* find header type. */
		index = map_str_to_int((const char *)header->name.buf,
				header->name.length, Http_Header_Names,
				NUM_HTTP_HEADER_NAMES, FALSE);
		if (header->value.length >= TmpBufSize) {
			free(TmpBuf);
			TmpBufSize = header->value.length + 1;
			TmpBuf = (char *)malloc(TmpBufSize);
			if (!TmpBuf)
				return HTTP_INTERNAL_SERVER_ERROR;
		}
		memcpy(TmpBuf, header->value.buf, header->value.length);
		TmpBuf[header->value.length] = '\0';
		if (index >= 0) {
			switch (Http_Header_Names[index].id) {
			case HDR_TE: {
				/* Request */
				RespInstr->IsChunkActive = 1;

				if (strlen(TmpBuf) > strlen("gzip")) {
					/* means client will accept trailer. */
					if (StrStr(TmpBuf, "trailers") != NULL) {
						RespInstr->IsTrailers = 1;
					}
				}
				break;
			}
			case HDR_CONTENT_LENGTH:
				RespInstr->RecvWriteSize = atoi(TmpBuf);
				break;
			case HDR_RANGE:
				RetCode = CreateHTTPRangeResponseHeader(TmpBuf,
					FileSize, RespInstr);
				if (RetCode != HTTP_OK) {
					free(TmpBuf);
					return RetCode;
				}
				break;
			case HDR_ACCEPT_LANGUAGE:
				if (header->value.length + 1 > sizeof(RespInstr->AcceptLanguageHeader)) {
					size_t length = sizeof(RespInstr->AcceptLanguageHeader) - 1;
					memcpy(RespInstr->AcceptLanguageHeader, TmpBuf, length);
					RespInstr->AcceptLanguageHeader[length] = '\0';
				} else {
					memcpy(RespInstr->AcceptLanguageHeader, TmpBuf,
						header->value.length + 1);
				}
				break;
			default:
				/*
				   TODO
				 */
				/*
				   header.value is the value.
				 */
				/*
				   case HDR_CONTENT_TYPE: return 1;
				   case HDR_CONTENT_LANGUAGE:return 1;
				   case HDR_LOCATION: return 1;
				   case HDR_CONTENT_LOCATION:return 1;
				   case HDR_ACCEPT: return 1;
				   case HDR_ACCEPT_CHARSET: return 1;
				   case HDR_USER_AGENT: return 1;
				 */

				/*Header check for encoding */
				/*
				   case HDR_ACCEPT_RANGE:
				   case HDR_CONTENT_RANGE:
				   case HDR_IF_RANGE:
				 */

				/*Header check for encoding */
				/*
				   case HDR_ACCEPT_ENCODING:
				   if(StrStr(TmpBuf, "identity"))
				   {
				   break;
				   }
				   else return -1;
				   case HDR_CONTENT_ENCODING:
				   case HDR_TRANSFER_ENCODING:
				 */
				break;
			}
		}
		node = ListNext(&Req->headers, node);
	}
	free(TmpBuf);

	return RetCode;
}

/*!
 * \brief Processes the request and returns the result in the output parameters.
 *
 * \return
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_REQUEST_RANGE_NOT_SATISFIABLE
 * \li \c HTTP_FORBIDDEN
 * \li \c HTTP_NOT_FOUND
 * \li \c HTTP_NOT_ACCEPTABLE
 * \li \c HTTP_OK
 */
static int process_request(
	/*! [in] HTTP Request message. */
	http_message_t *req,
	/*! [out] Tpye of response. */
	enum resp_type *rtype,
	/*! [out] Headers. */
	membuffer *headers,
	/*! [out] Get filename from request document. */
	membuffer *filename,
	/*! [out] Xml alias document from the request document. */
	struct xml_alias_t *alias,
	/*! [out] Send Instruction object where the response is set up. */
	struct SendInstruction *RespInstr)
{
	int code;
	int err_code;

	char *request_doc;
	UpnpFileInfo *finfo;
	time_t aux_LastModified;
	int using_alias;
	int using_virtual_dir;
	uri_type *url;
	const char *temp_str;
	int resp_major;
	int resp_minor;
	int alias_grabbed;
	size_t dummy;
	const char *extra_headers = NULL;

	print_http_headers(req);
	url = &req->uri;
	assert(req->method == HTTPMETHOD_GET ||
	       req->method == HTTPMETHOD_HEAD ||
	       req->method == HTTPMETHOD_POST ||
	       req->method == HTTPMETHOD_SIMPLEGET);
	/* init */
	memset(&finfo, 0, sizeof(finfo));
	request_doc = NULL;
	finfo = UpnpFileInfo_new();
	alias_grabbed = FALSE;
	err_code = HTTP_INTERNAL_SERVER_ERROR;	/* default error */
	using_virtual_dir = FALSE;
	using_alias = FALSE;

	http_CalcResponseVersion(req->major_version, req->minor_version,
				 &resp_major, &resp_minor);
	/* */
	/* remove dots */
	/* */
	request_doc = malloc(url->pathquery.size + 1);
	if (request_doc == NULL) {
		goto error_handler;	/* out of mem */
	}
	memcpy(request_doc, url->pathquery.buff, url->pathquery.size);
	request_doc[url->pathquery.size] = '\0';
	dummy = url->pathquery.size;
	remove_escaped_chars(request_doc, &dummy);
	code = remove_dots(request_doc, url->pathquery.size);
	if (code != 0) {
		err_code = HTTP_FORBIDDEN;
		goto error_handler;
	}
	if (*request_doc != '/') {
		/* no slash */
		err_code = HTTP_BAD_REQUEST;
		goto error_handler;
	}
	if (isFileInVirtualDir(request_doc)) {
		using_virtual_dir = TRUE;
		RespInstr->IsVirtualFile = 1;
		if (membuffer_assign_str(filename, request_doc) != 0) {
			goto error_handler;
		}
	} else {
		/* try using alias */
		if (is_valid_alias(&gAliasDoc)) {
			alias_grab(alias);
			alias_grabbed = TRUE;
			using_alias = get_alias(request_doc, alias, finfo);
			if (using_alias == TRUE) {
				UpnpFileInfo_set_ContentType(finfo,
					"text/xml; charset=\"utf-8\"");
				if (UpnpFileInfo_get_ContentType(finfo) == NULL) {
					goto error_handler;
				}
			}
		}
	}
	if (using_virtual_dir) {
		if (req->method != HTTPMETHOD_POST) {
			/* get file info */
			if (virtualDirCallback.
			    get_info(filename->buf, finfo) != 0) {
				err_code = HTTP_NOT_FOUND;
				goto error_handler;
			}
			/* try index.html if req is a dir */
			if (UpnpFileInfo_get_IsDirectory(finfo)) {
				if (filename->buf[filename->length - 1] == '/') {
					temp_str = "index.html";
				} else {
					temp_str = "/index.html";
				}
				if (membuffer_append_str(filename, temp_str) !=
				    0) {
					goto error_handler;
				}
				/* get info */
				if (virtualDirCallback.get_info(filename->buf, finfo) != UPNP_E_SUCCESS ||
				    UpnpFileInfo_get_IsDirectory(finfo)) {
					err_code = HTTP_NOT_FOUND;
					goto error_handler;
				}
			}
			/* not readable */
			if (!UpnpFileInfo_get_IsReadable(finfo)) {
				err_code = HTTP_FORBIDDEN;
				goto error_handler;
			}
			/* finally, get content type */
			/* if ( get_content_type(filename->buf, &content_type) != 0 ) */
			/*{ */
			/*  goto error_handler; */
			/* } */
		}
	} else if (!using_alias) {
		if (gDocumentRootDir.length == 0) {
			goto error_handler;
		}
		/* */
		/* get file name */
		/* */

		/* filename str */
		if (membuffer_assign_str(filename, gDocumentRootDir.buf) != 0 ||
		    membuffer_append_str(filename, request_doc) != 0) {
			goto error_handler;	/* out of mem */
		}
		/* remove trailing slashes */
		while (filename->length > 0 &&
		       filename->buf[filename->length - 1] == '/') {
			membuffer_delete(filename, filename->length - 1, 1);
		}
		if (req->method != HTTPMETHOD_POST) {
			/* get info on file */
			if (get_file_info(filename->buf, finfo) != 0) {
				err_code = HTTP_NOT_FOUND;
				goto error_handler;
			}
			/* try index.html if req is a dir */
			if (UpnpFileInfo_get_IsDirectory(finfo)) {
				if (filename->buf[filename->length - 1] == '/') {
					temp_str = "index.html";
				} else {
					temp_str = "/index.html";
				}
				if (membuffer_append_str(filename, temp_str) !=
				    0) {
					goto error_handler;
				}
				/* get info */
				if (get_file_info(filename->buf, finfo) != 0 ||
				    UpnpFileInfo_get_IsDirectory(finfo)) {
					err_code = HTTP_NOT_FOUND;
					goto error_handler;
				}
			}
			/* not readable */
			if (!UpnpFileInfo_get_IsReadable(finfo)) {
				err_code = HTTP_FORBIDDEN;
				goto error_handler;
			}
		}
		/* finally, get content type */
		/*      if ( get_content_type(filename->buf, &content_type) != 0 ) */
		/*      { */
		/*          goto error_handler; */
		/*      } */
	}
	RespInstr->ReadSendSize = UpnpFileInfo_get_FileLength(finfo);
	/* Check other header field. */
	code = CheckOtherHTTPHeaders(req, RespInstr,
		UpnpFileInfo_get_FileLength(finfo));
	if (code != HTTP_OK) {
		err_code = code;
		goto error_handler;
	}
	if (req->method == HTTPMETHOD_POST) {
		*rtype = RESP_POST;
		err_code = HTTP_OK;
		goto error_handler;
	}
	extra_headers = UpnpFileInfo_get_ExtraHeaders(finfo);
	if (!extra_headers) {
		extra_headers = "";
	}

	/* Check if chunked encoding should be used. */
	if (using_virtual_dir && UpnpFileInfo_get_FileLength(finfo) == UPNP_USING_CHUNKED) {
		/* Chunked encoding is only supported by HTTP 1.1 clients */
		if (resp_major == 1 && resp_minor == 1) {
			RespInstr->IsChunkActive = 1;
		} else {
			/* The virtual callback indicates that we should use
			 * chunked encoding however the client doesn't support
			 * it. Return with an internal server error. */
			err_code = HTTP_NOT_ACCEPTABLE;
			goto error_handler;
		}
	}

	aux_LastModified = UpnpFileInfo_get_LastModified(finfo);
	if (RespInstr->IsRangeActive && RespInstr->IsChunkActive) {
		/* Content-Range: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
		/* Transfer-Encoding: chunked */
		if (http_MakeMessage(headers, resp_major, resp_minor,
		    "R" "T" "GKLD" "s" "tcS" "Xc" "sCc",
		    HTTP_PARTIAL_CONTENT,	/* status code */
		    UpnpFileInfo_get_ContentType(finfo), /* content type */
		    RespInstr,	/* range info */
		    RespInstr,	/* language info */
		    "LAST-MODIFIED: ",
		    &aux_LastModified,
		    X_USER_AGENT, extra_headers) != 0) {
			goto error_handler;
		}
	} else if (RespInstr->IsRangeActive && !RespInstr->IsChunkActive) {
		/* Content-Range: bytes 222-3333/4000  HTTP_PARTIAL_CONTENT */
		if (http_MakeMessage(headers, resp_major, resp_minor,
		    "R" "N" "T" "GLD" "s" "tcS" "Xc" "sCc",
		    HTTP_PARTIAL_CONTENT,	/* status code */
		    RespInstr->ReadSendSize,	/* content length */
		    UpnpFileInfo_get_ContentType(finfo), /* content type */
		    RespInstr,	/* range info */
		    RespInstr,	/* language info */
		    "LAST-MODIFIED: ",
		    &aux_LastModified,
		    X_USER_AGENT, extra_headers) != 0) {
			goto error_handler;
		}
	} else if (!RespInstr->IsRangeActive && RespInstr->IsChunkActive) {
		/* Transfer-Encoding: chunked */
		if (http_MakeMessage(headers, resp_major, resp_minor,
		    "RK" "TLD" "s" "tcS" "Xc" "sCc",
		    HTTP_OK,	/* status code */
		    UpnpFileInfo_get_ContentType(finfo), /* content type */
		    RespInstr,	/* language info */
		    "LAST-MODIFIED: ",
		    &aux_LastModified,
		    X_USER_AGENT, extra_headers) != 0) {
			goto error_handler;
		}
	} else {
		/* !RespInstr->IsRangeActive && !RespInstr->IsChunkActive */
		if (RespInstr->ReadSendSize >= 0) {
			if (http_MakeMessage(headers, resp_major, resp_minor,
			    "R" "N" "TLD" "s" "tcS" "Xc" "sCc",
			    HTTP_OK,	/* status code */
			    RespInstr->ReadSendSize,	/* content length */
			    UpnpFileInfo_get_ContentType(finfo), /* content type */
			    RespInstr,	/* language info */
			    "LAST-MODIFIED: ",
			    &aux_LastModified,
			    X_USER_AGENT,
			    extra_headers) != 0) {
				goto error_handler;
			}
		} else {
			if (http_MakeMessage(headers, resp_major, resp_minor,
			    "R" "TLD" "s" "tcS" "Xc" "sCc",
			    HTTP_OK,	/* status code */
			    UpnpFileInfo_get_ContentType(finfo), /* content type */
			    RespInstr,	/* language info */
			    "LAST-MODIFIED: ",
			    &aux_LastModified,
			    X_USER_AGENT,
			    extra_headers) != 0) {
				goto error_handler;
			}
		}
	}
	if (req->method == HTTPMETHOD_HEAD) {
		*rtype = RESP_HEADERS;
	} else if (using_alias) {
		/* GET xml */
		*rtype = RESP_XMLDOC;
	} else if (using_virtual_dir) {
		*rtype = RESP_WEBDOC;
	} else {
		/* GET filename */
		*rtype = RESP_FILEDOC;
	}
	/* simple get http 0.9 as specified in http 1.0 */
	/* don't send headers */
	if (req->method == HTTPMETHOD_SIMPLEGET) {
		membuffer_destroy(headers);
	}
	err_code = HTTP_OK;

 error_handler:
	free(request_doc);
	UpnpFileInfo_delete(finfo);
	if (err_code != HTTP_OK && alias_grabbed) {
		alias_release(alias);
	}

	return err_code;
}

/*!
 * \brief Receives the HTTP post message.
 *
 * \return
 * \li \c HTTP_INTERNAL_SERVER_ERROR
 * \li \c HTTP_UNAUTHORIZED
 * \li \c HTTP_BAD_REQUEST
 * \li \c HTTP_SERVICE_UNAVAILABLE
 * \li \c HTTP_OK
 */
static int http_RecvPostMessage(
	/*! HTTP Parser object. */
	http_parser_t *parser,
	/*! [in] Socket Information object. */
	SOCKINFO *info,
	/*! File where received data is copied to. */
	char *filename,
	/*! Send Instruction object which gives information whether the file
	 * is a virtual file or not. */
	struct SendInstruction *Instr)
{
	size_t Data_Buf_Size = 1024;
	char Buf[1024];
	int Timeout = -1;
	FILE *Fp;
	parse_status_t status = PARSE_OK;
	int ok_on_close = FALSE;
	size_t entity_offset = 0;
	int num_read = 0;
	int ret_code = HTTP_OK;

	if (Instr && Instr->IsVirtualFile) {
		Fp = (virtualDirCallback.open) (filename, UPNP_WRITE);
		if (Fp == NULL)
			return HTTP_INTERNAL_SERVER_ERROR;
	} else {
		Fp = fopen(filename, "wb");
		if (Fp == NULL)
			return HTTP_UNAUTHORIZED;
	}
	parser->position = POS_ENTITY;
	do {
		/* first parse what has already been gotten */
		if (parser->position != POS_COMPLETE)
			status = parser_parse_entity(parser);
		if (status == PARSE_INCOMPLETE_ENTITY) {
			/* read until close */
			ok_on_close = TRUE;
		} else if ((status != PARSE_SUCCESS)
			   && (status != PARSE_CONTINUE_1)
			   && (status != PARSE_INCOMPLETE)) {
			/* error */
			ret_code = HTTP_BAD_REQUEST;
			goto ExitFunction;
		}
		/* read more if necessary entity */
		while (entity_offset + Data_Buf_Size > parser->msg.entity.length &&
		       parser->position != POS_COMPLETE) {
			num_read = sock_read(info, Buf, sizeof(Buf), &Timeout);
			if (num_read > 0) {
				/* append data to buffer */
				if (membuffer_append(&parser->msg.msg,
					Buf, (size_t)num_read) != 0) {
					/* set failure status */
					parser->http_error_code =
					    HTTP_INTERNAL_SERVER_ERROR;
					ret_code = HTTP_INTERNAL_SERVER_ERROR;
					goto ExitFunction;
				}
				status = parser_parse_entity(parser);
				if (status == PARSE_INCOMPLETE_ENTITY) {
					/* read until close */
					ok_on_close = TRUE;
				} else if ((status != PARSE_SUCCESS)
					   && (status != PARSE_CONTINUE_1)
					   && (status != PARSE_INCOMPLETE)) {
					ret_code = HTTP_BAD_REQUEST;
					goto ExitFunction;
				}
			} else if (num_read == 0) {
				if (ok_on_close) {
					UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
						"<<< (RECVD) <<<\n%s\n-----------------\n",
						parser->msg.msg.buf);
					print_http_headers(&parser->msg);
					parser->position = POS_COMPLETE;
				} else {
					/* partial msg or response */
					parser->http_error_code = HTTP_BAD_REQUEST;
					ret_code = HTTP_BAD_REQUEST;
					goto ExitFunction;
				}
			} else {
				ret_code = HTTP_SERVICE_UNAVAILABLE;
				goto ExitFunction;
			}
		}
		if ((entity_offset + Data_Buf_Size) > parser->msg.entity.length) {
			Data_Buf_Size =
			    parser->msg.entity.length - entity_offset;
		}
		memcpy(Buf,
		       &parser->msg.msg.buf[parser->entity_start_position + entity_offset],
		       Data_Buf_Size);
		entity_offset += Data_Buf_Size;
		if (Instr && Instr->IsVirtualFile) {
			int n = virtualDirCallback.write(Fp, Buf, Data_Buf_Size);
			if (n < 0) {
				ret_code = HTTP_INTERNAL_SERVER_ERROR;
				goto ExitFunction;
			}
		} else {
			size_t n = fwrite(Buf, 1, Data_Buf_Size, Fp);
			if (n != Data_Buf_Size) {
				ret_code = HTTP_INTERNAL_SERVER_ERROR;
				goto ExitFunction;
			}
		}
	} while (parser->position != POS_COMPLETE ||
		 entity_offset != parser->msg.entity.length);
ExitFunction:
	if (Instr && Instr->IsVirtualFile) {
		virtualDirCallback.close(Fp);
	} else {
		fclose(Fp);
	}

	return ret_code;
}

void web_server_callback(http_parser_t *parser, INOUT http_message_t *req,
	SOCKINFO *info)
{
	int ret;
	int timeout = -1;
	enum resp_type rtype = 0;
	membuffer headers;
	membuffer filename;
	struct xml_alias_t xmldoc;
	struct SendInstruction RespInstr;

	/*Initialize instruction header. */
	RespInstr.IsVirtualFile = 0;
	RespInstr.IsChunkActive = 0;
	RespInstr.IsRangeActive = 0;
	RespInstr.IsTrailers = 0;
	memset(RespInstr.AcceptLanguageHeader, 0,
	       sizeof(RespInstr.AcceptLanguageHeader));
	/* init */
	membuffer_init(&headers);
	membuffer_init(&filename);

	/*Process request should create the different kind of header depending on the */
	/*the type of request. */
	ret = process_request(req, &rtype, &headers, &filename, &xmldoc,
		&RespInstr);
	if (ret != HTTP_OK) {
		/* send error code */
		http_SendStatusResponse(info, ret, req->major_version,
			req->minor_version);
	} else {
		/* send response */
		switch (rtype) {
		case RESP_FILEDOC:
			http_SendMessage(info, &timeout, "Ibf",
					 &RespInstr,
					 headers.buf, headers.length,
					 filename.buf);
			break;
		case RESP_XMLDOC:
			http_SendMessage(info, &timeout, "Ibb",
				&RespInstr,
				headers.buf, headers.length,
				xmldoc.doc.buf, xmldoc.doc.length);
			alias_release(&xmldoc);
			break;
		case RESP_WEBDOC:
			/*http_SendVirtualDirDoc(info, &timeout, "Ibf",
				&RespInstr,
				headers.buf, headers.length,
				filename.buf);*/
			http_SendMessage(info, &timeout, "Ibf",
				&RespInstr,
				headers.buf, headers.length,
				filename.buf);
			break;
		case RESP_HEADERS:
			/* headers only */
			http_SendMessage(info, &timeout, "b",
				headers.buf, headers.length);
			break;
		case RESP_POST:
			/* headers only */
			ret = http_RecvPostMessage(parser, info, filename.buf,
				&RespInstr);
			/* Send response. */
			http_MakeMessage(&headers, 1, 1,
				"RTLSXcCc",
				ret, "text/html", &RespInstr, X_USER_AGENT);
			http_SendMessage(info, &timeout, "b",
				headers.buf, headers.length);
			break;
		default:
			UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
				"webserver: Invalid response type received.\n");
			assert(0);
		}
	}
	UpnpPrintf(UPNP_INFO, HTTP, __FILE__, __LINE__,
		   "webserver: request processed...\n");
	membuffer_destroy(&headers);
	membuffer_destroy(&filename);
}
#endif /* EXCLUDE_WEB_SERVER */

