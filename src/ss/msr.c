#include <glib.h>
#include <cpuid.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "stdlib.h"
#include "string.h"
#include "zenmonitor.h"
#include "msr.h"
#include "sysfs.h"

#define MSR_PWR_PRINTF_FORMAT " %8.3f W"
#define MESUREMENT_TIME 0.1

// AMD PPR  = https://www.amd.com/system/files/TechDocs/54945_PPR_Family_17h_Models_00h-0Fh.pdf
// AMD OSRR = https://developer.amd.com/wp-content/resources/56255_3_03.PDF

static guint cores = 0;
static gdouble energy_unit = 0;

static gint *msr_files = NULL;

static gulong package_eng_b = 0;
static gulong package_eng_a = 0;
static gulong *core_eng_b = NULL;
static gulong *core_eng_a = NULL;

gfloat package_power;
gfloat package_power_min;
gfloat package_power_max;
gfloat *core_power;
gfloat *core_power_min;
gfloat *core_power_max;


static gint open_msr(gshort devid) {
    gchar msr_path[20];
    sprintf(msr_path, "/dev/cpu/%d/msr", devid);
    return open(msr_path, O_RDONLY);
}

static gboolean read_msr(gint file, guint index, gulong *data) {
    if (file < 0)
        return FALSE;

    return pread(file, data, sizeof *data, index) == sizeof *data;
}

gdouble get_energy_unit() {
    gulong data;
    // AMD OSRR: page 139 - MSRC001_0299
    if (!read_msr(msr_files[0], 0xC0010299, &data))
        return 0.0;

    return pow(1.0/2.0, (double)((data >> 8) & 0x1F));
}

gulong get_package_energy() {
    gulong data;
    // AMD OSRR: page 139 - MSRC001_029B
    if (!read_msr(msr_files[0], 0xC001029B, &data))
        return 0;

    return data;
}

gulong get_core_energy(gint core) {
    gulong data;
    // AMD OSRR: page 139 - MSRC001_029A
    if (!read_msr(msr_files[core], 0xC001029A, &data))
        return 0;

    return data;
}

gboolean msr_init() {
    gshort *cpu_dev_ids = NULL;
    guint i;

    if (!check_zen())
        return FALSE;

    cores = get_core_count();
    if (cores == 0)
        return FALSE;

    cpu_dev_ids = get_cpu_dev_ids();
    msr_files = malloc(cores * sizeof (gint));
    for (i = 0; i < cores; i++) {
        msr_files[i] = open_msr(cpu_dev_ids[i]);
    }
    g_free(cpu_dev_ids);

    energy_unit = get_energy_unit();
    if (energy_unit == 0)
        return FALSE;

    core_eng_b = malloc(cores * sizeof (gulong));
    core_eng_a = malloc(cores * sizeof (gulong));
    core_power = malloc(cores * sizeof (gfloat));
    core_power_min = malloc(cores * sizeof (gfloat));
    core_power_max = malloc(cores * sizeof (gfloat));

    msr_update();
    memcpy(core_power_min, core_power, cores * sizeof (gfloat));
    memcpy(core_power_max, core_power, cores * sizeof (gfloat));
    package_power_min = package_power;
    package_power_max = package_power;

    return TRUE;
}

void msr_update() {
    guint i;

    package_eng_b = get_package_energy();
    for (i = 0; i < cores; i++) {
        core_eng_b[i] = get_core_energy(i);
    }

    usleep(MESUREMENT_TIME*1000000);

    package_eng_a = get_package_energy();
    for (i = 0; i < cores; i++) {
        core_eng_a[i] = get_core_energy(i);
    }

    package_power = (package_eng_a - package_eng_b) * energy_unit / MESUREMENT_TIME;
    if (package_power < package_power_min)
        package_power_min = package_power;
    if (package_power > package_power_max)
        package_power_max = package_power;

    for (i = 0; i < cores; i++) {
        core_power[i] = (core_eng_a[i] - core_eng_b[i]) * energy_unit / MESUREMENT_TIME;
        if (core_power[i] < core_power_min[i])
            core_power_min[i] = core_power[i];
        if (core_power[i] > core_power_max[i])
            core_power_max[i] = core_power[i];
    }
}

void msr_clear_minmax() {
    guint i;

    package_power_min = package_power;
    package_power_max = package_power;
    for (i = 0; i < cores; i++) {
        core_power_min[i] = core_power[i];
        core_power_max[i] = core_power[i];
    }
}

GSList* msr_get_sensors() {
    GSList *list = NULL;
    SensorInit *data;
    guint i;

    data = sensor_init_new();
    data->label = g_strdup("Package Power");
    data->value = &package_power;
    data->min = &package_power_min;
    data->max = &package_power_max;
    data->printf_format = MSR_PWR_PRINTF_FORMAT;
    list = g_slist_append(list, data);

    for (i = 0; i < cores; i++) {
        data = sensor_init_new();
        data->label = g_strdup_printf("Core %d Power", i);
        data->value = &(core_power[i]);
        data->min = &(core_power_min[i]);
        data->max = &(core_power_max[i]);
        data->printf_format = MSR_PWR_PRINTF_FORMAT;
        list = g_slist_append(list, data);
    }

    return list;
}
