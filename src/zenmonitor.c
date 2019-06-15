#include <gtk/gtk.h>
#include <cpuid.h>
#include "zenmonitor.h"
#include "zenpower.h"
#include "msr.h"
#include "gui.h"

#define AMD_STRING "AuthenticAMD"
#define ZEN_FAMILY 0x17

gboolean check_zen() {
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, ext_family; 
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

static SensorSource sensor_sources[] = {
  { "zenpower", zenpower_init, zenpower_get_sensors, zenpower_update, FALSE, NULL },
  { "msr",      msr_init,      msr_get_sensors,      msr_update,      FALSE, NULL },
  { NULL }
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
