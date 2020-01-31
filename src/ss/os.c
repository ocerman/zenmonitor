#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "zenmonitor.h"
#include "sysfs.h"
#include "os.h"

#define OS_FREQ_PRINTF_FORMAT " %8.3f GHz"

static gchar **frq_files = NULL;
static guint cores;
static struct cpudev *cpu_dev_ids;

gfloat *core_freq;
gfloat *core_freq_min;
gfloat *core_freq_max;

static gdouble get_frequency(guint coreid) {
    gchar *data;
    gdouble freq;

    if (!g_file_get_contents(frq_files[coreid], &data, NULL, NULL))
        return 0.0;

    freq = atoi(data) / 1000000.0;
    g_free(data);

    return freq;
}

gboolean os_init(void) {
    guint i;

    if (!check_zen())
        return FALSE;

    cores = get_core_count();
    if (cores == 0)
        return FALSE;

    cpu_dev_ids = get_cpu_dev_ids();
    frq_files = malloc(cores * sizeof (gchar*));
    for (i = 0; i < cores; i++) {
        frq_files[i] = g_strdup_printf(
                        "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
                        cpu_dev_ids[i].cpuid);
    }

    core_freq = malloc(cores * sizeof (gfloat));
    core_freq_min = malloc(cores * sizeof (gfloat));
    core_freq_max = malloc(cores * sizeof (gfloat));

    os_update();
    memcpy(core_freq_min, core_freq, cores * sizeof (gfloat));
    memcpy(core_freq_max, core_freq, cores * sizeof (gfloat));

    return TRUE;
}

void os_update(void) {
    guint i;

    for (i = 0; i < cores; i++) {
        core_freq[i] = get_frequency(i);
        if (core_freq[i] < core_freq_min[i])
            core_freq_min[i] = core_freq[i];
        if (core_freq[i] > core_freq_max[i])
            core_freq_max[i] = core_freq[i];
    }
}

void os_clear_minmax(void) {
    guint i;

    for (i = 0; i < cores; i++) {
        core_freq_min[i] = core_freq[i];
        core_freq_max[i] = core_freq[i];
    }
}

GSList* os_get_sensors(void) {
    GSList *list = NULL;
    SensorInit *data;
    guint i;

    for (i = 0; i < cores; i++) {
        data = sensor_init_new();
        data->label = g_strdup_printf("Core %d Frequency", display_coreid ? cpu_dev_ids[i].coreid: i);
        data->value = &(core_freq[i]);
        data->min = &(core_freq_min[i]);
        data->max = &(core_freq_max[i]);
        data->printf_format = OS_FREQ_PRINTF_FORMAT;
        list = g_slist_append(list, data);
    }

    return list;
}
