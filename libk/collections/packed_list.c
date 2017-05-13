// libk/collections/packed_list.c

#include <string.h>
#include <collections/packed_list.h>

size_t packed_list_resize (packed_list_t *list, size_t new_size) {
	const size_t old_size = packed_list_count (list);

	list->capacity = new_size;

	if (old_size > new_size) {
		list->count = new_size;
		return old_size - new_size;
	}

	return 0;
}

bool packed_list_append (packed_list_t *list, void *elm) {
	if (!packed_list_remaining (list))
		return false;

	void *dest = packed_list_get (list, list->count);

	memcpy (dest, elm, list->elm_size);

	list->count += 1;

	return true;
}

bool packed_list_remove (
		packed_list_t *list,
		size_t index
) {
	if (index >= packed_list_count (list))
		return false;

	if (packed_list_get (list, index) != packed_list_get_last (list)) {
		void *elm = packed_list_get (list, index);
		void *last = packed_list_get_last (list);

		memcpy (elm, last, list->elm_size);
	}

	list->count -= 1;

	return true;
}

// vim: set ts=4 sw=4 noet syn=c:
