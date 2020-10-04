#ifndef __ZENMONITOR_OS_H__
#define __ZENMONITOR_OS_H__

#include <glib.h>

gboolean os_init(void);
void os_update(void);
void os_clear_minmax(void);
GSList* os_get_sensors(void);

#endif /* __ZENMONITOR_OS_H__ */
