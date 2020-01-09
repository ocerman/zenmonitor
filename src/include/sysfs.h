#define SYSFS_DIR_CPUS "/sys/devices/system/cpu"

struct cpudev {
	gshort coreid;
	gshort cpuid;
};

struct cpudev * get_cpu_dev_ids(void);
