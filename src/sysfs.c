#include <glib.h>
#include <stdio.h>
#include "zenmonitor.h"

#define SYSFS_DIR_CPUS "/sys/bus/cpu/devices"

gshort* get_cpu_dev_ids(){
    gshort *cpu_dev_ids = NULL;
    gshort coreid, cpuid;
    GDir *dir;
    const gchar *entry;
    gchar *filename, *buffer;
    guint cores, i;

    cores = get_core_count();
    cpu_dev_ids = malloc(cores * sizeof (gshort));
    memset(cpu_dev_ids, -1, cores * sizeof (gshort));

    dir = g_dir_open(SYSFS_DIR_CPUS, 0, NULL);
    if (dir) {
        while ((entry = g_dir_read_name(dir))) {
            if (sscanf(entry, "cpu%hd", &cpuid) != 1) {
                continue;
            }

            filename = g_build_filename(SYSFS_DIR_CPUS, entry, "topology", "core_id", NULL);
            if (g_file_get_contents(filename, &buffer, NULL, NULL)) {
                coreid = (gshort) atoi(buffer);

                if (coreid < cores && cpu_dev_ids[coreid] == -1) {
                    cpu_dev_ids[coreid] = cpuid;
                }
            }

            g_free(filename);
            g_free(buffer);
        }
    }

    for (i = 0; i < cores; i++) {
        if (cpu_dev_ids[i] < 0) {
            cpu_dev_ids[i] = i;
        }
    }

    return cpu_dev_ids;
}

