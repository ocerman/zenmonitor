#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysfs.h"
#include "zenmonitor.h"

#define CPUD_MAX 512
struct bitset {
    guint bits[CPUD_MAX/32];
};

static int bitset_set(struct bitset *set, int id) {
    if (id < CPUD_MAX) {
        int v = (set->bits[id/32] >> (id & 31)) & 1;

        set->bits[id/32] |= 1 << (id & 31);
        return v;
    }
    return 1;
}

static int cmp_cpudev(const void *ap, const void *bp) {
    return ((struct cpudev *)ap)->cpuid - ((struct cpudev *)bp)->cpuid;
}

struct cpudev* get_cpu_dev_ids(void) {
    struct cpudev *cpu_dev_ids;
    gshort coreid, cpuid, siblingid;
    GDir *dir;
    const gchar *entry;
    gchar *filename, *buffer;
    gchar **cpusiblings;
    gchar **ptr;
    guint cores;
    gboolean found;
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

            found = FALSE;

            filename = g_build_filename(SYSFS_DIR_CPUS, entry, "topology", "core_id", NULL);
            if (g_file_get_contents(filename, &buffer, NULL, NULL)) {
                coreid = (gshort) atoi(buffer);

                g_free(filename);
                g_free(buffer);

                filename = g_build_filename(SYSFS_DIR_CPUS, entry, "topology", "thread_siblings_list", NULL);
                if (g_file_get_contents(filename, &buffer, NULL, NULL)) {
                    cpusiblings = g_strsplit(buffer, ",", -1);
                    found = TRUE;

                    // check whether cpu device is not for SMT thread sibling
                    for (ptr = cpusiblings; *ptr; ptr++) {
                        siblingid = (gshort) atoi(*ptr);

                        // let's save the cpu device with lower number
                        if (siblingid < cpuid)
                            cpuid = siblingid;

                        if (bitset_set(&seen, siblingid)) {
                            found = FALSE;
                        }
                    }
                    g_strfreev(cpusiblings);
                }
            }

            if (found && i < cores) {
                cpu_dev_ids[i++] = (struct cpudev) { coreid, cpuid };
            }

            g_free(filename);
            g_free(buffer);
        }
    }

    qsort(cpu_dev_ids, cores, sizeof(*cpu_dev_ids), cmp_cpudev);

    return cpu_dev_ids;
}
