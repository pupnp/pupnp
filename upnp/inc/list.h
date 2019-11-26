#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

#include <stddef.h> /* for offsetof */

#include "poison.h"
#include "UpnpGlobal.h" /* For UPNP_INLINE */

/**
 * upnp_container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define upnp_container_of(ptr, type, member) ({                 \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct upnp_list_head {
	struct upnp_list_head *next, *prev;
};

struct upnp_hlist_head {
        struct upnp_hlist_node *first;
};

struct upnp_hlist_node {
        struct upnp_hlist_node *next, **pprev;
};

#define UPNP_LIST_HEAD_INIT(name) { &(name), &(name) }

#define UPNP_LIST_HEAD(name) \
	struct upnp_list_head name = UPNP_LIST_HEAD_INIT(name)

static UPNP_INLINE void UPNP_INIT_LIST_HEAD(struct upnp_list_head *list)
{
	list->next = list;
	list->prev = list;
}

static UPNP_INLINE int __upnp_list_add_valid(struct upnp_list_head *newent,
				struct upnp_list_head *prev,
				struct upnp_list_head *next)
{
	return 1;
	newent++; prev++; next++; /* against compiler warnings */
}
static UPNP_INLINE int __upnp_list_del_entry_valid(struct upnp_list_head *entry)
{
	return 1;
	entry++; /* against compiler warnings */
}

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static UPNP_INLINE void __upnp_list_add(struct upnp_list_head *newent,
			      struct upnp_list_head *prev,
			      struct upnp_list_head *next)
{
	if (!__upnp_list_add_valid(newent, prev, next))
		return;

	next->prev = newent;
	newent->next = next;
	newent->prev = prev;
	prev->next = newent;
}

/**
 * list_add - add a new entry
 * @newent: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static UPNP_INLINE void list_add(struct upnp_list_head *newent, struct upnp_list_head *head)
{
	__upnp_list_add(newent, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @newent: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static UPNP_INLINE void list_add_tail(struct upnp_list_head *newent, struct upnp_list_head *head)
{
	__upnp_list_add(newent, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static UPNP_INLINE void __upnp_list_del(struct upnp_list_head * prev, struct upnp_list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return 1 after this, the entry is
 * in an undefined state.
 */
static UPNP_INLINE void __upnp_list_del_entry(struct upnp_list_head *entry)
{
	if (!__upnp_list_del_entry_valid(entry))
		return;

	__upnp_list_del(entry->prev, entry->next);
}

