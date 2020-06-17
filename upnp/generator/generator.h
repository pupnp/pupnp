#ifndef GENERATOR_H
#define GENERATOR_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof 0 [a])

enum e_Type
{
	TYPE_CLASS = 0,
	TYPE_INTEGER,
	TYPE_BUFFER,
	TYPE_LIST,
	TYPE_STRING,
	TYPE_DOMSTRING
};

struct s_Type_Integer
{
	const char *int_type;
};

struct s_Member
{
	const char *name;
	enum e_Type type;
	const char *type_name;
	const char *header;
};

/* clang-format off */
#define INIT_MEMBER(NAME, TYPE, TYPE_NAME, HEADER) \
	{ \
		.name = #NAME, \
		.type = TYPE, \
		.type_name = #TYPE_NAME, \
		.header = HEADER \
	}
/* clang-format on */

struct s_Class
{
	const char *name;
	unsigned n_members;
	struct s_Member *members;
	const char *source;
	const char *header;
};

/* clang-format off */
#define INIT_CLASS(CLASS) \
	{ \
		.name = #CLASS, \
		.n_members = ARRAY_SIZE(CLASS##_members), \
		.members = CLASS##_members, \
		.source = #CLASS ".c", \
		.header = #CLASS ".h", \
	}
/* clang-format on */

#endif /* GENERATOR_H */
