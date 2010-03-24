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
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel@gnome.org>
 */

#include <arvevaluator.h>
#include <arvtools.h>
#include <arvdebug.h>
#include <math.h>

#define ARV_EVALUATOR_STACK_SIZE	128

static GObjectClass *parent_class = NULL;

typedef enum {
	ARV_EVALUATOR_STATUS_SUCCESS,
	ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION,
	ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH,
	ARV_EVALUATOR_STATUS_SYNTAX_ERROR,
	ARV_EVALUATOR_STATUS_UNKNOWN_OPERATOR,
	ARV_EVALUATOR_STATUS_UNKNOWN_VARIABLE,
	ARV_EVALUATOR_STATUS_MISSING_ARGUMENTS,
	ARV_EVALUATOR_STATUS_REMAINING_OPERANDS
} ArvEvaluatorStatus;

static const char *arv_evaluator_status_strings[] = {
	"success",
	"empty expression",
	"parentheses mismatch",
	"syntax error",
	"unknown operator",
	"unknown variable",
	"missing arguments",
	"remaining operands"
};

struct _ArvEvaluatorPrivate {
	char *expression;
	GSList *rpn_stack;
	ArvEvaluatorStatus parsing_status;
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
#if 0
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
	ARV_EVALUATOR_TOKEN_FUNCTION_E,
	ARV_EVALUATOR_TOKEN_FUNCTION_PI
#endif
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
	{":",	5,	0, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* TERNARY_COLON */
	{"||",	10,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LOGICAL_OR */
	{"&&",	20,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* LOGICAL_AND */
	{"~",	30,	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_NOT */
	{"|",	40,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_OR */
	{"^",	50,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_XOR */
	{"&",	60,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* BITWISE_AND */
	{"==",	70,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* EQUAL, */
	{"!=",	70,	2, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT}, /* NOT_EQUAL */
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
	{"min",	130, 	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* MINUS */
	{"pls",	130, 	1, ARV_EVALUATOR_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT}, /* PLUS */
	{"sin",	200,	1, 0}, /* FUNCTION_SIN */
	{"cos",	200,	1, 0}, /* FUNCTION_COS */
#if 0
	FUNCTION_SGN,
	FUNCTION_NEG,
	FUNCTION_ATAN,
	FUNCTION_TAN,
	FUNCTION_ABS,
	FUNCTION_EXP,
	FUNCTION_LN,
	FUNCTION_LG,
	FUNCTION_SQRT,
	FUNCTION_TRUNC,
	FUNCTION_FLOOR,
	FUNCTION_CEIL,
	FUNCTION_ASIN,
	FUNCTION_ACOS,
	FUNCTION_E,
	FUNCTION_PI
#endif
	{")",	990, 	0, 0}, /* RIGHT_PARENTHESIS */
	{"(",	-1, 	0, 0}, /* LEFT_PARENTHESIS */
	{"int64" ,200,	0, 0}, /* CONSTANT_INT64 */
	{"double",200,	0, 0}, /* CONSTANT_DOUBLE */
	{"var",	200,	0, 0}, /* VARIABLE */
};

typedef struct {
	ArvEvaluatorTokenId	token_id;
	union {
		double		double_value;
		gint64		int64_value;
		char * 		string_value;
	} value;
} ArvEvaluatorToken;

void
arv_evaluator_token_debug (ArvEvaluatorToken *token)
{
	g_return_if_fail (token != NULL);

	switch (token->token_id) {
		case ARV_EVALUATOR_TOKEN_VARIABLE:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "%s",
				   token->value.string_value);
			break;
		case ARV_EVALUATOR_TOKEN_CONSTANT_INT64:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "(int64) %Ld",
				   token->value.int64_value);
			break;
		case ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "(double) %g",
				   token->value.double_value);
			break;
		default:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "%s", arv_evaluator_token_infos[token->token_id]);
	}
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
	ArvEvaluatorTokenAssociativity b_associativity;
	
	if (a == NULL || b == NULL ||
	    a->token_id >= G_N_ELEMENTS (arv_evaluator_token_infos) ||
	    b->token_id >= G_N_ELEMENTS (arv_evaluator_token_infos))
		return FALSE;

	a_precedence = arv_evaluator_token_infos[a->token_id].precedence;
	b_precedence = arv_evaluator_token_infos[b->token_id].precedence;
	a_associativity = arv_evaluator_token_infos[a->token_id].associativity;
	b_associativity = arv_evaluator_token_infos[b->token_id].associativity;

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
		gint64 value_int64;
		double value_double;
		ptrdiff_t length_int64;
		ptrdiff_t length_double;

		value_int64 = g_ascii_strtoll (*expression, &end, 0);
		length_int64 = end - *expression;

		end = *expression;
		arv_str_parse_double (&end, &value_double);
		length_double = end - *expression;

		if (length_double > 0 || length_int64 > 0) {
			if (length_double > length_int64) {
				token = g_new (ArvEvaluatorToken, 1);
				token->token_id = ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE;
				token->value.double_value = value_double;
				*expression += length_double;
			} else {
				token = g_new (ArvEvaluatorToken, 1);
				token->token_id = ARV_EVALUATOR_TOKEN_CONSTANT_INT64;
				token->value.int64_value = value_int64;
				*expression += length_int64;
			}
		}
	} else if (g_ascii_isalpha (**expression)) {
		char *end = *expression;
		ptrdiff_t token_length;

		while (g_ascii_isalpha (*end))
			end++;

		token_length = end - *expression;

		if (g_ascii_strncasecmp ("sin", *expression, token_length) == 0)
			token_id = ARV_EVALUATOR_TOKEN_FUNCTION_SIN;
		else if (g_ascii_strncasecmp ("cos", *expression, token_length) == 0)
			token_id = ARV_EVALUATOR_TOKEN_FUNCTION_COS;

		token = g_new (ArvEvaluatorToken, 1);
		if (token_id != ARV_EVALUATOR_TOKEN_UNKNOWN)
			token->token_id = token_id;
		else {
			token->token_id = ARV_EVALUATOR_TOKEN_VARIABLE;
			token->value.string_value = *expression;
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
			case '=': if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_EQUAL;
				  }
				  break;
			case '!': if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EVALUATOR_TOKEN_NOT_EQUAL;
				  }
				  break;
		}

		if (token_id != ARV_EVALUATOR_TOKEN_UNKNOWN) {
			(*expression)++;
			token = g_new (ArvEvaluatorToken, 1);
			token->token_id = token_id;
		}
	}

	return token;
}

ArvEvaluatorStatus
evaluate (GSList *token_stack, double *value)
{
	ArvEvaluatorToken *token;
	ArvEvaluatorStatus status;
	GSList *iter;
	double stack[128];
	int index = -1;

	for (iter = token_stack; iter != NULL; iter = iter->next) {
		token = iter->data;

		if (index < (arv_evaluator_token_infos[token->token_id].n_args - 1)) {
			status = ARV_EVALUATOR_STATUS_MISSING_ARGUMENTS;
			goto CLEANUP;
		}

		arv_evaluator_token_debug (token);

		switch (token->token_id) {
			case ARV_EVALUATOR_TOKEN_UNKNOWN:
				status = ARV_EVALUATOR_STATUS_UNKNOWN_OPERATOR;
				goto CLEANUP;
				break;
			case ARV_EVALUATOR_TOKEN_LOGICAL_OR:
				stack[index - 1] = (gint64) stack[index - 1] | (gint64) stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_EQUAL:
				stack[index - 1] = (stack[index - 1] == stack[index]);
				break;
			case ARV_EVALUATOR_TOKEN_SUBSTRACTION:
				stack[index - 1] -= stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_ADDITION:
				stack[index - 1] += stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_REMAINDER:
				stack[index - 1] = (gint64) stack[index - 1] % (gint64) stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_DIVISION:
				stack[index - 1] /= stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_MULTIPLICATION:
				stack[index - 1] *= stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_POWER:
				stack[index - 1] = pow (stack[index - 1], stack[index]);
				break;
			case ARV_EVALUATOR_TOKEN_MINUS:
				stack[index] = -stack[index];
				break;
			case ARV_EVALUATOR_TOKEN_PLUS:
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_SIN:
				stack[index] = sin (stack[index]);
				break;
			case ARV_EVALUATOR_TOKEN_FUNCTION_COS:
				stack[index] = cos (stack[index]);
				break;
			case ARV_EVALUATOR_TOKEN_CONSTANT_INT64:
				stack[index + 1] = token->value.int64_value;
				break;
			case ARV_EVALUATOR_TOKEN_CONSTANT_DOUBLE:
				stack[index + 1] = token->value.double_value;
				break;
			case ARV_EVALUATOR_TOKEN_VARIABLE:
				stack[index + 1] = token->value.double_value;
				break;
#if 0
	ARV_EVALUATOR_TOKEN_LOGICAL_AND,
	ARV_EVALUATOR_TOKEN_BITWISE_NOT,
	ARV_EVALUATOR_TOKEN_BITWISE_OR,
	ARV_EVALUATOR_TOKEN_BITWISE_XOR,
	ARV_EVALUATOR_TOKEN_BITWISE_AND,
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
	ARV_EVALUATOR_TOKEN_DIVISOR,
	ARV_EVALUATOR_TOKEN_MULTIPLIER,
	ARV_EVALUATOR_TOKEN_POWER,
	ARV_EVALUATOR_TOKEN_MINUS,
	ARV_EVALUATOR_TOKEN_PLUS,
	ARV_EVALUATOR_TOKEN_RIGHT_PARENTHESIS,
	ARV_EVALUATOR_TOKEN_LEFT_PARENTHESIS,
	ARV_EVALUATOR_TOKEN_CONSTANT,
	ARV_EVALUATOR_TOKEN_VARIABLE
#endif
			default:
				break;
		}

		index = index - arv_evaluator_token_infos[token->token_id].n_args + 1;
	}

	if (index != 0) {
		status = ARV_EVALUATOR_STATUS_REMAINING_OPERANDS;
		goto CLEANUP;
	}

	*value = stack[index];

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[Evaluator::evaluate] Result = %g", *value);

	return ARV_EVALUATOR_STATUS_SUCCESS;

CLEANUP:
	return status;
}

static ArvEvaluatorStatus
parse_expression (char *expression, GSList **rpn_stack)
{
	ArvEvaluatorToken *token;
	ArvEvaluatorToken *previous_token = NULL;
	ArvEvaluatorStatus status;
	GSList *token_stack = NULL;
	GSList *operator_stack = NULL;
	GSList *iter;

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, expression);

	/* Dijkstra's "shunting yard" algorithm */
	/* http://en.wikipedia.org/wiki/Shunting-yard_algorithm */

	do {
		token = arv_get_next_token (&expression, previous_token);
		previous_token = token;
		if (token != NULL) {
			if (arv_evaluator_token_is_operand (token)) {
				token_stack = g_slist_prepend (token_stack, token);
			} else if (arv_evaluator_token_is_comma (token)) {
				while (operator_stack != NULL &&
				       !arv_evaluator_token_is_left_parenthesis (operator_stack->data)) {
					token_stack = g_slist_prepend (token_stack, operator_stack->data);
					operator_stack = g_slist_delete_link (operator_stack, operator_stack);
				}
				if (operator_stack == NULL ||
				    !arv_evaluator_token_is_left_parenthesis (operator_stack->data)) {
					status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
					goto CLEANUP;
				}
				g_free (token);
			} else if (arv_evaluator_token_is_operator (token)) {
				while (operator_stack != NULL &&
				       arv_evaluator_token_compare_precedence (token, operator_stack->data)) {
					token_stack = g_slist_prepend (token_stack, operator_stack->data);
					operator_stack = g_slist_delete_link (operator_stack, operator_stack);
				}
				operator_stack = g_slist_prepend (operator_stack, token);
			} else if (arv_evaluator_token_is_left_parenthesis (token)) {
				operator_stack = g_slist_prepend (operator_stack, token);
			} else if (arv_evaluator_token_is_right_parenthesis (token)) {
				while (operator_stack != NULL &&
				       !arv_evaluator_token_is_left_parenthesis (operator_stack->data)) {
					token_stack = g_slist_prepend (token_stack, operator_stack->data);
					operator_stack = g_slist_delete_link (operator_stack, operator_stack);
				}
				if (operator_stack == NULL) {
					status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
					goto CLEANUP;
				}

				g_free (operator_stack->data);
				operator_stack = g_slist_delete_link (operator_stack, operator_stack);
			} else {
				status = ARV_EVALUATOR_STATUS_SYNTAX_ERROR;
				goto CLEANUP;
			}
		}
	} while (token != NULL);

	while (operator_stack != NULL) {
		if (arv_evaluator_token_is_left_parenthesis (operator_stack->data)) {
			status = ARV_EVALUATOR_STATUS_PARENTHESES_MISMATCH;
			goto CLEANUP;
		}

		token_stack = g_slist_prepend (token_stack, operator_stack->data);
		operator_stack = g_slist_delete_link (operator_stack, operator_stack);
	}

	*rpn_stack = g_slist_reverse (token_stack);

	return *rpn_stack == NULL ? ARV_EVALUATOR_STATUS_EMPTY_EXPRESSION : ARV_EVALUATOR_STATUS_SUCCESS;

CLEANUP:

	for (iter = token_stack; iter != NULL; iter = iter->next)
		g_free (iter->data);
	g_slist_free (token_stack);
	for (iter = operator_stack; iter != NULL; iter = iter->next)
		g_free (iter->data);
	g_slist_free (operator_stack);

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
}

double
arv_evaluator_evaluate_as_double (ArvEvaluator *evaluator, GError **error)
{
	ArvEvaluatorStatus status;
	double value;

	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), 0.0);

	if (evaluator->priv->parsing_status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, evaluator->priv->parsing_status);
		return 0.0;
	}

	status = evaluate (evaluator->priv->rpn_stack, &value);
	if (status != ARV_EVALUATOR_STATUS_SUCCESS) {
		arv_evaluator_set_error (error, status);
		return 0.0;
	}

	return value;
}

