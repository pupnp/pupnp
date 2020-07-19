/*
 * Automatic generation of UPNP classes.
 *
 * Copyright (C) Marcelo Roberto Jimenez <marcelo.jimenez@gmail.com>
 *
 */
#include "generator.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IXML_H "ixml.h"

static struct s_Member UpnpActionComplete_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(CtrlUrl, TYPE_STRING, 0, 0),
	INIT_MEMBER(ActionRequest, TYPE_INTEGER, IXML_Document *, IXML_H),
	INIT_MEMBER(ActionResult, TYPE_INTEGER, IXML_Document *, IXML_H),
};

static struct s_Member UpnpActionRequest_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(Socket, TYPE_INTEGER, int, 0),
	INIT_MEMBER(ErrStr, TYPE_STRING, 0, 0),
	INIT_MEMBER(ActionName, TYPE_STRING, 0, 0),
	INIT_MEMBER(DevUDN, TYPE_STRING, 0, 0),
	INIT_MEMBER(ServiceID, TYPE_STRING, 0, 0),
	INIT_MEMBER(ActionRequest, TYPE_INTEGER, IXML_Document *, IXML_H),
	INIT_MEMBER(ActionResult, TYPE_INTEGER, IXML_Document *, IXML_H),
	INIT_MEMBER(SoapHeader, TYPE_INTEGER, IXML_Document *, IXML_H),
	INIT_MEMBER(CtrlPtIPAddr,
		TYPE_BUFFER,
		struct sockaddr_storage,
		"UpnpInet.h"),
	INIT_MEMBER(Os, TYPE_STRING, 0, 0),
};

static struct s_Member UpnpDiscovery_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(Expires, TYPE_INTEGER, int, 0),
	INIT_MEMBER(DeviceID, TYPE_STRING, 0, 0),
	INIT_MEMBER(DeviceType, TYPE_STRING, 0, 0),
	INIT_MEMBER(ServiceType, TYPE_STRING, 0, 0),
	INIT_MEMBER(ServiceVer, TYPE_STRING, 0, 0),
	INIT_MEMBER(Location, TYPE_STRING, 0, 0),
	INIT_MEMBER(Os, TYPE_STRING, 0, 0),
	INIT_MEMBER(Date, TYPE_STRING, 0, 0),
	INIT_MEMBER(Ext, TYPE_STRING, 0, 0),
	INIT_MEMBER(
		DestAddr, TYPE_BUFFER, struct sockaddr_storage, "UpnpInet.h"),
};

static struct s_Member UpnpEvent_members[] = {
	INIT_MEMBER(EventKey, TYPE_INTEGER, int, 0),
	INIT_MEMBER(ChangedVariables, TYPE_INTEGER, IXML_Document *, IXML_H),
	INIT_MEMBER(SID, TYPE_STRING, 0, 0),
};

static struct s_Member UpnpEventSubscribe_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(TimeOut, TYPE_INTEGER, int, 0),
	INIT_MEMBER(SID, TYPE_STRING, 0, 0),
	INIT_MEMBER(PublisherUrl, TYPE_STRING, 0, 0),
};

static struct s_Member UpnpExtraHeaders_members[] = {
	INIT_MEMBER(node, TYPE_LIST, 0, 0),
	INIT_MEMBER(name, TYPE_STRING, 0, 0),
	INIT_MEMBER(value, TYPE_STRING, 0, 0),
	INIT_MEMBER(resp, TYPE_DOMSTRING, 0, 0),
};

static struct s_Member UpnpFileInfo_members[] = {
	INIT_MEMBER(FileLength, TYPE_INTEGER, off_t, "<sys/types.h>"),
	INIT_MEMBER(LastModified, TYPE_INTEGER, time_t, "<time.h>"),
	INIT_MEMBER(IsDirectory, TYPE_INTEGER, int, 0),
	INIT_MEMBER(IsReadable, TYPE_INTEGER, int, 0),
	INIT_MEMBER(ContentType, TYPE_DOMSTRING, 0, 0),
	INIT_MEMBER(ExtraHeadersList, TYPE_LIST, 0, 0),
	INIT_MEMBER(CtrlPtIPAddr,
		TYPE_BUFFER,
		struct sockaddr_storage,
		"UpnpInet.h"),
	INIT_MEMBER(Os, TYPE_STRING, 0, 0),
};

