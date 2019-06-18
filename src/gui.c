#include <cpuid.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "zenmonitor.h"

GtkWidget *window;

static GtkTreeModel *model = NULL;
static guint timeout = 0;
static SensorSource *sensor_sources;

enum {
    COLUMN_NAME,
    COLUMN_VALUE,
    NUM_COLUMNS
};

static void init_sensors() {
    GtkTreeIter iter;
    GSList *sensor;
    GtkListStore *store;
    SensorSource *source;
    const SensorInit *data;
    gboolean added;
    guint i = 0;

    store = GTK_LIST_STORE(model);
    for (source = sensor_sources; source->drv; source++) {
        if (source->func_init()){
            source->sensors = source->func_get_sensors();
            if (source->sensors != NULL) {
                source->enabled = TRUE;

                sensor = source->sensors;
                while (sensor) {
                    data = (SensorInit*)sensor->data;
                    gtk_list_store_append(store, &iter);
                    gtk_list_store_set(store, &iter,
                                       COLUMN_NAME, data->label,
                                       COLUMN_VALUE, " --- ",
                                       -1);
                    sensor = sensor->next;
                    i++;
                }
            }
        }
    }
}

static GtkTreeModel* create_model (void) {
    GtkListStore *store;
    store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    return GTK_TREE_MODEL (store);
}

static gboolean update_data (gpointer data) {
    GtkTreeIter iter;
    guint number;
    GSList *node;
    gchar *value;
    SensorSource *source;
    const SensorInit *sensorData;

    if (model == NULL)
        return G_SOURCE_REMOVE;

    if (!gtk_tree_model_get_iter_first (model, &iter))
        return G_SOURCE_REMOVE;

    for (source = sensor_sources; source->drv; source++) {
        if (!source->enabled)
            continue;
            
        source->func_update();
        if (source->sensors){
            node = source->sensors;

            while(node) {
                sensorData = (SensorInit*)node->data;
                if (*(sensorData->value) != ERROR_VALUE)
                    value = g_strdup_printf(sensorData->printf_format, *(sensorData->value));
                else
                    value = g_strdup("    ? ? ?");
                
                gtk_list_store_set(GTK_LIST_STORE (model), &iter, COLUMN_VALUE, value, -1);
                node = node->next;
                if (!gtk_tree_model_iter_next(model, &iter))
                    break;
            }
        }
    }
    return G_SOURCE_CONTINUE;
}

static void add_columns (GtkTreeView *treeview) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);

  // NAME
  renderer = gtk_cell_renderer_text_new ();
  
  column = gtk_tree_view_column_new_with_attributes ("Sensor", renderer,
                                                     "text", COLUMN_NAME,
                                                     NULL);
  g_object_set(renderer, "family", "monotype", NULL);
  gtk_tree_view_append_column (treeview, column);

  //VALUE
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Value", renderer,
                                                     "text", COLUMN_VALUE,
                                                     NULL);
  g_object_set(renderer, "family", "monotype", NULL);
  gtk_tree_view_append_column (treeview, column);
}

static void about_btn_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *dialog;
    const gchar *website = "https://github.com/ocerman/zenmonitor";
    const gchar *msg = "<b>Zen Monitor</b>\n"
                       "Monitoring software for AMD Zen-based CPUs\n"
                       "<a href=\"%s\">%s</a>\n\n"
                       "Created by: Ondrej Čerman";

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW (window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    msg, website, website);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

int start_gui (SensorSource *ss) {
    GtkWidget *button;
    GtkWidget *header;
    GtkWidget *treeview;
    GtkWidget *sw;
    GtkWidget *vbox;
    GtkWidget *dialog;
    GtkWidget *image;
    GIcon *icon;
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 330, 300);

    header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR (header), "Zen monitor");
    gtk_header_bar_set_has_subtitle(GTK_HEADER_BAR (header), FALSE);
    gtk_window_set_titlebar (GTK_WINDOW (window), header);

    button = gtk_button_new();
    icon = g_themed_icon_new("dialog-information");
    image = gtk_image_new_from_gicon(icon, GTK_ICON_SIZE_BUTTON);
    g_object_unref(icon);
    gtk_container_add(GTK_CONTAINER (button), image);
    gtk_header_bar_pack_start(GTK_HEADER_BAR (header), button);
    g_signal_connect (button, "clicked", G_CALLBACK (about_btn_clicked), NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER (window), vbox);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX (vbox), sw, TRUE, TRUE, 0);

    model = create_model();
    treeview = gtk_tree_view_new_with_model(model);
    g_object_unref(model);

    gtk_container_add (GTK_CONTAINER(sw), treeview);
    add_columns(GTK_TREE_VIEW(treeview));
    gtk_widget_show_all(window);

    if (check_zen()){
        sensor_sources = ss;
        init_sensors();
        timeout = g_timeout_add(300, update_data, NULL);
    }
    else{
        dialog = gtk_message_dialog_new(GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                        "Zen CPU not detected!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }

    gtk_main();
    return 0;
}

