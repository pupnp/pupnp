

#ifndef FILEINFO_H
#define FILEINFO_H


/*!
 * \file
 *
 * \brief UpnpFileInfo object declararion.
 *
 * \author Marcelo Roberto Jimenez
 */


/*! Detailed description of this class should go here */
typedef struct s_UpnpFileInfo UpnpFileInfo;


#include "ixml.h"       /* for DOMString */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */


#include <sys/types.h>  /* for off_t */
#include <time.h>       /* for time_t */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*! Constructor */
EXPORT_SPEC UpnpFileInfo *UpnpFileInfo_new();

/*! Destructor */
EXPORT_SPEC void UpnpFileInfo_delete(UpnpFileInfo *p);

/*! Copy Constructor */
EXPORT_SPEC UpnpFileInfo *UpnpFileInfo_dup(const UpnpFileInfo *p);

/*! Assignment operator */
EXPORT_SPEC void UpnpFileInfo_assign(UpnpFileInfo *p, const UpnpFileInfo *q);

/*! The length of the file. A length less than 0 indicates the size 
 *  is unknown, and data will be sent until 0 bytes are returned from
 *  a read call. */
EXPORT_SPEC off_t UpnpFileInfo_get_FileLength(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_FileLength(UpnpFileInfo *p, off_t l);

/*! The time at which the contents of the file was modified;
 *  The time system is always local (not GMT). */
EXPORT_SPEC const time_t *UpnpFileInfo_get_LastModified(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_LastModified(UpnpFileInfo *p, const time_t *t);

/*! If the file is a directory, \b is_directory contains
 * a non-zero value. For a regular file, it should be 0. */
EXPORT_SPEC int UpnpFileInfo_get_IsDirectory(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_IsDirectory(UpnpFileInfo *p, int b);

/*! If the file or directory is readable, this contains 
 * a non-zero value. If unreadable, it should be set to 0. */
EXPORT_SPEC int UpnpFileInfo_get_IsReadable(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_IsReadable(UpnpFileInfo *p, int b);

/*! The content type of the file. */
EXPORT_SPEC const DOMString UpnpFileInfo_get_ContentType(const UpnpFileInfo *p);
EXPORT_SPEC const char *UpnpFileInfo_get_ContentType_cstr(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_ContentType(UpnpFileInfo *p, const DOMString s);

/*! Additional HTTP headers to return. Each header line should be
 *  followed by "\r\n". */
EXPORT_SPEC const DOMString UpnpFileInfo_get_ExtraHeaders(const UpnpFileInfo *p);
EXPORT_SPEC const char *UpnpFileInfo_get_ExtraHeaders_cstr(const UpnpFileInfo *p);
EXPORT_SPEC void UpnpFileInfo_set_ExtraHeaders(UpnpFileInfo *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* FILEINFO_H */

