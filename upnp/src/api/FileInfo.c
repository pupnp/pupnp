

#include "config.h"


#include "FileInfo.h"


#include <stdlib.h> // for calloc(), free()
#include <string.h> // for strlen(), strdup()


/** This is a private struct, it belongs only to this file. */
struct SUpnpFileInfo
{
	off_t m_fileLength;
	time_t m_lastModified;
	int m_isDirectory;
	int m_isReadable;
	DOMString m_contentType;
	DOMString m_extraHeaders;
};


UpnpFileInfo *UpnpFileInfo_new()
{
	struct SUpnpFileInfo *p = calloc(1, sizeof (struct SUpnpFileInfo));

#if 0
	p->m_fileLength = 0;
	p->m_lastModified = 0;
	p->m_isDirectory = 0;
	p->m_isReadable = 0;
	p->m_contentType = NULL;
	p->m_extraHeaders = NULL;
#endif

	return (UpnpFileInfo *)p;
}


void UpnpFileInfo_delete(UpnpFileInfo *p)
{
	struct SUpnpFileInfo *q = (struct SUpnpFileInfo *)p;

	q->m_fileLength = 0;

	q->m_lastModified = 0;

	q->m_isDirectory = 0;

	q->m_isReadable = 0;

	ixmlFreeDOMString(q->m_contentType);
	q->m_contentType = NULL;
	
	ixmlFreeDOMString(q->m_extraHeaders);
	q->m_extraHeaders = NULL;

	free(p);
}


UpnpFileInfo *UpnpFileInfo_dup(const UpnpFileInfo *p)
{
	UpnpFileInfo *q = UpnpFileInfo_new();

	UpnpFileInfo_assign(q, p);

	return q;
}


void UpnpFileInfo_assign(UpnpFileInfo *q, const UpnpFileInfo *p)
{
	if (q != p) {
		UpnpFileInfo_set_FileLength(q, UpnpFileInfo_get_FileLength(p));
		UpnpFileInfo_set_LastModified(q, UpnpFileInfo_get_LastModified(p));
		UpnpFileInfo_set_IsDirectory(q, UpnpFileInfo_get_IsDirectory(p));
		UpnpFileInfo_set_IsReadable(q, UpnpFileInfo_get_IsReadable(p));
		UpnpFileInfo_set_ContentType(q, UpnpFileInfo_get_ContentType(p));
		UpnpFileInfo_set_ExtraHeaders(q, UpnpFileInfo_get_ExtraHeaders(p));
	}
}


off_t UpnpFileInfo_get_FileLength(const UpnpFileInfo *p)
{
	return ((struct SUpnpFileInfo *)p)->m_fileLength;
}


void UpnpFileInfo_set_FileLength(UpnpFileInfo *p, off_t l)
{
	((struct SUpnpFileInfo *)p)->m_fileLength = l;
}


const time_t *UpnpFileInfo_get_LastModified(const UpnpFileInfo *p)
{
	return &((struct SUpnpFileInfo *)p)->m_lastModified;
}


void UpnpFileInfo_set_LastModified(UpnpFileInfo *p, const time_t *t)
{
	((struct SUpnpFileInfo *)p)->m_lastModified = *t;
}


int UpnpFileInfo_get_IsDirectory(const UpnpFileInfo *p)
{
	return ((struct SUpnpFileInfo *)p)->m_isDirectory;
}


void UpnpFileInfo_set_IsDirectory(UpnpFileInfo *p, int b)
{
	((struct SUpnpFileInfo *)p)->m_isDirectory = b;
}


int UpnpFileInfo_get_IsReadable(const UpnpFileInfo *p)
{
	return ((struct SUpnpFileInfo *)p)->m_isReadable;
}


void UpnpFileInfo_set_IsReadable(UpnpFileInfo *p, int b)
{
	((struct SUpnpFileInfo *)p)->m_isReadable = b;
}


const DOMString UpnpFileInfo_get_ContentType(const UpnpFileInfo *p)
{
	return ((struct SUpnpFileInfo *)p)->m_contentType;
}


void UpnpFileInfo_set_ContentType(UpnpFileInfo *p, const DOMString s)
{
	ixmlFreeDOMString(((struct SUpnpFileInfo *)p)->m_contentType);
	((struct SUpnpFileInfo *)p)->m_contentType = ixmlCloneDOMString(s);
}


const DOMString UpnpFileInfo_get_ExtraHeaders(const UpnpFileInfo *p)
{
	return ((struct SUpnpFileInfo *)p)->m_extraHeaders;
}


void UpnpFileInfo_set_ExtraHeaders(UpnpFileInfo *p, const DOMString s)
{
	ixmlFreeDOMString(((struct SUpnpFileInfo *)p)->m_extraHeaders);
	((struct SUpnpFileInfo *)p)->m_extraHeaders = ixmlCloneDOMString(s);
}