static struct s_Member UpnpStateVarComplete_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(CtrlUrl, TYPE_STRING, 0, 0),
	INIT_MEMBER(StateVarName, TYPE_STRING, 0, 0),
	INIT_MEMBER(CurrentVal, TYPE_DOMSTRING, 0, 0),
};

static struct s_Member UpnpStateVarRequest_members[] = {
	INIT_MEMBER(ErrCode, TYPE_INTEGER, int, 0),
	INIT_MEMBER(Socket, TYPE_INTEGER, int, 0),
	INIT_MEMBER(ErrStr, TYPE_STRING, 0, 0),
	INIT_MEMBER(DevUDN, TYPE_STRING, 0, 0),
	INIT_MEMBER(ServiceID, TYPE_STRING, 0, 0),
	INIT_MEMBER(StateVarName, TYPE_STRING, 0, 0),
	INIT_MEMBER(CtrlPtIPAddr,
		TYPE_BUFFER,
		struct sockaddr_storage,
		"UpnpInet.h"),
	INIT_MEMBER(CurrentVal, TYPE_DOMSTRING, 0, 0),
};

static struct s_Member UpnpSubscriptionRequest_members[] = {
	INIT_MEMBER(ServiceId, TYPE_STRING, 0, 0),
	INIT_MEMBER(UDN, TYPE_STRING, 0, 0),
	INIT_MEMBER(SID, TYPE_STRING, 0, 0),
};

static struct s_Member GenlibClientSubscription_members[] = {
	INIT_MEMBER(RenewEventId, TYPE_INTEGER, int, 0),
	INIT_MEMBER(SID, TYPE_STRING, 0, 0),
	INIT_MEMBER(ActualSID, TYPE_STRING, 0, 0),
	INIT_MEMBER(EventURL, TYPE_STRING, 0, 0),
	INIT_MEMBER(Next, TYPE_INTEGER, GenlibClientSubscription *, 0),
};

static struct s_Member SSDPResultData_members[] = {
	INIT_MEMBER(Param, TYPE_CLASS, UpnpDiscovery, "UpnpDiscovery.h"),
	INIT_MEMBER(Cookie, TYPE_INTEGER, void *, 0),
	INIT_MEMBER(CtrlptCallback, TYPE_INTEGER, Upnp_FunPtr, "Callback.h"),
};

static struct s_Member TestClass_members[] = {
	INIT_MEMBER(TheList, TYPE_LIST, 0, 0),
	INIT_MEMBER(TheInteger, TYPE_INTEGER, int, 0),
	INIT_MEMBER(TheUnsignedLong, TYPE_INTEGER, unsigned long, 0),
	INIT_MEMBER(TheCharPointer, TYPE_INTEGER, char *, 0),
	INIT_MEMBER(TheBuffer, TYPE_BUFFER, struct TheStruct, "TheStruct.h"),
	INIT_MEMBER(TheActionComplete,
		TYPE_CLASS,
		UpnpActionComplete,
		"UpnpActionComplete.h"),
	INIT_MEMBER(TheString, TYPE_STRING, 0, 0),
	INIT_MEMBER(TheDomString, TYPE_DOMSTRING, 0, 0),
};

static struct s_Class my_classes[] = {
	INIT_CLASS(UpnpActionComplete),
	INIT_CLASS(UpnpActionRequest),
	INIT_CLASS(UpnpDiscovery),
	INIT_CLASS(UpnpEvent),
	INIT_CLASS(UpnpEventSubscribe),
	INIT_CLASS(UpnpExtraHeaders),
	INIT_CLASS(UpnpFileInfo),
	INIT_CLASS(UpnpStateVarComplete),
	INIT_CLASS(UpnpStateVarRequest),
	INIT_CLASS(UpnpSubscriptionRequest),
	INIT_CLASS(GenlibClientSubscription),
	INIT_CLASS(SSDPResultData),
	INIT_CLASS(TestClass),
};

