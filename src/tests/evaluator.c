#include <glib.h>
#include <arv.h>

typedef struct {
	const char *test_name;
	const char *expression;
	gint64 result_int64;
	double result_double;
} ExpressionTestData;

static const ExpressionTestData expression_test_data[] = {
	{"/evaluator/addition", 	"10+2", 		12, 	12.0},
	{"/evaluator/parentheses",	"(24+2)*2",		52, 	52.0},
	{"/evaluator/precedence",	"24+2*8",		40, 	40.0},
	{"/evaluator/sin",		"sin(PI/2.0)",		1,	1.0},
	{"/evaluator/ternary-true",	"1=1?1:0",		1,	1.0},
	{"/evaluator/ternary-false",	"1=0?1:0",		0,	0.0},
	{"/evaluator/greater-true",	"2>1",			1,	1.0},
	{"/evaluator/greater-false",	"2>2",			0,	0.0},
	{"/evaluator/lower-true",	"1<2",			1,	1.0},
	{"/evaluator/lower-false",	"2<2",			0,	0.0}
};

static void
expression_test (ExpressionTestData *data)
{
	ArvEvaluator *evaluator;
	gint64 v_int64;
	double v_double;

	evaluator = arv_evaluator_new (data->expression);

	v_int64 = arv_evaluator_evaluate_as_int64 (evaluator, NULL);
	v_double = arv_evaluator_evaluate_as_double (evaluator, NULL);

	g_assert_cmpint (v_int64, ==, data->result_int64);
	g_assert_cmpfloat (v_double, ==, data->result_double);

	g_object_unref (evaluator);
}

static void
set_get_expression_test (void)
{
	ArvEvaluator *evaluator;
	const char *expression;

	evaluator = arv_evaluator_new (NULL);

	expression = arv_evaluator_get_expression (evaluator);
	g_assert_cmpstr (expression, == , NULL);

	arv_evaluator_set_expression (evaluator, "1+1");
	expression = arv_evaluator_get_expression (evaluator);
	g_assert_cmpstr (expression, == , "1+1");

	arv_evaluator_set_expression (evaluator, NULL);
	expression = arv_evaluator_get_expression (evaluator);
	g_assert_cmpstr (expression, == , NULL);

	g_object_unref (evaluator);
}

static void
set_double_variable_test (void)
{
	ArvEvaluator *evaluator;
	const char *expression;
	double v_double;

	evaluator = arv_evaluator_new ("V_DBLA+V_DBLB");

	arv_evaluator_set_double_variable (evaluator, "V_DBLA", 123.0);
	arv_evaluator_set_double_variable (evaluator, "V_DBLB", 0.4);
	v_double = arv_evaluator_evaluate_as_double (evaluator, NULL);

	g_assert_cmpfloat (v_double, ==, 123.4);

	g_object_unref (evaluator);
}

static void
set_int64_variable_test (void)
{
	ArvEvaluator *evaluator;
	const char *expression;
	gint64 v_int64;

	evaluator = arv_evaluator_new ("A");

	arv_evaluator_set_int64_variable (evaluator, "A", 123);
	v_int64 = arv_evaluator_evaluate_as_int64 (evaluator, NULL);

	g_assert_cmpfloat (v_int64, ==, 123);

	g_object_unref (evaluator);
}

int
main (int argc, char *argv[])
{
	int i;

	g_test_init (&argc, &argv, NULL);

	g_type_init ();

	for (i = 0; i < G_N_ELEMENTS (expression_test_data); i++)
		g_test_add_data_func (expression_test_data[i].test_name,
				      &expression_test_data[i],
				      (void *) expression_test);

	g_test_add_func ("/evaluator/set-get-expression", set_get_expression_test);
	g_test_add_func ("/evaluator/double-variable", set_double_variable_test);
	g_test_add_func ("/evaluator/int64-variable", set_int64_variable_test);

	return g_test_run();
}
