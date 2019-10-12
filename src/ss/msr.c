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

#define MSR_PWR_PRINTF_FORMAT " %8.3f W"
#define MESUREMENT_TIME 0.1

// AMD PPR  = https://www.amd.com/system/files/TechDocs/54945_PPR_Family_17h_Models_00h-0Fh.pdf
// AMD OSRR = https://developer.amd.com/wp-content/resources/56255_3_03.PDF

guint cores = 0;
guint threads_per_code = 0;
gdouble energy_unit = 0;

gint *msr_files = NULL;

gulong package_eng_b = 0;
gulong package_eng_a = 0;
gulong *core_eng_b = NULL;
gulong *core_eng_a = NULL;

gfloat package_power;
gfloat package_power_min;
gfloat package_power_max;
gfloat *core_power;
gfloat *core_power_min;
gfloat *core_power_max;

static guint get_core_count() {
    guint eax = 0, ebx = 0, ecx = 0, edx = 0;
    guint logical_cpus;

    // AMD PPR: page 57 - CPUID_Fn00000001_EBX
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    logical_cpus = (ebx >> 16) & 0xFF;

    // AMD PPR: page 82 - CPUID_Fn8000001E_EBX 
    __get_cpuid(0x8000001E, &eax, &ebx, &ecx, &edx);
    threads_per_code = ((ebx >> 8) & 0xF) + 1;

    if (threads_per_code == 0)
        return logical_cpus;

    return logical_cpus / threads_per_code;
}

static gint open_msr(gshort core) {
    gchar msr_path[20];
    sprintf(msr_path, "/dev/cpu/%d/msr", core * threads_per_code);
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
    int i;
    size_t sz;

    if (!check_zen())
        return FALSE;

    cores = get_core_count();
    if (cores == 0)
        return FALSE;

    msr_files = malloc(cores * sizeof (gint));
    for (i = 0; i < cores; i++) {
        msr_files[i] = open_msr(i);
    }

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
    GSList *list = NULL;
    gint i;

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
    gint i;

    package_power_min = package_power;
    package_power_max = package_power;
    for (i = 0; i < cores; i++) {
        core_power_min[i] = core_power[i];
    }
}

GSList* msr_get_sensors() {
    GSList *list = NULL;
    SensorInit *data;
    gint i;

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
