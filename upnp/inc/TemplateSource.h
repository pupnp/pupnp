/*
 * C Template objects.
 *
 * Copyright (C) 2010 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
 */

#ifndef TEMPLATESOURCE_H
#define TEMPLATESOURCE_H

/*!
 * \file
 *
 * \brief Templates for source files of objects.
 *
 * Usage:
 *
 * - In the include file Token.h:
 *	#include "Token_def.h"
 *	#include "TemplateInclude.h"
 *
 * - In the source file Token.c:
 *	#include "Token.h"
 *	#include "TemplateSource.h"
 *
 * \author Marcelo Roberto Jimenez
 */


/******************************************************************************/
#define TEMPLATE_DEFINITION_INT(MEMBER, TYPE)		TYPE m_##MEMBER;
#define TEMPLATE_DEFINITION_BUFFER(MEMBER, TYPE)	TYPE m_##MEMBER;
#define TEMPLATE_DEFINITION_LIST(MEMBER)		struct list_head m_##MEMBER;
#define TEMPLATE_DEFINITION_OBJECT(MEMBER, TYPE)	TYPE *m_##MEMBER;
#define TEMPLATE_DEFINITION_STRING(MEMBER)		UpnpString *m_##MEMBER;
#define TEMPLATE_DEFINITION_DOMSTRING(MEMBER)		DOMString m_##MEMBER;

/******************************************************************************/
#define TEMPLATE_CONSTRUCTOR_INT(MEMBER, TYPE)		/* p->m_##MEMBER = 0; */
#define TEMPLATE_CONSTRUCTOR_BUFFER(MEMBER, TYPE)	\
	/* memset(&p->m_##MEMBER, 0, sizeof (TYPE)); */
#define TEMPLATE_CONSTRUCTOR_LIST(MEMBER, TYPE)		INIT_LIST_HEAD(&p->m_##MEMBER);
#define TEMPLATE_CONSTRUCTOR_OBJECT(MEMBER, TYPE)	p->m_##MEMBER = TYPE##_new();
#define TEMPLATE_CONSTRUCTOR_STRING(MEMBER)		p->m_##MEMBER = UpnpString_new();
#define TEMPLATE_CONSTRUCTOR_DOMSTRING(MEMBER)		 p->m_##MEMBER = NULL;

/******************************************************************************/
#define TEMPLATE_DESTRUCTOR_INT(MEMBER, TYPE)		p->m_##MEMBER = 0;
#define TEMPLATE_DESTRUCTOR_BUFFER(MEMBER, TYPE)	memset(&p->m_##MEMBER, 0, sizeof (TYPE));
#define TEMPLATE_DESTRUCTOR_LIST(MEMBER)		list_del(&p->m_##MEMBER);
#define TEMPLATE_DESTRUCTOR_OBJECT(MEMBER, TYPE)	TYPE##_delete(p->m_##MEMBER); p->m_##MEMBER = NULL;
#define TEMPLATE_DESTRUCTOR_STRING(MEMBER)		UpnpString_delete(p->m_##MEMBER); p->m_##MEMBER = NULL;
#define TEMPLATE_DESTRUCTOR_DOMSTRING(MEMBER)		ixmlFreeDOMString(p->m_##MEMBER); p->m_##MEMBER = NULL;

/******************************************************************************/
#define TEMPLATE_ASSIGNMENT(CLASS, MEMBER)	ok = ok && CLASS##_set_##MEMBER(p, CLASS##_get_##MEMBER(q));


/******************************************************************************/
#define TEMPLATE_METHODS_INT(CLASS, MEMBER, TYPE) \
	TEMPLATE_METHODS_INT_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_METHODS_INT_AUX(CLASS, MEMBER, TYPE) \
