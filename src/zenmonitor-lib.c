#include <assert.h>
#include <cpuid.h>
#include <strings.h>
#include <time.h>

#include "zenpower.h"
#include "msr.h"
#include "os.h"
#include "zenmonitor.h"

#define AMD_STRING "AuthenticAMD"
#define ZEN_FAMILY 0x17

const guint SENSOR_DATA_STORE_NUM = 4096;

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

SensorDataStore *sensor_data_store_new()
{
    SensorDataStore *ret;

    ret = g_new0(SensorDataStore, 1);
    ret->labels = g_ptr_array_new();
    ret->data = g_ptr_array_new();
    ret->time = g_array_new(FALSE, TRUE, sizeof(struct timespec));

    return ret;
}

void sensor_data_store_add_entry(SensorDataStore *store, gchar *entry)
{
    GArray *data;
    data = g_array_new(TRUE, TRUE, sizeof(float));

    g_ptr_array_add(store->labels, entry);
    g_ptr_array_add(store->data, data);
}

void sensor_data_store_keep_time(SensorDataStore *store)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    g_array_append_val(store->time, ts);
}

gint sensor_data_store_drop_entry(SensorDataStore *store, gchar *entry)
{
    guint index = 0;
    gboolean found = g_ptr_array_find(store->labels, entry, &index);
    if(!found)
    {
        return 1;
    }

    g_ptr_array_remove_index(store->labels, index);
    g_ptr_array_remove_index(store->data, index);
    
    return 0;
}

gint sensor_data_store_add_data(SensorDataStore *store, gchar *entry, float value)
{
    guint index = 0;
    gboolean found = g_ptr_array_find(store->labels, entry, &index);

    if(!found)
    {
        return 1;
    }

    GArray *data = g_ptr_array_index(store->data, index);
    g_array_append_val(data, value);

    return 0;
}

void sensor_data_store_free(SensorDataStore *store)
{
    if(store)
    {
        g_array_free(store->time, TRUE);
        g_ptr_array_free(store->labels, TRUE);
        g_ptr_array_free(store->data, TRUE);
        g_free(store);
    }
}

SensorInit *sensor_init_new() {
    return g_new0(SensorInit, 1);
}

void sensor_init_free(SensorInit *s) {
    if (s) {
        g_free(s->label);
        g_free(s->hint);
        g_free(s);
    }
}

