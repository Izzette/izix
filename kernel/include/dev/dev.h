// kernel/include/dev/dev.h

#ifndef IZIX_DEV_H
#define IZIX_DEV_H 1

#include <dev/dev_types.h>
#include <dev/dev_driver.h>

void dev_add (dev_driver_t *);
void dev_remove (dev_t);
void dev_map (dev_t, paging_data_t *);
void dev_map_all (paging_data_t *);

#endif

// vim: set ts=4 sw=4 noet syn=c:
