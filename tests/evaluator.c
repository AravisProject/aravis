#include <glib.h>
#include <arv.h>
#include <math.h>

typedef struct {
	const char *test_name;
	const char *expression;
	gint64 result_int64;
	gboolean expected_error_int64;
	double result_double;
} ExpressionTestData;

static const ExpressionTestData expression_test_data[] = {
	{"/evaluator/addition", 			"10+2", 			12, 	FALSE,	12.0},
	{"/evaluator/parentheses",			"(24+2)*2",			52, 	FALSE,	52.0},
	{"/evaluator/precedence",			"24+2*8",			40, 	FALSE,	40.0},
	{"/evaluator/ternary-true",			"1=1?1:0",			1,	FALSE,	1.0},
	{"/evaluator/ternary-false",			"1=0?1:0",			0,	FALSE,	0.0},
	{"/evaluator/greater-true",			"2>1",				1,	FALSE,	1.0},
	{"/evaluator/greater-false",			"2>2",				0,	FALSE,	0.0},
	{"/evaluator/lower-true",			"1<2",				1,	FALSE,	1.0},
	{"/evaluator/lower-false",			"2<2",				0,	FALSE,	0.0},
	{"/evaluator/substraction",			"10-8",				2,	FALSE,	2.0},
	{"/evaluator/substraction-float",		"10.1-8.1",			2,	FALSE,	2.0},
	{"/evaluator/multiplication",			"2.5*4",	       		8,	FALSE,	10.0},
	{"/evaluator/division",				"10/4",				2,	FALSE,	2.5},
	{"/evaluator/division-float",			"10.0/4",			2,	FALSE,	2.5},
	{"/evaluator/simple-minus", 			"-1",	 			-1, 	FALSE,	-1.0},
	{"/evaluator/minus",				"4*-3",				-12,	FALSE,	-12.0},
	{"/evaluator/plus",				"4*+3",				12,	FALSE,	12.0},
	{"/evaluator/remainder",			"10%3",				1,	FALSE,	1.0},
	{"/evaluator/power",				"2**10",			1024,	FALSE,	1024.0},
	{"/evaluator/power-precedence",			"2**10*2",			2048,	FALSE,	2048.0},
	{"/evaluator/ln",				"LN(E)",			1,	TRUE,	1.0},
	{"/evaluator/lg",				"LG(10)",			1,	TRUE,	1.0},
	{"/evaluator/sqrt",				"SQRT(16)",			4,	TRUE,	4.0},
	{"/evaluator/tan",				"TAN(0)",			0,	TRUE,	0.0},
	{"/evaluator/atan",				"ATAN(0)",			0,	TRUE,	0.0},
	{"/evaluator/exp",				"EXP(1)",			2,	TRUE,	M_E},
	{"/evaluator/trunc-plus",			"TRUNC(10.7)",			10,	TRUE,	10.0},
	{"/evaluator/trunc-minus",			"TRUNC(-11.9)",			-11,	TRUE,	-11.0},
	{"/evaluator/round-plus",			"ROUND(10.1)",			10,	TRUE,	10.0},
	{"/evaluator/round-plus-plus",			"ROUND(10.9)",			11,	TRUE,	11.0},
	{"/evaluator/round-minus",			"ROUND(-20.1)",			-20,	TRUE,	-20.0},
	{"/evaluator/round-minus-minus",		"ROUND(-20.9)",			-21,	TRUE,	-21.0},
	{"/evaluator/round-plus-with-precision",	"ROUND(10.11, 1)",		10,	TRUE,	10.1},
	{"/evaluator/round-plus-plus-with-precision",	"ROUND(10.99, 1)",		11,	TRUE,	11.0},
	{"/evaluator/round-minus-with-precision",	"ROUND(-20.11, 1)",		-20,	TRUE,	-20.1},
	{"/evaluator/round-minus-minus-with-precision",	"ROUND(-20.99, 1)",		-21,	TRUE,	-21.0},
	{"/evaluator/floor-plus",			"FLOOR(10.7)",			10,	TRUE,	10.0},
	{"/evaluator/floor-minus",			"FLOOR(-11.9)",			-12,	TRUE,	-12.0},
	{"/evaluator/ceil-plus",			"CEIL(10.7)",			11,	TRUE,	11.0},
	{"/evaluator/ceil-minus",			"CEIL(-11.9)",			-11,	TRUE,	-11.0},
	{"/evaluator/sign-plus",			"SGN(2)",			1,	FALSE,	1.0},
	{"/evaluator/sign-minus",			"SGN(-2)",			-1,	FALSE,	-1.0},
	{"/evaluator/sign-zero",			"SGN(0)",			0,	FALSE,	0.0},
	{"/evaluator/sign-plus-float",			"SGN(2.0)",			1,	FALSE,	1.0},
	{"/evaluator/sign-minus-floa",			"SGN(-2.0)",			-1,	FALSE,	-1.0},
	{"/evaluator/sign-zero-float",			"SGN(0.0)",			0,	FALSE,	0.0},
	{"/evaluator/neg",				"NEG(-1)",			1,	FALSE,	1.0},
	{"/evaluator/neg-float",			"NEG(-2.5)",			2,	FALSE,	2.5},
	{"/evaluator/and",				"255 & 8",			8,	FALSE,	8.0},
	{"/evaluator/or",				"128 | 8",			136,	FALSE,	136.0},
	{"/evaluator/xor",				"3 ^ 1",			2,	FALSE,	2.0},
	{"/evaluator/not",				"~255",				-256,   FALSE,	-256.0},
	{"/evaluator/not-equal-true",			"1<>2",				1,	FALSE,	1.0},
	{"/evaluator/not-equal-false",			"1<>1",				0,	FALSE,	0.0},
	{"/evaluator/equal-true",			"1=1",				1,	FALSE,	1.0},
	{"/evaluator/equal-false",			"1=2",				0,	FALSE,	0.0},
	{"/evaluator/less-true",			"1<2",				1,	FALSE,	1.0},
	{"/evaluator/less-false",			"1<1",				0,	FALSE,	0.0},
	{"/evaluator/greater-or-equal-true",		"2>=2",				1,	FALSE,	1.0},
	{"/evaluator/greater-or-equal-false",		"1>=2",				0,	FALSE,	0.0},
	{"/evaluator/less-or-equal-true",		"2<=2",				1,	FALSE,	1.0},
	{"/evaluator/less-or-equal-false",		"2<=1",				0,	FALSE,	0.0},
	{"/evaluator/not-equal-true-double",		"2.1<>2",			0,	FALSE,	1.0},
	{"/evaluator/not-equal-false-double",		"1.0<>1",			0,	FALSE,	0.0},
	{"/evaluator/equal-true-double",		"1.0=1",			1,	FALSE,	1.0},
	{"/evaluator/equal-false-double",		"2.1=2",			1,	FALSE,	0.0},
	{"/evaluator/greater-true-double",		"2.1>2",			0,	FALSE,	1.0},
	{"/evaluator/greater-false-double",		"2>2.1",			0,	FALSE,	0.0},
	{"/evaluator/less-true-double",			"2<2.1",			0,	FALSE,	1.0},
	{"/evaluator/less-false-double",		"2.1<2",			0,	FALSE,	0.0},
	{"/evaluator/greater-or-equal-true-double",	"2.1>=2",			1,	FALSE,	1.0},
	{"/evaluator/greater-or-equal-false-double",	"2>=2.1",			1,	FALSE,	0.0},
	{"/evaluator/less-or-equal-true-double",	"2<=2.1",			1,	FALSE,	1.0},
	{"/evaluator/less-or-equal-false-double",	"2.1<=2",			1,	FALSE,	0.0},
	{"/evaluator/logical-and-true",			"(2=2)&&(1=1)",			1,	FALSE,	1.0},
	{"/evaluator/logical-and-false",		"(2=2)&&(1=2)",			0,	FALSE,	0.0},
	{"/evaluator/logical-or-true",			"(2=2)||(1=2)",			1,	FALSE,	1.0},
	{"/evaluator/logical-or-false",			"(1=2)||(0=2)",			0,	FALSE,	0.0},
	{"/evaluator/left-shift",			"1<<4",				16,	FALSE,	16.0},
	{"/evaluator/right-shift",			"16>>4",			1,	FALSE,	1.0},
	{"/evaluator/cos",				"COS(PI)",			-1,	TRUE,	-1.0},
	{"/evaluator/sin",				"SIN(-PI/2)",			-1,	TRUE,	-1.0},
	{"/evaluator/acos",				"ACOS(1)",			0,	TRUE,	0.0},
	{"/evaluator/asin",				"ASIN(0)",			0,	TRUE,	0.0},
	{"/evaluator/abs",				"ABS(-1)",			1,	TRUE,	1.0},
	{"/evaluator/abs-float",			"ABS(-10.3)",			10,	TRUE,	10.3},
	{"/evaluator/abs64bits",			"ABS(-10000000000)",		10000000000,	TRUE,	1e10},

	{"/evaluator/bugs/681048-remaining-op",		"(0 & 1)=0?((0 & 1)+2):1",		2,	FALSE,	2.0},
	{"/evaluator/bugs/743025-division-by-zero",	"(4/(20/10000))",			2000,	TRUE,	2000.0},
	{"/evaluator/bugs/gh98-not-operatorprecedence",	"(~(~0xC2000221|0xFEFFFFFF)) ? 2:0", 	0, 	FALSE,	0.0}
};

