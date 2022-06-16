#ifndef DOCUMENT_TYPE_H
#define DOCUMENT_TYPE_H

/*! mapping of file extension to content-type of document */
struct s_document_type
{
	/*! . */
	const char *file_ext;
	/*! . */
	const char *content_type;
	/*! . */
	const char *content_subtype;
};

typedef struct s_document_type document_type_t;

#define NUM_MEDIA_TYPES 70

struct s_doc_type_array
{
	document_type_t doc[NUM_MEDIA_TYPES];
};

typedef struct s_doc_type_array doc_type_array_t;

#endif /* DOCUMENT_TYPE_H */
