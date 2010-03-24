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

#include <arvexpression.h>
#include <arvtools.h>
#include <arvdebug.h>
#include <math.h>

#define ARV_EXPRESSION_STACK_SIZE	128

typedef enum {
	ARV_EXPRESSION_TOKEN_UNKNOWN,
	ARV_EXPRESSION_TOKEN_COMMA,
	ARV_EXPRESSION_TOKEN_TERNARY_QUESTION_MARK,
	ARV_EXPRESSION_TOKEN_TERNARY_COLON,
	ARV_EXPRESSION_TOKEN_LOGICAL_OR,
	ARV_EXPRESSION_TOKEN_LOGICAL_AND,
	ARV_EXPRESSION_TOKEN_BITWISE_NOT,
	ARV_EXPRESSION_TOKEN_BITWISE_OR,
	ARV_EXPRESSION_TOKEN_BITWISE_XOR,
	ARV_EXPRESSION_TOKEN_BITWISE_AND,
	ARV_EXPRESSION_TOKEN_EQUAL,
	ARV_EXPRESSION_TOKEN_NOT_EQUAL,
	ARV_EXPRESSION_TOKEN_LESS_OR_EQUAL,
	ARV_EXPRESSION_TOKEN_GREATER_OR_EQUAL,
	ARV_EXPRESSION_TOKEN_LESS,
	ARV_EXPRESSION_TOKEN_GREATER,
	ARV_EXPRESSION_TOKEN_SHIFT_RIGHT,
	ARV_EXPRESSION_TOKEN_SHIFT_LEFT,
	ARV_EXPRESSION_TOKEN_SUBSTRACTION,
	ARV_EXPRESSION_TOKEN_ADDITION,
	ARV_EXPRESSION_TOKEN_REMAINDER,
	ARV_EXPRESSION_TOKEN_DIVISION,
	ARV_EXPRESSION_TOKEN_MULTIPLICATION,
	ARV_EXPRESSION_TOKEN_POWER,
	ARV_EXPRESSION_TOKEN_MINUS,
	ARV_EXPRESSION_TOKEN_PLUS,
	ARV_EXPRESSION_TOKEN_FUNCTION_SIN,
	ARV_EXPRESSION_TOKEN_FUNCTION_COS,
#if 0
	ARV_EXPRESSION_TOKEN_FUNCTION_SGN,
	ARV_EXPRESSION_TOKEN_FUNCTION_NEG,
	ARV_EXPRESSION_TOKEN_FUNCTION_ATAN,
	ARV_EXPRESSION_TOKEN_FUNCTION_TAN,
	ARV_EXPRESSION_TOKEN_FUNCTION_ABS,
	ARV_EXPRESSION_TOKEN_FUNCTION_EXP,
	ARV_EXPRESSION_TOKEN_FUNCTION_LN,
	ARV_EXPRESSION_TOKEN_FUNCTION_LG,
	ARV_EXPRESSION_TOKEN_FUNCTION_SQRT,
	ARV_EXPRESSION_TOKEN_FUNCTION_TRUNC,
	ARV_EXPRESSION_TOKEN_FUNCTION_FLOOR,
	ARV_EXPRESSION_TOKEN_FUNCTION_CEIL,
	ARV_EXPRESSION_TOKEN_FUNCTION_ASIN,
	ARV_EXPRESSION_TOKEN_FUNCTION_ACOS,
	ARV_EXPRESSION_TOKEN_FUNCTION_E,
	ARV_EXPRESSION_TOKEN_FUNCTION_PI
#endif
	ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS,
	ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS,
	ARV_EXPRESSION_TOKEN_CONSTANT,
	ARV_EXPRESSION_TOKEN_VARIABLE
} ArvExpressionTokenId;

typedef enum {
	ARV_EXPRESSION_TOKEN_ASSOCIATIVITY_LEFT_TO_RIGHT,
	ARV_EXPRESSION_TOKEN_ASSOCIATIVITY_RIGHT_TO_LEFT
} ArvExpressionTokenAssociativity;

typedef struct {
	const char *		tag;
	int			precedence;
	int			n_args;
} ArvExpressionTokenInfos;