static char *strupr(char *s)
{
	char *p = s;
	for (; *p; ++p) {
		*p = toupper(*p);
	}

	return s;
}

static int write_prototype(FILE *fp, const char *class_name, struct s_Member *m)
{
	switch (m->type) {
	case TYPE_CLASS:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC const %s *%s_get_%s(const %s *p);\n"
			"/*! %s_set_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, const %s *n);\n"
			"\n",
			class_name, m->name,
			m->type_name, class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name, m->type_name
		);
		/* clang-format on */
		break;
	case TYPE_INTEGER:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC %s %s_get_%s(const %s *p);\n"
			"/*! %s_set_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, %s n);\n"
			"\n",
			class_name, m->name,
			m->type_name, class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name, m->type_name
		);
		/* clang-format on */
		break;
	case TYPE_BUFFER:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC const %s *%s_get_%s(const %s *p);\n"
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, const %s *buf); \n"
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC void %s_clear_%s(%s *p); \n"
			"\n",
			class_name, m->name,
			m->type_name, class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name, m->type_name,
			class_name, m->name,
			class_name, m->name, class_name
		);
		/* clang-format on */
		break;
	case TYPE_LIST:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC const UpnpListHead *%s_get_%s(const %s *p);\n"
			"/*! %s_set_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, const UpnpListHead *q);\n"
			"/*! %s_add_to_list_%s */\n"
			"EXPORT_SPEC void %s_add_to_list_%s(%s *p, UpnpListHead *head);\n"
			"\n",
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name
		);
		/* clang-format on */
		break;
	case TYPE_STRING:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC const UpnpString *%s_get_%s(const %s *p);\n"
			"/*! %s_set_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, const UpnpString *s);\n"
			"/*! %s_get_%s_Length */\n"
			"EXPORT_SPEC size_t %s_get_%s_Length(const %s *p);\n"
			"/*! %s_get_%s_cstr */\n"
			"EXPORT_SPEC const char *%s_get_%s_cstr(const %s *p);\n"
			"/*! %s_strcpy_%s */\n"
			"EXPORT_SPEC int %s_strcpy_%s(%s *p, const char *s);\n"
			"/*! %s_strncpy_%s */\n"
			"EXPORT_SPEC int %s_strncpy_%s(%s *p, const char *s, size_t n); \n"
			"/*! %s_clear_%s */\n"
			"EXPORT_SPEC void %s_clear_%s(%s *p); \n"
			"\n",
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name
		);
		/* clang-format on */
		break;
	case TYPE_DOMSTRING:
		/* clang-format off */
		fprintf(fp,
			"/*! %s_get_%s */\n"
			"EXPORT_SPEC const DOMString %s_get_%s(const %s *p); \n"
			"/*! %s_set_%s */\n"
			"EXPORT_SPEC int %s_set_%s(%s *p, const DOMString s); \n"
			"/*! %s_get_%s_cstr */\n"
			"EXPORT_SPEC const char *%s_get_%s_cstr(const %s *p); \n"
			"\n",
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name
		);
		/* clang-format on */
		break;
	default:
		printf("%s(): Error! Unknown type: %d.\n", __func__, m->type);
		return 0;
	}

	return 1;
}

/* Returns the name of the include if needed, or 0 if not needed. */
static const char *member_needs_include(const struct s_Member *m)
{
	const char *ret = 0;

	if (m->header) {
		ret = m->header;
	} else {
		switch (m->type) {
		case TYPE_CLASS:
			break;
		case TYPE_INTEGER:
			break;
		case TYPE_BUFFER:
			break;
		case TYPE_LIST:
			ret = "list.h";
			break;
		case TYPE_STRING:
			ret = "UpnpString.h";
			break;
		case TYPE_DOMSTRING:
			ret = "ixml.h";
			break;
		default:
			printf("%s(): Error! Unknown type: %d.\n",
				__func__,
				m->type);
			return 0;
		}
	}

	return ret;
}

size_t included_headers_used;
size_t included_headers_size;
const char **included_headers;

/* Checks if the header has already been used and includes it in the array if
 * not. */
