/*
 * C Template objects.
 *
 * Copyright (C) 2010 Marcelo Roberto Jimenez <mroberto@users.sourceforge.net>
 */

#ifndef TEMPLATEINCLUDE_H
#define TEMPLATEINCLUDE_H


/*!
 * \file
 *
 * \brief Templates for include files of objects.
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
#define TEMPLATE_PROTOTYPE_COMMON(CLASS) \
	TEMPLATE_PROTOTYPE_COMMON_AUX(CLASS)
#define TEMPLATE_PROTOTYPE_COMMON_AUX(CLASS) \
	/*!
	 * DOC_##CLASS
	 */ \
	typedef struct s_##CLASS CLASS; \
	\
	/*! Constructor */ \
	EXPORT_SPEC CLASS *CLASS##_new(); \
	\
	/*! Destructor */ \
	EXPORT_SPEC void CLASS##_delete(CLASS *p); \
	\
	/*! Copy Constructor */ \
	EXPORT_SPEC CLASS *CLASS##_dup(const CLASS *p); \
	\
	/*! Assignment operator */ \
	EXPORT_SPEC int CLASS##_assign(CLASS *p, const CLASS *q); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_INT(CLASS, MEMBER, TYPE) \
	TEMPLATE_PROTOTYPE_INT_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_PROTOTYPE_INT_AUX(CLASS, MEMBER, TYPE) \
	/*! DOC_##CLASS##_##MEMBER */ \
	EXPORT_SPEC TYPE CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC int CLASS##_set_##MEMBER(CLASS *p, TYPE n); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_BUFFER(CLASS, MEMBER, TYPE) \
	TEMPLATE_PROTOTYPE_BUFFER_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_PROTOTYPE_BUFFER_AUX(CLASS, MEMBER, TYPE) \
	/*! DOC_##CLASS_##MEMBER */ \
	EXPORT_SPEC const TYPE *CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC int CLASS##_set_##MEMBER(CLASS *p, const TYPE *buf); \
	EXPORT_SPEC void CLASS##_clear_##MEMBER(CLASS *p); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_LIST(CLASS, MEMBER) \
	TEMPLATE_PROTOTYPE_LIST_AUX(CLASS, MEMBER)
#define TEMPLATE_PROTOTYPE_LIST_AUX(CLASS, MEMBER) \
	/*! DOC_##CLASS_##MEMBER */ \
	EXPORT_SPEC const struct list_head *CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC void CLASS##_add_to_list_##MEMBER(CLASS *p, struct list_head *head); \
	EXPORT_SPEC void CLASS##_remove_from_list_##MEMBER(CLASS *p); \
	EXPORT_SPEC void CLASS##_replace_in_list_##MEMBER(CLASS *p, struct list_head *new); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_OBJECT(CLASS, MEMBER, TYPE) \
	TEMPLATE_PROTOTYPE_OBJECT_AUX(CLASS, MEMBER, TYPE)
#define TEMPLATE_PROTOTYPE_OBJECT_AUX(CLASS, MEMBER, TYPE) \
	/*! DOC_##CLASS##_##MEMBER */ \
	EXPORT_SPEC const TYPE *CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC int CLASS##_set_##MEMBER(CLASS *p, const TYPE *n); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_STRING(CLASS, MEMBER) \
	TEMPLATE_PROTOTYPE_STRING_AUX(CLASS, MEMBER)
#define TEMPLATE_PROTOTYPE_STRING_AUX(CLASS, MEMBER) \
	/*! DOC_##CLASS##_##MEMBER */ \
	EXPORT_SPEC const UpnpString *CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC int CLASS##_set_##MEMBER(CLASS *p, const UpnpString *s); \
	EXPORT_SPEC size_t CLASS##_get_##MEMBER##_Length(const CLASS *p); \
	EXPORT_SPEC const char *CLASS##_get_##MEMBER##_cstr(const CLASS *p); \
	EXPORT_SPEC int CLASS##_strcpy_##MEMBER(CLASS *p, const char *s); \
	EXPORT_SPEC int CLASS##_strncpy_##MEMBER(CLASS *p, const char *s, size_t n); \
	EXPORT_SPEC void CLASS##_clear_##MEMBER(CLASS *p); \


/******************************************************************************/
#define TEMPLATE_PROTOTYPE_DOMSTRING(CLASS, MEMBER) \
	TEMPLATE_PROTOTYPE_DOMSTRING_AUX(CLASS, MEMBER)
#define TEMPLATE_PROTOTYPE_DOMSTRING_AUX(CLASS, MEMBER) \
	/*! DOC_##CLASS_##MEMBER */ \
	EXPORT_SPEC const DOMString CLASS##_get_##MEMBER(const CLASS *p); \
	EXPORT_SPEC int CLASS##_set_##MEMBER(CLASS *p, const DOMString s); \
	EXPORT_SPEC const char *CLASS##_get_##MEMBER##_cstr(const CLASS *p); \


#endif /* TEMPLATEINCLUDE_H */


/******************************************************************************
 *
 * Actual source starts here.
 *
 ******************************************************************************/


#include <stdlib.h> /* for size_t */


#include "ixml.h"       /* for DOMString, IXML_Document */
#include "list.h"	/* for struct list_head */
#include "UpnpGlobal.h" /* for EXPORT_SPEC */
#include "UpnpString.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

TEMPLATE_PROTOTYPE_COMMON(CLASS)

#define EXPAND_CLASS_MEMBER_INT(CLASS, MEMBER, TYPE)	TEMPLATE_PROTOTYPE_INT(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_BUFFER(CLASS, MEMBER, TYPE)	TEMPLATE_PROTOTYPE_BUFFER(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_LIST(CLASS, MEMBER)		TEMPLATE_PROTOTYPE_LIST(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_OBJECT(CLASS, MEMBER, TYPE)	TEMPLATE_PROTOTYPE_OBJECT(CLASS, MEMBER, TYPE)
#define EXPAND_CLASS_MEMBER_STRING(CLASS, MEMBER)	TEMPLATE_PROTOTYPE_STRING(CLASS, MEMBER)
#define EXPAND_CLASS_MEMBER_DOMSTRING(CLASS, MEMBER)	TEMPLATE_PROTOTYPE_DOMSTRING(CLASS, MEMBER)

EXPAND_CLASS_MEMBERS(CLASS)

#include "TemplateUndef.h"


#ifdef __cplusplus
}
#endif /* __cplusplus */


#ifdef TEMPLATE_GENERATE_SOURCE
	#include "TemplateSource.h"
#endif /* TEMPLATE_GENERATE_SOURCE */

/* Cleanup the template mess. */
#undef PREFIX
#undef CLASS
#undef EXPAND_CLASS_MEMBERS