static ArvExpressionTokenInfos arv_expression_token_infos[] = {
	{"",	0,	1}, /* ARV_EXPRESSION_TOKEN_UNKNOWN */
	{",",	0, 	0}, /* ARV_EXPRESSION_TOKEN_COMMA */
	{"?",	5,	3}, /* ARV_EXPRESSION_TOKEN_TERNARY_QUESTION_MARK */
	{":",	5,	0}, /* ARV_EXPRESSION_TOKEN_TERNARY_COLON */
	{"||",	10,	2}, /* ARV_EXPRESSION_TOKEN_LOGICAL_OR */
	{"&&",	20,	2}, /* ARV_EXPRESSION_TOKEN_LOGICAL_AND */
	{"~",	30,	1}, /* ARV_EXPRESSION_TOKEN_BITWISE_NOT */
	{"|",	40,	2}, /* ARV_EXPRESSION_TOKEN_BITWISE_OR */
	{"^",	50,	2}, /* ARV_EXPRESSION_TOKEN_BITWISE_XOR */
	{"&",	60,	2}, /* ARV_EXPRESSION_TOKEN_BITWISE_AND */
	{"==",	70,	2}, /* ARV_EXPRESSION_TOKEN_EQUAL, */
	{"!=",	70,	2}, /* ARV_EXPRESSION_TOKEN_NOT_EQUAL */
	{"<=",	80,	2}, /* ARV_EXPRESSION_TOKEN_LESS_OR_EQUAL */
	{">=",	80,	2}, /* ARV_EXPRESSION_TOKEN_GREATER_OR_EQUAL */
	{"<",	80,	2}, /* ARV_EXPRESSION_TOKEN_LESS */
	{">",	80,	2}, /* ARV_EXPRESSION_TOKEN_GREATER */
	{">>",	90,	2}, /* ARV_EXPRESSION_TOKEN_SHIFT_RIGHT */
	{"<<",	90,	2}, /* ARV_EXPRESSION_TOKEN_SHIFT_LEFT */
	{"-",	100, 	2}, /* ARV_EXPRESSION_TOKEN_SUBSTRACTION */
	{"+",	100, 	2}, /* ARV_EXPRESSION_TOKEN_ADDITION */
	{"%",	110,	2}, /* ARV_EXPRESSION_TOKEN_REMAINDER */
	{"/",	110,	2}, /* ARV_EXPRESSION_TOKEN_DIVISION */
	{"*",	110, 	2}, /* ARV_EXPRESSION_TOKEN_MULTIPLICATION */
	{"**",	120,	2}, /* ARV_EXPRESSION_TOKEN_POWER */
	{"min",	130, 	1}, /* ARV_EXPRESSION_TOKEN_MINUS */
	{"pls",	130, 	1}, /* ARV_EXPRESSION_TOKEN_PLUS */
	{"sin",	200,	1}, /* ARV_EXPRESSION_TOKEN_FUNCTION_SIN */
	{"cos",	200,	1}, /* ARV_EXPRESSION_TOKEN_FUNCTION_COS */
#if 0
	ARV_EXPRESSION_TOKEN_FUNCTION_SGN,
	ARV_EXPRESSION_TOKEN_FUNCTION_NEG,
	ARV_EXPRESSION_TOKEN_FUNCTION_ATAN,
	ARV_EXPRESSION_TOKEN_FUNCTION_TAN,
	ARV_EXPRESSION_TOKEN_FUNCTION_ABS,
	ARV_EXPRESSION_TOKEN_FUNCTION_EXP,
	ARV_EXPRESSION_TOKEN_FUNCTION_LN,
	ARV_EXPRESSION_TOKEN_FUNCTION_LG,
	ARV_EXPRESSION_TOKEN_FUNCTION_SQRT,
	ARV_EXPRESSION_TOKEN_FUNCTION_TRUNC,
	ARV_EXPRESSION_TOKEN_FUNCTION_FLOOR,
	ARV_EXPRESSION_TOKEN_FUNCTION_CEIL,
	ARV_EXPRESSION_TOKEN_FUNCTION_ASIN,
	ARV_EXPRESSION_TOKEN_FUNCTION_ACOS,
	ARV_EXPRESSION_TOKEN_FUNCTION_E,
	ARV_EXPRESSION_TOKEN_FUNCTION_PI
#endif
	{")",	990, 	0}, /* ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS */
	{"(",	-1, 	0}, /* ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS */
	{"cnst",200,	0}, /* ARV_EXPRESSION_TOKEN_CONSTANT */
	{"var",	200,	0}, /* ARV_EXPRESSION_TOKEN_VARIABLE */
};

