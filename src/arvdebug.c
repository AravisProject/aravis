#include <arvdebug.h>
#include <glib/gprintf.h>
#include <stdlib.h>

static gboolean arv_debug_checked = FALSE;
static gboolean arv_debug_level = ARV_DEBUG_LEVEL_NONE;

static gboolean
arv_debug_get_level (void)
{
	const char *debug_var;

	if (arv_debug_checked)
		return arv_debug_level;

	debug_var = g_getenv ("ARV_DEBUG");

	arv_debug_level = debug_var != NULL ? atoi (debug_var) : ARV_DEBUG_LEVEL_NONE;

	arv_debug_checked = TRUE;

	return arv_debug_level;
}

void
arv_debug (ArvDebugLevel level, char const *format, ...)
{
	va_list args;

	if (level > arv_debug_get_level ())
		return;

	va_start (args, format);
	g_vprintf (format, args);
	g_printf ("\n");
	va_end (args);
}

gboolean
arv_debug_check (ArvDebugLevel level)
{
	return arv_debug_get_level () >= level;
}

void
arv_debug_enable (ArvDebugLevel level)
{
	arv_debug_level = level;
	arv_debug_checked = TRUE;
}
