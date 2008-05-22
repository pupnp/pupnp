

#ifndef FILEINFO_H
#define FILEINFO_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Detailed description of this class should go here */
typedef struct s_UpnpFileInfo UpnpFileInfo;


#include "ixml.h"       // for DOMString


#include <sys/types.h>  // for off_t
#include <time.h>       // for time_t


/** Constructor */
UpnpFileInfo *UpnpFileInfo_new();

/** Destructor */
void UpnpFileInfo_delete(UpnpFileInfo *p);

/** Copy Constructor */
UpnpFileInfo *UpnpFileInfo_dup(const UpnpFileInfo *p);

/** Assignment operator */
void UpnpFileInfo_assign(UpnpFileInfo *q, const UpnpFileInfo *p);

/** The length of the file. A length less than 0 indicates the size 
 *  is unknown, and data will be sent until 0 bytes are returned from
 *  a read call. */
off_t UpnpFileInfo_get_FileLength(const UpnpFileInfo *p);
void UpnpFileInfo_set_FileLength(UpnpFileInfo *p, off_t l);

/** The time at which the contents of the file was modified;
 *  The time system is always local (not GMT). */
const time_t *UpnpFileInfo_get_LastModified(const UpnpFileInfo *p);
void UpnpFileInfo_set_LastModified(UpnpFileInfo *p, const time_t *t);

/** If the file is a directory, {\bf is_directory} contains
 * a non-zero value. For a regular file, it should be 0. */
int UpnpFileInfo_get_IsDirectory(const UpnpFileInfo *p);
void UpnpFileInfo_set_IsDirectory(UpnpFileInfo *p, int b);

/** If the file or directory is readable, this contains 
 * a non-zero value. If unreadable, it should be set to 0. */
int UpnpFileInfo_get_IsReadable(const UpnpFileInfo *p);
void UpnpFileInfo_set_IsReadable(UpnpFileInfo *p, int b);

/** The content type of the file. */
const DOMString UpnpFileInfo_get_ContentType(const UpnpFileInfo *p);
void UpnpFileInfo_set_ContentType(UpnpFileInfo *p, const DOMString s);

/** Additional HTTP headers to return. Each header line should be
 *  followed by "\r\n". */
const DOMString UpnpFileInfo_get_ExtraHeaders(const UpnpFileInfo *p);
void UpnpFileInfo_set_ExtraHeaders(UpnpFileInfo *p, const DOMString s);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* FILEINFO_H */