typedef struct {
	ArvExpressionTokenId	token_id;
	union {
		double		double_value;
		char * 		string_value;
	} value;
} ArvExpressionToken;

void
arv_expression_token_debug (ArvExpressionToken *token)
{
	g_return_if_fail (token != NULL);

	switch (token->token_id) {
		case ARV_EXPRESSION_TOKEN_VARIABLE:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "%s",
				   token->value.string_value);
			break;
		case ARV_EXPRESSION_TOKEN_CONSTANT:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "%g",
				   token->value.double_value);
			break;
		default:
			arv_debug (ARV_DEBUG_LEVEL_STANDARD, "%s", arv_expression_token_infos[token->token_id]);
	}
}

static gboolean
arv_expression_token_is_operand (ArvExpressionToken *token)
{
	return (token != NULL &&
		token->token_id > ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS);
}

static gboolean
arv_expression_token_is_operator (ArvExpressionToken *token)
{
	return (token != NULL &&
		token->token_id > ARV_EXPRESSION_TOKEN_UNKNOWN &&
		token->token_id < ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS);
}

static gboolean
arv_expression_token_is_left_parenthesis (ArvExpressionToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS);
}

static gboolean
arv_expression_token_is_right_parenthesis (ArvExpressionToken *token)
{
	return (token != NULL &&
		token->token_id == ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS);
}

static int
arv_expression_token_get_precedence (ArvExpressionToken *token)
{
	if (token != NULL &&
	    token->token_id < G_N_ELEMENTS (arv_expression_token_infos))
		return arv_expression_token_infos[token->token_id].precedence;

	return 0;
}

