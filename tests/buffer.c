#include <glib.h>
#include <arv.h>

static void
simple_buffer_test (void)
{
	ArvBuffer *buffer;

	buffer = arv_buffer_new (1024, NULL);

	g_assert (ARV_IS_BUFFER (buffer));
	g_assert (buffer->data != NULL);
	g_assert (buffer->size == 1024);

	g_assert (buffer->user_data == NULL);
	g_assert (buffer->user_data_destroy_func == NULL);

	g_assert (buffer->status == ARV_BUFFER_STATUS_CLEARED);

	g_object_unref (buffer);
}

static void
preallocated_buffer_test (void)
{
	ArvBuffer *buffer;
	void *data = g_malloc (1024);

	buffer = arv_buffer_new (1024, data);
	g_assert (ARV_IS_BUFFER (buffer));
	g_assert (buffer->data == data);

	g_assert (buffer->user_data == NULL);
	g_assert (buffer->user_data_destroy_func == NULL);

	g_assert (buffer->status == ARV_BUFFER_STATUS_CLEARED);

	g_object_unref (buffer);
}

static void
full_buffer_destroy_func (void *data)
{
	*((int *) data) = 4321;
}

static void
full_buffer_test (void)
{
	ArvBuffer *buffer;
	int value = 1234;


	buffer = arv_buffer_new_full (1024, NULL, &value, full_buffer_destroy_func);

	g_assert (ARV_IS_BUFFER (buffer));
	g_assert (buffer->data != NULL);
	g_assert (buffer->size == 1024);

	g_assert (buffer->user_data == &value);
	g_assert (buffer->user_data_destroy_func == full_buffer_destroy_func);

	g_assert (buffer->status == ARV_BUFFER_STATUS_CLEARED);

	g_object_unref (buffer);

	g_assert (value == 4321);
}

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	g_test_add_func ("/buffer/simple-buffer", simple_buffer_test);
	g_test_add_func ("/buffer/preallocated-buffer", preallocated_buffer_test);
	g_test_add_func ("/buffer/full-buffer", full_buffer_test);

	return g_test_run();
}

