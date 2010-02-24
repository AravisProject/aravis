#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdlib.h>

static gboolean debug_checked = FALSE;
static gboolean debug_enabled = FALSE;

static gboolean
_is_debug_enabled ()
{
	const char *debug_var;

	if (debug_checked)
		return debug_enabled;

	debug_var = g_getenv ("ARV_DEBUG");

	debug_enabled = debug_var != NULL ? atoi (debug_var) != 0 : FALSE;

	debug_checked = TRUE;

	return debug_enabled;
}

void
arv_debug (char const *format, ...)
{
	va_list args;

	if (!_is_debug_enabled())
		return;

	va_start (args, format);
	g_vprintf (format, args);
	g_printf ("\n");
	va_end (args);
}

void
arv_debug_enable (void)
{
	debug_enabled = TRUE;
	debug_checked = TRUE;
}
