/* Aravis - Digital camera library
 *
 * Copyright © 2009-2010 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

/**
 * SECTION: arvevaluator
 * @short_description: A math expression evaluator with Genicam syntax
 */

#include <arvevaluator.h>
#include <arvdebug.h>
#include <arvmisc.h>
#include <arvstr.h>
#include <math.h>
#include <stdlib.h>

#define ARV_EVALUATOR_STACK_SIZE	128

static GObjectClass *parent_class = NULL;

typedef enum {
	ARV_EVALUATOR_STATUS_SUCCESS,
	ARV_EVALUATOR_STATUS_NOT_PARSED,
	ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION,
	ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH,
	ARV_EVALUATOR_STATUS_SYNTAX_ERROR,
	ARV_EVALUATOR_STATUS_UNKNOWN_OPERATOR,
	ARV_EVALUATOR_STATUS_UNKNOWN_VARIABLE,
	ARV_EVALUATOR_STATUS_UNKNOWN_SUB_EXPRESSION,
	ARV_EVALUATOR_STATUS_UNKNOWN_CONSTANT,
	ARV_EVALUATOR_STATUS_MISSING_ARGUMENTS,
	ARV_EVALUATOR_STATUS_REMAINING_OPERANDS,
	ARV_EVALUATOR_STATUS_DIVISION_BY_ZERO,
	ARV_EVALUATOR_STATUS_STACK_OVERFLOW,
	ARV_EVALUATOR_STATUS_FORBIDDEN_RECUSRION
} ArvEvaluatorStatus;

static const char *arv_evaluator_status_strings[] = {
	"success",
	"not parsed",
	"empty expression",
	"parentheses mismatch",
	"syntax error",
	"unknown operator",
	"unknown variable",
	"unknown subexpression",
	"unknown constant",
	"missing arguments",
	"remaining operands",
	"division by zero",
	"stack overflow",
	"forbidden recursion"
};

struct _ArvEvaluatorPrivate {
	char *expression;
	GSList *rpn_stack;
	ArvEvaluatorStatus parsing_status;
	GHashTable *variables;
	GHashTable *sub_expressions;
	GHashTable *constants;
};

typedef enum {
	ARV_EVALUATOR_TOKEN_UNKNOWN,
	ARV_EVALUATOR_TOKEN_COMMA,
	ARV_EVALUATOR_TOKEN_TERNARY_QUESTION_MARK,
	ARV_EVALUATOR_TOKEN_TERNARY_COLON,
	ARV_EVALUATOR_TOKEN_LOGICAL_OR,
	ARV_EVALUATOR_TOKEN_LOGICAL_AND,
	ARV_EVALUATOR_TOKEN_BITWISE_NOT,
	ARV_EVALUATOR_TOKEN_BITWISE_OR,
	ARV_EVALUATOR_TOKEN_BITWISE_XOR,
	ARV_EVALUATOR_TOKEN_BITWISE_AND,
	ARV_EVALUATOR_TOKEN_EQUAL,
	ARV_EVALUATOR_TOKEN_NOT_EQUAL,
	ARV_EVALUATOR_TOKEN_LESS_OR_EQUAL,
	ARV_EVALUATOR_TOKEN_GREATER_OR_EQUAL,
	ARV_EVALUATOR_TOKEN_LESS,
	ARV_EVALUATOR_TOKEN_GREATER,
	ARV_EVALUATOR_TOKEN_SHIFT_RIGHT,
	ARV_EVALUATOR_TOKEN_SHIFT_LEFT,
	ARV_EVALUATOR_TOKEN_SUBSTRACTION,
	ARV_EVALUATOR_TOKEN_ADDITION,
	ARV_EVALUATOR_TOKEN_REMAINDER,
	ARV_EVALUATOR_TOKEN_DIVISION,
	ARV_EVALUATOR_TOKEN_MULTIPLICATION,
	ARV_EVALUATOR_TOKEN_POWER,
	ARV_EVALUATOR_TOKEN_MINUS,
	ARV_EVALUATOR_TOKEN_PLUS,
	ARV_EVALUATOR_TOKEN_FUNCTION_SIN,
	ARV_EVALUATOR_TOKEN_FUNCTION_COS,
	ARV_EVALUATOR_TOKEN_FUNCTION_SGN,
	ARV_EVALUATOR_TOKEN_FUNCTION_NEG,
	ARV_EVALUATOR_TOKEN_FUNCTION_ATAN,
	ARV_EVALUATOR_TOKEN_FUNCTION_TAN,
	ARV_EVALUATOR_TOKEN_FUNCTION_ABS,
	ARV_EVALUATOR_TOKEN_FUNCTION_EXP,
	ARV_EVALUATOR_TOKEN_FUNCTION_LN,
	ARV_EVALUATOR_TOKEN_FUNCTION_LG,
	ARV_EVALUATOR_TOKEN_FUNCTION_SQRT,
	ARV_EVALUATOR_TOKEN_FUNCTION_TRUNC,
	ARV_EVALUATOR_TOKEN_FUNCTION_FLOOR,
	ARV_EVALUATOR_TOKEN_FUNCTION_CEIL,
	ARV_EVALUATOR_TOKEN_FUNCTION_ASIN,
	ARV_EVALUATOR_TOKEN_FUNCTION_ACOS,
	ARV_EVALUATOR_TOKEN_RIGHT_PARENTHESIS,
	ARV_EVALUATOR_TOKEN_LEFT_PARENTHESIS,
	ARV_EVALUATOR_TOKEN_CONSTANT_INT64,
	ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE,
	ARV_EVALUATOR_TOKEN_VARIABLE
} ArvEvaluatorTokenId;

