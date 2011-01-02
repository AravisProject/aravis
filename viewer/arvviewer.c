#include <gtk/gtk.h>
#include <stdlib.h>

int
main (int argc,char *argv[])
{
	GtkBuilder *builder;
	GtkWidget *window;
	char *ui_filename;

	gtk_init (&argc, &argv);

	builder = gtk_builder_new ();

	ui_filename = g_build_filename (ARAVIS_DATA_DIR, "arv-viewer.ui", NULL);
	gtk_builder_add_from_file (builder, ui_filename, NULL);
	g_free (ui_filename);

	window = GTK_WIDGET (gtk_builder_get_object (builder, "main_window"));

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_widget_show_all (window);

	g_object_unref (builder);

	gtk_main ();

	return EXIT_SUCCESS;
}
