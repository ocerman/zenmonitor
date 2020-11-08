#ifndef __ZENMONITOR_ZENMONITOR_H__
#define __ZENMONITOR_ZENMONITOR_H__

#include <time.h>
#include <glib.h>

#define ERROR_VALUE -999.0
#define VERSION "1.4.2"

typedef struct
{
    gchar *label;
    gchar *hint;
    float *value;
    float *min;
    float *max;
    const gchar *printf_format;
}
SensorInit;

typedef struct {
    const gchar *drv;
    gboolean  (*func_init)();
    GSList* (*func_get_sensors)();
    void (*func_update)();
    void (*func_clear_minmax)();
    gboolean enabled;
    GSList *sensors;
} SensorSource;

typedef struct {
    GPtrArray *labels;
    GPtrArray *data;
    GArray *time;
} SensorDataStore;

SensorInit* sensor_init_new(void);
void sensor_init_free(SensorInit *s);
gboolean check_zen();
gchar *cpu_model();
guint get_core_count();
SensorDataStore* sensor_data_store_new();
void sensor_data_store_add_entry(SensorDataStore *store, gchar *entry);
gint sensor_data_store_drop_entry(SensorDataStore *store, gchar *entry);
void sensor_data_store_keep_time(SensorDataStore *store);
gint sensor_data_store_add_data(SensorDataStore *store, gchar *entry, float data);
void sensor_data_store_free(SensorDataStore *store);
extern gboolean display_coreid;

#endif /* __ZENMONITOR_ZENMONITOR_H__ */
