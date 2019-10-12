#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "zenmonitor.h"
#include "zenpower.h"

GSList *zp_sensors = NULL;
static int nodes = 0;

typedef struct
{
    const gchar *label;
    const gchar *file;
    const gchar *printf_format;
    const double adjust_ratio;
} HwmonSensorType;

typedef struct
{
    float current_value;
    float min;
    float max;
    HwmonSensorType *type;
    gchar *hwmon_dir;
    int node;
} HwmonSensor;

static HwmonSensorType hwmon_stype[] = {
  {"CPU Temperature (tCtrl)",   "temp1_input",  " %6.2f°C", 1000.0},
  {"CPU Temperature (tDie)",    "temp2_input",  " %6.2f°C", 1000.0},
  {"CPU Core Voltage (SVI2)",   "in1_input",    " %8.3f V", 1000.0},
  {"SOC Voltage (SVI2)",        "in2_input",    " %8.3f V", 1000.0},
  {"CPU Core Current (SVI2)",   "curr1_input",  " %8.3f A", 1000.0},
  {"SOC Current (SVI2)",        "curr2_input",  " %8.3f A", 1000.0},
  {"CPU Core Power (SVI2)",     "power1_input", " %8.3f W", 1000000.0},
  {"SOC Power (SVI2)",          "power2_input", " %8.3f W", 1000000.0},
  {0, NULL}
};

static gboolean hwmon_file_exists(const gchar *dir, const gchar *file) {
    gchar *full_path;
    gboolean result;

    full_path = g_strdup_printf("/sys/class/hwmon/%s/%s", dir, file);
    result = g_file_test(full_path, G_FILE_TEST_EXISTS);

    g_free(full_path);
    return result;
}

static gboolean read_raw_hwmon_value(const gchar *dir, const gchar *file, gchar **result) {
    gchar *full_path;
    gboolean file_result;

    full_path = g_strdup_printf("/sys/class/hwmon/%s/%s", dir, file);
    file_result = g_file_get_contents(full_path, result, NULL, NULL);

    g_free(full_path);
    return file_result;
}

static HwmonSensor *hwmon_sensor_new(HwmonSensorType *type, const gchar *dir, gint node) {
    HwmonSensor *s;
    s = g_new0(HwmonSensor, 1);
    s->min = 999.0;
    s->type = type;
    s->hwmon_dir = g_strdup(dir);
    s->node = node;
    return s;
}

gboolean zenpower_init() {
    GDir *hwmon;
    const gchar *entry;
    gchar *name = NULL;
    HwmonSensorType *type;

    hwmon = g_dir_open("/sys/class/hwmon", 0, NULL);
    if (!hwmon)
        return FALSE;

    while ((entry = g_dir_read_name(hwmon))) {
        read_raw_hwmon_value(entry, "name", &name);

        if (strcmp(g_strchomp(name), "zenpower") == 0) {

            for (type = hwmon_stype; type->label; type++) {
                if (hwmon_file_exists(entry, type->file)) {
                    zp_sensors = g_slist_append(zp_sensors, hwmon_sensor_new(type, entry, nodes));
                }
            }
            nodes++;

        }
        g_free(name);
    }

    if (zp_sensors == NULL)
        return FALSE;

    return TRUE;
}

void zenpower_update() {
    gchar *tmp = NULL;
    GSList *node;
    HwmonSensor *sensor;

    node = zp_sensors;
    while(node) {
        sensor = (HwmonSensor *)node->data;

        if (read_raw_hwmon_value(sensor->hwmon_dir, sensor->type->file, &tmp)){
            sensor->current_value = atof(tmp) / sensor->type->adjust_ratio;

            if (sensor->current_value < sensor->min)
                sensor->min = sensor->current_value;

            if (sensor->current_value > sensor->max)
                sensor->max = sensor->current_value;

            g_free(tmp);
        }
        else{
            sensor->current_value = ERROR_VALUE;
        }
        node = node->next;
    }
}

void zenpower_clear_minmax() {
    HwmonSensor *sensor;
    GSList *node;
    node = zp_sensors;
    while(node) {
        sensor = (HwmonSensor *)node->data;
        sensor->min = sensor->current_value;
        sensor->max = sensor->current_value;
        node = node->next;
    }
}

GSList* zenpower_get_sensors() {
    GSList *list = NULL;
    HwmonSensor *sensor;
    GSList *node;
    SensorInit *data;

    node = zp_sensors;
    while(node) {
        sensor = (HwmonSensor *)node->data;

        data = sensor_init_new();
        if (nodes > 1){
            data->label = g_strdup_printf("Node %d - %s", sensor->node, sensor->type->label);
        }
        else{
            data->label = g_strdup(sensor->type->label);
        }
        data->value = &sensor->current_value;
        data->min = &sensor->min;
        data->max = &sensor->max;
        data->printf_format = sensor->type->printf_format;
        list = g_slist_append(list, data);

        node = node->next;
    }

    return list;
}
