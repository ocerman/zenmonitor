#ifndef __ZENMONITOR_ZENPOWER_H__
#define __ZENMONITOR_ZENPOWER_H__

#include <glib.h>

gboolean zenpower_init();
GSList* zenpower_get_sensors();
void zenpower_update();
void zenpower_clear_minmax();

#endif /* __ZENMONITOR_ZENPOWER_H__ */