TYPE CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return ((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
int CLASS##_set_##MEMBER(CLASS *p, TYPE n) \
{ \
	((struct S##CLASS *)p)->m_##MEMBER = n; \
	return 1; \
} \


/******************************************************************************/
#define TEMPLATE_METHODS_BUFFER(CLASS, MEMBER, TYPE) \
	TEMPLATE_METHODS_BUFFER_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_METHODS_BUFFER_AUX(CLASS, MEMBER, TYPE) \
const TYPE *CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return (TYPE *)&((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
int CLASS##_set_##MEMBER(CLASS *p, const TYPE *buf) \
{ \
	((struct S##CLASS *)p)->m_##MEMBER = *(TYPE *)buf; \
	return 1; \
} \
\
void CLASS##_clear_##MEMBER(CLASS *p) \
{ \
	memset(&((struct S##CLASS *)p)->m_##MEMBER, 0, sizeof(TYPE)); \
} \


/******************************************************************************/
#define TEMPLATE_METHODS_LIST(CLASS, MEMBER) \
	TEMPLATE_METHODS_LIST_AUX(CLASS, MEMBER)
#define TEMPLATE_METHODS_LIST_AUX(CLASS, MEMBER) \
const struct list_head *CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return (struct list_head *)&((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
void CLASS##_add_to_list_##MEMBER(CLASS *p, struct list_head *head) \
{ \
	list_add(&((struct S##CLASS *)p)->m_##MEMBER, head); \
} \
\
void CLASS##_remove_from_list_##MEMBER(CLASS *p) \
{ \
	list_del_init(&((struct S##CLASS *)p)->m_##MEMBER); \
} \
\
void CLASS##_replace_in_list_##MEMBER(CLASS *p, struct list_head *new) \
{ \
	list_replace_init(&((struct S##CLASS *)p)->m_##MEMBER, new); \
} \


/******************************************************************************/
#define TEMPLATE_METHODS_OBJECT(CLASS, MEMBER, TYPE) \
	TEMPLATE_METHODS_OBJECT_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_METHODS_OBJECT_AUX(CLASS, MEMBER, TYPE) \
const TYPE *CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return ((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
int CLASS##_set_##MEMBER(CLASS *p, const TYPE *s) \
{ \
	TYPE *q = TYPE##_dup(s); \
	if (!q) return 0; \
	TYPE##_delete(((struct S##CLASS *)p)->m_##MEMBER); \
	((struct S##CLASS *)p)->m_##MEMBER = q; \
	return 1; \
} \


/******************************************************************************/
#define TEMPLATE_METHODS_STRING(CLASS, MEMBER) \
	TEMPLATE_METHODS_STRING_AUX(CLASS, MEMBER)
#define TEMPLATE_METHODS_STRING_AUX(CLASS, MEMBER) \
const UpnpString *CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return ((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
int CLASS##_set_##MEMBER(CLASS *p, const UpnpString *s) \
{ \
	const char *q = UpnpString_get_String(s); \
	return UpnpString_set_String(((struct S##CLASS *)p)->m_##MEMBER, q); \
} \
\
size_t CLASS##_get_##MEMBER##_Length(const CLASS *p) \
{ \
	return UpnpString_get_Length(CLASS##_get_##MEMBER(p)); \
} \
const char *CLASS##_get_##MEMBER##_cstr(const CLASS *p) \
{ \
	return UpnpString_get_String(CLASS##_get_##MEMBER(p)); \
} \
\
int CLASS##_strcpy_##MEMBER(CLASS *p, const char *s) \
{ \
	return UpnpString_set_String(((struct S##CLASS *)p)->m_##MEMBER, s); \
} \
\
int CLASS##_strncpy_##MEMBER(CLASS *p, const char *s, size_t n) \
{ \
	return UpnpString_set_StringN(((struct S##CLASS *)p)->m_##MEMBER, s, n); \
} \
\
void CLASS##_clear_##MEMBER(CLASS *p) \
{ \
	UpnpString_clear(((struct S##CLASS *)p)->m_##MEMBER); \
} \


/******************************************************************************/
#define TEMPLATE_METHODS_DOMSTRING(CLASS, MEMBER) \
	TEMPLATE_METHODS_DOMSTRING_AUX(CLASS, MEMBER)
#define TEMPLATE_METHODS_DOMSTRING_AUX(CLASS, MEMBER) \
const DOMString CLASS##_get_##MEMBER(const CLASS *p) \
{ \
	return ((struct S##CLASS *)p)->m_##MEMBER; \
} \
\
int CLASS##_set_##MEMBER(CLASS *p, const DOMString s) \
{ \
	DOMString q = ixmlCloneDOMString(s); \
	if (!q) return 0; \
	ixmlFreeDOMString(((struct S##CLASS *)p)->m_##MEMBER); \
	((struct S##CLASS *)p)->m_##MEMBER = q; \
	return 1; \
} \
\
const char *CLASS##_get_##MEMBER##_cstr(const CLASS *p) \
{ \
	return (const char *)CLASS##_get_##MEMBER(p); \
} \


/******************************************************************************
 *
 * Actual source starts here.
 *
 ******************************************************************************/

#include "config.h"

#include <stdlib.h> /* for calloc(), free() */	
#include <string.h> /* for strlen(), strdup() */

/******************************************************************************/
#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_DEFINITION_INT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_DEFINITION_BUFFER(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		TEMPLATE_DEFINITION_LIST(MEMBER)
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_DEFINITION_OBJECT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_DEFINITION_STRING(MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_DEFINITION_DOMSTRING(MEMBER)

#define TEMPLATE_DECLARATION_STRUCT(CLASS) \
	TEMPLATE_DECLARATION_STRUCT_AUX(CLASS)
#define TEMPLATE_DECLARATION_STRUCT_AUX(CLASS) \
struct S##CLASS { \
	EXPAND_CLASS_MEMBERS(CLASS) \
};

TEMPLATE_DECLARATION_STRUCT(CLASS)

#include "TemplateUndef.h"

/******************************************************************************/
#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_CONSTRUCTOR_INT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_CONSTRUCTOR_BUFFER(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		TEMPLATE_CONSTRUCTOR_LIST(MEMBER, MEMBER)
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_CONSTRUCTOR_OBJECT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_CONSTRUCTOR_STRING(MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_CONSTRUCTOR_DOMSTRING(MEMBER)

#define TEMPLATE_DEFINITION_CONSTRUCTOR(CLASS) \
	TEMPLATE_DEFINITION_CONSTRUCTOR_AUX(CLASS)
#define TEMPLATE_DEFINITION_CONSTRUCTOR_AUX(CLASS) \
CLASS *CLASS##_new() \
{ \
	struct S##CLASS *p = calloc(1, sizeof (struct S##CLASS)); \
\
	if (!p) return NULL; \
\
	EXPAND_CLASS_MEMBERS(CLASS) \
\
	return (CLASS *)p; \
}

TEMPLATE_DEFINITION_CONSTRUCTOR(CLASS)

#include "TemplateUndef.h"

/******************************************************************************/
#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_DESTRUCTOR_INT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_DESTRUCTOR_BUFFER(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		TEMPLATE_DESTRUCTOR_LIST(MEMBER)
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_DESTRUCTOR_OBJECT(MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_DESTRUCTOR_STRING(MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_DESTRUCTOR_DOMSTRING(MEMBER)

#define TEMPLATE_DEFINITION_DESTRUCTOR(CLASS) \
	TEMPLATE_DEFINITION_DESTRUCTOR_AUX(CLASS)
#define TEMPLATE_DEFINITION_DESTRUCTOR_AUX(CLASS) \
void CLASS##_delete(CLASS *q) \
{ \
	struct S##CLASS *p = (struct S##CLASS *)q; \
\
	if (!p) return; \
\
	EXPAND_CLASS_MEMBERS(CLASS) \
\
	free(p); \
}

TEMPLATE_DEFINITION_DESTRUCTOR(CLASS)

#include "TemplateUndef.h"

/******************************************************************************/
#define TEMPLATE_DEFINITION_COPY_CONSTRUCTOR(CLASS) \
	TEMPLATE_DEFINITION_COPY_CONSTRUCTOR_AUX(CLASS)
#define TEMPLATE_DEFINITION_COPY_CONSTRUCTOR_AUX(CLASS) \
CLASS *CLASS##_dup(const CLASS *q) \
{ \
	CLASS *p = CLASS##_new(); \
\
	if (!p) return NULL; \
\
	CLASS##_assign(p, q); \
\
	return p; \
}

TEMPLATE_DEFINITION_COPY_CONSTRUCTOR(CLASS)

/******************************************************************************/
#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_ASSIGNMENT(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_ASSIGNMENT(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		/* Do not assing. */
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_ASSIGNMENT(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_ASSIGNMENT(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_ASSIGNMENT(CLASS, MEMBER)

#define TEMPLATE_DEFINITION_ASSIGNMENT(CLASS) \
	TEMPLATE_DEFINITION_ASSIGNMENT_AUX(CLASS)
#define TEMPLATE_DEFINITION_ASSIGNMENT_AUX(CLASS) \
int CLASS##_assign(CLASS *p, const CLASS *q) \
{ \
	int ok = 1; \
	if (p != q) { \
		EXPAND_CLASS_MEMBERS(CLASS) \
	} \
	return ok; \
}

TEMPLATE_DEFINITION_ASSIGNMENT(CLASS)

#include "TemplateUndef.h"

/******************************************************************************/
#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_METHODS_INT(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_METHODS_BUFFER(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		TEMPLATE_METHODS_LIST(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_METHODS_OBJECT(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_METHODS_STRING(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_METHODS_DOMSTRING(CLASS, MEMBER)

EXPAND_CLASS_MEMBERS(CLASS)

#include "TemplateUndef.h"


#endif /* TEMPLATESOURCE_H */

