#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

// Function to execute the emulator command
void run_emulator(const gchar *rom_path) {
    char command[1024];
    sprintf(command, "./build/gbemu/gbemu \"%s\"", rom_path);
    system(command);
}

// Function to handle file chooser dialog response
void file_chooser_response(GtkFileChooserDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_ACCEPT) {
        gchar *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (file_path != NULL) {
            run_emulator(file_path);
            g_free(file_path);
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

// Function to open file chooser dialog
void open_file_chooser(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open ROM File", NULL,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "Cancel", GTK_RESPONSE_CANCEL,
                                                    "Open", GTK_RESPONSE_ACCEPT,
                                                    NULL);
    gtk_dialog_run(GTK_DIALOG(dialog));
    g_signal_connect(dialog, "response", G_CALLBACK(file_chooser_response), NULL);
}

// Main function
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gameboy Emulator");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 300, 100);

    GtkWidget *button = gtk_button_new_with_label("Open ROM");
    g_signal_connect(button, "clicked", G_CALLBACK(open_file_chooser), NULL);
    gtk_container_add(GTK_CONTAINER(window), button);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