static int already_included(const char *header)
{
	int ret = 0;
	int i;

	/* Realloc if needed. */
	if (included_headers_used == included_headers_size) {
		included_headers_size += 16;
		included_headers = realloc(included_headers,
			included_headers_size * sizeof(const char *));
		if (!included_headers_size) {
			printf("%s(): Error! realloc() failed.\n", __func__);
			exit(1);
		}
	}
	i = 0;
	while (i < (int)included_headers_used) {
		if (!strcmp(header, included_headers[i])) {
			ret = 1;
			break;
		}
		++i;
	}
	if (!ret) {
		included_headers[included_headers_used] = header;
		++included_headers_used;
	}

	return ret;
}

static int write_header(FILE *fp, struct s_Class *c)
{
	int i;
	int ok = 1;
	const char *header;
	struct s_Member *m;
	int len = strlen(c->name);
	char *class_name_upr = malloc(len + 1);

	if (!class_name_upr) {
		return 0;
	}
	strupr(strcpy(class_name_upr, c->name));
	included_headers_used = 0;
	included_headers_size = 16;
	included_headers = calloc(included_headers_size, sizeof(const char *));
	fprintf(fp,
		"#ifndef %s_H\n"
		"#define %s_H\n"
		"\n"
		"/*!\n"
		" * \\file\n"
		" *\n"
		" * \\brief Header file for %s methods.\n"
		" *\n"
		" * Do not edit this file, it is automatically generated. "
		"Please look at generator.c.\n"
		" *\n"
		" * \\author Marcelo Roberto Jimenez\n"
		" */\n"
		"#include <stdlib.h> /* for size_t */\n"
		"\n"
		"#include \"UpnpGlobal.h\" /* for EXPORT_SPEC */\n"
		"\n",
		class_name_upr,
		class_name_upr,
		c->name);

	/* Include files: look for which members need includes and only include
	 * them once. */
	for (i = 0; i < (int)c->n_members; ++i) {
		m = c->members + i;
		header = member_needs_include(m);
		if (header && !already_included(header)) {
			if (header[0] == '<') {
				fprintf(fp, "#include %s\n", header);
			} else {
				fprintf(fp, "#include \"%s\"\n", header);
			}
		}
	}

	fprintf(fp,
		"\n"
		"#ifdef __cplusplus\n"
		"extern \"C\" {\n"
		"#endif /* __cplusplus */\n"
		"\n"
		"/*!\n"
		" * %s\n"
		" */\n"
		"typedef struct s_%s %s;\n"
		"\n"
		"/*! Constructor */\n"
		"EXPORT_SPEC %s *%s_new();\n"
		"/*! Destructor */\n"
		"EXPORT_SPEC void %s_delete(%s *p);\n"
		"/*! Copy Constructor */\n"
		"EXPORT_SPEC %s *%s_dup(const %s *p);\n"
		"/*! Assignment operator */\n"
		"EXPORT_SPEC int %s_assign(%s *p, const %s *q);\n"
		"\n",
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name);
	for (i = 0; i < (int)c->n_members; ++i) {
		ok = ok && write_prototype(fp, c->name, c->members + i);
	}
	fprintf(fp,
		"#ifdef __cplusplus\n"
		"}\n"
		"#endif /* __cplusplus */\n"
		"\n"
		"#endif /* %s_H */\n\n",
		class_name_upr);
	free(included_headers);
	free(class_name_upr);

	return ok;
}

static int write_definition(FILE *fp, struct s_Member *m)
{
	switch (m->type) {
	case TYPE_CLASS:
		fprintf(fp, "\t%s *m_%s;\n", m->type_name, m->name);
		break;
	case TYPE_INTEGER:
		fprintf(fp, "\t%s m_%s;\n", m->type_name, m->name);
		break;
	case TYPE_BUFFER:
		fprintf(fp, "\t%s m_%s;\n", m->type_name, m->name);
		break;
	case TYPE_LIST:
		fprintf(fp, "\tUpnpListHead m_%s;\n", m->name);
		break;
	case TYPE_STRING:
		fprintf(fp, "\tUpnpString *m_%s;\n", m->name);
		break;
	case TYPE_DOMSTRING:
		fprintf(fp, "\tDOMString m_%s;\n", m->name);
		break;
	default:
		printf("%s(): Error! Unknown type: %d.\n", __func__, m->type);
		return 0;
	}

	return 1;
}

