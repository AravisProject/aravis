/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#ifndef ARV_DOM_NODE_PRIVATE_H
#define ARV_DOM_NODE_PRIVATE_H

#include <arvdomnode.h>

G_BEGIN_DECLS

gboolean	arv_dom_node_is_write_locked	(ArvDomNode *self);
void		arv_dom_node_lazy_lock		(ArvDomNode *self);
void		arv_dom_node_lazy_unlock	(ArvDomNode *self);

typedef void	(*ArvDomNodeLazyBuildFunc)	(ArvDomNode *self);

void		arv_dom_node_lazy_build		(ArvDomNode *self, gint *is_frozen, ArvDomNodeLazyBuildFunc build);

G_END_DECLS

#endif