static void
expression_test (ExpressionTestData *data)
{
	ArvEvaluator *evaluator;
	const char *expression;
	gint64 v_int64;
	double v_double;
	GError *error = NULL;

	evaluator = arv_evaluator_new (data->expression);

	v_int64 = arv_evaluator_evaluate_as_int64 (evaluator, &error);
	if (data->expected_error_int64) {
		g_assert (error != NULL);
		g_clear_error (&error);
	} else {
		g_assert (error == NULL);
		g_assert_cmpint (v_int64, ==, data->result_int64);
	}

	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error == NULL);
	g_assert_cmpfloat (v_double, ==, data->result_double);

	expression = arv_evaluator_get_expression (evaluator);
	g_assert_cmpstr (expression, ==, data->expression);

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
	GError *error = NULL;
	double v_double;

	evaluator = arv_evaluator_new ("V_DBLA+V_DBLB");

	arv_evaluator_set_double_variable (evaluator, "V_DBLA", 123.0);
	arv_evaluator_set_double_variable (evaluator, "V_DBLB", 0.4);
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert_cmpfloat (v_double, ==, 123.4);
	g_assert (error == NULL);

	g_object_unref (evaluator);
}

static void
set_int64_variable_test (void)
{
	ArvEvaluator *evaluator;
	GError *error = NULL;
	gint64 v_int64;

	evaluator = arv_evaluator_new ("A_1");

	arv_evaluator_set_int64_variable (evaluator, "A_1", 123);
	v_int64 = arv_evaluator_evaluate_as_int64 (evaluator, &error);
	g_assert_cmpfloat (v_int64, ==, 123);
	g_assert (error == NULL);

	g_object_unref (evaluator);
}