ArvExpressionToken *
arv_get_next_token (char **expression, ArvExpressionToken *previous_token)
{
	ArvExpressionToken *token = NULL;
	ArvExpressionTokenId token_id = ARV_EXPRESSION_TOKEN_UNKNOWN;

	g_return_val_if_fail (expression != NULL && *expression != NULL, NULL);
	arv_str_skip_spaces (expression);

	if (**expression == '\0')
		return NULL;

	if (g_ascii_isdigit (**expression)) {
		char *end;
		double value;

		value = g_ascii_strtoll (*expression, &end, 0);
		if (end != *expression) {
			*expression = end;

			token = g_new (ArvExpressionToken, 1);
			token->token_id = ARV_EXPRESSION_TOKEN_CONSTANT;
			token->value.double_value = value;
		} else if (arv_str_parse_double (expression, &value)) {
			token = g_new (ArvExpressionToken, 1);
			token->token_id = ARV_EXPRESSION_TOKEN_CONSTANT;
			token->value.double_value = value;
		}
	} else if (g_ascii_isalpha (**expression)) {
		char *end = *expression;
		ptrdiff_t token_length;

		while (g_ascii_isalpha (*end))
			end++;

		token_length = end - *expression;

		if (g_ascii_strncasecmp ("sin", *expression, token_length) == 0)
			token_id = ARV_EXPRESSION_TOKEN_FUNCTION_SIN;
		else if (g_ascii_strncasecmp ("cos", *expression, token_length) == 0)
			token_id = ARV_EXPRESSION_TOKEN_FUNCTION_COS;

		token = g_new (ArvExpressionToken, 1);
		if (token_id != ARV_EXPRESSION_TOKEN_UNKNOWN)
			token->token_id = token_id;
		else {
			token->token_id = ARV_EXPRESSION_TOKEN_VARIABLE;
			token->value.string_value = *expression;
		}

		*expression = end;

		g_message ("exp = '%s'", *expression);

	} else {
		switch (**expression) {
			case '(': token_id = ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS; break;
			case ')': token_id = ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS; break;
			case ',': token_id = ARV_EXPRESSION_TOKEN_COMMA; break;
			case '?': token_id = ARV_EXPRESSION_TOKEN_TERNARY_QUESTION_MARK; break;
			case ':': token_id = ARV_EXPRESSION_TOKEN_TERNARY_COLON; break;
			case '+': if (previous_token != NULL &&
				      (arv_expression_token_is_operand (previous_token) ||
				       arv_expression_token_is_right_parenthesis (previous_token)))
					  token_id = ARV_EXPRESSION_TOKEN_ADDITION;
				  else
					  token_id = ARV_EXPRESSION_TOKEN_PLUS;
				  break;
			case '-': if (previous_token != NULL &&
				      (arv_expression_token_is_operand (previous_token) ||
				       arv_expression_token_is_right_parenthesis (previous_token)))
					  token_id = ARV_EXPRESSION_TOKEN_SUBSTRACTION;
				  else
					  token_id = ARV_EXPRESSION_TOKEN_MINUS;
				  break;
			case '*': if ((*expression)[1] == '*') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_POWER;
				  } else
					  token_id = ARV_EXPRESSION_TOKEN_MULTIPLICATION;
				  break;
			case '/': token_id = ARV_EXPRESSION_TOKEN_DIVISION; break;
			case '%': token_id = ARV_EXPRESSION_TOKEN_REMAINDER; break;
			case '&': if ((*expression)[1] == '&') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_LOGICAL_AND;
				  } else
					  token_id = ARV_EXPRESSION_TOKEN_BITWISE_AND;
				  break;
			case '|': if ((*expression)[1] == '|') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_LOGICAL_OR;
				  } else
					  token_id = ARV_EXPRESSION_TOKEN_BITWISE_OR;
				  break;
			case '^': token_id = ARV_EXPRESSION_TOKEN_BITWISE_XOR; break;
			case '~': token_id = ARV_EXPRESSION_TOKEN_BITWISE_NOT; break;
			case '<': if ((*expression)[1] == '>') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_NOT_EQUAL;
				  } else if ((*expression)[1] == '<') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_SHIFT_LEFT;
				  } else if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_LESS_OR_EQUAL;
				  } else
					  token_id = ARV_EXPRESSION_TOKEN_LESS;
				  break;
			case '>': if ((*expression)[1] == '>') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_SHIFT_RIGHT;
				  } else if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_GREATER_OR_EQUAL;
				  } else
					  token_id = ARV_EXPRESSION_TOKEN_GREATER;
				  break;
			case '=': if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_EQUAL;
				  }
				  break;
			case '!': if ((*expression)[1] == '=') {
					  (*expression)++;
					  token_id = ARV_EXPRESSION_TOKEN_NOT_EQUAL;
				  }
				  break;
		}

		if (token_id != ARV_EXPRESSION_TOKEN_UNKNOWN) {
			(*expression)++;
			token = g_new (ArvExpressionToken, 1);
			token->token_id = token_id;
		}
	}

	return token;
}