gint64
arv_evaluator_evaluate_as_int64 (ArvEvaluator *evaluator, GError **error)
{
	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), 0.0);

	return 0;
}

void
arv_evaluator_set_expression (ArvEvaluator *evaluator, const char *expression)
{
	GSList *iter;

	g_return_if_fail (ARV_IS_EVALUATOR (evaluator));

	if (g_strcmp0 (expression, evaluator->priv->expression) == 0)
		return;

	g_free (evaluator->priv->expression);
	evaluator->priv->expression = NULL;
	for (iter = evaluator->priv->rpn_stack; iter != NULL; iter = iter->next)
		g_free (iter->data);
	g_slist_free (evaluator->priv->rpn_stack);
	evaluator->priv->rpn_stack = NULL;

	if (expression == NULL)
		return;

	evaluator->priv->expression = g_strdup (expression);
	evaluator->priv->parsing_status = parse_expression (evaluator->priv->expression,
							    &evaluator->priv->rpn_stack);

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "[Evaluator::set_expression] Parsing status = %d",
		   evaluator->priv->parsing_status);
}

const char *
arv_evaluator_get_expression (ArvEvaluator *evaluator)
{
	g_return_val_if_fail (ARV_IS_EVALUATOR (evaluator), NULL);

	return evaluator->priv->expression;
}

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
}

static void
arv_evaluator_finalize (GObject *object)
{
	ArvEvaluator *evaluator = ARV_EVALUATOR (object);

	arv_evaluator_set_expression (evaluator, NULL);

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
