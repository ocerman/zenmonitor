#include <glib.h>
#include "zenmonitor.h"
#include "sysfs.h"
#include "os.h"

#define OS_FREQ_PRINTF_FORMAT " %8.3f GHz"

static gchar **frq_files = NULL;
static guint cores;

gfloat *core_freq;
gfloat *core_freq_min;
gfloat *core_freq_max;

static gdouble get_frequency(guint coreid) {
    gchar *data;
    if (!g_file_get_contents(frq_files[coreid], &data, NULL, NULL))
        return 0.0;

    return atoi(data) / 1000000.0;
}

gboolean os_init() {
    gshort *cpu_dev_ids = NULL;
    gint i;

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
                        cpu_dev_ids[i]);
    }
    g_free(cpu_dev_ids);

    core_freq = malloc(cores * sizeof (gfloat));
    core_freq_min = malloc(cores * sizeof (gfloat));
    core_freq_max = malloc(cores * sizeof (gfloat));

    os_update();
    memcpy(core_freq_min, core_freq, cores * sizeof (gfloat));
    memcpy(core_freq_max, core_freq, cores * sizeof (gfloat));

    return TRUE;
}

void os_update() {
    gint i;

    for (i = 0; i < cores; i++) {
        core_freq[i] = get_frequency(i);
        if (core_freq[i] < core_freq_min[i])
            core_freq_min[i] = core_freq[i];
        if (core_freq[i] > core_freq_max[i])
            core_freq_max[i] = core_freq[i];
    }
}

void os_clear_minmax() {
    gint i;

    for (i = 0; i < cores; i++) {
        core_freq_min[i] = core_freq[i];
        core_freq_max[i] = core_freq[i];
    }
}

GSList* os_get_sensors() {
    GSList *list = NULL;
    SensorInit *data;
    gint i;

    for (i = 0; i < cores; i++) {
        data = sensor_init_new();
        data->label = g_strdup_printf("Core %d Frequency", i);
        data->value = &(core_freq[i]);
        data->min = &(core_freq_min[i]);
        data->max = &(core_freq_max[i]);
        data->printf_format = OS_FREQ_PRINTF_FORMAT;
        list = g_slist_append(list, data);
    }

    return list;
}