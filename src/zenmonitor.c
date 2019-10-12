#include <gtk/gtk.h>
#include <cpuid.h>
#include <string.h>
#include "zenmonitor.h"
#include "zenpower.h"
#include "msr.h"
#include "os.h"
#include "gui.h"

#define AMD_STRING "AuthenticAMD"
#define ZEN_FAMILY 0x17

// AMD PPR = https://www.amd.com/system/files/TechDocs/54945_PPR_Family_17h_Models_00h-0Fh.pdf

gboolean check_zen() {
    guint32 eax = 0, ebx = 0, ecx = 0, edx = 0, ext_family;
    char vendor[13];

    __get_cpuid(0, &eax, &ebx, &ecx, &edx);

    memcpy(vendor, &ebx, 4);
    memcpy(vendor+4, &edx, 4);
    memcpy(vendor+8, &ecx, 4);
    vendor[12] = 0;

    if (strcmp(vendor, AMD_STRING) != 0){
        return FALSE;
    }

    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    ext_family = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);
    if (ext_family != ZEN_FAMILY){
        return FALSE;
    }

    return TRUE;
}

gchar *cpu_model() {
    guint32 eax = 0, ebx = 0, ecx = 0, edx = 0;
    char model[48];

    // AMD PPR: page 65-68 - CPUID_Fn80000002_EAX-CPUID_Fn80000004_EDX
    __get_cpuid(0x80000002, &eax, &ebx, &ecx, &edx);
    memcpy(model, &eax, 4);
    memcpy(model+4, &ebx, 4);
    memcpy(model+8, &ecx, 4);
    memcpy(model+12, &edx, 4);

    __get_cpuid(0x80000003, &eax, &ebx, &ecx, &edx);
    memcpy(model+16, &eax, 4);
    memcpy(model+20, &ebx, 4);
    memcpy(model+24, &ecx, 4);
    memcpy(model+28, &edx, 4);

    __get_cpuid(0x80000004, &eax, &ebx, &ecx, &edx);
    memcpy(model+32, &eax, 4);
    memcpy(model+36, &ebx, 4);
    memcpy(model+40, &ecx, 4);
    memcpy(model+44, &edx, 4);

    model[48] = 0;
    return g_strdup(g_strchomp(model));
}

guint get_core_count() {
    guint eax = 0, ebx = 0, ecx = 0, edx = 0;
    guint logical_cpus, threads_per_code;

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

static SensorSource sensor_sources[] = {
    {
        "zenpower",
        zenpower_init, zenpower_get_sensors, zenpower_update, zenpower_clear_minmax,
        FALSE, NULL
    },
    {
        "msr",
        msr_init, msr_get_sensors, msr_update, msr_clear_minmax,
        FALSE, NULL
    },
    {
        "os",
        os_init, os_get_sensors, os_update, os_clear_minmax,
        FALSE, NULL
    },
    {
        NULL
    }
};

SensorInit *sensor_init_new() {
    return g_new0(SensorInit, 1);
}

void sensor_init_free(SensorInit *s) {
    if (s) {
        g_free(s->label);
        g_free(s);
    }
}

int main (int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    start_gui(sensor_sources);
}