static int write_constructor(FILE *fp, struct s_Class *c)
{
	int i;
	struct s_Member *m;

	/* clang-format off */
	fprintf(fp,
		"%s *%s_new()\n"
		"{\n"
		"\tstruct s_%s *p = calloc(1, sizeof (struct s_%s));\n"
		"\n"
		"\tif (!p) return 0;\n"
		"\n",
		c->name, c->name, c->name, c->name);
	/* clang-format on */
	for (i = 0; i < (int)c->n_members; ++i) {
		m = c->members + i;
		switch (m->type) {
		case TYPE_CLASS:
			fprintf(fp,
				"\tp->m_%s = %s_new();\n",
				m->name,
				m->type_name);
			break;
		case TYPE_INTEGER:
			fprintf(fp, "\t/*p->m_%s = 0;*/\n", m->name);
			break;
		case TYPE_BUFFER:
			fprintf(fp,
				"\t/* memset(&p->m_%s, 0, sizeof (%s)); */\n",
				m->name,
				m->type_name);
			break;
		case TYPE_LIST:
			fprintf(fp, "\tUpnpListInit(&p->m_%s);\n", m->name);
			break;
		case TYPE_STRING:
			fprintf(fp, "\tp->m_%s = UpnpString_new();\n", m->name);
			break;
		case TYPE_DOMSTRING:
			fprintf(fp, "\t/*p->m_%s = 0;*/\n", m->name);
			break;
		default:
			printf("%s(): Error! Unknown type: %d.\n",
				__func__,
				m->type);
			return 0;
		}
	}
	/* clang-format off */
	fprintf(fp,
		"\n"
		"\treturn (%s *)p;\n"
		"}\n",
		c->name);
	/* clang-format on */

	return 1;
}

static int write_destructor(FILE *fp, struct s_Class *c)
{
	int i;
	struct s_Member *m;

	/* clang-format off */
	fprintf(fp,
		"void %s_delete(%s *q)\n"
		"{\n"
		"\tstruct s_%s *p = (struct s_%s *)q;\n"
		"\n"
		"\tif (!p) return;\n"
		"\n",
		c->name, c->name, c->name, c->name
	);
	/* clang-format on */
	for (i = c->n_members - 1; i >= 0; --i) {
		m = c->members + i;
		switch (m->type) {
		case TYPE_CLASS:
			fprintf(fp,
				"\t%s_delete(p->m_%s);\n"
				"\tp->m_%s = 0;\n",
				m->type_name,
				m->name,
				m->name);
			break;
		case TYPE_INTEGER:
			fprintf(fp, "\tp->m_%s = 0;\n", m->name);
			break;
		case TYPE_BUFFER:
			fprintf(fp,
				"\tmemset(&p->m_%s, 0, sizeof (%s));\n",
				m->name,
				m->type_name);
			break;
		case TYPE_LIST:
			fprintf(fp, "\tUpnpListInit(&p->m_%s);\n", m->name);
			break;
		case TYPE_STRING:
			fprintf(fp,
				"\tUpnpString_delete(p->m_%s);\n"
				"\tp->m_%s = 0;\n",
				m->name,
				m->name);
			break;
		case TYPE_DOMSTRING:
			fprintf(fp,
				"\tixmlFreeDOMString(p->m_%s);\n"
				"\tp->m_%s = 0;\n",
				m->name,
				m->name);
			break;
		default:
			printf("%s(): Error! Unknown type: %d.\n",
				__func__,
				m->type);
			return 0;
		}
	}
	/* clang-format off */
	fprintf(fp,
		"\n"
		"\tfree(p);\n"
		"}\n"
	);
	/* clang-format on */

	return 1;
}

