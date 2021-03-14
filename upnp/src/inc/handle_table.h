#ifndef HANDLE_TABLE_H
#define HANDLE_TABLE_H

#define HANDLE_TABLE_MAX_NUM_ELEMENTS 200

typedef struct s_handle_table {
	void *handle[HANDLE_TABLE_MAX_NUM_ELEMENTS];
} handle_table_t;

#endif /* HANDLE_TABLE_H */

