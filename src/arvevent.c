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

/**
 * ArvEvent:
 *
 * [class@ArvEvent] holds the data of an event notification sent by a device.
 *
 * Events are enabled using the EventSelector and EventNotification features, or the
 * [method@ArvCamera.set_event_notification] and [method@ArvCamera.set_events] convenience functions. They are delivered
 * by the [signal@ArvDevice::device-event] signal and by the callback set with [method@ArvDevice.set_event_callback].
 *
 * An event is immutable and can safely be shared between threads.
 *
 * Since: 0.10.0
 */

#include <arveventprivate.h>
#include <string.h>

struct _ArvEvent {
	GObject	object;

	guint id;
	guint64 timestamp;
	guint64 block_id;
	guint16 stream_channel;
	GBytes *data;
};

struct _ArvEventClass {
	GObjectClass parent_class;
};

G_DEFINE_TYPE (ArvEvent, arv_event, G_TYPE_OBJECT)

/**
 * arv_event_get_id:
 * @event: a #ArvEvent
 *
 * Returns: the event identifier, as found in the EventID element of the Genicam data.
 *
 * Since: 0.10.0
 */

guint
arv_event_get_id (ArvEvent *event)
{
	g_return_val_if_fail (ARV_IS_EVENT (event), 0);

	return event->id;
}

/**
 * arv_event_get_timestamp:
 * @event: a #ArvEvent
 *
 * Returns: the device timestamp of the event, in device ticks, or 0 if not available.
 *
 * Since: 0.10.0
 */

guint64
arv_event_get_timestamp (ArvEvent *event)
{
	g_return_val_if_fail (ARV_IS_EVENT (event), 0);

	return event->timestamp;
}

/**
 * arv_event_get_block_id:
 * @event: a #ArvEvent
 *
 * Returns: the identifier of the stream block related to the event, or 0 if not available.
 *
 * Since: 0.10.0
 */

guint64
arv_event_get_block_id (ArvEvent *event)
{
	g_return_val_if_fail (ARV_IS_EVENT (event), 0);

	return event->block_id;
}

/**
 * arv_event_get_stream_channel:
 * @event: a #ArvEvent
 *
 * Returns: the index of the stream channel related to the event, or 0xffff if not related to a stream channel.
 *
 * Since: 0.10.0
 */

guint16
arv_event_get_stream_channel (ArvEvent *event)
{
	g_return_val_if_fail (ARV_IS_EVENT (event), ARV_EVENT_STREAM_CHANNEL_NONE);

	return event->stream_channel;
}

/**
 * arv_event_get_data:
 * @event: a #ArvEvent
 *
 * Returns: (transfer full): the event data, as seen by the Genicam event port.
 *
 * Since: 0.10.0
 */

GBytes *
arv_event_get_data (ArvEvent *event)
{
	g_return_val_if_fail (ARV_IS_EVENT (event), NULL);

	return g_bytes_ref (event->data);
}

/**
 * arv_event_peek_data:
 * @event: a #ArvEvent
 * @size: (out) (optional): data size placeholder
 *
 * Returns: (transfer none) (array length=size) (element-type guint8): a pointer to the event data, valid during the
 * event lifetime.
 *
 * Since: 0.10.0
 */

const void *
arv_event_peek_data (ArvEvent *event, size_t *size)
{
	gsize data_size = 0;
	const void *data;

	if (size != NULL)
		*size = 0;

	g_return_val_if_fail (ARV_IS_EVENT (event), NULL);

	data = g_bytes_get_data (event->data, &data_size);
	if (size != NULL)
		*size = data_size;

	return data;
}

ArvEvent *
arv_event_new (guint id, guint64 timestamp, guint64 block_id, guint16 stream_channel, const void *data, size_t size)
{
	ArvEvent *event;

	g_return_val_if_fail (data != NULL || size == 0, NULL);

	event = g_object_new (ARV_TYPE_EVENT, NULL);
	event->id = id;
	event->timestamp = timestamp;
	event->block_id = block_id;
	event->stream_channel = stream_channel;
	event->data = g_bytes_new (data, size);

	return event;
}

static void
arv_event_init (ArvEvent *event)
{
	event->stream_channel = ARV_EVENT_STREAM_CHANNEL_NONE;
}

static void
arv_event_finalize (GObject *object)
{
	ArvEvent *event = ARV_EVENT (object);

	g_clear_pointer (&event->data, g_bytes_unref);

	G_OBJECT_CLASS (arv_event_parent_class)->finalize (object);
}

static void
arv_event_class_init (ArvEventClass *event_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (event_class);

	object_class->finalize = arv_event_finalize;
}
