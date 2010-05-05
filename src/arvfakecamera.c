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

#include <arvfakecamera.h>
#include <arvgvcp.h>
#include <arvdebug.h>
#include <string.h>

static GObjectClass *parent_class = NULL;

struct _ArvFakeCameraPrivate {
	void *memory;
};

/* ArvFakeCamera implementation */

const char *
arv_get_fake_camera_genicam_data (size_t *size)
{
	static GMappedFile *genicam_file = NULL;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);

	if (genicam_file == NULL ) {
		char *filename;

		filename = g_build_filename (ARAVIS_DATA_DIR, "arv-fake-camera.xml", NULL);
		genicam_file = g_mapped_file_new (filename, FALSE, NULL);

		if (genicam_file != NULL) {
			arv_debug ("fakegenicam", "[get_fake_camera_genicam_data] %s [size = %d]",
				   filename,
				   g_mapped_file_get_length (genicam_file));
			arv_debug ("fakegenicam", g_mapped_file_get_contents (genicam_file));
		}

		g_free (filename);
	}

	g_static_mutex_unlock (&mutex);

	g_return_val_if_fail( genicam_file != NULL, NULL);

	if (size != NULL)
		*size = g_mapped_file_get_length (genicam_file);

	return g_mapped_file_get_contents (genicam_file);
}

/* GObject implemenation */

ArvFakeCamera *
arv_fake_camera_new (void)
{
	ArvFakeCamera *fake_camera;
	void *memory;
	char *xml_url;
	size_t size;

	fake_camera = g_object_new (ARV_TYPE_FAKE_CAMERA, NULL);

	memory = g_malloc0 (ARV_FAKE_CAMERA_MEMORY_SIZE);

	strcpy (memory + ARV_GVBS_MANUFACTURER_NAME, "Aravis");
	strcpy (memory + ARV_GVBS_MODEL_NAME, "Fake");
	strcpy (memory + ARV_GVBS_DEVICE_VERSION, PACKAGE_VERSION);
	strcpy (memory + ARV_GVBS_SERIAL_NUMBER, "0");

	arv_get_fake_camera_genicam_data (&size);
	xml_url = g_strdup_printf ("Local:arv-fake-camera-%s.xml;%x;%x",
				   PACKAGE_VERSION,
				   ARV_FAKE_CAMERA_MEMORY_SIZE, size);
	strcpy (memory + ARV_GVBS_FIRST_XML_URL, xml_url);
	g_free (xml_url);

	fake_camera->priv->memory = g_malloc0 (ARV_FAKE_CAMERA_MEMORY_SIZE);

	return fake_camera;
}

static void
arv_fake_camera_init (ArvFakeCamera *fake_camera)
{
	fake_camera->priv = G_TYPE_INSTANCE_GET_PRIVATE (fake_camera, ARV_TYPE_FAKE_CAMERA, ArvFakeCameraPrivate);
}

static void
arv_fake_camera_finalize (GObject *object)
{
	ArvFakeCamera *fake_camera = ARV_FAKE_CAMERA (object);

	g_free (fake_camera->priv->memory);

	parent_class->finalize (object);
}

static void
arv_fake_camera_class_init (ArvFakeCameraClass *fake_camera_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (fake_camera_class);

	g_type_class_add_private (fake_camera_class, sizeof (ArvFakeCameraPrivate));

	parent_class = g_type_class_peek_parent (fake_camera_class);

	object_class->finalize = arv_fake_camera_finalize;
}

G_DEFINE_TYPE (ArvFakeCamera, arv_fake_camera, G_TYPE_OBJECT)
