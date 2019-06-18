#define ERROR_VALUE -999.0

typedef struct
{
    gchar *label;
    float *value;
    const gchar *printf_format;
}
SensorInit;

typedef struct {
    const gchar *drv;
    gboolean  (*func_init)();
    GSList* (*func_get_sensors)();
    void (*func_update)();
    gboolean enabled;
    GSList *sensors;
    
} SensorSource;

SensorInit* sensor_init_new(void);
void sensor_init_free(SensorInit *s);
gboolean check_zen();
gchar *cpu_model();
