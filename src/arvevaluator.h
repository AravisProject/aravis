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

#ifndef ARV_EVALUATOR_H
#define ARV_EVALUATOR_H

#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_EVALUATOR             (arv_evaluator_get_type ())
#define ARV_EVALUATOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ARV_TYPE_EVALUATOR, ArvEvaluator))
#define ARV_EVALUATOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ARV_TYPE_EVALUATOR, ArvEvaluatorClass))
#define ARV_IS_EVALUATOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_EVALUATOR))
#define ARV_IS_EVALUATOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ARV_TYPE_EVALUATOR))
#define ARV_EVALUATOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), ARV_TYPE_EVALUATOR, ArvEvaluatorClass))

typedef struct _ArvEvaluatorPrivate ArvEvaluatorPrivate;
typedef struct _ArvEvaluatorClass ArvEvaluatorClass;

struct _ArvEvaluator {
	GObject	object;

	ArvEvaluatorPrivate *priv;
};

struct _ArvEvaluatorClass {
	GObjectClass parent_class;
};

GType arv_evaluator_get_type (void);

ArvEvaluator *	arv_evaluator_new			(const char *expression);
void 		arv_evaluator_set_expression		(ArvEvaluator *evaluator, const char *expression);
const char *	arv_evaluator_get_expression		(ArvEvaluator *evaluator);
double		arv_evaluator_evaluate_as_double	(ArvEvaluator *evaluator, GError **error);
gint64		arv_evaluator_evaluate_as_int64		(ArvEvaluator *evaluator, GError **error);
void		arv_evaluator_set_double_variable	(ArvEvaluator *evaluator, const char *name, double v_double);
void		arv_evaluator_set_int64_variable	(ArvEvaluator *evaluator, const char *name, gint64 v_int64);

G_END_DECLS

#endif
