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

#ifndef ARV_TYPES_H
#define ARV_TYPES_H

#include <arvenums.h>

G_BEGIN_DECLS

typedef struct _ArvCamera 		ArvCamera;

typedef struct _ArvEvaluator 		ArvEvaluator;

typedef struct _ArvGc 			ArvGc;

typedef struct _ArvGcNode 		ArvGcNode;
typedef struct _ArvGcCategory		ArvGcCategory;
typedef struct _ArvGcBoolean		ArvGcBoolean;
typedef struct _ArvGcEnumeration	ArvGcEnumeration;
typedef struct _ArvGcEnumEntry		ArvGcEnumEntry;
typedef struct _ArvGcIntegerNode	ArvGcIntegerNode;
typedef struct _ArvGcFloatNode		ArvGcFloatNode;
typedef struct _ArvGcRegister 		ArvGcRegister;
typedef struct _ArvGcCommand		ArvGcCommand;
typedef struct _ArvGcSwissKnife		ArvGcSwissKnife;
typedef struct _ArvGcConverter		ArvGcConverter;
typedef struct _ArvGcPort		ArvGcPort;

typedef struct _ArvGcInteger		ArvGcInteger;
typedef struct _ArvGcFloat		ArvGcFloat;
typedef struct _ArvGcString		ArvGcString;

typedef struct _ArvInterface 		ArvInterface;
typedef struct _ArvDevice 		ArvDevice;
typedef struct _ArvStream 		ArvStream;
typedef struct _ArvBuffer 		ArvBuffer;

typedef struct _ArvGvInterface 		ArvGvInterface;
typedef struct _ArvGvDevice 		ArvGvDevice;
typedef struct _ArvGvStream 		ArvGvStream;

typedef struct _ArvFakeDevice		ArvFakeDevice;
typedef struct _ArvFakeStream		ArvFakeStream;

typedef struct _ArvZip			ArvZip;
typedef struct _ArvZipFile 		ArvZipFile;

G_END_DECLS

#endif
