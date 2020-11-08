#ifndef __ZENMONITOR_MSR_H__
#define __ZENMONITOR_MSR_H__

#include <glib.h>

gboolean msr_init();
void msr_update();
void msr_clear_minmax();
GSList* msr_get_sensors();

#endif /* __ZENMONITOR_MSR_H__ */
