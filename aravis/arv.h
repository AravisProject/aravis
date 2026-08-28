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

#ifndef ARV_H
#define ARV_H

#define ARV_H_INSIDE

#include <aravis/arvtypes.h>

#include <aravis/arvbuffer.h>
#include <aravis/arvcamera.h>
#include <aravis/arvchunkparser.h>
#include <aravis/arvdebug.h>
#include <aravis/arvdevice.h>

#include <aravis/arvdomcharacterdata.h>
#include <aravis/arvdomdocumentfragment.h>
#include <aravis/arvdomdocument.h>
#include <aravis/arvdomelement.h>
#include <aravis/arvdomimplementation.h>
#include <aravis/arvdomnamednodemap.h>
#include <aravis/arvdomnode.h>
#include <aravis/arvdomnodelist.h>
#include <aravis/arvdomnodechildlist.h>
#include <aravis/arvdomparser.h>
#include <aravis/arvdomtext.h>

#include <aravis/arvenums.h>
#include <aravis/arvevaluator.h>

#include <aravis/arvfakecamera.h>
#include <aravis/arvfakedevice.h>
#include <aravis/arvfakeinterface.h>
#include <aravis/arvfakestream.h>

#include <aravis/arvfeatures.h>

#include <aravis/arvgc.h>
#include <aravis/arvgcboolean.h>
#include <aravis/arvgccategory.h>
#include <aravis/arvgccommand.h>
#include <aravis/arvgcconverter.h>
#include <aravis/arvgcconverternode.h>
#include <aravis/arvgcenumentry.h>
#include <aravis/arvgcenumeration.h>
#include <aravis/arvgcenums.h>
#include <aravis/arvgcfeaturenode.h>
#include <aravis/arvgcfloat.h>
#include <aravis/arvgcfloatnode.h>
#include <aravis/arvgcfloatregnode.h>
#include <aravis/arvgcgroupnode.h>
#include <aravis/arvgcindexnode.h>
#include <aravis/arvgcintconverternode.h>
#include <aravis/arvgcinteger.h>
#include <aravis/arvgcintegernode.h>
#include <aravis/arvgcintregnode.h>
#include <aravis/arvgcintswissknifenode.h>
#include <aravis/arvgcinvalidatornode.h>
#include <aravis/arvgcnode.h>
#include <aravis/arvgcmaskedintregnode.h>
#include <aravis/arvgcport.h>
#include <aravis/arvgcpropertynode.h>
#include <aravis/arvgcregisterdescriptionnode.h>
#include <aravis/arvgcregister.h>
#include <aravis/arvgcregisternode.h>
#include <aravis/arvgcselector.h>
#include <aravis/arvgcstring.h>
#include <aravis/arvgcstringnode.h>
#include <aravis/arvgcstringregnode.h>
#include <aravis/arvgcstructregnode.h>
#include <aravis/arvgcstructentrynode.h>
#include <aravis/arvgcswissknife.h>
#include <aravis/arvgcswissknifenode.h>
#include <aravis/arvgcvalueindexednode.h>

#include <aravis/arvgvdevice.h>
#include <aravis/arvgvfakecamera.h>
#include <aravis/arvgvinterface.h>
#include <aravis/arvgvstream.h>

#include <aravis/arvgentlsystem.h>
#include <aravis/arvgentlinterface.h>
#include <aravis/arvgentldevice.h>
#include <aravis/arvgentlstream.h>

#include <aravis/arvinterface.h>
#include <aravis/arvmisc.h>
#include <aravis/arvnetwork.h>
#include <aravis/arvrealtime.h>
#include <aravis/arvstream.h>
#include <aravis/arvstr.h>
#include <aravis/arvsystem.h>

#if ARAVIS_HAS_USB
#include <aravis/arvuvinterface.h>
#include <aravis/arvuvdevice.h>
#include <aravis/arvuvstream.h>
#endif

#if ARAVIS_HAS_V4L2
#include <aravis/arvv4l2interface.h>
#include <aravis/arvv4l2device.h>
#include <aravis/arvv4l2stream.h>
#endif

#include <aravis/arvversion.h>
#include <aravis/arvzip.h>
#include <aravis/arvxmlschema.h>

#undef ARV_H_INSIDE

#endif
