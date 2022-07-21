/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
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
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_EVALUATOR_H
#define ARV_EVALUATOR_H

#if !defined (ARV_H_INSIDE) && !defined (ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>

G_BEGIN_DECLS

#define ARV_TYPE_EVALUATOR             (arv_evaluator_get_type ())
ARV_API G_DECLARE_FINAL_TYPE (ArvEvaluator, arv_evaluator, ARV, EVALUATOR, GObject)

ARV_API ArvEvaluator *		arv_evaluator_new			(const char *expression);
ARV_API void 			arv_evaluator_set_expression		(ArvEvaluator *evaluator, const char *expression);
ARV_API const char *		arv_evaluator_get_expression		(ArvEvaluator *evaluator);
ARV_API void			arv_evaluator_set_sub_expression	(ArvEvaluator *evaluator, const char *name, const char *expression);
ARV_API const char *		arv_evaluator_get_sub_expression	(ArvEvaluator *evaluator, const char *name);
ARV_API void			arv_evaluator_set_constant		(ArvEvaluator *evaluator, const char *name, const char *constant);
ARV_API const char *		arv_evaluator_get_constant		(ArvEvaluator *evaluator, const char *name);
ARV_API double			arv_evaluator_evaluate_as_double	(ArvEvaluator *evaluator, GError **error);
ARV_API gint64			arv_evaluator_evaluate_as_int64		(ArvEvaluator *evaluator, GError **error);
ARV_API void			arv_evaluator_set_double_variable	(ArvEvaluator *evaluator, const char *name, double v_double);
ARV_API void			arv_evaluator_set_int64_variable	(ArvEvaluator *evaluator, const char *name, gint64 v_int64);

G_END_DECLS

#endif
