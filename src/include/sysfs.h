#ifndef __ZENMONITOR_SYSFS_H__
#define __ZENMONITOR_SYSFS_H__

#include <glib.h>

#define SYSFS_DIR_CPUS "/sys/devices/system/cpu"

struct cpudev {
	gshort coreid;
	gshort cpuid;
};

struct cpudev * get_cpu_dev_ids(void);

#endif /* __ZENMONITOR_SYSFS_H__ */
