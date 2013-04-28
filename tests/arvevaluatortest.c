#include <arv.h>
#include <stdlib.h>

static char **arv_option_expressions = NULL;
static char *arv_option_debug_domains = NULL;

static const GOptionEntry arv_option_entries[] =
{
	{ G_OPTION_REMAINING,	' ', 0, G_OPTION_ARG_STRING_ARRAY,
		&arv_option_expressions,		NULL, NULL},
	{ "debug", 		'd', 0, G_OPTION_ARG_STRING,
		&arv_option_debug_domains, 	"Debug mode", NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	ArvEvaluator *evaluator;
	GOptionContext *context;
	GError *error = NULL;
	int i;
	double value;

	arv_g_type_init ();

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, arv_option_entries, NULL);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_option_context_free (context);
		g_print ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	arv_debug_enable (arv_option_debug_domains);

	evaluator = arv_evaluator_new (NULL);

	arv_evaluator_set_double_variable (evaluator, "TDBL", 124.2);
	arv_evaluator_set_int64_variable (evaluator, "TINT", 3200);

	if (arv_option_expressions == NULL) {
		g_print ("Missing expression.\n");
		return EXIT_FAILURE;
	}

	for (i = 0; arv_option_expressions[i] != NULL; i++) {
		arv_evaluator_set_expression (evaluator, arv_option_expressions[i]);
		value = arv_evaluator_evaluate_as_double (evaluator, &error);

		if (error != NULL) {
			g_print ("Error in '%s': %s\n", arv_option_expressions[i],
				 error->message);
			g_error_free (error);
			error = NULL;
		} else
			g_print ("%s = %g\n", arv_option_expressions[i], value);
	}

	g_object_unref (evaluator);

	return EXIT_SUCCESS;
}
