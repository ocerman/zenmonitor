#include <cpuid.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "zenmonitor.h"

GtkWidget *window;

static GtkTreeModel *model = NULL;
static guint timeout = 0;
static SensorSource *sensor_sources;
static const guint defaultHeight = 350;

enum {
    COLUMN_NAME,
    COLUMN_HINT,
    COLUMN_VALUE,
    COLUMN_MIN,
    COLUMN_MAX,
    NUM_COLUMNS
};

static void init_sensors() {
    GtkTreeIter iter;
    GSList *sensor;
    GtkListStore *store;
    SensorSource *source;
    const SensorInit *data;
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
                                       COLUMN_NAME,  data->label,
                                       COLUMN_HINT,  data->hint,
                                       COLUMN_VALUE, " --- ",
                                       COLUMN_MIN,   " --- ",
                                       COLUMN_MAX,   " --- ",
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
    store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    return GTK_TREE_MODEL (store);
}

static void set_list_column_value(float num, const gchar *printf_format, GtkTreeIter *iter, gint column){
    gchar *value;
    if (num != ERROR_VALUE)
        value = g_strdup_printf(printf_format, num);
    else
        value = g_strdup("    ? ? ?");
    gtk_list_store_set(GTK_LIST_STORE (model), iter, column, value, -1);
    g_free(value);
}

static gboolean update_data (gpointer data) {
    GtkTreeIter iter;
    GSList *node;
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
                set_list_column_value(*(sensorData->value), sensorData->printf_format, &iter, COLUMN_VALUE);
                set_list_column_value(*(sensorData->min), sensorData->printf_format, &iter, COLUMN_MIN);
                set_list_column_value(*(sensorData->max), sensorData->printf_format, &iter, COLUMN_MAX);

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

    //MIN
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Min", renderer,
                                                     "text", COLUMN_MIN,
                                                     NULL);
    g_object_set(renderer, "family", "monotype", NULL);
    gtk_tree_view_append_column (treeview, column);

    //MAX
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes ("Max", renderer,
                                                     "text", COLUMN_MAX,
                                                     NULL);
    g_object_set(renderer, "family", "monotype", NULL);
    gtk_tree_view_append_column (treeview, column);
}

static void about_btn_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *dialog;
    const gchar *website = "https://github.com/ocerman/zenmonitor";
    const gchar *msg = "<b>Zen Monitor</b> %s\n"
                       "Monitoring software for AMD Zen-based CPUs\n"
                       "<a href=\"%s\">%s</a>\n\n"
                       "Created by: Ondrej Čerman";

    dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW (window),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                    msg, VERSION, website, website);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void clear_btn_clicked(GtkButton *button, gpointer user_data) {
    SensorSource *source;

    for (source = sensor_sources; source->drv; source++) {
        if (!source->enabled)
            continue;

        source->func_clear_minmax();
    }
}

static gboolean mid_search_eq_func(GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter) {
    gchar *iter_string = NULL, *lc_iter_string = NULL, *lc_key = NULL;
    gboolean result;

    gtk_tree_model_get(model, iter, column, &iter_string, -1);
    lc_iter_string = g_utf8_strdown(iter_string, -1);
    lc_key = g_utf8_strdown(key, -1);

    result = (g_strrstr(lc_iter_string, lc_key) == NULL);

    g_free(iter_string);
    g_free(lc_iter_string);
    g_free(lc_key);

    return result;
}

static void resize_to_treeview(GtkWindow* window, GtkTreeView* treeview) {
    gint uiHeight, cellHeight, vSeparator, rows;
    GdkRectangle r;

    GtkTreeViewColumn *col = gtk_tree_view_get_column(treeview, 0);
    if (!col)
        return;

    gtk_tree_view_column_cell_get_size(col, NULL, NULL, NULL, NULL, &cellHeight);
    gtk_widget_style_get(GTK_WIDGET(treeview), "vertical-separator", &vSeparator, NULL);
    rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(treeview), NULL);

    gtk_tree_view_get_visible_rect(treeview, &r);
    uiHeight = defaultHeight - r.height;

    gtk_window_resize(window, 500, uiHeight + (vSeparator + cellHeight) * rows);
}

int start_gui (SensorSource *ss) {
    GtkWidget *about_btn;
    GtkWidget *clear_btn;
    GtkWidget *box;
    GtkWidget *header;
    GtkWidget *treeview;
    GtkWidget *sw;
    GtkWidget *vbox;
    GtkWidget *dialog;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 500, defaultHeight);

    header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR (header), "Zen monitor");
    gtk_header_bar_set_has_subtitle(GTK_HEADER_BAR (header), TRUE);
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR (header), cpu_model());
    gtk_window_set_titlebar (GTK_WINDOW (window), header);

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class (gtk_widget_get_style_context (box), "linked");

    about_btn = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(about_btn), gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_BUTTON));
    gtk_container_add(GTK_CONTAINER(box), about_btn);
    gtk_widget_set_tooltip_text(about_btn, "About Zen monitor");

    clear_btn = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(clear_btn), gtk_image_new_from_icon_name("edit-clear-all", GTK_ICON_SIZE_BUTTON));
    gtk_container_add(GTK_CONTAINER(box), clear_btn);
    gtk_widget_set_tooltip_text(clear_btn, "Clear Min/Max");

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), box);
    g_signal_connect(about_btn, "clicked", G_CALLBACK(about_btn_clicked), NULL);
    g_signal_connect(clear_btn, "clicked", G_CALLBACK(clear_btn_clicked), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER (window), vbox);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX (vbox), sw, TRUE, TRUE, 0);

    model = create_model();
    treeview = gtk_tree_view_new_with_model(model);
    gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(treeview), COLUMN_HINT);

    gtk_container_add (GTK_CONTAINER(sw), treeview);
    add_columns(GTK_TREE_VIEW(treeview));
    gtk_widget_show_all(window);

    gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview), COLUMN_NAME);
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview),
        (GtkTreeViewSearchEqualFunc)mid_search_eq_func, model, NULL);

    g_object_unref(model);

    if (check_zen()){
        sensor_sources = ss;
        init_sensors();

        resize_to_treeview(GTK_WINDOW(window), GTK_TREE_VIEW(treeview));
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
