#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msr.h"
#include "os.h"
#include "zenpower.h"
#include "zenmonitor.h"

gboolean display_coreid = 0;
gdouble delay = 0.5;
gchar *format = "";
gchar *columns[2048];

static GOptionEntry options[] =
{
    { "format", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &format, "Output format (csv, json)", "FORMAT" },
    { "delay", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_DOUBLE, &delay, "Interval of refreshing informations", "SECONDS" },
    { "coreid", 'c', 0, G_OPTION_ARG_NONE, &display_coreid, "Display core_id instead of core index", NULL },
    { NULL }
};

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

void init_sensors() {
    GSList *sensor;
    SensorSource *source;
    const SensorInit *data;
    guint i = 0;

    for(source = sensor_sources; source->drv; source++)
    {
        if(source->func_init())
        {
            source->sensors = source->func_get_sensors();
            if(source->sensors != NULL)
            {
                source->enabled = TRUE;
                sensor = source->sensors;
                while(sensor)
                {
                    data = (SensorInit*)sensor->data;
                    columns[i++] = data->label;
                    sensor = sensor->next;
                }
            }
        }
    }
    columns[i] = NULL;
}

void update_data()
{
    SensorSource *source;
    GSList *node;
    const SensorInit *sensorData;

    for(source = sensor_sources; source->drv; source++)
    {
        if(source->enabled)
        {
            source->func_update();
            if(source->sensors)
            {
                node = source->sensors;

                while(node)
                {
                    sensorData = (SensorInit*)node->data;
                    printf("%s\t%f\n", sensorData->label, *sensorData->value);
                    node = node->next;
                }
            }
        }
    }
}

void start_watching()
{
    guint i = 0;
    while(columns[i])
    {
        printf("%s\n", columns[i++]);
    }

    while(1)
    {
        update_data();
        usleep(delay * 1000 * 1000);
    }
}

int main(int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;

    context = g_option_context_new("- Zenmonitor options");
    g_option_context_add_main_entries(context, options, NULL);
    if(!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    init_sensors();
    start_watching();

    return EXIT_SUCCESS;
}