typedef enum {
	ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT,
	ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT
} ArvEvaluatorTokenAssociativity;

typedef struct {
	const char *		tag;
	int			precedence;
	int			n_args;
	ArvEvaluatorTokenAssociativity	associativity;
} ArvEvaluatorTokenInfos;

static ArvEvaluatorTokenInfos arv_evaluator_token_infos[] = {
	{"",	0,	1, 0}, /* UNKNOWN */
	{",",	0, 	0, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* COMMA */
	{"?",	5,	3, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* TERNARY_QUESTION_MARK */
	{":",	5,	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* TERNARY_COLON */
	{"||",	10,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LOGICAL_OR */
	{"&&",	20,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LOGICAL_AND */
	{"~",	30,	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_NOT */
	{"|",	40,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_OR */
	{"^",	50,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_XOR */
	{"&",	60,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_AND */
	{"=",	70,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* EQUAL, */
	{"<>",	70,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* NOT_EQUAL */
	{"<=",	80,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LESS_OR_EQUAL */
	{">=",	80,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* GREATER_OR_EQUAL */
	{"<",	80,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LESS */
	{">",	80,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* GREATER */
	{">>",	90,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* SHIFT_RIGHT */
	{"<<",	90,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* SHIFT_LEFT */
	{"-",	100, 	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* SUBSTRACTION */
	{"+",	100, 	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* ADDITION */
	{"%",	110,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* REMAINDER */
	{"/",	110,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* DIVISION */
	{"*",	110, 	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* MULTIPLICATION */
	{"**",	120,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* POWER */
	{"minus",130, 	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* MINUS */
	{"plus",130, 	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* PLUS */
	{"sin",	200,	1, 0}, /* FUNCTION_SIN */
	{"cos",	200,	1, 0}, /* FUNCTION_COS */
	{"sgn",	200,	1, 0}, /* FUNCTION_SGN */
	{"neg",	200,	1, 0}, /* FUNCTION_NEG */
	{"atan",200,	1, 0}, /* FUNCTION_ATAN */
	{"tan" ,200,	1, 0}, /* FUNCTION_TAN */
	{"abs" ,200,	1, 0}, /* FUNCTION_ABS */
	{"exp" ,200,	1, 0}, /* FUNCTION_EXP */
	{"ln",  200,	1, 0}, /* FUNCTION_LN */
	{"lg",  200,	1, 0}, /* FUNCTION_LG */
	{"sqrt",200,	1, 0}, /* FUNCTION_SQRT */
	{"trunc",200,	1, 0}, /* FUNCTION_TRUNC */
	{"floor",200,	1, 0}, /* FUNCTION_FLOOR */
	{"ceil",200, 	1, 0}, /* FUNCTION_CEIL */
	{"asin",200, 	1, 0}, /* FUNCTION_ASIN */
	{"acos",200, 	1, 0}, /* FUNCTION_ACOS */
	{")",	990, 	0, 0}, /* RIGHT_PARENTHESIS */
	{"(",	-1, 	0, 0}, /* LEFT_PARENTHESIS */
	{"int64" ,200,	0, 0}, /* CONSTANT_INT64 */
	{"double",200,	0, 0}, /* CONSTANT_DOUBLE */
	{"var",	200,	0, 0}, /* VARIABLE */
};

typedef struct {
	ArvEvaluatorTokenId	token_id;
	union {
		double		v_double;
		gint64		v_int64;
		char * 		name;
	} data;
} ArvEvaluatorToken;

static ArvEvaluatorToken *
arv_evaluator_token_new (ArvEvaluatorTokenId token_id)
{
	ArvEvaluatorToken *token = g_new0 (ArvEvaluatorToken, 1);
	token->token_id = token_id;

	return token;
}

static ArvEvaluatorToken *
arv_evaluator_token_new_double (double v_double)
{
	ArvEvaluatorToken *token = arv_evaluator_token_new (ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE);
	token->data.v_double = v_double;

	return token;
}

static ArvEvaluatorToken *
arv_evaluator_token_new_int64 (double v_int64)
{
	ArvEvaluatorToken *token = arv_evaluator_token_new (ARV_EVALUATOR_TOKEN_CONSTANT_INT64);
	token->data.v_int64 = v_int64;

	return token;
}

static ArvEvaluatorToken *
arv_evaluator_token_new_variable (const char *name)
{
	ArvEvaluatorToken *token = arv_evaluator_token_new (ARV_EVALUATOR_TOKEN_VARIABLE);
	token->data.name = g_strdup (name);

	return token;
}

static void
arv_evaluator_token_free (ArvEvaluatorToken *token)
{
	if (token == NULL)
		return;

	if (token->token_id == ARV_EVALUATOR_TOKEN_VARIABLE)
		g_free (token->data.name);
	g_free (token);
}

void
arv_evaluator_token_debug (ArvEvaluatorToken *token, GHashTable *variables)
{
	ArvValue *value;

	g_return_if_fail (token != NULL);

	switch (token->token_id) {
		case ARV_EVALUATOR_TOKEN_VARIABLE:
			value = g_hash_table_lookup (variables, token->data.name);
			arv_log_evaluator ("(var) %s = %g%s", token->data.name,
					   value != NULL ? arv_value_get_double (value) : 0,
					   value != NULL ? "" : " not found");
			break;
		case ARV_EVALUATOR_TOKEN_CONSTANT_INT64:
			arv_log_evaluator ("(int64) %Ld", token->data.v_int64);
			break;
		case ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE:
			arv_log_evaluator ("(double) %g", token->data.v_double);
			break;
		default:
			arv_log_evaluator ("(operator) %s", arv_evaluator_token_infos[token->token_id].tag);
	}
}

static gboolean
arv_evaluator_token_is_variable (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EVALUATOR_TOKEN_VARIABLE);
}

static gboolean
arv_evaluator_token_is_operand (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id > ARV_EVALUATOR_TOKEN_LEFT_PARENTHESIS);
}

static gboolean
arv_evaluator_token_is_operator (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id > ARV_EVALUATOR_TOKEN_UNKNOWN &&
		token->token_id < ARV_EVALUATOR_TOKEN_RIGHT_PARENTHESIS);
}

static gboolean
arv_evaluator_token_is_comma (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EVALUATOR_TOKEN_COMMA);
}

static gboolean
arv_evaluator_token_is_left_parenthesis (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EVALUATOR_TOKEN_LEFT_PARENTHESIS);
}

static gboolean
arv_evaluator_token_is_right_parenthesis (ArvEvaluatorToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EVALUATOR_TOKEN_RIGHT_PARENTHESIS);
}

gboolean
arv_evaluator_token_compare_precedence (ArvEvaluatorToken *a, ArvEvaluatorToken *b)
{
	gint a_precedence;
	gint b_precedence;
	ArvEvaluatorTokenAssociativity a_associativity;

	if (a == NULL || b == NULL ||
	    a->token_id >= G_N_ELEMENTS (arv_evaluator_token_infos) ||
	    b->token_id >= G_N_ELEMENTS (arv_evaluator_token_infos))
		return FALSE;

	a_precedence = arv_evaluator_token_infos[a->token_id].precedence;
	b_precedence = arv_evaluator_token_infos[b->token_id].precedence;
	a_associativity = arv_evaluator_token_infos[a->token_id].associativity;

	return (((a_precedence <= b_precedence) &&
		 a_associativity == ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT) ||
		((a_precedence < b_precedence) &&
		 a_associativity == ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT));
}

ArvEvaluatorToken *
arv_get_next_token (char **expression, ArvEvaluatorToken *previous_token)
{
	ArvEvaluatorToken *token = NULL;
	ArvEvaluatorTokenId token_id = ARV_EVALUATOR_TOKEN_UNKNOWN;

	g_return_val_if_fail (expression != NULL && *expression != NULL, NULL);
	arv_str_skip_spaces (expression);

	if (**expression == '\0')
		return NULL;

	if (g_ascii_isdigit (**expression)) {
		char *end;
		gint64 v_int64;
		double v_double;
		ptrdiff_t length_int64;
		ptrdiff_t length_double;

		v_int64 = g_ascii_strtoll (*expression, &end, 0);
		length_int64 = end - *expression;

		end = *expression;
		arv_str_parse_double (&end, &v_double);
		length_double = end - *expression;

		if (length_double > 0 || length_int64 > 0) {
			if (length_double > length_int64) {
				token = arv_evaluator_token_new_double (v_double);
				*expression += length_double;
			} else {
				token = arv_evaluator_token_new_int64 (v_int64);
				*expression += length_int64;
			}
		}
	} else if (g_ascii_isalpha (**expression) || **expression=='_') {
		char *end = *expression;
		ptrdiff_t token_length;

		while (g_ascii_isalnum (*end) || *end == '_')
			end++;

		token_length = end - *expression;

		if (token_length == 2) {
			if (g_ascii_strncasecmp ("ln", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_LN;
			else if (g_ascii_strncasecmp ("lg", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_LG;
		} else if (token_length == 3) {
			if (g_ascii_strncasecmp ("sin", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_SIN;
			else if (g_ascii_strncasecmp ("cos", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_COS;
			else if (g_ascii_strncasecmp ("sgn", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_SGN;
			else if (g_ascii_strncasecmp ("neg", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_NEG;
			else if (g_ascii_strncasecmp ("tan", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_TAN;
			else if (g_ascii_strncasecmp ("abs", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_ABS;
			else if (g_ascii_strncasecmp ("exp", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_EXP;
		} else if (token_length == 4) {
			if (g_ascii_strncasecmp ("atan", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_ATAN;
			else if (g_ascii_strncasecmp ("sqrt", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_SQRT;
			else if (g_ascii_strncasecmp ("ceil", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_CEIL;
			else if (g_ascii_strncasecmp ("asin", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_ASIN;
			else if (g_ascii_strncasecmp ("acos", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_ACOS;
		} else if (token_length == 5) {
			if (g_ascii_strncasecmp ("trunc", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_TRUNC;
			else if (g_ascii_strncasecmp ("floor", *expression, token_length) == 0)
				token_id = ARV_EVALUATOR_TOKEN_FUNCTION_FLOOR;
		}

		if (token_id != ARV_EVALUATOR_TOKEN_UNKNOWN)
			token = arv_evaluator_token_new (token_id);
		else {
			char *name = g_strndup (*expression, token_length);
			token = arv_evaluator_token_new_variable (name);
			g_free (name);
		}

		*expression = end;
	} else {
		switch (**expression) {
			case '(': token_id = ARV_EVALUATOR_TOKEN_LEFT_PARENTHESIS; break;
			case ')': token_id = ARV_EVALUATOR_TOKEN_RIGHT_PARENTHESIS; break;
			case ',': token_id = ARV_EVALUATOR_TOKEN_COMMA; break;
			case '?': token_id = ARV_EVALUATOR_TOKEN_TERNARY_QUESTION_MARK; break;
			case ':': token_id = ARV_EVALUATOR_TOKEN_TERNARY_COLON; break;
			case '+': if (previous_token != NULL &&
				      (arv_evaluator_token_is_operand (previous_token) ||
				       arv_evaluator_token_is_right_parenthesis (previous_token)))
					  token_id = ARV_EVALUATOR_TOKEN_ADDITION;
				  else
					  token_id = ARV_EVALUATOR_TOKEN_PLUS;
				  break;
			case '-': if (previous_token != NULL &&
				      (arv_evaluator_token_is_operand (previous_token) ||
				       arv_evaluator_token_is_right_parenthesis (previous_token)))
					  token_id = ARV_EVALUATOR_TOKEN_SUBSTRACTION;
				  else
					  token_id = ARV_EVALUATOR_TOKEN_MINUS;
				  break;
			case '*': if ((*expression)[1] == '*') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_POWER;
				  } else
					  token_id = ARV_EVALUATOR_TOKEN_MULTIPLICATION;
				  break;
			case '/': token_id = ARV_EVALUATOR_TOKEN_DIVISION; break;
			case '%': token_id = ARV_EVALUATOR_TOKEN_REMAINDER; break;
			case '&': if ((*expression)[1] == '&') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_LOGICAL_AND;
				  } else
					  token_id = ARV_EVALUATOR_TOKEN_BITWISE_AND;
				  break;
			case '|': if ((*expression)[1] == '|') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_LOGICAL_OR;
				  } else
					  token_id = ARV_EVALUATOR_TOKEN_BITWISE_OR;
				  break;
			case '^': token_id = ARV_EVALUATOR_TOKEN_BITWISE_XOR; break;
			case '~': token_id = ARV_EVALUATOR_TOKEN_BITWISE_NOT; break;
			case '<': if ((*expression)[1] == '>') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_NOT_EQUAL;
				  } else if ((*expression)[1] == '<') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_SHIFT_LEFT;
				  } else if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_LESS_OR_EQUAL;
				  } else
					  token_id = ARV_EVALUATOR_TOKEN_LESS;
				  break;
			case '>': if ((*expression)[1] == '>') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_SHIFT_RIGHT;
				  } else if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_GREATER_OR_EQUAL;
				  } else
					  token_id = ARV_EVALUATOR_TOKEN_GREATER;
				  break;
			case '=': token_id = ARV_EVALUATOR_TOKEN_EQUAL; break;
		}

		if (token_id != ARV_EVALUATOR_TOKEN_UNKNOWN) {
			(*expression)++;
			token = arv_evaluator_token_new (token_id);
		}
	}

	return token;
}

ArvEvaluatorStatus
evaluate (GSList *token_stack, GHashTable *variables, gint64 *v_int64, double *v_double)
{
	ArvEvaluatorToken *token;
	ArvEvaluatorStatus status;
	GSList *iter;
	ArvValue stack[ARV_EVALUATOR_STACK_SIZE];
	ArvValue *value;
	int index = -1;

	for (iter = token_stack; iter != NULL; iter = iter->next) {
		token = iter->data;

		if (index < (arv_evaluator_token_infos[token->token_id].n_args - 1)) {
			status = ARV_EVALUATOR_STATUS_MISSING_ARGUMENTS;
			goto CLEANUP;
		}

		if (index >= ARV_EVALUATOR_STACK_SIZE - 1) {
			status = ARV_EVALUATOR_STATUS_STACK_OVERFLOW;
			goto CLEANUP;
		}

		arv_evaluator_token_debug (token, variables);

		switch (token->token_id) {
			case ARV_EVALUATOR_TOKEN_LOGICAL_AND:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) &&
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_LOGICAL_OR:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) ||
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_BITWISE_NOT:
				arv_value_set_int64 (&stack[index],
						      ~arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_BITWISE_AND:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) &
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_BITWISE_OR:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) |
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_BITWISE_XOR:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) ^
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_EQUAL:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) ==
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) ==
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_NOT_EQUAL:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) !=
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) !=
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_LESS_OR_EQUAL:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) <=
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) <=
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_GREATER_OR_EQUAL:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) >=
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) >=
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_LESS:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) <
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) <
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_GREATER:
				if (arv_value_holds_int64 (&stack[index-1]) &&
				    arv_value_holds_int64 (&stack[index]))
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) >
							      arv_value_get_int64 (&stack[index]));
				else
					arv_value_set_int64 (&stack[index - 1],
							      arv_value_get_double (&stack[index-1]) >
							      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_SHIFT_RIGHT:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) >>
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_SHIFT_LEFT:
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) <<
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_SUBSTRACTION:
				if (arv_value_holds_double (&stack[index-1]) ||
				    arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index-1],
							       arv_value_get_double (&stack[index-1]) -
							       arv_value_get_double (&stack[index]));
				else
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) -
							      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_ADDITION:
				if (arv_value_holds_double (&stack[index-1]) ||
				    arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index-1],
							       arv_value_get_double (&stack[index-1]) +
							       arv_value_get_double (&stack[index]));
				else
					arv_value_set_int64 (&stack[index-1],
							      arv_value_get_int64 (&stack[index-1]) +
							      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_REMAINDER:
				if (arv_value_get_int64 (&stack[index]) == 0) {
					status = ARV_EVALUATOR_STATUS_DIVISION_BY_ZERO;
					goto CLEANUP;
				}
				arv_value_set_int64 (&stack[index-1],
						      arv_value_get_int64 (&stack[index-1]) %
						      arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_DIVISION:
				if (arv_value_get_double (&stack[index]) == 0.0) {
					status = ARV_EVALUATOR_STATUS_DIVISION_BY_ZERO;
					goto CLEANUP;
				}
				arv_value_set_double (&stack[index-1],
						      arv_value_get_double (&stack[index-1]) /
						      arv_value_get_double (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_MULTIPLICATION:
				if (arv_value_holds_double (&stack[index-1]) ||
				    arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index-1],
							      arv_value_get_double (&stack[index-1]) *
							      arv_value_get_double (&stack[index]));
				else
					arv_value_set_int64 (&stack[index-1],
							     arv_value_get_int64 (&stack[index-1]) *
							     arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_POWER:
				arv_value_set_double (&stack[index-1],
						      pow (arv_value_get_double(&stack[index-1]),
							   arv_value_get_double(&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_MINUS:
				if (arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index],
							      -arv_value_get_double (&stack[index]));
				else
					arv_value_set_int64 (&stack[index],
							     -arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_PLUS:
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_SIN:
				arv_value_set_double (&stack[index], sin (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_COS:
				arv_value_set_double (&stack[index], cos (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_SGN:
				if (arv_value_holds_double (&stack[index])) {
					double value = arv_value_get_double (&stack[index]);
					if (value < 0.0)
						arv_value_set_int64 (&stack[index], -1);
					else if (value > 0.0)
						arv_value_set_int64 (&stack[index], 1);
					else
						arv_value_set_int64 (&stack[index], 0);
				} else {
					gint64 value = arv_value_get_int64 (&stack[index]);
					if (value < 0)
						arv_value_set_int64 (&stack[index], -1);
					else if (value > 0)
						arv_value_set_int64 (&stack[index], 1);
					else
						arv_value_set_int64 (&stack[index], 0);
				}
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_NEG:
				if (arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index],
							      -arv_value_get_double (&stack[index-1]));
				else
					arv_value_set_int64 (&stack[index],
							     -arv_value_get_int64 (&stack[index]));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_ATAN:
				arv_value_set_double (&stack[index], atan (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_TAN:
				arv_value_set_double (&stack[index], tan (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_ABS:
				if (arv_value_holds_double (&stack[index]))
					arv_value_set_double (&stack[index],
							      fabs (arv_value_get_double (&stack[index-1])));
				else
					arv_value_set_int64 (&stack[index],
							     abs (arv_value_get_int64 (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_EXP:
				arv_value_set_double (&stack[index], exp (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_LN:
				arv_value_set_double (&stack[index], log (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_LG:
				arv_value_set_double (&stack[index], log10 (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_SQRT:
				arv_value_set_double (&stack[index], sqrt (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_TRUNC:
				if (arv_value_get_double (&stack[index]) > 0.0)
					arv_value_set_double (&stack[index],
							      floor (arv_value_get_double (&stack[index])));
				else
					arv_value_set_double (&stack[index],
							      ceil (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_FLOOR:
				arv_value_set_double (&stack[index], floor (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_CEIL:
				arv_value_set_double (&stack[index], ceil (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_ASIN:
				arv_value_set_double (&stack[index], asin (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_ACOS:
				arv_value_set_double (&stack[index], acos (arv_value_get_double (&stack[index])));
				break;
			case ARV_EVALUATOR_TOKEN_CONSTANT_INT64:
				arv_value_set_int64 (&stack[index+1], token->data.v_int64);
				break;
			case ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE:
				arv_value_set_double (&stack[index+1], token->data.v_double);
				break;
			case ARV_EVALUATOR_TOKEN_VARIABLE:
				value = g_hash_table_lookup (variables, token->data.name);
				if (value != NULL) {
					arv_value_copy (&stack[index+1], value);
				} else {
					status = ARV_EVALUATOR_STATUS_UNKNOWN_VARIABLE;
					goto CLEANUP;
				}
				break;
			case ARV_EVALUATOR_TOKEN_TERNARY_COLON:
				break;
			case ARV_EVALUATOR_TOKEN_TERNARY_QUESTION_MARK:
				if (arv_value_get_int64 (&stack[index-2]) != 0)
					arv_value_copy (&stack[index-2], &stack[index-1]);
				else
					arv_value_copy (&stack[index-2], &stack[index]);
				break;
			default:
				status = ARV_EVALUATOR_STATUS_UNKNOWN_OPERATOR;
				goto CLEANUP;
				break;
		}

		index = index - arv_evaluator_token_infos[token->token_id].n_args + 1;
	}

	if (index != 0) {
		status = ARV_EVALUATOR_STATUS_REMAINING_OPERANDS;
		goto CLEANUP;
	}

	if (v_double != NULL)
		*v_double = arv_value_get_double (stack);

	if (v_int64 != NULL)
		*v_int64 = arv_value_get_int64 (stack);

	if (arv_value_holds_int64 (stack))
		arv_log_evaluator ("[Evaluator::evaluate] Result = (int64) %Ld", arv_value_get_int64 (stack));
	else
		arv_log_evaluator ("[Evaluator::evaluate] Result = (double) %g", arv_value_get_double (stack));

	return ARV_EVALUATOR_STATUS_SUCCESS;
CLEANUP:
	if (v_double != NULL)
		*v_double = 0.0;

	if (v_int64 != NULL)
		*v_int64 = 0.0;

	return status;
}

typedef struct {
	int count;
	ArvEvaluatorToken *previous_token;
	GSList *token_stack;
	GSList *operator_stack;
	GSList *garbage_stack;
	gboolean in_sub_expression;
} ArvEvaluatorParserState;

static ArvEvaluatorStatus
parse_to_stacks (ArvEvaluator *evaluator, char *expression, ArvEvaluatorParserState *state)
{
	ArvEvaluatorToken *token;
	ArvEvaluatorStatus status;

	if (expression == NULL)
		return ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION;

	/* Dijkstra's "shunting yard" algorithm */
	/* http://en.wikipedia.org/wiki/Shunting-yard_algorithm */

	do {
		token = arv_get_next_token (&expression, state->previous_token);
		if (token != NULL) {
			if (arv_evaluator_token_is_variable (token)) {
				if (g_hash_table_lookup_extended (evaluator->priv->constants, token->data.name, NULL, NULL)) {
					const char *constant;

					constant = g_hash_table_lookup (evaluator->priv->constants, token->data.name);

					if (constant != NULL) {
						arv_evaluator_token_free (token);
						token = arv_get_next_token ((char **) &constant, NULL);
						if (token != NULL)
							state->token_stack = g_slist_prepend (state->token_stack, token);
					} else {
						status = ARV_EVALUATOR_STATUS_UNKNOWN_CONSTANT;
						goto CLEANUP;
					}
				} else if (g_hash_table_lookup_extended (evaluator->priv->sub_expressions, token->data.name, NULL, NULL)) {
					const char *sub_expression;

					sub_expression = g_hash_table_lookup (evaluator->priv->sub_expressions, token->data.name);

					if (sub_expression != NULL) {
						char *string;

						if (state->in_sub_expression) {
							status = ARV_EVALUATOR_STATUS_FORBIDDEN_RECUSRION;
							goto CLEANUP;
						}

						string = g_strdup_printf ("(%s)", sub_expression);
						state->in_sub_expression = TRUE;
						status = parse_to_stacks (evaluator, string, state);
						state->in_sub_expression = FALSE;
						g_free (string);

						if (status != ARV_EVALUATOR_STATUS_SUCCESS) {
							goto CLEANUP;
						}

						arv_evaluator_token_free (token);
					} else {
						status = ARV_EVALUATOR_STATUS_UNKNOWN_SUB_EXPRESSION;
						goto CLEANUP;
					}
				} else {
					state->token_stack = g_slist_prepend (state->token_stack, token);
				}
			} else if (arv_evaluator_token_is_operand (token)) {
				state->token_stack = g_slist_prepend (state->token_stack, token);
			} else if (arv_evaluator_token_is_comma (token)) {
				while (state->operator_stack != NULL &&
				       !arv_evaluator_token_is_left_parenthesis (state->operator_stack->data)) {
					state->token_stack = g_slist_prepend (state->token_stack, state->operator_stack->data);
					state->operator_stack = g_slist_delete_link (state->operator_stack, state->operator_stack);
				}
				if (state->operator_stack == NULL ||
				    !arv_evaluator_token_is_left_parenthesis (state->operator_stack->data)) {
					status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
					goto CLEANUP;
				}
				state->garbage_stack = g_slist_prepend (state->garbage_stack, token);
			} else if (arv_evaluator_token_is_operator (token)) {
				while (state->operator_stack != NULL &&
				       arv_evaluator_token_compare_precedence (token, state->operator_stack->data)) {
					state->token_stack = g_slist_prepend (state->token_stack, state->operator_stack->data);
					state->operator_stack = g_slist_delete_link (state->operator_stack, state->operator_stack);
				}
				state->operator_stack = g_slist_prepend (state->operator_stack, token);
			} else if (arv_evaluator_token_is_left_parenthesis (token)) {
				state->operator_stack = g_slist_prepend (state->operator_stack, token);
			} else if (arv_evaluator_token_is_right_parenthesis (token)) {
				while (state->operator_stack != NULL &&
				       !arv_evaluator_token_is_left_parenthesis (state->operator_stack->data)) {
					state->token_stack = g_slist_prepend (state->token_stack, state->operator_stack->data);
					state->operator_stack = g_slist_delete_link (state->operator_stack, state->operator_stack);
				}
				if (state->operator_stack == NULL) {
					status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
					goto CLEANUP;
				}
				state->garbage_stack = g_slist_prepend (state->garbage_stack, token);
				state->garbage_stack = g_slist_prepend (state->garbage_stack, state->operator_stack->data);
				state->operator_stack = g_slist_delete_link (state->operator_stack, state->operator_stack);
			} else {
				status = ARV_EVALUATOR_STATUS_SYNTAX_ERROR;
				goto CLEANUP;
			}
			(state->count)++;
		} else if (*expression != '\0') {
			status = ARV_EVALUATOR_STATUS_SYNTAX_ERROR;
			goto CLEANUP;
		}
		state->previous_token = token;
	} while (token != NULL);

	return ARV_EVALUATOR_STATUS_SUCCESS;

CLEANUP:
	if (token != NULL)
		arv_evaluator_token_free (token);

	return status;
}

static void
free_rpn_stack (ArvEvaluator *evaluator)
{
	GSList *iter;

	for (iter = evaluator->priv->rpn_stack; iter != NULL; iter = iter->next)
		arv_evaluator_token_free (iter->data);
	g_slist_free (evaluator->priv->rpn_stack);
	evaluator->priv->rpn_stack = NULL;
}

static ArvEvaluatorStatus
parse_expression (ArvEvaluator *evaluator)
{
	ArvEvaluatorParserState state;
	ArvEvaluatorStatus status;
	GSList *iter;
	int count;

	state.count  =0;
	state.previous_token = NULL;
	state.token_stack = NULL;
	state.operator_stack = NULL;
	state.garbage_stack = NULL;
	state.in_sub_expression = FALSE;

	free_rpn_stack (evaluator);

	arv_log_evaluator ("[Evaluator::parse_expression] %s", evaluator->priv->expression);

	status = parse_to_stacks (evaluator, evaluator->priv->expression, &state);

	if (status != ARV_EVALUATOR_STATUS_SUCCESS)
		goto CLEANUP;

	arv_log_evaluator ("[Evaluator::parse_expression] Found %d items in expression", state.count);

	while (state.operator_stack != NULL) {
		if (arv_evaluator_token_is_left_parenthesis (state.operator_stack->data)) {
			status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
			goto CLEANUP;
		}

		state.token_stack = g_slist_prepend (state.token_stack, state.operator_stack->data);
		state.operator_stack = g_slist_delete_link (state.operator_stack, state.operator_stack);
	}

	evaluator->priv->rpn_stack = g_slist_reverse (state.token_stack);

	for (iter = state.garbage_stack, count = 0; iter != NULL; iter = iter->next, count++)
		arv_evaluator_token_free (iter->data);
	g_slist_free (state.garbage_stack);

	arv_log_evaluator ("[Evaluator::parse_expression] %d items in garbage list", count);
	arv_log_evaluator ("[Evaluator::parse_expression] %d items in token list", g_slist_length (evaluator->priv->rpn_stack));

	return evaluator->priv->rpn_stack == NULL ? ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION : ARV_EVALUATOR_STATUS_SUCCESS;

CLEANUP:

	for (iter = state.garbage_stack; iter != NULL; iter = iter->next)
		arv_evaluator_token_free (iter->data);
	g_slist_free (state.garbage_stack);
	for (iter = state.token_stack; iter != NULL; iter = iter->next)
		arv_evaluator_token_free (iter->data);
	g_slist_free (state.token_stack);
	for (iter = state.operator_stack; iter != NULL; iter = iter->next)
		arv_evaluator_token_free (iter->data);
	g_slist_free (state.operator_stack);

	return status;
}

static void
arv_evaluator_set_error (GError **error, ArvEvaluatorStatus status)
{
	g_set_error (error,
		     g_quark_from_string ("Aravis"),
		     status,
		     "Parsing error: %s",
		     arv_evaluator_status_strings [MIN (status,
							G_N_ELEMENTS (arv_evaluator_status_strings)-1)]);

	arv_warning_evaluator ("[Evaluator::set_error] Error '%s'",
			       arv_evaluator_status_strings [MIN (status,
								  G_N_ELEMENTS (arv_evaluator_status_strings)-1)]);
}

double
arv_evaluator_evaluate_as_double (ArvEvaluator *evaluator, GError **error)
{
	ArvEvaluatorStatus status;
	double value;

	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), 0.0);

	arv_log_evaluator ("[Evaluator::evaluate_as_double] Expression = '%s'",
			   evaluator->priv->expression);

	if (evaluator->priv->parsing_status == ARV_EVALUATOR_STATUS_NOT_PARSED) {
		evaluator->priv->parsing_status = parse_expression (evaluator);
		arv_log_evaluator ("[Evaluator::evaluate_as_double] Parsing status = %d",
				   evaluator->priv->parsing_status);
	}

	if (evaluator->priv->parsing_status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, evaluator->priv->parsing_status);
		return 0.0;
	}

	status = evaluate (evaluator->priv->rpn_stack, evaluator->priv->variables, NULL, &value);
	if (status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, status);
		return 0.0;
	}

	return value;
}

gint64
arv_evaluator_evaluate_as_int64 (ArvEvaluator *evaluator, GError **error)
{
	ArvEvaluatorStatus status;
	gint64 value;

	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), 0.0);

	arv_log_evaluator ("[Evaluator::evaluate_as_int64] Expression = '%s'",
			   evaluator->priv->expression);

	if (evaluator->priv->parsing_status == ARV_EVALUATOR_STATUS_NOT_PARSED) {
		evaluator->priv->parsing_status = parse_expression (evaluator);
		arv_log_evaluator ("[Evaluator::evaluate_as_int64] Parsing status = %d",
				   evaluator->priv->parsing_status);
	}

	if (evaluator->priv->parsing_status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, evaluator->priv->parsing_status);
		return 0.0;
	}

	status = evaluate (evaluator->priv->rpn_stack, evaluator->priv->variables, &value, NULL);
	if (status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, status);
		return 0.0;
	}

	return value;
}

void
arv_evaluator_set_expression (ArvEvaluator *evaluator, const char *expression)
{
	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));

	if (g_strcmp0 (expression, evaluator->priv->expression) == 0)
		return;

	g_free (evaluator->priv->expression);
	evaluator->priv->expression = NULL;

	if (expression == NULL) {
		evaluator->priv->parsing_status = ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION;
		return;
	}

	evaluator->priv->parsing_status = ARV_EVALUATOR_STATUS_NOT_PARSED;
	evaluator->priv->expression = g_strdup (expression);
}

const char *
arv_evaluator_get_expression (ArvEvaluator *evaluator)
{
	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), NULL);

	return evaluator->priv->expression;
}

/**
 * arv_evaluator_set_sub_expression:
 * @evaluator:a #ArvEvaluator
 * @name: sub-expression name
 * @expression: (allow-none): sub-pexression formula
 *
 * Assign a formula to a sub-expression. If @expression == %NULL, the sub-expression previously assigned to @name will be removed.
 * A sub-expression may not reference another sub-expression.
 *
 * Since: 0.6.0
 */

void
arv_evaluator_set_sub_expression (ArvEvaluator *evaluator, const char *name, const char *expression)
{
	const char *old_expression;

	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));

	if (name == NULL)
		return;

	old_expression = g_hash_table_lookup (evaluator->priv->sub_expressions, name);
	if (old_expression != NULL && g_strcmp0 (old_expression, expression) == 0)
		return;

	if (expression != NULL)
		g_hash_table_replace (evaluator->priv->sub_expressions, g_strdup (name), g_strdup (expression));
	else
		g_hash_table_remove (evaluator->priv->sub_expressions, name);

	evaluator->priv->parsing_status = ARV_EVALUATOR_STATUS_NOT_PARSED;

	arv_log_evaluator ("[Evaluator::set_sub_expression] %s = %s", name, expression);
}

/**
 * arv_evaluator_get_sub_expression:
 * @evaluator: a #ArvEvaluator
 * @name: sub-expression name
 *
 * Returns: The formula of the sub-expression corresponding to @name, %NULL if not defined.
 *
 * Since: 0.6.0
 */

const char *
arv_evaluator_get_sub_expression (ArvEvaluator *evaluator, const char *name)
{
	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), NULL);

	if (name == NULL)
		return NULL;

	return g_hash_table_lookup (evaluator->priv->sub_expressions, name);
}

/**
 * arv_evaluator_set_constant:
 * @evaluator:a #ArvEvaluator
 * @name: constant name
 * @constant: (allow-none): constant as a string
 *
 * Assign a string to a constant. If @constant == %NULL, the constant previously assigned to @name will be removed.
 *
 * Since: 0.6.0
 */

void
arv_evaluator_set_constant (ArvEvaluator *evaluator, const char *name, const char *constant)
{
	const char *old_constant;

	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));

	if (name == NULL)
		return;

	old_constant = g_hash_table_lookup (evaluator->priv->constants, name);
	if (old_constant != NULL && g_strcmp0 (old_constant, constant) == 0)
		return;

	if (constant != NULL)
		g_hash_table_replace (evaluator->priv->constants, g_strdup (name), g_strdup (constant));
	else
		g_hash_table_remove (evaluator->priv->constants, name);

	evaluator->priv->parsing_status = ARV_EVALUATOR_STATUS_NOT_PARSED;

	arv_log_evaluator ("[Evaluator::set_constant] %s = %s", name, constant);
}

/**
 * arv_evaluator_get_constant:
 * @evaluator: a #ArvEvaluator
 * @name: constant name
 *
 * Returns: The formula of the constant corresponding to @name, %NULL if not defined.
 *
 * Since: 0.6.0
 */

const char *
arv_evaluator_get_constant (ArvEvaluator *evaluator, const char *name)
{
	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), NULL);

	if (name == NULL)
		return NULL;

	return g_hash_table_lookup (evaluator->priv->constants, name);
}

void
arv_evaluator_set_double_variable (ArvEvaluator *evaluator, const char *name, double v_double)
{
	ArvValue *old_value;

	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));
	g_return_if_fail (name != NULL);

	old_value = g_hash_table_lookup (evaluator->priv->variables, name);
	if (old_value != NULL && (arv_value_get_double (old_value) == v_double))
		return;

	g_hash_table_insert (evaluator->priv->variables,
			     g_strdup (name),
			     arv_value_new_double (v_double));

	arv_log_evaluator ("[Evaluator::set_double_variable] %s = %g",
			   name, v_double);
}

void
arv_evaluator_set_int64_variable (ArvEvaluator *evaluator, const char *name, gint64 v_int64)
{
	ArvValue *old_value;

	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));
	g_return_if_fail (name != NULL);

	old_value = g_hash_table_lookup (evaluator->priv->variables, name);
	if (old_value != NULL && (arv_value_get_int64 (old_value) == v_int64))
		return;

	g_hash_table_insert (evaluator->priv->variables,
			     g_strdup (name),
			     arv_value_new_int64 (v_int64));

	arv_log_evaluator ("[Evaluator::set_int64_variable] %s = %Ld", name, v_int64);
}

/**
 * arv_evaluator_new:
 * @expression: (allow-none): an evaluator expression
 *
 * Creates a new #ArvEvaluator object. The syntax is described in the genicam standard specification.
 *
 * Return value: a new #ArvEvaluator object.
 */

ArvEvaluator *
arv_evaluator_new (const char *expression)
{
	ArvEvaluator *evaluator;

	evaluator = g_object_new (ARV_TYPE_EVALUATOR, NULL);

	arv_evaluator_set_expression (evaluator, expression);

	return evaluator;
}

static void
arv_evaluator_init (ArvEvaluator *evaluator)
{
	evaluator->priv = G_TYPE_INSTANCE_GET_PRIVATE (evaluator, ARV_TYPE_EVALUATOR, ArvEvaluatorPrivate);

	evaluator->priv->expression = NULL;
	evaluator->priv->rpn_stack = NULL;
	evaluator->priv->variables = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) arv_value_free);
	evaluator->priv->sub_expressions = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	evaluator->priv->constants = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	arv_evaluator_set_double_variable (evaluator, "PI", M_PI);
	arv_evaluator_set_double_variable (evaluator, "E", M_E);
}

static void
arv_evaluator_finalize (GObject *object)
{
	ArvEvaluator *evaluator = ARV_EVALUATOR (object);

	arv_evaluator_set_expression (evaluator, NULL);
	g_hash_table_unref (evaluator->priv->variables);
	g_hash_table_unref (evaluator->priv->sub_expressions);
	g_hash_table_unref (evaluator->priv->constants);
	free_rpn_stack (evaluator);

	parent_class->finalize (object);
}

static void
arv_evaluator_class_init (ArvEvaluatorClass *evaluator_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (evaluator_class);

	g_type_class_add_private (evaluator_class, sizeof (ArvEvaluatorPrivate));

	parent_class = g_type_class_peek_parent (evaluator_class);

	object_class->finalize = arv_evaluator_finalize;
}

G_DEFINE_TYPE (ArvEvaluator, arv_evaluator, G_TYPE_OBJECT)
