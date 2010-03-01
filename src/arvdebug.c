#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdlib.h>

static gboolean arv_debug_checked = FALSE;
static gboolean arv_debug_level = ARV_DEBUG_LEVEL_NONE;

static gboolean
_get_debug_level ()
{
	const char *debug_var;

	if (arv_debug_checked)
		return arv_debug_level;

	debug_var = g_getenv ("ARV_DEBUG");

	arv_debug_level = debug_var != NULL ? atoi (debug_var) != 0 : ARV_DEBUG_LEVEL_NONE;

	arv_debug_checked = TRUE;

	return arv_debug_level;
}

void
arv_debug (ArvDebugLevel level, char const *format, ...)
{
	va_list args;

	if (_get_debug_level () < level)
		return;

	va_start (args, format);
	g_vprintf (format, args);
	g_printf ("\n");
	va_end (args);
}

void
arv_debug_enable (ArvDebugLevel level)
{
	arv_debug_level = level;
	arv_debug_checked = TRUE;
}
