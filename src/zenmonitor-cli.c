#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "msr.h"
#include "os.h"
#include "zenpower.h"
#include "zenmonitor.h"

gboolean display_coreid = 0;
gdouble delay = 0.5;
gchar *file = "";
SensorDataStore *store;
int quit = 0;

static GOptionEntry options[] = {
    {"file", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING, &file,
     "Output to csv file", "FILE"},
    {"delay", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_DOUBLE, &delay,
     "Interval of refreshing informations", "SECONDS"},
    {"coreid", 'c', 0, G_OPTION_ARG_NONE, &display_coreid,
     "Display core_id instead of core index", NULL},
    {NULL}};

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

void flush_to_csv()
{
    FILE *csv;
    csv = fopen(file, "w");
    guint i = 0;
    int cont = 1;

    fprintf(csv, "time(epoch),");
    while(cont)
    {
        fprintf(csv, "%s", (char*)g_ptr_array_index(store->labels, i++));
        if(g_ptr_array_index(store->labels, i))
        {
            fprintf(csv, ",");
        }
        else
        {
            cont = 0;
        }
    }
    
    fprintf(csv, "\n");
    cont = 1;
    i = 0;

    while(cont)
    {
        struct timespec ts = g_array_index(store->time, struct timespec, i);
        fprintf(csv, "%ld.%.9ld,", ts.tv_sec, ts.tv_nsec);

        int j = 0;
        int cont2 = 1;
        while(cont2)
        {
            GArray *data = g_ptr_array_index(store->data, j++);
            fprintf(csv, "%f", g_array_index(data, float, i));
            if(g_ptr_array_index(store->data, j))
            {
                fprintf(csv, ",");
            }
            else
            {
                cont2 = 0;
            }
        }
        fprintf(csv, "\n");

        i++;

        if(g_array_index(store->time, struct timespec, i).tv_sec <= 0)
        {
            cont = 0;
        }
    }

    fclose(csv);

    quit = 1;
}

void init_sensors()
{
    GSList *sensor;
    SensorSource *source;
    const SensorInit *data;

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
                    sensor_data_store_add_entry(store, data->label);
                    sensor = sensor->next;
                }
            }
        }
    }
}

void update_data()
{
    SensorSource *source;
    GSList *node;
    const SensorInit *sensorData;

    sensor_data_store_keep_time(store);

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
                    sensor_data_store_add_data(store, sensorData->label, *sensorData->value);
                    printf("%s\t%f\n", sensorData->label, *sensorData->value);
                    node = node->next;
                }
            }
        }
    }
    printf("\v");
}

void start_watching()
{
    while(!quit)
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

    if(strcmp(file, "") != 0)
    {
        signal(SIGINT, flush_to_csv);
    }
    store = sensor_data_store_new();

    init_sensors();
    start_watching();

    sensor_data_store_free(store);

    return EXIT_SUCCESS;
}
