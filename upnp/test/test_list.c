/* Force asserts enabled for the test */
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

struct list_test_item
{
	UpnpListHead list;
	int index;
};

struct list_test_item *list_test_item__new(int i)
{
	struct list_test_item *p = calloc(1, sizeof(*p));
	assert(p != NULL);

	p->index = i;
	return p;
}

int main(void)
{
	int i;
	UpnpListHead list;
	UpnpListInit(&list);

	/* Fill list with items */
	for (i = 0; i < 10; i++) {
		struct list_test_item *item = list_test_item__new(i);

		UpnpListInsert(&list, UpnpListEnd(&list), &item->list);
	}

	/* Verify list contents */
	{
		int i = 0;
		UpnpListIter list_iter = UpnpListBegin(&list);
		while (list_iter != UpnpListEnd(&list)) {
			struct list_test_item *item =
				(struct list_test_item *)list_iter;

			printf("List item index: %i, expected: %i\n",
				item->index,
				i);
			assert(item->index == i);

			i++;
			list_iter = UpnpListNext(&list, list_iter);
		}
	}

	/* Verify list is still fine */
	assert(UpnpListBegin(&list) != UpnpListEnd(&list));

	/* Clear list */
	{
		int i = 0;
		UpnpListIter list_iter = UpnpListBegin(&list);
		while (list_iter != UpnpListEnd(&list)) {
			struct list_test_item *item =
				(struct list_test_item *)list_iter;

			printf("List item index: %i, expected: %i\n",
				item->index,
				i);
			assert(item->index == i);

			i++;
			list_iter = UpnpListErase(&list, list_iter);
			free(item);
		}
	}

	/* Verify list is cleared */
	assert(UpnpListBegin(&list) == UpnpListEnd(&list));

	UpnpListIter list_iter = UpnpListBegin(&list);
	assert(UpnpListNext(&list, list_iter) == UpnpListEnd(&list));
	assert(list_iter == UpnpListEnd(&list));

	return 0;
}