static int write_assignment_operator(FILE *fp, struct s_Class *c)
{
	int i;
	struct s_Member *m;

	fprintf(fp,
		"int %s_assign(%s *p, const %s *q)\n"
		"{\n"
		"\tint ok = 1;\n"
		"\n"
		"\tif (p != q) {\n",
		c->name,
		c->name,
		c->name);
	for (i = 0; i < (int)c->n_members; ++i) {
		m = c->members + i;
		fprintf(fp,
			"\t\tok = ok && %s_set_%s(p, %s_get_%s(q));\n",
			c->name,
			m->name,
			c->name,
			m->name);
	}
	fprintf(fp,
		"\t}\n"
		"\n"
		"\treturn ok;\n"
		"}\n");

	return 1;
}

static int write_copy_constructor(FILE *fp, struct s_Class *c)
{
	fprintf(fp,
		"%s *%s_dup(const %s *q)\n"
		"{\n"
		"	%s *p = %s_new();\n"
		"\n"
		"	if (!p) return 0;\n"
		"\n"
		"	%s_assign(p, q);\n"
		"\n"
		"	return p;\n"
		"}\n",
		c->name,
		c->name,
		c->name,
		c->name,
		c->name,
		c->name);

	return 1;
}

static int write_methods(FILE *fp, const char *class_name, struct s_Member *m)
{
	switch (m->type) {
	case TYPE_CLASS:
		/* clang-format off */
		fprintf(fp,
			"const %s *%s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, const %s *s)\n"
			"{\n"
			"\t%s *q = %s_dup(s);\n"
			"\tif (!q) return 0;\n"
			"\t%s_delete(p->m_%s);\n"
			"\tp->m_%s = q;\n"
			"\n"
			"\treturn 1;\n"
			"}\n"
			"\n",
			m->type_name, class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name, m->type_name,
			m->type_name, m->type_name,
			m->type_name, m->name,
			m->name
		);
		/* clang-format on */
		break;
	case TYPE_INTEGER:
		/* clang-format off */
		fprintf(fp,
			"%s %s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, %s n)\n"
			"{\n"
			"\tp->m_%s = n;\n"
			"\n"
			"\treturn 1;\n"
			"}\n"
			"\n",
			m->type_name, class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name, m->type_name,
			m->name
		);
		/* clang-format on */
		break;
	case TYPE_BUFFER:
		/* clang-format off */
		fprintf(fp,
			"const %s *%s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn &p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, const %s *buf)\n"
			"{\n"
			"\tp->m_%s = *buf;\n"
			"\n"
			"\treturn 1;\n"
			"}\n"
			"\n"
			"void %s_clear_%s(%s *p)\n"
			"{\n"
			"\tmemset(&p->m_%s, 0, sizeof(%s));\n"
			"}\n"
			"\n",
			m->type_name, class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name, m->type_name,
			m->name,
			class_name, m->name, class_name,
			m->name, m->type_name
		);
		/* clang-format on */
		break;
	case TYPE_LIST:
		/* clang-format off */
		fprintf(fp,
			"const UpnpListHead *%s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn &p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, const UpnpListHead *q)\n"
			"{\n"
			"\tp->m_%s = *q;\n"
			"\n"
			"\treturn 1;\n"
			"}\n"
			"\n"
			"void %s_add_to_list_%s(%s *p, struct UpnpListHead *head)\n"
			"{\n"
			"\tUpnpListHead *list = &p->m_%s;\n"
			"\tUpnpListInsert(list, UpnpListEnd(list), head);\n"
			"}\n"
			"\n",
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name
		);
		/* clang-format on */
		break;
	case TYPE_STRING:
		/* clang-format off */
		fprintf(fp,
			"const UpnpString *%s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, const UpnpString *s)\n"
			"{\n"
			"\tconst char *q = UpnpString_get_String(s);\n"
			"\n"
			"\treturn UpnpString_set_String(p->m_%s, q);\n"
			"}\n"
			"\n"
			"size_t %s_get_%s_Length(const %s *p)\n"
			"{\n"
			"\treturn UpnpString_get_Length(%s_get_%s(p));\n"
			"}\n"
			"\n"
			"const char *%s_get_%s_cstr(const %s *p)\n"
			"{\n"
			"\treturn UpnpString_get_String(%s_get_%s(p));\n"
			"}\n"
			"\n"
			"int %s_strcpy_%s(%s *p, const char *s)\n"
			"{\n"
			"\treturn UpnpString_set_String(p->m_%s, s);\n"
			"}\n"
			"\n"
			"int %s_strncpy_%s(%s *p, const char *s, size_t n)\n"
			"{\n"
			"\treturn UpnpString_set_StringN(p->m_%s, s, n);\n"
			"}\n"
			"\n"
			"void %s_clear_%s(%s *p)\n"
			"{\n"
			"\tUpnpString_clear(p->m_%s);\n"
			"}\n"
			"\n",
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			class_name, m->name,
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name
		);
		/* clang-format on */
		break;
	case TYPE_DOMSTRING:
		/* clang-format off */
		fprintf(fp,
			"const DOMString %s_get_%s(const %s *p)\n"
			"{\n"
			"\treturn p->m_%s;\n"
			"}\n"
			"\n"
			"int %s_set_%s(%s *p, const DOMString s)\n"
			"{\n"
			"\tDOMString q = ixmlCloneDOMString(s);\n"
			"\tif (!q) return 0;\n"
			"\tixmlFreeDOMString(p->m_%s);\n"
			"\tp->m_%s = q;\n"
			"\n"
			"\treturn 1;\n"
			"}\n"
			"\n"
			"const char *%s_get_%s_cstr(const %s *p)\n"
			"{\n"
			"\treturn (const char *)%s_get_%s(p);\n"
			"}\n"
			"\n",
			class_name, m->name, class_name,
			m->name,
			class_name, m->name, class_name,
			m->name,
			m->name,
			class_name, m->name, class_name,
			class_name, m->name
		);
		/* clang-format on */
		break;
	default:
		printf("%s(): Error! Unknown type: %d.\n", __func__, m->type);
		return 0;
	}

	return 1;
}

