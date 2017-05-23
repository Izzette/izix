// kernel/include/dev/dev_driver.h

#ifndef IZIX_DEV_DRIVER_H
#define IZIX_DEV_DRIVER_H 1

#include <dev/dev_types.h>
#include <mm/paging.h>

typedef struct dev_driver_struct dev_driver_t;
typedef struct dev_driver_struct {
// Private data.
	void *pimpl;
// Device descriptor.
	dev_t dev;
// Get the next physical page address needed for this device driver.
// Example usage:
//   page_t *pg = NULL;
//   while ((pg = driver->next_page_mapping (pg)))
//     <...>
	page_t *(*next_page_mapping) (dev_driver_t *, page_t *);
} dev_driver_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