static void
sub_expression_test (void)
{
	ArvEvaluator *evaluator;
	GError *error = NULL;
	double v_double;
	const char *sub_expression;

	evaluator = arv_evaluator_new ("ZERO5 + SUB_EXP");
	arv_evaluator_set_sub_expression (evaluator, "ZERO5", "0.5");
	arv_evaluator_set_sub_expression (evaluator, "SUB_EXP", "2*X");
	arv_evaluator_set_int64_variable (evaluator, "X", 6);
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert_cmpfloat (v_double, ==, 12.5);
	g_assert (error == NULL);

	sub_expression = arv_evaluator_get_sub_expression (evaluator, "SUB_EXP");
	g_assert_cmpstr (sub_expression, ==, "2*X");

	arv_evaluator_set_sub_expression (evaluator, "SUB_EXP", "2 + X");
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert_cmpfloat (v_double, ==, 8.5);
	g_assert (error == NULL);

	arv_evaluator_set_sub_expression (evaluator, "SUB_EXP", NULL);
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	sub_expression = arv_evaluator_get_sub_expression (evaluator, "SUB_EXP");
	g_assert (sub_expression == NULL);

	arv_evaluator_set_sub_expression (evaluator, "SUB_EXP", "SUB_EXP");
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	g_object_unref (evaluator);
}

