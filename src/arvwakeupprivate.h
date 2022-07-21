/*
 * Copyright © 2009-2022 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef ARV_WAKEUP_PRIVATE_H
#define ARV_WAKEUP_PRIVATE_H

#include <arvtypes.h>

typedef struct _ArvWakeup ArvWakeup;

ArvWakeup * 	arv_wakeup_new            (void);
void            arv_wakeup_free           (ArvWakeup *wakeup);

void            arv_wakeup_get_pollfd     (ArvWakeup *wakeup,
					   GPollFD *poll_fd);
void            arv_wakeup_signal         (ArvWakeup *wakeup);
void            arv_wakeup_acknowledge    (ArvWakeup *wakeup);

#endif