double
evaluate (GSList *token_stack)
{
	ArvExpressionToken *token;
	GSList *iter;
	double stack[128];
	int index = -1;

	for (iter = token_stack; iter != NULL; iter = iter->next) {
		token = iter->data;

		if (index < (arv_expression_token_infos[token->token_id].n_args - 1))
			g_error ("Too few arguments");

		switch (token->token_id) {
			case ARV_EXPRESSION_TOKEN_UNKNOWN:
				g_error ("Unknow token");
				break;
			case ARV_EXPRESSION_TOKEN_LOGICAL_OR:
				stack[index - 1] = (gint64) stack[index - 1] | (gint64) stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_SUBSTRACTION:
				stack[index - 1] -= stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_ADDITION:
				stack[index - 1] += stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_REMAINDER:
				stack[index - 1] = (gint64) stack[index - 1] % (gint64) stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_DIVISION:
				stack[index - 1] /= stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_MULTIPLICATION:
				stack[index - 1] *= stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_POWER:
				stack[index - 1] = pow (stack[index - 1], stack[index]);
				break;
			case ARV_EXPRESSION_TOKEN_MINUS:
				stack[index] = -stack[index];
				break;
			case ARV_EXPRESSION_TOKEN_PLUS:
				break;
			case ARV_EXPRESSION_TOKEN_CONSTANT:
				stack[index + 1] = token->value.double_value;
				break;
			case ARV_EXPRESSION_TOKEN_VARIABLE:
				stack[index + 1] = token->value.double_value;
				break;
			default:
				break;
		}

		index = index - arv_expression_token_infos[token->token_id].n_args + 1;
	}
#if 0
	ARV_EXPRESSION_TOKEN_LOGICAL_AND,
		ARV_EXPRESSION_TOKEN_BITWISE_NOT,
		ARV_EXPRESSION_TOKEN_BITWISE_OR,
		ARV_EXPRESSION_TOKEN_BITWISE_XOR,
		ARV_EXPRESSION_TOKEN_BITWISE_AND,
		ARV_EXPRESSION_TOKEN_EQUAL,
		ARV_EXPRESSION_TOKEN_NOT_EQUAL,
		ARV_EXPRESSION_TOKEN_LESS_OR_EQUAL,
		ARV_EXPRESSION_TOKEN_GREATER_OR_EQUAL,
		ARV_EXPRESSION_TOKEN_LESS,
		ARV_EXPRESSION_TOKEN_GREATER,
		ARV_EXPRESSION_TOKEN_SHIFT_RIGHT,
		ARV_EXPRESSION_TOKEN_SHIFT_LEFT,
		ARV_EXPRESSION_TOKEN_SUBSTRACTION,
		ARV_EXPRESSION_TOKEN_ADDITION,
		ARV_EXPRESSION_TOKEN_REMAINDER,
		ARV_EXPRESSION_TOKEN_DIVISOR,
		ARV_EXPRESSION_TOKEN_MULTIPLIER,
		ARV_EXPRESSION_TOKEN_POWER,
		ARV_EXPRESSION_TOKEN_MINUS,
		ARV_EXPRESSION_TOKEN_PLUS,
		ARV_EXPRESSION_TOKEN_FUNCTION_SIN,
		ARV_EXPRESSION_TOKEN_FUNCTION_COS,
		ARV_EXPRESSION_TOKEN_RIGHT_PARENTHESIS,
		ARV_EXPRESSION_TOKEN_LEFT_PARENTHESIS,
		ARV_EXPRESSION_TOKEN_CONSTANT,
		ARV_EXPRESSION_TOKEN_VARIABLE
#endif

		if (index != 0)
			g_error ("remaining operands %d", index);

	return stack[index];
}

void
parse_expression (char *expression)
{
	ArvExpressionToken *token;
	ArvExpressionToken *previous_token = NULL;
	GSList *token_stack = NULL;
	GSList *operator_stack = NULL;
	GSList *iter;

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, expression);

	do {
		token = arv_get_next_token (&expression, previous_token);
		previous_token = token;
		if (token != NULL) {
			int precedence = arv_expression_token_get_precedence (token);

			arv_expression_token_debug (token);

			if (arv_expression_token_is_operand (token))
				token_stack = g_slist_prepend (token_stack, token);
			else if (arv_expression_token_is_operator (token)) {
				while (operator_stack != NULL &&
				       precedence <=
				       arv_expression_token_get_precedence (operator_stack->data)) {
					token_stack = g_slist_prepend (token_stack, operator_stack->data);
					operator_stack = g_slist_delete_link (operator_stack, operator_stack);
				}
				operator_stack = g_slist_prepend (operator_stack, token);
			} else if (arv_expression_token_is_left_parenthesis (token)) {
				operator_stack = g_slist_prepend (operator_stack, token);
			} else if (arv_expression_token_is_right_parenthesis (token)) {
				while (operator_stack != NULL &&
				       !arv_expression_token_is_left_parenthesis (operator_stack->data)) {
					token_stack = g_slist_prepend (token_stack, operator_stack->data);
					operator_stack = g_slist_delete_link (operator_stack, operator_stack);
				}
				if (operator_stack == NULL)
					g_error ("parenthesis error");
				g_free (operator_stack->data);
				operator_stack = g_slist_delete_link (operator_stack, operator_stack);
			} else
				g_error ("syntax error");
		}
	} while (token != NULL);

	while (operator_stack != NULL) {
		if (arv_expression_token_is_left_parenthesis (operator_stack->data))
			g_error ("parenthesis error");

		token_stack = g_slist_prepend (token_stack, operator_stack->data);
		operator_stack = g_slist_delete_link (operator_stack, operator_stack);
	}

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "RPN token stack");

	token_stack = g_slist_reverse (token_stack);

	for (iter = token_stack; iter != NULL; iter = iter->next) {
		arv_expression_token_debug (iter->data);
	}

	arv_debug (ARV_DEBUG_LEVEL_STANDARD, "Result = %g", evaluate (token_stack));

}