static UPNP_INLINE void list_del(struct upnp_list_head *entry)
{
	__upnp_list_del_entry(entry);
	entry->next = (struct upnp_list_head*)LIST_POISON1;
	entry->prev =  (struct upnp_list_head*)LIST_POISON2;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @newent : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static UPNP_INLINE void list_replace(struct upnp_list_head *old,
				struct upnp_list_head *newent)
{
	newent->next = old->next;
	newent->next->prev = newent;
	newent->prev = old->prev;
	newent->prev->next = newent;
}

static UPNP_INLINE void list_replace_init(struct upnp_list_head *old,
					struct upnp_list_head *newent)
{
	list_replace(old, newent);
	UPNP_INIT_LIST_HEAD(old);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static UPNP_INLINE void list_del_init(struct upnp_list_head *entry)
{
	__upnp_list_del_entry(entry);
	UPNP_INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static UPNP_INLINE void list_move(struct upnp_list_head *list, struct upnp_list_head *head)
{
	__upnp_list_del_entry(list);
	list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static UPNP_INLINE void list_move_tail(struct upnp_list_head *list,
				  struct upnp_list_head *head)
{
	__upnp_list_del_entry(list);
	list_add_tail(list, head);
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static UPNP_INLINE int list_is_last(const struct upnp_list_head *list,
				const struct upnp_list_head *head)
{
	return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static UPNP_INLINE int list_empty(const struct upnp_list_head *head)
{
	return head->next == head;
}

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
static UPNP_INLINE int list_empty_careful(const struct upnp_list_head *head)
{
	struct upnp_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static UPNP_INLINE void list_rotate_left(struct upnp_list_head *head)
{
	struct upnp_list_head *first;

	if (!list_empty(head)) {
		first = head->next;
		list_move_tail(first, head);
	}
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static UPNP_INLINE int list_is_singular(const struct upnp_list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}

static UPNP_INLINE void __upnp_list_cut_position(struct upnp_list_head *list,
		struct upnp_list_head *head, struct upnp_list_head *entry)
{
	struct upnp_list_head *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static UPNP_INLINE void list_cut_position(struct upnp_list_head *list,
		struct upnp_list_head *head, struct upnp_list_head *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		UPNP_INIT_LIST_HEAD(list);
	else
		__upnp_list_cut_position(list, head, entry);
}

static UPNP_INLINE void __upnp_list_splice(const struct upnp_list_head *list,
				 struct upnp_list_head *prev,
				 struct upnp_list_head *next)
{
	struct upnp_list_head *first = list->next;
	struct upnp_list_head *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static UPNP_INLINE void list_splice(const struct upnp_list_head *list,
				struct upnp_list_head *head)
{
	if (!list_empty(list))
		__upnp_list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static UPNP_INLINE void list_splice_tail(struct upnp_list_head *list,
				struct upnp_list_head *head)
{
	if (!list_empty(list))
		__upnp_list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static UPNP_INLINE void list_splice_init(struct upnp_list_head *list,
				    struct upnp_list_head *head)
{
	if (!list_empty(list)) {
		__upnp_list_splice(list, head, head->next);
		UPNP_INIT_LIST_HEAD(list);
	}
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static UPNP_INLINE void list_splice_tail_init(struct upnp_list_head *list,
					 struct upnp_list_head *head)
{
	if (!list_empty(list)) {
		__upnp_list_splice(list, head->prev, head);
		UPNP_INIT_LIST_HEAD(list);
	}
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct upnp_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_entry(ptr, type, member) \
	upnp_container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member) ({ \
	struct upnp_list_head *head__ = (ptr); \
	struct upnp_list_head *pos__ = head__->next; \
	pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct upnp_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct upnp_list_head to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct upnp_list_head to use as a loop cursor.
 * @n:		another &struct upnp_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct upnp_list_head to use as a loop cursor.
 * @n:		another &struct upnp_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = list_prev_entry(pos, member))

/**
 * list_prepare_entry - prepare a pos entry for use in list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Prepares a pos entry for use as a start point in list_for_each_entry_continue().
 */
#define list_prepare_entry(pos, head, member) \
	((pos) ? : list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define list_for_each_entry_continue_reverse(pos, head, member)		\
	for (pos = list_prev_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = list_prev_entry(pos, member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define list_for_each_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_from_reverse - iterate backwards over list of given type
 *                                    from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Iterate backwards over list of given type, continuing from current position.
 */
#define list_for_each_entry_from_reverse(pos, head, member)		\
	for (; &pos->member != (head);					\
	     pos = list_prev_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member) 		\
	for (pos = list_next_entry(pos, member), 				\
		n = list_next_entry(pos, member);				\
	     &pos->member != (head);						\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define list_for_each_entry_safe_from(pos, n, head, member) 			\
	for (n = list_next_entry(pos, member);					\
	     &pos->member != (head);						\
	     pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the upnp_list_head within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_last_entry(head, typeof(*pos), member),		\
		n = list_prev_entry(pos, member);			\
	     &pos->member != (head); 					\
	     pos = n, n = list_prev_entry(n, member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the list_for_each_entry_safe loop
 * @n:		temporary storage used in list_for_each_entry_safe
 * @member:	the name of the upnp_list_head within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define list_safe_reset_next(pos, n, member)				\
	n = list_next_entry(pos, member)

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define UPNP_HLIST_HEAD_INIT { .first = NULL }
#define UPNP_HLIST_HEAD(name) struct upnp_hlist_head name = {  .first = NULL }
#define UPNP_INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static UPNP_INLINE void INIT_HLIST_NODE(struct upnp_hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static UPNP_INLINE int hlist_unhashed(const struct upnp_hlist_node *h)
{
	return !h->pprev;
}

static UPNP_INLINE int hlist_empty(const struct upnp_hlist_head *h)
{
	return !h->first;
}

static UPNP_INLINE void __upnp_hlist_del(struct upnp_hlist_node *n)
{
	struct upnp_hlist_node *next = n->next;
	struct upnp_hlist_node **pprev = n->pprev;

	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static UPNP_INLINE void hlist_del(struct upnp_hlist_node *n)
{
	__upnp_hlist_del(n);
	n->next =  (struct upnp_hlist_node*)LIST_POISON1;
	n->pprev =  (struct upnp_hlist_node**)LIST_POISON2;
}

static UPNP_INLINE void hlist_del_init(struct upnp_hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		__upnp_hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static UPNP_INLINE void hlist_add_head(struct upnp_hlist_node *n, struct upnp_hlist_head *h)
{
	struct upnp_hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* next must be != NULL */
static UPNP_INLINE void hlist_add_before(struct upnp_hlist_node *n,
					struct upnp_hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

static UPNP_INLINE void hlist_add_behind(struct upnp_hlist_node *n,
				    struct upnp_hlist_node *prev)
{
	n->next = prev->next;
	prev->next = n;
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev  = &n->next;
}

/* after that we'll appear to be on some hlist and hlist_del will work */
static UPNP_INLINE void hlist_add_fake(struct upnp_hlist_node *n)
{
	n->pprev = &n->next;
}

static UPNP_INLINE int hlist_fake(struct upnp_hlist_node *h)
{
	return h->pprev == &h->next;
}

/*
 * Check whether the node is the only node of the head without
 * accessing head:
 */
static UPNP_INLINE int
hlist_is_singular_node(struct upnp_hlist_node *n, struct upnp_hlist_head *h)
{
	return !n->next && n->pprev == &h->first;
}

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static UPNP_INLINE void hlist_move_list(struct upnp_hlist_head *old,
				   struct upnp_hlist_head *newent)
{
	newent->first = old->first;
	if (newent->first)
		newent->first->pprev = &newent->first;
	old->first = NULL;
}

#define hlist_entry(ptr, type, member) upnp_container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hlist_entry(____ptr, type, member) : NULL; \
	})

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the upnp_hlist_node within the struct.
 */
#define hlist_for_each_entry(pos, head, member)				\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the upnp_hlist_node within the struct.
 */
#define hlist_for_each_entry_continue(pos, member)			\
	for (pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from current point
 * @pos:	the type * to use as a loop cursor.
 * @member:	the name of the upnp_hlist_node within the struct.
 */
#define hlist_for_each_entry_from(pos, member)				\
	for (; pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another &struct upnp_hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the upnp_hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = hlist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = hlist_entry_safe(n, typeof(*pos), member))

#endif
