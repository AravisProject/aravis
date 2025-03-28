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

#include <arvtypes.h>

#include <arvbuffer.h>
#include <arvcamera.h>
#include <arvchunkparser.h>
#include <arvdebug.h>
#include <arvdevice.h>

#include <arvdomcharacterdata.h>
#include <arvdomdocumentfragment.h>
#include <arvdomdocument.h>
#include <arvdomelement.h>
#include <arvdomimplementation.h>
#include <arvdomnamednodemap.h>
#include <arvdomnode.h>
#include <arvdomnodelist.h>
#include <arvdomnodechildlist.h>
#include <arvdomparser.h>
#include <arvdomtext.h>

#include <arvenums.h>
#include <arvevaluator.h>

#include <arvfakecamera.h>
#include <arvfakedevice.h>
#include <arvfakeinterface.h>
#include <arvfakestream.h>

#include <arvfeatures.h>

#include <arvgc.h>
#include <arvgcboolean.h>
#include <arvgccategory.h>
#include <arvgccommand.h>
#include <arvgcconverter.h>
#include <arvgcconverternode.h>
#include <arvgcenumentry.h>
#include <arvgcenumeration.h>
#include <arvgcenums.h>
#include <arvgcfeaturenode.h>
#include <arvgcfloat.h>
#include <arvgcfloatnode.h>
#include <arvgcfloatregnode.h>
#include <arvgcgroupnode.h>
#include <arvgcindexnode.h>
#include <arvgcintconverternode.h>
#include <arvgcinteger.h>
#include <arvgcintegernode.h>
#include <arvgcintregnode.h>
#include <arvgcintswissknifenode.h>
#include <arvgcinvalidatornode.h>
#include <arvgcnode.h>
#include <arvgcmaskedintregnode.h>
#include <arvgcport.h>
#include <arvgcpropertynode.h>
#include <arvgcregisterdescriptionnode.h>
#include <arvgcregister.h>
#include <arvgcregisternode.h>
#include <arvgcselector.h>
#include <arvgcstring.h>
#include <arvgcstringnode.h>
#include <arvgcstringregnode.h>
#include <arvgcstructregnode.h>
#include <arvgcstructentrynode.h>
#include <arvgcswissknife.h>
#include <arvgcswissknifenode.h>
#include <arvgcvalueindexednode.h>

#include <arvgvdevice.h>
#include <arvgvfakecamera.h>
#include <arvgvinterface.h>
#include <arvgvstream.h>

#include <arvgentlsystem.h>
#include <arvgentlinterface.h>
#include <arvgentldevice.h>
#include <arvgentlstream.h>

#include <arvinterface.h>
#include <arvmisc.h>
#include <arvnetwork.h>
#include <arvrealtime.h>
#include <arvstream.h>
#include <arvstr.h>
#include <arvsystem.h>

#if ARAVIS_HAS_USB
#include <arvuvinterface.h>
#include <arvuvdevice.h>
#include <arvuvstream.h>
#endif

#if ARAVIS_HAS_V4L2
#include <arvv4l2interface.h>
#include <arvv4l2device.h>
#include <arvv4l2stream.h>
#endif

#include <arvversion.h>
#include <arvzip.h>
#include <arvxmlschema.h>

#undef ARV_H_INSIDE

#endif
