#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysfs.h"
#include "zenmonitor.h"

#define CORES_MAX 256
struct bitset {
    guint bits[CORES_MAX/32];
};

static int bitset_set(struct bitset *set, int id) {
    if (id < CORES_MAX) {
        int v = (set->bits[id/32] >> (id & 31)) & 1;

        set->bits[id/32] |= 1 << (id & 31);
        return v;
    }
    return 1;
}

static int cmp_cpudev(const void *ap, const void *bp) {
    return ((struct cpudev *)ap)->coreid - ((struct cpudev *)bp)->coreid;
}

struct cpudev* get_cpu_dev_ids(void) {
    struct cpudev *cpu_dev_ids;
    gshort coreid, cpuid;
    GDir *dir;
    const gchar *entry;
    gchar *filename, *buffer;
    guint cores;
    struct bitset seen = { 0 };
    int i;

    cores = get_core_count();
    cpu_dev_ids = malloc(cores * sizeof (*cpu_dev_ids));
    for (i=0;i<cores;i++)
        cpu_dev_ids[i] = (struct cpudev) { -1, -1 };

    dir = g_dir_open(SYSFS_DIR_CPUS, 0, NULL);
    if (dir) {
        int i = 0;

        while ((entry = g_dir_read_name(dir))) {
            if (sscanf(entry, "cpu%hd", &cpuid) != 1) {
                continue;
            }

            filename = g_build_filename(SYSFS_DIR_CPUS, entry, "topology", "core_id", NULL);
            if (g_file_get_contents(filename, &buffer, NULL, NULL)) {
                coreid = (gshort) atoi(buffer);

                if (i < cores && !bitset_set(&seen, coreid)) {
                    cpu_dev_ids[i++] = (struct cpudev) { coreid, cpuid };
                }
            }

            g_free(filename);
            g_free(buffer);
        }
    }

    qsort(cpu_dev_ids, cores, sizeof(*cpu_dev_ids), cmp_cpudev);

    return cpu_dev_ids;
}
