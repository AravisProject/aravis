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

#include <arvbuffer.h>

static GObjectClass *parent_class = NULL;

ArvBuffer *
arv_buffer_new (size_t size, void *preallocated)
{
	ArvBuffer *buffer;

	buffer = g_object_new (ARV_TYPE_BUFFER, NULL);
	buffer->size = size;

	if (preallocated != NULL) {
		buffer->is_preallocated = TRUE;
		buffer->data = preallocated;
	} else {
		buffer->is_preallocated = FALSE;
		buffer->data = g_malloc (size);
	}

	return buffer;
}

void
arv_buffer_clear (ArvBuffer *buffer)
{
	g_return_if_fail (ARV_IS_BUFFER (buffer));

	buffer->status = ARV_BUFFER_STATUS_CLEARED;
}

static void
arv_buffer_init (ArvBuffer *buffer)
{
	buffer->status = ARV_BUFFER_STATUS_CLEARED;
}

static void
arv_buffer_finalize (GObject *object)
{
	ArvBuffer *buffer = ARV_BUFFER (object);

	if (!buffer->is_preallocated) {
		g_free (buffer->data);
		buffer->data = NULL;
		buffer->size = 0;
	}

	parent_class->finalize (object);
}

static void
arv_buffer_class_init (ArvBufferClass *node_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (node_class);

	parent_class = g_type_class_peek_parent (node_class);

	object_class->finalize = arv_buffer_finalize;
}

G_DEFINE_TYPE (ArvBuffer, arv_buffer, G_TYPE_OBJECT)

