#include <glib.h>
#include "zenmonitor.h"
#include "zenpower.h"

static gchar *zenpowerDir = NULL;

typedef struct
{
    const gchar *label;
    const gchar *file;
    const gchar *printf_format;
    const double adjust_ratio;
    float current_value;
    float min;
    float max;
} HwmonSensor;

HwmonSensor hwmon_sensors[] = {
  {"CPU Temperature (tCtrl)",   "temp1_input",  " %6.2f°C", 1000.0,    0.0, 999.0, 0.0},
  {"CPU Temperature (tDie)",    "temp2_input",  " %6.2f°C", 1000.0,    0.0, 999.0, 0.0},
  {"CPU Core Voltage (SVI2)",   "in1_input",    " %8.3f V", 1000.0,    0.0, 999.0, 0.0},
  {"SOC Voltage (SVI2)",        "in2_input",    " %8.3f V", 1000.0,    0.0, 999.0, 0.0},
  {"CPU Core Current (SVI2)",   "curr1_input",  " %8.3f A", 1000.0,    0.0, 999.0, 0.0},
  {"SOC Current (SVI2)",        "curr2_input",  " %8.3f A", 1000.0,    0.0, 999.0, 0.0},
  {"CPU Core Power (SVI2)",     "power1_input", " %8.3f W", 1000000.0, 0.0, 999.0, 0.0},
  {"SOC Power (SVI2)",          "power2_input", " %8.3f W", 1000000.0, 0.0, 999.0, 0.0},
  {0, NULL}
};

static gboolean read_raw_hwmon_value(const gchar *dir, const gchar *file, gchar **result) {
    gchar *full_path;
    gboolean file_result;

    full_path = g_strdup_printf("/sys/class/hwmon/%s/%s", dir, file);
    file_result = g_file_get_contents(full_path, result, NULL, NULL);

    g_free(full_path);
    return file_result;
}

gboolean zenpower_init() {
    GDir *hwmon;
    const gchar *entry;
    gchar* name = NULL;
    
    hwmon = g_dir_open("/sys/class/hwmon", 0, NULL);
    if (!hwmon)
        return FALSE;

    while ((entry = g_dir_read_name(hwmon))) {
        read_raw_hwmon_value(entry, "name", &name);
        if (strcmp(g_strchomp(name), "zenpower") == 0) {
            zenpowerDir = g_strdup(entry);
            break;
        }
        g_free(name);
    }

    if (!zenpowerDir)
        return FALSE;
}

void zenpower_update() {
    gchar *tmp = NULL;
    GSList *list = NULL;
    HwmonSensor *sensor;

    for (sensor = hwmon_sensors; sensor->label; sensor++) {
        if (read_raw_hwmon_value(zenpowerDir, sensor->file, &tmp)){
            sensor->current_value = atof(tmp) / sensor->adjust_ratio;

            if (sensor->current_value < sensor->min)
                sensor->min = sensor->current_value;

            if (sensor->current_value > sensor->max)
                sensor->max = sensor->current_value;

            g_free(tmp);
        }
        else{
            sensor->current_value = ERROR_VALUE;
        }
    }
}

GSList* zenpower_get_sensors() {
    GSList *list = NULL;
    HwmonSensor *sensor;
    SensorInit *data;

    for (sensor = hwmon_sensors; sensor->label; sensor++) {
        data = sensor_init_new();
        data->label = g_strdup(sensor->label);
        data->value = &sensor->current_value;
        data->min = &sensor->min;
        data->max = &sensor->max;
        data->printf_format = sensor->printf_format;
        list = g_slist_append(list, data);
    }

    return list;
}