static int write_source(FILE *fp, struct s_Class *c)
{
	int i;
	struct s_Member *m;

	fprintf(fp,
		"/*!\n"
		" * \\file\n"
		" *\n"
		" * \\brief Source file for %s methods.\n"
		" *\n"
		" * Do not edit this file, it is automatically generated. "
		"Please look at generator.c.\n"
		" *\n"
		" * \\author Marcelo Roberto Jimenez\n"
		" */\n"
		"#include \"config.h\"\n"
		"\n"
		"#include <stdlib.h> /* for calloc(), free() */\n"
		"#include <string.h> /* for strlen(), strdup() */\n"
		"\n",
		c->name);
	fprintf(fp,
		"\n"
		"#include \"%s\"\n"
		"\n"
		"struct s_%s\n"
		"{\n",
		c->header,
		c->name);
	for (i = 0; i < (int)c->n_members; ++i) {
		write_definition(fp, c->members + i);
	}
	fprintf(fp, "};\n\n");
	write_constructor(fp, c);
	fprintf(fp, "\n");
	write_destructor(fp, c);
	fprintf(fp, "\n");
	write_assignment_operator(fp, c);
	fprintf(fp, "\n");
	write_copy_constructor(fp, c);
	fprintf(fp, "\n");
	/* Member methods */
	for (i = 0; i < (int)c->n_members; ++i) {
		m = c->members + i;
		write_methods(fp, c->name, m);
	}

	return 1;
}

int main(int argc, const char *argv[])
{
	(void)argc;
	(void)argv;
	FILE *fp;
	int i;
	int n;
	struct s_Class *c = my_classes;

	n = (int)ARRAY_SIZE(my_classes);
	printf("Found %d classes.\n", n);
	for (i = 0; i < n; ++i) {
		/* Header file */
		fp = fopen(c[i].header, "w+");
		if (!fp) {
			continue;
		}
		printf("Writting %s ... ", c[i].header);
		write_header(fp, c + i);
		fclose(fp);
		printf("done!\n");
		/* Source file */
		fp = fopen(c[i].source, "w+");
		if (!fp) {
			continue;
		}
		printf("Writting %s ... ", c[i].source);
		write_source(fp, c + i);
		fclose(fp);
		printf("done!\n");
	}

	return 0;
}