static void
constant_test (void)
{
	ArvEvaluator *evaluator;
	GError *error = NULL;
	double v_double;
	const char *constant;

	evaluator = arv_evaluator_new ("ZERO5 + TEN * X");

	arv_evaluator_set_constant (evaluator, "ZERO5", "0.5");
	arv_evaluator_set_constant (evaluator, "TEN", "10");
	arv_evaluator_set_int64_variable (evaluator, "X", 6);
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert_cmpfloat (v_double, ==, 60.5);
	g_assert (error == NULL);

	constant = arv_evaluator_get_constant (evaluator, "TEN");
	g_assert_cmpstr (constant, ==, "10");

	arv_evaluator_set_constant (evaluator, "TEN", "20");
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert_cmpfloat (v_double, ==, 120.5);
	g_assert (error == NULL);

	arv_evaluator_set_constant (evaluator, "TEN", NULL);
	v_double = arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	constant = arv_evaluator_get_constant (evaluator, "TEN");
	g_assert (constant == NULL);

	g_object_unref (evaluator);
}

static void
empty_test (void)
{
	ArvEvaluator *evaluator;
	GError *error = NULL;
	gint64 v_int64;

	evaluator = arv_evaluator_new (NULL);
	arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	arv_evaluator_set_expression (evaluator, "10 * 3");
	v_int64 = arv_evaluator_evaluate_as_int64 (evaluator, &error);
	g_assert_cmpint (v_int64, ==, 30);
	g_assert (error == NULL);

	arv_evaluator_set_expression (evaluator, NULL);
	arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	g_object_unref (evaluator);
}

static void
error_test (void)
{
	ArvEvaluator *evaluator;
	GError *error = NULL;

	evaluator = arv_evaluator_new ("(");

	arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	arv_evaluator_evaluate_as_int64 (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	g_object_unref (evaluator);

	evaluator = arv_evaluator_new ("UNKNOWN(");

	arv_evaluator_evaluate_as_double (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	arv_evaluator_evaluate_as_int64 (evaluator, &error);
	g_assert (error != NULL);
	g_clear_error (&error);

	g_object_unref (evaluator);
}

int
main (int argc, char *argv[])
{
	int result;
	int i;

	g_test_init (&argc, &argv, NULL);

	for (i = 0; i < G_N_ELEMENTS (expression_test_data); i++)
		g_test_add_data_func (expression_test_data[i].test_name,
				      &expression_test_data[i],
				      (void *) expression_test);

	g_test_add_func ("/evaluator/set-get-expression", set_get_expression_test);
	g_test_add_func ("/evaluator/double-variable", set_double_variable_test);
	g_test_add_func ("/evaluator/int64-variable", set_int64_variable_test);
	g_test_add_func ("/evaluator/sub-expression", sub_expression_test);
	g_test_add_func ("/evaluator/constant", constant_test);
	g_test_add_func ("/evaluator/empty", empty_test);
	g_test_add_func ("/evaluator/error", error_test);

	result = g_test_run();

	arv_shutdown ();

	return result;
}
