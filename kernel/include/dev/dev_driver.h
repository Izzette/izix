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
//   bool need_write;
//   while ((pg = driver->next_page_mapping (driver, pg, &need_write)))
//     <...>
	page_t *(*next_page_mapping) (dev_driver_t *, page_t *, bool *);
} dev_driver_t;

#endif

// vim: set ts=4 sw=4 noet syn=c:
