/* Aravis - Digital camera library
 *
 * Copyright © 2009-2025 Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include <arvviewertypes.h>
#include <arv.h>

G_BEGIN_DECLS

#define ARV_TYPE_VIEWER             (arv_viewer_get_type ())
#define ARV_IS_VIEWER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ARV_TYPE_VIEWER))

GType 			arv_viewer_get_type 		(void);

ArvViewer * 		arv_viewer_new 			(void);
void			arv_viewer_set_options		(ArvViewer *viewer,
							 gboolean auto_socket_buffer,
							 gboolean packet_resend,
							 guint initial_packet_timeout,
							 guint packet_timeout,
							 guint frame_retention,
							 ArvRegisterCachePolicy register_cache_policy,
							 ArvRangeCheckPolicy range_check_policy,
                                                         ArvUvUsbMode usb_mode);

G_END_DECLS
